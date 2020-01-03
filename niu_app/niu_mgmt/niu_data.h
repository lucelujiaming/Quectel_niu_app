/********************************************************************
 * @File name: niu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: niu data define and operation
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef NIU_DATA_H_
#define NIU_DATA_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "niu_types.h"
#include "niu_data_pri.h"


#define NIU_MASK_SYNC_FLAG_SERVER_URL               0x01
#define NIU_MASK_SYNC_FLAG_SERVER_PORT              0x02
#define NIU_MASK_SYNC_FLAG_SERIAL_NUMBER            0x04
#define NIU_MASK_SYNC_FLAG_ECU_EID                  0x08
#define NIU_MASK_SYNC_FLAG_BLE_AES                  0x10
#define NIU_MASK_SYNC_FLAG_BLE_MAC                  0x20
#define NIU_MASK_SYNC_FLAG_BLE_NAME                 0x40
#define NIU_MASK_SYNC_FLAG_BLE_PASSWORD             0x80
#define NIU_MASK_SYNC_FLAG_SERVER_URL2              0x100
#define NIU_MASK_SYNC_FLAG_SERVER_PORT2             0x200
#define NIU_MASK_SYNC_FLAG_SERVER_ERROR_URL         0x400
#define NIU_MASK_SYNC_FLAG_SERVER_ERROR_PORT        0x800
#define NIU_MASK_SYNC_FLAG_SERVER_ERROR_URL2        0x1000
#define NIU_MASK_SYNC_FLAG_SERVER_ERROR_PORT2       0x2000
#define NIU_MASK_SYNC_FLAG_SERVER_LBS_URL           0x4000
#define NIU_MASK_SYNC_FLAG_SERVER_LBS_PORT          0x8000
#define NIU_MASK_SYNC_FLAG_SERVER_LBS_URL2          0x10000
#define NIU_MASK_SYNC_FLAG_SERVER_LBS_PORT2         0x20000
#define NIU_MASK_SYNC_FLAG_SERVERUSERNAME           0x40000
#define NIU_MASK_SYNC_FLAG_SERVERPASSWORD           0x80000
#define NIU_MASK_SYNC_FLAG_AESKEY                   0x100000
#define NIU_MASK_SYNC_FLAG_MQTTPUBLISH              0x200000
#define NIU_MASK_SYNC_FLAG_MQTTSUBSCRIBE            0x400000
#define NIU_MASK_SYNC_FLAG_ALL                      0x7FFFFF


typedef enum  _NIU_DATA_CONFIG_INDEX_
{
    NIU_DATA_CONFIG_DEVICE_TYPE = 0,
    NIU_DATA_CONFIG_DEVICE,
    NIU_DATA_CONFIG_PRODUCT_TYPE,
    NIU_DATA_CONFIG_EID,
    NIU_DATA_CONFIG_SERVER_URL,
    NIU_DATA_CONFIG_SERVER_PORT,
    NIU_DATA_CONFIG_SERIAL_NUMBER,
    NIU_DATA_CONFIG_SOFT_VER,
    NIU_DATA_CONFIG_HARD_VER,
    NIU_DATA_CONFIG_CLIENT_ID,
    NIU_DATA_CONFIG_USERNAME,
    NIU_DATA_CONFIG_PASSWORD,
    NIU_DATA_CONFIG_AES_KEY,
    NIU_DATA_CONFIG_MQTT_SUBSCRIBE,
    NIU_DATA_CONFIG_MQTT_SUBSCRIBE_ITEMS,
    NIU_DATA_CONFIG_MQTT_PUBLISH,
    NIU_DATA_CONFIG_MQTT_PUBLISH_ITEMS,
    NIU_DATA_CONFIG_BLE_MAC,
    NIU_DATA_CONFIG_BLE_PASSWORD,
    NIU_DATA_CONFIG_BLE_AES,
    NIU_DATA_CONFIG_BLE_NAME,
    NIU_DATA_CONFIG_SERVER_URL2,
    NIU_DATA_CONFIG_SERVER_PORT2,
    NIU_DATA_CONFIG_SERVER_ERROR_URL,
    NIU_DATA_CONFIG_SERVER_ERROR_PORT,
    NIU_DATA_CONFIG_SERVER_ERROR_URL2,
    NIU_DATA_CONFIG_SERVER_ERROR_PORT2,
    NIU_DATA_CONFIG_SERVER_LBS_URL,
    NIU_DATA_CONFIG_SERVER_LBS_PORT,
    NIU_DATA_CONFIG_SERVER_LBS_URL2,
    NIU_DATA_CONFIG_SERVER_LBS_PORT2,
    NIU_DATA_CONFIG_REMARK,
    NIU_DATA_CONFIG_ECU_ID,
    NIU_DATA_CONFIG_IMEI,
}NIU_DATA_CONFIG_INDEX;

typedef enum _NIU_DATA_RW_INDEX_
{
    NIU_DATA_TEMPLATE_VER = 0,
    NIU_DATA_RW_TEMPLATE_1,
    NIU_DATA_RW_TEMPLATE_2,
    NIU_DATA_RW_TEMPLATE_3,
    NIU_DATA_RW_TEMPLATE_4,
    NIU_DATA_RW_TEMPLATE_5,
    NIU_DATA_RW_TEMPLATE_6,
    NIU_DATA_RW_TEMPLATE_7,
    NIU_DATA_RW_TEMPLATE_8,
    NIU_DATA_RW_TEMPLATE_9,
    NIU_DATA_RW_TEMPLATE_10,
    NIU_DATA_RW_TEMPLATE_11,
    NIU_DATA_RW_TEMPLATE_12,
    NIU_DATA_RW_TEMPLATE_13,
    NIU_DATA_RW_TEMPLATE_14,
    NIU_DATA_RW_TEMPLATE_15,
    NIU_DATA_RW_TEMPLATE_16,
    NIU_DATA_RW_TEMPLATE_17,
    NIU_DATA_RW_TEMPLATE_18,
    NIU_DATA_RW_TEMPLATE_19,
    NIU_DATA_RW_TEMPLATE_20,
    NIU_DATA_RW_TEMPLATE_21,
    NIU_DATA_RW_TEMPLATE_22,
    NIU_DATA_RW_TEMPLATE_23,
    NIU_DATA_RW_TEMPLATE_24,
    NIU_DATA_RW_TEMPLATE_25,
    NIU_DATA_RW_TEMPLATE_26,
    NIU_DATA_RW_TEMPLATE_27,
    NIU_DATA_RW_TEMPLATE_28,
    NIU_DATA_RW_TEMPLATE_29,
    NIU_DATA_RW_TEMPLATE_30,
    NIU_DATA_RW_TEMPLATE_31,
    NIU_DATA_RW_TEMPLATE_32,
    NIU_DATA_RW_TEMPLATE_33,
    NIU_DATA_RW_TEMPLATE_34,
    NIU_DATA_RW_TEMPLATE_35,
    NIU_DATA_RW_TEMPLATE_36,
    NIU_DATA_RW_TEMPLATE_37,
    NIU_DATA_RW_TEMPLATE_38,
    NIU_DATA_RW_TEMPLATE_39,
    NIU_DATA_RW_TEMPLATE_40,
    NIU_DATA_RW_TEMPLATE_41,
    NIU_DATA_RW_TEMPLATE_42,
    NIU_DATA_RW_TEMPLATE_43,
    NIU_DATA_RW_TEMPLATE_44,
    NIU_DATA_RW_TEMPLATE_45,
    NIU_DATA_RW_TEMPLATE_46,
    NIU_DATA_RW_TEMPLATE_47,
    NIU_DATA_RW_TEMPLATE_48,
    NIU_DATA_RW_TEMPLATE_49,
    NIU_DATA_RW_TEMPLATE_50,
    NIU_DATA_RW_MAX,
}NIU_DATA_RW_INDEX;

#define NIU_DATA(base, id) (niu_data_read(base, id))
#define NIU_DATA_ECU(id) (niu_data_read(NIU_ECU, id))
#define NIU_DATA_BMS(id) (niu_data_read(NIU_BMS, id))
#define NIU_DATA_FOC(id) (niu_data_read(NIU_FOC, id))
#define NIU_DATA_LOCK(id) (niu_data_read(NIU_LOCKBOARD, id))
#define NIU_DATA_LCU(id) (niu_data_read(NIU_LCU, id))
#define NIU_DATA_DB(id) (niu_data_read(NIU_DB, id))
#define NIU_DATA_ALARM(id) (niu_data_read(NIU_ALARM, id))
#define NIU_DATA_BCS(id) (niu_data_read(NIU_BCS, id))
#define NIU_DATA_BMS2(id) (niu_data_read(NIU_BMS2, id))
#define NIU_DATA_QC(id) (niu_data_read(NIU_QC, id))
#define NIU_DATA_EMCU(id) (niu_data_read(NIU_EMCU, id))


JBOOL niu_data_init();
void niu_ecu_data_sync();

JBOOL niu_data_write(NIU_DATA_ADDR_BASE base, NUINT32 id, NIU_DATA_VALUE *value);
JBOOL niu_data_clean_value(NIU_DATA_ADDR_BASE base, NUINT32 id);


JBOOL niu_data_write_value_from_stm32(NIU_DATA_ADDR_BASE base, NUINT32 id, NUINT8 *value, NUINT8 size);
JBOOL niu_data_write_value(NIU_DATA_ADDR_BASE base, NUINT32 id, NUINT8*value, NUINT8 size);
NIU_DATA_VALUE*  niu_data_read(NIU_DATA_ADDR_BASE base, NUINT32 id);



void niu_data_value_trace(NIU_DATA_ADDR_BASE base, NUINT32 id, NIU_DATA_VALUE *value);

NIU_SERVER_UPL_TEMPLATE *niu_data_read_template(NUINT8 index);
JBOOL niu_data_write_template(NIU_SERVER_UPL_TEMPLATE *upl_cmd, NUINT8 index);



//function define for niu_data_config.c
JBOOL niu_data_config_init();
JBOOL niu_data_config_read(NIU_DATA_CONFIG_INDEX index, NUINT8* addr, NUINT32 len);
int niu_config_data_check(char *fname);

JVOID niu_data_rw_init();
JVOID niu_data_rw_default();

JBOOL niu_data_rw_read(NIU_DATA_RW_INDEX index, NUINT8 *addr, NUINT32 len);
JBOOL niu_data_rw_write(NIU_DATA_RW_INDEX index, NUINT8 *addr, NUINT32 len);
int niu_config_data_sync_to_rw(char *fname);

JVOID niu_data_rw_template_write(NUINT32 index, NIU_SERVER_UPL_TEMPLATE* templ);
JVOID niu_data_rw_template_read(NUINT32 index, NIU_SERVER_UPL_TEMPLATE* templ);

//baron.qian add start: check config fields sync 
JVOID niu_data_config_sync_flag_set(int flag);
int niu_data_config_sync_mask_get();
JVOID niu_data_config_sync_mask_reset();
JVOID niu_data_config_sync_mask_set(NUINT32 id);
JVOID niu_data_all_config_fields_req();
int niu_config_data_sync_to_niu_data(char *fname);
//test function

JVOID show_ecu_state(NUINT32 state);
#ifdef __cplusplus 
}
#endif
#endif /* !NIU_DATA_H_ */

