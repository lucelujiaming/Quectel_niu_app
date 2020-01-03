#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <ql_oe.h>
#include "niu_data.h"
#include "utils.h"
#include "service.h"
#include "niu_device.h"
#include "niu_data_pri.h"
#include "MQTTClient.h"
#include "niu_thread.h"
#include "low_power.h"
#include "niu_fota.h"
#include "ql_mutex.h"

extern int get_process_time();
extern MQTTClient g_mqttclient;
extern int g_history_data_count;
extern NUINT32 g_fota_method;
extern history_data_t g_history_data_content[255];
JSTATIC NUINT32 g_niu_car_state_prev = 0;
JSTATIC NUINT32 g_niu_car_trigger_value = 0;
JSTATIC NUINT32 g_niu_car_trigger_value_ex = 0;
//baron.qian-2019/06/20: Requirements change, and this function is implemented on the MCU side
//JSTATIC NUINT32 g_niu_car_acc_off_time = 0;

#define NIU_UPL_BUFFER_LEN (512)
extern int tmr_handler_tmr_template_min_time_remaining(int *remaining_sec, int *remaining_usec);

typedef struct _upl_cmd_info_
{
    NUINT32 timer_id;
    NUINT32 interval;
    NUINT32 sended_count;
    NUINT32 sended_count_max;
    NUINT32 trigger0;
    NUINT32 trigger1;
    NUINT32 trigger2;
    NUINT32 trigger3;
    NUINT32 trigger4;
    NUINT32 trigger5;
    NUINT32 trigger6;
    NUINT32 trigger7;
    NUINT32 trigger8;
    NUINT32 trigger9;
    JBOOL control_gps;
    JBOOL triggered;
    JBOOL trigger_updated;
} UPL_CMD_INFO;

JSTATIC UPL_CMD_INFO g_niu_upl_cmd_info[NIU_SERVER_UPL_CMD_NUM] = {{0}, {0}};

void clear_niu_upl_cmd()
{
    memset(&g_niu_upl_cmd_info, 0, sizeof(g_niu_upl_cmd_info));
}

void clear_upl_cmd_info(int index, int triggerflag)
{
    if((index < 0) || (index >= NIU_SERVER_UPL_CMD_NUM)) {
        LOG_D("clear upl cmd info failure while index is %d", index);
        return;
    }
    tmr_handler_tmr_del(g_niu_upl_cmd_info[index].timer_id);
    g_niu_upl_cmd_info[index].interval = 0;
    g_niu_upl_cmd_info[index].sended_count = 0;
    g_niu_upl_cmd_info[index].sended_count_max = 0;
    if(triggerflag)
    {
        g_niu_upl_cmd_info[index].trigger0 = 0;
        g_niu_upl_cmd_info[index].trigger1 = 0;
        g_niu_upl_cmd_info[index].trigger2 = 0;
        g_niu_upl_cmd_info[index].trigger3 = 0;
        g_niu_upl_cmd_info[index].trigger4 = 0;
        g_niu_upl_cmd_info[index].trigger5 = 0;
        g_niu_upl_cmd_info[index].trigger6 = 0;
        g_niu_upl_cmd_info[index].trigger7 = 0;
        g_niu_upl_cmd_info[index].trigger8 = 0;
        g_niu_upl_cmd_info[index].trigger9 = 0;
        g_niu_upl_cmd_info[index].control_gps = 0;
        g_niu_upl_cmd_info[index].triggered = 0;
        g_niu_upl_cmd_info[index].trigger_updated = 0;
    }
}

int get_overlayable_template_index(int received_template_index)
{
    int i = 0, ret = 0;
    NIU_SERVER_UPL_TEMPLATE *current_template = NULL;

    if((received_template_index >= 0) && (received_template_index < NIU_SERVER_UPL_CMD_NUM)) {
        ret = received_template_index;
    }
    else if(255 == received_template_index) {
        for(i = 0; i < NIU_SERVER_UPL_CMD_NUM; i++) {
            current_template = niu_data_read_template(i);
            if((JFALSE == g_niu_upl_cmd_info[i].triggered) && (0 == current_template->trigged_count)) {
                ret = i;
                break;
            }
        }
        if(i == NIU_SERVER_UPL_CMD_NUM) {
            ret = NIU_SERVER_UPL_CMD_NUM - 1;
        }
    }
    else {
        LOG_D("received template index is %d, throw the template", received_template_index);
        ret = -1;
    }
    return ret;
}

void empyt_template_low_power(int tmr_id, void *param)
{
    struct timespec time_remaining;
    time_remaining.tv_sec = MAX_TIME;
    time_remaining.tv_nsec = 0;
    low_power_check(LOCK_TYPE_TEMPLATE, time_remaining);
}

void check_template_low_power()
{
    int tmr_id, remaining_sec, remaining_usec;
    struct timespec time_remaining;
    if(1 == tmr_handler_tmr_is_exist(TIMER_ID_TEMPLATE_EMPTY_LOW_POWER)) {
        tmr_handler_tmr_del(TIMER_ID_TEMPLATE_EMPTY_LOW_POWER);
    }
    tmr_id = tmr_handler_tmr_template_min_time_remaining(&remaining_sec, &remaining_usec);
    if((tmr_id >= TIMER_ID_TEMPLATE_NIU_TRIGGER_1) && (tmr_id <= TIMER_ID_TEMPLATE_NIU_TRIGGER_50)) {
        if(remaining_sec > TEMPLATE_INTERVAL) {
            time_remaining.tv_sec = remaining_sec - TEMPLATE_INTERVAL;  //juson.zhang-2019/06/12:modify template wakeup system time
            time_remaining.tv_nsec = 0;
            low_power_check(LOCK_TYPE_TEMPLATE, time_remaining);
        }
    }
    else {
        time_remaining.tv_sec = MAX_TIME;
        time_remaining.tv_nsec = 0;
        low_power_check(LOCK_TYPE_TEMPLATE, time_remaining);
    }
}

