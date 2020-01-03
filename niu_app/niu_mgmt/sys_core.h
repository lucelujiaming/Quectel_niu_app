/*-----------------------------------------------------------------------------------------------*/
/**
  @file sys_core.h
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

#ifndef __SYS_INFO_H__
#define __SYS_INFO_H__
#include "ql_common.h"
#include "ql_mutex.h"
#include "service.h"
#include "niu_types.h"
typedef enum {
    SYS_NET_STATUS_NONE = 0,
    SYS_NET_STATUS_IDLE,
    SYS_NET_STATUS_CONNECTING,
    SYS_NET_STATUS_CONNECTED,
    SYS_NET_STATUS_DISCONNECTED,
    SYS_NET_STATUS_OSS
} SYS_NET_STATUS_E;

typedef struct data_registration_state_info_struct {
    ql_mutex_t mutex;
    uint8_t is_registration;
    SYS_NET_STATUS_E net_state;
    int profile_id;
    int ip_family;
}data_registration_state_info_t;

typedef struct register_info_struct {
    int domain_index;
    int radio_index;
}register_info_t;

typedef struct base_station_info_struct {
    char cdma_flag;
    char non_cdma_flag;
    char cdma_mcc[4];
    char cdma_mnc[4];
    char non_cdma_mcc[4];
    char non_cdma_mnc[4];
    unsigned int cid;
    unsigned short lac;
    unsigned short psc;
    unsigned short tac;
    unsigned short sid;
    unsigned short nid;
    unsigned short bsid;
}base_station_info_t;

void net_info_get();
void cmd_sys_init(void *param);
void cmd_data_call_start(void *param);
void cmd_data_call_stop(void *param);
void cmd_operator_info_get(void *param);
void cmd_register_info_get(void *param);
int cmd_iccid_info_get(char *iccid, int len);
int cmd_imsi_info_get(char *imsi, int len);
int cmd_imei_info_get(char *imei, int len);
int cmd_cell_info_get(QL_MCM_NW_CELL_INFO_T *cell_info, int len);
void get_base_station_info(base_station_info_t *base_info);
int cmd_signal_strength_info_get(void *param);
void cmd_set_lowpower(unsigned int flag);
void ip_to_int(char *ipstring);
#endif

