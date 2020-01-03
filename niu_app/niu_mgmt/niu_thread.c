#include "cwpack.h"
#include "aes2.h"
#include "service.h"
#include "niu_thread.h"
#include "ql_mutex.h"
#include "ql_customer.h"
#include "sys_core.h"
#include "niu_data_pri.h"
#include "upload_error_lbs.h"
#include "niu_fota.h"
#include "low_power.h"
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>

#define RECONNECT_INTERVAL 150*1000
#define RECONNECT_EXCHANGE_SERVER_INTERVAL 10*60*1000*1000
#define TEMPERATURE_VALUE_ERROR -9999

#define TEMPERATURE_FILE_PATH_4G_MODULE "/sys/devices/qpnp-vadc-8/xo_therm"

MQTTClient g_mqttclient;
int g_history_data_count = 0;
int g_gps_upload_flag = 0;
int g_log_upload_flag = 0;
int g_sub_topic_flag = 0;
base_info_list_t g_base_info_list;
char subscribe_topic[50] = {0};
unsigned char g_mqtt_inter_send_buf[MQTT_BUF_LEN] = {0};
unsigned char g_mqtt_inter_recv_buf[MQTT_BUF_LEN] = {0};
history_data_t g_history_data_content[HISTORY_MAX_NUM];

int g_connect_flag = 0;
pthread_mutex_t g_connect_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_connect_cond = PTHREAD_COND_INITIALIZER;


typedef void(*signal_handler_t)(int);
extern void sys_event_push(SYS_EVENT_E cmd, void *param, int param_len);