void upload_realtime_data(int tmr_id, void *param)
{
    int i = 0, triggered_flag = 0;
    unsigned short packed_name = 0;
    unsigned short addr_num = 0;
    char publish_topic[50] = {0};
    unsigned char tempbuf_eid[17] = {0};
    unsigned char tempbuf_aes[17] = {0};
    memset(tempbuf_eid, 0, 17);
    NIU_DATA_VALUE *eid = NULL;
    NIU_DATA_VALUE *aes_key = NULL;
    NIU_DATA_VALUE **data_array = NULL;
    unsigned char *tempdata = NULL;
    MQTTMessage publish_msg;
    

    if(NULL == param) {
        LOG_D("upload realtime tempalte while param is NULL");
        return;
    }

    aes_key = NIU_DATA_ECU(NIU_ID_ECU_AESKEY);
    if(aes_key)
    {
        memcpy(tempbuf_aes, aes_key->addr, aes_key->len);
    }
    if(strlen(tempbuf_aes) == 0)
    {
        printf("get aes key failed, maybe not sync.\n");
        return;
    }
    int template_index = *((int *)param);
    tempdata = (unsigned char *)QL_MEM_ALLOC(MQTT_BUF_LEN);
    if(NULL == tempdata) {
        LOG_D("upload realtime template while QL_MEM_ALLOC failure");
        return;
    }
    memset(tempdata, 0, MQTT_BUF_LEN);
    NIU_SERVER_UPL_TEMPLATE *current_template = niu_data_read_template(template_index);
    if(NULL != current_template) {
        packed_name = current_template->packed_name;
        addr_num = current_template->addr_num;
        data_array = (NIU_DATA_VALUE **)QL_MEM_ALLOC(addr_num * sizeof(NIU_DATA_VALUE*));
        if(NULL != data_array) {
            for(i = 0; i < addr_num; i++) {
                data_array[i] = niu_data_read(current_template->addr_array[i].base, current_template->addr_array[i].id);
            }
        }

        eid = NIU_DATA_ECU(NIU_ID_ECU_EID);
        memcpy(tempbuf_eid, (unsigned char *)eid->addr, eid->len);

        snprintf(publish_topic, 50, "t/%s/%d", tempbuf_eid, packed_name);

        publish_msg.payloadlen = payload_pack_encrypt(data_array, addr_num, packed_name, tempdata, MQTT_BUF_LEN);
        publish_msg.payload = (void *)tempdata;
        publish_msg.qos = QOS0;
        publish_msg.retained = 0;
        publish_msg.dup = 0;
        
        if(1 == g_mqttclient.isconnected) {
            MQTTPublish(&g_mqttclient, publish_topic, &publish_msg);
        }
        else {
            put_data_to_history(publish_msg.payload, publish_msg.payloadlen, packed_name, HISTORY_DATA_TEMPLATE);
        }
        QL_MEM_FREE(data_array);
        data_array = NULL;
        QL_MEM_FREE(tempdata);
        tempdata = NULL;

        g_niu_upl_cmd_info[template_index].trigger_updated = JFALSE;

        if (JFALSE == g_niu_upl_cmd_info[template_index].triggered)
        {
            tmr_handler_tmr_del(g_niu_upl_cmd_info[template_index].timer_id);
            g_niu_upl_cmd_info[template_index].sended_count = 0;
            g_niu_upl_cmd_info[template_index].interval = 0;
            g_niu_upl_cmd_info[template_index].sended_count_max = 0;
        }
        g_niu_upl_cmd_info[template_index].sended_count++;

#if __TEST_TEMPLATE_MODE__        
        g_niu_upl_cmd_info[template_index].interval = 1;
#endif //__TEST_TEMPLATE_MODE__
        if(g_niu_upl_cmd_info[template_index].trigger1 || 
                g_niu_upl_cmd_info[template_index].trigger2 || 
                g_niu_upl_cmd_info[template_index].trigger3 || 
                g_niu_upl_cmd_info[template_index].trigger4 || 
                (g_niu_upl_cmd_info[template_index].trigger7 & NIU_TRIGGER_FOTA_WORKING)) {
            triggered_flag = 1;
        }
        else if((g_niu_upl_cmd_info[template_index].trigger0 & (~NIU_TRIGGER_EVENT_MASK)) || 
                (g_niu_upl_cmd_info[template_index].trigger5 & (~NIU_TRIGGER_EVENT_MASK))) {
            triggered_flag = 1;
        }
        else {
            triggered_flag = 0;
        }
        
        if((1 == triggered_flag) && ((0XFFFF == g_niu_upl_cmd_info[template_index].sended_count_max) || (g_niu_upl_cmd_info[template_index].sended_count < g_niu_upl_cmd_info[template_index].sended_count_max))) {
            tmr_handler_tmr_add(g_niu_upl_cmd_info[template_index].timer_id, upload_realtime_data, param, g_niu_upl_cmd_info[template_index].interval * 1000, 0);
            LOG_D("TRIGGER_INFO--index:%d, triggered:%d----CONTINUE", template_index, g_niu_upl_cmd_info[template_index].triggered);
        }
        else {
            QL_MEM_FREE(param);            
            param = NULL;
            tmr_handler_tmr_del(g_niu_upl_cmd_info[template_index].timer_id);
            LOG_D("TRIGGER_INFO--index:%d, triggered:%d----OVER", template_index, g_niu_upl_cmd_info[template_index].triggered);
        }
    }
    else {
        LOG_D("upload template while get template info failed");
    }
    check_template_low_power();
}

