#include <string.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ctype.h>
#include "niu_fota.h"
#include "upload_error_lbs.h"
#include "niu_thread.h"
#include "niu_data.h"
#include "niu_data_pri.h"
#include "aes2.h"
//#include "base64.h"
#include "niu_version.h"
#include "mcu_data.h"
#include "utils.h"
#include "low_power.h"

#define RECONNECT_NUM 3
#define RESEND_NUM 3
#define REREAD_NUM 3
#define DEFAULT_MTD_NUM 15
#define FOTA_INTERVAL_TIME 150*1000
#define FOTA_CHECK_SERVER_URL "fota.niu.com"
#define FOTA_CHECK_SERVER_PORT 80
#define FOTA_LOG_UPLOAD_SERVER_URL "fota.niu.com"
#define FOTA_LOG_UPLOAD_SERVER_PORT 80
#define ECU_EX_SYS_VERSION_PATH "/etc/quectel-project-version"
#define FOTA_SYS_UPDATE_FILE "/usrdata/cache/fota/ipth_package.bin"

char *g_prev_upgrade_sys[] = {
    "rm -rf /usrdata/chche",
    "mkdir -p /usrdata/cache/fota",
    "mkdir -p /usrdata/chche/recovery"
};

char *g_upgrade_sys[] = {
    "mkdir -p /tmp/mount_recovery",
    "mount -t ubifs /dev/ubi3_0 /tmp/mount_recovery -o bulk_read",
    "rm /tmp/mount_recovery/sbin/usb/boot_hsusb_composition",
    "ln -s /sbin/usb/compositions/recovery_9607 /tmp/mount_recovery/sbin/usb/boot_hsusb_composition",
    "sync",
    "sys_reboot recovery"
};

int g_fota_upload_log_count = 0;
int g_fota_upload_log_result = 0;
unsigned char g_is_reboot = 0;
unsigned char g_fota_download_percent = 0;

FOTA_OPERATION_TYPE g_fota_flag = FOTA_NULL;
pthread_mutex_t g_fota_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_fota_cond = PTHREAD_COND_INITIALIZER;
char g_fota_data[HTTP_BUF_LEN] = {0};
NUINT32 g_fota_method = FOTA_CHECK_METHOD_NONE;

all_fota_devices_info_t g_fota_devices = {0};
fota_device_upgrade_cb_t g_device_cb[FOTA_DEVICES_MAX_NUM];

char g_updateid[120] = {0};

char g_http_protocal_fota[] = "POST %s?%s HTTP/1.1\r\nContent-Length: %d\r\nContent-Type: application/json\r\nHost: %s\r\nCache-Control: no-cache\r\n\r\n%s";

char *g_fota_type[] = {
    "fota_null",
    "fota_check",
    "fota_download",
    "fota_upload_log_1",
    "fota_upgrade",
    "fota_upload_log_2"
};
void *thread_fota_proc(void *param);
extern void sys_event_handler(SYS_EVENT_E cmd, void *param, int param_len);

int fota_init()
{
    int rc = 0;
    char sys_version[64] = {0};
    pthread_t thread_fota;
    memset(g_device_cb, 0, sizeof(g_device_cb));
    memcpy(g_device_cb[0].type, "DB", strlen("DB"));
    g_device_cb[0].device_upgrade_cb = db_upgrade_cb;
    g_device_cb[0].enable_upgrade = 0;
    memcpy(g_device_cb[1].type, "FOC", strlen("FOC"));
    g_device_cb[1].device_upgrade_cb = foc_upgrade_cb;
    g_device_cb[1].enable_upgrade = 0;
    memcpy(g_device_cb[2].type, "BMS", strlen("BMS"));
    g_device_cb[2].device_upgrade_cb = bms_upgrade_cb;
    g_device_cb[2].enable_upgrade = 0;
    memcpy(g_device_cb[3].type, "BMS2", strlen("BMS2"));
    g_device_cb[3].device_upgrade_cb = bms2_upgrade_cb;
    g_device_cb[3].enable_upgrade = 0;
    memcpy(g_device_cb[4].type, "ALARM", strlen("ALARM"));
    g_device_cb[4].device_upgrade_cb = alarm_upgrade_cb;
    g_device_cb[4].enable_upgrade = 0;
    memcpy(g_device_cb[5].type, "LCU", strlen("LCU"));
    g_device_cb[5].device_upgrade_cb = lcu_upgrade_cb;
    g_device_cb[5].enable_upgrade = 0;
    memcpy(g_device_cb[6].type, "QC", strlen("QC"));
    g_device_cb[6].device_upgrade_cb = qc_upgrade_cb;
    g_device_cb[6].enable_upgrade = 0;
    memcpy(g_device_cb[7].type, "LKU", strlen("LKU"));
    g_device_cb[7].device_upgrade_cb = lku_upgrade_cb;
    g_device_cb[7].enable_upgrade = 0;
    memcpy(g_device_cb[8].type, "BCS", strlen("BCS"));
    g_device_cb[8].device_upgrade_cb = bcs_upgrade_cb;
    g_device_cb[8].enable_upgrade = 0;
    memcpy(g_device_cb[9].type, "TBS", strlen("TBS"));
    g_device_cb[9].device_upgrade_cb = tbs_upgrade_cb;
    g_device_cb[9].enable_upgrade = 0;
    memcpy(g_device_cb[10].type, "CPM", strlen("CPM"));
    g_device_cb[10].device_upgrade_cb = cpm_upgrade_cb;
    g_device_cb[10].enable_upgrade = 0;
    memcpy(g_device_cb[11].type, "EMCU", strlen("EMCU"));
    g_device_cb[11].device_upgrade_cb = emcu_upgrade_cb;
    g_device_cb[11].enable_upgrade = 1;
    memcpy(g_device_cb[12].type, "ECU_EX_APP", strlen("ECU_EX_APP"));
    g_device_cb[12].device_upgrade_cb = ecu_ex_app_upgrade_cb;
    g_device_cb[12].enable_upgrade = 1;
    memcpy(g_device_cb[13].type, "ECU_EX_SYS", strlen("ECU_EX_SYS"));
    g_device_cb[13].device_upgrade_cb = ecu_ex_sys_upgrade_cb;
    g_device_cb[13].enable_upgrade = 1;

    g_fota_method = FOTA_CHECK_METHOD_NONE;
    get_sys_version(sys_version, 64);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_EX_MODEL_VERSION, (NUINT8*)sys_version, 32);

    rc = pthread_create(&thread_fota, NULL, (void *)thread_fota_proc, NULL);
    if(0 !=  rc) {
        LOG_F("fota check thread create fails");
        return rc;
    }
    return 0;
}

void *thread_fota_proc(void *param)
{
    pthread_detach(pthread_self());
    while(1) {
        pthread_mutex_lock(&g_fota_mut);
        while(FOTA_NULL == g_fota_flag) {
            LOG_D("fota thread suspend");
            pthread_cond_wait(&g_fota_cond, &g_fota_mut);
        }
        pthread_mutex_unlock(&g_fota_mut);
        LOG_D("fota thread run");

        if((FOTA_CHECK == g_fota_flag) || (FOTA_UPLOAD_LOG_1 == g_fota_flag) || (FOTA_UPLOAD_LOG_2 == g_fota_flag)) {
            send_req_to_fota_server();
        }
        else if(FOTA_DOWNLOAD == g_fota_flag) {
            fota_download_file();
        }
        sleepdown_fota_thread();
    }
    return NULL;
}

void wakeup_fota_thread(FOTA_OPERATION_TYPE flag)
{
    if(FOTA_NULL != g_fota_flag) {
        LOG_D("fota thread is alread running");
        return;
    }

    if((FOTA_CHECK != flag) && (FOTA_DOWNLOAD != flag) && (FOTA_UPLOAD_LOG_1 != flag) && (FOTA_UPLOAD_LOG_2 != flag)) {
        LOG_D("wakeup thread while operation is not recognizated");
        return;
    }

    pthread_mutex_lock(&g_fota_mut);
    LOG_D("wake up fota thread");

    if(FOTA_CHECK == flag) {
        //TODO lock 
        gsensor_wakeup_handler(3*60);
        clear_fota_devices();
        memset(&g_updateid, 0, 120);
        create_fota_check_req_data(g_fota_method);
    }
    else if((FOTA_UPLOAD_LOG_1 == flag) || (FOTA_UPLOAD_LOG_2 == flag)) {
    LOG_D("=======start UPLOAD LOG======\n");
        create_fota_log_req_data(g_fota_method);
    LOG_D("=======end UPLOAD LOG======\n");
    }
    g_fota_flag = flag;
    pthread_cond_signal(&g_fota_cond);
    pthread_mutex_unlock(&g_fota_mut);
}

void sleepdown_fota_thread()
{
    if(FOTA_NULL == g_fota_flag) {
        LOG_D("fota thread is alread sleep");
        return;
    }
    pthread_mutex_lock(&g_fota_mut);
    LOG_D("sleepdown fota thread");
    g_fota_flag = FOTA_NULL;
    pthread_mutex_unlock(&g_fota_mut);
}