int payload_pack_encrypt(NIU_DATA_VALUE **data_array, unsigned int data_array_size, unsigned short packed_num, unsigned char *result_data, int result_data_len)
{
    int i = 0, j = 0, mod_num = 0, return_code = 0;
    unsigned int packed_len, block_num = 0;
    char* str = NULL;
    unsigned char input[SECTION_LEN] = {0};
    unsigned char tempbuf_aes_key[SECTION_LEN] = {0};
    NIU_DATA_VALUE *temp_data_value = NULL; 
    memset(tempbuf_aes_key, 0, SECTION_LEN);

    cw_pack_context pc = {0};
    memset(result_data, 0, result_data_len);
    return_code = cw_pack_context_init(&pc, result_data, result_data_len, NULL);
    return_code = pc.return_code;
    
    cw_pack_array_size(&pc, data_array_size + 1);
    if(packed_num > 0) {
        cw_pack_signed(&pc, packed_num);
    }
    while(i < data_array_size &&  0 == return_code) {
        if(NULL == data_array[i]) {
            cw_pack_nil(&pc);
            return_code = pc.return_code;
        }
        else {
            if(data_array[i]->addr && data_array[i]->len ) {
                switch(data_array[i]->type){
                case NIU_INT8:
                    if (data_array[i]->len == 1) {
                        cw_pack_signed(&pc, *((char*)data_array[i]->addr));
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_INT16:
                    if (data_array[i]->len == 2) {
                        cw_pack_signed(&pc, *((short*)data_array[i]->addr));
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_INT32:
                    if (data_array[i]->len == 4) {
                        cw_pack_signed (&pc, *((int*)data_array[i]->addr));
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_UINT8:
                    if (data_array[i]->len == 1) {
                        cw_pack_signed (&pc, *((unsigned char*)data_array[i]->addr));
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_UINT16:
                    if (data_array[i]->len == 2) {
                        cw_pack_unsigned (&pc, *((unsigned short*)data_array[i]->addr));
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_UINT32:
                    if (data_array[i]->len == 4) {
                        cw_pack_unsigned (&pc, *((unsigned int*)data_array[i]->addr));
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_FLOAT:
                    if (data_array[i]->len == sizeof(float)) {
                        cw_pack_float(&pc, *((float*)data_array[i]->addr));
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_DOUBLE:
                    if (data_array[i]->len == sizeof(double)) {
                        cw_pack_double(&pc, *((double*)data_array[i]->addr));
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_STRING:
                    if (data_array[i]->len) {
                        str = (char *)QL_MEM_ALLOC(data_array[i]->len+1);
                        if(NULL == str) {
                            LOG_V("msgpack string data while QL_MEM_ALLOC failure");
                            return 0;
                        }
                        if(str) {
                            memset(str, 0, data_array[i]->len + 1);
                            memcpy(str, data_array[i]->addr, data_array[i]->len);
                            cw_pack_str(&pc, str, strlen(str));
                            QL_MEM_FREE(str);
                            str = NULL;
                        }
                        else {
                            cw_pack_str(&pc, data_array[i]->addr, data_array[i]->len);
                        }
                        return_code = pc.return_code;
                    }
                    break;
                case NIU_ARRAY:
                    cw_pack_bin(&pc, data_array[i]->addr, data_array[i]->len);
                    return_code = pc.return_code;
                    break;
                }
            }
        }
        i++;
    }
    packed_len = pc.current - pc.start;
    block_num = (packed_len / SECTION_LEN) + 1;
    mod_num = packed_len % SECTION_LEN; 
    temp_data_value = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
    if(temp_data_value)
    {
        memcpy(tempbuf_aes_key, temp_data_value->addr, SECTION_LEN);
    }
    //niu_data_config_read(NIU_DATA_CONFIG_AES_KEY, tempbuf_aes_key, SECTION_LEN);

    for(i = 0; i < block_num; i++) {
        memset(input, 0, SECTION_LEN);
        if((i + 1) == block_num) {
            if(mod_num != 0) {
                memcpy(input, result_data + (i * SECTION_LEN), mod_num);
                for(j = mod_num; j < SECTION_LEN; j++) {
                    input[j] = SECTION_LEN - mod_num;
                }
            }
            else {
                memset(input, 16, SECTION_LEN);
            }
        }
        else {
            memcpy(input, result_data + (i * SECTION_LEN), SECTION_LEN);
        }
        AES128_ECB_encrypt2(input, tempbuf_aes_key, result_data + (i * SECTION_LEN));
    }
    return block_num * 16;
}

int payload_encrypt(unsigned char *data, int data_len, unsigned char *result_data, int result_data_len)
{
    int i = 0;
    int block_num = 0, mod_num = 0;
    unsigned char input[SECTION_LEN] = {'\0'};
    unsigned char tempbuf_aes_key[SECTION_LEN] = {0};
    NIU_DATA_VALUE *temp_data_value = NULL;
    memset(tempbuf_aes_key, 0, SECTION_LEN);
    memset(result_data, 0, result_data_len);
    
    block_num = (data_len / SECTION_LEN) + 1;
    mod_num = data_len % SECTION_LEN; 
    
    temp_data_value = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
    if(temp_data_value)
    {
        memcpy(tempbuf_aes_key, temp_data_value->addr, SECTION_LEN);
    }
    //niu_data_config_read(NIU_DATA_CONFIG_AES_KEY, tempbuf_aes_key, SECTION_LEN);

    for(i = 0; i < block_num; i++) {
        memset(input, 0, SECTION_LEN);
        if((i + 1) == block_num) {
            if(0 != mod_num) {
                memcpy(input, data + (i * SECTION_LEN), mod_num);
                memset(input + mod_num, SECTION_LEN - mod_num, SECTION_LEN - mod_num);
            }
            else {
                memset(input, 16, SECTION_LEN);
            }
        }
        else {
            memcpy(input, data + (i * SECTION_LEN), SECTION_LEN);
        }
        AES128_ECB_encrypt2(input, tempbuf_aes_key, result_data + (i * SECTION_LEN));
    }
    return block_num * 16;
}

int payload_decrypt(unsigned char *data, int data_len, unsigned char *result_data, int result_data_len)
{
    int i = 0, length = 0;
    int block_num = 0;
    NIU_DATA_VALUE *temp_data_value = NULL;
    LOG_V("%s entry", __FUNCTION__);
    unsigned char tempbuf_aes_key[SECTION_LEN] = {0};
    memset(tempbuf_aes_key, 0, SECTION_LEN);

    unsigned char input[SECTION_LEN] = {'\0'};
    memset(result_data, 0, result_data_len);
    
    block_num = data_len / SECTION_LEN; 
    temp_data_value = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
    if(temp_data_value)
    {
        memcpy(tempbuf_aes_key, temp_data_value->addr, SECTION_LEN);
    }
    //niu_data_config_read(NIU_DATA_CONFIG_AES_KEY, tempbuf_aes_key, SECTION_LEN);

    for(i = 0; i < block_num; i++) {
        memset(input, 0, SECTION_LEN);
        memcpy(input, data + (i * SECTION_LEN), SECTION_LEN);
        AES128_ECB_decrypt2(input, tempbuf_aes_key, result_data + (i * SECTION_LEN));
    }
    length = data_len - result_data[data_len - 1];
    LOG_V("%s exit", __FUNCTION__);
    return length;
}

//flag = 0,cmd
//flag = 1,write
void unpacked_cmd_write_data(unsigned char *data, int length, int flag)
{
    int base = 0;
    int id = 0;
    int item_index = 0;
    //upgrade_info_t *fota_info = NULL;
    NIU_DATA_VALUE *data_orginal = NULL;
    cwpack_item_types item_type = CWP_NOT_AN_ITEM;
    cw_unpack_context upc;
    
    if((NULL == data) || (length <= 0)) {
        return;
    }
    LOG_V("%s entry", __FUNCTION__);
    cw_unpack_context_init(&upc, data, length, NULL);
    cw_unpack_next(&upc);
    item_type = upc.item.type;
    while((upc.current - data) < length) {
        cw_unpack_next(&upc);
        item_type = upc.item.type;
        if(CWP_NOT_AN_ITEM == item_type) {
            break;
        }
        if((0 == (item_index % 3)) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            base = upc.item.as.u64;
        }
        else if((1 == (item_index % 3)) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            id = upc.item.as.u64;
        }
        else if(2 == (item_index % 3)) {
            data_orginal = NIU_DATA(base, id);
            if(NULL != data_orginal) {
                if((flag == 1) && (base == NIU_ECU) && (id == NIU_ID_ECU_CALU_SPEED_INDEX))
                    break;
                if(((data_orginal->type == NIU_INT8) || (data_orginal->type==NIU_INT16) || (data_orginal->type == NIU_INT32)) && 
                        ((CWP_ITEM_NEGATIVE_INTEGER == item_type) || (CWP_ITEM_POSITIVE_INTEGER == item_type))) {
                    if(data_orginal->type == NIU_INT8) {
                        NINT8 new_value = upc.item.as.i64;
                        niu_data_write_value(base, id, (NUINT8*)&new_value, sizeof(NINT8));
                    }
                    else if(data_orginal->type==NIU_INT16) {
                        NINT16 new_value = upc.item.as.i64;
                        niu_data_write_value(base, id, (NUINT8*)&new_value, sizeof(NINT16));
                    }
                    else if(data_orginal->type==NIU_INT32) {
                        NINT32 new_value = upc.item.as.i64;
                        if((NIU_ECU == base) && (NIU_ID_ECU_UTC == id)) {
                            if((new_value < -780) || (new_value > 720)) {
                                item_index++;
                                continue;
                            }
                        }
                        niu_data_write_value(base, id, (NUINT8*)&new_value, sizeof(NINT32));
                    }
                }
                else if(((data_orginal->type == NIU_UINT8) || (data_orginal->type==NIU_UINT16) || (data_orginal->type == NIU_UINT32)) && 
                        (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
                    if(data_orginal->type == NIU_UINT8) {
                        NUINT8 new_value = upc.item.as.i64;
                        niu_data_write_value(base, id, (NUINT8*)&new_value, sizeof(NUINT8));
                    }
                    else if(data_orginal->type==NIU_UINT16) {
                        NUINT16 new_value = upc.item.as.i64;
                        niu_data_write_value(base, id, (NUINT8*)&new_value, sizeof(NINT16));
                    }
                    else if(data_orginal->type==NIU_UINT32) {
                        NUINT32 new_value = upc.item.as.i64;
                        niu_data_write_value(base, id, (NUINT8*)&new_value, sizeof(NINT32));
                        if((NIU_ECU == base) && (NIU_ID_ECU_TIMESTAMP == id)) {
                            struct timeval tv;
                            tv.tv_sec = new_value;   //east eight area
                            tv.tv_usec = 0;
                            if(0 != settimeofday(&tv, NULL)) {
                                LOG_D("set system time failure, err is %s", strerror(errno));
                            }
                            low_power_check(LOCK_TYPE_AGPS, get_agps_time_out());
                        }
                    }
                }
                else if((data_orginal->type == NIU_FLOAT) && (CWP_ITEM_FLOAT == item_type)) {
                    NFLOAT new_value = upc.item.as.real;
                    niu_data_write_value(base, id, (NUINT8*)&new_value, sizeof(NFLOAT));
                }
                else if((data_orginal->type == NIU_DOUBLE) && (CWP_ITEM_DOUBLE == item_type)) {
                    double new_value = upc.item.as.long_real;
                    niu_data_write_value(base, id, (NUINT8*)&new_value, sizeof(double));
                }
                else if((data_orginal->type == NIU_STRING) && (CWP_ITEM_STR == item_type)) {
                    if(base == NIU_ECU && id == NIU_ID_ECU_BLE_MAC)
                    {
                        NCHAR ble_mac[8] = {0};
                        if(upc.item.as.str.length > 16)
                        {
                            LOG_E("NIU_ID_ECU_BLE_MAC length error[len=%d].", upc.item.as.str.length);
                        }
                        else
                        {
                            StrToHex(ble_mac, (char *)upc.item.as.str.start, upc.item.as.str.length);
                            //niu_trace_dump_hex("ble_mac!", ble_mac, sizeof(ble_mac));
                            niu_data_write_value(base, id, (NUINT8 *)ble_mac, sizeof(ble_mac));
                        }
                    }
                    else
                    {
                        niu_data_write_value(base, id, (NUINT8 *)upc.item.as.str.start, upc.item.as.str.length);

                    }
                    //printf("CWP_ITEM_STR base: %d, id: %d, len: %d\n", base, id, upc.item.as.str.length);
                }
                else if((data_orginal->type == NIU_ARRAY) && (CWP_ITEM_BIN == item_type)) {
                    niu_data_write_value(base, id, (NUINT8 *)upc.item.as.bin.start, upc.item.as.bin.length);
                }
            }
        }
        else {
            break;
        }
        item_index++;
    }

    LOG_V("%s exit", __FUNCTION__);
}

NIU_SERVER_UPL_TEMPLATE unpacked_template_data(unsigned char *data, int length)
{
    int template_index = 0;
    int add_index = 0;
    int item_index = 0;
    NIU_SERVER_UPL_TEMPLATE temp_template;
    memset(&temp_template, 0, sizeof(NIU_SERVER_UPL_TEMPLATE));
    cwpack_item_types item_type = CWP_NOT_AN_ITEM;
    cw_unpack_context upc;
    if((NULL == data) || (length <= 0)) {
        temp_template.index = NIU_SERVER_UPL_CMD_NUM;
        return temp_template;
    }
    cw_unpack_context_init(&upc, data, length, NULL);
    cw_unpack_next(&upc);
    item_type = upc.item.type;
    while((upc.current - data) < length)
    {
        cw_unpack_next(&upc);
        item_type = upc.item.type;

        if(CWP_NOT_AN_ITEM == item_type) {
            break;
        }
        if((0 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.index = upc.item.as.u64;
        }
        else if((1 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger0 = upc.item.as.u64;
        }
        else if((2 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger1= upc.item.as.u64;
        }
        else if((3 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger2 = upc.item.as.u64;
        }
        else if((4 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger3 = upc.item.as.u64;
        }
        else if((5 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger4 = upc.item.as.u64;
        }
        else if((6 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger5 = upc.item.as.u64;
        }
        else if((7 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger6 = upc.item.as.u64;
        }
        else if((8 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger7 = upc.item.as.u64;
        }
        else if((9 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger8 = upc.item.as.u64;
        }
        else if((10 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.trigger9 = upc.item.as.u64;
        }
        else if((11 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.packed_name = upc.item.as.u64;
        }
        else if((12 == item_index) && ((CWP_ITEM_NEGATIVE_INTEGER == item_type)  || (CWP_ITEM_POSITIVE_INTEGER == item_type))) {
            temp_template.trigged_count = upc.item.as.u64;
        }              
        else if((13 == item_index) && ((CWP_ITEM_NEGATIVE_INTEGER == item_type)  || (CWP_ITEM_POSITIVE_INTEGER == item_type))) {
            temp_template.send_max= upc.item.as.u64;
        }
        else if((14 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.interval = upc.item.as.u64;
        }
        else if((15 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.cur_time = upc.item.as.u64;
        }
        else if((16 == item_index) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            temp_template.addr_num = upc.item.as.u64;
        }
        else if((item_index >= 17) && (item_index < (17 + temp_template.addr_num * 2)) && (CWP_ITEM_POSITIVE_INTEGER == item_type)) {
            add_index = item_index - 17;
            if(add_index % 2) {
                temp_template.addr_array[(add_index / 2)].id = upc.item.as.u64;
            }
            else {
                temp_template.addr_array[(add_index / 2)].base= upc.item.as.u64;
            }                   
        }
        else {
            break;
        }
        item_index++;   
    }

    if(item_index >= ((17 + temp_template.addr_num * 2))) {
        template_index = get_overlayable_template_index(temp_template.index);
        if(-1 != template_index) {
            if(temp_template.index == 255) {
                temp_template.index = template_index;
            }
            niu_data_write_template(&temp_template, template_index);
            clear_upl_cmd_info(template_index, 1);
            niu_update_trigger_template();
        }
        else {
            LOG_D("server update template failure");
        }
        
        check_template_low_power();
    }
    return temp_template;
}

void message_arrived(MessageData* md)
{
    int length = 0;
    LOG_V("%s entry", __FUNCTION__);
    unsigned char *tempdata = (unsigned char *)QL_MEM_ALLOC(MQTT_BUF_LEN);
    if(NULL == tempdata) {
        LOG_D("message arived while QL_MEM_ALLOC tempdata failure");
        return;
    }

    MQTTMessage* message = md->message;
    LOG_D("receive message from %s, lenght is %d\n",md->topicName->lenstring.data ,(int)message->payloadlen);
    length = payload_decrypt((unsigned char *)message->payload, (int)message->payloadlen, tempdata, MQTT_BUF_LEN); 
    
    if(strstr(md->topicName->lenstring.data, SUB_TOPIC_WRITE)) {
        unpacked_cmd_write_data(tempdata, length, 1);
        //wakeup_fota_thread(FOTA_CHECK);
    }
    else if(strstr(md->topicName->lenstring.data, SUB_TOPIC_COMMAND)) {
        unpacked_cmd_write_data(tempdata, length, 0);
    }
    else if(strstr(md->topicName->lenstring.data, SUB_TOPIC_TEMPLATE)) {
        unpacked_template_data(tempdata, length);
    }
    else if(strstr(md->topicName->lenstring.data, SUB_TOPIC_GPS_CMD)) {
        LOG_D("gps cmd package...\n");
    }
    else {
        LOG_D("unrecognized topic %s", md->topicName->lenstring.data);
    }
    QL_MEM_FREE(tempdata);
    tempdata = NULL;
    LOG_V("%s exit", __FUNCTION__);
}

void *thread_mqtt_read_proc(void *param)
{
    pthread_detach(pthread_self());
    while(1) {  
        if((0 != g_sub_topic_flag) && (0 != g_mqttclient.isconnected)) {
            MQTTYield(&g_mqttclient, 1000);
        }
        usleep(1000);
    }
    return NULL;
}

/*
 * P is connect to Primary server
 * S is connect to Standby server
 *
 * determine current server and connect interval time according to g_reconnect_count 
 * "__" is RECONNECT_INTERVAL,___________ is RECONNECT_EXCHANGE_SERVER_INTERVAL
 *
 * P__P__P__S__S__S___________P__P__P___________S__S__S___________P__P__...
 *
 * before g_reconnect_count auto-increment
 * (g_reconnect_count / 3) % 2 is 0, connect to P
 * (g_reconnect_count / 3) % 2 is 1, connect to S
 *
 *  do connect operation
 *
 *  g_reconnect_count auto increment
 *  if g_reconnect_count >=6 && g_reconnect_count % 3 == 0, 
 *      reconnect interval is RECONNECT_EXCHANGE_SERVER_INTERVAL
 *  else
 *      reconnect interval is RECONNECT_INTERVAL
*/
void *thread_mqtt_connect_proc(void *param)
{
    int rc = 0, config_retry = 0;
    int reconnect_count = 0;
    int reconnect_interval = 0;
    unsigned int value = 0;
    NIU_DATA_VALUE *temp_niu_data = NULL;
    unsigned char tempbuf_server_url[32] = {0};
    //unsigned char tempbuf_server_port[4] = {0};
    int tempbuf_server_port = 0;
    unsigned char tempbuf_id[32] = {0};
    unsigned char tempbuf_user[32] = {0};
    unsigned char tempbuf_pass[32] = {0};
    unsigned char tempbuf_eid[32] = {0};
    unsigned char tempbuf_aes[17] = {0};
    Network g_network;
    
    pthread_detach(pthread_self());
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    while(1) {
        pthread_mutex_lock(&g_connect_mut);
        while(1 != g_connect_flag) {
            reconnect_count = 0;
            LOG_D("connect thread suspend");
            pthread_cond_wait(&g_connect_cond, &g_connect_mut);
        }
        pthread_mutex_unlock(&g_connect_mut);
        LOG_D("connect thread run");
        
        memset(tempbuf_eid, 0, 32);
        memset(tempbuf_id, 0, 32);
        memset(tempbuf_aes, 0, 17);
        temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_EID);
        if(temp_niu_data)
        {
            memcpy(tempbuf_eid, (unsigned char *)temp_niu_data->addr, temp_niu_data->len);
            //clientid is same as eid
            memcpy(tempbuf_id, (unsigned char *)temp_niu_data->addr, temp_niu_data->len);
        }

        memset(tempbuf_user, 0, 32);
        temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_SERVERUSERNAME);
        if(temp_niu_data)
        {
            memcpy(tempbuf_user, (unsigned char*)temp_niu_data->addr, temp_niu_data->len);
        }
        //niu_data_config_read(NIU_DATA_CONFIG_USERNAME, tempbuf_user, 32);


        memset(tempbuf_pass, 0, 32);
        temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_SERVERPASSWORD);
        if(temp_niu_data)
        {
            memcpy(tempbuf_pass, (unsigned char*)temp_niu_data->addr, temp_niu_data->len);
        }

        temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
        if(temp_niu_data)
        {
            memcpy(tempbuf_aes, temp_niu_data->addr, temp_niu_data->len);
        }
        //niu_data_config_read(NIU_DATA_CONFIG_PASSWORD, tempbuf_pass, 32);

        data.willFlag = 0;
        data.MQTTVersion = 3;
        data.clientID.cstring = (char *)tempbuf_id;
        data.username.cstring = (char *)tempbuf_user;
        data.password.cstring = (char *)tempbuf_pass;
        data.cleansession = 1;
        
        temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_ECU_KEEPALIVE);
        data.keepAliveInterval = *((NUINT32 *)(temp_niu_data->addr));

        memset(tempbuf_server_url, 0, 32);
        memset(&tempbuf_server_port, 0, sizeof(tempbuf_server_port));
        if(0 == ((reconnect_count / 3) % 2)) {
            //niu_data_rw_read(NIU_DATA_RW_SERVER_URL, tempbuf_server_url, 32);
            //niu_data_rw_read(NIU_DATA_RW_SERVER_PORT, tempbuf_server_port, 4);
            temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_SERVER_URL);
            if(temp_niu_data)
            {
                memcpy(tempbuf_server_url, temp_niu_data->addr, sizeof(tempbuf_server_url));
            }
            temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_SERVER_PORT);
            if(temp_niu_data)
            {
                memcpy((void*)&tempbuf_server_port, temp_niu_data->addr, sizeof(tempbuf_server_port));
            }
        }
        else {
            temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_SERVER_URL2);
            if(temp_niu_data)
            {
                memcpy(tempbuf_server_url, temp_niu_data->addr, temp_niu_data->len);
            }

            temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_SERVER_PORT2);
            if(temp_niu_data)
            {
                memcpy((void*)&tempbuf_server_port, temp_niu_data->addr, temp_niu_data->len);
            }
        }
        LOG_D("===============url: %s, port: %d, id: %s, user: %s, password: %s, aes_key: %s\n",
                tempbuf_server_url, tempbuf_server_port, tempbuf_id, tempbuf_user, tempbuf_pass, tempbuf_aes);

        if(strlen(tempbuf_server_url) == 0 || tempbuf_server_port == 0 || strlen(data.username.cstring) == 0 
            || strlen(data.password.cstring) == 0 || strlen(tempbuf_aes) == 0)
        {
            LOG_D("config param is invalid, try again after 1 sec.");
            ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 100, 1000);
            sleep(1);
            config_retry++;
            if(config_retry == 60)
            {
                LOG_D("resync local config file to mcu.\n");
                niu_data_config_sync_flag_set(0);
                niu_data_config_sync_mask_reset();
                niu_ecu_data_sync();

                config_retry = 0;
            }
            continue;
        }
       
        reconnect_count++;
        if((reconnect_count >= 6) && (0 == (reconnect_count % 3))) {
            reconnect_interval = RECONNECT_EXCHANGE_SERVER_INTERVAL;
        }
        else {
            reconnect_interval = RECONNECT_INTERVAL;
        }
        ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 1000, 0);
        NetworkInit(&g_network);
        rc = NetworkConnect(&g_network, (char *)tempbuf_server_url, tempbuf_server_port);
        if(0 != rc) {
            reset_socket(4);
            sys_event_push(SYS_EVENT_SOCKET_DISCONNECT, NULL, 0);
            usleep(reconnect_interval);
            continue;
        }
        else {
            sys_event_push(SYS_EVENT_SOCKET_CONNECTED, NULL, 0);
        }
        MQTTClientInit(&g_mqttclient, &g_network, 1000, g_mqtt_inter_send_buf, MQTT_BUF_LEN, g_mqtt_inter_recv_buf, MQTT_BUF_LEN);
        rc = MQTTConnect(&g_mqttclient, &data);
        if(0 != rc) {
            reset_socket(4);
            error_upload(NIU_ERROR_CONN_FAIL);
            sys_event_push(MQTT_EVENT_MQTT_DISCONNECT, NULL, 0);
            usleep(reconnect_interval);
            continue;
        }
        else {
            sys_event_push(MQTT_EVENT_MQTT_CONNECTED, NULL, 0);
        }
        snprintf(subscribe_topic, 50, "s/%s/#", tempbuf_eid);
        rc = MQTTSubscribe(&g_mqttclient, subscribe_topic, QOS1, message_arrived);
        if(0 != rc) {
            reset_socket(5);
            LOG_D("mqtt subscribe failure, ret is %d", rc);
            sys_event_push(MQTT_EVENT_MQTT_SUB_FAILED, NULL, 0);
            usleep(reconnect_interval);
            continue;
        }
        else {
            value = time(NULL) - DIFF_TIME;
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SERVER_CONNECTED_TIME, (NUINT8 *)&value, 4);
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SERVER_DO_CONNECT_COUNT, (NUINT8 *)&reconnect_count, 4);
            ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 100, 3000);
            sys_event_push(MQTT_EVENT_MQTT_SUB_SUCCEED, NULL, 0);
            g_sub_topic_flag = 1;
            niu_car_state_set(NIU_STATE_MQTT_CONNECTED, JTRUE);
        }
        mqtt_client_keep_alive(0, NULL);
        reconnect_count = 0;
        sleepdown_connect_thread();
        upload_history_data();
    }
    return NULL;
}

void sleepdown_connect_thread()
{
    pthread_mutex_lock(&g_connect_mut);
    if(0 != g_connect_flag) {
        g_connect_flag= 0;
        LOG_D("sleep down connect thread");
    }
    pthread_mutex_unlock(&g_connect_mut);
}

void wakeup_connect_thread()
{
    pthread_mutex_lock(&g_connect_mut);
    if(1 != g_connect_flag) {
        g_connect_flag= 1;
        LOG_D("wakeup connect thread");
        pthread_cond_signal(&g_connect_cond);
    }
    pthread_mutex_unlock(&g_connect_mut);
}

int mqtt_sub_preinit()
{
    int i = 0, rc = 0;
    pthread_t thread_mqtt_read;
    pthread_t thread_mqtt_conn;
    g_mqttclient.isconnected = 0;
    g_sub_topic_flag = 0;
    for(i = 0; i < HISTORY_MAX_NUM; i++) {
        g_history_data_content[i].data_type = -1;
        g_history_data_content[i].packed_name = 0;
        g_history_data_content[i].length = 0;
        g_history_data_content[i].data = NULL;
        g_history_data_content[i].data_time = 0;
    }
    
    memset(&g_base_info_list, 0, sizeof(base_info_list_t));
    
    rc = pthread_create(&thread_mqtt_read, NULL, (void *)thread_mqtt_read_proc, NULL);
    if(0 != rc) {
        LOG_F("mqtt event loop thread create fails.");
    }

    rc = pthread_create(&thread_mqtt_conn, NULL, (void *)thread_mqtt_connect_proc, NULL);
    if(0 != rc){
        LOG_F("mqtt connect thread create fails");
    }

    return 0;
}

void upload_gps_log_data(void *param)
{
    char publish_topic[50] = {0};
    unsigned char tempdata[MQTT_BUF_LEN] = {0};
    unsigned char tempbuf_eid[17] = {0};    /*length is 16,add '\0' char is 17*/
    NIU_DATA_VALUE *eid = NULL;
    MQTTMessage publish_msg; 
    memset(tempbuf_eid, 0, 17);
    if(NULL == param) {
        LOG_D("upload gps or log data while param is NULL");
        return;
    }
    upload_gps_log_t *upload_data = (upload_gps_log_t *)param;
    
    publish_msg.payloadlen = payload_encrypt((unsigned char *)(upload_data->data), upload_data->data_len, tempdata, MQTT_BUF_LEN);
    publish_msg.payload = (void *)tempdata;
    publish_msg.qos = QOS0;
    publish_msg.retained = 0;
    publish_msg.dup = 0;

    eid = NIU_DATA_ECU(NIU_ID_ECU_EID);
    memcpy(tempbuf_eid, (unsigned char *)eid->addr, eid->len);
    if(0 == upload_data->flag) {
        snprintf(publish_topic, 50, "t/%s/%s", tempbuf_eid, PUB_TOPIC_GPS);
    }
    else {
        snprintf(publish_topic, 50, "t/%s/%s", tempbuf_eid, PUB_TOPIC_LOG);
    }

    if(1 == g_mqttclient.isconnected) {
        MQTTPublish(&g_mqttclient, publish_topic, &publish_msg);
    }
    else {
        put_data_to_history(tempdata, publish_msg.payloadlen, -1, upload_data->flag);
    }

    QL_MEM_FREE(upload_data->data);
    upload_data->data = NULL;
    QL_MEM_FREE(upload_data);
    upload_data = NULL;
}

void upload_history_data()
{
    int i = 0, tempcount = 0;
    char publish_topic[50] = {0};
    MQTTMessage publish_msg; 
    publish_msg.qos = QOS0;
    publish_msg.retained = 0;
    publish_msg.dup = 0;
    
    unsigned char tempbuf_eid[17] = {0};//length is 16,add '\0' char is 17
    memset(tempbuf_eid, 0, 17);
    NIU_DATA_VALUE *eid = NULL;

    eid = NIU_DATA_ECU(NIU_ID_ECU_EID);
    memcpy(tempbuf_eid, (unsigned char *)eid->addr, eid->len);
    tempcount = g_history_data_count >= HISTORY_MAX_NUM ? HISTORY_MAX_NUM : g_history_data_count;

    for(i = 0; i < tempcount; i++) {
        memset(publish_topic, 0, 50);
        if(HISTORY_DATA_TEMPLATE == g_history_data_content[i].data_type) {
            snprintf(publish_topic, 50, "t/%s/%d", tempbuf_eid, g_history_data_content[i].packed_name);
        }
        else if(HISTORY_DATA_GPS == g_history_data_content[i].data_type) {
            snprintf(publish_topic, 50, "t/%s/%s", tempbuf_eid, PUB_TOPIC_GPS);
        }
        else if(HISTORY_DATA_LOG == g_history_data_content[i].data_type) {
            snprintf(publish_topic, 50, "t/%s/%s", tempbuf_eid, PUB_TOPIC_LOG);
        }
        else {
            LOG_D("upload history data while data_type not recognized");
            QL_MEM_FREE(g_history_data_content[i].data);
            g_history_data_content[i].data= NULL;
            g_history_data_content[i].data_type = -1;
            g_history_data_content[i].length = 0;
            g_history_data_content[i].packed_name = 0;
            g_history_data_content[i].data_time = 0;
            continue;
        }
        publish_msg.payload = g_history_data_content[i].data;
        publish_msg.payloadlen = g_history_data_content[i].length;

        if(1 == g_mqttclient.isconnected) {
            MQTTPublish(&g_mqttclient, publish_topic, &publish_msg);
        }
        
        QL_MEM_FREE(g_history_data_content[i].data);
        g_history_data_content[i].data= NULL;
        g_history_data_content[i].data_type = -1;
        g_history_data_content[i].length = 0;
        g_history_data_content[i].packed_name = 0;
        g_history_data_content[i].data_time = 0;        
    }
    g_history_data_count = 0;
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_PDU_MESSAGE_CURRENT_NUMBER, (NUINT8*)&g_history_data_count, sizeof(NINT16));
}

void put_data_to_history(unsigned char *data, int length, int packed_name, HISTORY_DATA_TYPE data_type)
{
    unsigned char *temp_data = (unsigned char *)QL_MEM_ALLOC(length);
    if(NULL == temp_data) {
        LOG_D("save history data while QL_MEM_ALLOC failed");
        return;
    }
    memcpy(temp_data, data, length);
    
    if(NULL != g_history_data_content[g_history_data_count % HISTORY_MAX_NUM].data) {
        QL_MEM_FREE(g_history_data_content[g_history_data_count % HISTORY_MAX_NUM].data);
        g_history_data_content[g_history_data_count % HISTORY_MAX_NUM].data = NULL;
    }

    g_history_data_content[g_history_data_count % HISTORY_MAX_NUM].data_type = data_type;
    g_history_data_content[g_history_data_count % HISTORY_MAX_NUM].data = temp_data;
    g_history_data_content[g_history_data_count % HISTORY_MAX_NUM].length = length;
    g_history_data_content[g_history_data_count % HISTORY_MAX_NUM].packed_name = packed_name;
    g_history_data_content[g_history_data_count % HISTORY_MAX_NUM].data_time = time(NULL) - DIFF_TIME;

    g_history_data_count++;
    if(g_history_data_count <= HISTORY_MAX_NUM) {
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_PDU_MESSAGE_CURRENT_NUMBER, (NUINT8*)&g_history_data_count, sizeof(NINT16));
    }
}

void upload_log_info(char *format, ...)
{  
    if(1 == g_log_upload_flag) {
        va_list ap;
        int length = 0;
        char* log_buf = NULL;
    
        upload_gps_log_t *upload_data = (upload_gps_log_t *)QL_MEM_ALLOC(sizeof(upload_gps_log_t));
        if(NULL == upload_data) {
            LOG_V("handler log info while QL_MEM_ALLOC upload_data failure");
            return;
        }
        
        va_start(ap, format);
        length = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        if(length < 0) {
            LOG_V("handler log info while the result of calling vsnprintf is smaller than 0");
            return;
        }

        log_buf = (char *)QL_MEM_ALLOC(length + 1);
        if(NULL == log_buf) {
            LOG_V("handler log info while QL_MEM_ALLOC log_buf failure");
            return;
        }

        memset(log_buf, 0, length + 1);
        va_start(ap, format);
        vsnprintf(log_buf, length + 1, format, ap);
        va_end(ap);
        
        upload_data->flag = 1;
        upload_data->data = log_buf;
        upload_data->data_len = length;
        upload_gps_log_data(upload_data);
    }
}

void upload_gps_info(char *format, ...)
{  
    if(1 == g_gps_upload_flag) {
        va_list ap;
        int length = 0;
        char* gps_buf = NULL;
    
        upload_gps_log_t *upload_data = (upload_gps_log_t *)QL_MEM_ALLOC(sizeof(upload_gps_log_t));
        if(NULL == upload_data) {
            LOG_V("handler log info while QL_MEM_ALLOC upload_data failure");
            return;
        }
        
        va_start(ap, format);
        length = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        if(length < 0) {
            LOG_V("handler log info while the result of calling vsnprintf is smaller than 0");
            return;
        }

        gps_buf = (char *)QL_MEM_ALLOC(length + 1);
        if(NULL == gps_buf) {
            LOG_V("handler log info while QL_MEM_ALLOC gps_buf failure");
            return;
        }

        memset(gps_buf, 0, length + 1);
        va_start(ap, format);
        vsnprintf(gps_buf, length + 1, format, ap);
        va_end(ap);
        
        upload_data->flag = 0;
        upload_data->data = gps_buf;
        upload_data->data_len = length;
        upload_gps_log_data(upload_data);
    }
}

int do_system(const char *cmd, int retry)
{
    int i = 0, status = 0, ret = 0;
    if((NULL == cmd) || (retry <=0 )) {
        LOG_D("do_system while param is invalid");
        return 5;
    }
    for(i = 0; i < retry; i++) {
        signal_handler_t old_handler;
        old_handler = signal(SIGCHLD, SIG_DFL);
        status = system(cmd);
        signal(SIGCHLD, old_handler);
    
        if(status < 0) {
            LOG_D("cmd: %s,error:%s occured\n",cmd, strerror(errno));
            ret = 1;
            continue;
        }
        if(WIFEXITED(status)) {
            LOG_D("cmd %s normal termination, exit status = %d\n", cmd, WEXITSTATUS(status));
            ret = 0;
            break;
        }
        else if(WIFSIGNALED(status)) {
            LOG_D("cmd %s abnormal termination, signal number = %d\n", cmd, WTERMSIG(status));
            ret = 2;
            continue;
        }
        else if(WIFSTOPPED(status)) {
            LOG_D("cmd %s process stopped, signal number = %d\n", cmd, WSTOPSIG(status));
            ret = 3;
            continue;
        }
        else {
            ret = 4;
            continue;
        }
    }
    return ret;
}

int is_mqtt_connected()
{
    return g_mqttclient.isconnected;
}

int is_mqtt_ping()
{
    return g_mqttclient.ping_outstanding;
}

void set_mqtt_disconnected()
{
    g_mqttclient.isconnected = 0;
    g_sub_topic_flag = 0;
    delete_ping_timer();
}

void reset_socket(unsigned int error_flag)
{
    unsigned int value = 0;
    unsigned short temp_short = 0;
    NIU_DATA_VALUE *temp_value = NULL;
    LOG_D("reset socket, error flag is %d", error_flag);
    if(NULL != g_mqttclient.ipstack) {
        if(g_mqttclient.ipstack->my_socket > 0) {
            close(g_mqttclient.ipstack->my_socket);
            g_mqttclient.ipstack->my_socket = 0;
        }
        g_mqttclient.ipstack = NULL;
    }
    niu_car_state_set(NIU_STATE_MQTT_CONNECTED, JFALSE);
    set_mqtt_disconnected();
    if(0 != error_flag) {
        value = error_flag;
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SERVER_ERROR_CODE, (NUINT8*)&value, 4);

        value = time(NULL) - DIFF_TIME;
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SERVER_ERROR_CODE_TIME, (NUINT8 *)&value, 4);
    }
    
    temp_value = NIU_DATA_ECU(NIU_ID_ECU_ECU_CFG);
    temp_short = *((unsigned short *)temp_value->addr);
    if(temp_short & 0x0004) {  // no prof file
        ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 100, 1000);
    }
    else {
        ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 1000, 0);
    }
}

void reset_4g_count_update(int flag)
{
    unsigned int value = 0;
    NIU_DATA_VALUE *temp_data= NIU_DATA_ECU(NIU_ID_ECU_ECU_SERVER_RESET_4G_COUNT);
    if(NULL != temp_data) {
        value = *((NUINT32 *)temp_data->addr);
        value++;
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SERVER_RESET_4G_COUNT, (NUINT8 *)&value, 4);
    }

    if(0 == flag) {
        ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 1000, 0);
    }
    else {
        ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 100, 3000);
    }
}

int get_4g_temperature()
{
    char *temperature_flag = NULL;
    char *temperature_start = NULL;
    char *temperature_end = NULL;
    char temperature_str[4] = {0};
    struct stat file_info;
    char *buf = NULL;
    int temperature = 0;
  
    stat(TEMPERATURE_FILE_PATH_4G_MODULE, &file_info);
    
    FILE *file_handler = fopen(TEMPERATURE_FILE_PATH_4G_MODULE, "r");
    if(NULL == file_handler) {
        LOG_D("get 4g temperature while open file failure");
        return TEMPERATURE_VALUE_ERROR;
    }

    buf = (char *)QL_MEM_ALLOC(file_info.st_size);
    if(NULL == buf) {
        LOG_D("get 4g temperature while QL_MEM_ALLOC failure");
        fclose(file_handler);
        return TEMPERATURE_VALUE_ERROR;
    }

    fread(buf, file_info.st_size, 1, file_handler);
    temperature_flag = strstr(buf, "Result:");
    if(NULL == temperature_flag) {
        LOG_D("read 4g temperature failure");
        QL_MEM_FREE(buf);
        buf = NULL;
        fclose(file_handler);
        return TEMPERATURE_VALUE_ERROR;
    }
    else {
        temperature_start = temperature_flag + strlen("Result:");
        temperature_end = strstr(temperature_start, " ");
        if(NULL == temperature_end) {
            temperature_end = temperature_start + 2;
        }
        strncpy(temperature_str, temperature_start, temperature_end - temperature_start);
        temperature = atoi(temperature_str);
   
        QL_MEM_FREE(buf);
        buf = NULL;
        fclose(file_handler);
        return temperature;
    }
}

void write_base_station_to_datatable()
{
    char imei[32] = {0};
    short temp_short = 0;
    char base_format[255] = {0};
    unsigned char cell_buf[16] = {0};
    short signal = 0;
    double lon = 0;
    double lat = 0;
    NIU_DATA_VALUE *temp_niu_data;
    base_station_info_t base_info;

    temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_CSQ);
    signal = *((NUINT8 *)temp_niu_data->addr);
    temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_LONGITUDE);
    lon = *((NDOUBLE *)temp_niu_data->addr);
    temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_LATITUDE);
    lat = *((NDOUBLE *)temp_niu_data->addr);

    cmd_imei_info_get(imei, 32);
    get_base_station_info(&base_info);
    niu_data_clean_value(NIU_ECU, NIU_ID_ECU_ECU_CDMA_LOCATION);
    niu_data_clean_value(NIU_ECU, NIU_ID_ECU_ECU_NO_CDMA_LOCATION);
    niu_data_clean_value(NIU_ECU, NIU_ID_ECU_UTC);
    if(1 == base_info.non_cdma_flag) {
        memset(base_format, 0, 255);
        snprintf(base_format, 255, "accesstype=0&imei=%s&cdma=0&bts=%s,%s,%d,%d,%d", 
                imei, base_info.non_cdma_mcc, base_info.non_cdma_mnc, base_info.tac, base_info.cid, signal);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_NO_CDMA_LOCATION, (NUINT8 *)base_format, 255);
            
        memset(cell_buf, 0, 16);
        temp_short = atoi(base_info.non_cdma_mcc);
        memcpy(cell_buf + 0, &temp_short, 2);
        temp_short = atoi(base_info.non_cdma_mnc);
        memcpy(cell_buf + 2, &temp_short, 2);
        memcpy(cell_buf + 4, &(base_info.tac), 2);
        memcpy(cell_buf + 6, &(base_info.cid), 4);
        memcpy(cell_buf + 10, &signal, 2);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_NOW_BTS, (NUINT8 *)cell_buf, 16);
    }
    if(1 == base_info.cdma_flag) {
        memset(base_format, 0, 255);
        snprintf(base_format, 255, "accesstype=0&imei=%s&cdma=1&bts=%d,%d,%d,%f,%f,%d", 
                imei, base_info.sid, base_info.nid, base_info.bsid, lon, lat, signal);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_CDMA_LOCATION, (NUINT8 *)base_format, 255);
    }
}

