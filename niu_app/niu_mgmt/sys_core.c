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

#include <ql_oe.h>
#include "service.h"
#include "sys_core.h"
#include "niu_data_pri.h"
int g_lower_power_mode = 0;
sim_client_handle_type g_h_sim;
nw_client_handle_type g_h_nw;

extern int g_data_init;
extern data_registration_state_info_t g_data_reg_state_info;
extern void sys_event_push(SYS_EVENT_E cmd, void *param, int param_len);
extern JBOOL niu_data_write_value(NIU_DATA_ADDR_BASE base, NUINT32 id, NUINT8 *value, NUINT8 size);

char *tech_domain[] = {
    "NONE", "3GPP", "3GPP2"
};
char *radio_tech[] = {
    "unknown", "TD_SCDMA", "GSM", "HSPAP", "LTE", "EHRPD",
    "EVDO_B", "HSPA", "HSUPA", "HSDPA", "EVDO_A", "EVDO_0",
    "1xRTT", "IS95B", "IS95A", "UMTS", "EDGE", "GPRS", "NONE"
};

void failed_init_info_post(SYS_ABORT_CODE_E exitcode, char *exit_log, char *func_name, int ret)
{
    LOG_D("%s failed, ret = %d", func_name, ret);
    sys_abort(exitcode, exit_log);
}

void net_info_get()
{
    int ret = 0;
    ql_data_call_s call = {0};
    ql_data_call_error_e err = QL_DATA_CALL_ERROR_NONE;
    ql_data_call_info_s info;  

    QL_MUTEX_LOCK(&g_data_reg_state_info.mutex);
    call.profile_idx = g_data_reg_state_info.profile_id;
    call.ip_family = g_data_reg_state_info.ip_family;
    call.reconnect = true;
    QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);

    ret=QL_Data_Call_Info_Get(call.profile_idx, call.ip_family, &info, &err);
    if(ret != 0) {
        LOG_D("QL_Data_Call_Info_Get failed,error is %d,ret is %d", err, ret);
    }
    else {
        QL_MUTEX_LOCK(&g_data_reg_state_info.mutex);
        if((QL_DATA_CALL_TYPE_IPV4 == call.ip_family) || (QL_DATA_CALL_TYPE_IPV4V6 == call.ip_family)) {
            if(QL_DATA_CALL_CONNECTED == info.v4.state) {
                g_data_reg_state_info.net_state = SYS_NET_STATUS_CONNECTED;
                ip_to_int(inet_ntoa(info.v4.addr.ip));
            }
        }
        else if((QL_DATA_CALL_TYPE_IPV6 == call.ip_family) || (QL_DATA_CALL_TYPE_IPV4V6 == call.ip_family)) {
            if(QL_DATA_CALL_CONNECTED == info.v6.state) {
                g_data_reg_state_info.net_state = SYS_NET_STATUS_CONNECTED;
            }
        }
        QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
    }
}