void niu_data_ecu_state_init()
{
    LOG_D("niu_data_ecu_state_init");
    NUINT32 ecu_car_sta_value = 0;
   // NUINT32 trigger_value = 0, trigger_value_ex = 0;
    NIU_DATA_VALUE *ecu_state = NIU_DATA_ECU(NIU_ID_ECU_ECU_STATE);
    NIU_DATA_VALUE *ecu_car_sta = NIU_DATA_ECU(NIU_ID_ECU_CAR_STA);
    if(ecu_car_sta != NULL)
    {
        memcpy(&ecu_car_sta_value, ecu_car_sta->addr, sizeof(NUINT32));
    }
    g_niu_upl_cmd_info[0].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_1;
    g_niu_upl_cmd_info[1].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_2;
    g_niu_upl_cmd_info[2].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_3;
    g_niu_upl_cmd_info[3].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_4;
    g_niu_upl_cmd_info[4].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_5;
    g_niu_upl_cmd_info[5].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_6;
    g_niu_upl_cmd_info[6].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_7;
    g_niu_upl_cmd_info[7].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_8;
    g_niu_upl_cmd_info[8].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_9;
    g_niu_upl_cmd_info[9].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_10;
    g_niu_upl_cmd_info[10].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_11;
    g_niu_upl_cmd_info[11].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_12;
    g_niu_upl_cmd_info[12].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_13;
    g_niu_upl_cmd_info[13].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_14;
    g_niu_upl_cmd_info[14].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_15;
    g_niu_upl_cmd_info[15].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_16;
    g_niu_upl_cmd_info[16].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_17;
    g_niu_upl_cmd_info[17].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_18;
    g_niu_upl_cmd_info[18].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_19;
    g_niu_upl_cmd_info[19].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_20;
    g_niu_upl_cmd_info[20].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_21;
    g_niu_upl_cmd_info[21].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_22;
    g_niu_upl_cmd_info[22].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_23;
    g_niu_upl_cmd_info[23].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_24;
    g_niu_upl_cmd_info[24].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_25;
    g_niu_upl_cmd_info[25].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_26;
    g_niu_upl_cmd_info[26].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_27;
    g_niu_upl_cmd_info[27].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_28;
    g_niu_upl_cmd_info[28].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_29;
    g_niu_upl_cmd_info[29].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_30;
    g_niu_upl_cmd_info[30].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_31;
    g_niu_upl_cmd_info[31].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_32;
    g_niu_upl_cmd_info[32].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_33;
    g_niu_upl_cmd_info[33].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_34;
    g_niu_upl_cmd_info[34].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_35;
    g_niu_upl_cmd_info[35].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_36;
    g_niu_upl_cmd_info[36].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_37;
    g_niu_upl_cmd_info[37].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_38;
    g_niu_upl_cmd_info[38].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_39;
    g_niu_upl_cmd_info[39].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_40;
    g_niu_upl_cmd_info[40].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_41;
    g_niu_upl_cmd_info[41].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_42;
    g_niu_upl_cmd_info[42].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_43;
    g_niu_upl_cmd_info[43].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_44;
    g_niu_upl_cmd_info[44].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_45;
    g_niu_upl_cmd_info[45].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_46;
    g_niu_upl_cmd_info[46].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_47;
    g_niu_upl_cmd_info[47].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_48;
    g_niu_upl_cmd_info[48].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_49;
    g_niu_upl_cmd_info[49].timer_id = TIMER_ID_TEMPLATE_NIU_TRIGGER_50;

    if (ecu_car_sta_value & NIU_STATE_ACC)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_ACC_ON;
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_ACC_Y;
    }
    else
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_ACC_OFF;
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_ACC_N;
    }

    if (ecu_car_sta_value & NIU_STATE_RUNNING)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_RUNNING_Y;
    }
    else
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_RUNNING_N;
    }

    if (ecu_car_sta_value & NIU_STATE_BATTERY)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_BATTER_IN;
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_BATTERY_Y;
    }
    else
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_BATTER_OUT;
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_BATTERY_N;
    }

    if (ecu_car_sta_value & NIU_STATE_SLAVE)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_SLAVE_Y;
    }
    else
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_SLAVE_N;
    }

    if (ecu_car_sta_value & NIU_STATE_SHAKE)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_SHAKE;
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_SHAKE_Y;
    }
    else
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_SHAKE_N;
    }

    if (ecu_car_sta_value & NIU_STATE_MOVE)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_MOVE;
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_MOVE_Y;
    }
    else
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_MOVE_N;
    }

    if (ecu_car_sta_value & NIU_STATE_FALL)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_FALL;
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_FALL_Y;
    }
    else
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_FALL_N;
    }

    if (ecu_car_sta_value & NIU_STATE_MOVE_EX)
    {
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_EVENT_MOVE_GSENSOR;
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_STATE_MOVE_GSENSOR_Y;
    }
    else
    {
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_STATE_MOVE_GSENSOR_N;
    }

    if(ecu_car_sta_value & NIU_STATE_BMS2_ERR)
    {
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_STATE_BMS2_DISCONNECT;
    }

    if(ecu_car_sta_value & NIU_STATE_BCS_ERR)
    {
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_STATE_BCS_DISCONNECT;
    }

    if (ecu_car_sta_value & NIU_STATE_INCHARGE)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_INCHARGE;
    }
    else
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_DISCHARGE;
    }

    if (ecu_car_sta_value & NIU_STATE_GPS_ON)
    {
        if (ecu_car_sta_value & NIU_STATE_GPS_LOCATION)
        {
            g_niu_car_trigger_value |= NIU_TRIGGER_STATE_GPS_Y;
            g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_GPS_N);
        }
        else
        {
            g_niu_car_trigger_value |= NIU_TRIGGER_STATE_GPS_N;
            g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_GPS_Y);
        }
    }
    else
    {
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_GPS_N);
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_GPS_Y);
    }
    //FOC ERROR
    if(ecu_car_sta_value & NIU_STATE_FOC_ERR)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_FOC_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value &= ~NIU_TRIGGER_STATE_FOC_DISCONNECT;
    }
    //BMS ERROR
    if(ecu_car_sta_value & NIU_STATE_BMS_ERR)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_BMS_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value &= ~NIU_TRIGGER_STATE_BMS_DISCONNECT;
    }
    //DB ERROR
    if(ecu_car_sta_value & NIU_STATE_DB_ERR)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_DB_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value &= ~NIU_TRIGGER_STATE_DB_DISCONNECT;
    }
    //LCU ERROR
    if(ecu_car_sta_value & NIU_STATE_LCU_ERR)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_LCU_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value &= ~NIU_TRIGGER_STATE_LCU_DISCONNECT;
    }
    //LOG_D("trigger_value:%x updated\n", trigger_value);

    if (ecu_state)
    {
        NUINT32 ecu_state_prv = 0;

        memcpy(&ecu_state_prv, ecu_state->addr, jmin(ecu_state->len, sizeof(ecu_state_prv)));

        ecu_state_prv = (ecu_state_prv & (~NIU_TRIGGER_STATE_MASK)) | g_niu_car_trigger_value;

        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_STATE, (NUINT8 *)&ecu_state_prv, sizeof(NUINT32));
    }
    //baron.qian-2019/06/20: Requirements change, and this function is implemented on the MCU side
    //niu_acc_off_time_update();
    return;
}