void create_fota_check_req_data(NUINT32 method)
{
    int en_cbc_data_len = 0;
    int ret_len = 0;
    char *temp_data = NULL;
    char *temp_64_data = NULL;
    unsigned char key[17] = {0};
    unsigned char iv[16] = {0};
    char url_param[256] = {0};
    unsigned char en_cbc_data[HTTP_BUF_LEN] = {0};

    cJSON *send_root = cJSON_CreateObject();
    cJSON *root = cJSON_CreateObject();
    cJSON *devices = cJSON_CreateObject();
    cJSON *data_array = cJSON_CreateArray();

    cJSON *db = cJSON_CreateObject();
    cJSON *foc = cJSON_CreateObject();
    cJSON *bms = cJSON_CreateObject();
    cJSON *bms2 = cJSON_CreateObject();
    cJSON *emcu = cJSON_CreateObject();
    cJSON *alarm = cJSON_CreateObject();
    cJSON *lcu = cJSON_CreateObject();
    cJSON *qc = cJSON_CreateObject();
    cJSON *lku = cJSON_CreateObject();
    cJSON *bcs = cJSON_CreateObject();
    cJSON *cpm = cJSON_CreateObject();
    cJSON *ecu_ex_app = cJSON_CreateObject();
    cJSON *ecu_ex_sys = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "devices", devices);
    add_niu_data_to_json(devices, "EID", NIU_ECU, NIU_ID_ECU_EID);
    add_niu_data_to_json(devices, "iccid", NIU_ECU, NIU_ID_ECU_ICCID);
    add_niu_data_to_json(devices, "UTC", NIU_ECU, NIU_ID_ECU_UTC);     
    add_niu_data_to_json(devices, "latitude", NIU_ECU, NIU_ID_ECU_LATITUDE);
    add_niu_data_to_json(devices, "longtitude", NIU_ECU, NIU_ID_ECU_LONGITUDE);
    cJSON_AddItemToObject(devices, "fotaVersion", cJSON_CreateString("2.3.4"));
    cJSON_AddItemToObject(devices, "method", cJSON_CreateNumber(method));

    cJSON_AddItemToObject(devices, "data", data_array);
    LOG_D("add common node to fota check");
    cJSON_AddItemToArray(data_array, db);
    cJSON_AddItemToArray(data_array, foc);
    cJSON_AddItemToArray(data_array, bms);
    cJSON_AddItemToArray(data_array, bms2);
    cJSON_AddItemToArray(data_array, emcu);
    cJSON_AddItemToArray(data_array, alarm);
    cJSON_AddItemToArray(data_array, lcu);
    cJSON_AddItemToArray(data_array, qc);
    cJSON_AddItemToArray(data_array, lku);
    cJSON_AddItemToArray(data_array, bcs);
    cJSON_AddItemToArray(data_array, cpm);
    cJSON_AddItemToArray(data_array, ecu_ex_app);
    cJSON_AddItemToArray(data_array, ecu_ex_sys);
    LOG_D("add devices node to fota check");
    
    cJSON_AddItemToObject(db, "devicetype", cJSON_CreateString("DB"));
    add_niu_data_to_json(db, "soft_version", NIU_DB, NIU_ID_DB_DB_SOFT_VER);
    add_niu_data_to_json(db, "hard_version", NIU_DB, NIU_ID_DB_DB_HARD_VER);

    cJSON_AddItemToObject(foc, "devicetype", cJSON_CreateString("FOC"));
    add_niu_data_to_json(foc, "soft_version", NIU_FOC, NIU_ID_FOC_FOC_MODE);
    add_niu_data_to_json(foc, "hard_version", NIU_FOC, NIU_ID_FOC_FOC_MODE);

    cJSON_AddItemToObject(bms, "devicetype", cJSON_CreateString("BMS"));
    add_niu_data_to_json(bms, "soft_version", NIU_BMS, NIU_ID_BMS_BMS_S_VER);
    add_niu_data_to_json(bms, "hard_version", NIU_BMS, NIU_ID_BMS_BMS_H_VER);
    //cJSON_AddItemToObject(bms, "hard_version", cJSON_CreateString(""));
    //cJSON_AddItemToObject(bms, "hard_version", cJSON_CreateString(""));

    cJSON_AddItemToObject(bms2, "devicetype", cJSON_CreateString("BMS2"));
    add_niu_data_to_json(bms2, "soft_version", NIU_BMS2, NIU_ID_BMS2_BMS2_S_VER);
    add_niu_data_to_json(bms2, "hard_version", NIU_BMS2, NIU_ID_BMS2_BMS2_H_VER);
    //cJSON_AddItemToObject(bms2, "hard_version", cJSON_CreateString(""));
    //cJSON_AddItemToObject(bms2, "hard_version", cJSON_CreateString(""));
    
    cJSON_AddItemToObject(emcu, "devicetype", cJSON_CreateString("EMCU"));
    add_niu_data_to_json(emcu, "soft_version", NIU_EMCU, NIU_ID_EMCU_EMCU_SOFT_VER);
    add_niu_data_to_json(emcu, "hard_version", NIU_EMCU, NIU_ID_EMCU_EMCU_HARD_VER);

    cJSON_AddItemToObject(alarm, "devicetype", cJSON_CreateString("ALARM"));
    add_niu_data_to_json(alarm, "soft_version", NIU_ALARM, NIU_ID_ALARM_ALARM_SOFT_VER);
    add_niu_data_to_json(alarm, "hard_version", NIU_ALARM, NIU_ID_ALARM_ALARM_HARD_VER);

    cJSON_AddItemToObject(lcu, "devicetype", cJSON_CreateString("LCU"));
    add_niu_data_to_json(lcu, "soft_version", NIU_LCU, NIU_ID_LCU_LCU_SOFT_VER);   
    add_niu_data_to_json(lcu, "hard_version", NIU_LCU, NIU_ID_LCU_LCU_HARD_VER);   
    
    cJSON_AddItemToObject(qc, "devicetype", cJSON_CreateString("QC"));
    add_niu_data_to_json(qc, "soft_version", NIU_QC, NIU_ID_QC_QC_SOFTWARE_VERSION);
    add_niu_data_to_json(qc, "hard_version", NIU_QC, NIU_ID_QC_QC_HARDWARE_VERSION);
    
    cJSON_AddItemToObject(lku, "devicetype", cJSON_CreateString("LKU"));
    add_niu_data_to_json(lku, "soft_version", NIU_LOCKBOARD, NIU_ID_LOCKBOARD_LOCK_BOARD_SOFT_VER);
    add_niu_data_to_json(lku, "hard_version", NIU_LOCKBOARD, NIU_ID_LOCKBOARD_LOCK_BOARD_HARD_VER);
    
    cJSON_AddItemToObject(bcs, "devicetype", cJSON_CreateString("BCS"));
    add_niu_data_to_json(bcs, "soft_version", NIU_BCS, NIU_ID_BCS_BCS_S_VER);
    add_niu_data_to_json(bcs, "hard_version", NIU_BCS, NIU_ID_BCS_BCS_H_VER);
    
    cJSON_AddItemToObject(cpm, "devicetype", cJSON_CreateString("CPM"));
    add_niu_data_to_json(cpm, "soft_version", NIU_CPM, NIU_ID_CPM_DEV_SW_VERNO);
    add_niu_data_to_json(cpm, "hard_version", NIU_CPM, NIU_ID_CPM_DEV_HW_VERNO);

    cJSON_AddItemToObject(ecu_ex_app, "devicetype", cJSON_CreateString("ECU_EX_APP"));
    add_niu_data_to_json(ecu_ex_app, "soft_version", NIU_ECU, NIU_ID_ECU_ECU_SOFT_VER);
    cJSON_AddItemToObject(ecu_ex_app, "hard_version", cJSON_CreateString(""));

    cJSON_AddItemToObject(ecu_ex_sys, "devicetype", cJSON_CreateString("ECU_EX_SYS"));
    add_niu_data_to_json(ecu_ex_sys, "soft_version", NIU_ECU, NIU_ID_ECU_EX_MODEL_VERSION);
    cJSON_AddItemToObject(ecu_ex_sys, "hard_version", cJSON_CreateString(""));
    
    LOG_D("add soft and hard version to device in fota check");

    temp_data = cJSON_Print(root);

    //printf("fota check source data:\n");
    //printf("%s\n",temp_data);

    memset(iv, 48, 16);
    memset(en_cbc_data, 0, HTTP_BUF_LEN);
    //niu_data_config_read(NIU_DATA_CONFIG_AES_KEY, key, SECTION_LEN);
    NIU_DATA_VALUE *temp_data_value = NULL;
    temp_data_value = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
    if(temp_data_value)
    {
        memcpy(key, temp_data_value->addr, SECTION_LEN);
    }
    en_cbc_data_len = encrypt_data_in_cbc_mode((unsigned char *)temp_data, strlen(temp_data),iv, SECTION_LEN, key, SECTION_LEN ,en_cbc_data, HTTP_BUF_LEN);
    LOG_D("encode fota check data");
    free(temp_data);
    temp_data = NULL;
    cJSON_Delete(root);
    
    temp_64_data = (char *)base64_encode(en_cbc_data, en_cbc_data_len, &ret_len);
    cJSON_AddItemToObject(send_root, "data", cJSON_CreateString(temp_64_data));
    temp_data = cJSON_PrintUnformatted(send_root);

    LOG_D("base64 fota check data");

    //printf("fota check send data:\n");
    //printf("%s\n",temp_data);

    memset(g_fota_data, 0, HTTP_BUF_LEN);
    get_fota_url_param(url_param, 256);
    snprintf(g_fota_data, sizeof(g_fota_data), g_http_protocal_fota, "/v2/checkupdate", url_param, strlen(temp_data), FOTA_CHECK_SERVER_URL, temp_data);
    
    free(temp_data);
    temp_data = NULL;
    cJSON_Delete(send_root);
    QL_MEM_FREE(temp_64_data);
    temp_64_data = NULL;    
}

