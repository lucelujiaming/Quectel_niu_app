/*-----------------------------------------------------------------------------------------------*/
/**
  @file service.h
  @brief Framework APIs 
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

#ifdef __cplusplus
 extern "C" {
#endif


#ifndef __SERVICE_H__
#define __SERVICE_H__
#include "ql_common.h"
#include "ql_mem.h"


#define DIFF_TIME 28800
#define MAX_CHAR 32
typedef enum {
    SYS_ABORT_CODE_MIN = 0,
    SYS_ABORT_CODE_INIT,
    SYS_ABORT_CODE_API_FAILED,
    SYS_ABORT_CODE_TIMEOUT,
    SYS_ABORT_CODE_MEM_ALLOC,
    SYS_ABORT_CODE_INTERNAL,
    SYS_ABORT_CODE_UNKNOW,
    SYS_ABORT_CODE_MAX
} SYS_ABORT_CODE_E;

typedef enum {
    SYS_EVENT_MIN = 0,
    SYS_EVENT_SYS_INIT_SUCCEED,
    SYS_EVENT_DATA_REG_SUCCEED,
    SYS_EVENT_DATA_REG_FAILED,
    SYS_EVENT_NET_CONNECTED,
    SYS_EVENT_NET_DISCONNECT,
    SYS_EVENT_SOCKET_CONNECTED,
    SYS_EVENT_SOCKET_DISCONNECT,
    MQTT_EVENT_MQTT_CONNECTED,
    MQTT_EVENT_MQTT_DISCONNECT,
    MQTT_EVENT_MQTT_SUB_SUCCEED,
    MQTT_EVENT_MQTT_SUB_FAILED,
    /*SYS_EVENT_NET_IDLE,
    SYS_EVENT_NET_CONNECTING,
    SYS_EVENT_NET_CONNECTED,
    SYS_EVENT_NET_DISCONNECTED,
    SYS_EVENT_NET_OSS,*/
    SYS_EVENT_GPS_EVENT_RX,
    SYS_EVENT_ASYNC_ON_PROGRESS,
    SYS_EVENT_ASYNC_ON_RESULT,
    MQTT_EVENT_PING_REQ_START,
    MQTT_EVENT_PING_RESP_FAILED,
    MQTT_EVENT_PING_RESP_SUCCEED,
    MQTT_EVENT_SOCKET_READ_ERROR,
    MQTT_EVENT_SOCKET_SERVER_CLOSE,
    MQTT_EVENT_MQTT_ERROR,
    MQTT_EVENT_TEMPLATE_UPDATE,

    FOTA_EVENT_CHECK_COMPLETED,
    FOTA_EVENT_DOWNLOAD_COMPLETED,
    FOTA_EVENT_UPGRADE_DEVICE_COMPLETED,
    FOTA_EVENT_UPGRADE_COMPLETED,
    FOTA_EVENT_UPLOAD_LOG_1_COMPLETED,
    FOTA_EVENT_UPLOAD_LOG_1_AGAIN,
    FOTA_EVENT_UPLOAD_LOG_2_COMPLETED,
    FOTA_EVENT_UPLOAD_LOG_2_AGAIN,

    FOTA_EVENT_UPGRADE_FILE_TYPE_ERROR,
    FOTA_EVENT_UPGRADE_EC25_TO_STM32_RATE,      //stm32升级进度
    FOTA_EVENT_UPGRADE_EC25_TO_STM32_FINISH,    //升级成功
    FOTA_EVENT_UPGRADE_STM32_TO_EC25_RATE,      //ec25升级进度   
    FOTA_EVENT_CONFIG_STM32_TO_EC25_FINISH,     //升级成功
    FOTA_EVENT_CONFIG_STM32_TO_EC25_RATE,      //ec25配置进度   
    FOTA_EVENT_UPGRADE_STM32_TO_EC25_FINISH,    //升级成功
    
    SYS_EVENT_MAX
} SYS_EVENT_E;