JSTATIC JBOOL g_niu_shake_enable = JTRUE;

JVOID niu_shake_enable(int tmr_id, void *param)
{
    if(tmr_id == TIMER_ID_NIU_ENABLE_SHAKE)
    {
        LOG_D("g_niu_shake_enable:%d", g_niu_shake_enable);
        g_niu_shake_enable = JTRUE;
    }
}



JVOID niu_car_state_set(NIU_CAR_STATE state, JBOOL value)
{
    NUINT32 niu_car_state = 0;
    NIU_DATA_VALUE *car_stae_value = NIU_DATA_ECU(NIU_ID_ECU_CAR_STA);

    if(car_stae_value == NULL)
    {
        LOG_E("get niu_data_ecu[car_sta] error.");
        return;
    }
    memcpy(&niu_car_state, car_stae_value->addr, sizeof(NUINT32));

    JBOOL state_changed = (value != ((niu_car_state & state) ? JTRUE : JFALSE));

    //LOG_D("g_niu_car_state:%x, state:%x, value:%d, changed:%d", niu_car_state, state, value, state_changed);

    if (state == NIU_STATE_ACC && value == JFALSE)
    {
        g_niu_shake_enable = JFALSE;
        tmr_handler_tmr_add(TIMER_ID_NIU_ENABLE_SHAKE, niu_shake_enable, NULL, 2 * 60 * 1000, 0);
    }
    else if (state == NIU_STATE_ACC && value == JTRUE)
    {
        g_niu_shake_enable = JFALSE;
        tmr_handler_tmr_del(TIMER_ID_NIU_ENABLE_SHAKE);
    }

    if ((state == NIU_STATE_SHAKE || state == NIU_STATE_MOVE || state == NIU_STATE_MOVE_EX /*|| state == NIU_STATE_FALL*/) && 
                value == JTRUE && ((niu_car_state & NIU_STATE_RUNNING) || JFALSE == g_niu_shake_enable))
    {
        LOG_D("Running unable shake move shake");
        return;
    }
    /* maybe want to setting shake value in niu_data , just maybe */
    // if ((state == NIU_STATE_SHAKE || state == NIU_STATE_MOVE_EX) && value == JTRUE)
    // {
    //     JUINT8 shake_value = 3;
    //     NIU_DATA_VALUE *niu_shake_value = NIU_DATA_ECU(NIU_ID_ECU_SHAKING_VALUE);

    //     if (niu_shake_value)
    //     {
    //         memcpy(&shake_value, niu_shake_value->addr, 1);
    //     }

    //     if (!niu_shake_value)
    //     {
    //         return;
    //     }
    // }

    niu_car_state = (value) ? (niu_car_state | (state)) : (niu_car_state & (~state));

    if (state_changed)
    {
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_CAR_STA, (NUINT8 *)&niu_car_state, sizeof(niu_car_state));
        niu_update_trigger_template();
    }
}

JBOOL niu_car_state_get(NIU_CAR_STATE state)
{
    JBOOL ret = JFALSE;
    NUINT32 car_sta;
    NIU_DATA_VALUE *value = NIU_DATA_ECU(NIU_ID_ECU_CAR_STA);

    if(value == NULL)
    {
        LOG_E("get NIU_DATA_ECU[NIU_ID_ECU_CAR_STA] failed.\n");
        return FALSE;
    }
    memcpy(&car_sta, value->addr, sizeof(NUINT32));
    ret = (car_sta & state) ? JTRUE : JFALSE;
    //LOG_D("car_sta:0x%x, state:0x%x, ret:%d", car_sta, state, ret);

    return ret;
}

