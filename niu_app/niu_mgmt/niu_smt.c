/*-----------------------------------------------------------------------------------------------*/
/**
  @niu_smt.c
  @brief xiaoniu smt test 
*/
/*-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
  EDIT HISTORY
  This section contains comments describing changes made to the file.
  Notice that changes are listed in reverse chronological order.
  $Header: $
  when       who          what, where, why
  --------   ---          ----------------------------------------------------------
  20190304   juson.zhang  Created .
-------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include "ql_oe.h"
#include "niu_data_pri.h"
#include "nmea_parse.h"
#include "utils.h"
#include "ql_common.h"
#include "niu_data.h"
#include "mcu_data.h"
#include "niu_smt.h"
#include "sys_core.h"
#include "gps_data.h"
#include "gsensor_data.h"
#include "niu_packet.h"

#define NIU_SMT_FILE_PATH "/home/root/xn_config/niu_smt_cfg"

#define EC25_INIT_OUT_PIN   (37)
#define GPS_POWER_ON_PIN    (5) //GPS_POWER_ON(GPIO24)
#define WATCHDOG_EC25_PIN   (3) //WATCHDOG_EC25
#define WAKEUP_OUT_PIN      (23)
#define WAKEUP_IN_PIN       (1)

static unsigned short g_smt_test_result = 0;
static int g_gps_uart_fd = -1;
static int g_smt_mcu_fd = -1;
static int g_h_sim = -1;
static int g_h_nw = -1;
pthread_mutex_t mutex_lock;

void wakeup_in_irq_cb(Enum_PinName pin_name, int level)
{
    static unsigned char irq_cnt = 0;

    LOG_V("<%s> pin name:%d, level=%d ", __func__, pin_name, level);
    if (WAKEUP_IN_PIN != pin_name)
    {
        LOG_E("[%s] invalid pin name! ", __func__);
        return;
    }
    if (0 == level)
    {
        if (irq_cnt++ > 0)
        {
            LOG_D("[%s] wakeup in detect success!", __func__);
            g_smt_test_result |= NIU_SMT_WAKEUP;
            irq_cnt = 0;
            Ql_EINT_Disable(pin_name);            
        }
    }
    else if(1 == level)
    {
        if (irq_cnt++ > 0)
        {
            LOG_D("[%s] wakeup in detect success!", __func__);
            g_smt_test_result |= NIU_SMT_WAKEUP;
            irq_cnt = 0;
            Ql_EINT_Disable(pin_name);
        }
    }
}

static void *gpio_trd_handler(void *arg)
{
    unsigned char pin_level = 0;

    while (1)
    {
        pin_level = (~pin_level) & 0x01;

        Ql_GPIO_SetLevel(WATCHDOG_EC25_PIN, pin_level);
        Ql_GPIO_SetLevel(WAKEUP_OUT_PIN, pin_level);

        LOG_V("[%s] pin_level = %d ", __func__, pin_level);
        usleep(500000);
    }

    return (void *)0;
}

/*time: ms*/
int niu_smt_gpio_test_init(void)
{    
    int ret = -1;
    pthread_t ptd;

    system("rmmod ql_lpm && sync");
    usleep(500*1000);

    Ql_GPIO_Init(WATCHDOG_EC25_PIN, PINDIRECTION_OUT, PINLEVEL_LOW, PINPULLSEL_DISABLE); //init watchdog gpio pin
    Ql_GPIO_Init(WAKEUP_OUT_PIN, PINDIRECTION_OUT, PINLEVEL_LOW, PINPULLSEL_DISABLE); //init wakeup out gpio pin
    // Ql_GPIO_Init(WAKEUP_IN_PIN, PINDIRECTION_IN, PINLEVEL_LOW, PINPULLSEL_PULLUP); //init wakeup in gpio pin

    Ql_EINT_Enable(WAKEUP_IN_PIN, EINT_SENSE_BOTH, wakeup_in_irq_cb);   //enable wakeup_in_pin irq

    ret = pthread_create(&ptd, NULL, gpio_trd_handler, NULL);
    if (0 != ret)
    {
        LOG_E("[%s] fail to create smt gpio thread! ", __func__);
        return -1;
    }

    return 0;
}