void ql_mcm_nw_event_cb_func(nw_client_handle_type h_nw,uint32_t ind_flag,void *ind_msg_buf,uint32_t ind_msg_len,void *contextptr)
{
    //LOG_D("enter, ind_flag = %08x\n",ind_flag);
    switch(ind_flag) {
        case NW_IND_DATA_REG_EVENT_IND_FLAG:
        {
            QL_MCM_NW_DATA_REG_EVENT_IND_T *data_reg_info = (QL_MCM_NW_DATA_REG_EVENT_IND_T *)ind_msg_buf;

            if(data_reg_info->registration_valid) {
                QL_MUTEX_LOCK(&g_data_reg_state_info.mutex);
                if(data_reg_info->registration.registration_state == E_QL_MCM_NW_SERVICE_FULL) {
                    if(g_data_reg_state_info.is_registration == 0) {
                        g_data_reg_state_info.is_registration = 1;
                        sys_event_push(SYS_EVENT_DATA_REG_SUCCEED, NULL, 0);
                    }
                }
                else {
                    if(g_data_reg_state_info.is_registration == 1) {
                        g_data_reg_state_info.is_registration = 0;
                        sys_event_push(SYS_EVENT_DATA_REG_FAILED, NULL, 0);
                    }
                }
                QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
            }
            break;
        }
        case NW_IND_SIGNAL_STRENGTH_EVENT_IND_FLAG:
        {
            int rssi_value = 0;
            unsigned char csq_value = 0;
            QL_MCM_NW_SINGNAL_EVENT_IND_T *signal_strength_info=(QL_MCM_NW_SINGNAL_EVENT_IND_T *)ind_msg_buf;
            if(signal_strength_info->gsm_sig_info_valid) {
                rssi_value = signal_strength_info->gsm_sig_info.rssi;
                //printf("gsm...\n");
            }
            if(signal_strength_info->wcdma_sig_info_valid) {
                rssi_value = signal_strength_info->wcdma_sig_info.rssi;
                //printf("wcdma...\n");
            }
            if(signal_strength_info->tdscdma_sig_info_valid) {
                rssi_value = signal_strength_info->tdscdma_sig_info.rssi;
                //printf("tdscdma...\n");
            }
            if(signal_strength_info->lte_sig_info_valid) {
                rssi_value = signal_strength_info->lte_sig_info.rssi;
                //printf("lte...\n");
            }
            if(signal_strength_info->cdma_sig_info_valid) {
                rssi_value = signal_strength_info->cdma_sig_info.rssi;
                //printf("cdma...\n");
            }
            if(signal_strength_info->hdr_sig_info_valid) {
                rssi_value = signal_strength_info->hdr_sig_info.rssi;
                //printf("hdr...\n");
            }
            csq_value = (rssi_value + 113)/2;
            if(1 == g_data_init) {
                niu_data_write_value(NIU_ECU, NIU_ID_ECU_CSQ, (NUINT8 *)&csq_value, sizeof(unsigned char));
            }
            break;
        }
        default:
            break;
    }
}

static void data_call_cb_func(ql_data_call_state_s *state)
{ 
    QL_MUTEX_LOCK(&g_data_reg_state_info.mutex);
    if((g_data_reg_state_info.profile_id == state->profile_idx) &&
            (QL_DATA_CALL_CONNECTED == state->state) && 
            (g_data_reg_state_info.ip_family == state->ip_family)) {
        ip_to_int(inet_ntoa(state->v4.ip));
        g_data_reg_state_info.net_state = SYS_NET_STATUS_CONNECTED;
        sys_event_push(SYS_EVENT_NET_CONNECTED, NULL, 0);
    }
    else {
        g_data_reg_state_info.net_state = SYS_NET_STATUS_CONNECTING;
        sys_event_push(SYS_EVENT_NET_DISCONNECT, NULL, 0);
    }
    QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
}

