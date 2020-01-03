#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ql_oe.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utils.h"
#include "mcu_data.h"
#include "niu_data.h"
#include "niu_packet.h"
#include "ql_cmdq.h"
#include "service.h"
#include "ql_mem.h"
#include "niu_fota.h"
#include "niu_device.h"
#include "ql_common.h"


#define _MCU_CRC8_CHECK_
#define CHECK_UPGRADE_TIMEOUT (10*1000)
#define MCU_UPGRADE_RETRY       (5)

int g_mcu_fd = -1;
int g_mcu_upgrade_flag = 0;
/* used to fota upgrade */
struct mcu_file_trans g_fota_upgrade_stm32;
struct mcu_file_trans g_fota_upgrade_ec25;
struct mcu_file_trans g_config_upgrade_ec25;

ql_value_name_item_t g_niu_vn_device_type[] = {
    {NIU_DEVICE_ID_DB, "DB"},
    {NIU_DEVICE_ID_FOC, "FOC"},
    {NIU_DEVICE_ID_BMS, "BMS"},
    {NIU_DEVICE_ID_BMS2, "BMS2"},
    {NIU_DEVICE_ID_ALARM, "ALARM"},
    {NIU_DEVICE_ID_LCU, "LCU"},
    {NIU_DEVICE_ID_QC, "QC"},
    {NIU_DEVICE_ID_LKU, "LKU"},
    {NIU_DEVICE_ID_BCS, "BCS"},
    {NIU_DEVICE_ID_EMCU, "EMCU"},
    {-1, NULL}
};

int mcu_send(int fd, NUINT8* buff, int len);
int mcu_upgrade_stm32_package();

/**
 * check mcu upgrade timeout. while mcu upgrade, there is something 
 * write in mcu_file_trans struct and file maybe opened. if 10 mins
 * ago, the upgrade process is not complete, we close the file descriptor
 * and reset mcu_file_trans struct.
 * 
 */
void mcu_upgrade_timeout_check(int tmr_id, void *param)
{
    NUINT32 now = niu_device_get_timestamp();
    LOG_D("mcu_upgrade_timeout_check!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    if(tmr_id != TIMER_ID_MCU_FOTA_TIMEOUT_CHECK)
    {
        LOG_E("tmr_id is wrong, expect %d, real is %d", TIMER_ID_MCU_FOTA_TIMEOUT_CHECK, tmr_id);
        return;
    }
    if(g_fota_upgrade_stm32.fsize > 0 || g_fota_upgrade_stm32.is_start == 1)
    {
        if(now - g_fota_upgrade_stm32.last_time > 60)
        {
            if(g_fota_upgrade_stm32.fd > 0)
            {
                close(g_fota_upgrade_stm32.fd);
            }
            memset(&g_fota_upgrade_stm32, 0, sizeof(struct mcu_file_trans));
            g_mcu_upgrade_flag = 0;
        }
    }
    if(g_fota_upgrade_ec25.fsize > 0 || g_fota_upgrade_ec25.is_start == 1)
    {
        if(now - g_fota_upgrade_ec25.last_time > 60)
        {
            if(g_fota_upgrade_ec25.fd > 0)
            {
                close(g_fota_upgrade_ec25.fd);
            }
            memset(&g_fota_upgrade_ec25, 0, sizeof(struct mcu_file_trans));
            g_mcu_upgrade_flag = 0;
        }
    }
    if(g_config_upgrade_ec25.fsize > 0 || g_config_upgrade_ec25.is_start == 1)
    {
        if(now - g_config_upgrade_ec25.last_time > 60)
        {
            if(g_config_upgrade_ec25.fd > 0)
            {
                close(g_config_upgrade_ec25.fd);
            }
            memset(&g_config_upgrade_ec25, 0, sizeof(struct mcu_file_trans));
            g_mcu_upgrade_flag = 0;
        }
    }
    tmr_handler_tmr_add(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK, mcu_upgrade_timeout_check, NULL, CHECK_UPGRADE_TIMEOUT, 0);
    return;
}


int mcu_send_write(NUINT8 base, NUINT16 id, NUINT8 *value, NUINT32 len)
{
    int size, pos;
    NUINT8 *datazone = NULL;
    NUINT8 *pkt_buff = NULL;
    NIU_DATA_VALUE *niu_data = NULL;
    NUINT32 pkt_len = 0;

    if(value == NULL || len == 0)
    {
        LOG_E("value or len error.[value: %p, len: %d]", value, len);
        return -1;
    }

    niu_data = NIU_DATA(base, id);
    if(niu_data)
    {
        if(niu_data->len != len)
        {
            LOG_E("length error, base: 0x%02X, id: 0x%04X, len: %d, niu_data_len: %d", base, id, len, niu_data->len);
            return -1;
        }
        else
        {
            if(memcmp(niu_data->addr, value, len) == 0)
            {
                LOG_V("value not change, base: 0x%02X, id: 0x%04X", base, id);
                return 0;
            }
        }
    }

    size = 3 + len;

    datazone = QL_MEM_ALLOC(size);
    if(datazone == NULL)
    {
        LOG_E("malloc error.[size: %d]", size);
        return -1;
    }
    pos = 0;
    memcpy(datazone + pos, &base, sizeof(NUINT8));
    pos += sizeof(NUINT8);
    memcpy(datazone + pos, &id, sizeof(NUINT16));
    pos += sizeof(NUINT16);
    memcpy(datazone + pos, value, len);
    pos += len;


    pkt_buff = generate_mcu_packet(SERIAL_COMMAND_WRITE, 0, datazone, pos, &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate packet error.");
        goto _error;
    }
    QL_MEM_FREE(datazone);
    datazone = NULL;
    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);

_error:
    if(datazone)
    {
        QL_MEM_FREE(datazone);
        datazone = NULL;
    }

    if(pkt_buff)
    {
        QL_MEM_FREE(pkt_buff);
        pkt_buff = NULL;
    }

    return -1;
}

/**
 * send read command to mcu
 * @base: device type in v3table
 * @id:   device id  in v3table
 * 
 * @return:
 *      if success, return 0, else return -1
 */ 
int mcu_send_read(NUINT8 base, NUINT16 id)
{
    NUINT8 datazone[3] = {0};
    NUINT8 *pkt_buff = NULL;
    NUINT32 pkt_len = 0;

    memcpy(datazone, &base, sizeof(NUINT8));
    memcpy(datazone + 1, &id, sizeof(NUINT16));

    pkt_buff = generate_mcu_packet(SERIAL_COMMAND_READ, 0, datazone, 3, &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate packet error.");
        goto _error;
    }
    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);
_error:
    if(pkt_buff)
    {
        QL_MEM_FREE(pkt_buff);
        pkt_buff = NULL;
    }
    return -1;
}

/**
 * send read command to mcu
 * @base: device type in v3table
 * @id:   device id  in v3table
 * 
 * @return:
 *      if success, return 0, else return -1
 */ 
int mcu_send_seqread(NUINT8 base, NUINT16 id, NUINT8 count)
{
    NUINT8 datazone[3] = {0};
    NUINT8 *pkt_buff = NULL;
    NUINT32 pkt_len = 0;

    memcpy(datazone, &base, sizeof(NUINT8));
    memcpy(datazone + 1, &id, sizeof(NUINT16));
    memcpy(datazone + 3, &count, sizeof(NUINT8));

    pkt_buff = generate_mcu_packet(SERIAL_COMMAND_SRAED, 0, datazone, 4, &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate packet error.");
        goto _error;
    }
    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);
_error:
    if(pkt_buff)
    {
        QL_MEM_FREE(pkt_buff);
        pkt_buff = NULL;
    }
    return -1;
}
/**
 * send data to mcu by UART
 * @fd:   UART open fd
 * @buff: buff want to send, must be malloc
 * @len:  buff len
 * 
 * @return:
 *      if success, return 0, else return -1
 */ 