typedef enum {
    ASYNC_TASK_TYPE_CMD =0,     /**< running in cmd handler list */
    ASYNC_TASK_TYPE_THREAD = 1, /**< running in new thread */  
} ASYNC_TASK_TYPE_E;

/**
 * timer id define
 * 
 */ 
typedef enum {
  TIMER_ID_HANDLER_DEADLOCK_CHECK = 0x01,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_1,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_2,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_3,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_4,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_5,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_6,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_7,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_8,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_9,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_10,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_11,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_12,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_13,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_14,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_15,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_16,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_17,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_18,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_19,
  TIMER_ID_TEMPLATE_NIU_TRIGGER_20,    //21
  TIMER_ID_TEMPLATE_NIU_TRIGGER_21,    //22
  TIMER_ID_TEMPLATE_NIU_TRIGGER_22,    //23
  TIMER_ID_TEMPLATE_NIU_TRIGGER_23,    //24
  TIMER_ID_TEMPLATE_NIU_TRIGGER_24,    //25
  TIMER_ID_TEMPLATE_NIU_TRIGGER_25,    //26
  TIMER_ID_TEMPLATE_NIU_TRIGGER_26,    //27
  TIMER_ID_TEMPLATE_NIU_TRIGGER_27,    //28
  TIMER_ID_TEMPLATE_NIU_TRIGGER_28,    //29
  TIMER_ID_TEMPLATE_NIU_TRIGGER_29,    //30
  TIMER_ID_TEMPLATE_NIU_TRIGGER_30,    //31
  TIMER_ID_TEMPLATE_NIU_TRIGGER_31,    //32
  TIMER_ID_TEMPLATE_NIU_TRIGGER_32,    //33
  TIMER_ID_TEMPLATE_NIU_TRIGGER_33,    //34
  TIMER_ID_TEMPLATE_NIU_TRIGGER_34,    //35
  TIMER_ID_TEMPLATE_NIU_TRIGGER_35,    //36
  TIMER_ID_TEMPLATE_NIU_TRIGGER_36,    //37
  TIMER_ID_TEMPLATE_NIU_TRIGGER_37,    //39
  TIMER_ID_TEMPLATE_NIU_TRIGGER_38,    //40
  TIMER_ID_TEMPLATE_NIU_TRIGGER_39,    //41
  TIMER_ID_TEMPLATE_NIU_TRIGGER_40,    //42
  TIMER_ID_TEMPLATE_NIU_TRIGGER_41,    //43
  TIMER_ID_TEMPLATE_NIU_TRIGGER_42,    //44
  TIMER_ID_TEMPLATE_NIU_TRIGGER_43,    //45
  TIMER_ID_TEMPLATE_NIU_TRIGGER_44,    //46
  TIMER_ID_TEMPLATE_NIU_TRIGGER_45,    //47
  TIMER_ID_TEMPLATE_NIU_TRIGGER_46,    //48
  TIMER_ID_TEMPLATE_NIU_TRIGGER_47,    //49
  TIMER_ID_TEMPLATE_NIU_TRIGGER_48,    //50
  TIMER_ID_TEMPLATE_NIU_TRIGGER_49,    //51
  TIMER_ID_TEMPLATE_NIU_TRIGGER_50,    //52
  TIMER_ID_FEED_DOG,                //53
  TIMER_ID_NIU_V_TO_ZERO,           //54
  TIMER_ID_NIU_UPDATE_DATA,         //55
  TIMER_ID_REINIT,                  //56
  TIMER_ID_PINGRESP,                //57
  TIMER_ID_TEMPLATE_EMPTY_LOW_POWER,//58
  TIMER_ID_NIU_UPDATE_RW_DATA,      //59
  TIMER_ID_NIU_UPDATE_TEMPLATE,     //60
  TIMER_ID_NIU_ENABLE_SHAKE,        //61
  TIMER_ID_GPSDATA_VALID,         //62
  TIMER_ID_WORKMODE_DATA_UPDATE,  //63
  TIMER_ID_NIU_OPEN_GPS,          //64
  TIMER_ID_NIU_CLOSE_GPS,         //65
  TIMER_ID_WORKMODE_TO_SLEEP,     //66
  TIMER_ID_FOTA_UPGRADE_START,    //67 mcu fota upgrade
  TIMER_ID_MCU_FOTA_TIMEOUT_CHECK, //68
  //TIMER_ID_FOTA_UPGRADE_STM32_CHECK,    //68
  //TIMER_ID_FOTA_UPGRADE_EC25_CHECK,    //69
  //TIMER_ID_CONFIG_UPGRADE_EC25_CHECK,    //70
  TIMER_ID_GSENSOR_INT_VALUE,         //71
  TIMER_ID_GSENSOR_INT_RESET,         //72
  TIMER_ID_TEST_HANDLER,              //73
  TIMER_ID_TEST_HANDLER2,             //74
  TIMER_ID_CELL_INFO_GET,             //75
  TIMER_ID_TIMESTAMP,                 //76
  TIMER_ID_PINGSTART,				  //77

}TIMER_ID_E;