void cmd_sys_init(void *param)
{
    int ret = 0;
   
    ret = QL_MCM_NW_Client_Init(&g_h_nw);
    if(0 != ret) {
        failed_init_info_post(SYS_ABORT_CODE_API_FAILED, "nw client init failed", "QL_MCM_NW_Client_Init", ret);
        return;
    }
    
    ret = QL_MCM_NW_SetLowPowerMode(g_h_nw, 0);
    if(0 != ret) {
        LOG_D("set low power mode as 0 failure, return is %d", ret);
    }
    else {
        g_lower_power_mode = 0;
    }

    ret = QL_MCM_NW_AddRxMsgHandler(g_h_nw, ql_mcm_nw_event_cb_func, NULL);
    if(0 != ret) {
        failed_init_info_post(SYS_ABORT_CODE_API_FAILED, "nw add message handler failed", "QL_MCM_NW_AddRxMsgHandler", ret);
        return;
    }

    ret = QL_MCM_NW_EventRegister(g_h_nw, NW_IND_DATA_REG_EVENT_IND_FLAG | NW_IND_SIGNAL_STRENGTH_EVENT_IND_FLAG | NW_IND_CELL_ACCESS_STATE_CHG_EVENT_IND_FLAG);
    if(0 != ret) {
        failed_init_info_post(SYS_ABORT_CODE_API_FAILED, "nw event register failed", "QL_MCM_NW_EventRegister", ret);
        return;
    }

    ret = QL_MCM_SIM_Client_Init(&g_h_sim);
    if(0 != ret) {
        failed_init_info_post(SYS_ABORT_CODE_API_FAILED, "sim init failed", "QL_MCM_SIM_Client_Init", ret);
        return;
    }

    ret = QL_Data_Call_Init(data_call_cb_func);
    if(0 != ret) {
        failed_init_info_post(SYS_ABORT_CODE_API_FAILED, "data call init failed", "ql_data_call_init", ret);
        return;
    }
    sys_event_push(SYS_EVENT_SYS_INIT_SUCCEED, NULL, 0);
}

int cmd_data_set_apn_item(char *name, char *username, char *password, int auth_type)
{
    int ret;
    ql_apn_info_s info = {0};

    if(name == NULL || username == NULL || password == NULL)
    {
        LOG_E("apn params error.");
        return -1;
    }

    if(strlen(name) >= QL_APN_NAME_SIZE || strlen(username) >= QL_APN_USERNAME_SIZE || strlen(password) >= QL_APN_PASSWORD_SIZE)
    {
        LOG_E("apn params length error.");
        return -1;
    }

    if(auth_type < QL_APN_AUTH_PROTO_NONE || auth_type > QL_APN_AUTH_PROTO_PAP_CHAP)
    {
        LOG_E("apn auth type error.");
        return -1;
    }

    info.profile_idx = 1;
    info.pdp_type = QL_APN_PDP_TYPE_IPV4;
    strncpy(info.apn_name, name, sizeof(info.apn_name));
    strncpy(info.username, username, sizeof(info.username));
    strncpy(info.password, password, sizeof(info.password));
    info.auth_proto = auth_type;

    ret = QL_APN_Set(&info);
    if(ret)
    {
        LOG_D("QL_APN_Set error[name: %s].", info.apn_name);
    }
    return ret;
}

int cmd_data_set_apn()
{
    int ret = -1;
    char iccid[21] = {0};

    ret = cmd_iccid_info_get(iccid, sizeof(iccid));
    if(ret)
    {
        LOG_E("cmd_iccid_info_get failed.");
        return -1;
    }
    LOG_D("set apn: iccid: %s", iccid);
    if(memcmp(iccid, "894301", 6) == 0)
    {
        //TELK
        return cmd_data_set_apn_item("m2m.tag.com", "tagm2m", "m2m572", QL_APN_AUTH_PROTO_NONE);
    }
    else if(memcmp(iccid, "894700", 6) == 0)
    {
        //NORGE
        return cmd_data_set_apn_item("telenor","","", QL_APN_AUTH_PROTO_NONE);
    }
    else if(memcmp(iccid, "893144", 6) == 0)
    {
        //VODAFONE
        return cmd_data_set_apn_item("niutech","","", QL_APN_AUTH_PROTO_NONE);
    }
    else if(memcmp(iccid, "891039", 6) == 0)
    {
        //ANTRAX
        return cmd_data_set_apn_item("Pulic4.m2minternet.com","","", QL_APN_AUTH_PROTO_NONE);
    }
    else if(memcmp(iccid, "899770", 6) == 0)
    {
        //NIBOER
        return cmd_data_set_apn_item("smart","","", QL_APN_AUTH_PROTO_NONE);
    }
    //898600|898602|898604|898607
    else if(memcmp(iccid, "898600", 6) == 0 || memcmp(iccid, "898602", 6) == 0 || 
            memcmp(iccid, "898604", 6) == 0 || memcmp(iccid, "898607", 6) == 0)
    {
        //Chinamobile
        return cmd_data_set_apn_item("CMIOT","","", QL_APN_AUTH_PROTO_NONE);
    }
    else if(memcmp(iccid, "898601", 6) == 0 || memcmp(iccid, "898606", 6) == 0 ||
            memcmp(iccid, "898609", 6) == 0)
    //898601|898606|898609
    {
        //Chinaunion
        return cmd_data_set_apn_item("UNIM2M.NJM2MAPN","","", QL_APN_AUTH_PROTO_NONE);
    }
    return -1;
}