int mcu_send(int fd, NUINT8* buff, int len)
{
    struct frame_packet *sp = NULL;

    if(fd <= 0 || buff == NULL || len <= 0)
    {
        LOG_E("mcu send param error.");
        return -1;
    }
    
    sp = QL_MEM_ALLOC(sizeof(struct frame_packet));
    if(sp == NULL)
    {
        LOG_E("malloc error.");
        goto _error;
    }
    sp->fd = fd;
    sp->buff = buff;
    sp->len = len;
    
    if(0 != cmd_handler_push(send_packet_handle, (void*)sp))
    {
        LOG_E("cmd_handler_push error.");
        goto _error;
    }

    return 0;

_error:
    if(sp)
    {
        if(sp->buff)
        {
            QL_MEM_FREE(sp->buff);
            sp->buff = NULL;
        }
        QL_MEM_FREE(sp);
        sp = NULL;
    }
    return -1;
}

/**
 * send file verify crc16 to stm32 while package is send over.
 * @cmd: mcu cmd define in v3
 * @resp: if set must be SERIAL_RESP_TYPE_NORMAL or SERIAL_RESP_TYPE_ERROR, otherwhise set 0
 * 
 * 
 */ 
int mcu_send_file_verify(NUINT8 cmd, NUINT8 resp, NUINT16 verify)
{
    NUINT8 *pkt_buff = NULL;
    NUINT32 pkt_len = 0;
    NUINT16 verify2 = 0;

    if(verify == 0 || cmd == 0)
    {
        LOG_E("verify or cmd is wrong.");
        return -1;
    }

    if(resp > 0)
    {
        if(resp != SERIAL_RESP_TYPE_NORMAL && resp != SERIAL_RESP_TYPE_ERROR)
        {
            LOG_E("resp error.[resp=%d]", resp);
            return -1;
        }
    }
    verify2 = verify;
    pkt_buff = generate_mcu_packet(cmd, resp, (NUINT8*)&verify2, sizeof(NUINT16), &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate_mcu_packet error.[cmd: %d, resp: %d, verify: %d]", cmd, resp, verify);
        return -1;
    }

    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);
}

int mcu_send_file_content(struct mcu_file_trans *file_trans, NUINT8 cmd, NUINT8 resp)
{
    NUINT8 *pkt_buff = NULL, *buff = NULL;
    NUINT32 pkt_len = 0;
    FILE *fp = NULL;
    int len = 0, ret = 0;

    if(file_trans == NULL)
    {
        LOG_E("file_trans is null.");
        return -1;
    }

    if(ql_com_file_exist(file_trans->fname))
    {
        LOG_E("fname is not exist[%s].", file_trans->fname);
        return -1;
    }

    if(file_trans->fsize < 0)
    {
        LOG_E("fsize error[%d].", file_trans->fsize);
        return -1;
    }

    if(file_trans->deal_size >= file_trans->fsize)
    {
        LOG_E("deal_size: %d, fsize: %d, file is send over.", file_trans->deal_size, file_trans->fsize);
        return -1;
    }
    
    if(file_trans->fsize - file_trans->deal_size > file_trans->package_size)
    {
        len = file_trans->package_size;
    }
    else
    {
        len = file_trans->fsize - file_trans->deal_size;
    }
    LOG_D("fsize: %d, deal_size: %d, len: %d", file_trans->fsize, file_trans->deal_size, len);
    fp = fopen(file_trans->fname, "r");
    if(fp == NULL)
    {
        LOG_E("fopen error[%s].", file_trans->fname);
        goto _error;
    }

    buff = QL_MEM_ALLOC(len + 4);
    if(buff == NULL)
    {
        LOG_E("malloc error[%d].", len + 2);
        goto _error;
    }

    fseek(fp, file_trans->deal_size, SEEK_SET);

    ret = fread(buff + 4, 1, len, fp);
    if(ret != len)
    {
        LOG_E("fread error[len=%d, ret=%d].", len, ret);
        goto _error;
    }

    memcpy(buff, (NUINT8*)&file_trans->package_index, sizeof(NUINT32));

    pkt_buff = generate_mcu_packet(cmd, 0, buff, len + 4, &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate_mcu_packet error[cmd=%d, len=%d].", cmd, len + 4);
        goto _error;
    }

    fclose(fp);
    if(buff != NULL)
    {
        QL_MEM_FREE(buff);
        buff = NULL;
    }
    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);

_error:
    if(fp)
    {
        fclose(fp);
    }
    if(buff != NULL)
    {
        QL_MEM_FREE(buff);
        buff = NULL;
    }
    return -1;
}

int mcu_send_file_start(struct mcu_file_trans *file_trans, NUINT8 cmd, NUINT8 resp)
{
    NUINT8 *pkt_buff = NULL;
    NUINT32 pkt_len = 0;
    NUINT8 buff[6];

    if(file_trans == NULL)
    {
        LOG_E("file_trans is null.");
        return -1;
    }

    if(ql_com_file_exist(file_trans->fname))
    {
        LOG_E("mcu_file_trans fname is not exist[%s].", file_trans->fname);
        return -1;
    }

    if(file_trans->fsize <= 0)
    {
        LOG_E("mcu_file_trans fsize error[%d].", file_trans->fsize);
        return -1;
    }
    memcpy(buff, (NUINT8*)&file_trans->fsize, sizeof(file_trans->fsize));
    memcpy(buff + sizeof(file_trans->fsize), (NUINT8*)&file_trans->device_id, sizeof(file_trans->device_id));
    pkt_buff = generate_mcu_packet(cmd, resp, buff, sizeof(buff), &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate_mcu_packet error[cmd: %d].", cmd);
        return -1;
    }
    
    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);
}

static int mcu_command_read_process(NUINT8 *buff, int len)
{
    int size, pos, index, ret = 0;
    NUINT8 *datazone = NULL, *pkt_buff = NULL;
    NUINT32 pkt_len = 0;
    NIU_DATA_VALUE *value;
    NUINT8 base;

    if(buff == NULL || len < COMMAND_READ_LENGTH_MIN)
    {
        LOG_E("buff or len error.[buff: %p, len: %d]", buff, len);
        ret = SERIAL_PACKET_OTHER_ERROR;
        goto _error;
    }

    if(len % COMMAND_READ_LENGTH_MIN != 0)
    {
        ret = SERIAL_PACKET_ADDRESS_ERROR;
        goto _error;
    }

    pos = 0;
    size = 0;
    //calc buffer size
    while(pos < len)
    {
        base = buff[pos];

        value = niu_data_read(base, *((NUINT16*)(buff + pos + 1)));
        if(value == NULL)
        {
            LOG_E("v3 data table read error[base: 0x%02X, id: 0x%04X].", buff[pos], *((NUINT16*)(buff + pos + 1)));
            ret = SERIAL_PACKET_ADDRESS_ERROR;
            goto _error;
        }
        size += value->len;
        pos += COMMAND_READ_LENGTH_MIN;
    }

    if(size == 0)
    {
        LOG_E("buff or len error[buff: %p, len: %d].", buff, len);
        ret = SERIAL_PACKET_OTHER_ERROR;
        goto _error;
    }
    datazone = QL_MEM_ALLOC(size + len);
    if(datazone == NULL)
    {
        LOG_E("alloc error[len: %d].", size);
        ret = SERIAL_PACKET_OTHER_ERROR;
        goto _error;
    }

    pos = 0;
    index = 0;
    while(pos < len)
    {
        base = buff[pos];
        NUINT16 field_id = *((NUINT16*)(buff + pos + 1));
        if(base == NIU_ECU && field_id == NIU_ID_ECU_ECU_SOFT_VER)
        {
            LOG_D("===read, baseid: %d, id: %d\n", NIU_ECU, field_id);
        }
        value = niu_data_read(base, *((NUINT16*)(buff + pos + 1)));
        if(value == NULL)
        {
            ret = SERIAL_PACKET_ADDRESS_ERROR;
            goto _error;
        }
        //set reply data
        memcpy(datazone + index, buff + pos, sizeof(NUINT8));
        index += sizeof(NUINT8);
        memcpy(datazone + index, buff + pos + 1, sizeof(NUINT16));
        index += sizeof(NUINT16);
        memcpy(datazone + index, value->addr, value->len);
        index += value->len;
        pos += COMMAND_READ_LENGTH_MIN;
    }

    pkt_buff = generate_mcu_packet(SERIAL_COMMAND_WRITE, 0, datazone, size + len, &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate packet error.");
        ret = SERIAL_PACKET_OTHER_ERROR;
        goto _error;
    }
    if(datazone)
    {
        QL_MEM_FREE(datazone);
        datazone = NULL;
    }
    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);
