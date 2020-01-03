#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ql_oe.h>
#include "utils.h"
#include "niu_data.h"
#include "niu_data_trace.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)       sizeof(a) / sizeof(a[0])
#endif

static char *ecu_id_string[] = {"server_url","server_port","ecu_serial_number","ecu_soft_ver","ecu_hard_ver","imei","iccid","imsi","eid","moved_length","dealer_code","utc","car_type","car_speed_index","tize_size","speed_loc_soc","lost_enery","lost_soc","last_mileage","total_mileage","charge_value","discharge_value","shaking_value","ecu_cfg","ecu_cmd","bms_cmd","foc_cmd","db_cmd","lcu_cmd","alarm_cmd","display_random_code","uploading_template_1","uploading_template_2","uploading_template_3","uploading_template_4","uploading_template_5","uploading_template_6","uploading_template_7","uploading_template_8","uploading_template_9","uploading_template_10","uploading_template_11","uploading_template_12","uploading_template_13","uploading_template_14","uploading_template_15","uploading_template_16","uploading_template_17","uploading_template_18","uploading_template_19","uploading_template_20","longitude","latitude","hdop","phop","heading","gps_speed","ecu_battary","now_bts","near_bts1","near_bts2","near_bts3","near_bts4","ap_ip","car_no_gps_signal_time","car_no_gsm_signal_time","car_sleep_time","x_acc","y_acc","z_acc","x_ang","y_ang","z_ang","x_mag","y_mag","z_mag","air_pressure","ecu_state","car_sta","timestamp","gps_singal","csq","car_running_time","car_stoping_time","ota_exec_eqp","ota_rate","ota_bin_url","ota_soft_ver","ota_hard_ver","ota_pack_size","ota_pack_num","ota_eqp","stop_longitude","stop_latitude","db_code_dis_time","cell_mcc","cell_mnc","cell_lac_1","cell_cid_1","cell_rssi_1","cell_lac_2","cell_cid_2","cell_rssi_2","cell_lac_3","cell_cid_3","cell_rssi_3","cell_lac_4","cell_cid_4","cell_rssi_4","cell_lac_5","cell_cid_5","cell_rssi_5","pole_pairs","radius","calu_speed_index","wifi1_bssid_1","wifi1_bssid_2","wifi1_bssid_3","wifi1_bssid_4","wifi1_bssid_5","wifi1_bssid_6","wifi1_rssi","wifi2_bssid_1","wifi2_bssid_2","wifi2_bssid_3","wifi2_bssid_4","wifi2_bssid_5","wifi2_bssid_6","wifi2_rssi","wifi3_bssid_1","wifi3_bssid_2","wifi3_bssid_3","wifi3_bssid_4","wifi3_bssid_5","wifi3_bssid_6","wifi3_rssi","wifi4_bssid_1","wifi4_bssid_2","wifi4_bssid_3","wifi4_bssid_4","wifi4_bssid_5","wifi4_bssid_6","wifi4_rssi","wifi5_bssid_1","wifi5_bssid_2","wifi5_bssid_3","wifi5_bssid_4","wifi5_bssid_5","wifi5_bssid_6","wifi5_rssi","smt_test","app_control","fota_download_percent","car_eqp_cfg","car_eqp_state","fail_value","fall_pitch","fall_roll","ecu_state2","ecu_adups_ver","move_gsensor_count","move_gsensor_time","pdu_message_current_number","max_snr_1","max_snr_2","eye_settings","model_IMEI","model_temperature","ecu_velocity","ble_mac","ble_password","ble_aes","ble_name","smt_test2","limiting_velocity_max","limiting_velocity_min","front_tire_size","server_url2","server_port2","server_error_url","server_error_port","server_error_url2","server_error_port2","server_lbs_url","server_lbs_port","server_lbs_url2","server_lbs_port2","v35_alarm_state","ecu_keepalive","ecu_gps_max_cnr","ecu_gps_min_cnr","ecu_gps_avg_cnr","ecu_gps_seill_num","ecu_no_cdma_location","ecu_cdma_location","ecu_gps_sleep_time","ecu_gps_awake_time","ecu_avg_longitude","ecu_avg_latitude","ecu_v35_config","ecu_pitch","ecu_roll","ecu_yaw","ecu_server_error_code","ecu_server_error_code_time","ecu_server_connected_time","ecu_server_ka_timeout_time","ecu_server_reset_4g_time","ecu_server_do_connect_count","ecu_server_reset_4g_count","ecu_realtime_longitude","ecu_realtime_latitude","ecu_app_contorl","ecu_app_contorl2","ecu_app_contorl3","ecu_app_contorl10","ecu_app_contorl11","ecu_app_contorl12","ecu_cmd_id","ecu_cmd_result","ecu_cmd_expiry_timestamp","ecu_cmd_receive_timestamp","ecu_cmd_exec_timestamp","ecu_auto_close_time","ecu_ex_model_version","ecu_gps_version","uploading_template_21","uploading_template_22","uploading_template_23","uploading_template_24","uploading_template_25","uploading_template_26","uploading_template_27","uploading_template_28","uploading_template_29","uploading_template_30","uploading_template_31","uploading_template_32","uploading_template_33","uploading_template_34","uploading_template_35","uploading_template_36","uploading_template_37","uploading_template_38","uploading_template_39","uploading_template_40","uploading_template_41","uploading_template_42","uploading_template_43","uploading_template_44","uploading_template_45","uploading_template_46","uploading_template_47","uploading_template_48","uploading_template_49","uploading_template_50","ecu_inherit_mileage","ecu_reboot_count","serverUsername","serverPassword","aesKey","mqttSubscribe","mqttPublish"};
static char *emcu_id_string[] = {"emcu_soft_ver", "emcu_hard_ver", "emcu_sn", "emcu_compiletime", "emcu_cmd", "emcu_alarmkeyid", "emcu_smt_test", "emcu_remote_group_code"};
static char *bms_id_string[] = {"bms_user", "bms_bms_id", "bms_s_ver", "bms_h_ver", "bms_p_pwrd", "bms_bat_type", "bms_sn_id", "bms_oc_vlt_p", "bms_roc_vlt", "bms_dc_vlt_p", "bms_rdc_vlt_p", "bms_c_b_vlt", "bms_c_cur_p", "bms_dc_cur_p", "bms_isc_p", "bms_dcotp_p", "bms_rdcotp_p", "bms_cotp_p", "bms_rcotp_p", "bms_dcltp_p", "bms_rdcltp_p", "bms_ncom_dc_en", "bms_rated_vlt", "bms_bat_to_cap", "bms_c_cont", "bms_to_vlt_rt", "bms_c_cur_rt", "bms_dc_cur_rt", "bms_soc_rt", "bms_bat_sta_rt", "bms_dc_fl_t_rt", "bms_tp1_rt", "bms_tp2_rt", "bms_tp3_rt", "bms_tp4_rt", "bms_tp5_rt", "bms_g_vlt_rt1", "bms_g_vlt_rt2", "bms_g_vlt_rt3", "bms_g_vlt_rt4", "bms_g_vlt_rt5", "bms_g_vlt_rt6", "bms_g_vlt_rt7", "bms_g_vlt_rt8", "bms_g_vlt_rt9", "bms_g_vlt_rt10", "bms_g_vlt_rt11", "bms_g_vlt_rt12", "bms_g_vlt_rt13", "bms_g_vlt_rt14", "bms_g_vlt_rt15", "bms_g_vlt_rt16", "bms_g_vlt_rt17", "bms_g_vlt_rt18", "bms_g_vlt_rt19", "bms_g_vlt_rt20"};
static char *lcu_id_string[] = {"lcu_soft_ver", "lcu_hard_ver", "lcu_sn", "lcu_state", "lcu_switch_state", "lcu_light_state", "lcu_x_acc", "lcu_y_acc", "lcu_z_acc", "lcu_x_ang", "lcu_y_ang", "lcu_z_ang", "lcu_car_state"};
static char *db_id_string[] = {"db_status", "db_gears", "db_speed", "db_mileage", "db_c_cur", "db_dc_cur", "db_bat_soc", "db_full_c_t", "db_hour", "db_min", "db_f_code", "db_bat2_soc", "db_status2", "db_total_soc", "db_rgb_set", "db_rgb_config", "db_soft_ver", "db_hard_ver", "db_sn"};
static char *alarm_id_string[] = {"alarm_soft_ver", "alarm_hard_ver", "alarm_sn", "alarm_state", "alarm_control_cmd"};
static char *lock_id_string[] = {"lock_board_hard_ver", "lock_board_soft_ver", "lock_board_SN", "lock_board_cfg", "lock_board_status", "lock_board_alarm_state", "lock_board_lock1_state", "lock_board_lock2_state", "lock_board_lock3_state", "lock_board_helmet_state", "lock_board_alarm_cmd", "lock_board_lock1_cmd", "lock_board_lock2_cmd", "lock_board_lock3_cmd", "lock_board_batterysta", "lock_board_powersta", "lock_board_lock1cur", "lock_board_lock2cur", "lock_board_lock3cur", "lock_board_vlocksta", "lock_board_vin_status", "lock_board_vlock_status", "lock_board_vbat_status", "lock_board_vdd_status", "lock_board_lock1_timeout", "lock_board_lock2_timeout", "lock_board_lock3_timeout"};
static char *bms2_id_string[] = {"bms2_User", "bms2_id", "bms2_s_ver", "bms2_h_ver", "bms2_p_pwrd", "bms2_bat_type", "bms2_sn_id", "bms2_oc_vlt_p", "bms2_roc_vlt", "bms2_dc_vlt_p", "bms2_rdc_vlt_p", "bms2_c_b_vlt", "bms2_c_cur_p", "bms2_dc_cur_p", "bms2_isc_p", "bms2_dcotp_p", "bms2_rdcotp_p", "bms2_cotp_p", "bms2_rcotp_p", "bms2_dcltp_p", "bms2_rdcltp_p", "bms2_ncom_dc_en", "bms2_rated_vlt", "bms2_bat_to_cap", "bms2_c_cont", "bms2_to_vlt_rt", "bms2_c_cur_rt", "bms2_dc_cur_rt", "bms2_soc_rt", "bms2_bat_sta_rt", "bms2_dc_fl_t_rt", "bms2_tp1_rt", "bms2_tp2_rt", "bms2_tp3_rt", "bms2_tp4_rt", "bms2_tp5_rt", "bms2_g_vlt_rt1", "bms2_g_vlt_rt2", "bms2_g_vlt_rt3", "bms2_g_vlt_rt4", "bms2_g_vlt_rt5", "bms2_g_vlt_rt6", "bms2_g_vlt_rt7", "bms2_g_vlt_rt8", "bms2_g_vlt_rt9", "bms2_g_vlt_rt10", "bms2_g_vlt_rt11", "bms2_g_vlt_rt12", "bms2_g_vlt_rt13", "bms2_g_vlt_rt14", "bms2_g_vlt_rt15", "bms2_g_vlt_rt16", "bms2_g_vlt_rt17", "bms2_g_vlt_rt18", "bms2_g_vlt_rt19", "bms2_g_vlt_rt20"};
static char *bcs_id_string[] = {"bcs_user", "bcs_hard_id", "bcs_s_ver", "bcs_h_ver", "bcs_p_pwrd", "bcs_sn_id", "bcs_charge_cur_p", "bcs_discharge_cur_p", "bcs_cur_cut_state", "bcs_charge_vol_p", "bcs_discharge_vol_p", "bcs_cmp_vol", "bcs_time_charge_cur_p", "bcs_time_discharge_cur_p", "bcs_temp_het_in_p", "bcs_temp_cold_in_p", "bcs_temp_het_out_p", "bcs_temp_cold_out_p", "bcs_bat_to_cap", "bcs_state", "bcs_cmd_sel_bt", "bcs_bt1_mos_temp", "bcs_bt2_mos_temp", "bcs_bcs_mos_temp", "bcs_charge_cur", "bcs_discharge_cur", "bcs_soc_rt", "bcs_dc_fl_t_rt", "bcs_to_charger_cur", "bcs_to_charger_vol", "bcs_bms1_soc", "bcs_bms2_soc", "bcs_bcs_output_vlt", "bcs_bms1_input_vlt", "bcs_bms2_input_vlt"};
static char *foc_id_string[] = {"reserved0", "foc_id", "foc_mode", "foc_sn", "foc_version", "foc_ratedvlt", "foc_minvlt", "foc_maxvlt", "foc_gears", "foc_ratedcur", "foc_barcur", "foc_motorspeed", "foc_cotlsta", "foc_ctrlsta2", "foc_acc_mode_stat", "reserved2", "foc_bat_soft_uv_en", "foc_bat_soft_uv_vlt", "foc_max_phase_cur", "foc_min_phase_cur", "foc_avg_phase_cur", "foc_throttle_mode_set", "foc_foc_accele", "foc_foc_dccele", "foc_ebs_en", "foc_ebs_re_vlt", "foc_ebs_re_cur", "foc_high_speed_ratio", "foc_mid_speed_ratio", "foc_low_speed_ratio", "foc_high_ger_cur_limt_ratio", "foc_mid_ger_cur_limt_ratio", "foc_low_ger_cur_limt_ratio", "foc_flux_weaken_en", "foc_enter_flux_speed_ratio", "foc_exit_flux_speed_ratio", "foc_flux_w_max_cur", "foc_enter_flux_throttle_vlt", "foc_boost_en", "foc_boost_mode", "foc_boost_cur", "foc_boost_keep_time", "foc_boost_interval_time", "foc_start_auto_en", "foc_start_gear", "foc_start_keep_time"};
static char *qc_id_string[] = {"qc_led_state", "qc_fault1", "qc_fault2", "qc_voltage1_adc", "qc_voltage1_100mv", "qc_voltage2_adc", "qc_voltage2_100mv", "qc_current_adc", "qc_current_100ma", "qc_reference_adc", "qc_temperature_adc", "qc_temperature_01degree", "qc_pfc_in", "qc_work_mode", "qc_work_strategy", "qc_work_phases", "qc_en_out", "qc_mos_out", "qc_goalvoltage_100mv", "qc_voltagepwm_0001percent", "qc_goalcurrent_100ma", "qc_currentpwm_0001percent", "qc_charge_voltage_p_100mv", "qc_charge_current_p_100ma", "reserved0", "reserved1", "reserved2", "reserved3", "reserved4", "reserved5", "reserved6", "reserved7", "reserved8", "reserved9", "reserved10", "reserved11", "reserved12", "reserved13", "reserved14", "reserved15", "reserved16", "reserved17", "reserved18", "qc_software_version", "qc_hardware_version", "qc_frequencygrade_10", "qc_temperature_p_01degree", "qc_temperature_r_01degree", "qc_outvoltage_adj", "qc_outcurrent_adj", "qc_involtage_adj", "qc_incurrent_adj", "qc_voltagepwm_p_001percent", "qc_voltagepwm_i_001percent", "qc_voltagepwm_d_001percent", "qc_voltage_p_100mv", "qc_currentpwm_p_001percent", "qc_currentpwm_i_001percent", "qc_currentpwm_d_001percent", "qc_current_p_100ma", "reserved19", "reserved20", "reserved21", "reserved22", "reserved23", "reserved24", "reserved25", "reserved26", "reserved27", "reserved28", "reserved29", "reserved30", "reserved31", "reserved32", "reserved33", "qc_charge_sn_code"};
static char *cpm_id_string[] = {"cpm_dev_name", "cpm_dev_sw_verno", "cpm_dev_hw_verno", "cpm_dev_space", "cpm_dev_free_space", "cpm_dev_model", "cpm_dev_sn", "cpm_dev_mac", "cpm_settings_ble_state", "cpm_settings_gps_state", "cpm_settings_bike_mode", "cpm_settings_bike_state", "cpm_settings_brightness", "cpm_settings_backlight", "cpm_settings_colormode", "cpm_settings_alert_volume", "cpm_settings_key_volume", "cpm_settings_data_config", "cpm_settings_auto_poweroff", "cpm_settings_time_format", "cpm_settings_language", "cpm_settings_mileage_unit", "cpm_settings_temperature_unit", "cpm_settings_weight_unit", "cpm_settings_bound_bsc_id", "cpm_settings_bound_bs_id", "cpm_settings_bound_bc_id", "cpm_settings_bound_hrm_id", "cpm_settings_bound_bpwr_id", "cpm_user_name", "cpm_user_sex", "cpm_user_height", "cpm_user_weight", "cpm_user_birthday", "cpm_user_cur_id", "cpm_user_count", "cpm_bike_cur_id", "cpm_bike_count", "cpm_bike_name", "cpm_bike_type", "cpm_bike_suit", "cpm_bike_display", "cpm_bike_frame_size", "cpm_bike_wheelsize", "cpm_bike_handlelength", "cpm_bike_weight", "cpm_bike_mileage", "cpm_cycling_index", "cpm_cycling_start_time", "cpm_cycling_stop_time", "cpm_cycling_latitude", "cpm_cycling_longitude", "cpm_cycling_mileage", "cpm_cycling_duration", "cpm_cycling_v_max", "cpm_cycling_v_ave", "cpm_cycling_v", "cpm_cycling_rpm_max", "cpm_cycling_rpm_ave", "cpm_cycling_rpm", "cpm_cycling_hrm_max", "cpm_cycling_hrm_ave", "cpm_cycling_hrm", "cpm_cycling_bpwr_max", "cpm_cycling_bpwr_ave", "cpm_cycling_bpwr", "cpm_cycling_elevation", "cpm_cycling_rising_distance", "cpm_cycling_decline_distance", "cpm_cycling_max_left_angle", "cpm_cycling_max_right_angle", "cpm_cycling_kcal", "cpm_cycling_temperature", "cpm_cycling_als", "cpm_cycling_pressure", "cpm_cycling_sub_index", "cpm_cycling_sub_start_time", "cpm_cycling_sub_end_time", "cpm_cycling_sub_mileage", "cpm_cycling_sub_v_ave", "cpm_record_start_time", "cpm_record_max_mileage", "cpm_record_max_duration", "cpm_record_fastest_10km_duration", "cpm_record_max_v", "cpm_record_max_rpm", "cpm_record_max_hrm", "cpm_record_max_bpwr", "cpm_record_max_angle", "cpm_record_max_rising_distance", "cpm_record_max_decline_distance", "cpm_record_max_kcal", "cpm_cumulative_start_time", "cpm_cumulative_total_cycling_counts", "cpm_cumulative_total_mileage", "cpm_cumulative_total_duration", "cpm_cumulative_total_rising_distance", "cpm_cumulative_total_decline_distance", "cpm_cumulative_total_kcal", "cpm_timestamp", "cpm_b9watch_server_cmd", "cpm_riding_id", "cpm_state"};