void cmd_data_call_start(void *param)
{
   int ret = 0;
   ql_data_call_error_e call_err = QL_DATA_CALL_ERROR_NONE;
   ql_data_call_s call = {0};
   QL_MUTEX_LOCK(&g_data_reg_state_info.mutex);
   call.profile_idx = g_data_reg_state_info.profile_id;
   call.ip_family = g_data_reg_state_info.ip_family;
   call.reconnect = true;
   QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
   
   net_info_get();
   
   QL_MUTEX_LOCK(&g_data_reg_state_info.mutex);
   if(g_data_reg_state_info.net_state == SYS_NET_STATUS_NONE) {
       /*baron.qian-2019/05/16 start: add apn set*/
       LOG_D("=================================start cmd_data_set_apn..");
       cmd_data_set_apn();
       LOG_D("=================================end cmd_data_set_apn..");
       /*baron.qian-2019/05/16 end*/
       ret = QL_Data_Call_Start(&call, &call_err);
       if(ret != 0) {
           LOG_D("QL_Data_Call_Start failed,ret is %d", ret);
           QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
           return;
       }
       g_data_reg_state_info.net_state = SYS_NET_STATUS_CONNECTING;
   }
   else if(g_data_reg_state_info.net_state == SYS_NET_STATUS_CONNECTED) {
       sys_event_push(SYS_EVENT_NET_CONNECTED, NULL, 0);
   }
   else if(g_data_reg_state_info.net_state == SYS_NET_STATUS_CONNECTING) {
   }
   else {
   }
   QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
}

void cmd_data_call_stop(void *param)
{
    int ret = 0;
    ql_data_call_error_e err;
    ql_data_call_s call;
    
    QL_MUTEX_LOCK(&g_data_reg_state_info.mutex);

    call.profile_idx = g_data_reg_state_info.profile_id;
    call.ip_family = g_data_reg_state_info.ip_family;

    if((g_data_reg_state_info.net_state == SYS_NET_STATUS_CONNECTED) || (g_data_reg_state_info.net_state == SYS_NET_STATUS_CONNECTING)) {
        ret = QL_Data_Call_Stop(call.profile_idx, call.ip_family, &err);
        if(ret != 0) {
            LOG_D("QL_Data_Call_Stop,err is %d", err);
            QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
        }
    }
    
    g_data_reg_state_info.net_state = SYS_NET_STATUS_IDLE; 
    QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
}

void cmd_operator_info_get(void *param)
{
    int ret = 0;
    QL_MCM_NW_OPERATOR_NAME_INFO_T info;
    ret = QL_MCM_NW_GetOperatorName(g_h_nw, &info);
    if(0 != ret) {
        LOG_D("get operatin name failed, return value is %d", ret);
    }
}

int cmd_iccid_info_get(char *iccid, int len)
{
    memset(iccid, 0, len);
    return QL_MCM_SIM_GetICCID(g_h_sim, E_QL_MCM_SIM_SLOT_ID_1, iccid, len);
}