_error:
    if(datazone)
    {
        QL_MEM_FREE(datazone);
        datazone = NULL;
    }
    return ret;
}



static int mcu_command_seqread_process(NUINT8 *buff, int len)
{
    int ret, pos, i, size;
    NUINT8 baseid, field_count;
    NUINT8 *datazone = NULL;
    NUINT16 field_addr;
    NUINT8 *pkt_buff = NULL;
    NUINT32 pkt_len = 0;
    NIU_DATA_VALUE *value = NULL;

    if(buff == NULL || len != COMMAND_SEQREAD_LENGTH_MIN)
    {
        LOG_E("buff or len error[buff: %p, len: %d].", buff, len);
        return SERIAL_PACKET_OTHER_ERROR;
    }

    pos = 0;
    memcpy(&baseid, buff + pos, sizeof(NUINT8));
    pos += sizeof(NUINT8);
    memcpy((NUINT8*)&field_addr, buff + pos, sizeof(NUINT16));
    pos += sizeof(NUINT16);
    memcpy(&field_count, buff + pos, sizeof(NUINT8));
    pos += sizeof(NUINT8);

    size = 0;
    for(i = 0; i < field_count; i++)
    {
        value = niu_data_read(baseid, field_addr + i);
        if(value != NULL)
        {
            size += value->len;
        }
        else
        {
            LOG_E("address error: baseid 0x%02X, field_addr: 0x%04X", baseid, field_addr + i);
            ret = SERIAL_PACKET_ADDRESS_ERROR;
            goto _error;
        }
    }

    if(size == 0)
    {
        LOG_E("buff or len error.[buff: %p, len: %d]", buff, len);
        ret = SERIAL_PACKET_OTHER_ERROR;
        goto _error;
    }

    datazone = QL_MEM_ALLOC(size + len);
    if(datazone == NULL)
    {
        LOG_E("alloc error[len: %d]", size);
        ret = SERIAL_PACKET_OTHER_ERROR;
        goto _error;
    }

    pos = 0;
    memcpy(datazone + pos, buff, len);
    pos += len;
    for(i = 0; i < field_count; i++)
    {
        if(baseid == NIU_ECU && field_addr + i == NIU_ID_ECU_ECU_SOFT_VER)
        {
            LOG_D("===seqread, baseid: %d, id: %d\n", NIU_ECU, field_addr + i);
        }
        value = niu_data_read(baseid, field_addr + i);
        if(value == NULL)
        {
            ret = SERIAL_PACKET_ADDRESS_ERROR;
            goto _error;
        }
        //set reply data
        memcpy(datazone + pos, value->addr, value->len);
        pos += value->len;
    }
 
   
    pkt_buff = generate_mcu_packet(SERIAL_COMMAND_SWRITE, 0, datazone, size + len, &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate packet error.");
        ret = SERIAL_PACKET_OTHER_ERROR;
        goto _error;
    }

    if(datazone)
    {
        QL_MEM_FREE(datazone);
    }
    //niu_trace_dump_hex("swrite", pkt_buff, pkt_len);
    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);

_error:
    if(datazone)
    {
        QL_MEM_FREE(datazone);
    }
    return ret;

}


/**
 * seq read (request):
 * ------------------------------------------------------------------------
    |   1   |        2        |   1    |
 * ------------------------------------------------------------------------
 *     dev          field         count
 * seq write (response):
 * ------------------------------------------------------------------------
 * |    1   |        2        |   1  |    data   |        data        |   data  | 
 * ------------------------------------------------------------------------
 *      dev          field      count  .....        
 **/
int mcu_command_seqwrite_process(NUINT8 *buff, NUINT16 len)
{
    int ret = SERIAL_PACKET_PPOCESS_OK;
    int pos;
    NUINT8 baseid, field_count;
    NUINT16 field_addr, i;
    NIU_DATA_VALUE *value;

    if(buff == NULL || len < COMMAND_SEQWRITE_LENGTH_MIN)
    {
        LOG_E("buff or len error[buff: %p, len: %d]", buff, len);
        return SERIAL_PACKET_OTHER_ERROR;
    }
    
    pos = 0;
    memcpy(&baseid, buff + pos, sizeof(NUINT8));
    pos += sizeof(NUINT8);
    memcpy((NUINT8*)&field_addr, buff + pos, sizeof(NUINT16));
    pos += sizeof(NUINT16);
    memcpy(&field_count, buff + pos, sizeof(NUINT8));
    pos += sizeof(NUINT8);

    for(i = 0; i < field_count; i++)
    {
        value = niu_data_read(baseid, field_addr + i);
        if(value == NULL)
        {
            LOG_E("data read error[baseid 0x%02X, addr: 0x%04X]", baseid, field_addr + i);
            ret = SERIAL_PACKET_ADDRESS_ERROR;
            break;
        }
        if(pos + value->len <= len)
        {
            if( JFALSE == niu_data_write_value_from_stm32(baseid, field_addr + i, buff + pos, value->len))
            {
                ret = SERIAL_PACKET_DATA_ERROR;
                LOG_E("set data error[baseid 0x%02X, addr: 0x%04X, data_len: %d, give_len: %d]", 
                            baseid, field_addr + i, value->len, len - pos);
                break;
            }
        }
        else
        {
            LOG_E("set data error[baseid 0x%02X, addr: 0x%04X, data_len: %d, give_len: %d]", 
                            baseid, field_addr + i, value->len, len - pos);
            ret = SERIAL_PACKET_DATA_ERROR;
            break;
        }
        pos += value->len;
    }

    return ret;
}

int mcu_command_write_process(NUINT8 *buff, NUINT16 len)
{
    int ret = SERIAL_PACKET_PPOCESS_OK;
    int pos;
    NUINT8 baseid;
    NUINT16 field_addr;
    NIU_DATA_VALUE *value;

    if(buff == NULL || len < COMMAND_WRITE_LENGTH_MIN)
    {
        LOG_E("buff or len error[buff: %p, len: %d]", buff, len);
        return SERIAL_PACKET_OTHER_ERROR;
    }
    pos = 0;

    while(pos < len)
    {
        if(pos + sizeof(NUINT8) > len)
        {
            LOG_E("address parse error[pos:%d, len:%d]", pos, len);
            ret = SERIAL_PACKET_ADDRESS_ERROR;
            break;
        }
        memcpy(&baseid, buff + pos, sizeof(NUINT8));
        pos += sizeof(NUINT8);
        if(pos + sizeof(NUINT16) > len)
        {
            LOG_E("address parse error[pos:%d, len:%d]", pos, len);
            ret = SERIAL_PACKET_ADDRESS_ERROR;
            break;
        }
        memcpy((NUINT8*)&field_addr, buff + pos, sizeof(NUINT16));
        pos += sizeof(NUINT16);

        value = niu_data_read(baseid, field_addr);
        if(value == NULL)
        {
            LOG_E("data read error[baseid 0x%02X, addr: 0x%04X]", baseid, field_addr);
            ret = SERIAL_PACKET_ADDRESS_ERROR;
            break;
        }
        if(pos + value->len > len)
        {
            LOG_E("set data error[baseid 0x%02X, addr: 0x%04X, data_len: %d, give_len: %d]", 
                            baseid, field_addr, value->len, len - pos);
            ret = SERIAL_PACKET_DATA_ERROR;
            break;
        }

        if( JFALSE == niu_data_write_value_from_stm32(baseid, field_addr, buff + pos, value->len))
        {
            LOG_E("niu_data_write_value_from_stm32 baseid: %02x, id: %04x failed.", baseid, field_addr);
            ret = SERIAL_PACKET_DATA_ERROR;
            break;
        }
        
        pos += value->len;
    }

    return ret;
}

/**
 *  reply error message
 *  when packet is error.and command type is not read or seq read.
 * @opt_orig:   [input] orignal opt
 * @error_type: [input] error type
 * 
 * @return:
 */