JVOID niu_value_trace(char *name, NIU_DATA_VALUE *value)
{
    if (value)
    {
        switch (value->type)
        {
        case NIU_INT8:
        {
            if (value->len == 1)
            {
                LOG_D("%s: %d ", name, *(NINT8 *)value->addr);
            }
            else
            {
                niu_trace_dump_data(name, value->addr, value->len);
            }
        }
        break;
        case NIU_UINT8:
        {
            if (value->len == 1)
            {
                LOG_D("%s: %d ", name, *(NUINT8 *)value->addr);
            }
            else
            {
                niu_trace_dump_data(name, value->addr, value->len);
            }
        }
        break;
        case NIU_INT16:
        {
            if (value->len == 2)
            {
                LOG_D("%s: %d ", name, *(NINT16 *)value->addr);
            }
            else
            {
                niu_trace_dump_data(name, value->addr, value->len);
            }
        }
        break;
        case NIU_UINT16:
        {
            if (value->len == 2)
            {
                LOG_D("%s: %d ", name, *(NUINT16 *)value->addr);
            }
            else
            {
                niu_trace_dump_data(name, value->addr, value->len);
            }
        }
        break;
        case NIU_INT32:
        {
            if (value->len == 4)
            {
                LOG_D("%s: %d ", name, *(NINT32 *)value->addr);
            }
            else
            {
                niu_trace_dump_data(name, value->addr, value->len);
            }
        }
        break;
        case NIU_UINT32:
        {
            if (value->len == 4)
            {
                LOG_D("%s: %d ", name, *(NUINT32 *)value->addr);
            }
            else
            {
                niu_trace_dump_data(name, value->addr, value->len);
            }
        }
        break;
        case NIU_STRING:
        {
            NUINT8 print_buf[256] = {0};
            memcpy(print_buf, value->addr, value->len);
            LOG_D("%s: %s ", name, print_buf);
        }
        break;
        case NIU_FLOAT:
        {
            LOG_D("%s: %f ", name, *(NFLOAT *)value->addr);
        }
        break;
        case NIU_DOUBLE:
        {
            LOG_D("%s: %f ", name, *(NDOUBLE *)value->addr);
        }
        break;
        case NIU_ARRAY:
        {
            LOG_D("%s: %s ", name, (char *)value->addr);
        }
        break;
        default:
            break;
        }
    }
    return;
}