int get_mcc_mnc_value(char *plmn, int plmn_len, unsigned short *mcc, unsigned short *mnc)
{
    int mcc1 = 0, mcc2 = 0, mcc3 = 0;
    int mnc1 = 0, mnc2 = 0, mnc3 = 0;
    
    if(plmn_len < 3) {
        LOG_D("get mcc mnc from plmn error while the length of plmn is %d", plmn_len);
        return 1;
    }

    mcc1 = plmn[0] & 0X0F;
    mcc2 = plmn[0] >> 4;
    mcc3 = plmn[1] & 0x0F;
    
    mnc3 = plmn[1] >> 4;
    mnc2 = plmn[2] >> 4;
    mnc1 = plmn[2] & 0x0F;
    
    *mcc = mcc1 * 100 + mcc2 * 10 + mcc3;
    
    if(0X0F == mnc3) {
        *mnc = mnc1 * 10 + mnc2;
    }
    else {
        *mnc = mnc1 * 100 + mnc2 * 10 + mnc3;
    }
    return 0;
}

void read_base_info(base_info_list_t *base_info)
{
    if(NULL == base_info) {
        LOG_D("read cell info while param is NULL");
        return;
    }
    memset(base_info, 0, sizeof(base_info_list_t));
    memcpy(base_info, &g_base_info_list, sizeof(base_info_list_t));
}