void create_fota_log_req_data(NUINT32 method)
{
    int en_cbc_data_len = 0;
    int ret_len = 0;
    char *temp_data = NULL;
    char *temp_64_data = NULL;

    unsigned char key[17] = {0};
    unsigned char iv[16] = {0};

    cJSON *send_root = cJSON_CreateObject();
    cJSON *root = cJSON_CreateObject();
    cJSON *devices =cJSON_CreateObject();
    cJSON *data_array = cJSON_CreateArray();
    char url_param[256] = {0};
    unsigned char en_cbc_data[HTTP_BUF_LEN] = {0};

    cJSON *db = cJSON_CreateObject();
    cJSON *foc = cJSON_CreateObject();
    cJSON *bms = cJSON_CreateObject();
    cJSON *bms2 = cJSON_CreateObject();
    cJSON *emcu = cJSON_CreateObject();
    cJSON *alarm = cJSON_CreateObject();
    cJSON *lcu = cJSON_CreateObject();
    cJSON *qc = cJSON_CreateObject();
    cJSON *lku = cJSON_CreateObject();
    cJSON *bcs = cJSON_CreateObject();
    cJSON *cpm = cJSON_CreateObject();
    cJSON *ecu_ex_app = cJSON_CreateObject();
    cJSON *ecu_ex_sys = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "devices", devices);
    cJSON_AddItemToObject(root, "updateid", cJSON_CreateString(g_updateid));
    add_niu_data_to_json(devices, "EID", NIU_ECU, NIU_ID_ECU_EID);
    add_niu_data_to_json(devices, "iccid", NIU_ECU, NIU_ID_ECU_ICCID);
    add_niu_data_to_json(devices, "UTC", NIU_ECU, NIU_ID_ECU_UTC);     
    add_niu_data_to_json(devices, "latitude", NIU_ECU, NIU_ID_ECU_LATITUDE);
    add_niu_data_to_json(devices, "longitude", NIU_ECU, NIU_ID_ECU_LONGITUDE);
    cJSON_AddItemToObject(devices, "fotaVersion", cJSON_CreateString("2.3.4"));
    cJSON_AddItemToObject(devices, "method", cJSON_CreateNumber(method));
    cJSON_AddItemToObject(devices, "data", data_array);
    LOG_D("add common node to fota log");

    cJSON_AddItemToArray(data_array, db);
    cJSON_AddItemToArray(data_array, foc);
    cJSON_AddItemToArray(data_array, bms);
    cJSON_AddItemToArray(data_array, bms2);
    cJSON_AddItemToArray(data_array, emcu);
    cJSON_AddItemToArray(data_array, alarm);
    cJSON_AddItemToArray(data_array, lcu);
    cJSON_AddItemToArray(data_array, qc);
    cJSON_AddItemToArray(data_array, lku);
    cJSON_AddItemToArray(data_array, bcs);
    cJSON_AddItemToArray(data_array, cpm);
    cJSON_AddItemToArray(data_array, ecu_ex_app);
    cJSON_AddItemToArray(data_array, ecu_ex_sys);
    LOG_D("add devices node to fota log");

    add_device_to_log_json(db, "DB");
    add_device_to_log_json(foc, "FOC");
    add_device_to_log_json(bms, "BMS");
    add_device_to_log_json(bms2, "BMS2");
    add_device_to_log_json(emcu, "EMCU");
    add_device_to_log_json(alarm, "ALARM");
    add_device_to_log_json(lcu, "LCU");
    add_device_to_log_json(qc, "QC");
    add_device_to_log_json(lku, "LKU");
    add_device_to_log_json(bcs, "BCS");
    add_device_to_log_json(cpm, "CPM");
    add_device_to_log_json(ecu_ex_app, "ECU_EX_APP");
    add_device_to_log_json(ecu_ex_sys, "ECU_EX_SYS");

    LOG_D("add soft and hard version to device in fota log");

    temp_data = cJSON_Print(root);

    LOG_D("temp_data: %s", temp_data);
    memset(iv, 48, 16);
    memset(en_cbc_data, 0, HTTP_BUF_LEN);
    //niu_data_config_read(NIU_DATA_CONFIG_AES_KEY, key, SECTION_LEN);
    NIU_DATA_VALUE *temp_data_value = NULL;
    temp_data_value = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
    if(temp_data_value)
    {
        memcpy(key, temp_data_value->addr, SECTION_LEN);
    }
    en_cbc_data_len = encrypt_data_in_cbc_mode((unsigned char *)temp_data, strlen(temp_data),iv, SECTION_LEN, key, SECTION_LEN ,en_cbc_data, HTTP_BUF_LEN);
    LOG_D("encode fota check data");

    free(temp_data);
    temp_data = NULL;
    cJSON_Delete(root);
    LOG_D("en_cbc_data_len: %d", en_cbc_data_len);
    temp_64_data = (char *)base64_encode(en_cbc_data, en_cbc_data_len, &ret_len);
    cJSON_AddItemToObject(send_root, "data", cJSON_CreateString(temp_64_data));
    temp_data = cJSON_PrintUnformatted(send_root);
    LOG_D("base64 fota check data");

    memset(g_fota_data, 0, HTTP_BUF_LEN);
    get_fota_url_param(url_param, 256);
    snprintf(g_fota_data, sizeof(g_fota_data), g_http_protocal_fota, "/v2/updatelog", url_param, strlen(temp_data), FOTA_LOG_UPLOAD_SERVER_URL, temp_data);
    
    free(temp_data);
    temp_data = NULL;
    cJSON_Delete(send_root);
    QL_MEM_FREE(temp_64_data);
    temp_64_data = NULL;
}

void send_req_to_fota_server()
{
    int i = 0, j = 0;
    int write_length = 0, content_length = 0;
    int socket_handler = 0;
    char url[32] = {0};
    int port = 0;

    struct timeval tm = {0};
    tm.tv_sec = 3;
    tm.tv_usec = 0;

    memset(url, 0, 32);
    if(FOTA_CHECK == g_fota_flag) {
        clear_fota_devices(); 
        memcpy(url, FOTA_CHECK_SERVER_URL, strlen(FOTA_CHECK_SERVER_URL));
        port = FOTA_CHECK_SERVER_PORT;
    }
    else if((FOTA_UPLOAD_LOG_1 == g_fota_flag) || (FOTA_UPLOAD_LOG_2 == g_fota_flag)) {
        memcpy(url, FOTA_LOG_UPLOAD_SERVER_URL, strlen(FOTA_LOG_UPLOAD_SERVER_URL));
        port = FOTA_LOG_UPLOAD_SERVER_PORT;
        g_fota_upload_log_count++;
    }
    else {
        LOG_D("can not recognizated fota operation");
        return;
    }

    for(i = 0; i < RECONNECT_NUM; i++) {
        LOG_D("[%s]:fota connect number is %d, url is %s, port is %d", g_fota_type[g_fota_flag], i + 1, url, port);
        if(0 != network_connect(&socket_handler, url, port)) {
            LOG_D("[%s]:fota connect failure, number is %d", g_fota_type[g_fota_flag], i + 1);
            usleep(FOTA_INTERVAL_TIME);
            continue;
        }
        else {
            LOG_D("[%s]:fota connect succeed, number is %d", g_fota_type[g_fota_flag], i + 1);
            content_length = strlen((char *)g_fota_data);
            for(j = 0; j < RESEND_NUM; j++) {
                setsockopt(socket_handler, SOL_SOCKET, SO_SNDTIMEO, (char *)&tm, sizeof(struct timeval));
                write_length = write(socket_handler, g_fota_data, content_length);
                if(content_length == write_length) {
                    LOG_D("[%s]:fota thread send data successful at %d number", g_fota_type[g_fota_flag], j + 1);
                    if(0 == read_resq_from_fota_server(socket_handler)) {
                        break;  
                    }
                    else {
                        if(FOTA_CHECK == g_fota_flag) {
                            close(socket_handler);
                            return;
                        }
                        else {
                            break;
                        }
                    }
                }
                else {
                    LOG_D("[%s]:fota thread send data failure at %d number", g_fota_type[g_fota_flag], j + 1);
                    continue;
                }
            }
            if(RESEND_NUM == j) {
                LOG_D("[%s]:fota send data failure in %d times retry", RESEND_NUM);
                close(socket_handler);
                return;
            }
            close(socket_handler);
            break;
        }
    }

    if(RECONNECT_NUM == i) {
        LOG_D("fota connect failure in %d times retry", RECONNECT_NUM);
        return;
    }

    if(FOTA_CHECK == g_fota_flag) {
        sys_event_push(FOTA_EVENT_CHECK_COMPLETED, NULL, 0);
    }
    else if(FOTA_UPLOAD_LOG_1 == g_fota_flag) {
        if((0 == g_fota_upload_log_result) && (g_fota_upload_log_count < 3)){
            sys_event_push(FOTA_EVENT_UPLOAD_LOG_1_AGAIN, NULL, 0);
        }
        else {
            g_fota_upload_log_result = 0;
            g_fota_upload_log_count = 0;
            sys_event_push(FOTA_EVENT_UPLOAD_LOG_1_COMPLETED, NULL, 0);
        }
    }
    else if(FOTA_UPLOAD_LOG_2 == g_fota_flag) {
        if((0 == g_fota_upload_log_result) && (g_fota_upload_log_count < 3)){
            sys_event_push(FOTA_EVENT_UPLOAD_LOG_2_AGAIN, NULL, 0);
        }
        else {
            g_fota_upload_log_result = 0;
            g_fota_upload_log_count = 0;
            sys_event_push(FOTA_EVENT_UPLOAD_LOG_2_COMPLETED, &g_is_reboot, sizeof(unsigned char));
        }
    }
}