JVOID niu_item_trace(NIU_DEVICE_TYPE base, NUINT32 id)
{
    NIU_DATA_VALUE *value = NULL;
    switch(base)
    {
        case NIU_DB:
        {
            if(ARRAY_SIZE(db_id_string) != NIU_ID_DB_MAX)
            {
                LOG_D("db_id_string items count error.[%d,%d]", ARRAY_SIZE(db_id_string), NIU_ID_DB_MAX);
                break;
            }
            if(id < NIU_ID_DB_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(db_id_string[id], value);
                }
            }
        }
        break;
        case NIU_FOC:
        {
            if(ARRAY_SIZE(foc_id_string) != NIU_ID_FOC_MAX)
            {
                LOG_D("foc_id_string items count error.");
                break;
            }
            if(id < NIU_ID_FOC_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(foc_id_string[id], value);
                }
            }
        }
        break;
        case NIU_LOCKBOARD:
        {
            if(ARRAY_SIZE(lock_id_string) != NIU_ID_LOCKBOARD_MAX)
            {
                LOG_D("lock_id_string items count error.");
                break;
            }
            if(id < NIU_ID_LOCKBOARD_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(lock_id_string[id], value);
                }
            }
        }
        break;
        case NIU_BMS:
        {
            if(ARRAY_SIZE(bms_id_string) != NIU_ID_BMS_MAX)
            {
                LOG_D("bms_id_string items count error.");
                break;
            }
            if(id < NIU_ID_BMS_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(bms_id_string[id], value);
                }
            }
        }
        break;
        case NIU_ECU:
        {
            if(ARRAY_SIZE(ecu_id_string) != NIU_ID_ECU_MAX)
            {
                LOG_D("db_id_string items count error.[%d,%d]", ARRAY_SIZE(ecu_id_string), NIU_ID_ECU_MAX);
                break;
            }
            if(id < NIU_ID_ECU_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(ecu_id_string[id], value);
                }
            }
        }
        break;
        case NIU_ALARM:
        {
            if(ARRAY_SIZE(alarm_id_string) != NIU_ID_ALARM_MAX)
            {
                LOG_D("alarm_id_string items count error.");
                break;
            }
            if(id < NIU_ID_ALARM_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(alarm_id_string[id], value);
                }
            }
        }
        break;
        case NIU_LCU:
        {
            if(ARRAY_SIZE(lcu_id_string) != NIU_ID_LCU_MAX)
            {
                LOG_D("lcu_id_string items count error.");
                break;
            }
            if(id < NIU_ID_LCU_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(lcu_id_string[id], value);
                }
            }
        }
        break;
        case NIU_BCS:
        {
            if(ARRAY_SIZE(bcs_id_string) != NIU_ID_BCS_MAX)
            {
                LOG_D("bcs_id_string items count error.");
                break;
            }
            if(id < NIU_ID_BCS_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(bcs_id_string[id], value);
                }
            }
        }
        break;
        case NIU_BMS2:
        {
            if(ARRAY_SIZE(bms2_id_string) != NIU_ID_BMS2_MAX)
            {
                LOG_D("bms2_id_string items count error.");
                break;
            }
            if(id < NIU_ID_BMS2_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(bms2_id_string[id], value);
                }
            }
        }
        break;
        case NIU_QC:
        {
            if(ARRAY_SIZE(qc_id_string) != NIU_ID_QC_MAX)
            {
                LOG_D("qc_id_string items count error.");
                break;
            }
            if(id < NIU_ID_QC_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(qc_id_string[id], value);
                }
            }
        }
        break;
        case NIU_EMCU:
        {
            if(ARRAY_SIZE(emcu_id_string) != NIU_ID_EMCU_MAX)
            {
                LOG_D("emcu_id_string items count error.");
                break;
            }
            if(id < NIU_ID_EMCU_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(emcu_id_string[id], value);
                }
            }
        }
        break;
        case NIU_CPM:
        {
            if(ARRAY_SIZE(cpm_id_string) != NIU_ID_CPM_MAX)
            {
                LOG_D("cpm_id_string items count error.");
                break;
            }
            if(id < NIU_ID_CPM_MAX)
            {
                value = NIU_DATA(base, id);
                if(value)
                {
                    niu_value_trace(cpm_id_string[id], value);
                }
            }
        }
        break;
        default:
            break;
    }
    return;
}