void get_base_info(char *plmn, int plmn_len, unsigned short lac, unsigned int cellid)
{
    int index = 0;
    if(BASE_STATION_MAX_NUM == g_base_info_list.base_count) {
        LOG_D("base station info have already great than %d", BASE_STATION_MAX_NUM);
        return;
    }
    index = g_base_info_list.base_count;
    get_mcc_mnc_value(plmn, plmn_len, &(g_base_info_list.base_list[index].mcc), 
            &(g_base_info_list.base_list[index].mnc));
    g_base_info_list.base_list[index].lac = lac;
    g_base_info_list.base_list[index].cellid = cellid % 65536;
    g_base_info_list.base_list[index].signal = 0;
    g_base_info_list.base_count++;
}

void tmr_cmd_handler_get_cell_info(int tmr_id, void *param)
{
    int i = 0, ret = 0, count = 0;
    short signal = 0;
    unsigned char cell_buf[10] = {0};
    unsigned char base_info_64[64];
    NIU_DATA_VALUE *temp_niu_data;
    
    if(0 == is_mqtt_connected()) {
        LOG_D("mqtt is not connected, so do not get cell info");
        return;
    }

    QL_MCM_NW_CELL_INFO_T tempinfo;
    ret = cmd_cell_info_get(&tempinfo, sizeof(QL_MCM_NW_CELL_INFO_T));
    if(0 == ret) {
        LOG_D("get cell info success, update basestation and cell information\n");
        g_base_info_list.base_count = 0;
        
        temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_CSQ);
        signal = *((NUINT8 *)temp_niu_data->addr);

        if(tempinfo.gsm_info_valid) {
            for(i = 0; i < tempinfo.gsm_info_len; i++) {
                if(0 != tempinfo.gsm_info[i].cid) {
                    get_base_info(tempinfo.gsm_info[i].plmn, 3, tempinfo.gsm_info[i].lac, tempinfo.gsm_info[i].cid);
                }
                else {
                    continue;
                }
            }
        }
        if(tempinfo.umts_info_valid) {
            for(i = 0; i < tempinfo.umts_info_len; i++) {
                if(0 != tempinfo.umts_info[i].cid) {
                    get_base_info(tempinfo.umts_info[i].plmn, 3, tempinfo.umts_info[i].lac, tempinfo.umts_info[i].cid);
                }
                else {
                    continue;
                }
            }
        }
        if(tempinfo.lte_info_valid) {
            for(i = 0; i < tempinfo.lte_info_len; i++) {
                if(0 != tempinfo.lte_info[i].cid) {
                    get_base_info(tempinfo.lte_info[i].plmn, 3, tempinfo.lte_info[i].tac, tempinfo.lte_info[i].cid);
                }
                else {
                    continue;
                }
            }
        }

        if(0 != g_base_info_list.base_count) {
            for(i = 0; i < g_base_info_list.base_count; i++) {
                g_base_info_list.base_list[i].signal = signal;
            }
        }

        memset(base_info_64, 0, 64);
        if(g_base_info_list.base_count > 5) {
            count = 5;
        }
        else {
            count = g_base_info_list.base_count;
        }
        
        for(i = 0; i < count; i++) {
            memset(cell_buf, 0, 10);
            memcpy(cell_buf + 0, &(g_base_info_list.base_list[i].mcc), 2);
            memcpy(cell_buf + 2, &(g_base_info_list.base_list[i].mnc), 2);
            memcpy(cell_buf + 4, &(g_base_info_list.base_list[i].lac), 2);
            memcpy(cell_buf + 6, &(g_base_info_list.base_list[i].cellid), 2);
            memcpy(cell_buf + 8, &(g_base_info_list.base_list[i].signal), 2);
            memcpy(base_info_64 + i * 10, cell_buf, 10);

            niu_data_write_value(NIU_ECU, NIU_ID_ECU_NOW_BTS + i, (NUINT8 *)cell_buf, 10);
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_CELL_LAC_1 + i * 3, (NUINT8 *)&(g_base_info_list.base_list[i].lac), 2);
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_CELL_CID_1 + i * 3, (NUINT8 *)&(g_base_info_list.base_list[i].cellid), 2);
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_CELL_RSSI_1 + i * 3, (NUINT8 *)&(g_base_info_list.base_list[i].signal), 2);
        }

        niu_data_write_value(NIU_ECU, NIU_ID_ECU_CELL_MCC, (NUINT8 *)&(g_base_info_list.base_list[0].mcc), 2);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_CELL_MNC, (NUINT8 *)&(g_base_info_list.base_list[0].mnc), 2);
        
        if((E_QL_MCM_NW_RADIO_TECH_EVDO_0 == tempinfo.serving_rat) || 
            (E_QL_MCM_NW_RADIO_TECH_EVDO_A == tempinfo.serving_rat) || 
            (E_QL_MCM_NW_RADIO_TECH_EVDO_B == tempinfo.serving_rat)){
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_CDMA_LOCATION, (NUINT8 *)base_info_64, 64);
            niu_data_clean_value(NIU_ECU, NIU_ID_ECU_ECU_NO_CDMA_LOCATION);
        }
        else {
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_NO_CDMA_LOCATION, (NUINT8 *)base_info_64, 64);
            niu_data_clean_value(NIU_ECU, NIU_ID_ECU_ECU_CDMA_LOCATION);
        }
    }
    else {
        LOG_D("get cell info error, clear basestation and cell information");
        for(i = 0; i < BASE_STATION_MAX_NUM; i++) {
            niu_data_clean_value(NIU_ECU, NIU_ID_ECU_NOW_BTS + i);
            niu_data_clean_value(NIU_ECU, NIU_ID_ECU_CELL_LAC_1 + i * 3);
            niu_data_clean_value(NIU_ECU, NIU_ID_ECU_CELL_CID_1 + i * 3);
            niu_data_clean_value(NIU_ECU, NIU_ID_ECU_CELL_RSSI_1 + i * 3);
        }
        niu_data_clean_value(NIU_ECU, NIU_ID_ECU_CELL_MCC);
        niu_data_clean_value(NIU_ECU, NIU_ID_ECU_CELL_MNC); 

        niu_data_clean_value(NIU_ECU, NIU_ID_ECU_ECU_CDMA_LOCATION);
        niu_data_clean_value(NIU_ECU, NIU_ID_ECU_ECU_NO_CDMA_LOCATION);
    }
}