int niu_data_ecu_state_update()
{
    NUINT32 car_sta_value = 0;
    NIU_DATA_VALUE *ecu_state = NIU_DATA_ECU(NIU_ID_ECU_ECU_STATE);
    NIU_DATA_VALUE *ecu_state2 = NIU_DATA_ECU(NIU_ID_ECU_ECU_STATE2);
    NIU_DATA_VALUE *ecu_car_sta = NIU_DATA_ECU(NIU_ID_ECU_CAR_STA);

    if(ecu_car_sta == NULL)
    {
        LOG_E("read data ECU:car_sta error.");
        return -1;
    }
    memcpy(&car_sta_value, ecu_car_sta->addr, sizeof(NUINT32));

    if (0 != (car_sta_value & NIU_STATE_ACC) && 0 == (g_niu_car_state_prev & NIU_STATE_ACC))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_ACC_ON;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_ACC_OFF);

        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_ACC_Y;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_ACC_N);
    }
    else if (0 == (car_sta_value & NIU_STATE_ACC) && 0 != (g_niu_car_state_prev & NIU_STATE_ACC))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_ACC_OFF;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_ACC_ON);

        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_ACC_N;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_ACC_Y);
    }

    if (0 != (car_sta_value & NIU_STATE_RUNNING) && 0 == (g_niu_car_state_prev & NIU_STATE_RUNNING))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_RUNNING_Y;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_RUNNING_N);
    }
    else if (0 == (car_sta_value & NIU_STATE_RUNNING) && 0 != (g_niu_car_state_prev & NIU_STATE_RUNNING))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_RUNNING_N;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_RUNNING_Y);
    }

    if (0 != (car_sta_value & NIU_STATE_BATTERY) && 0 == (g_niu_car_state_prev & NIU_STATE_BATTERY))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_BATTER_IN;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_BATTER_OUT);

        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_BATTERY_Y;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_BATTERY_N);
        printf("%s\n", __FUNCTION__);
        g_fota_method = FOTA_CHECK_METHOD_AUTO;
        wakeup_fota_thread(FOTA_CHECK);
    }
    else if (0 == (car_sta_value & NIU_STATE_BATTERY) && 0 != (g_niu_car_state_prev & NIU_STATE_BATTERY))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_BATTER_OUT;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_BATTER_IN);

        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_BATTERY_N;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_BATTERY_Y);
    }

    if (0 != (car_sta_value & NIU_STATE_SLAVE) && 0 == (g_niu_car_state_prev & NIU_STATE_SLAVE))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_SLAVE_Y;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_SLAVE_N);

    }
    else if (0 == (car_sta_value & NIU_STATE_SLAVE) && 0 != (g_niu_car_state_prev & NIU_STATE_SLAVE))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_SLAVE_N;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_SLAVE_Y);
    }

    if(0 != (car_sta_value & NIU_STATE_INCHARGE) && 0 == (g_niu_car_state_prev & NIU_STATE_INCHARGE))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_INCHARGE;
        g_niu_car_trigger_value &= ~NIU_TRIGGER_STATE_DISCHARGE;
    }
    else if (0 == (car_sta_value & NIU_STATE_INCHARGE) && 0 != (g_niu_car_state_prev & NIU_STATE_INCHARGE))
    {
        g_niu_car_trigger_value &= ~NIU_TRIGGER_STATE_INCHARGE;
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_DISCHARGE;
    }

    if (0 != (car_sta_value & NIU_STATE_SHAKE) && 0 == (g_niu_car_state_prev & NIU_STATE_SHAKE))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_SHAKE;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_SHAKE_STOP);

        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_SHAKE_Y;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_SHAKE_N);
    }
    else if (0 == (car_sta_value & NIU_STATE_SHAKE) && 0 != (g_niu_car_state_prev & NIU_STATE_SHAKE))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_SHAKE_STOP;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_SHAKE);
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_SHAKE_N;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_SHAKE_Y);
    }

    if (0 != (car_sta_value & NIU_STATE_MOVE) && 0 == (g_niu_car_state_prev & NIU_STATE_MOVE))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_MOVE;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_MOVE_STOP);
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_MOVE_Y;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_MOVE_N);
    }
    else if (0 == (car_sta_value & NIU_STATE_MOVE) && 0 != (g_niu_car_state_prev & NIU_STATE_MOVE))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_MOVE_STOP;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_MOVE);
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_MOVE_N;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_MOVE_Y);
    }

    if (0 != (car_sta_value & NIU_STATE_FALL) && 0 == (g_niu_car_state_prev & NIU_STATE_FALL))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_FALL;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_FALL_STOP);
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_FALL_Y;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_FALL_N);
    }
    else if (0 == (car_sta_value & NIU_STATE_FALL) && 0 != (g_niu_car_state_prev & NIU_STATE_FALL))
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_EVENT_FALL_STOP;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_EVENT_FALL);
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_FALL_N;
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_FALL_Y);
    }

    if (car_sta_value & NIU_STATE_GPS_ON)
    {
        if (car_sta_value & NIU_STATE_GPS_LOCATION)
        {
            g_niu_car_trigger_value |= NIU_TRIGGER_STATE_GPS_Y;
            g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_GPS_N);
        }
        else
        {
            g_niu_car_trigger_value |= NIU_TRIGGER_STATE_GPS_N;
            g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_GPS_Y);
        }
    }
    else
    {
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_GPS_N);
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_GPS_Y);
    }

    if (car_sta_value & NIU_STATE_FOC_ERR)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_FOC_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_FOC_DISCONNECT);
    }

    if (car_sta_value & NIU_STATE_BMS_ERR)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_BMS_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_BMS_DISCONNECT);
    }

    if (car_sta_value & NIU_STATE_DB_ERR)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_DB_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_DB_DISCONNECT);
    }

    if (car_sta_value & NIU_STATE_LCU_ERR)
    {
        g_niu_car_trigger_value |= NIU_TRIGGER_STATE_LCU_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value &= (~NIU_TRIGGER_STATE_LCU_DISCONNECT);
    }


    //set by stm32
    if ((car_sta_value & NIU_STATE_MOVE_EX))
    {
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_EVENT_MOVE_GSENSOR;
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_STATE_MOVE_GSENSOR_Y;
        g_niu_car_trigger_value_ex &= (~NIU_TRIGGER_STATE_MOVE_GSENSOR_N);
        g_niu_car_trigger_value_ex &= (~NIU_TRIGGER_EVENT_MOVE_GSENSOR_STOP);
    }
    else
    {
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_STATE_MOVE_GSENSOR_N;
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_EVENT_MOVE_GSENSOR_STOP;
        g_niu_car_trigger_value_ex &= (~NIU_TRIGGER_EVENT_MOVE_GSENSOR);
        g_niu_car_trigger_value_ex &= (~NIU_TRIGGER_STATE_MOVE_GSENSOR_Y);
    }

    if(car_sta_value & NIU_STATE_BMS2_ERR)
    {
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_STATE_BMS2_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value_ex &= ~NIU_TRIGGER_STATE_BMS2_DISCONNECT;
    }

    if(car_sta_value & NIU_STATE_BCS_ERR)
    {
        g_niu_car_trigger_value_ex |= NIU_TRIGGER_STATE_BCS_DISCONNECT;
    }
    else
    {
        g_niu_car_trigger_value_ex &= ~NIU_TRIGGER_STATE_BCS_DISCONNECT;
    }


    g_niu_car_state_prev = car_sta_value;


    if (ecu_state)
    {
        NUINT32 ecu_state_prv = 0;
        NUINT32 ecu_state_new = 0;

        memcpy(&ecu_state_prv, ecu_state->addr, jmin(ecu_state->len, sizeof(ecu_state_prv)));
        
        ecu_state_new = g_niu_car_trigger_value;
        if (ecu_state_new != ecu_state_prv)
        {
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_STATE, (NUINT8 *)&ecu_state_new, sizeof(NUINT32));
        }
    }

    if(ecu_state2)
    {
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_STATE2, (NUINT8 *)&g_niu_car_trigger_value_ex, sizeof(NUINT32));
    }
    return 0;
}