static void* gps_uart_recv_handler(void* arg)
{
    int read_bytes, total_bytes = 0, ret;
    char buffer[1024] = {0}, nema_buff[256] = {0};
    char *start, *end;
    fd_set fdset;

    while (g_gps_uart_fd >=0)
    {
        FD_ZERO(&fdset);
        FD_SET(g_gps_uart_fd, &fdset);
        ret = select(g_gps_uart_fd + 1, &fdset, NULL, NULL, NULL);
        if (-1 == ret)
        {
            LOG_E("[%s] failed to select! ", __func__);
            continue;
        }
        else
        {// data is in Rx data
            if (FD_ISSET(g_gps_uart_fd, &fdset))
            {
_rec:
                read_bytes = Ql_UART_Read(g_gps_uart_fd, buffer + total_bytes, 20);
                if(-1 == read_bytes)
                {
                    continue;
                }
                total_bytes += read_bytes;

                if(total_bytes > sizeof(buffer) - 20)
                {
                    total_bytes = 0;
                    goto _rec;
                }

                start = memchr(buffer, '$', total_bytes);
                if (NULL == start) {
                    total_bytes = 0;
                    goto _rec;
                }
                /* find end of line */
                end = memchr(start, NMEA_END_CHAR_1, total_bytes - (start - buffer));
                if (NULL == end || NMEA_END_CHAR_2 != *(++end)) {
                    goto _rec;
                }
                total_bytes = 0;
                /*upload gps data*/
                memset(nema_buff, 0, 256);
                /* do not include the end of nmea \r\n */
                memcpy(nema_buff, start, jmin(255, end - start - 1));
                LOG_V("[%s] %s", __func__, nema_buff); 
                if (NMEA_UNKNOWN != nmea_get_type(nema_buff))   //GPS pins are OK
                {
                    LOG_D("[%s] gps detect success! ", __func__);  
                    g_smt_test_result |= NIU_SMT_GPS;                        
                    close(g_gps_uart_fd);
                    g_gps_uart_fd = -1;
                    break;                                     
                }          
            }
        }
    }
    return (void *)1;
}

int niu_smt_gps_test_init(void)
{
    int ret = -1;
    pthread_t ptd;   

    ql_gps_power_on();
    g_gps_uart_fd = uart_connect(UART_DEV_GPS_NAME, UART_DEV_GPS_BOUNDRATE, 
                        UART_DEV_GPS_FLOWCTRL, UART_DEV_GPS_PARITY);
    if (0 > g_gps_uart_fd)
    {
        LOG_E("[%s] open gps uart dev failed!", __func__);
        return -1;
    }

    ret = pthread_create(&ptd, NULL, gps_uart_recv_handler, NULL);
    if (0 != ret)
    {
        LOG_E("[%s] fail to create gps uart thread! ", __func__);
        return -1;
    }

    return 0;
}

int smt_mcu_send(int fd, NUINT8* buff, int len)
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
    
    send_packet_handle((void*)sp);
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