int cmd_imsi_info_get(char *imsi, int len)
{
    int ret = 0;
    QL_SIM_APP_ID_INFO_T t_info;
    t_info.e_slot_id =  E_QL_MCM_SIM_SLOT_ID_1;
    t_info.e_app = E_QL_MCM_SIM_APP_TYPE_3GPP;
    memset(imsi, 0, len);
    //ret =  QL_SIM_GetIMSI((E_QL_SIM_ID_T)g_h_sim, g_imsi_info.imsi, MAX_CHAR);
    ret = QL_MCM_SIM_GetIMSI(g_h_sim, &t_info, imsi, len);  
    //printf("imsi is %s\n", imsi);
    return ret;
}

int cmd_imei_info_get(char *imei, int len)
{
    memset(imei, 0, len);
    return QL_DEV_GetImei(imei, len);
}

int cmd_cell_info_get(QL_MCM_NW_CELL_INFO_T *cell_info, int len)
{
    int ret = 0;
    if((NULL == cell_info) || (len != sizeof(QL_MCM_NW_CELL_INFO_T))) {
        LOG_D("get cell info failed while param is not valid");
        return -1;
    }
    memset(cell_info, 0, len);
    ret = QL_MCM_NW_GetCellInfo(g_h_nw, cell_info);
    if(0 != ret) {
        LOG_D("get cell info failed, ret is %d", ret);
        return -2;
    }
    return ret;
}

int cmd_signal_strength_info_get(void *param)
{
    int ret = 0;
    int signal = 0;
    unsigned char csq_value = 0;
    QL_MCM_NW_SIGNAL_STRENGTH_INFO_T t_info;
    memset(&t_info, 0, sizeof(QL_MCM_NW_SIGNAL_STRENGTH_INFO_T));
    ret = QL_MCM_NW_GetSignalStrength(g_h_nw, &t_info);
    if(0 != ret) {
        LOG_D("QL_MCM_NW_GetSignalStrength failed, ret = %d", ret);
        return 0;
    }
    if(t_info.gsm_sig_info_valid) {
        signal = t_info.gsm_sig_info.rssi;
    } 
    if(t_info.wcdma_sig_info_valid) {
        signal = t_info.wcdma_sig_info.rssi;
    } 
    if(t_info.tdscdma_sig_info_valid) {
        signal = t_info.tdscdma_sig_info.rssi;
    } 
    if(t_info.lte_sig_info_valid) {
        signal = t_info.lte_sig_info.rssi;
    } 
    if(t_info.cdma_sig_info_valid) {
        signal = t_info.cdma_sig_info.rssi;
    } 
    if(t_info.hdr_sig_info_valid) {
        signal = t_info.hdr_sig_info.rssi;
    }
    csq_value = (signal + 113)/2;
    if(1 == g_data_init) {
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_CSQ, (NUINT8 *)&csq_value, sizeof(unsigned char));
    }
    return signal;
}

void cmd_register_info_get(void *param)
{
    int ret = 0;
    QL_MCM_NW_REG_STATUS_INFO_T reg_state_info = {0};

    ret=QL_MCM_NW_GetRegStatus(g_h_nw, &reg_state_info);
    if(ret != 0) {
        LOG_D("get reg status failed\n");
    }
    else {
        QL_MUTEX_LOCK(&g_data_reg_state_info.mutex);
        if(reg_state_info.data_registration_valid) {
            LOG_D("data registration state is %d", reg_state_info.data_registration.registration_state);
            if(reg_state_info.data_registration.registration_state == E_QL_MCM_NW_SERVICE_FULL) {
                if(g_data_reg_state_info.is_registration == 0) {
                    g_data_reg_state_info.is_registration = 1;
                    sys_event_push(SYS_EVENT_DATA_REG_SUCCEED, NULL, 0); 
                }
            }
            else {
                if(g_data_reg_state_info.is_registration == 1) {
                    g_data_reg_state_info.is_registration=0;
                    sys_event_push(SYS_EVENT_DATA_REG_FAILED, NULL, 0);
                }
            }
        }
        QL_MUTEX_UNLOCK(&g_data_reg_state_info.mutex);
    }
}