JSTATIC JVOID niu_template_trigger(NUINT32 index, JBOOL trigger)
{
    if (index < NIU_SERVER_UPL_CMD_NUM)
    {
        if (trigger)
        {
            NIU_SERVER_UPL_TEMPLATE *templ = niu_data_read_template(index);
            NUINT32 interval = templ->interval;
            
            
            tmr_handler_tmr_del(g_niu_upl_cmd_info[index].timer_id);
            g_niu_upl_cmd_info[index].sended_count = 0;
            g_niu_upl_cmd_info[index].interval = 0;
            g_niu_upl_cmd_info[index].sended_count_max = 0;
            
            if (templ->trigged_count > 0)
            {
                if (templ->cur_time && templ->cur_time <= templ->interval)
                {
                    interval -= templ->cur_time;
                }
                g_niu_upl_cmd_info[index].sended_count_max = templ->send_max;
                g_niu_upl_cmd_info[index].interval = templ->interval;
                
                if(0 == tmr_handler_tmr_is_exist(g_niu_upl_cmd_info[index].timer_id))
                {
                    NUINT32 *teml_index = (NUINT32 *)QL_MEM_ALLOC(sizeof(NUINT32));
                    if(teml_index != NULL)
                    {
                        *teml_index = index;
                        tmr_handler_tmr_add(g_niu_upl_cmd_info[index].timer_id, upload_realtime_data, (void*)teml_index, ((interval) ? interval : 1) * 1000, 0);
                    }
                }

                if (0xFFFF != templ->trigged_count && 0 < templ->trigged_count)
                {
                    templ->trigged_count--;
                }
                g_niu_upl_cmd_info[index].triggered = JTRUE;
                
            }
            else
            {
                NIU_SERVER_UPL_TEMPLATE empty_templ = {0};
                empty_templ.index = index;
                niu_data_write_template(&empty_templ, index);
            }
        }
        else
        {
            //if (JFALSE == g_niu_upl_cmd_info[index].trigger_updated)
            {
                tmr_handler_tmr_del(g_niu_upl_cmd_info[index].timer_id);
                g_niu_upl_cmd_info[index].sended_count = 0;
                g_niu_upl_cmd_info[index].interval = 0;
                g_niu_upl_cmd_info[index].sended_count_max = 0;
            }
            g_niu_upl_cmd_info[index].triggered = JFALSE;
        }
    }
}
//baron.qian-2019/06/20: Requirements change, and this function is implemented on the MCU side
// void niu_acc_off_time_update()
// {
//     g_niu_car_acc_off_time = niu_device_get_timestamp();
// }