int read_resq_from_fota_server(int socket_fd)
{
    int i = 0;
    int readlength = 0, datalength = 0, fota_data_length = 0;
    char read_buf[HTTP_BUF_LEN] = {0};
    char json_buf[HTTP_BUF_LEN] = {0};
    
    struct timeval tm = {0};
    tm.tv_sec = 3;
    tm.tv_usec = 0;
    memset(read_buf, 0, HTTP_BUF_LEN);

    for(i = 0; i < REREAD_NUM; i++) {
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tm, sizeof(struct timeval));
        readlength = read(socket_fd, read_buf + datalength, HTTP_BUF_LEN);
        datalength = datalength + readlength;
        if(0 < readlength) {
            fota_data_length = fota_response_data_parse(read_buf, readlength, json_buf, HTTP_BUF_LEN);
            if(0 != fota_data_length) {
                LOG_D("[%s]:fota response data read completed in %d times read", g_fota_type[g_fota_flag], i + 1);
                if(FOTA_CHECK == g_fota_flag) {
                    get_fota_response_result(json_buf, fota_data_length);
                }
                return 0;
            }
            else {
                LOG_D("[%s]:fota response data read not completed in %d times read", g_fota_type[g_fota_flag], i + 1);
                continue;
            }
        }
        else if(0 == readlength) {
            LOG_D("[%s]:fota thread while read response data, the server close the socket at %d times", g_fota_type[g_fota_flag], i + 1);
            return 1;
        }
        else {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) {
                LOG_D("[%s]:fota thread while read response data, the read length is smaller than 0 at %d times, but can read again", g_fota_type[g_fota_flag], i + 1);
                continue;
            }
            else {
                LOG_D("[%s]:fota thread while read response data, the read length is smaller than 0 at %d times, but can not read again, errno is %d, errinfo is %s", g_fota_type[g_fota_flag], i + 1, errno, strerror(errno));
                return 1;
            }
        }
    }
    if(REREAD_NUM ==i) {
        LOG_D("[%s]:fota thread read response data failure in %d times retry", g_fota_type[g_fota_flag], REREAD_NUM);
        return 1;
    }
    return 1;
}

int fota_response_data_parse(char *data, int data_len, char *json_data, int json_data_len)
{
    int d_len = 0, de_cbc_data_len = 0;
    unsigned char *d_data = NULL;
    unsigned char iv[SECTION_LEN] = {0};
    unsigned char key[17] = {0};
    unsigned char de_cbc_data[HTTP_BUF_LEN] = {0};
    char *p = NULL, *p1 = NULL;
    cJSON *root = NULL;
    cJSON *code = NULL;
    cJSON *content = NULL;
    cJSON *msg = NULL;
    cJSON *err_msg = NULL;
    NIU_DATA_VALUE *temp_data_value = NULL;

    if((NULL == data) || (NULL == json_data)) {
        LOG_D("parse [%s] response data while param is NULL", g_fota_type[g_fota_flag]);
        return 0;
    }
    
    p = strstr(data, "{");
    p1 = strstr(data, "}");
    if((NULL == p) || (NULL == p1)) {
        return 0;
    }

    root = cJSON_Parse(data + (p - data));
    if(NULL == root) {
        LOG_D("the result of parse [%s] response data is NULL", g_fota_type[g_fota_flag]);
        return 0;
    }

    code = cJSON_GetObjectItem(root, "code");
    if((0 == code->valueint)) {
        msg = cJSON_GetObjectItem(root, "msg");
        if(0 == strcmp("Success", msg->valuestring)) {
            if(FOTA_CHECK != g_fota_flag) {
                g_fota_upload_log_result = 1;
                cJSON_Delete(root);
                return 1;
            }
            content = cJSON_GetObjectItem(root, "data");
            d_data = base64_decode((unsigned char *)(content->valuestring), strlen(content->valuestring), &d_len);
            if(NULL == d_data) {
                LOG_D("decode in base64 error, return value is NULL");
                cJSON_Delete(root);
                return 1;
            }
            memset(iv, 48, SECTION_LEN);
            memset(de_cbc_data, 0, HTTP_BUF_LEN);
            //niu_data_config_read(NIU_DATA_CONFIG_AES_KEY, key, SECTION_LEN);
            temp_data_value = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
            if(temp_data_value)
            {
                memcpy(key, temp_data_value->addr, SECTION_LEN);
            }
            de_cbc_data_len = decrypt_data_in_cbc_mode(d_data, d_len, iv, SECTION_LEN, key, SECTION_LEN ,de_cbc_data, HTTP_BUF_LEN);
            //printf("fota check result data:\n");
            //printf("%s\n", de_cbc_data);
            memcpy(json_data, de_cbc_data, de_cbc_data_len);
            cJSON_Delete(root);
            QL_MEM_FREE(d_data);
            return 1;
        }
        else {
            LOG_D("[%s] response data is valid, code is %d, msg is %s", g_fota_type[g_fota_flag], code->valueint, msg->valuestring); 
            cJSON_Delete(root);
            return 0;
        }
    }
    else {
         err_msg = cJSON_GetObjectItem(root, "error");

        LOG_D("[%s] response data is valid, code is %d, msg is %s", g_fota_type[g_fota_flag], code->valueint, err_msg->valuestring); 
        cJSON_Delete(root);
        return 0;
    }
}

int get_fota_response_result(char *data, int data_len)
{
    int i = 0, j = 0, itemcount = 0;
    cJSON *device_item = NULL;
    cJSON *param_item = NULL;
    cJSON *devices = NULL;
    cJSON *root = cJSON_Parse(data);
    if(NULL == root) {
        LOG_D("parse json data failed");
        return 1;
    }
    if(FOTA_CHECK == g_fota_flag) {
        param_item = cJSON_GetObjectItem(root, "updateid");
        if(param_item == NULL)
        {
            LOG_E("fota response updateid is null.");
            return 1;
        }
        memset(g_updateid, 0, 120);
        memcpy(g_updateid, param_item->valuestring, strlen(param_item->valuestring));
        devices = cJSON_GetObjectItem(root, "devices");
        if(devices == NULL)
        {
            LOG_E("fota response devices is null.");
            return 1;
        }
        itemcount = cJSON_GetArraySize(devices);
        g_fota_devices.devices_num = itemcount;

        for(i = 0; i < itemcount; i++) {
            device_item = cJSON_GetArrayItem(devices, i);
            if(device_item == NULL)
            {
                LOG_E("fota response device_item is null.");
                continue;
            }
            param_item = cJSON_GetObjectItem(device_item, "devicetype");
            if(param_item == NULL)
            {
                LOG_E("fota response devicetype is null.");
                continue;
            }
            memcpy(g_fota_devices.devices[i].type, param_item->valuestring, strlen(param_item->valuestring));
            for(j = 0; j < FOTA_DEVICES_MAX_NUM; j++) {
                if(0 == strcmp(g_fota_devices.devices[i].type, g_device_cb[j].type)) {
                    g_fota_devices.devices[i].fota_upgrade_cb = g_device_cb[j].device_upgrade_cb;
                    g_fota_devices.devices[i].enable_upgrade = g_device_cb[j].enable_upgrade;
                    break;
                }
            }

            get_soft_hard_version(param_item->valuestring, i);
            param_item = cJSON_GetObjectItem(device_item, "hasnew");
            if(param_item == NULL)
            {
                LOG_E("fota response hasnew is null.");
                continue;
            }
            if(1 == param_item->valueint) {
                g_fota_devices.devices[i].hasnew = 1;
                param_item = cJSON_GetObjectItem(device_item, "version");
                if(param_item == NULL)
                {
                    LOG_E("fota response version is null.");
                    continue;
                }
                memcpy(g_fota_devices.devices[i].version, param_item->valuestring, strlen(param_item->valuestring));

                param_item = cJSON_GetObjectItem(device_item, "size");
                if(param_item == NULL)
                {
                    LOG_E("fota response size is null.");
                    continue;
                }
                g_fota_devices.devices[i].package_size = param_item->valueint;

                param_item = cJSON_GetObjectItem(device_item, "packagetype");
                if(param_item == NULL)
                {
                    LOG_E("fota response packagetype is null.");
                    continue;
                }
                memcpy(g_fota_devices.devices[i].package_type, param_item->valuestring, strlen(param_item->valuestring));

                param_item = cJSON_GetObjectItem(device_item, "url");
                if(param_item == NULL)
                {
                    LOG_E("fota response url is null.");
                    continue;
                }
                memcpy(g_fota_devices.devices[i].url, param_item->valuestring, strlen(param_item->valuestring));

                param_item = cJSON_GetObjectItem(device_item, "md5");
                if(param_item == NULL)
                {
                    LOG_E("fota response md5 is null.");
                    continue;
                }
                memcpy(g_fota_devices.devices[i].md5, param_item->valuestring, strlen(param_item->valuestring));
            }
            else {
                g_fota_devices.devices[i].hasnew = 0;
            }
        }
        cJSON_Delete(root);
    }
    return 0;
}