JVOID niu_device_trace(NIU_DEVICE_TYPE base)
{
    int i, max_id = 0;

    if(base == NIU_DB)
    {
        max_id = NIU_ID_DB_MAX;
    }
    else if(base == NIU_FOC)
    {
        max_id = NIU_ID_FOC_MAX;
    }
    else if(base == NIU_LOCKBOARD)
    {
        max_id = NIU_ID_LOCKBOARD_MAX;
    }
    else if(base == NIU_BMS)
    {
        max_id = NIU_ID_BMS_MAX;
    }
    else if(base == NIU_ECU)
    {
        max_id = NIU_ID_ECU_MAX;
    }
    else if(base == NIU_ALARM)
    {
        max_id = NIU_ID_ALARM_MAX;
    }
    else if(base == NIU_LCU)
    {
        max_id = NIU_ID_LCU_MAX;
    }
    else if(base == NIU_BCS)
    {
        max_id = NIU_ID_BCS_MAX;
    }
    else if(base == NIU_BMS2)
    {
        max_id = NIU_ID_BMS2_MAX;
    }
    else if(base == NIU_QC)
    {
        max_id = NIU_ID_QC_MAX;
    }
    else if(base == NIU_EMCU)
    {
        max_id = NIU_ID_EMCU_MAX;
    }
    else if(base == NIU_CPM)
    {
        max_id = NIU_ID_CPM_MAX;
    }

    for(i = 0; i < max_id; i++)
    {
        niu_item_trace(base, i);
    }
    return;
}

JVOID niu_all_trace()
{
    niu_device_trace(NIU_DB);
    niu_device_trace(NIU_FOC);
    niu_device_trace(NIU_LOCKBOARD);
    niu_device_trace(NIU_BMS);
    niu_device_trace(NIU_ECU);
    niu_device_trace(NIU_ALARM);
    niu_device_trace(NIU_LCU);
    niu_device_trace(NIU_BCS);
    niu_device_trace(NIU_BMS2);
    niu_device_trace(NIU_QC);
    niu_device_trace(NIU_EMCU);
    niu_device_trace(NIU_CPM);
}
