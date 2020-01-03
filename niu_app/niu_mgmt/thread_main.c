/*-----------------------------------------------------------------------------------------------*/
/**
  @file thread_main.c
  @brief Processing center
*/
/*-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
  EDIT HISTORY
  This section contains comments describing changes made to the file.
  Notice that changes are listed in reverse chronological order.
  $Header: $
  when       who          what, where, why
  --------   ---          ----------------------------------------------------------
  20180809   tyler.kuang  Created .
-------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <ql_oe.h>
#include "service.h"
#include "ql_cmdq.h"
#include "sys_core.h"
#include "ql_virtual_at.h"
#include "niu_thread.h"
#include "MQTTClient.h"
#include "niu_data.h"
#include "niu_entry.h"
#include "mcu_data.h"
#include "niu_data_pri.h"
#include "ql_customer.h"
#include "utils.h"
#include "ql_mcm_nw.h"
#include "upload_error_lbs.h"
#include "niu_fota.h"
#include "low_power.h"
#include "niu_version.h"

#define SYS_HANDLER_MAX_LEN_DEFAULT 20
#define SYS_MAX_PEEDING_TIME_MS (60*1000)
int g_data_init = 0;     //the flag of updating csq
int g_mqtt_ping_timeout; //mqtt ping time out
ql_cmdq_handle_t g_sys_handler;
data_registration_state_info_t g_data_reg_state_info;
void sys_at_cmd_handler(const char *cmd);
void sys_event_handler(SYS_EVENT_E cmd, void *param, int param_len);
extern int g_sub_topic_flag;
extern void async_task_handler(SYS_EVENT_E evt, void *param, int len);

ql_value_name_item_t g_vn_sys_event[] = {
    {SYS_EVENT_SYS_INIT_SUCCEED, "system init successful"},
    {SYS_EVENT_DATA_REG_SUCCEED, "data registration successful"},
    {SYS_EVENT_DATA_REG_FAILED,  "data registration failure"},
    {SYS_EVENT_NET_CONNECTED, "net connect successful"},
    {SYS_EVENT_NET_DISCONNECT, "net connect failure"},
    {SYS_EVENT_SOCKET_CONNECTED, "socket connect successful"},
    {SYS_EVENT_SOCKET_DISCONNECT, "socket connect failure"},
    {MQTT_EVENT_MQTT_CONNECTED, "mqtt connect successful"},
    {MQTT_EVENT_MQTT_DISCONNECT, "mqtt connect failure"},
    {MQTT_EVENT_MQTT_SUB_SUCCEED, "mqtt sub topic successful"},
    {MQTT_EVENT_MQTT_SUB_FAILED, "mqtt sub topic failure"},
    {MQTT_EVENT_PING_REQ_START, "mqtt ping req start"},
    {MQTT_EVENT_PING_RESP_FAILED, "mqtt ping resp failure"},
    {MQTT_EVENT_PING_RESP_SUCCEED, "mqtt ping resq successful"},
    {MQTT_EVENT_SOCKET_READ_ERROR, "mqtt socket read error"},
    {MQTT_EVENT_SOCKET_SERVER_CLOSE, "mqtt socket server closed"},
    {MQTT_EVENT_TEMPLATE_UPDATE, "mqtt template update"},
    {FOTA_EVENT_CHECK_COMPLETED, "fota check completed"},
    {FOTA_EVENT_DOWNLOAD_COMPLETED, "fota download completed"},
    {FOTA_EVENT_UPGRADE_DEVICE_COMPLETED, "fota upgrade device completed"},
    {FOTA_EVENT_UPGRADE_COMPLETED, "fota upgrade completed"},
    {FOTA_EVENT_UPLOAD_LOG_1_COMPLETED, "fota upload log 1 completed"},
    {FOTA_EVENT_UPLOAD_LOG_1_AGAIN, "fota upload log 1 again"},
    {FOTA_EVENT_UPLOAD_LOG_2_COMPLETED, "fota upload log 2 completed"},
    {FOTA_EVENT_UPLOAD_LOG_2_AGAIN, "fota upload log 2 again"},
    {FOTA_EVENT_UPGRADE_FILE_TYPE_ERROR, "upgrade file type error"},
    {FOTA_EVENT_UPGRADE_EC25_TO_STM32_RATE, "fota upgrade (ec25->stm32) package"},
    {FOTA_EVENT_UPGRADE_EC25_TO_STM32_FINISH, "fota upgrade (ec25->stm32) finish"},
    {FOTA_EVENT_UPGRADE_STM32_TO_EC25_RATE, "fota upgrade (stm32->ec25) package"},
    {FOTA_EVENT_UPGRADE_STM32_TO_EC25_FINISH, "fota upgrade (stm32->ec25) finish"},
    {-1, NULL}
};


#define GET_SYS_EVT_STR(cmd) ql_v2n(g_vn_sys_event, cmd)

void tmr_cmd_handler_deadlock_check(int tmr_id, void *param)
{
    if(cmd_handler_peeding_time_ms() >= SYS_MAX_PEEDING_TIME_MS) {
        LOG_F("cmd handler deadlocks");
        sys_abort(-1, "deadlocks ocurred");
    }
}

void tmr_cmd_handler_feed_dog(int tmr_id, void *param)
{
    if(tmr_id == TIMER_ID_FEED_DOG)
    {
        ql_gpio_feed_dog(100);
    }
}

void tmr_cmd_handler_update_timestamp(int tmr_id, void *param)
{
    unsigned int new_value = 0;
    new_value = time(NULL) - DIFF_TIME;
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_TIMESTAMP, (NUINT8*)&new_value, sizeof(NINT8));
}

int main_thread(void)
{
    int ret = 0;
    int max_fd = -1;
    int sys_fd = -1;
    int at_fd = -1;
    fd_set rfds; 
    struct timeval tm = {0};
    ql_cmdq_item_t *item = NULL;
    char buf[256];
    memset(&g_data_reg_state_info, 0, sizeof(data_registration_state_info_t));
    ql_mutex_init(&g_data_reg_state_info.mutex, "data mutex");
    g_data_reg_state_info.is_registration = 0;
    g_data_reg_state_info.net_state = SYS_NET_STATUS_NONE;
    g_data_reg_state_info.profile_id = 1;
    g_data_reg_state_info.ip_family = QL_DATA_CALL_TYPE_IPV4;
    
    ret = ql_cmdq_init(&g_sys_handler, SYS_HANDLER_MAX_LEN_DEFAULT);
    if(ret != 0) {
        LOG_F("Failed to ql_cmdq_init, ret=%d", ret);
        sys_abort(-1, "init cmdq error");
    }
    sys_fd = ql_cmdq_get_fd(&g_sys_handler); 

    ret = cmd_handler_init();
    if(ret != 0) {
        LOG_F("Failed to cmd_handler_init");
        return -1;
    }

    ret = tmr_handler_init();
    if(ret != 0) {
        LOG_F("Failed to tmr_handler_init");
        return -1;
    }

    LOG_D("version: %s, build at %s.", NIU_VERSION, NIU_BUILD_TIMESTAMP);

    /** Check for deadlocks of cmd handler */
    tmr_handler_tmr_add(TIMER_ID_HANDLER_DEADLOCK_CHECK, tmr_cmd_handler_deadlock_check, NULL,2000, 1);

    /** start system init */
    clear_niu_upl_cmd();
    fota_init();
    error_lbs_init();
    mqtt_sub_preinit();
    ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 1000, 0);
    cmd_handler_push(cmd_sys_init, NULL); 
    while(1) {
        max_fd = -1;
        FD_ZERO(&rfds);

        tm.tv_sec = 10;
        tm.tv_usec = 0;

        FD_SET(sys_fd, &rfds);
        max_fd = max_fd > sys_fd ? max_fd : sys_fd;

        FD_SET(at_fd, &rfds);
        max_fd = max_fd > at_fd ? max_fd : at_fd;

        ret = select(max_fd + 1, &rfds, NULL, NULL, &tm);
        if(ret < 0)  {
            if(errno == EINTR) {
                continue;
            }
            LOG_F("Failed to select, err=%s", strerror(errno));
            sys_abort(-1, "system error");
        }
        else if(ret == 0) {
            continue;
        }

        if(FD_ISSET(sys_fd, &rfds)) {
            item = NULL;
            ret = ql_cmdq_pull(&g_sys_handler, 0, &item);
            if(ret<0) {
                LOG_F("Failed to thread_cmd_handler_proc, ret=%d", ret);
                sys_abort(-1, "system error");
            }

            if(item != NULL) {
                if((SYS_EVENT_E)item->cmd==SYS_EVENT_ASYNC_ON_PROGRESS || (SYS_EVENT_E)item->cmd==SYS_EVENT_ASYNC_ON_RESULT) {
                    async_task_handler((SYS_EVENT_E)item->cmd, item->param, item->param_len);
                }
                else {
                    sys_event_handler((SYS_EVENT_E)item->cmd, item->param, item->param_len);
                }
                ql_cmdq_item_free(&g_sys_handler, item);
            }
        }
        else if(FD_ISSET(at_fd, &rfds)) {
            ret = ql_at_read(buf, sizeof(buf));
            if(ret < 0) {
                LOG_F("Failed to ql_at_read");
                sys_abort(-1, "at read error");
            }
            else if(ret >= sizeof(buf)) {
                LOG_E("Size of AT read buffer is not enough");
                continue;
            }
            buf[ret] = 0;
            sys_at_cmd_handler(buf);
        }
    }

    return 0;
}