void clear_fota_devices()
{
    int i = 0;
    memset(&g_fota_devices, 0, sizeof(g_fota_devices));
    for(i = 0; i < FOTA_DEVICES_MAX_NUM; i++) {
        g_fota_devices.devices[i].result =  UPGRADE_CODE_INIT;
    }
}

void delete_file(char *path)
{
    char cmd_str[256] = {0};
    if(NULL == path) {
        LOG_D("delete file while path is NULL");
        return;
    }
    if(0 == ql_com_file_exist(path)) {
        snprintf(cmd_str, sizeof(cmd_str), "rm -f %s", path);
        system(cmd_str);
    }
}

void get_last_item(char *path, int path_len, char splitter, char *get_str, int get_str_len)
{
    char *p = NULL, *p1 = NULL;
    if((NULL == path) && (NULL != get_str)) {
        LOG_D("get last item failure while parma is NULL ");
        return;
    }
    memset(get_str, 0, get_str_len);
    p = strchr(path, splitter);

    if(NULL != p) {
        p1 = p + 1;
        while(1) {
            p = strchr(p1, splitter);
            if(NULL != p) {
                p1 = p + 1;
                continue;
            }
            else {
                if(strlen(p1) > get_str_len) {
                    memcpy(get_str, p1, get_str_len);
                }
                else {
                    memcpy(get_str, p1, strlen(p1));
                }
                break;
            }
        }
    }
    else {
        if(path_len > get_str_len) {
            memcpy(get_str, path, get_str_len);
        }
        else {
            memcpy(get_str, path, path_len);
        }
    }
}

void fota_download_file()
{
    int i = 0, ret = 0;
    char cmd_str[256] = {0};
    char file_name[33] = {0};
    char md5_value[33] = {0};
    char file_path[64] = {0};
    if(1 == ql_com_file_exist(FOTA_UPGRADE_PATH)) {
        mkdir(FOTA_UPGRADE_PATH, 0755);
    }

    for(i = 0; i < g_fota_devices.devices_num; i++) {
        if(0 == g_fota_devices.devices[i].hasnew) {
            g_fota_devices.devices[i].result = UPGRADE_CODE_NO_UPGRADE;
            continue;
        }
        
        get_last_item(g_fota_devices.devices[i].url, strlen(g_fota_devices.devices[i].url), '/', file_name, 33);
        snprintf(file_path, sizeof(file_path), "%s/%s", FOTA_UPGRADE_PATH, file_name);
        delete_file(file_path);
        memset(cmd_str, 0, sizeof(cmd_str));
        snprintf(cmd_str, sizeof(cmd_str), "wget -T 3 -t 10 -P %s/ 2>&1 %s", FOTA_UPGRADE_PATH, g_fota_devices.devices[i].url);
        ret = execute_wget(cmd_str, g_fota_devices.devices[i].package_size);
        if(0 != ret) {
            g_fota_devices.devices[i].result = UPGRADE_CODE_DOWNLOAD_FAIL;
            continue;
        }
        get_md5_value(file_path, md5_value, 33, 0);
        if(0 != strcmp(g_fota_devices.devices[i].md5, md5_value)) {
            g_fota_devices.devices[i].result = UPGRADE_CODE_AUTH_FAIL;
            continue;
        }
        else {
            g_fota_devices.devices[i].result = UPGRADE_CODE_DOWNLOAD_SUCCESS;
        }
    }
    sys_event_push(FOTA_EVENT_DOWNLOAD_COMPLETED, NULL, 0);
}

void fota_upgrade_devices()
{
    int i = 0;
    int upgrade_flag = 0;
    if(1 == ql_com_file_exist(FOTA_BACKUP_PATH)) {
        mkdir(FOTA_BACKUP_PATH, 0755);
    }
    LOG_D("start upgrade all devices which need to upgrade");
    for(i = 0; i < g_fota_devices.devices_num; i++) {
        
        LOG_D("devices_type:%s, hasnew:%d, enable_upgrade:%d, status:%d\n", 
                g_fota_devices.devices[i].type, 
                g_fota_devices.devices[i].hasnew, 
                g_fota_devices.devices[i].enable_upgrade, 
                g_fota_devices.devices[i].result);
        if((1 == g_fota_devices.devices[i].hasnew) && (1 == g_fota_devices.devices[i].enable_upgrade) && 
                (UPGRADE_CODE_DOWNLOAD_SUCCESS == g_fota_devices.devices[i].result)) {
            upgrade_flag++;
            LOG_D("upgrade %s device", g_fota_devices.devices[i].type);
            g_fota_devices.devices[i].fota_upgrade_cb(i);
        }
    }
    if(0 == upgrade_flag) {
        sys_event_push(FOTA_EVENT_UPGRADE_COMPLETED, NULL, 0);
    }
}

int execute_wget(char *cmd, int length)
{
    int ret = 0, percent = 0, count = 0;
    char last_item[128] = {0};
    char url[256] = {0};
    char cmdinfo[16] = {0};
    char fileinfo[256] = {0};
    char get_item[32] = {0};
    FILE *cmdp = NULL;
    char *p = NULL;
    struct stat file_state;
    if(NULL == cmd) {
        ret = 1;
        LOG_D("execute cmd while cmd is null");
        return ret;
    }
    p = strstr(cmd, "http");
    if(NULL != p) {
        memset(url, 0, 256);
        memcpy(url, p, strlen(p));
    }
    else {
        ret = 2;
        LOG_D("download path is not http, cmd is %s", cmd);
        return ret;
    }
    cmdp = popen(cmd, "r");
    if(NULL == cmdp) {
        ret = 3;
        LOG_D("get %s file error while call popen return NULL", url);
        return ret;
    }
    while(NULL != fgets(cmdinfo, sizeof(cmdinfo), cmdp)) {
        p = strstr(cmdinfo, "%");
        if(NULL == p) {
            count = strlen(cmdinfo);
            memcpy(get_item, cmdinfo, count);
        }
        else {
            memcpy(get_item + count, cmdinfo, strlen(cmdinfo)); 
            percent = 0;
            if(NULL != (p = strstr(get_item, "%"))) {
                if((p - get_item) > 3) {
                    p = p - 3;
                    if((' ' != *p) && (*p <= '9') && (*p >= '0')) {
                        percent += (*p - 0x30) * 100;
                    }
                    if((' ' != *(p + 1)) && (*(p + 1) <= '9') && (*(p + 1) >= '0')) {
                        percent += (*(p + 1) - 0x30) * 10;
                    }
                    if((' ' != *(p + 2)) && (*(p + 2) <= '9') && (*(p + 2) >= '0')) {
                        percent += (*(p + 2) - 0x30) * 1;
                    }
                    g_fota_download_percent = percent;
                    LOG_D("download %s, percent is %d", url, g_fota_download_percent);
                }
            }
            memset(get_item, 0, 32);
        }
    }

    get_last_item(cmd, strlen(cmd), ' ', last_item, 128);
    memset(cmd, 0, strlen(cmd));
    memcpy(cmd, last_item, strlen(last_item));
    get_last_item(cmd, strlen(cmd), '/', last_item, 128);
    memset(fileinfo, 0, sizeof(fileinfo));
    snprintf(fileinfo, sizeof(fileinfo), "%s/%s", FOTA_UPGRADE_PATH, last_item);
    stat(fileinfo, &file_state);
    if(length == file_state.st_size) {
        g_fota_download_percent = 100;
    }

    if(100 != g_fota_download_percent) {
        ret = 4;
        pclose(cmdp);
        LOG_D("download file %s error, percent is %d", url, g_fota_download_percent);
        return ret;
    }
    pclose(cmdp);
    return ret;
}

void get_execute_file_name(char *process_name, int length)
{
    char *p = NULL, *p1 = NULL;
    char path[1024] = {0};
    if(NULL == process_name) {
        LOG_D("get process name fialed while process_name is NULL");
        return;
    }
    memset(process_name, 0, length);
    if(readlink("/proc/self/exe", path, 1024) <= 0) {
        memcpy(process_name, "niu_mgmtd", strlen("niu_mgmtd"));
        return;
    }
    else {
        p = strchr(path, '/');
        if(NULL != p) {
            p1 = p + 1;
            while(1) {
                p = strchr(p1, '/');
                if(NULL != p) {
                    p1 = p + 1;
                    continue;
                }
                else {
                    memcpy(process_name, p1, strlen(p1));
                    break;
                }
            }
        }
        else {
            memcpy(process_name, path, strlen(path));
        }
    }
}

