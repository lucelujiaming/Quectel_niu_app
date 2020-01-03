#ifndef __NIU_FOTA_H__
#define __NIU_FOTA_H__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "service.h"
#include "cJSON.h"
#include "utils.h"
#define FOTA_DEVICES_MAX_NUM 20

#define FOTA_CHECK_METHOD_NONE      (0)
#define FOTA_CHECK_METHOD_AUTO      (1)
#define FOTA_CHECK_METHOD_MANUAL    (2)

typedef enum {
    UPGRADE_CODE_SUCCESS = 0,
    UPGRADE_CODE_DOWNLOAD_FAIL = 1001,
    UPGRADE_CODE_AUTH_FAIL = 1002,
    UPGRADE_CODE_INSTALL_FAIL = 1003,
    UPGRADE_CODE_OTHER_ERROR = 1010,
    UPGRADE_CODE_NO_UPGRADE = 2001,
    UPGRADE_CODE_DOWNLOAD_SUCCESS = 2002,
    UPGRADE_CODE_INIT = 10000,
} UPGRADE_RESULT_CODE;

typedef enum {
    FOTA_NULL = 0,
    FOTA_CHECK,
    FOTA_DOWNLOAD,
    FOTA_UPLOAD_LOG_1,
    FOTA_UPGRADE,
    FOTA_UPLOAD_LOG_2
} FOTA_OPERATION_TYPE;

typedef struct fota_device_info_struct {
    char hasnew;
    char enable_upgrade;
    char type[16];
    int package_size;
    char package_type[8];
    char url[128];
    char md5[33];
    char version[16];
    char before_soft_version[48];
    char hard_version[48];
    void (*fota_upgrade_cb)(int);
    UPGRADE_RESULT_CODE result;
}fota_device_info_t;
typedef struct all_fota_devices_info_struct {
    int devices_num;
    fota_device_info_t devices[FOTA_DEVICES_MAX_NUM];
}all_fota_devices_info_t;

typedef struct fota_upgrade_device_complated_sturct {
    char type[16];
    UPGRADE_RESULT_CODE result;
}fota_upgrade_device_completed_t;

typedef struct fota_device_upgrade_cb_struct {
    char type[16];
    char enable_upgrade;
    void (*device_upgrade_cb)(int);
}fota_device_upgrade_cb_t;

int fota_init();

void wakeup_fota_thread(FOTA_OPERATION_TYPE flag);

void sleepdown_fota_thread();

void create_fota_check_req_data(NUINT32 method);

void create_fota_log_req_data(NUINT32 method);

void send_req_to_fota_server();

int read_resq_from_fota_server(int socket_fd);

int fota_response_data_parse(char *data, int data_len, char *json_data, int json_data_len);

void clear_fota_devices();

int get_fota_response_result(char *data, int data_len);

void get_sys_version(char *sys_version, int sys_version_legnth);

void get_soft_hard_version(char *device_type, int index);

unsigned char get_fota_download_percent();

void add_niu_data_to_json(cJSON *json_obj, char *label, int base, int id);

int execute_wget(char *cmd, int length);

void delete_file(char *path);

void get_last_item(char *path, int path_len, char splitter, char *get_str, int get_str_len);

void fota_download_file();

void fota_upgrade_devices();

void get_execute_file_name(char *process_name, int length);

void get_md5_value(char *string, char *md5value, int md5value_len, int flag);

void add_device_to_log_json(cJSON* obj, char *device_type);

void get_random_string(char *random_string, int random_len);

void get_fota_url_param(char *url_param, int url_param_len);

int encrypt_data_in_cbc_mode(unsigned char *s_data, int s_data_len, unsigned char *iv, int iv_len, unsigned char *key, int key_len, unsigned char *d_data, int d_data_len);

int decrypt_data_in_cbc_mode(unsigned char *s_data, int s_data_len, unsigned char *iv, int iv_len, unsigned char *key, int key_len, unsigned char *d_data, int d_data_len);

void printf_data(unsigned char *data, int data_len, int num_in_line);

void db_upgrade_cb(int index);

void foc_upgrade_cb(int index);

void bms_upgrade_cb(int index);

void bms2_upgrade_cb(int index);

void alarm_upgrade_cb(int index);

void lcu_upgrade_cb(int index);

void qc_upgrade_cb(int index);

void lku_upgrade_cb(int index);

void bcs_upgrade_cb(int index);

void tbs_upgrade_cb(int index);

void cpm_upgrade_cb(int index);

void emcu_upgrade_cb(int index);

void ecu_ex_app_upgrade_cb(int index);

void ecu_ex_sys_upgrade_cb(int index);

void set_fota_upgrade_device_result(void *param, int len);

void check_all_devices_upgrade_completed();

int get_mtd_num();

#endif