int smt_mcu_send_write(NUINT8 base, NUINT16 id, NUINT8 *value, NUINT32 len)
{
    int size, pos;
    NUINT8 *datazone = NULL;
    NUINT8 *pkt_buff = NULL;
    NUINT32 pkt_len = 0;

    if(value == NULL || len == 0)
    {
        LOG_E("value or len error.[value: %p, len: %d]", value, len);
        return -1;
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
    return smt_mcu_send(g_smt_mcu_fd, pkt_buff, pkt_len);

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

void smt_niu_data_write_value(NIU_DATA_ADDR_BASE base, NUINT32 id, NUINT8 *value, NUINT8 size)
{
    unsigned short write_data = 0;

    pthread_mutex_lock(&mutex_lock);
    if (value && size)
    {
        if (id < NIU_ID_ECU_MAX)
        {
            memcpy(&write_data, value, sizeof(unsigned short));
            //sync value to mcu, without these fields
            if(id != NIU_ID_ECU_CAR_TYPE && id != NIU_ID_ECU_TIZE_SIZE &&
                id != NIU_ID_ECU_LOST_ENERY && id != NIU_ID_ECU_LOST_SOC &&
                id != NIU_ID_ECU_LAST_MILEAGE && id != NIU_ID_ECU_TOTAL_MILEAGE)
            {
                if(smt_mcu_send_write(NIU_ECU, id, (NUINT8 *)&write_data, sizeof(unsigned short)) != 0)
                {
                    LOG_E("mcu_send_write error.[base: 0x%02X, id: 0x%04X]", base, id);
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex_lock);
}

static void *ant_main_sianal_handler(void* arg)
{
    int ret = 0;
    int signal_val = 0;
    unsigned char csq_value = 0;
    QL_MCM_NW_SIGNAL_STRENGTH_INFO_T t_info;

    while (1)
    {
        memset(&t_info, 0, sizeof(QL_MCM_NW_SIGNAL_STRENGTH_INFO_T));
        ret = QL_MCM_NW_GetSignalStrength(g_h_nw, &t_info);
        if(0 != ret) {
            LOG_E("QL_MCM_NW_GetSignalStrength failed, ret = %d", ret);
            return (void *)-1;
        }
        if(t_info.gsm_sig_info_valid) {
            signal_val = t_info.gsm_sig_info.rssi;
        } 
        if(t_info.wcdma_sig_info_valid) {
            signal_val = t_info.wcdma_sig_info.rssi;
        } 
        if(t_info.tdscdma_sig_info_valid) {
            signal_val = t_info.tdscdma_sig_info.rssi;
        } 
        if(t_info.lte_sig_info_valid) {
            signal_val = t_info.lte_sig_info.rssi;
        } 
        if(t_info.cdma_sig_info_valid) {
            signal_val = t_info.cdma_sig_info.rssi;
        } 
        if(t_info.hdr_sig_info_valid) {
            signal_val = t_info.hdr_sig_info.rssi;
        }
        csq_value = (signal_val + 113)/2;

        LOG_V("[%s] csq_value=%d",__func__, csq_value);
        smt_niu_data_write_value(NIU_ECU, NIU_ID_ECU_CSQ, (unsigned char *)&csq_value, sizeof(unsigned char));
        usleep(500000);
    }

    return (void *)0;
}

int niu_smt_ant_main_test_init(void)
{
    int ret = -1;
    pthread_t ptd;

    ret = QL_MCM_NW_Client_Init((uint32 *)&g_h_nw);
    if (0 != ret)
    {
        LOG_E("[%s] QL_MCM_NW_Client_Init failed! ", __func__);
        return ret;
    }

    ret = pthread_create(&ptd, NULL, ant_main_sianal_handler, NULL);
    if (0 != ret)
    {
        LOG_E("[%s] fail to create ant main thread! ", __func__);
        return -1;
    }

    return 0;
}

int niu_smt_pwrkey_test(void)
{
    FILE *fp = NULL;
    char buf[64] = {0};

    memset(buf, 0, sizeof(buf));
    fp = popen("cat /proc/sys/kernel/boot_reason", "r");
    fread(buf, 1, 63, fp);
    pclose(fp);

    if(strstr(buf, "7")==NULL)
    {
        return 0;
    }

    return -1;
}

int niu_smt_reset_test(void)
{
    FILE *fp = NULL;
    char buf1[64] = {0};
    char buf2[64] = {0};

    memset(buf1, 0, sizeof(buf1));
    fp = popen("cat /proc/sys/kernel/boot_reason", "r");
    fread(buf1, 1, 63, fp);
    pclose(fp);
    fp = NULL;

    system("echo 0x80C > /sys/kernel/debug/spmi/spmi-0/address");
    memset(buf2, 0, sizeof(buf2));
    fp = popen("cat /sys/kernel/debug/spmi/spmi-0/data_raw", "r");
    fread(buf2, 1, 63, fp);
    pclose(fp);

    if(NULL!=strstr(buf1, "1") && NULL!=strstr(buf2, "0x40"))
    {
        return 0;
    }

    return -1;
}

int niu_smt_usim_test(void)
{
#define BUF_SIZE 32
    int ret = -1;
    char imsi_buf[BUF_SIZE] = {0};
    char iccid_buf[BUF_SIZE] = {0};
    QL_SIM_APP_ID_INFO_T t_info;

    ret = QL_MCM_SIM_Client_Init((uint32 *)&g_h_sim);
    if (0 != ret)
    {
        LOG_E("[%s] QL_MCM_SIM_Client_Init failed! ", __func__);
        return -1;
    }
    
    memset(imsi_buf, 0, sizeof(imsi_buf));
    t_info.e_slot_id    = E_QL_MCM_SIM_SLOT_ID_1;
    t_info.e_app        = E_QL_MCM_SIM_APP_TYPE_3GPP;
    ret = QL_MCM_SIM_GetIMSI(g_h_sim, &t_info, imsi_buf, sizeof(imsi_buf));
    if (0 != ret)
    {
        LOG_E("[%s] QL_MCM_SIM_GetIMSI failed! ", __func__);
        return -1;
    }

    memset(iccid_buf, 0, sizeof(iccid_buf));
    ret = QL_MCM_SIM_GetICCID(g_h_sim, E_QL_MCM_SIM_SLOT_ID_1, iccid_buf, sizeof(iccid_buf));
    if (0 != ret)
    {
        LOG_E("[%s] QL_MCM_SIM_GetICCID failed! ", __func__);
        return -1;
    }

    if (0 >= strlen(imsi_buf) || 0 >= strlen(iccid_buf))
    {
        LOG_E("[%s] read USIM info failed! ", __func__);
        return -1;
    }

    LOG_V("[%s]imsi:%s, iccid:%s", __func__, imsi_buf, iccid_buf);
    return 0;
}

void* thread_uart_recv_mcu(void *arg)
{
    int ret;
    fd_set fdset;
    int read_bytes, total_bytes = 0;
    NUINT8 *buffer = NULL;
    struct timeval timeout = {3, 0};    
    
    pthread_detach(pthread_self());
    buffer = QL_MEM_ALLOC(1024);
    if (buffer == NULL)
    {
        LOG_E("malloc failed.!");
        return NULL;
    }
_loop:
    if(g_smt_mcu_fd > 0)
    {
        Ql_UART_Close(g_smt_mcu_fd);
        g_smt_mcu_fd = -1;
    }

    g_smt_mcu_fd = uart_connect(UART_DEV_STM_NAME, UART_DEV_STM_BOUNDRATE, UART_DEV_STM_FLOWCTRL, UART_DEV_STM_PARITY);  
    if(g_smt_mcu_fd < 0)
    {
        LOG_E("uart connect failed.");
        sleep(1);
        goto _loop;
    }

    while(1)
    {
        total_bytes = 0;
        FD_ZERO(&fdset);
		FD_SET(g_smt_mcu_fd, &fdset); 
        timeout.tv_sec  = 3;
		timeout.tv_usec = 0;
		ret = select(g_smt_mcu_fd + 1, &fdset, NULL, NULL, &timeout);
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
			if (FD_ISSET(g_smt_mcu_fd, &fdset)) 
			{
_rec:
                read_bytes = read(g_smt_mcu_fd, (char*)buffer + total_bytes, 100);
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
                if (total_bytes > sizeof(buffer) - 100)
                {
                    total_bytes = 0;
                    goto _rec;
                }
                LOG_V("[%s] recv from mcu: %s\n", __func__, buffer);
            }
        }
    }
    return NULL;
}

int smt_mcu_data_init()
{
    int ret = 0;
    pthread_t pid;

    ret = pthread_create(&pid, NULL, thread_uart_recv_mcu, NULL);
    if(ret)
    {
        LOG_E("create recv mcu msg thread error.");
        return -1;
    }
    sleep(1);

    return 0;
}

#define SMT_GYRO_REG_SEL    "/sys/bus/iio/devices/iio:device0/reg_sel"
#define SMT_GYRO_REG_VAL    "/sys/bus/iio/devices/iio:device0/reg_val"
int smt_write_gyro_reg(char *file_path, char *p_data, int len)
{
    int ret = -1;
    int smt_gyro_reg_fd = -1;

    if (NULL==file_path || NULL == p_data)
    {
        return -1;
    }

    smt_gyro_reg_fd = open(file_path, O_WRONLY);
    if (0 > smt_gyro_reg_fd)
    {
        return -1;
    }
    ret = write(smt_gyro_reg_fd, p_data, len);
    if (0 > ret)
    {
        LOG_E("[%s] write failed! \n", __func__);
        return ret;
    }

    close(smt_gyro_reg_fd);

    return 0;
}

int smt_read_gyro_reg(char *file_path, char *p_data, int len)
{
    int ret = -1;
    int smt_gyro_reg_fd = -1;

    if (NULL==file_path || NULL == p_data)
    {
        return -1;
    }

    smt_gyro_reg_fd = open(file_path, O_RDONLY);
    if (0 > smt_gyro_reg_fd)
    {
        return -1;
    }
    ret = read(smt_gyro_reg_fd, p_data, len);
    if (0 > ret)
    {
        LOG_E("[%s] read failed! \n", __func__);
        return ret;
    }

    close(smt_gyro_reg_fd);

    return 0;
}

int smt_gyro_int_test(void)
{
    FILE *fp = NULL;
    int ret = 0;
    int i = 0;
    char tmp_data[10] = {0};
    int tmp_interrupt_num[3] = {0};
    char gyro_ori_reg_val1[20] = {0};
    char gyro_ori_reg_val2[20] = {0};
    char gyro_reg_sel1[] = "0x50 3";
    char gyro_reg_sel2[] = "0x55 3";
    char gyro_new_reg_val1[] = "00 10 00";
    char gyro_new_reg_val2[] = "00 88 00";

    ret = smt_write_gyro_reg(SMT_GYRO_REG_SEL, gyro_reg_sel1, sizeof(gyro_reg_sel1));
    if (0 > ret)
    {
        LOG_E("[%s] write gyro reg1 failed! \n", __func__);
        return ret;
    }
    ret = smt_read_gyro_reg(SMT_GYRO_REG_VAL, gyro_ori_reg_val1, sizeof(gyro_ori_reg_val1));
    if (0 > ret)
    {
        LOG_E("[%s] read gyro ori reg val1 failed! \n", __func__);
        return ret;
    }
    ret = smt_write_gyro_reg(SMT_GYRO_REG_VAL, gyro_new_reg_val1, sizeof(gyro_new_reg_val1));
    if (0 > ret)
    {
        LOG_E("[%s] write gyro new val1 failed! \n", __func__);
        return ret;
    }

    ret = smt_write_gyro_reg(SMT_GYRO_REG_SEL, gyro_reg_sel2, sizeof(gyro_reg_sel2));
    if (0 > ret)
    {
        LOG_E("[%s] write gyro reg2 failed! \n", __func__);
        return ret;
    }
    ret = smt_read_gyro_reg(SMT_GYRO_REG_VAL, gyro_ori_reg_val2, sizeof(gyro_ori_reg_val2));
    if (0 > ret)
    {
        LOG_E("[%s] read gyro ori reg val2 failed! \n", __func__);
        return ret;
    }
    ret = smt_write_gyro_reg(SMT_GYRO_REG_VAL, gyro_new_reg_val2, sizeof(gyro_new_reg_val2));
    if (0 > ret)
    {
        LOG_E("[%s] write gyro new val2 failed! \n", __func__);
        return ret;
    }
    
    for (i=0; i<3; i++)
    {
        memset(tmp_data, 0, sizeof(tmp_data));
        fp = popen("cat /proc/interrupts | grep bmi_irq | awk '{print $2}'", "r");
        fread(tmp_data, 1, 127, fp);
        pclose(fp);
             
        tmp_interrupt_num[i] = atoi(tmp_data);
        LOG_V("[%s] tmp_interrupt_num[%d] = %d", __func__, i, tmp_interrupt_num[i]);
        usleep(500000);
    }

    if (tmp_interrupt_num[1]>tmp_interrupt_num[0] && tmp_interrupt_num[2] > tmp_interrupt_num[1])
    {
        ret = smt_write_gyro_reg(SMT_GYRO_REG_SEL, gyro_reg_sel1, sizeof(gyro_reg_sel1));
        if (0 > ret)
        {
            LOG_E("[%s] write gyro reg1 failed! \n", __func__);
            return ret;
        }
        ret = smt_write_gyro_reg(SMT_GYRO_REG_VAL, gyro_ori_reg_val1, sizeof(gyro_new_reg_val1));
        if (0 > ret)
        {
            LOG_E("[%s] write gyro new val1 failed! \n", __func__);
            return ret;
        }

        ret = smt_write_gyro_reg(SMT_GYRO_REG_SEL, gyro_reg_sel2, sizeof(gyro_reg_sel2));
        if (0 > ret)
        {
            LOG_E("[%s] write gyro reg2 failed! \n", __func__);
            return ret;
        }
        ret = smt_write_gyro_reg(SMT_GYRO_REG_VAL, gyro_ori_reg_val2, sizeof(gyro_new_reg_val2));
        if (0 > ret)
        {
            LOG_E("[%s] write gyro new val2 failed! \n", __func__);
            return ret;
        }

        LOG_D("[%s] gyro int detect success!", __func__);
        g_smt_test_result |= NIU_SMT_GYRO_INT;
    }

    return 0;
}

int niu_smt_check_file_exist(void)
{
    if (0 == access(NIU_SMT_FILE_PATH, F_OK)) //file existed
    {
        return 0;
    }

    return -1;
}

void start_niu_smt_test(int cmd)
{
    int niu_smt_cfg_fd = -1;
    
    niu_smt_cfg_fd = open(NIU_SMT_FILE_PATH, O_WRONLY|O_CREAT);
    if (-1 == niu_smt_cfg_fd)
    {
        LOG_E("[%s] open <%s> failed!", __func__, NIU_SMT_FILE_PATH);
    }
    close(niu_smt_cfg_fd);
    system("sync");
    printf("start niu smt test success! \n");

    LOG_D("[%s] MCU send cmd is 0x%04x ", __func__, cmd);
    if (0x0020 == cmd)  //not need reboot
    {
        system("pkill -9 niu_mgmt && sync");
    }
}

void niu_smt_test_exit(void)
{
    int ret = 0;
    char cmd_buf[64] = {0};

    ql_app_ready(EC25_INIT_OUT_PIN, PINLEVEL_HIGH);  //EC25_INIT_OUT
    ret = Ql_GPIO_Uninit(WAKEUP_OUT_PIN); //uninit wakeup out gpio pin
    LOG_D("[niu_smt_test_exit]uninit WAKEUP_OUT_PIN, ret=%d", ret);
    ret = Ql_GPIO_Uninit(WATCHDOG_EC25_PIN);  //uninit watchdog gpio pin
    LOG_D("[niu_smt_test_exit]uninit WATCHDOG_EC25_PIN, ret=%d\n", ret);
    // ret = Ql_EINT_Disable(WAKEUP_IN_PIN); //disable wakeup_in_pin irq
    ret = Ql_GPIO_Uninit(WAKEUP_IN_PIN);
    LOG_D("[niu_smt_test_exit]uninit WAKEUP_IN_PIN, ret=%d\n", ret); 

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "rm -r %s && sync", NIU_SMT_FILE_PATH);
    system(cmd_buf);    
    LOG_D("<<<smt test end>>>");
    system("pkill -9 niu_mgmt && sync");
}

void niu_smt_test_after_reset(void)
{
    int try_cnt = 0;
#if 0
    BMI_SENSOR_DATA gsensor_data;    
#endif
    g_smt_test_result = 0;
    // if (0 == niu_smt_reset_test())  //RESETB
    {
        LOG_D("<<<enter smt test after reset>>>");
        LOG_D("[%s] PWRKEY detect success!", __func__);
        g_smt_test_result |= NIU_SMT_PWRKEY;

        pthread_mutex_init(&mutex_lock,NULL);
        smt_mcu_data_init();
        niu_smt_ant_main_test_init();    //ANT_MAIN
        ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 500, 500); //EC21_STATUS
        ql_app_ready(EC25_INIT_OUT_PIN, PINLEVEL_LOW);  //EC25_INIT_OUT
        niu_smt_gpio_test_init();   //EC25_WATCHDOG, EC25_WAKEUP_OUT, EC25_WAKEUP_IN 
#if 0      
        smt_gyro_int_test();        //GYRO INT PINS
#endif
        niu_smt_gps_test_init();    //GPS PINS
#if 0
        gsensor_init();
#endif
        sleep(2);

_loop_usim_detect:        
        if (0 != niu_smt_usim_test())    //USIM PINS
        {
            if (try_cnt++ < 3)
            {
                sleep(1);
                goto _loop_usim_detect;
            }
            LOG_E("[%s] usim detect failed! ", __func__);
        }
        else
        {
            LOG_D("[%s] usim detect success!", __func__);
            g_smt_test_result |= NIU_SMT_USIM;
        }
        try_cnt = 0;
        
_loop_gps_ant_detect:
        if (ANT_NORMAL != ql_gps_ant_det()) //GPS_ANT_AD,GPS_ANT_DET
        {
            if (try_cnt++ < 3)
            {
                sleep(1);
                goto _loop_gps_ant_detect;
            }
            LOG_E("[%s] gps ant detect failed! ", __func__);
        }
        else
        {
            LOG_D("[%s] gps ant detect success!", __func__);
            g_smt_test_result |= NIU_SMT_GPS_ANT;
        }
        // ql_gps_power_off();        
        try_cnt = 0;
#if 0
_loop_gsensor_detect:
        gsensor_data_read(&gsensor_data);
        LOG_V("accdata[0]=%d, accdata[1]=%d, accdata[2]=%d, gdata[0]=%d, gdata[1]=%d, gdata[2]=%d", \
            gsensor_data.accdata[0], gsensor_data.accdata[1], gsensor_data.accdata[2], \
            gsensor_data.gyrodata[0], gsensor_data.gyrodata[1], gsensor_data.gyrodata[2]); 
        if (0 == gsensor_data.accdata[0]  && 0 == gsensor_data.accdata[1] && \
            0 == gsensor_data.accdata[2]  && 0 == gsensor_data.gyrodata[0] && \
            0 == gsensor_data.gyrodata[1] && 0 == gsensor_data.gyrodata[2])
        {
            if (try_cnt++ < 20)
            {
                sleep(1);
                goto _loop_gsensor_detect;
            }
            LOG_E("[%s] gsensor detect failed! ", __func__);
        }
        else
        {
            LOG_D("[%s] gsensor detect success!", __func__);
            g_smt_test_result |= NIU_SMT_GYRO;
        }           
#endif
        LOG_D("[%s] smt_test_result=0x%04x \n", __func__, g_smt_test_result);
        smt_niu_data_write_value(NIU_ECU, NIU_ID_ECU_SMT_TEST, (unsigned char*)&g_smt_test_result, sizeof(g_smt_test_result));
        sleep(1);
    }
    niu_smt_test_exit();

    exit(0);
}