int mcu_send_reply(NUINT8 cmd, NUINT8 resp, NUINT8 error_type)
{
    NUINT8 datazone;
    NUINT8 *pkt_buff = NULL;
    NUINT32 pkt_len = 0;

    if(resp != SERIAL_RESP_TYPE_NORMAL && resp != SERIAL_RESP_TYPE_ERROR)
    {
        LOG_E("resp: 0x%02X, not define.", resp);
        return -1;
    }

    datazone = error_type;

    pkt_buff = generate_mcu_packet(cmd, resp, &datazone, 1, &pkt_len);
    if(pkt_buff == NULL || pkt_len == 0)
    {
        LOG_E("generate packet error.");
        return -1;
    }
    return mcu_send(g_mcu_fd, pkt_buff, pkt_len);
}


// request process
static int mcu_command_process(NUINT8 *buff, NUINT16 pkt_len)
{
    int ret = SERIAL_PACKET_PPOCESS_OK, data_len;
    NUINT8 *p;
    NUINT8 opt;

    if(buff == NULL || pkt_len < SERIAL_PACKET_LENGTH_MIN)
    {
        LOG_E("buff or len error[buff: %p, len %d]", buff, pkt_len);
        return SERIAL_PACKET_OTHER_ERROR;
    }
    p = buff + V35_PACKET_OFFSET_DATA;
    data_len = PACKET_DATA_LENGTH(pkt_len);
    opt = *(buff + V35_PACKET_OFFSET_OPT);

    NUINT8 cmd_type = opt & SERIAL_COMMAND_MASK;
    NUINT8 resp_type = opt & SERIAL_RESP_TYPE_MASK;
    //LOG_D("recv mcu cmd: 0x%02X, len: %d", cmd_type, pkt_len);
    switch(cmd_type)
    {
        case SERIAL_COMMAND_SRAED:
        {
            //need response
            // total 4 bytes
            // base_addr + field_addr + fieldcount
            //    1 byte      2bytes         1byte
            if(resp_type != 0)
            {
                LOG_E("opt: 0x%02X, cmd: 0x%02X, resp: 0x%02X", opt, cmd_type, resp_type);
                return SERIAL_PACKET_COMMAND_ERROR;
            }
            //niu_trace_dump_hex("sread", buff, pkt_len);
            ret = mcu_command_seqread_process(p, data_len);
            if(ret != SERIAL_PACKET_PPOCESS_OK)
            {
                LOG_E("cmd: 0x%02X, failed!", cmd_type);
            }
        }
        break;
        case SERIAL_COMMAND_SWRITE:
        {
            if(resp_type == SERIAL_RESP_TYPE_ERROR)
            {
                //resp error.
                NUINT8 err = *(p);
                LOG_D("opt: 0x%02X, cmd: 0x%02X, resp: 0x%02X", opt, cmd_type, resp_type);
                LOG_D("recv a error response: 0x%02X", err);
            }
            else
            {
                ret = mcu_command_seqwrite_process(p, data_len);
                if(ret != SERIAL_PACKET_PPOCESS_OK)
                {
                    LOG_E("cmd: 0x%02X, failed!", cmd_type);
                }
            }
        }
        break;
        case SERIAL_COMMAND_READ:
        {
            //need response
            if(resp_type != 0)
            {
                LOG_E("opt: 0x%02X, cmd: 0x%02X, resp: 0x%02X", opt, cmd_type, resp_type);
                return SERIAL_PACKET_COMMAND_ERROR;
            }

            ret = mcu_command_read_process(p, data_len);
            if(ret != SERIAL_PACKET_PPOCESS_OK)
            {
                LOG_E("cmd: 0x%02X, failed!", cmd_type);
            }
        }
        break;
        case SERIAL_COMMAND_WRITE:
        {
            if(resp_type == SERIAL_RESP_TYPE_ERROR)
            {
                //resp error.
                NUINT8 err = *(p);
                LOG_D("opt: 0x%02X, cmd: 0x%02X, resp: 0x%02X", opt, cmd_type, resp_type);
                LOG_D("recv a error response: 0x%02X", err);
            }
            else// if(resp_type == SERIAL_RESP_TYPE_NORMAL)
            {
                ret = mcu_command_write_process(p, data_len);
                if(ret != SERIAL_PACKET_PPOCESS_OK)
                {
                    LOG_E("cmd: 0x%02X, failed!", cmd_type);
                }
            }
        }
        break;
        /**
         * after SERIAL_COMMAND_FOTA_START, resp_type is valid.
         * resp_type must be set 0 in request, and must be set SERIAL_RESP_TYPE_ERROR
         * or SERIAL_RESP_TYPE_NORMAL in response.
         */
        case SERIAL_COMMAND_FOTA_START:
        {
            LOG_D("------------SERIAL_COMMAND_FOTA_START--resp=%d----------", resp_type);
            if(resp_type != 0)
            {
                //ec25 upgrade stm32
                NUINT8 err = *(NUINT8*)p;
                if(resp_type == SERIAL_RESP_TYPE_NORMAL && err == 0)
                {
                    //success, push fota_package event
                    LOG_D("------------success, send fota_package------------");
                    if(tmr_handler_tmr_is_exist(TIMER_ID_FOTA_UPGRADE_START))
                    {
                        LOG_D("------------stop time-TIMER_ID_FOTA_UPGRADE_START------------");
                        tmr_handler_tmr_del(TIMER_ID_FOTA_UPGRADE_START);
                    }
                    g_fota_upgrade_stm32.is_start = 1;
                    g_fota_upgrade_stm32.retry = 0;
                    g_fota_upgrade_stm32.last_time = niu_device_get_timestamp();
                    sys_event_push(FOTA_EVENT_UPGRADE_EC25_TO_STM32_RATE, NULL, 0);
                    mcu_upgrade_stm32_package();
                }
                else
                {
                    LOG_D("------recv fota_start response error. resp_type: %d, err: %d, retry: %d", 
                                                        resp_type, err, g_fota_upgrade_stm32.retry);
                }
            }
            else if(resp_type == 0)
            {
                /*if resp type is 0, this means stm32 send upgrade request to ec25.
                * we must send response to stm32, resp_normal or resp_error,
                * and then set package index = 1, and open file fd used to write package data
                */
                NUINT8 reply_code = 0;
                NUINT8 reply_resp = 0;
                NUINT32 size = *(NUINT32*)p;

                if(g_mcu_upgrade_flag == 1)
                {
                    reply_code = MCU_PACKET_ERRCODE_COMMAND;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                }
                else if(size == 0 || size > MAX_UPGRADE_FILE_SIZE)
                {
                    LOG_E("fota upgrade start, size error[size=%u].", size);
                    reply_code = MCU_PACKET_ERRCODE_DATA;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                }
                else
                {
                    if(ql_com_file_exist(FOTA_UPGRADE_PATH))
                    {
                        LOG_D("%s is not exist.", FOTA_UPGRADE_PATH);
                        ret = mkdir(FOTA_UPGRADE_PATH, 0777);
                        if(ret)
                        {
                            LOG_E("mkdir error.[%s]", FOTA_UPGRADE_PATH);
                        }
                    }
                    else
                    {
                        if(!ql_com_file_exist(FOTA_UPGRADE_FILE_EC25))
                        {
                            LOG_D("file %s is exist, del it", FOTA_UPGRADE_FILE_EC25);
                            unlink(FOTA_UPGRADE_FILE_EC25);
                        }
                    }
                    g_fota_upgrade_ec25.fd = open(FOTA_UPGRADE_FILE_EC25, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IXOTH);
                    if(g_fota_upgrade_ec25.fd > 0)
                    {
                        reply_code = 0;
                        reply_resp = SERIAL_RESP_TYPE_NORMAL;

                        g_fota_upgrade_ec25.fsize = size;
                        g_fota_upgrade_ec25.is_start = 1;
                        g_fota_upgrade_ec25.package_index = 1;
                        g_fota_upgrade_ec25.deal_size = 0;
                        g_fota_upgrade_ec25.last_time = niu_device_get_timestamp();
                        g_mcu_upgrade_flag = 1;
                        //start upgrade check, 10 minutes timeout
                        // if(0 != tmr_handler_tmr_add(TIMER_ID_FOTA_UPGRADE_EC25_CHECK, mcu_upgrade_check_timeout, &g_fota_upgrade_ec25, CHECK_UPGRADE_TIMEOUT, 0))
                        // {
                        //     LOG_E("tmr handler add timer id=%d error", TIMER_ID_FOTA_UPGRADE_EC25_CHECK);
                        // }
                        if(!tmr_handler_tmr_is_exist(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK))
                        {
                            if(tmr_handler_tmr_add(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK, mcu_upgrade_timeout_check, NULL, CHECK_UPGRADE_TIMEOUT, 0) != 0)
                            {
                                LOG_E("tmr handler add timer id=%d error", TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);
                            }
                        }
                    }
                    else
                    {
                        reply_code = MCU_PACKET_ERRCODE_DATA;
                        reply_resp = SERIAL_RESP_TYPE_ERROR;
                    }
                }

                ret = mcu_send_reply(cmd_type, reply_resp, reply_code);
                if(ret != SERIAL_PACKET_PPOCESS_OK)
                {
                    LOG_E("cmd: 0x%02X, failed!", cmd_type);
                }
                
            }
        }
        break;
        case SERIAL_COMMAND_FOTA_PACKAGE:
        {
            //need response
            LOG_D("------------resp SERIAL_COMMAND_FOTA_PACKAGE------------");
            NUINT8 err = *(NUINT8*)p;
            if(resp_type != 0)
            {
                //ec25 upgrade stm32
                if(resp_type == SERIAL_RESP_TYPE_NORMAL && err == 0)
                {
                    //success, push fota_package event
                    int len = g_fota_upgrade_stm32.fsize - g_fota_upgrade_stm32.deal_size;
                    if(len > FOTA_UPGRADE_PACKAGE_SIZE)
                    {
                        g_fota_upgrade_stm32.deal_size += g_fota_upgrade_stm32.package_size;
                    }
                    else
                    {
                        g_fota_upgrade_stm32.deal_size += len;
                    }
                    g_fota_upgrade_stm32.package_index += 1;
                    g_fota_upgrade_stm32.retry = 0;
                    g_fota_upgrade_stm32.last_time = niu_device_get_timestamp();
                    // push event to update upgrade percent.
                    NUINT16 *percent = QL_MEM_ALLOC(sizeof(NUINT16));
                    if(percent != NULL && g_fota_upgrade_stm32.fsize > 0)
                    {
                        *percent = (NUINT16)(g_fota_upgrade_stm32.deal_size * 100 / g_fota_upgrade_stm32.fsize );
                        sys_event_push(FOTA_EVENT_UPGRADE_EC25_TO_STM32_RATE, percent, sizeof(NUINT16));
                    }
                    mcu_upgrade_stm32_package();
                }
                else
                {
                    //failed, retry send
                    if(g_fota_upgrade_stm32.retry < MCU_UPGRADE_RETRY)
                    {
                        g_fota_upgrade_stm32.retry += 1;
                        g_fota_upgrade_stm32.last_time = niu_device_get_timestamp();
                        mcu_upgrade_stm32_package();
                    }
                    else
                    {
                        LOG_E("upgrade to stm32 package, retry 3 times, exit upgrade.");
                    }
                }
            }
            else if(resp_type == 0)
            {
                //stm32 upgrade ec25
                NUINT8 reply_code = 0;
                NUINT8 reply_resp = 0;
                NUINT32 package_index = *(NUINT32*)p;

                LOG_D("fota upgrade from stm32, [index=%u, local=%u].", package_index, g_fota_upgrade_ec25.package_index);
                if(package_index == 0 || 
                        (package_index != g_fota_upgrade_ec25.package_index && 
                            package_index != g_fota_upgrade_ec25.package_index -1) 
                    )
                {
                    LOG_E("fota upgrade from stm32, package index error[index=%u, local=%u].", package_index, g_fota_upgrade_ec25.package_index);
                    reply_code = MCU_PACKET_ERRCODE_DATA;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                }
                else
                {
                    if(g_fota_upgrade_ec25.fd > 0)
                    {
                        if(package_index == g_fota_upgrade_ec25.package_index -1)
                        {
                            off_t offset = lseek(g_fota_upgrade_ec25.fd, -1 * (data_len - 4), SEEK_CUR);
                            g_fota_upgrade_ec25.deal_size -= data_len - 4;
                            g_fota_upgrade_ec25.package_index = package_index;
                            LOG_D("========fota upgrade retransmission ===========");
                            LOG_D("offset=%d, packetsize=%d,deal_size=%d", offset, data_len - 4, g_fota_upgrade_ec25.deal_size);
                        }
                        if(data_len - 4 != write(g_fota_upgrade_ec25.fd, p + 4, data_len - 4))
                        {
                            LOG_E("write error.[len=%d]", data_len - 4);
                            reply_code = MCU_PACKET_ERRCODE_DATA;
                            reply_resp = SERIAL_RESP_TYPE_ERROR;
                        }
                        else
                        {
                            g_fota_upgrade_ec25.package_index += 1;
                            g_fota_upgrade_ec25.deal_size += data_len - 4;
                            g_fota_upgrade_ec25.last_time = niu_device_get_timestamp();
                            reply_code = 0;
                            reply_resp = SERIAL_RESP_TYPE_NORMAL;
                        }
                    }
                    else
                    {
                        reply_code = MCU_PACKET_ERRCODE_DATA;
                        reply_resp = SERIAL_RESP_TYPE_ERROR;
                    }
                    // push event to update upgrade percent.
                    NUINT16 *percent = QL_MEM_ALLOC(sizeof(NUINT16));
                    if(percent != NULL && g_fota_upgrade_ec25.fsize > 0)
                    {
                        *percent = (NUINT16)(g_fota_upgrade_ec25.deal_size * 100 / g_fota_upgrade_ec25.fsize );
                        sys_event_push(FOTA_EVENT_UPGRADE_STM32_TO_EC25_RATE, percent, sizeof(NUINT16));
                    }
                }
                //reply to mcu
                LOG_D("reply_resp: %d, reply_code: %d", reply_resp, reply_code);
                ret = mcu_send_reply(cmd_type, reply_resp, reply_code);
                if(ret != SERIAL_PACKET_PPOCESS_OK)
                {
                    LOG_E("cmd: 0x%02X, failed!", cmd_type);
                }
            }
        }
        break;
        case SERIAL_COMMAND_FOTA_VERIFY:
        {
            //need response
            fota_upgrade_device_completed_t *upgrade_result = NULL;
            LOG_D("------------SERIAL_COMMAND_FOTA_VERIFY------------");
            if(resp_type != 0)
            {   //ec25 upgrade stm32
				NUINT8 err = *(NUINT8*)p;
                upgrade_result = (fota_upgrade_device_completed_t *)QL_MEM_ALLOC(sizeof(fota_upgrade_device_completed_t));
                if(NULL == upgrade_result) {
                    sys_event_push(FOTA_EVENT_UPGRADE_DEVICE_COMPLETED, NULL, 0);
                }
                else {
                    if((resp_type == SERIAL_RESP_TYPE_NORMAL) && (err == 0x00)) {
                        upgrade_result->result = UPGRADE_CODE_SUCCESS;
                    }
                    else {
                        upgrade_result->result = UPGRADE_CODE_INSTALL_FAIL;
                    }
                    //strncpy(upgrade_result->type, "EMCU", sizeof(upgrade_result->type));
                    strncpy(upgrade_result->type, g_fota_upgrade_stm32.device_type, sizeof(upgrade_result->type));
                    sys_event_push(FOTA_EVENT_UPGRADE_DEVICE_COMPLETED, (void *)upgrade_result, sizeof(fota_upgrade_device_completed_t));
                }
                memset(&g_fota_upgrade_stm32, 0, sizeof(g_fota_upgrade_stm32));
				g_mcu_upgrade_flag = 0;
                if(tmr_handler_tmr_is_exist(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK))
                {
                    LOG_D("tmr_handler_tmr_del(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);");
                    tmr_handler_tmr_del(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);
                }
            }
            else
            {
                //stm32 upgrade ec25
                NUINT8 reply_code = 0;
                NUINT8 reply_resp = 0;
                NUINT16 verify = *(NUINT16*)p;

                if(g_fota_upgrade_ec25.fd > 0)
                {
                    close(g_fota_upgrade_ec25.fd);
                    g_fota_upgrade_ec25.fd = -1;
                }
                memset(&g_fota_upgrade_stm32, 0, sizeof(g_fota_upgrade_stm32));

                NUINT16 verify2 = CRC16_file(FOTA_UPGRADE_FILE_EC25);
                if(verify != verify2)
                {
                    reply_code = MCU_PACKET_ERRCODE_DATA;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                    LOG_E("verify error.[local: %d, remote: %d]", verify2, verify);
                    
                //    sys_event_push(FOTA_EVENT_UPGRADE_STM32_TO_EC25_FAILED, NULL, 0);
                }
                else
                {
                    reply_code = 0;
                    reply_resp = SERIAL_RESP_TYPE_NORMAL;
                    sys_event_push(FOTA_EVENT_UPGRADE_STM32_TO_EC25_FINISH, NULL, 0);
                }
                //reply to mcu
                ret = mcu_send_reply(cmd_type, reply_resp, reply_code);
                if(ret != SERIAL_PACKET_PPOCESS_OK)
                {
                    LOG_E("cmd: 0x%02X, failed!", cmd_type);
                }
                g_mcu_upgrade_flag = 0;
                // stop check 
                if(tmr_handler_tmr_is_exist(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK))
                {
                    LOG_D("tmr_handler_tmr_del(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);");
                    tmr_handler_tmr_del(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);
                }

            }
        }
        break;
        case SERIAL_COMMAND_CONFIG_START:
        {
            if(resp_type == 0)
            {
                /*if resp type is 0, this means stm32 send upgrade request to ec25.
                * we must send response to stm32, resp_normal or resp_error,
                * and then set package index = 1, and open file fd used to write package data
                */
                NUINT8 reply_code = 0;
                NUINT8 reply_resp = 0;
                NUINT32 size = *(NUINT32*)p;
                LOG_D("=====SERIAL_COMMAND_CONFIG_START===== size: %d", size);
                niu_trace_dump_hex("buff", buff, pkt_len);
                if(size == 0 || size > MAX_UPGRADE_FILE_SIZE)
                {
                    LOG_E("fota upgrade start, size error[size=%u].", size);
                    reply_code = MCU_PACKET_ERRCODE_DATA;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                }
                else
                {
                    if(ql_com_file_exist(FOTA_UPGRADE_PATH))
                    {
                        LOG_D("%s is not exist.", FOTA_UPGRADE_PATH);
                        ret = mkdir(FOTA_UPGRADE_PATH, 0777);
                        if(ret)
                        {
                            LOG_E("mkdir error.[%s]", FOTA_UPGRADE_PATH);
                        }
                    }
                    else
                    {
                        if(!ql_com_file_exist(CONFIG_UPGRADE_FILE_EC25))
                        {
                            LOG_D("file %s is exist, del it", CONFIG_UPGRADE_FILE_EC25);
                            unlink(CONFIG_UPGRADE_FILE_EC25);
                        }
                    }
                    g_config_upgrade_ec25.fd = open(CONFIG_UPGRADE_FILE_EC25, O_RDWR|O_CREAT|O_TRUNC, 0777);
                    if(g_config_upgrade_ec25.fd > 0)
                    {
                        reply_code = 0;
                        reply_resp = SERIAL_RESP_TYPE_NORMAL;

                        g_config_upgrade_ec25.fsize = size;
                        g_config_upgrade_ec25.is_start = 1;
                        g_config_upgrade_ec25.package_index = 1;
                        g_config_upgrade_ec25.deal_size = 0;
                        g_config_upgrade_ec25.last_time = niu_device_get_timestamp();

                        if(!tmr_handler_tmr_is_exist(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK))
                        {
                            if(tmr_handler_tmr_add(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK, mcu_upgrade_timeout_check, NULL, CHECK_UPGRADE_TIMEOUT, 0) != 0)
                            {
                                LOG_E("tmr handler add timer id=%d error", TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);
                            }
                        }
                    }
                    else
                    {
                        LOG_E("open file to write error.");
                        reply_code = MCU_PACKET_ERRCODE_DATA;
                        reply_resp = SERIAL_RESP_TYPE_ERROR;
                    }
                }

                ret = mcu_send_reply(cmd_type, reply_resp, reply_code);
                if(ret != SERIAL_PACKET_PPOCESS_OK)
                {
                    LOG_E("cmd: 0x%02X, failed!", cmd_type);
                }
            }
        }
        break;
        case SERIAL_COMMAND_CONFIG_PACKAGE:
        {
            if(resp_type == 0)
            {
                //stm32 upgrade ec25
                NUINT8 reply_code = 0;
                NUINT8 reply_resp = 0;
                NUINT32 package_index = *(NUINT32*)p;
                if(g_config_upgrade_ec25.is_start != 1)
                {
                    LOG_E("please send config start command first.");
                    reply_code = MCU_PACKET_ERRCODE_COMMAND;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                    ret = mcu_send_reply(cmd_type, reply_resp, reply_code);
                    if(ret != SERIAL_PACKET_PPOCESS_OK)
                    {
                        LOG_E("cmd: 0x%02X, failed!", cmd_type);
                    }
                    break;
                }
                LOG_D("fota upgrade from stm32, [index=%u, local=%u].", package_index, g_config_upgrade_ec25.package_index);
                niu_trace_dump_hex("buff", buff, pkt_len);
                if(package_index == 0 || 
                        (package_index != g_config_upgrade_ec25.package_index && 
                            package_index != g_config_upgrade_ec25.package_index -1) 
                    )
                {
                    LOG_E("fota upgrade from stm32, package index error[index=%u, local=%u].", package_index, g_config_upgrade_ec25.package_index);
                    reply_code = MCU_PACKET_ERRCODE_DATA;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                }
                else
                {
                    if(g_config_upgrade_ec25.fd > 0)
                    {
                        if(package_index == g_config_upgrade_ec25.package_index -1)
                        {
                            off_t offset = lseek(g_config_upgrade_ec25.fd, -1 * (data_len - 4), SEEK_CUR);
                            g_config_upgrade_ec25.deal_size -= data_len - 4;
                            g_config_upgrade_ec25.package_index = package_index;
                            LOG_D("========config upgrade retransmission ===========");
                            LOG_D("offset=%d, packetsize=%d,deal_size=%d", offset, data_len - 4, g_config_upgrade_ec25.deal_size);
                        }
                        if(data_len - 4 != write(g_config_upgrade_ec25.fd, p + 4, data_len - 4))
                        {
                            LOG_E("write error.[len=%d]", data_len - 4);
                            reply_code = MCU_PACKET_ERRCODE_DATA;
                            reply_resp = SERIAL_RESP_TYPE_ERROR;
                        }
                        else
                        {
                            g_config_upgrade_ec25.package_index += 1;
                            g_config_upgrade_ec25.deal_size += data_len - 4;
                            g_config_upgrade_ec25.last_time = niu_device_get_timestamp();
                            reply_code = 0;
                            reply_resp = SERIAL_RESP_TYPE_NORMAL;
                        }
                    }
                    else
                    {
                        reply_code = MCU_PACKET_ERRCODE_DATA;
                        reply_resp = SERIAL_RESP_TYPE_ERROR;
                    }
                    // push event to update upgrade percent.
                    NUINT16 *percent = QL_MEM_ALLOC(sizeof(NUINT16));
                    if(percent != NULL && g_config_upgrade_ec25.fsize > 0)
                    {
                        *percent = (NUINT16)(g_config_upgrade_ec25.deal_size * 100 / g_config_upgrade_ec25.fsize );
                        sys_event_push(FOTA_EVENT_CONFIG_STM32_TO_EC25_RATE, percent, sizeof(NUINT16));
                    }
                }
                //reply to mcu
                LOG_D("reply_resp: %d, reply_code: %d", reply_resp, reply_code);
                ret = mcu_send_reply(cmd_type, reply_resp, reply_code);
                if(ret != SERIAL_PACKET_PPOCESS_OK)
                {
                    LOG_E("cmd: 0x%02X, failed!", cmd_type);
                }
            }
        }
        break;
        case SERIAL_COMMAND_CONFIG_VERIFY:
        {
            if(resp_type == 0)
            {
                //stm32 upgrade ec25
                NUINT8 reply_code = 0;
                NUINT8 reply_resp = 0;
                NUINT16 verify = *(NUINT16*)p;
                niu_trace_dump_hex("buff", buff, pkt_len);
                if(g_config_upgrade_ec25.is_start != 1)
                {
                    LOG_E("please send config start command first.");
                    reply_code = MCU_PACKET_ERRCODE_COMMAND;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                    ret = mcu_send_reply(cmd_type, reply_resp, reply_code);
                    if(ret != SERIAL_PACKET_PPOCESS_OK)
                    {
                        LOG_E("cmd: 0x%02X, failed!", cmd_type);
                    }
                    break;
                }
                if(g_config_upgrade_ec25.fd > 0)
                {
                    close(g_config_upgrade_ec25.fd);
                    g_config_upgrade_ec25.fd = -1;
                }
                memset(&g_config_upgrade_ec25, 0, sizeof(g_config_upgrade_ec25));

                NUINT16 verify2 = CRC16_file(CONFIG_UPGRADE_FILE_EC25);
                if(verify != verify2)
                {
                    reply_code = MCU_PACKET_ERRCODE_DATA;
                    reply_resp = SERIAL_RESP_TYPE_ERROR;
                    LOG_E("verify error.[local: %d, remote: %d]", verify2, verify);
                    
                    //sys_event_push(FOTA_EVENT_CONFIG_STM32_TO_EC25_FINISH, NULL, 0);
                }
                else
                {
                    reply_code = 0;
                    reply_resp = SERIAL_RESP_TYPE_NORMAL;
                    sys_event_push(FOTA_EVENT_CONFIG_STM32_TO_EC25_FINISH, NULL, 0);
                }
                //reply to mcu
                ret = mcu_send_reply(cmd_type, reply_resp, reply_code);
                if(ret != SERIAL_PACKET_PPOCESS_OK)
                {
                    LOG_E("cmd: 0x%02X, failed!", cmd_type);
                }
                // stop check 
                if(tmr_handler_tmr_is_exist(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK))
                {
                    LOG_D("tmr_handler_tmr_del(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);");
                    tmr_handler_tmr_del(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);
                }
            }
        }
        break;
        default:
            break;
    }
    return ret;
}