void printf_lte_info(QL_MCM_NW_LTE_INFO_T lte_info)
{
    printf("lte info is:\n");
    printf("\tcid is %d\n\tplmn is %d-%d-%d\n\ttac is %d\n\tpci is %d\n\tearfcn is %d\n",lte_info.cid, 
            lte_info.plmn[0],lte_info.plmn[1], lte_info.plmn[2],
            lte_info.tac, lte_info.pci, lte_info.earfcn);
}

void printf_umts_info(QL_MCM_NW_UMTS_INFO_T umts_info)
{
    printf("umts info is:\n");
    printf("\tcid is %d\n\tlcid is %d\n\tplmn is %d-%d-%d\n\tlac is %d\n\tpsc is %d\n\tuarfcn is %d\n",umts_info.cid, 
            umts_info.lcid,umts_info.plmn[0],umts_info.plmn[1], umts_info.plmn[2],
            umts_info.lac, umts_info.psc, umts_info.uarfcn);
}

void printf_gsm_info(QL_MCM_NW_GSM_INFO_T gsm_info)
{
    printf("gsm info is:\n");
    printf("\tcid is %d\n\tplmn is %d-%d-%d\n\tlac is %d\n\tbsic is %d\n\tarfcn is %d\n\t",gsm_info.cid, 
            gsm_info.plmn[0],gsm_info.plmn[1], gsm_info.plmn[2],
            gsm_info.lac, gsm_info.bsic, gsm_info.arfcn);
}