void get_base_station_info(base_station_info_t *base_info)
{
    int ret = 0;
    QL_MCM_NW_REG_STATUS_INFO_T t_info = {0};
    
    if(NULL == base_info) {
        LOG_D("get base station info while param is null");
        return;
    }

    memset(base_info, 0, sizeof(base_station_info_t));

    ret=QL_MCM_NW_GetRegStatus(g_h_nw, &t_info);
    if(ret != 0) {
        LOG_D("get reg status failed while get base station info\n");
        return;
    }
    if(t_info.data_registration_details_3gpp_valid) {
        base_info->non_cdma_flag = 1;
        memcpy(base_info->non_cdma_mcc, t_info.data_registration_details_3gpp.mcc, sizeof(base_info->non_cdma_mcc));
        memcpy(base_info->non_cdma_mnc, t_info.data_registration_details_3gpp.mnc, sizeof(base_info->non_cdma_mnc));
        base_info->cid = t_info.data_registration_details_3gpp.cid;
        base_info->lac = t_info.data_registration_details_3gpp.lac;
        base_info->psc = t_info.data_registration_details_3gpp.psc;
        base_info->tac = t_info.data_registration_details_3gpp.tac;
    }
    if(t_info.voice_registration_details_3gpp_valid) {
        base_info->non_cdma_flag = 1;
        memcpy(base_info->non_cdma_mcc, t_info.voice_registration_details_3gpp.mcc, sizeof(base_info->non_cdma_mcc));
        memcpy(base_info->non_cdma_mnc, t_info.voice_registration_details_3gpp.mnc, sizeof(base_info->non_cdma_mnc));
        base_info->cid = t_info.voice_registration_details_3gpp.cid;
        base_info->lac = t_info.voice_registration_details_3gpp.lac;
        base_info->psc = t_info.voice_registration_details_3gpp.psc;
        base_info->tac = t_info.voice_registration_details_3gpp.tac;
    }
    if(t_info.data_registration_details_3gpp2_valid) {
        base_info->cdma_flag = 1;
        memcpy(base_info->cdma_mcc, t_info.data_registration_details_3gpp2.mcc, sizeof(base_info->cdma_mcc));
        memcpy(base_info->cdma_mnc, t_info.data_registration_details_3gpp2.mnc, sizeof(base_info->cdma_mnc));
        base_info->sid = t_info.data_registration_details_3gpp2.sid;
        base_info->nid = t_info.data_registration_details_3gpp2.nid;
        base_info->bsid = t_info.data_registration_details_3gpp2.bsid;
    }
    if(t_info.voice_registration_details_3gpp2_valid) {
        base_info->cdma_flag = 1;
        memcpy(base_info->cdma_mcc, t_info.voice_registration_details_3gpp2.mcc, sizeof(base_info->cdma_mcc));
        memcpy(base_info->cdma_mnc, t_info.voice_registration_details_3gpp2.mnc, sizeof(base_info->cdma_mnc));
        base_info->sid = t_info.voice_registration_details_3gpp2.sid;
        base_info->nid = t_info.voice_registration_details_3gpp2.nid;
        base_info->bsid = t_info.voice_registration_details_3gpp2.bsid;
    }
}

void cmd_set_lowpower(unsigned int flag) 
{
    int ret = 0;
    if(flag == g_lower_power_mode) {
        return;
    }
    // ret = QL_MCM_NW_SetLowPowerMode(g_h_nw, flag);
    ret = QL_NW_ForbidInd(flag);    //juson.zhang-2019/05/17
    if(0 != ret) {
        LOG_D("set low power mode as %d failure, return is %d", flag, ret);
    }
    else {
        g_lower_power_mode = flag;
    }
}

void ip_to_int(char *ipstring)
{
    unsigned int ui_ip = 0;
    if(NULL != ipstring) {
        ui_ip = inet_addr(ipstring);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_AP_IP, (NUINT8 *)&ui_ip, sizeof(unsigned int));
    }
}