static int frame_packet_format_check(NUINT8 *buff, NUINT16 pkt_len)
{
    int ret;

    ret = SERIAL_PACKET_PPOCESS_OK;

    if(buff == NULL || pkt_len < SERIAL_PACKET_LENGTH_MIN)
    {
        LOG_E("serial packet begin error.");
        ret = SERIAL_PACKET_OTHER_ERROR;
        goto _out;
    }
    //check header $CMC, is check already
    //check cmd and resp
    NUINT8 opt = *(buff + V35_PACKET_OFFSET_OPT);
    NUINT8 cmd_type = opt & SERIAL_COMMAND_MASK;
    NUINT8 resp_type = opt & SERIAL_RESP_TYPE_MASK;

    if(cmd_type == SERIAL_COMMAND_NONE || cmd_type >= SERIAL_COMMAND_MAX)
    {
        LOG_E("serial packet opt error.");
        niu_trace_dump_hex("packet", buff, pkt_len);
        ret = SERIAL_PACKET_COMMAND_ERROR;
        goto _out;
    }
    if(resp_type > 0)
    {
        if(resp_type != SERIAL_RESP_TYPE_NORMAL && resp_type != SERIAL_RESP_TYPE_ERROR)
        {
            LOG_E("serial packet opt error.");
            niu_trace_dump_hex("packet", buff, pkt_len);
            ret = SERIAL_PACKET_COMMAND_ERROR;
            goto _out;
        }
    }

    //check tail
    if(buff[pkt_len - SERIAL_PACKET_TAIL_SIZE] != SERIAL_PACKET_TAIL)
    {
        //failed, the last character is not '&'
        LOG_E("serial packet tail error.");
        niu_trace_dump_hex("packet", buff, pkt_len);
        ret = SERIAL_PACKET_TAIL_ERROR;
        goto _out;
    }
    //check crc8
#ifdef _MCU_CRC8_CHECK_
    NUINT8 crc_check = CRC8(buff + V35_PACKET_OFFSET_OPT, 
                        CRC8_CALC_LENGTH(pkt_len));
    //niu_trace_dump_hex("dump", buff, pkt_len);
    if(crc_check != buff[V35_PACKET_OFFSET_CRC8(pkt_len)])
    {
        LOG_E("check crc8 failed.[%x - %x]", crc_check,  buff[V35_PACKET_OFFSET_CRC8(pkt_len)]);
        ret = SERIAL_PACKET_VERIFY_ERROR;
        goto _out;
    }
#endif //_MCU_CRC8_CHECK_

_out:
    return ret;
}
/**
 * process uart packet.
 * @buff: received data, include header + framelen + opt + data + crc8 + tail
 * @len: received packet length
 * 
 * @return:
 *  if process success ,return 0
 *  else return error code
 * @error code:
 *  0x80 verify error
 *  0x40 address error
 *  0x20 command error
 *  0x10 data error
 *   < 0   other 
 */