/*
 * flag is 0, param string is file path
 * flag is 1, param string is character sequence
*/
void get_md5_value(char *string, char *md5value, int md5value_len, int flag)
{
    FILE *fp = NULL;
    char command[1024] = {0};
    char result_buf[1024] = {0};
    
    if((NULL == string) || (NULL == md5value) || (md5value_len < 33)) {
        LOG_D("get file md5 value while param is invalid");
        return;
    }
    memset(command, 0, 1024);
    if(0 == flag) {
        snprintf(command, sizeof(command), "md5sum %s | cut -d ' ' -f1", string);
    }
    else {
        snprintf(command, sizeof(command), "echo -n \"%s\" | md5sum | cut -d ' ' -f1", string);
    }
    fp = popen(command, "r");
    if(NULL == fp) {
        LOG_D("get the md5 value of %s while popen failed", string);
        return;
    }
    else {
        memset(md5value, 0, md5value_len);
        while(NULL != fgets(result_buf, sizeof(result_buf), fp)) {
            if('\n' == result_buf[strlen(result_buf) - 1]) {
                result_buf[strlen(result_buf) - 1] = '\0';
            }
            memcpy(md5value, result_buf, 32);
        }
        pclose(fp);
    }
    return;
}

void get_sys_version(char *sys_version, int sys_version_length)
{
    int length = 0;
    char *p = NULL, *p1 = NULL;
    char buf[256] = {0};
    FILE *fp = NULL;
    if(NULL == sys_version) {
        LOG_D("get sys version while parma is NULL");
        return;
    }
    if(NULL == (fp = fopen(ECU_EX_SYS_VERSION_PATH, "r"))) {
        LOG_D("get sys version while %s open failed", ECU_EX_SYS_VERSION_PATH);
    }
    fread(buf, 256, 1, fp);

    p = strstr(buf, "Project Rev : ");
    if(NULL != p) {
        p = p + strlen("Project REV : ");
        p1 = strstr(p, "\n");
        if(NULL == p1) {
            p1 = p + strlen(p);
        }
        else {
            p1 = p1 - 1;
        }
        length = p1 - p + 1;
        
        if(sys_version_length < (length + 1)) {
            LOG_D("get sys version while param is %d smaller than the length of version %d", sys_version_length, length);
        }
        else {
            memset(sys_version, 0, sys_version_length);
            memcpy(sys_version, p, length);
        }
    }
    else {
        LOG_D("get sys version failed while not found inforamtion in file %s", ECU_EX_SYS_VERSION_PATH);
        fclose(fp);
        return;
    }
    fclose(fp);
}

void get_soft_hard_version(char *device_type, int index)
{
    int base_id = 0, soft_id = 0, hard_id = 0;
    NIU_DATA_VALUE *temp_niu_data = NULL;
    unsigned char temp_data[32] = {0};

    if(NULL == device_type) {
        LOG_D("get device soft and hard version while device type is NULL");
        return;
    }

    if(0 == strcmp(device_type, "FOC")) {
        temp_niu_data = NIU_DATA_FOC(NIU_ID_FOC_FOC_MODE);
        memset(temp_data, 0, 32);
        memcpy(temp_data, temp_niu_data->addr, temp_niu_data->len);
        memcpy(g_fota_devices.devices[index].before_soft_version, temp_data, 8);
        memcpy(g_fota_devices.devices[index].hard_version, temp_data + 8, 8);
    }
    else {
        if(0 == strcmp(device_type, "DB")) { 
            base_id = NIU_DB;
            soft_id = NIU_ID_DB_DB_SOFT_VER;
            hard_id = NIU_ID_DB_DB_HARD_VER;
        }
        else if(0 == strcmp(device_type, "BMS")) { 
            base_id = NIU_BMS;
            soft_id = NIU_ID_BMS_BMS_S_VER;
            hard_id = NIU_ID_BMS_BMS_H_VER;
        }
        else if(0 == strcmp(device_type, "BMS2")) { 
            base_id = NIU_BMS2;
            soft_id = NIU_ID_BMS2_BMS2_S_VER;
            hard_id = NIU_ID_BMS2_BMS2_H_VER;
        }
        else if(0 == strcmp(device_type, "EMCU")) {
            base_id = NIU_EMCU;
            soft_id = NIU_ID_EMCU_EMCU_SOFT_VER;
            hard_id = NIU_ID_EMCU_EMCU_HARD_VER;
        }
        else if(0 == strcmp(device_type, "ALARM")) {
            base_id = NIU_ALARM;
            soft_id = NIU_ID_ALARM_ALARM_SOFT_VER;
            hard_id = NIU_ID_ALARM_ALARM_HARD_VER;
        }
        else if(0 == strcmp(device_type, "LCU")) {
            base_id = NIU_LCU;
            soft_id = NIU_ID_LCU_LCU_SOFT_VER;
            hard_id = NIU_ID_LCU_LCU_HARD_VER;
        }
        else if(0 == strcmp(device_type, "QC")) {
            base_id = NIU_QC;
            soft_id = NIU_ID_QC_QC_SOFTWARE_VERSION;
            hard_id = NIU_ID_QC_QC_HARDWARE_VERSION;
        }
        else if(0 == strcmp(device_type, "LKU")) {
            base_id = NIU_LOCKBOARD;
            soft_id = NIU_ID_LOCKBOARD_LOCK_BOARD_SOFT_VER;
            hard_id = NIU_ID_LOCKBOARD_LOCK_BOARD_HARD_VER;
        }
        else if(0 == strcmp(device_type, "BCS")) {
            base_id = NIU_BCS;
            soft_id = NIU_ID_BCS_BCS_S_VER;
            hard_id = NIU_ID_BCS_BCS_H_VER;
        }
        else if(0 == strcmp(device_type, "CPM")) {
            base_id = NIU_CPM;
            soft_id = NIU_ID_CPM_DEV_SW_VERNO;
            hard_id = NIU_ID_CPM_DEV_HW_VERNO;
        }
        else if(0 == strcmp(device_type, "ECU_EX_APP")) {
            base_id = NIU_ECU;
            soft_id = NIU_ID_ECU_ECU_SOFT_VER;
            hard_id = NIU_ID_ECU_ECU_HARD_VER;
        }
        else if(0 == strcmp(device_type, "ECU_EX_SYS")) {
            base_id = NIU_ECU;
            soft_id = NIU_ID_ECU_EX_MODEL_VERSION;
            hard_id = NIU_ID_ECU_ECU_HARD_VER;
        }
        else {
            LOG_D("get device version in fota upload log while device type unrecognized, %s", device_type);
            return;
        }
        temp_niu_data = NIU_DATA(base_id, soft_id);
        memcpy(g_fota_devices.devices[index].before_soft_version, temp_niu_data->addr, temp_niu_data->len);
        temp_niu_data = NIU_DATA(base_id, hard_id);
        memcpy(g_fota_devices.devices[index].hard_version, temp_niu_data->addr, temp_niu_data->len);
    }
}

unsigned char get_fota_download_percent()
{
    return g_fota_download_percent;
}

void add_niu_data_to_json(cJSON *json_obj, char *label, int base, int id)
{
    unsigned char tempchar = 0;
    int length = 0;
    char *str = NULL;
    char tempstring[9] = {0};
    NIU_DATA_VALUE *temp_niu_data = NULL;
    unsigned char tempbuf[128] = {0};
    memset(tempbuf, 0, 128);
    temp_niu_data = NIU_DATA(base, id);
    switch(temp_niu_data->type) {
        case NIU_UINT8:
            if(temp_niu_data->len == 1) {
                if((NIU_ID_BMS_BMS_S_VER == id) || (NIU_ID_BMS_BMS_H_VER == id) || 
                        (NIU_ID_BMS2_BMS2_S_VER == id) || (NIU_ID_BMS2_BMS2_H_VER == id)) {
                    memset(tempstring, 0, sizeof(tempstring));
                    tempchar = *((unsigned char*)temp_niu_data->addr);
                    tempstring[0] = tempchar;
                    cJSON_AddItemToObject(json_obj, label, cJSON_CreateString(tempstring));
                }
                else {
                    cJSON_AddItemToObject(json_obj, label, cJSON_CreateNumber(*((unsigned char*)temp_niu_data->addr)));
                }
            }
            break;
        case NIU_UINT16:
            cJSON_AddItemToObject(json_obj, label, cJSON_CreateNumber(*((unsigned short*)temp_niu_data->addr)));
            break;
        case NIU_INT32:
            if(temp_niu_data->len == 4) {
                cJSON_AddItemToObject(json_obj, label, cJSON_CreateNumber(*((unsigned int*)temp_niu_data->addr)));
            }
            break;
        case NIU_DOUBLE:
            if(temp_niu_data->len == 8) {
                cJSON_AddItemToObject(json_obj, label, cJSON_CreateNumber(*((double*)temp_niu_data->addr)));
            }
            break;
        case NIU_STRING:
            if(temp_niu_data->len) {
                if(NIU_ID_FOC_FOC_MODE == id) {
                    length = 17;
                }
                else {
                    length = temp_niu_data->len + 1;
                }

                str = (char *)QL_MEM_ALLOC(length);
                if(NULL == str) {
                    LOG_D("add niu string data to json while QL_MEM_ALLOC failure");
                    return;
                }
                else {
                    memset(str, 0, length);
                    memcpy(str, temp_niu_data->addr, length -1);
                    if(NIU_ID_FOC_FOC_MODE == id) {
                        if(strcmp(label, "soft_version")) {
                            memcpy(tempstring, str, 8);
                            cJSON_AddItemToObject(json_obj, label, cJSON_CreateString(tempstring));
                        }
                        else {
                            memcpy(tempstring, str + 8, 8);
                            cJSON_AddItemToObject(json_obj, label, cJSON_CreateString(tempstring));
                        }
                    }
                    else {
                        cJSON_AddItemToObject(json_obj, label, cJSON_CreateString(str));
                    }
                    QL_MEM_FREE(str);
                    str = NULL;
                }
            }
            break;
        default:
            break;
    }
}