JSTATIC JBOOL niu_template_is_trggered(NIU_SERVER_UPL_TEMPLATE *templ, JBOOL *trigger_changed)
{
    JBOOL ret = JFALSE;
    NUINT16 bms_bat_sta_rt = 0;
    NUINT16 foc_cotlsta = 0;
    NUINT32 ecu_state = 0;
    NUINT32 ecu_state_ex = 0;
    NUINT8 alarm_state = 0;
    NUINT16 ecu_cfg = 0;
    NUINT32 ecu_car_sta = 0;
    NIU_DATA_VALUE *niu_data_bms_bat_sta_rt = NULL;
    NIU_DATA_VALUE *niu_data_foc_cotlsta = NULL;
    NIU_DATA_VALUE *niu_data_ecu_state = NULL;
    NIU_DATA_VALUE *niu_data_alarm_state = NULL;
    NIU_DATA_VALUE *niu_data_ecu_cfg = NULL;
    NIU_DATA_VALUE *niu_data_ecu_state_ex = NULL;
    NIU_DATA_VALUE *niu_data_ecu_car_sta = NULL;

    niu_data_bms_bat_sta_rt = NIU_DATA_BMS(NIU_ID_BMS_BMS_BAT_STA_RT);
    if (niu_data_bms_bat_sta_rt && niu_data_bms_bat_sta_rt->len == sizeof(NUINT16))
    {
        memcpy(&bms_bat_sta_rt, niu_data_bms_bat_sta_rt->addr, sizeof(NUINT16));
    }

    niu_data_foc_cotlsta = NIU_DATA_FOC(NIU_ID_FOC_FOC_COTLSTA);
    if (niu_data_foc_cotlsta && niu_data_foc_cotlsta->len == sizeof(NUINT16))
    {
        memcpy(&foc_cotlsta, niu_data_foc_cotlsta->addr, sizeof(NUINT16));
    }

    niu_data_ecu_state = NIU_DATA_ECU(NIU_ID_ECU_ECU_STATE);
    if (niu_data_ecu_state && niu_data_ecu_state->len == sizeof(NUINT32))
    {
        memcpy(&ecu_state, niu_data_ecu_state->addr, sizeof(NUINT32));
    }


    niu_data_alarm_state = NIU_DATA_ALARM(NIU_ID_ALARM_ALARM_STATE);
    if (niu_data_alarm_state && niu_data_alarm_state->len == sizeof(NUINT8))
    {
        memcpy(&alarm_state, niu_data_alarm_state->addr, sizeof(NUINT8));
    }

    niu_data_ecu_cfg = NIU_DATA_ECU(NIU_ID_ECU_ECU_CFG);
    if (niu_data_ecu_cfg && niu_data_ecu_cfg->len == sizeof(NUINT16))
    {
        memcpy(&ecu_cfg, niu_data_ecu_cfg->addr, sizeof(NUINT16));
    }

    niu_data_ecu_state_ex = NIU_DATA_ECU(NIU_ID_ECU_ECU_STATE2);
    if (niu_data_ecu_state_ex && niu_data_ecu_state_ex->len == sizeof(NUINT32))
    {
        memcpy(&ecu_state_ex, niu_data_ecu_state_ex->addr, sizeof(NUINT32));
    }
    niu_data_ecu_car_sta = NIU_DATA_ECU(NIU_ID_ECU_CAR_STA);
    if(niu_data_ecu_car_sta && niu_data_ecu_car_sta->len == sizeof(NUINT32))
    {
        memcpy(&ecu_car_sta, niu_data_ecu_car_sta->addr, sizeof(NUINT32));
    }
    //baron.qian-2019/05/13: not reporting shake detection template within two minutes of acc off.
    //baron.qian-2019/06/20: Requirements change, and this function is implemented on the MCU side
    // if(niu_device_get_timestamp() - g_niu_car_acc_off_time < 2 * 60  && niu_data_get_acc_status() == 0)
    // {
    //     ecu_state |= NIU_TRIGGER_EVENT_SHAKE_STOP;
    //     ecu_state &= (~NIU_TRIGGER_EVENT_SHAKE);

    //     alarm_state &= (~NIU_TRIGGER_ALARM_ALERT);
    //     alarm_state &= (~NIU_TRIGGER_ALARM_LOCK);

    //     ecu_state_ex |= NIU_TRIGGER_EVENT_MOVE_GSENSOR_STOP;
    //     ecu_state_ex &= (~NIU_TRIGGER_EVENT_MOVE_GSENSOR);
    // }
    //baron.qian-2019/06/20: end
    if (templ && templ->addr_num)
    {
        NUINT32 trigger0_v = 0;
        NUINT32 trigger1_v = 0;
        NUINT32 trigger2_v = 0;
        NUINT32 trigger3_v = 0;
        NUINT32 trigger4_v = 0;
        NUINT32 trigger5_v = 0;
        NUINT32 trigger7_v = 0;

        //LOG_D("ecu_state:%x, ecu_car_sta:%x\n", ecu_state, ecu_car_sta);

        trigger0_v = (templ->trigger0 & ecu_state);
        trigger1_v = (templ->trigger1 & bms_bat_sta_rt);
        trigger2_v = (templ->trigger2 & foc_cotlsta);
        trigger3_v = (((templ->trigger3 & ecu_state) == templ->trigger3) && templ->trigger3) ? templ->trigger3 : 0;
        trigger4_v = (alarm_state & templ->trigger4);
        trigger5_v = templ->trigger5 & ecu_state_ex;
        trigger7_v = templ->trigger7 & (((ecu_cfg & 0x10) ? NIU_TRIGGER_FOTA_READY : 0) | 
                                            ((ecu_cfg & 0x01) ? NIU_TRIGGER_OTA_READY : 0) | 
                                            ((ecu_state & NIU_TRIGGER_STATE_BATTERY_Y) ? NIU_TRIGGER_BATTERY_IN : 0));

        if (NIU_STATE_FOTA & ecu_car_sta)
        {
            trigger7_v |= NIU_TRIGGER_FOTA_WORKING;
        }
        else
        {
            trigger7_v &= (~NIU_TRIGGER_FOTA_WORKING);
        }
        trigger7_v &= templ->trigger7;

        // LOG_D("index:%d,trigger0:0x%x,1:0x%x,2:0x%x,3:0x%x,4:0x%x,7:0x%x", 
        //                                             templ->index, templ->trigger0, templ->trigger1, templ->trigger2, 
        //                                             templ->trigger3, templ->trigger4, templ->trigger7);
        // LOG_D("Curr index:%d,trigger0_v:0x%x,1_v:0x%x,2_v:0x%x,3_v:0x%x,4_v:0x%x,7_v:0x%x", 
        //                                             templ->index, trigger0_v, trigger1_v, trigger2_v, 
        //                                             trigger3_v, trigger4_v, trigger7_v);
        // LOG_D("Prev index:%d,trigger0_v:0x%x,1_v:0x%x,2_v:0x%x,3_v:0x%x,4_v:0x%x,7_v:0x%x", templ->index,
        //                                             g_niu_upl_cmd_info[templ->index].trigger0,
        //                                             g_niu_upl_cmd_info[templ->index].trigger1,
        //                                             g_niu_upl_cmd_info[templ->index].trigger2,
        //                                             g_niu_upl_cmd_info[templ->index].trigger3,
        //                                             g_niu_upl_cmd_info[templ->index].trigger4,
        //                                             g_niu_upl_cmd_info[templ->index].trigger7);

        ret = (trigger0_v || trigger1_v || trigger2_v || trigger3_v || (trigger4_v != g_niu_upl_cmd_info[templ->index].trigger4) || trigger7_v || (trigger7_v != (g_niu_upl_cmd_info[templ->index].trigger7)) || trigger5_v);
        /*baron.qian-2019/05/15 start: add these print to debug not reporting shake detection template within two minutes of acc off */
        #if 0
        if(templ->index == 7)
        {
            printf("Curr ret: %d, index:%d,trigger0_v:0x%x, 1_v:0x%x, 2_v:0x%x, 3_v:0x%x, 4_v:0x%x, 5_v:0x%x, 7_v:0x%x\n", 
                                                ret, templ->index, trigger0_v, trigger1_v, trigger2_v, 
                                                trigger3_v, trigger4_v, trigger5_v, trigger7_v);
            printf("Prev ret: %d, index:%d,trigger0_v:0x%x, 1_v:0x%x, 2_v:0x%x, 3_v:0x%x, 4_v:0x%x, 5_v:0x%x, 7_v:0x%x\n", ret, templ->index,
                                                    g_niu_upl_cmd_info[templ->index].trigger0,
                                                    g_niu_upl_cmd_info[templ->index].trigger1,
                                                    g_niu_upl_cmd_info[templ->index].trigger2,
                                                    g_niu_upl_cmd_info[templ->index].trigger3,
                                                    g_niu_upl_cmd_info[templ->index].trigger4,
                                                    g_niu_upl_cmd_info[templ->index].trigger5,
                                                    g_niu_upl_cmd_info[templ->index].trigger7);
        }
        /*baron.qian-2019/05/15 end:*/
        #endif
        if (ret &&
            ((trigger0_v & (~g_niu_upl_cmd_info[templ->index].trigger0)) || 
            (trigger1_v & (~g_niu_upl_cmd_info[templ->index].trigger1)) || 
            (trigger2_v & (~g_niu_upl_cmd_info[templ->index].trigger2)) || 
            (trigger3_v & (~g_niu_upl_cmd_info[templ->index].trigger3)) || 
            (trigger4_v & (~g_niu_upl_cmd_info[templ->index].trigger4)) || 
            (trigger4_v != g_niu_upl_cmd_info[templ->index].trigger4) || 
            (trigger5_v & (~g_niu_upl_cmd_info[templ->index].trigger5)) || 
            (trigger7_v != (g_niu_upl_cmd_info[templ->index].trigger7))))
        {
            *trigger_changed = JTRUE;
            g_niu_upl_cmd_info[templ->index].trigger_updated = JTRUE;
        }

        g_niu_upl_cmd_info[templ->index].trigger0 = trigger0_v;
        g_niu_upl_cmd_info[templ->index].trigger1 = trigger1_v;
        g_niu_upl_cmd_info[templ->index].trigger2 = trigger2_v;
        g_niu_upl_cmd_info[templ->index].trigger3 = trigger3_v;
        g_niu_upl_cmd_info[templ->index].trigger4 = trigger4_v;
        g_niu_upl_cmd_info[templ->index].trigger5 = trigger5_v;
        g_niu_upl_cmd_info[templ->index].trigger7 = trigger7_v;
    }

    //LOG_D("index:%d, ret:%d, trigger_changed:%d", templ->index, ret, *trigger_changed);
    return ret;
}