static int frame_packet_process(NUINT8 *buff, NUINT16 pkt_len)
{
    if(buff == NULL || pkt_len < SERIAL_PACKET_LENGTH_MIN)
    {
        LOG_E("buff or pkt_len error.[pkt_len=%d]", pkt_len);
        return SERIAL_PACKET_OTHER_ERROR;
    }

    return mcu_command_process(buff, pkt_len);
}

#define UART_RECV_BUFF_SIZE_MAX (10*1024)
void* thread_uart_data_recv_niu(void *arg)
{
    int ret, i;
    fd_set fdset;
    int read_bytes, total_bytes = 0;
    NUINT8 *buffer = NULL, *q = NULL, *start = NULL;
    NUINT16 frame_len;

    struct timeval timeout = {3, 0};
    
    pthread_detach(pthread_self());
    g_mcu_fd = -1;


    buffer = QL_MEM_ALLOC(UART_RECV_BUFF_SIZE_MAX);

    if (buffer == NULL)
    {
        LOG_E("malloc failed.!");
        return NULL;
    }
_loop:
    total_bytes = 0;
    if(g_mcu_fd > 0)
    {
        Ql_UART_Close(g_mcu_fd);
    }

    g_mcu_fd = uart_connect(UART_DEV_STM_NAME, UART_DEV_STM_BOUNDRATE, UART_DEV_STM_FLOWCTRL, UART_DEV_STM_PARITY);
  
    if(g_mcu_fd < 0)
    {
        LOG_E("uart connect failed.");
        sleep(1);
        goto _loop;
    }

    while(1)
    {
        FD_ZERO(&fdset);
        FD_SET(g_mcu_fd, &fdset);
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        ret = select(g_mcu_fd + 1, &fdset, NULL, NULL, &timeout);
        if (ret == -1)
		{
			LOG_E("failed to select");
            goto _loop;
		}
        else if (ret == 0)
		{// no data in Rx buffer
			continue;
		}
        else
		{// data is in Rx data
			if (FD_ISSET(g_mcu_fd, &fdset)) 
			{
_rec:
                read_bytes = Ql_UART_Read(g_mcu_fd, (char*)buffer + total_bytes, 100);
                if (read_bytes == -1)
                {
                    if(errno == EAGAIN)
                    {
                        continue;
                    }
                    else
                    {
                        goto _loop;
                    }
                }
                else if(read_bytes == 0)
                {
                    goto _loop;
                }
                total_bytes += read_bytes;
                if (total_bytes > UART_RECV_BUFF_SIZE_MAX - 100)
                {
                    total_bytes = 0;
                    goto _rec;
                }

                q = buffer;
_repeat:
                if (total_bytes < SERIAL_PACKET_HEADER_SIZE)
                {
                    goto _rec;
                }
                start = NULL;
                for (i = 0; i < total_bytes; i++)
                {
                    if (memcmp(q + i, "$", 1) == 0)
                    { 
                        //check first char
                        start = q + i;
                        break;
                    }
                }

                if (start == NULL)
                {
                    total_bytes = 0;
                    goto _rec;
                }

                if (total_bytes - (start - q) >= SERIAL_PACKET_HEADER_SIZE)
                {
                    if (memcmp(start, SERIAL_PACKET_HEADER, SERIAL_PACKET_HEADER_SIZE) != 0)
                    {
                        //just skip first character
                        total_bytes -= start - q + 1;
                        q = start + 1;
                        goto _repeat;
                    }
                }
                else
                {
                    total_bytes -= (start - q);
                    if (buffer != memmove(buffer, start, total_bytes))
                    {
                        total_bytes = 0;
                    }
                    goto _rec;
                }
                
                frame_len = 0;
                if (total_bytes - (start - q) >= SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE)
                {
                    memcpy(&frame_len, start + SERIAL_PACKET_HEADER_SIZE, SERIAL_PACKET_LENGTH_SIZE);
                }
                else
                {
                    total_bytes -= (start - q);
                    if (buffer != memmove(buffer, start, total_bytes))
                    {
                        total_bytes = 0;
                    }
                    goto _rec;
                }

                if (frame_len < SERIAL_FRAME_LENGTH_MIN || frame_len > SERIAL_FRAME_LENGTH_MAX)
                {
                    //skip header
                    total_bytes -= (start - q + SERIAL_PACKET_HEADER_SIZE);
                    q = start + SERIAL_PACKET_HEADER_SIZE;
                    goto _repeat;
                }
                if (total_bytes - (start - q) - SERIAL_PACKET_HEADER_SIZE - SERIAL_PACKET_LENGTH_SIZE < frame_len)
                {
                    //printf("frame_len: %d, valid_len: %d, rest_len: %d, goto rec.\n", frame_len, total_bytes - (start - q) - 6, total_bytes);
                    total_bytes -= (start - q);
                    if (buffer != memmove(buffer, start, total_bytes))
                    {
                        total_bytes = 0;
                    }

                    goto _rec;
                }
                //check frame format
                if (frame_packet_format_check(start, frame_len + SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE) != 0)
                {
                    total_bytes -= (start - q + SERIAL_PACKET_HEADER_SIZE);
                    q = start + SERIAL_PACKET_HEADER_SIZE;
                    goto _repeat;
                }
                else
                {
                    ret = frame_packet_process(start, frame_len + SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE);
                    if(ret != SERIAL_PACKET_PPOCESS_OK)
                    {
                        LOG_D("serial packet process error.[ret=%d]", ret);
                        niu_trace_dump_hex("error", start, frame_len + SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE);
                    }
                    else
                    {
                        //niu_trace_dump_hex("recv", start, frame_len + SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE);
                    }

                    total_bytes -= (start - q + frame_len + SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE);
                    q += (start - q + frame_len + SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE);
                    if (total_bytes == 0)
                    {
                        goto _rec;
                    }
                    goto _repeat;
                }
            }
        }
    }
    if(buffer)
    {
        QL_MEM_FREE(buffer);
    }
    return NULL;
}