void add_device_to_log_json(cJSON* obj, char *device_type)
{
    int i = 0;
    char temp_str[32] = {0};
    for(i = 0; i < g_fota_devices.devices_num; i++) {
        if(0 == strcmp(g_fota_devices.devices[i].type, device_type)) {
            cJSON_AddItemToObject(obj, "devicetype", cJSON_CreateString(g_fota_devices.devices[i].type));
            cJSON_AddItemToObject(obj, "before_soft_version", cJSON_CreateString(g_fota_devices.devices[i].before_soft_version));
            cJSON_AddItemToObject(obj, "update_soft_version", cJSON_CreateString(g_fota_devices.devices[i].version));
            cJSON_AddItemToObject(obj, "hard_version", cJSON_CreateString(g_fota_devices.devices[i].hard_version));
            cJSON_AddItemToObject(obj, "md5", cJSON_CreateString(g_fota_devices.devices[i].md5));
            snprintf(temp_str, sizeof(temp_str), "%d", g_fota_devices.devices[i].result);
            cJSON_AddItemToObject(obj, "updatestatus", cJSON_CreateString(temp_str));
            return;
        }
    }
}

void get_random_string(char *random_string, int random_len)
{
    int i = 0;
    int pool_len = 0;
    struct timeval now_time;
    unsigned int seed  = 0;
    char char_pool[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    if((NULL == random_string) || (random_len < 33)) {
        LOG_D("get random string failed while param is not valid");
        return;
    }
    pool_len = strlen(char_pool);
    memset(random_string, 0, random_len);
    seed = now_time.tv_sec + now_time.tv_usec;
    srandom(seed);
    for(i = 0; i < 32; i++) {
        random_string[i] = char_pool[random() % pool_len];
    }
}

void get_fota_url_param(char *url_param, int url_param_len)
{
    int i = 0;
    unsigned int timestamp = 0;
    char eid[17] = {0};
    char random_string[33] = {0};
    char sign[33] = {0};
    unsigned char aes_key[17] = {0};
    NIU_DATA_VALUE *temp_data_value = NULL;
    
    char dictionary[256] = {0};
    NIU_DATA_VALUE *temp_niu_data = NULL;

    if((NULL == url_param) || (url_param_len < 256)) {
        LOG_D("get fota url param while param is not valid");
        return;
    }

    temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_EID);
    memcpy(eid, temp_niu_data->addr, temp_niu_data->len);
    get_random_string(random_string, 33);
    temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_TIMESTAMP);
    timestamp = *((unsigned int *)temp_niu_data->addr);

    //niu_data_config_read(NIU_DATA_CONFIG_AES_KEY, aes_key, 16);
    temp_data_value = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
    if(temp_data_value)
    {
        memcpy(aes_key, temp_data_value->addr, SECTION_LEN);
    }
    
    memset(dictionary, 0, 256);
    snprintf(dictionary, sizeof(dictionary), "eid=%s&noncestr=%s&timestamp=%d&aeskey=%s", eid, random_string, timestamp, aes_key);
    get_md5_value(dictionary, sign, 33, 1);
    for(i = 0; i < 32; i++) {
        if((sign[i] <= 'z') && (sign[i] >= 'a')) {
            sign[i] = sign[i] - ('a' - 'A');
        }
    }
    snprintf(url_param, url_param_len, "eid=%s&noncestr=%s&timestamp=%d&sign=%s", eid, random_string, timestamp, sign);
}

int encrypt_data_in_cbc_mode(unsigned char *s_data, int s_data_len, unsigned char *iv, int iv_len, unsigned char *key, int key_len, unsigned char *d_data, int d_data_len)
{
    int i = 0;
    int block_num = 0;
    int mod_num = 0;
    unsigned char input[SECTION_LEN] = {0};
    unsigned char output[SECTION_LEN] = {0};
    unsigned char new_iv[SECTION_LEN] = {0};

    if((NULL == s_data) || (NULL == d_data) || 
            (NULL == iv) || (SECTION_LEN != iv_len) || 
            (NULL == key) || (SECTION_LEN != key_len)) {
        LOG_D("encrypt data while param is not valid");
        return 0;
    }

    memcpy(new_iv, iv, iv_len);
    block_num = (s_data_len / SECTION_LEN) + 1;
    mod_num = s_data_len % SECTION_LEN;

    for(i = 0; i < block_num; i++) {
        if((i + 1) == block_num) {
            if(0 != mod_num) {
                memcpy(input, s_data + i * SECTION_LEN, mod_num);
                memset(input + mod_num, SECTION_LEN - mod_num, SECTION_LEN - mod_num);
            }
            else {
                memset(input, SECTION_LEN, SECTION_LEN);
            }
        }
        else {
            memcpy(input, s_data + i * SECTION_LEN, SECTION_LEN);
        }
        AES128_CBC_encrypt_buffer2(output, input, SECTION_LEN, key, new_iv);
        memcpy(new_iv, output, SECTION_LEN);
        memcpy(d_data + i * SECTION_LEN, output, SECTION_LEN);
    }
    return block_num * SECTION_LEN;
}

int decrypt_data_in_cbc_mode(unsigned char *s_data, int s_data_len, unsigned char *iv, int iv_len, unsigned char *key, int key_len, unsigned char *d_data, int d_data_len)
{
    int i = 0;
    int block_num = 0;
    unsigned char input[SECTION_LEN] = {0};
    unsigned char output[16] = {0};
    unsigned char new_iv[SECTION_LEN] = {0};

    if((NULL == s_data) || (0 != (s_data_len % SECTION_LEN)) || 
            (NULL == d_data) || 
            (NULL == iv) || (SECTION_LEN != iv_len) || 
            (NULL == key) || (SECTION_LEN != key_len)) {
        LOG_D("decrypt data while param is not valid");
        return 0;
    }

    memcpy(new_iv, iv, iv_len);
    block_num = s_data_len / SECTION_LEN;
    
    for(i = 0; i < block_num; i++) {
        memcpy(input, s_data + i * SECTION_LEN, SECTION_LEN);
        AES128_CBC_decrypt_buffer2(output, input, SECTION_LEN, key, new_iv);
        memcpy(new_iv, input, SECTION_LEN);
        memcpy(d_data + i * SECTION_LEN, output, SECTION_LEN);
    }
    return s_data_len - d_data[s_data_len - 1];
}

void printf_data(unsigned char *data, int data_len, int num_in_line)
{
    int i = 0;
    if(NULL == data) {
        printf("display data while data is NULL\n");
        return;
    }
    for(i = 0; i < data_len; i++) {
        if((i > 0) && (0 == (i % num_in_line))) {
            printf("\n");
        }
        printf("%02x ", data[i]);
    }
    printf("\n");
}

void test_aes_cbc()
{
    int i = 0;
    int encrypt_length = 0;
    int decrypt_length = 0;
    unsigned char iv[SECTION_LEN] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    unsigned char key[SECTION_LEN] = {0x29,0xb3,0x5e,0x88,0xef,0xa3,0xd7,0x68,0x45,0xf4,0xc9,0xaa,0x5f,0x9e,0x19,0xaa};
    unsigned char source1[37] = {0};
    unsigned char source2[128] = {0};
    unsigned char dest[128] = {0};
    for(i = 0; i < 37; i++) {
        source1[i] = i + 1;
    }
    printf("the source data is:\n");
    printf_data(source1, 37, SECTION_LEN);
    
    encrypt_length = encrypt_data_in_cbc_mode(source1, 37, iv, SECTION_LEN, key, SECTION_LEN, dest, 128);
    printf("after encrypt, data length is %d, the data is:\n", encrypt_length);
    printf_data(dest, 128, SECTION_LEN);

    decrypt_length = decrypt_data_in_cbc_mode(dest, encrypt_length, iv, SECTION_LEN, key, SECTION_LEN, source2, 128);
    printf("after decrypt, data length is %d, the data is:\n", decrypt_length);
    printf_data(source2, 128, SECTION_LEN);
}

void db_upgrade_cb(int index)
{}

void foc_upgrade_cb(int index)
{}

void bms_upgrade_cb(int index)
{}

void bms2_upgrade_cb(int index)
{}

void alarm_upgrade_cb(int index)
{}

void lcu_upgrade_cb(int index)
{}

void qc_upgrade_cb(int index)
{}

void lku_upgrade_cb(int index)
{}

void bcs_upgrade_cb(int index)
{}

void tbs_upgrade_cb(int index)
{}

void cpm_upgrade_cb(int index)
{}