void sys_at_cmd_handler(const char *cmd)
{
    LOG_D("Rece AT cmd : %s", cmd);
    ql_at_print("OK\r\nI am ql_poc\r\n");
}

void sys_event_push(SYS_EVENT_E cmd, void *param, int param_len)
{
    int ret = 0;
    ret = ql_cmdq_cmd_push(&g_sys_handler, (void*)cmd, param, param_len);
    if(ret != 0) {
        LOG_F("Failed to push system event, ret=%d", ret);
        sys_abort(-1, "push system event failed");
    }
}

void sys_event_handler(SYS_EVENT_E cmd, void *param, int param_len)
{
    LOG_V("enter");
    if(cmd == MQTT_EVENT_SOCKET_READ_ERROR) {
        LOG_D("Recv events : %d:%s,errno is %d", cmd, GET_SYS_EVT_STR(cmd), *((int *)param));
    }
    else {
        LOG_D("Recv events : %d:%s", cmd, GET_SYS_EVT_STR(cmd));
    }
    switch(cmd) {
        case SYS_EVENT_SYS_INIT_SUCCEED:
        {
            struct timespec interval_time;
            interval_time.tv_sec = 60;
            interval_time.tv_nsec = 0;

            low_power_init();
            low_power_check(LOCK_TYPE_WATCHDOG, interval_time);
            cmd_handler_push(cmd_register_info_get, NULL);
            niu_entry();
            
            LOG_D("call ql_app_ready.");

            niu_data_config_sync_flag_set(0);
            niu_data_config_sync_mask_reset();

            niu_ecu_data_sync();
            //tmr_handler_tmr_add(TIMER_ID_TIMESTAMP, tmr_cmd_handler_update_timestamp, NULL, 1000, 1);
            g_data_init = 1;
            break;
        }
        case SYS_EVENT_DATA_REG_SUCCEED:
            cmd_handler_push(cmd_data_call_start, NULL);
            break;
        /*case SYS_EVENT_DATA_REG_FAILED:
            set_mqtt_disconnected();
            sleepdown_connect_thread();
            cmd_handler_push(cmd_data_call_stop, NULL);
            break;
        */
        case SYS_EVENT_NET_CONNECTED:
        {   
            int retry = 30;
            NIU_DATA_VALUE *temp = NULL;
            if(g_sub_topic_flag)
            {
                return;
            }
            //send read commond to mcu, get all config fields
            
            //niu_data_all_config_fields_req();

            // while(retry > 0)
            // {
            //     int mask = niu_data_config_sync_mask_get();
            //     if(mask == NIU_MASK_SYNC_FLAG_ALL)
            //     {
            //         break;
            //     }
            //     usleep(100*1000);
            //     retry--;
            // }
            // if(retry == 0)
            // {
            //     LOG_D("sync config from mcu failed.");
            //     return;
            // }
            // else
            // {
            //     LOG_D("=====================sync config from mcu success.=====================");
            // }

            
            reset_4g_count_update(1);
            wakeup_connect_thread();
            temp = NIU_DATA_ECU(NIU_ID_ECU_ECU_KEEPALIVE);
            if(temp)
            {
                //g_mqtt_ping_timeout = *((NUINT32 *)(temp->addr)) * 1000 / 2; //baron
                memcpy(&g_mqtt_ping_timeout, temp->addr, temp->len);
                g_mqtt_ping_timeout = g_mqtt_ping_timeout * 1000 / 2;
            }
            LOG_D("g_mqtt_ping_timeout: %d", g_mqtt_ping_timeout);
            if(g_mqtt_ping_timeout == 0)
            {
                g_mqtt_ping_timeout = 60*1000;
            }
            lbs_upload(); //baron
            break;
        }
        case SYS_EVENT_NET_DISCONNECT:
            reset_socket(2);
            reset_4g_count_update(0);
            sleepdown_connect_thread();
            break;
        case MQTT_EVENT_SOCKET_SERVER_CLOSE:
            error_upload(NIU_ERROR_DISCONNECT);
            reset_socket(1);
            wakeup_connect_thread();
            break;
        case MQTT_EVENT_SOCKET_READ_ERROR:
            reset_socket(2);
            wakeup_connect_thread();
            break;
        case MQTT_EVENT_PING_RESP_SUCCEED:
        {
            struct timespec wakeup_time;
            NIU_DATA_VALUE *temp = NULL;

            delete_ping_timer();
            temp = NIU_DATA_ECU(NIU_ID_ECU_ECU_KEEPALIVE);
            if(temp)
            {
                //g_mqtt_ping_timeout = *((NUINT32 *)(temp->addr)) * 1000 / 2; //baron
                memcpy(&g_mqtt_ping_timeout, temp->addr, temp->len);
                g_mqtt_ping_timeout = g_mqtt_ping_timeout * 1000 / 2;
            }
            LOG_D("g_mqtt_ping_timeout: %d", g_mqtt_ping_timeout);
            if(g_mqtt_ping_timeout == 0)
            {
                g_mqtt_ping_timeout = 60*1000;
            }
            wakeup_time.tv_sec = g_mqtt_ping_timeout / 1000 * 2;
            wakeup_time.tv_nsec = 0;
            low_power_check(LOCK_TYPE_HEARTBEAT, wakeup_time);
            break;
        }
        case MQTT_EVENT_PING_RESP_FAILED:
            if(1 == is_mqtt_ping()) {
                reset_socket(3);
                wakeup_connect_thread();
            }
            break;
        case MQTT_EVENT_PING_REQ_START:
        {
            unsigned int value = 0;
            value = time(NULL) - DIFF_TIME;
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SERVER_KA_TIMEOUT_TIME, (NUINT8 *)&value, 4);
            tmr_handler_tmr_add(TIMER_ID_PINGRESP, tmr_cmd_handler_mqtt_ping_req, NULL, g_mqtt_ping_timeout, 0);
            break;
        }
        case FOTA_EVENT_CHECK_COMPLETED:
            usleep(10 * 1000);
            wakeup_fota_thread(FOTA_DOWNLOAD);
            break;
        case FOTA_EVENT_UPLOAD_LOG_1_AGAIN:
        case FOTA_EVENT_DOWNLOAD_COMPLETED:
            usleep(10 * 1000);
            wakeup_fota_thread(FOTA_UPLOAD_LOG_1);
            break;
        case FOTA_EVENT_UPLOAD_LOG_1_COMPLETED:
            usleep(10 * 1000);
            fota_upgrade_devices();
            break;
        case FOTA_EVENT_UPGRADE_DEVICE_COMPLETED:
            set_fota_upgrade_device_result(param, param_len);
            break;
        case FOTA_EVENT_UPLOAD_LOG_2_AGAIN:
        case FOTA_EVENT_UPGRADE_COMPLETED:
            wakeup_fota_thread(FOTA_UPLOAD_LOG_2);
            break;
        case FOTA_EVENT_UPLOAD_LOG_2_COMPLETED:
        {
            struct timespec interval_time;
            memset(&interval_time, 0, sizeof(struct timespec));
            low_power_check(LOCK_TYPE_GSENSOR, interval_time);
            unsigned char is_reboot = *((unsigned char *)param);
            if(1 == is_reboot) {
                LOG_D("system reboot after fota upgrade");
                exit(0);
            }
            break;
        }
        case FOTA_EVENT_UPGRADE_FILE_TYPE_ERROR:
            break;
        //ec25 to stm32
        case FOTA_EVENT_UPGRADE_EC25_TO_STM32_RATE:
        {
            NUINT16 percent = 0;
            if(param != NULL && param_len == sizeof(NUINT16))
            {
                percent = *(NUINT16*)param;
                LOG_D("fota upgrade [ec25->stm32] percent: %d%%", percent);
                QL_MEM_FREE(param);
            }
        }
        break;
        case FOTA_EVENT_UPGRADE_EC25_TO_STM32_FINISH:
        {
            LOG_D("fota upgrade [ec25->stm32] success.");
        }
        break;
        //stm32 to ec25
        case FOTA_EVENT_UPGRADE_STM32_TO_EC25_RATE:
        case FOTA_EVENT_CONFIG_STM32_TO_EC25_RATE:
        {
            NUINT16 percent = 0;
            if(param != NULL && param_len == sizeof(NUINT16))
            {
                percent = *(NUINT16*)param;
                LOG_D("fota upgrade [stm32->ec25] percent: %d%%", percent);
                QL_MEM_FREE(param);
            }
        }
        break;
        case FOTA_EVENT_UPGRADE_STM32_TO_EC25_FINISH:
        {
            int status;
            unsigned char key[16] = {0};
            LOG_D("fota upgrade [stm32->ec25] success.");
            //decrypt
            if(ql_com_file_exist(FOTA_UPGRADE_FILE_EC25))
            {
                LOG_E("%s is not exist.", FOTA_UPGRADE_FILE_EC25);
            }
            else
            {
                get_ec25_tea_key(key, sizeof(key));
                status = TEA_decrypt_file(FOTA_UPGRADE_FILE_EC25, FOTA_UPGRADE_FILE_EC25_DECRYPT, key);
                if(status >= 0)
                {
                    //decrypt success, then delete orig file
                    LOG_D("%s decrypt success.", FOTA_UPGRADE_FILE_EC25_DECRYPT);
                    unlink(FOTA_UPGRADE_FILE_EC25);
                    if(ql_com_file_exist(FOTA_BACKUP_PATH))
                    {
                        LOG_D("%s is not exist.", FOTA_BACKUP_PATH);
                        status = mkdir(FOTA_BACKUP_PATH, 0777);
                        if(status)
                        {
                            LOG_E("mkdir error.[%s]", FOTA_BACKUP_PATH);
                        }
                    }
                    status = ql_com_system("mv %s %s", APP_EXEC, FOTA_BACKUP_EXEC);
                    if(WIFEXITED(status))
                    {
                        if(WEXITSTATUS(status))
                        {
                            LOG_E("cmd error, [mv %s %s][error:%d]", APP_EXEC, FOTA_BACKUP_EXEC, WEXITSTATUS(status));
                        }
                    }
                    status = ql_com_system("mv %s %s", FOTA_UPGRADE_FILE_EC25_DECRYPT, APP_EXEC);
                    if(WIFEXITED(status))
                    {
                        if(WEXITSTATUS(status))
                        {
                            LOG_E("cmd error, [mv %s %s][error:%d]", FOTA_UPGRADE_FILE_EC25_DECRYPT, APP_EXEC, WEXITSTATUS(status));
                        }
                    }
                    if(chmod(APP_EXEC, S_IXUSR | S_IWUSR | S_IRUSR))
                    {
                        LOG_E("chmod failed.");
                    }
                    else
                    {
                        //quit, then niu_daemon will backup old file and replace upgrade file as a new one, then start run
                        exit(0);
                    }
                }
                else
                {
                    LOG_D("decrypt file error[%s][status=%d].", FOTA_UPGRADE_FILE_EC25, status);
                }
            }
        }
        break;

        case FOTA_EVENT_CONFIG_STM32_TO_EC25_FINISH:
        {
            //config file upload success

            LOG_D("config upgrade [stm32->ec25] success.");
            if(ql_com_file_exist(CONFIG_UPGRADE_FILE_EC25))
            {
                LOG_E("file `%s` is not exist.", CONFIG_UPGRADE_FILE_EC25);
            }
            else
            {
                if(niu_config_data_check(CONFIG_UPGRADE_FILE_EC25) == 0)
                {
                    int ret, mask = 0;
                    int retry = 50; //timeout 5 seconds
                    niu_data_config_sync_flag_set(1);
                    niu_data_config_sync_mask_reset();
                    //Synchronize all fields in v3 table which contained in configuration files to MCU
                    ret = niu_config_data_sync_to_niu_data(CONFIG_UPGRADE_FILE_EC25);
                    if(ret)
                    {
                        LOG_E("sync json config fields to mcu failed.");
                        break;
                    }
                    sleep(1);
                    niu_data_all_config_fields_req();

                    while(retry > 0)
                    {
                        mask = niu_data_config_sync_mask_get();
                        if(mask == NIU_MASK_SYNC_FLAG_ALL)
                        {
                            LOG_D("mask %04x", mask);
                            break;
                        }
                        usleep(100*1000);
                        retry--;
                    }
                    niu_data_config_sync_flag_set(0);
                    
                    if(retry == 0)
                    {
                        LOG_E("Synchronize all config fields to MCU failed[mask: 0x%04x].", mask);
                    }
                    LOG_D("restart app.");
                    sleep(1);
                    exit(0);
                }
            }
            //baron.qian 2019/06/13 start: custom modify config json file save policy.
            //                             the config value saved in mcu, ec25 did not
            //                             save yet.
            #if 0
            if(ql_com_file_exist(CONFIG_UPGRADE_FILE_EC25))
            {
                LOG_E("file `%s` is not exist.", CONFIG_UPGRADE_FILE_EC25);
            }
            else
            {
                if(niu_config_data_check(CONFIG_UPGRADE_FILE_EC25) == 0)
                {
                    //config file is valid.
                    //backup config file first.
                    cmd_imei_info_get(imei, sizeof(imei));
                    snprintf(buff, sizeof(buff), "%s/%s.json", CONFIG_FILE_PREFIX, imei);
                    if(ql_com_file_exist(buff) == 0)
                    {
                        status = ql_com_system("cp -a %s %s", buff, FOTA_BACKUP_PATH);
                        if(WIFEXITED(status))
                        {
                            if(WEXITSTATUS(status))
                            {
                                LOG_E("cmd error, [cp -a %s %s][error:%d]", buff, FOTA_BACKUP_PATH, WEXITSTATUS(status));
                            }
                        }
                    }
                    
                    //copy new config file to /home/root/xn_config
                    status = ql_com_system("cp -a %s %s/%s.json", CONFIG_UPGRADE_FILE_EC25, CONFIG_FILE_PREFIX, imei);
                    if(WIFEXITED(status))
                    {
                        if(WEXITSTATUS(status))
                        {
                            LOG_E("cmd error, [cp -a %s %s/%s.json][error:%d]", CONFIG_UPGRADE_FILE_EC25, CONFIG_FILE_PREFIX, imei, WEXITSTATUS(status));
                        }
                    }
                    //copy new config file to /usrdata/xn_config
                    status = ql_com_system("cp -a %s %s/%s.json", CONFIG_UPGRADE_FILE_EC25, CONFIG_FILE_PREFIX2, imei);
                    if(WIFEXITED(status))
                    {
                        if(WEXITSTATUS(status))
                        {
                            LOG_E("cmd error, [cp -a %s %s/%s.json][error:%d]", CONFIG_UPGRADE_FILE_EC25, CONFIG_FILE_PREFIX2, imei, WEXITSTATUS(status));
                        }
                    }
                    //sync config value to rw
                    snprintf(buff, sizeof(buff), "%s/%s.json", CONFIG_FILE_PREFIX, imei);
                    niu_config_data_sync_to_rw(buff);
                    unlink(CONFIG_UPGRADE_FILE_EC25);
                    LOG_D("config download success, reboot.");
                    sleep(1);
                    exit(0);
                }
                else
                {
                    LOG_D("file `%s` is invalid, skip", CONFIG_UPGRADE_FILE_EC25);
                }
            }
            #endif
                //baron.qian 2019/06/13 end.
        }
        break;
        default:
           break; 
    }
}

void sys_abort(int status, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    strncat(buf, "\n", sizeof(buf));

    LOG_F("System exit, status=%d, reason=%s", status, buf);
    fprintf(stderr, "System exit, status=%d, reason=%s", status, buf);
    abort();
}

