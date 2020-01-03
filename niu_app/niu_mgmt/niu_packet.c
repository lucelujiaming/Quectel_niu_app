#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ql_oe.h>
#include "niu_data.h"
#include "utils.h"
#include "niu_data_pri.h"
#include "mcu_data.h"
#include "ql_mem.h"
#include "ql_common.h"
#include "niu_packet.h"



//extern int g_mcu_fd;
/**
 * generate serial packet buffer
 * @cmd: command 
 * @type: response type
 * @data: data zone in packet, refer to V3 protocol in kernel
 * @data_len: send data length
 * @pkt_len: [out] generate packet buffer length
 * 
 * @return:
 *  if generate success, return point of the buffer
 *  else return NULL, that means generate failed.
 */ 
NUINT8* generate_mcu_packet(NUINT8 cmd, NUINT8 resp, NUINT8 *data, NUINT16 data_len, NUINT32 *pkt_len)
{
    int len, pos;
    NUINT8 crc, opt;
    NUINT16 frame_len;
    NUINT8 *buff = NULL;

    if(data == NULL || data_len <= 0)
    {
        LOG_E("serial packet data zone error.");
        return NULL;
    }

    if(cmd <= SERIAL_COMMAND_NONE || cmd >= SERIAL_COMMAND_MAX)
    {
        LOG_E("serial packet command type error.");
        return NULL;
    }

    // if(resp != SERIAL_RESP_TYPE_ERROR && resp != SERIAL_RESP_TYPE_NORMAL)
    // {
    //     LOG_E("serial packet response type error.");
    //     return NULL;
    // }

    len = data_len + SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE + SERIAL_PACKET_OPT_SIZE +SERIAL_PACKET_CRC8_SIZE + SERIAL_PACKET_TAIL_SIZE;

    buff = QL_MEM_ALLOC(len);
    if(buff == NULL)
    {
        LOG_E("alloc buffer error.[len: %d]", len);
        return NULL;
    }

    pos = 0;
    //header
    memcpy(buff + pos, SERIAL_PACKET_HEADER, SERIAL_PACKET_HEADER_SIZE);
    pos += SERIAL_PACKET_HEADER_SIZE;
    //frame length
    pos += SERIAL_PACKET_LENGTH_SIZE;
    //opt
    opt = cmd | resp;

    memcpy(buff + pos, &opt, sizeof(NUINT8));
    pos += sizeof(NUINT8);
    //data zone
    memcpy(buff + pos, data, data_len);
    pos += data_len;
    //crc8
    pos += SERIAL_PACKET_CRC8_SIZE;
    //memcpy(buff + pos, SERIAL_PACKET_TAIL, );
    buff[pos] = SERIAL_PACKET_TAIL;
    pos += SERIAL_PACKET_TAIL_SIZE;

    //padding frame_len
    frame_len = pos - SERIAL_PACKET_HEADER_SIZE - SERIAL_PACKET_LENGTH_SIZE;
    memcpy(buff + SERIAL_PACKET_HEADER_SIZE, (NUINT8*)&frame_len, sizeof(NUINT16));
    //padding crc8
    crc = CRC8(buff + SERIAL_PACKET_HEADER_SIZE + SERIAL_PACKET_LENGTH_SIZE, CRC8_CALC_LENGTH(pos));
    buff[pos - SERIAL_PACKET_TAIL_SIZE - SERIAL_PACKET_CRC8_SIZE] = crc;

    *pkt_len = len;

    return buff;
}


/**
 * send packet handle, called by cmdq
 * params:
 *  @param:
 *      function param, maybe a struct piont
 * 
 * @return: void
 * 
 *
 */ 
void send_packet_handle(void *param)
{
    int ret;
    fd_set wfds;
    struct timeval tm;
    int total = 0, cnt;

    if(param == NULL)
    {
        LOG_E("param is null.");
        return ;
    }

    struct frame_packet *pkt = (struct frame_packet*)param;
    if(pkt->len > 0 && pkt->buff != NULL && pkt->fd > 0)
    {
        while(pkt->len > total)
        {
            FD_ZERO(&wfds);
            FD_SET(pkt->fd, &wfds);
        
            tm.tv_sec = 3;
            tm.tv_usec = 0;
            cnt = select(pkt->fd + 1, NULL, &wfds, NULL, &tm);
            if(cnt < 0 && errno != EINTR)  {
                LOG_E("Failed to select, err=%s", strerror(errno));
                goto _out;
            }
            if(cnt > 0) {
                ret = Ql_UART_Write(pkt->fd, (char*)pkt->buff + total, pkt->len - total);
                if(ret <= 0)
                {
                    LOG_E("Failed to write, err=%s", strerror(errno));
                    goto _out;
                }
                total += ret;
            }
        }
        if(memcmp(pkt->buff, "\x24\x43\x4D\x43\x0E\x00\x04\x40\x03\x00", 10) == 0)
        {
            char buffer[128] = {0};
            int index = 0, pos = 0;
            for (index = 0; index < pkt->len; index++)
            {
                snprintf(buffer + pos, 127 - index, "%02X ", pkt->buff[index]);
                pos += 3;
            }
            LOG_D("softver: %s", buffer);
        }
    }

_out:
    if(pkt->buff)
    {
        QL_MEM_FREE(pkt->buff);
    }
    QL_MEM_FREE(pkt);

    return;
}