JVOID niu_update_trigger_template()
{
    NUINT32 index = 0;

    //LOG_D("niu_update_trigger_template");

    //for debug
    niu_data_ecu_state_update();

    for (index = 0; index < NIU_SERVER_UPL_CMD_NUM; index++)
    {
        NIU_SERVER_UPL_TEMPLATE *templ = NULL;
        JBOOL triggered = JFALSE;
        //JBOOL use_gps = JFALSE;
        JBOOL trigger_changed = JFALSE;
        //NUINT32 j = 0;

        g_niu_upl_cmd_info[index].control_gps = JFALSE;
        templ = niu_data_read_template(index);
        triggered = niu_template_is_trggered(templ, &trigger_changed);

        
        if (JTRUE == triggered)
        {
            if (trigger_changed || g_niu_upl_cmd_info[index].triggered == JFALSE)
            {
                g_niu_upl_cmd_info[index].triggered = JFALSE;
                niu_template_trigger(index, JTRUE);
                check_template_low_power(); //juson.zhang-2019/07/09:When template update, check lowpower
            }

            // LOG_D("index:%d, trigger0:%d, trigger1:%d,trigger2:%d,trigger3:%d,trigger4:%d",
            //             index,
            //             g_niu_upl_cmd_info[index].trigger0,
            //             g_niu_upl_cmd_info[index].trigger1,
            //             g_niu_upl_cmd_info[index].trigger2,
            //             g_niu_upl_cmd_info[index].trigger3,
            //             g_niu_upl_cmd_info[index].trigger4);
        }
        else
        {
            //StopTimer(g_niu_upl_cmd_info[index].timer_id);
            // g_niu_upl_cmd_info[index].triggered = JFALSE;
            niu_template_trigger(index, JFALSE);
        }
        // LOG_D("index:%d,IsMyTimerExist:%d-%d", 
        //                     index, g_niu_upl_cmd_info[index].timer_id, 
        //                     tmr_handler_tmr_is_exist(g_niu_upl_cmd_info[index].timer_id));
    }

    //LOG_D("niu_update_trigger_template quit, %d exist? %d", TIMER_ID_NIU_UPDATE_TEMPLATE, tmr_handler_tmr_is_exist(TIMER_ID_NIU_UPDATE_TEMPLATE));
}