void tmr_cmd_handler_mqtt_ping_req(int tmr_id, void *param)
{
    set_mqtt_disconnected();
    sys_event_push(MQTT_EVENT_PING_RESP_FAILED, NULL, 0);
}

void delete_ping_timer()
{
    if(1 == tmr_handler_tmr_is_exist(TIMER_ID_PINGRESP)) {
        LOG_D("delete timer id of ping resp");
        tmr_handler_tmr_del(TIMER_ID_PINGRESP);
    }
}

void mqtt_client_keep_alive(int tmr_id, void *param)
{
    int timeout_time = 0;
    int send_remaining = 0;
    int recv_remaining = 0;
    struct timespec wakeup_time;
    if(1 == is_mqtt_connected()) {
        keepalive(&g_mqttclient);
    }
    else {
        wakeup_time.tv_sec = 99999;
        wakeup_time.tv_nsec = 0;
        low_power_check(LOCK_TYPE_HEARTBEAT, wakeup_time);
        LOG_D("mqtt disconnected, so do not send PINGREQ packet");
        return;
    }
    if(1 == g_mqttclient.ping_outstanding) {
        sys_event_push(MQTT_EVENT_PING_REQ_START, NULL, 0);
    }
    else {
        send_remaining = TimerLeftMS(&(g_mqttclient.last_sent));
        recv_remaining = TimerLeftMS(&(g_mqttclient.last_received));
        timeout_time = send_remaining > recv_remaining ? recv_remaining : send_remaining; 
        if(timeout_time < (TEMPLATE_INTERVAL*1000)) {
            tmr_handler_tmr_add(TIMER_ID_PINGSTART, mqtt_client_keep_alive, NULL, timeout_time, 0);
        }
        else {
            wakeup_time.tv_sec = timeout_time / 1000 + 1;
            wakeup_time.tv_nsec = 0;
            low_power_check(LOCK_TYPE_HEARTBEAT, wakeup_time);
        }
    }
}