/**
 * 
 * if remaining size of file > package_size, then send package_size,
 * 
 * 
 * 
 */ 
int mcu_upgrade_stm32_package()
{
    if(g_fota_upgrade_stm32.deal_size < g_fota_upgrade_stm32.fsize)
    {
        return mcu_send_file_content(&g_fota_upgrade_stm32, SERIAL_COMMAND_FOTA_PACKAGE, 0);
    }
    else
    {
        //send over
        LOG_D("send over, fsize: %d, deal_size: %d.", g_fota_upgrade_stm32.fsize, g_fota_upgrade_stm32.deal_size);
        NUINT16 verify = CRC16_file(g_fota_upgrade_stm32.fname);
        return mcu_send_file_verify(SERIAL_COMMAND_FOTA_VERIFY, 0, verify);
    }

    return -1;
}

/**
 * 
 * 数据域：
 * ------------------------------------
 * |               4                  | 
 * ------------------------------------
 *       32bit数 FOTA包长度
 * 
 */ 
void mcu_upgrade_stm32_start(int tmr_id, void *param)
{
    int ret;

    if(tmr_id != TIMER_ID_FOTA_UPGRADE_START)
    {
        LOG_E("tmr_id(%d) != TIMER_ID_FOTA_UPGRADE_START", tmr_id);
        return;
    }

    if(g_fota_upgrade_stm32.retry < MCU_UPGRADE_RETRY)
    {
        tmr_handler_tmr_add(TIMER_ID_FOTA_UPGRADE_START, mcu_upgrade_stm32_start, NULL, 2*1000, 0);
    }
    else
    {
        LOG_E("mcu_upgrade_stm32_start retry 3 times error, exit upgrade.");
        return ;
    }

    ret = mcu_send_file_start(&g_fota_upgrade_stm32, SERIAL_COMMAND_FOTA_START, 0);
    if(ret == 0)
    {
        //package_index begin with 1
        g_fota_upgrade_stm32.package_index = 1;
        g_fota_upgrade_stm32.retry += 1;
    }
    return ;
}