void emcu_upgrade_cb(int index)
{
    int status = 0;
    unsigned char device_type;
    char file_name[33] = {0};
    unsigned char key[16] = {0};
    char file_path[128] = {0};
    char new_file_path[128] = {0};
    NIU_DATA_VALUE *value = NULL;
    int battery_percent = 0;
    JBOOL battery_state = JFALSE;

    value = NIU_DATA_ECU(NIU_ID_ECU_ECU_BATTARY);
    if(value)
    {
        memcpy(&battery_percent, value->addr, value->len);
    }

    battery_state = niu_car_state_get(NIU_STATE_BATTERY);

    LOG_D("!!!!!!!!!!!!!!!!!!!!!!!!fota emcu: battery state: %d, battery_percent: %d", battery_state, battery_percent);
    if(battery_state == JFALSE && battery_percent < 60)
    {
        LOG_D("fota emcu ignor: battery state: %d, battery_percent: %d", battery_state, battery_percent);
        g_fota_devices.devices[index].result = UPGRADE_CODE_OTHER_ERROR;
        check_all_devices_upgrade_completed();
        return;
    }

    memset(file_path, 0, sizeof(file_path));
    memset(new_file_path, 0, sizeof(new_file_path));
    get_last_item(g_fota_devices.devices[index].url, strlen(g_fota_devices.devices[index].url), '/', file_name, 33);
    get_stm32_tea_key(key, sizeof(key));
    snprintf(file_path, sizeof(file_path), "%s/%s", FOTA_UPGRADE_PATH, file_name);
    snprintf(new_file_path, sizeof(new_file_path), "%s/%s_decrypt", FOTA_UPGRADE_PATH, file_name);
    
    device_type = 0x41;
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_OTA_EXEC_EQP, (NUINT8*)&device_type, sizeof(device_type));

    LOG_D("upgrade emcu, decrypt file is %s", file_path);
    status = TEA_decrypt_file(file_path, new_file_path, key);
    if(status >= 0) {
        if(0 == mcu_upgrade_to_stm32(new_file_path, g_fota_devices.devices[index].type)) {
        }
        else {
            g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
            check_all_devices_upgrade_completed();
        }
    }
    else {
        g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
        check_all_devices_upgrade_completed();
    }
}

void ecu_ex_app_upgrade_cb(int index)
{
    int status = 0;
    int retry = 3;
    unsigned char key[16] = {0};
    char file_path[128] = {0};
    char new_file_path[128] = {0};
    memset(file_path, 0, sizeof(file_path));
    memset(new_file_path, 0, sizeof(new_file_path));
    char file_name[33] = {0};
    char cmd[256] = {0};

    g_is_reboot = 0;
    get_last_item(g_fota_devices.devices[index].url, strlen(g_fota_devices.devices[index].url), '/', file_name, 33);

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "cp %s/%s %s/%s.old", APP_PATH, NIU_APP, FOTA_BACKUP_PATH, NIU_APP);
    if(0 == do_system(cmd, retry)) {
        memset(cmd, 0, sizeof(cmd));
        get_ec25_tea_key(key, sizeof(key));
        snprintf(file_path, sizeof(file_path), "%s/%s", FOTA_UPGRADE_PATH, file_name);
        snprintf(new_file_path, sizeof(new_file_path), "%s/%s_decrypt", FOTA_UPGRADE_PATH, file_name);

        LOG_D("upgrade ecu_ex_app, decrypt file is %s", file_path);
        status = TEA_decrypt_file(file_path, new_file_path, key);
        if(status >= 0) {
            snprintf(cmd, sizeof(cmd), "cp %s %s/%s", new_file_path, APP_PATH, NIU_APP);
            if(0 == do_system(cmd, retry)) {
                memset(cmd, 0, sizeof(cmd));
                snprintf(cmd, sizeof(cmd), "chmod +x %s/%s", APP_PATH, NIU_APP);
                if(0 == do_system(cmd, retry)) {
                    g_fota_devices.devices[index].result = UPGRADE_CODE_SUCCESS;
                    g_is_reboot = 1;
                    
                    //pony.ma added to copy file which after update to backup at 20190612
                    snprintf(cmd, sizeof(cmd), "cp %s/%s %s", APP_PATH, NIU_APP,APP_BACKUP_PATH);
                    if(0 == do_system(cmd, retry)){
                        memset(cmd, 0, sizeof(cmd));
                        LOG_D("copy %s/%s to %s success", APP_PATH, NIU_APP,APP_BACKUP_PATH);
                    }
                    //end add
                }
                else {
                    g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
                    g_is_reboot = 0;
                    LOG_D("change %s/%s mode failure", APP_PATH, NIU_APP);
                }
            }
            else {
                LOG_D("upgrade file(copy) %s to %s/%s failure", new_file_path, APP_PATH, NIU_APP);
                g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
                g_is_reboot = 0;
            }
        }       
        else {
            LOG_D("decrypt file %s failure", file_path);
            g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
            g_is_reboot = 0;
        }
    }
    else {
        LOG_D("backup %s/%s failed", APP_PATH, NIU_APP);
        g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
        g_is_reboot = 0;
    }
    LOG_D("ecu_ex_app upgrade completed, reboot flag is %d", g_is_reboot);
    check_all_devices_upgrade_completed();
}

void ecu_ex_sys_upgrade_cb(int index)
{
    int i = 0, retry = 3, cmd_count = 0;
    char file_name[33] = {0};
    char cmd[256] = {0};
    int mtd_num = 0;

    cmd_count = sizeof(g_prev_upgrade_sys) / sizeof(*g_prev_upgrade_sys);
    for(i = 0; i < cmd_count; i++) {
        if(0 != do_system(g_prev_upgrade_sys[i], retry)) {
            LOG_D("upgrade system while command %s failed in %d times", g_prev_upgrade_sys[i], retry);
            g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
            check_all_devices_upgrade_completed();
            return;
        }
    }
    
    get_last_item(g_fota_devices.devices[index].url, strlen(g_fota_devices.devices[index].url), '/', file_name, 33);
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "mv %s/%s %s", FOTA_UPGRADE_PATH, file_name, FOTA_SYS_UPDATE_FILE);
    if(0 != do_system(cmd, retry)) {
        LOG_D("upgrade system while command %s failed in %d times", cmd, retry);
        g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
        check_all_devices_upgrade_completed();
        return;
    }

    mtd_num = get_mtd_num();
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "ubiattach -m %d -d 3 /dev/ubi_ctrl", mtd_num);
    if(0 != do_system(cmd, retry)) {
        LOG_D("upgrade system while command %s failed in %d times", cmd, retry);
        g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
        check_all_devices_upgrade_completed();
        return;
    }

    cmd_count = sizeof(g_upgrade_sys) / sizeof(*g_upgrade_sys);
    for(i = 0; i < cmd_count; i++) {
        if(0 != do_system(g_upgrade_sys[i], retry)) {
            LOG_D("upgrade system while command %s failed in %d times", g_upgrade_sys[i], retry);
            g_fota_devices.devices[index].result = UPGRADE_CODE_INSTALL_FAIL;
            check_all_devices_upgrade_completed();
            return;
        }
    }
}

void set_fota_upgrade_device_result(void *param, int len)
{
    int i = 0;
    fota_upgrade_device_completed_t *temp_param = NULL;
    if(NULL == param) {
        LOG_D("set fota upgrade device result while parma is NULL, FOTA_EVENT_UPGRADE_COMPLETED and FOTA_EVENT_UPLOAD_LOG_2_COMPLETED will not send forever");
        return;
    }
    temp_param = (fota_upgrade_device_completed_t *)param;
    if(0 != strcmp("ECU_EX_APP", temp_param->type)) {
        for(i = 0; i< g_fota_devices.devices_num; i++) {
            if(0 == strcmp(temp_param->type, g_fota_devices.devices[i].type)) {
                g_fota_devices.devices[i].result = temp_param->result;
                break;
            }
        }
    }
    QL_MEM_FREE(param);
    param = NULL;
    check_all_devices_upgrade_completed();
}

void check_all_devices_upgrade_completed()
{
    int i = 0;
    for(i = 0; i < g_fota_devices.devices_num; i++) {
        if((1 == g_fota_devices.devices[i].hasnew) && (1 == g_fota_devices.devices[i].enable_upgrade)) {
            if((UPGRADE_CODE_SUCCESS == g_fota_devices.devices[i].result) || 
                    (UPGRADE_CODE_INSTALL_FAIL == g_fota_devices.devices[i].result)) {
                continue;
            }
            else {
                return;
            }
        }
    }
    sys_event_push(FOTA_EVENT_UPGRADE_COMPLETED, NULL, 0);
}

int get_mtd_num()
{
    int ret = DEFAULT_MTD_NUM;
    char cmd[] = "cat /proc/mtd | grep -w recoveryfs | awk -F[\"d\"\":\"] \'{printf $2}\'";
    char cmdinfo[16] = {0};
    FILE *cmdp = NULL;
    cmdp = popen(cmd, "r");
    if(NULL == cmdp) {
        LOG_D("get mtd_num failed while upgrading system, use default mtd_num %d", DEFAULT_MTD_NUM);
        return ret;
    }
    while(NULL != fgets(cmdinfo, sizeof(cmdinfo), cmdp)) {
        if((strlen(cmdinfo) < 15) && (strlen(cmdinfo) > 0)) {
            ret = atoi(cmdinfo);
        }
    }
    pclose(cmdp);
    return ret;
}