struct async_task_struct;
typedef struct async_task_struct async_task_t;

typedef void (*async_task_handler_f)(async_task_t *task, void *param);
typedef void (*async_task_progress_f)(async_task_t *task, int progress, void *param);
typedef void (*async_task_result_f)(async_task_t *task, int result, void *param);

typedef void (*cmd_handler_func_f)(void *param);
typedef void (*tmr_handler_func_f)(int tmr_id, void *param);

struct async_task_struct {
    void *param;
    ASYNC_TASK_TYPE_E type;
    async_task_handler_f on_handler;
    async_task_progress_f on_progress;
    async_task_result_f on_result; 
};


int cmd_handler_init(void);
int cmd_handler_push(cmd_handler_func_f func, void *param);
int cmd_handler_peeding_time_ms(void);

int tmr_handler_init(void);
int tmr_handler_tmr_add(int tmr_id, tmr_handler_func_f func, void *param, uint32_t interval_ms, int is_cycle);
int tmr_handler_tmr_set(int tmr_id, tmr_handler_func_f func, void *param, uint32_t interval_ms, int is_cycle);
int tmr_handler_tmr_del(int tmr_id);
int tmr_handler_tmr_is_exist(int tmr_id);

void sys_event_push(SYS_EVENT_E cmd, void *param, int param_len);

void sys_abort(int status, const char *fmt, ...);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Start a asynchronous task, on_progress and on_result will be called in main thread 
  @param[in] type Task mode
  @param[in] on_handler Hanlder
  @param[in] on_progress Rate of progress callback
  @param[in] on_result Resule callback
  @param[in] param Parameter invoked by on_handler
  @return 0-succeed, other-failed
  */
/*-----------------------------------------------------------------------------------------------*/
int async_task_start(ASYNC_TASK_TYPE_E type, 
        async_task_handler_f on_handle, 
        async_task_progress_f on_process, 
        async_task_result_f on_result, 
        void *param);

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Push task current progress, on_progress will be called in main thread 
  @param[in] task 
  @param[in] progress User defined 
  @param[in] param Extended paramter 
  @return 0-succeed, other-failed
  */
/*-----------------------------------------------------------------------------------------------*/
int async_task_progress_push(async_task_t *task, int progress, void *param);

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Push task result, on_result will be called in main thread 
  @param[in] task 
  @param[in] result User defined 
  @param[in] param Extended paramter 
  @return 0-succeed, other-failed
  */
/*-----------------------------------------------------------------------------------------------*/
int async_task_result_push(async_task_t *task, int result, void *param);


#endif

#ifdef __cplusplus
 }
#endif