/**
 * used to upgrade stm32, the upgrade bin file will download by fota.
 * after decrypt can invoke this function to complete upgrade.
 * 
 * @fname: the upgrade bin file name, full name include path.
 * 
 * @return:
 *          if success return 0, else return -1
 * 
 * @note:
 *          this function return success just means execute this function success.
 *      it 
 */ 
int mcu_upgrade_to_stm32(char *fname, char* device_type)
{
    int fsize;
    int device_id = -1;

    if(fname == NULL || device_type == NULL)
    {
        LOG_E("mcu_file_trans fname or device_type is null.");
        return -1;
    }

    if(ql_com_file_exist(fname))
    {
        LOG_E("mcu_file_trans fname is not exist[%s].", fname);
        return -1;
    }

    fsize = ql_com_file_get_size(fname);
    if(fsize < 0 || fsize > 20*1024*1024)
    {
        LOG_E("mcu_file_trans fname size error[%s:%d].", fname, fsize);
        return -1;
    }

    device_id = (NUINT16)ql_n2v(&g_niu_vn_device_type, device_type);
    if(device_id < 0)
    {
        LOG_E("device type error[%s].", device_type);
        return -1;
    }
    //check upgrade state now. if stm32 upgrading ec25, then
    //ec25 can not upgrade stm32.
    if(g_mcu_upgrade_flag == 1)
    {
        LOG_E("mcu upgrade already run.");
        return -1;
    }
    g_fota_upgrade_stm32.fsize = fsize;
    g_fota_upgrade_stm32.package_index = 0;
    g_fota_upgrade_stm32.deal_size = 0;
    g_fota_upgrade_stm32.is_start = 0;
    g_fota_upgrade_stm32.retry = 0;
    g_fota_upgrade_stm32.device_id = device_id;
    strncpy(g_fota_upgrade_stm32.device_type, device_type, sizeof(g_fota_upgrade_stm32.device_type));
    g_fota_upgrade_stm32.package_size = FOTA_UPGRADE_PACKAGE_SIZE;
    strncpy(g_fota_upgrade_stm32.fname, fname, sizeof(g_fota_upgrade_stm32.fname));
    g_mcu_upgrade_flag = 1;
    //start upgrade_stm32_check
    if(!tmr_handler_tmr_is_exist(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK))
    {
        if(tmr_handler_tmr_add(TIMER_ID_MCU_FOTA_TIMEOUT_CHECK, mcu_upgrade_timeout_check, NULL, CHECK_UPGRADE_TIMEOUT, 0) != 0)
        {
            LOG_E("tmr handler add timer id=%d error", TIMER_ID_MCU_FOTA_TIMEOUT_CHECK);
        }
    }
    tmr_handler_tmr_add(TIMER_ID_FOTA_UPGRADE_START, mcu_upgrade_stm32_start, NULL, 2*1000, 0);
    return 0;
}



int mcu_data_init()
{
    int ret;
    pthread_t pid;

    ret = pthread_create(&pid, NULL, thread_uart_data_recv_niu, NULL);
    if(ret)
    {
        LOG_E("create serial recv thread error.");
        return -1;
    }

    LOG_D("start uart data recv from mcu.");

    
    

    return 0;
}
