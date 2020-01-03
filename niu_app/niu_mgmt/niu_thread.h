#ifndef __NIU_THREAD_H__
#define __NIU_THREAD_H__
#include <pthread.h>
#include "MQTTClient.h"
#include "niu_data_pri.h"
#include "service.h"
#include "com_list.h"
#include "niu_types.h"
#include "niu_data.h"


#define SECTION_LEN 16
#define MQTT_BUF_LEN 2048
#define UPGRADE_ADDR_LEN 128
#define UPGRADE_FILE_VERSION_LEN 32
#define UPGRADE_RETRY_NUM 5
#define HISTORY_MAX_NUM 500
#define BASE_STATION_MAX_NUM 10


#define SUB_TOPIC_WRITE "write"
#define SUB_TOPIC_COMMAND "cmd"
#define SUB_TOPIC_TEMPLATE "template"
#define SUB_TOPIC_GPS_CMD "gps_cmd"

#define PUB_TOPIC_GPS "gps"
#define PUB_TOPIC_LOG "ecu_log"

typedef enum {
    UPGRADE_FROM_SERVER = 0,
    UPGRADE_FROM_SINGLECHIP
} UPGRADE_TYPE;

typedef enum {
    HISTORY_DATA_GPS = 0,
    HISTORY_DATA_LOG,
    HISTORY_DATA_TEMPLATE
} HISTORY_DATA_TYPE;
typedef enum {
    UPGRADE_FILE_EC25_APP = 0,
    UPGRADE_FILE_EC25_SYS,
    UPGRADE_FILE_MCU_APP
} UPGRADE_FILE_TYPE;

typedef struct upload_data_struct {
    HISTORY_DATA_TYPE data_type;
    int packed_name;
    int length;
    unsigned char *data;
    time_t data_time;
}history_data_t;

/*flag=0,gps; flag=1,log*/
typedef struct upload_gps_log_struct {
    int flag;
    int data_len;
    char *data;
}upload_gps_log_t;

typedef struct list_test_struct {
    int num;
    char letter;
    float *floatp;
    com_list_head_t myself;
}list_test_t;

typedef struct upgrade_info_struct {
    UPGRADE_TYPE upgrade_type;
    UPGRADE_FILE_TYPE file_type;
    char file_addr[UPGRADE_ADDR_LEN];
    char file_md5[33];
    char file_version[UPGRADE_FILE_VERSION_LEN];
}upgrade_info_t;

typedef struct base_info_struct {
    unsigned short mcc;
    unsigned short mnc;
    unsigned short lac;
    unsigned short cellid;
    short signal;
}base_info_t;

typedef struct base_info_struct_list {
    int base_count;
    base_info_t base_list[BASE_STATION_MAX_NUM];
}base_info_list_t;

int do_system(const char *cmd, int retry);
int payload_pack_encrypt(NIU_DATA_VALUE **data_array, unsigned int data_array_size, unsigned short packed_num, unsigned char *result_data, int result_data_len);
void put_data_to_history(unsigned char *data, int length, int packed_name, HISTORY_DATA_TYPE data_type);
int payload_encrypt(unsigned char *data, int data_len, unsigned char *result_data, int result_data_len);
int payload_decrypt(unsigned char *data, int data_len, unsigned char *result_data, int result_data_len);
int mqtt_sub_preinit();

void upload_history_data();
void upload_gps_log_data(void *param);
void upload_log_info(char *format, ...);
void upload_gps_info(char *format, ...);
int is_mqtt_connected();
int is_mqtt_ping();
void set_mqtt_disconnected();
void reset_socket(unsigned int error_flag);
int get_4g_temperature();
void reset_4g_count_update(int flag);
void sleepdown_connect_thread();
void wakeup_connect_thread();

void write_base_station_to_datatable();
int get_mcc_mnc_value(char *plmn, int plmn_len, unsigned short *mcc, unsigned short *mnc);
void get_base_info(char *plmn, int plmn_len, unsigned short lac, unsigned int cellid);

void read_base_info(base_info_list_t *base_info);
void tmr_cmd_handler_mqtt_ping_req(int tmr_id, void *param);
void tmr_cmd_handler_get_cell_info(int tmr_id, void *param);
void tmr_cmd_handler_fota_thread_wakeup(int tmr_id, void *param);
void delete_ping_timer();

void printf_lte_info(QL_MCM_NW_LTE_INFO_T lte_info);
void printf_umts_info(QL_MCM_NW_UMTS_INFO_T umts_info);
void printf_gsm_info(QL_MCM_NW_GSM_INFO_T gsm_info);
void mqtt_client_keep_alive(int tmr_id, void *param);
#endif
