#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ql_oe.h>
#include <ql_powerdown.h>
#include "niu_data.h"
#include "niu_data_trace.h"
#include "mcu_data.h"
#include "sys_core.h"
#include "niu_device.h"
#include "niu_thread.h"
#include "niu_entry.h"
#include "niu_work_mode.h"
#include "utils.h"
#include "niu_fota.h"
#include "upload_error_lbs.h"
#include "niu_smt.h"

extern int g_gps_upload_flag;
extern int g_log_upload_flag;
static int g_flag_config_fields_sync = 0;
static int g_flag_config_fields_sync_mask = 0;
extern NUINT32 g_fota_method;

typedef struct _niu_data_body_ecu_
{
    NCHAR server_url[32];                //服务器域名
    NUINT32 server_port;                 //服务器端口
    NCHAR ecu_serial_number[16];         //中控序列号
    NCHAR ecu_soft_ver[8];               //中控软件版本
    NCHAR ecu_hard_ver[8];               //中控硬件版本
    NCHAR imei[16];                      //IMEI
    NCHAR iccid[21];                     //ICCID
    NCHAR imsi[16];                      //IMSI
    NCHAR eid[16];                       //eid
    NUINT16 moved_length;                //异动判断距离
    NCHAR dealer_code[4];                //经销商编码
    NINT32 utc;                          //时区
    NUINT8 car_type;                     //车型编号
    NFLOAT car_speed_index;              //仪表速度系数
    NFLOAT tize_size;                    //轮胎周长
    NUINT8 speed_loc_soc;                //低电量限速开始电量
    NUINT32 lost_enery;                  //当日消耗电量wh
    NUINT32 lost_soc;                    //当日消耗电量
    NUINT32 last_mileage;                //上次骑行里程
    NUINT32 total_mileage;               //总行驶里程
    NUINT8 charge_value;                 //充电开启电压
    NUINT8 discharge_value;              //结束充电电压
    NUINT8 shaking_value;                //震动检测阀值
    NUINT16 ecu_cfg;                     //中控配置
    NUINT16 ecu_cmd;                     //中控命令
    NUINT16 bms_cmd;                     //BMS命令
    NUINT16 foc_cmd;                     //控制器命令
    NUINT16 db_cmd;                      //仪表命令
    NUINT16 lcu_cmd;                     //灯控命令
    NUINT32 alarm_cmd;                   //报警器命令
    NUINT32 display_random_code;         //仪表显示随机码
    NUINT16 uploading_template_1;         //包组合编号1
    NUINT16 uploading_template_2;         //包组合编号2
    NUINT16 uploading_template_3;         //包组合编号3
    NUINT16 uploading_template_4;         //包组合编号4
    NUINT16 uploading_template_5;         //包组合编号5
    NUINT16 uploading_template_6;         //包组合编号6
    NUINT16 uploading_template_7;         //包组合编号7
    NUINT16 uploading_template_8;         //包组合编号8
    NUINT16 uploading_template_9;         //包组合编号9
    NUINT16 uploading_template_10;        //包组合编号10
    NUINT16 uploading_template_11;        //包组合编号11
    NUINT16 uploading_template_12;        //包组合编号12
    NUINT16 uploading_template_13;        //包组合编号13
    NUINT16 uploading_template_14;        //包组合编号14
    NUINT16 uploading_template_15;        //包组合编号15
    NUINT16 uploading_template_16;        //包组合编号16
    NUINT16 uploading_template_17;        //包组合编号17
    NUINT16 uploading_template_18;        //包组合编号18
    NUINT16 uploading_template_19;        //包组合编号19
    NUINT16 uploading_template_20;        //包组合编号20
    NDOUBLE longitude;                   //经度
    NDOUBLE latitude;                    //纬度
    NFLOAT hdop;                         //定位精度
    NFLOAT phop;                         //定位精度
    NFLOAT heading;                      //航向
    NFLOAT gps_speed;                    //GPS速度速度
    NUINT16 ecu_battary;                 //中控电池电量
    NCHAR now_bts[16];                   //当前基站信息
    NCHAR near_bts1[16];                 //周边基站信息
    NCHAR near_bts2[16];                 //周边基站信息
    NCHAR near_bts3[16];                 //周边基站信息
    NCHAR near_bts4[16];                 //周边基站信息
    NUINT32 ap_ip;                       //接入点IP
    NUINT32 car_no_gps_signal_time;      //没有GPS定位计时
    NUINT32 car_no_gsm_signal_time;      //没有GSM信号计时
    NUINT32 car_sleep_time;              //车辆休眠了多少时间
    NINT16 x_acc;                        //X轴加速度
    NINT16 y_acc;                        //Y轴加速度
    NINT16 z_acc;                        //Z轴加速度
    NINT16 x_ang;                        //X轴角加速度
    NINT16 y_ang;                        //Y轴角加速度
    NINT16 z_ang;                        //Z轴角加速度
    NINT16 x_mag;                        //X轴磁力值
    NINT16 y_mag;                        //Y轴磁力值
    NINT16 z_mag;                        //Z轴磁力值
    NUINT32 air_pressure;                //气压
    NUINT32 ecu_state;                   //中控状态
    NUINT32 car_sta;                     //机车状态
    NUINT32 timestamp;                   //时间戳
    NUINT8 gps_singal;                   //GPS信号强度
    NUINT8 csq;                          //GSM信号强度
    NUINT32 car_running_time;            //车辆行驶了多少时间
    NUINT32 car_stoping_time;            //车辆停车了多少时间
    NUINT8 ota_exec_eqp;                 //固件升级设备
    NUINT8 ota_rate;                     //固件升级设备进度
    NCHAR ota_bin_url[96];               //固件URL
    NCHAR ota_soft_ver[8];               //固件升级软件版本
    NCHAR ota_hard_ver[8];               //固件升级硬件版本
    NUINT32 ota_pack_size;               //固件大小
    NUINT32 ota_pack_num;                //固件分包数量
    NUINT8 ota_eqp;                      //要升级的设备
    NDOUBLE stop_longitude;              //停车时经度
    NDOUBLE stop_latitude;               //停车时纬度
    NUINT16 db_code_dis_time;            //仪表随机码显示时间
    NUINT16 cell_mcc;                    //移动国家号
    NUINT16 cell_mnc;                    //移动网络号码
    NUINT16 cell_lac_1;                  //位置区码?
    NUINT32 cell_cid_1;                  //小区编号
    NUINT16 cell_rssi_1;                 //信号强度
    NUINT16 cell_lac_2;                  //位置区码?
    NUINT32 cell_cid_2;                  //小区编号
    NUINT16 cell_rssi_2;                 //信号强度
    NUINT16 cell_lac_3;                  //位置区码?
    NUINT32 cell_cid_3;                  //小区编号
    NUINT16 cell_rssi_3;                 //信号强度
    NUINT16 cell_lac_4;                  //位置区码?
    NUINT32 cell_cid_4;                  //小区编号
    NUINT16 cell_rssi_4;                 //信号强度
    NUINT16 cell_lac_5;                  //位置区码?
    NUINT32 cell_cid_5;                  //小区编号
    NUINT16 cell_rssi_5;                 //信号强度
    NUINT16 pole_pairs;                  //极对数
    NUINT16 radius;                      //速度校准配置
    NFLOAT calu_speed_index;             //速度计算系数
    NUINT8 wifi1_bssid_1;                //wifi1_bssid_1
    NUINT8 wifi1_bssid_2;                //wifi1_bssid_2
    NUINT8 wifi1_bssid_3;                //wifi1_bssid_3
    NUINT8 wifi1_bssid_4;                //wifi1_bssid_4
    NUINT8 wifi1_bssid_5;                //wifi1_bssid_5
    NUINT8 wifi1_bssid_6;                //wifi1_bssid_6
    NUINT8 wifi1_rssi;                   //wifi1_rssi
    NUINT8 wifi2_bssid_1;                //wifi2_bssid_1
    NUINT8 wifi2_bssid_2;                //wifi2_bssid_2
    NUINT8 wifi2_bssid_3;                //wifi2_bssid_3
    NUINT8 wifi2_bssid_4;                //wifi2_bssid_4
    NUINT8 wifi2_bssid_5;                //wifi2_bssid_5
    NUINT8 wifi2_bssid_6;                //wifi2_bssid_6
    NUINT8 wifi2_rssi;                   //wifi2_rssi
    NUINT8 wifi3_bssid_1;                //wifi3_bssid_1
    NUINT8 wifi3_bssid_2;                //wifi3_bssid_2
    NUINT8 wifi3_bssid_3;                //wifi3_bssid_3
    NUINT8 wifi3_bssid_4;                //wifi3_bssid_4
    NUINT8 wifi3_bssid_5;                //wifi3_bssid_5
    NUINT8 wifi3_bssid_6;                //wifi3_bssid_6
    NUINT8 wifi3_rssi;                   //wifi3_rssi
    NUINT8 wifi4_bssid_1;                //wifi4_bssid_1
    NUINT8 wifi4_bssid_2;                //wifi4_bssid_2
    NUINT8 wifi4_bssid_3;                //wifi4_bssid_3
    NUINT8 wifi4_bssid_4;                //wifi4_bssid_4
    NUINT8 wifi4_bssid_5;                //wifi4_bssid_5
    NUINT8 wifi4_bssid_6;                //wifi4_bssid_6
    NUINT8 wifi4_rssi;                   //wifi4_rssi
    NUINT8 wifi5_bssid_1;                //wifi5_bssid_1
    NUINT8 wifi5_bssid_2;                //wifi5_bssid_2
    NUINT8 wifi5_bssid_3;                //wifi5_bssid_3
    NUINT8 wifi5_bssid_4;                //wifi5_bssid_4
    NUINT8 wifi5_bssid_5;                //wifi5_bssid_5
    NUINT8 wifi5_bssid_6;                //wifi5_bssid_6
    NUINT8 wifi5_rssi;                   //wifi5_rssi
    NUINT16 smt_test;                    //smt_test
    NUINT32 app_control;                 //服务器控制字段
    NUINT8 fota_download_percent;        //FOTA进度
    NUINT32 car_eqp_cfg;                 //车内设备列表
    NUINT32 car_eqp_state;               //车内可读状态
    NUINT16 fail_value;                  //倾倒检测阈值
    NFLOAT fall_pitch;                   //校正后pitch
    NFLOAT fall_roll;                    //校正后roll
    NUINT32 ecu_state2;                  //中控状态2
    NCHAR ecu_adups_ver[48];             //FOTA版本号
    NUINT8 move_gsensor_count;           //检测到震动次数
    NUINT8 move_gsensor_time;            //检测时间
    NUINT16 pdu_message_current_number;   //实时缓存报文数量
    NUINT8 max_snr_1;                    //最大GPS信号值
    NUINT8 max_snr_2;                    //最大GPS信号值
    NUINT8 eye_settings;                 //天眼配置值
    NCHAR imei_4g[16];                   //IMEI
    NINT8 temperature_4g;               //4G模块温度
    NUINT16 ecu_velocity;                //hall转速
    NCHAR ble_mac[8];                    //蓝牙mac地址
    NCHAR ble_password[16];              //蓝牙密码
    NCHAR ble_aes[16];                   //蓝牙秘钥
    NCHAR ble_name[32];                  //蓝牙名称
    NUINT16 smt_test2;                   //SMT测试结果
    NUINT32 limiting_velocity_max;       //进入双限速转速
    NUINT32 limiting_velocity_min;       //退出双限速转速
    NFLOAT front_tire_size;              //前轮周长
    NCHAR server_url2[32];               //备用服务器域名
    NUINT32 server_port2;                //备用服务器端口
    NCHAR server_error_url[32];          //err上报服务器域名
    NUINT32 server_error_port;           //err上报服务器端口
    NCHAR server_error_url2[32];         //err上报备用服务器域名
    NUINT32 server_error_port2;          //err上报备用服务器端口
    NCHAR server_lbs_url[32];            //LBS服务器域名
    NUINT32 server_lbs_port;             //LBS服务器端口
    NCHAR server_lbs_url2[32];           //LBS备用服务器域名
    NUINT32 server_lbs_port2;            //LBS备用服务器端口
    NUINT32 v35_alarm_state;             //V35报警器状态字段
    NUINT32 ecu_keepalive;               //心跳包间隔
    NUINT32 ecu_gps_max_cnr;             //最大GPS信号强度
    NUINT32 ecu_gps_min_cnr;             //最小GPS信号强度
    NUINT32 ecu_gps_avg_cnr;             //平均GPS信号强度
    NUINT32 ecu_gps_seill_num;           //定位卫星数量
    NCHAR ecu_no_cdma_location[255];      //非cdma时基站信息
    NCHAR ecu_cdma_location[255];         //cdma时基站信息
    NUINT32 ecu_gps_sleep_time;          //电门关闭时，gps休眠间隔
    NUINT32 ecu_gps_awake_time;          //电门关闭时，gps自动唤醒时长
    NDOUBLE ecu_avg_longitude;            //电门关闭时GPS平均经度
    NDOUBLE ecu_avg_latitude;             //电门关闭时GPS平均纬度
    NUINT32 ecu_v35_config;              //V35报警器配置
    NFLOAT ecu_pitch;                    //俯仰角
    NFLOAT ecu_roll;                     //翻滚角
    NFLOAT ecu_yaw;                      //俯仰角
    NUINT32 ecu_server_error_code;       //最近一次断网原因
    NUINT32 ecu_server_error_code_time;  //最近一次断网时间戳
    NUINT32 ecu_server_connected_time;   //最近一次中控联网登录成功时间戳
    NUINT32 ecu_server_ka_timeout_time;  //最近一次中控检测KA_TIMEOUT时间戳
    NUINT32 ecu_server_reset_4g_time;    //最近中控复位4G模块时间戳
    NUINT32 ecu_server_do_connect_count; //socket connect操作的次数，即几次连上
    NUINT32 ecu_server_reset_4g_count;   //中控复位4G模块的累积次数
    NDOUBLE ecu_realtime_longitude;       //实时经度
    NDOUBLE ecu_realtime_latitude;        //实时纬度
    NUINT32 ecu_app_contorl;//服务器控制字段1
    NUINT32 ecu_app_contorl2;//服务器控制字段2
    NUINT32 ecu_app_contorl3;//服务器控制字段3
    NCHAR ecu_app_contorl10[16];//服务器控制字段4
    NCHAR ecu_app_contorl11[16];//服务器控制字段5
    NCHAR ecu_app_contorl12[16];//服务器控制字段6
    NCHAR ecu_cmd_id[32];//命令ID
    NUINT32 ecu_cmd_result;//命令结果
    NUINT32 ecu_cmd_expiry_timestamp;//命令失效时间
    NUINT32 ecu_cmd_receive_timestamp;//命令接收时间
    NUINT32 ecu_cmd_exec_timestamp;//命令执行完毕时间
    NUINT32 ecu_auto_close_time;//自动关机时间
    NCHAR ecu_ex_model_version[32];//外置模块版本
    NCHAR ecu_gps_version[32]; //GPS模板版本
    NUINT16 uploading_template_21;         //包组合编号21
    NUINT16 uploading_template_22;         //包组合编号22
    NUINT16 uploading_template_23;         //包组合编号23
    NUINT16 uploading_template_24;         //包组合编号24
    NUINT16 uploading_template_25;         //包组合编号25
    NUINT16 uploading_template_26;         //包组合编号26
    NUINT16 uploading_template_27;         //包组合编号27
    NUINT16 uploading_template_28;         //包组合编号28
    NUINT16 uploading_template_29;         //包组合编号29
    NUINT16 uploading_template_30;         //包组合编号30
    NUINT16 uploading_template_31;         //包组合编号31
    NUINT16 uploading_template_32;         //包组合编号32
    NUINT16 uploading_template_33;         //包组合编号33
    NUINT16 uploading_template_34;         //包组合编号34
    NUINT16 uploading_template_35;         //包组合编号35
    NUINT16 uploading_template_36;         //包组合编号36
    NUINT16 uploading_template_37;         //包组合编号37
    NUINT16 uploading_template_38;         //包组合编号38
    NUINT16 uploading_template_39;         //包组合编号39
    NUINT16 uploading_template_40;         //包组合编号40
    NUINT16 uploading_template_41;         //包组合编号41
    NUINT16 uploading_template_42;         //包组合编号42
    NUINT16 uploading_template_43;         //包组合编号43
    NUINT16 uploading_template_44;         //包组合编号44
    NUINT16 uploading_template_45;         //包组合编号45
    NUINT16 uploading_template_46;         //包组合编号46
    NUINT16 uploading_template_47;         //包组合编号47
    NUINT16 uploading_template_48;         //包组合编号48
    NUINT16 uploading_template_49;         //包组合编号49
    NUINT16 uploading_template_50;         //包组合编号50
    NUINT32 ecu_inherit_mileage;           //继承里程
    NUINT32 ecu_reboot_count;              //中控重启计数
    NCHAR   serverUsername[16];                 //username
    NCHAR   serverPassword[16];                 //password
    NCHAR   aesKey[16];                         //aes key
    NCHAR   mqttSubscribe[32];                  //subscribe
    NCHAR   mqttPublish[32];                    //publish

} NIU_DATA_BODY_ECU;

typedef struct _niu_data_body_emcu_
{
    NCHAR emcu_soft_ver[8];     //软件版本
    NCHAR emcu_hard_ver[8];     //硬件版本
    NCHAR emcu_sn[16];          //设备序列号
    NCHAR emcu_compiletime[24]; //编译时间
    NUINT32 emcu_cmd;//emcu指令
    NUINT32 emcu_alarmkeyid;//远程对码
    NUINT32 emcu_smt_test; //SMT测试结果
    NCHAR emcu_remote_group_code[10]; //遥控器组合代码
} NIU_DATA_BODY_EMCU;

typedef struct _niu_data_body_bms_
{
    NUINT8 bms_user;        //用户权限
    NUINT8 bms_bms_id;      //设备ID
    NUINT8 bms_s_ver;       //软件版本
    NUINT8 bms_h_ver;       //硬件版本
    NCHAR bms_p_pwrd[4];   //参数保护密码
    NUINT8 bms_bat_type;    //电芯型号代码
    NCHAR bms_sn_id[16];    //设备序列号
    NUINT8 bms_oc_vlt_p;    //过充保护电压
    NUINT8 bms_roc_vlt;     //过充恢复电压
    NUINT8 bms_dc_vlt_p;    //过放保护电压
    NUINT8 bms_rdc_vlt_p;   //过放恢复电压
    NUINT8 bms_c_b_vlt;     //充电均衡起始电压
    NUINT8 bms_c_cur_p;     //充电保护电流
    NUINT8 bms_dc_cur_p;    //放电保护电流
    NUINT8 bms_isc_p;       //短路保护电流
    NINT8 bms_dcotp_p;     //放电高温保护
    NINT8 bms_rdcotp_p;    //放电恢复高温保护
    NINT8 bms_cotp_p;      //充电高温保护
    NINT8 bms_rcotp_p;     //充电恢复高温保护
    NINT8 bms_dcltp_p;     //放电低温保护
    NINT8 bms_rdcltp_p;    //放电恢复低温保护
    NUINT8 bms_ncom_dc_en;  //额定电压固定
    NUINT8 bms_rated_vlt;   //额定电压
    NUINT16 bms_bat_to_cap; //电池容量
    NUINT16 bms_c_cont;     //电池循环次数
    NUINT16 bms_to_vlt_rt;  //总电压
    NUINT16 bms_c_cur_rt;   //充电电流
    NUINT16 bms_dc_cur_rt;  //放电电流
    NUINT8 bms_soc_rt;      //SOC估算
    NUINT16 bms_bat_sta_rt; //实时电池状态
    NUINT8 bms_dc_fl_t_rt;  //充满电剩余时间
    NINT8 bms_tp1_rt;      //当前温度1
    NINT8 bms_tp2_rt;      //当前温度2
    NINT8 bms_tp3_rt;      //当前温度3
    NINT8 bms_tp4_rt;      //当前温度4
    NINT8 bms_tp5_rt;      //当前温度5
    NUINT16 bms_g_vlt_rt1;  //电池组电压1
    NUINT16 bms_g_vlt_rt2;  //电池组电压2
    NUINT16 bms_g_vlt_rt3;  //电池组电压3
    NUINT16 bms_g_vlt_rt4;  //电池组电压4
    NUINT16 bms_g_vlt_rt5;  //电池组电压5
    NUINT16 bms_g_vlt_rt6;  //电池组电压6
    NUINT16 bms_g_vlt_rt7;  //电池组电压7
    NUINT16 bms_g_vlt_rt8;  //电池组电压8
    NUINT16 bms_g_vlt_rt9;  //电池组电压9
    NUINT16 bms_g_vlt_rt10; //电池组电压10
    NUINT16 bms_g_vlt_rt11; //电池组电压11
    NUINT16 bms_g_vlt_rt12; //电池组电压12
    NUINT16 bms_g_vlt_rt13; //电池组电压13
    NUINT16 bms_g_vlt_rt14; //电池组电压14
    NUINT16 bms_g_vlt_rt15; //电池组电压15
    NUINT16 bms_g_vlt_rt16; //电池组电压16
    NUINT16 bms_g_vlt_rt17; //电池组电压17
    NUINT16 bms_g_vlt_rt18; //电池组电压18
    NUINT16 bms_g_vlt_rt19; //电池组电压19
    NUINT16 bms_g_vlt_rt20; //电池组电压20
} NIU_DATA_BODY_BMS;

typedef struct _niu_data_body_lcu_
{
    NCHAR lcu_soft_ver[8];    //软件版本
    NCHAR lcu_hard_ver[8];    //硬件版本
    NCHAR lcu_sn[16];         //序列号
    NUINT16 lcu_state;        //整机状态
    NUINT16 lcu_switch_state; //组合开关状态
    NUINT16 lcu_light_state;  //灯具状态
    NINT16 lcu_x_acc;         //X轴加速度
    NINT16 lcu_y_acc;         //Y轴加速度
    NINT16 lcu_z_acc;         //Z轴加速度
    NINT16 lcu_x_ang;         //X轴角加速度
    NINT16 lcu_y_ang;         //Y轴角加速度
    NINT16 lcu_z_ang;         //Y轴角加速度
    NUINT16 lcu_car_state;    //车辆状态
} NIU_DATA_BODY_LCU;

typedef struct _niu_data_body_db_
{
    NUINT16 db_status;     //机车状态
    NUINT8 db_gears;       //档位
    NUINT8 db_speed;       //速度
    NUINT32 db_mileage;    //里程数
    NUINT8 db_c_cur;       //充电电流
    NUINT8 db_dc_cur;      //放电电流
    NUINT8 db_bat_soc;     //电池SOC
    NUINT8 db_full_c_t;    //充电剩余时间
    NUINT8 db_hour;        //当前小时
    NUINT8 db_min;         //当前分钟
    NUINT8 db_f_code;      //故障代码
    NUINT8 db_bat2_soc;    //电池2SOC
    NUINT16 db_status2;    //车辆状态2
    NUINT8 db_total_soc;   //总电池SOC
    NCHAR db_rgb_set[10];  //RGB指令
    NUINT32 db_rgb_config; //RGB配色命令
    NCHAR db_soft_ver[8];  //软件版本
    NCHAR db_hard_ver[8];  //硬件版本
    NCHAR db_sn[16];       //序列号
} NIU_DATA_BODY_DB;

typedef struct _niu_data_body_alarm_
{
    NCHAR alarm_soft_ver[8];  //软件版本
    NCHAR alarm_hard_ver[8];  //硬件版本
    NCHAR alarm_sn[16];       //序列号
    NUINT8 alarm_state;       //报警器状态
    NUINT8 alarm_control_cmd; //报警器控制命令
} NIU_DATA_BODY_ALARM;

typedef struct _niu_data_body_lockboard_
{
    NCHAR lock_board_hard_ver[8];     //硬件版本号
    NCHAR lock_board_soft_ver[8];     //型号
    NCHAR lock_board_sn[16];          //序列号
    NUINT32 lock_board_cfg;           //配置表
    NUINT16 lock_board_status;        //锁控状态
    NUINT16 lock_board_alarm_state;   //报警器状态
    NUINT8 lock_board_lock1_state;    //坐垫锁状态
    NUINT8 lock_board_lock2_state;    //尾箱锁状态
    NUINT8 lock_board_lock3_state;    //龙头锁状态
    NUINT8 lock_board_helmet_state;   //头盔状态
    NUINT8 lock_board_alarm_cmd;      //报警器命令
    NUINT8 lock_board_lock1_cmd;      //坐垫锁命令
    NUINT8 lock_board_lock2_cmd;      //尾箱锁命令
    NUINT8 lock_board_lock3_cmd;      //龙头锁命令
    NUINT8 lock_board_batterysta;     //内电池电量
    NCHAR lock_board_powersta[8];     //所有电池状态
    NUINT16 lock_board_lock1cur;      //最后一次开解锁坐垫锁电流
    NUINT16 lock_board_lock2cur;      //最后一次开解锁尾箱锁电流
    NUINT16 lock_board_lock3cur;      //最后一次开解锁龙头锁电流
    NUINT16 lock_board_vlocksta;      //最后一次开解锁锁供电电压
    NUINT16 lock_board_vin_status;    //外电电压
    NUINT16 lock_board_vlock_status;  //锁供电电压
    NUINT16 lock_board_vbat_status;   //内电电压
    NUINT16 lock_board_vdd_status;    //MCU供电电压
    NUINT16 lock_board_lock1_timeout; //坐垫锁开解锁超时时间
    NUINT16 lock_board_lock2_timeout; //尾箱锁开解锁超时时间
    NUINT16 lock_board_lock3_timeout; //龙头锁开解锁超时时间
} NIU_DATA_BODY_LOCKBOARD;

typedef struct _niu_data_body_bms2_
{
    NUINT8 bms2_user;        //用户权限
    NUINT8 bms2_id;          //设备ID
    NUINT8 bms2_s_ver;       //软件版本
    NUINT8 bms2_h_ver;       //硬件版本
    NCHAR bms2_p_pwrd[4];   //参数保护密码
    NUINT8 bms2_bat_type;    //电芯型号代码
    NCHAR bms2_sn_id[16];    //设备序列号
    NUINT8 bms2_oc_vlt_p;    //过充保护电压
    NUINT8 bms2_roc_vlt;     //过充恢复电压
    NUINT8 bms2_dc_vlt_p;    //过放保护电压
    NUINT8 bms2_rdc_vlt_p;   //过放恢复电压
    NUINT8 bms2_c_b_vlt;     //充电均衡起始电压
    NUINT8 bms2_c_cur_p;     //充电保护电流
    NUINT8 bms2_dc_cur_p;    //放电保护电流
    NUINT8 bms2_isc_p;       //短路保护电流
    NINT8 bms2_dcotp_p;     //放电高温保护
    NINT8 bms2_rdcotp_p;    //放电恢复高温保护
    NINT8 bms2_cotp_p;      //充电高温保护
    NINT8 bms2_rcotp_p;     //充电恢复高温保护
    NINT8 bms2_dcltp_p;     //放电低温保护
    NINT8 bms2_rdcltp_p;    //放电恢复低温保护
    NUINT8 bms2_ncom_dc_en;  //额定电压固定
    NUINT8 bms2_rated_vlt;   //额定电压
    NUINT16 bms2_bat_to_cap; //电池容量
    NUINT16 bms2_c_cont;     //电池循环次数
    NUINT16 bms2_to_vlt_rt;  //总电压
    NUINT16 bms2_c_cur_rt;   //充电电流
    NUINT16 bms2_dc_cur_rt;  //放电电流
    NUINT8 bms2_soc_rt;      //SOC估算
    NUINT16 bms2_bat_sta_rt; //实时电池状态
    NUINT8 bms2_dc_fl_t_rt;  //充满电剩余时间
    NINT8 bms2_tp1_rt;      //当前温度1
    NINT8 bms2_tp2_rt;      //当前温度2
    NINT8 bms2_tp3_rt;      //当前温度3
    NINT8 bms2_tp4_rt;      //当前温度4
    NINT8 bms2_tp5_rt;      //当前温度5
    NUINT16 bms2_g_vlt_rt1;  //电池组电压1
    NUINT16 bms2_g_vlt_rt2;  //电池组电压2
    NUINT16 bms2_g_vlt_rt3;  //电池组电压3
    NUINT16 bms2_g_vlt_rt4;  //电池组电压4
    NUINT16 bms2_g_vlt_rt5;  //电池组电压5
    NUINT16 bms2_g_vlt_rt6;  //电池组电压6
    NUINT16 bms2_g_vlt_rt7;  //电池组电压7
    NUINT16 bms2_g_vlt_rt8;  //电池组电压8
    NUINT16 bms2_g_vlt_rt9;  //电池组电压9
    NUINT16 bms2_g_vlt_rt10; //电池组电压10
    NUINT16 bms2_g_vlt_rt11; //电池组电压11
    NUINT16 bms2_g_vlt_rt12; //电池组电压12
    NUINT16 bms2_g_vlt_rt13; //电池组电压13
    NUINT16 bms2_g_vlt_rt14; //电池组电压14
    NUINT16 bms2_g_vlt_rt15; //电池组电压15
    NUINT16 bms2_g_vlt_rt16; //电池组电压16
    NUINT16 bms2_g_vlt_rt17; //电池组电压17
    NUINT16 bms2_g_vlt_rt18; //电池组电压18
    NUINT16 bms2_g_vlt_rt19; //电池组电压19
    NUINT16 bms2_g_vlt_rt20; //电池组电压20
} NIU_DATA_BODY_BMS2;

typedef struct _niu_data_body_bcs_
{
    NUINT8 bcs_user;                 //用户权限
    NUINT8 bcs_hard_id;              //设备ID
    NCHAR bcs_s_ver[8];              //软件版本
    NCHAR bcs_h_ver[8];              //硬件版本
    NCHAR bcs_p_pwrd[4];             //参数保护密码
    NCHAR bcs_sn_id[16];             //设备序列号
    NUINT32 bcs_charge_cur_p;        //充电保护电流
    NUINT32 bcs_discharge_cur_p;     //放电保护电流
    NUINT32 bcs_cur_cut_state;       //切换电流值
    NUINT32 bcs_charge_vol_p;        //充电保护电压
    NUINT32 bcs_discharge_vol_p;     //放电保护电压
    NUINT32 bcs_cmp_vol;             //电压差值设定值
    NUINT8 bcs_time_charge_cur_p;    //充电过流保护恢复时间设定
    NUINT8 bcs_time_discharge_cur_p; //放电过流保护恢复时间设定
    NINT8 bcs_temp_het_in_p;        //高温保护值
    NINT8 bcs_temp_cold_in_p;       //低温保护值
    NINT8 bcs_temp_het_out_p;       //高温保护退出值
    NINT8 bcs_temp_cold_out_p;      //低温保护退出值
    NUINT16 bcs_bat_to_cap;          //电池容量
    NUINT32 bcs_state;               //工作状态
    NUINT8 bcs_cmd_sel_bt;           //电池选择
    NINT8 bcs_bt1_mos_temp;         //一号电池MOS温度
    NINT8 bcs_bt2_mos_temp;         //二号电池MOS温度
    NINT8 bcs_bcs_mos_temp;         //干路MOS温度
    NUINT32 bcs_charge_cur;          //充电电流
    NUINT32 bcs_discharge_cur;       //放电电流
    NUINT8 bcs_soc_rt;               //SOC估算
    NUINT8 bcs_dc_fl_t_rt;           //充满电剩余时间
    NUINT16 bcs_to_charger_cur;      //需要的充电电流
    NUINT32 bcs_to_charger_vol;      //需要的充电压
    NUINT8 bcs_bms1_soc;             //电池1SOC估算
    NUINT8 bcs_bms2_soc;             //电池2SOC估算
    NUINT16 bcs_bcs_output_vlt;      //BCS输出电压
    NUINT16 bcs_bms1_input_vlt;      //电池包1接入电压
    NUINT16 bcs_bms2_input_vlt;      //电池包2接入电压
} NIU_DATA_BODY_BCS;

typedef struct _niu_data_body_foc_
{
    NUINT8 reserved0;                   //reserved
    NUINT8 foc_id;                      //ID号
    NCHAR foc_mode[16];                 //型号
    NCHAR foc_sn[16];                   //序列号
    NUINT8 foc_version;                 //版本号
    NUINT8 foc_ratedvlt;                //额定电压
    NUINT8 foc_minvlt;                  //最低电压
    NUINT8 foc_maxvlt;                  //最高电压
    NUINT8 foc_gears;                   //档位
    NUINT16 foc_ratedcur;               //额定电流
    NINT16 foc_barcur;                 //母线电流
    NINT16 foc_motorspeed;             //电机转速
    NUINT16 foc_cotlsta;                //FOC状态
    NUINT16 foc_ctrlsta2;               //控制器状态2
    NUINT8 foc_acc_mode_stat;           //控制器加速度模式
    NUINT8 reserved2;                   //reserved
    NUINT8 foc_bat_soft_uv_en;          //电池软欠压保护使能
    NUINT8 foc_bat_soft_uv_vlt;         //电池软欠压电压
    NUINT16 foc_max_phase_cur;          //最大相电流
    NUINT16 foc_min_phase_cur;          //最小相电流
    NUINT16 foc_avg_phase_cur;          //平均电流
    NUINT8 foc_throttle_mode_set;       //转把模式选择
    NUINT16 foc_foc_accele;             //加速度
    NUINT16 foc_foc_dccele;             //减速度
    NUINT8 foc_ebs_en;                  //EBS使能
    NUINT8 foc_ebs_re_vlt;              //EBS反充电压
    NUINT16 foc_ebs_re_cur;             //EBS反充电流
    NUINT8 foc_high_speed_ratio;        //高档位速度
    NUINT8 foc_mid_speed_ratio;         //中档位速度
    NUINT8 foc_low_speed_ratio;         //低档位速度
    NUINT8 foc_high_ger_cur_limt_ratio; //高档位限流比例
    NUINT8 foc_mid_ger_cur_limt_ratio;  //中档位限流比例
    NUINT8 foc_low_ger_cur_limt_ratio;  //低档位限流比例
    NUINT8 foc_flux_weaken_en;          //弱磁使能
    NUINT8 foc_enter_flux_speed_ratio;  //弱磁进入速度比例
    NUINT8 foc_exit_flux_speed_ratio;   //弱磁退出速度比例
    NUINT16 foc_flux_w_max_cur;         //弱磁最大电流
    NUINT8 foc_enter_flux_throttle_vlt; //弱磁进入转把电压
    NUINT8 foc_boost_en;                //Boost使能
    NUINT8 foc_boost_mode;              //Boost模式
    NUINT16 foc_boost_cur;              //Boost电流
    NUINT8 foc_boost_keep_time;         //Boost保持时间
    NUINT8 foc_boost_interval_time;     //Boost间隔时间
    NUINT8 foc_start_auto_en;           //暴起模式使能
    NUINT8 foc_start_gear;              //暴起档位
    NUINT8 foc_start_keep_time;         //暴起持续时间
} NIU_DATA_BODY_FOC;

typedef struct _niu_data_body_qc_
{
    NUINT8 qc_led_state;               //充电器指示灯状态
    NUINT32 qc_fault1;                 //充电器故障代码1
    NUINT32 qc_fault2;                 //充电器故障代码2
    NUINT16 qc_voltage1_adc;           //电压1ADC
    NUINT16 qc_voltage1_100mv;         //电压1
    NUINT16 qc_voltage2_adc;           //电压2ADC
    NUINT16 qc_voltage2_100mv;         //电压2
    NUINT16 qc_current_adc;            //电流ADC
    NUINT16 qc_current_100ma;          //电流
    NUINT16 qc_reference_adc;          //参考基准电压ADC
    NUINT16 qc_temperature_adc;        //温度采集ADC
    NINT16 qc_temperature_01degree;    //温度
    NUINT8 qc_pfc_in;                  //PFC状态
    NUINT8 qc_work_mode;               //电源模式
    NUINT8 qc_work_strategy;           //当前执行策略号
    NUINT8 qc_work_phases;             //当前充电阶段
    NUINT8 qc_en_out;                  //使能信号状态
    NUINT8 qc_mos_out;                 //输出MOS状态
    NUINT16 qc_goalvoltage_100mv;      //输出目标电压
    NUINT16 qc_voltagepwm_0001percent; //电压PWM占空比
    NUINT16 qc_goalcurrent_100ma;      //输出目标电流
    NUINT16 qc_currentpwm_0001percent; //电流PWM占空比
    NUINT16 qc_charge_voltage_p_100mv; //充电保护电压
    NUINT16 qc_charge_current_p_100ma; //充电保护电流
    NUINT8 reserved0;                  //reserved
    NUINT8 reserved1;                  //reserved
    NUINT8 reserved2;                  //reserved
    NUINT8 reserved3;                  //reserved
    NUINT8 reserved4;                  //reserved
    NUINT8 reserved5;                  //reserved
    NUINT8 reserved6;                  //reserved
    NUINT8 reserved7;                  //reserved
    NUINT8 reserved8;                  //reserved
    NUINT8 reserved9;                  //reserved
    NUINT8 reserved10;                 //reserved
    NUINT8 reserved11;                 //reserved
    NUINT8 reserved12;                 //reserved
    NUINT8 reserved13;                 //reserved
    NUINT8 reserved14;                 //reserved
    NUINT8 reserved15;                 //reserved
    NUINT8 reserved16;                 //reserved
    NUINT8 reserved17;                 //reserved
    NUINT8 reserved18;                 //reserved
    NCHAR qc_software_version[8];      //软件版本
    NCHAR qc_hardware_version[8];      //硬件版本
    NUINT8 qc_frequencygrade_10;       //PWM输出频率等级
    NINT16 qc_temperature_p_01degree;  //温度保护值
    NINT16 qc_temperature_r_01degree;  //撤销温度保护值
    NUINT16 qc_outvoltage_adj;         //输出电压校准
    NUINT16 qc_outcurrent_adj;         //输出电流校准
    NUINT16 qc_involtage_adj;          //电压采集校准
    NUINT16 qc_incurrent_adj;          //电流采集校准
    NINT16 qc_voltagepwm_p_001percent; //电压控制P
    NINT16 qc_voltagepwm_i_001percent; //电压控制I
    NINT16 qc_voltagepwm_d_001percent; //电压控制D
    NUINT16 qc_voltage_p_100mv;        //电源保护电压
    NINT16 qc_currentpwm_p_001percent; //电流控制P
    NINT16 qc_currentpwm_i_001percent; //电流控制I
    NINT16 qc_currentpwm_d_001percent; //电流控制D
    NUINT16 qc_current_p_100ma;        //电源保护电流
    NUINT8 reserved19;                 //reserved
    NUINT8 reserved20;                 //reserved
    NUINT8 reserved21;                 //reserved
    NUINT8 reserved22;                 //reserved
    NUINT8 reserved23;                 //reserved
    NUINT8 reserved24;                 //reserved
    NUINT8 reserved25;                 //reserved
    NUINT8 reserved26;                 //reserved
    NUINT8 reserved27;                 //reserved
    NUINT8 reserved28;                 //reserved
    NUINT8 reserved29;                 //reserved
    NUINT8 reserved30;                 //reserved
    NUINT8 reserved31;                 //reserved
    NUINT8 reserved32;                 //reserved
    NUINT8 reserved33;                 //reserved
    NCHAR qc_charge_sn_code[16];       //SN
} NIU_DATA_BODY_QC;

typedef struct _niu_data_body_cpm_
{
    NCHAR cpm_dev_name[32];                       //码表名称
    NCHAR cpm_dev_sw_verno[32];                   //码表软件版本
    NCHAR cpm_dev_hw_verno[32];                   //码表硬件版本
    NUINT32 cpm_dev_space;                        //码表总空间
    NUINT32 cpm_dev_free_space;                   //码表剩余空间
    NCHAR cpm_dev_model[32];                      //码表型号
    NCHAR cpm_dev_sn[16];                         //码表序列号
    NCHAR cpm_dev_mac[8];                         //码表MAC地址
    NUINT8 cpm_settings_ble_state;                //BLE状态
    NUINT8 cpm_settings_gps_state;                //GPS状态
    NUINT8 cpm_settings_bike_mode;                //骑行模式
    NUINT16 cpm_settings_bike_state;              //车辆在使用过程中的状态设置
    NUINT8 cpm_settings_brightness;               //亮度
    NUINT8 cpm_settings_backlight;                //背光时间
    NUINT8 cpm_settings_colormode;                //颜色模式
    NUINT8 cpm_settings_alert_volume;             //提示音音量
    NUINT8 cpm_settings_key_volume;               //按键音音量
    NUINT8 cpm_settings_data_config;              //踏频数据记录配置
    NUINT8 cpm_settings_auto_poweroff;            //自动关机
    NUINT8 cpm_settings_time_format;              //时间制式
    NUINT8 cpm_settings_language;                 //语言
    NUINT8 cpm_settings_mileage_unit;             //里程单位
    NUINT8 cpm_settings_temperature_unit;         //温度单位
    NUINT8 cpm_settings_weight_unit;              //重量单位
    NUINT32 cpm_settings_bound_bsc_id;            //绑定踏频速度计ID
    NUINT32 cpm_settings_bound_bs_id;             //绑定速度计ID
    NUINT32 cpm_settings_bound_bc_id;             //绑定踏频计ID
    NUINT32 cpm_settings_bound_hrm_id;            //绑定心率计ID
    NUINT32 cpm_settings_bound_bpwr_id;           //绑定功率计ID
    NCHAR cpm_user_name[32];                      //用户名
    NUINT8 cpm_user_sex;                          //性别
    NUINT8 cpm_user_height;                       //身高
    NFLOAT cpm_user_weight;                       //体重
    NUINT32 cpm_user_birthday;                    //出生日期
    NUINT32 cpm_user_cur_id;                       //当前用户ID
    NUINT8 cpm_user_count;                        //用户个数
    NUINT32 cpm_bike_cur_id;                       //当前车辆ID
    NUINT8 cpm_bike_count;                        //车辆数
    NCHAR cpm_bike_name[32];                      //车辆名称
    NUINT8 cpm_bike_size;                         //车辆类型
    NUINT8 cpm_bike_suit;                         //套件配置   标配 高配
    NUINT8 cpm_bike_display;                      //骑行显示
    NUINT16 cpm_bike_frame_size;                  //车架尺寸
    NUINT16 cpm_bike_wheelsize;                   //车轮尺寸
    NFLOAT cpm_bike_handlelength;                 //曲柄长度
    NFLOAT cpm_bike_weight;                       //车重
    NFLOAT cpm_bike_mileage;                      //车辆里程
    NUINT32 cpm_total_cyc_count;                  //当前骑行数据标志
    NUINT32 cpm_del_cyc_count;                   //当前骑行数据删除到哪标志
    NUINT32 cpm_cycling_start_time;               //骑行开始时间
    NUINT32 cpm_cycling_stop_time;                //骑行结束时间
    NFLOAT cpm_cycling_latitude;                 //纬度
    NFLOAT cpm_cycling_longitude;                //经度
    NFLOAT cpm_cycling_mileage;                   //骑行里程
    NUINT32 cpm_cycling_duration;                 //骑行时间
    NFLOAT cpm_cycling_v_max;                     //骑行最大速度
    NFLOAT cpm_cycling_v_ave;                     //骑行平均速度
    NFLOAT cpm_cycling_v;                         //骑行实时速度
    NUINT16 cpm_cycling_rpm_max;                  //骑行最大踏频
    NFLOAT cpm_cycling_rpm_ave;                  //骑行平均踏频
    NUINT16 cpm_cycling_rpm;                      //骑行实时踏频
    NUINT8 cpm_cycling_hrm_max;                   //骑行最大心率
    NFLOAT cpm_cycling_hrm_ave;                   //骑行平均心率
    NUINT8 cpm_cycling_hrm;                       //骑行实时心率
    NFLOAT cpm_cycling_bpwr_max;                  //最大功率
    NFLOAT cpm_cycling_bpwr_ave;                  //平均功率
    NFLOAT cpm_cycling_bpwr;                      //实时功率
    NFLOAT cpm_cycling_elevation;                 //海拔
    NFLOAT cpm_cycling_rising_distance;           //上升距离
    NFLOAT cpm_cycling_decline_distance;          //下降距离
    NUINT8 cpm_cycling_max_left_angle;            //最大左倾角
    NUINT8 cpm_cycling_max_right_angle;           //最大右倾角
    NFLOAT cpm_cycling_kcal;                      //消耗热量
    NFLOAT cpm_cycling_temperature;               //温度
    NFLOAT cpm_cycling_als;                       //环境光数值
    NFLOAT cpm_cycling_pressure;                  //大气压强
    NUINT8 cpm_cycling_sub_index;                 //当前骑行段数
    NUINT32 cpm_cycling_sub_start_time;           //分段骑行开始时间
    NUINT32 cpm_cycling_sub_end_time;             //分段骑行结束时间
    NFLOAT cpm_cycling_sub_mileage;               //分段骑行里程
    NFLOAT cpm_cycling_sub_v_ave;                 //分段骑行平均速度
    NUINT32 cpm_record_start_time;                //个人记录-开始于
    NFLOAT cpm_record_max_mileage;                //个人记录-最远距离
    NUINT32 cpm_record_max_duration;              //个人记录-最久骑行时间
    NUINT32 cpm_record_fastest_10km_duration;     //个人记录-最快10公里骑行时间
    NFLOAT cpm_record_max_v;                      //个人记录-最大速度
    NUINT16 cpm_record_max_rpm;                   //个人记录-最大踏频
    NUINT8 cpm_record_max_hrm;                    //个人记录-最大心率
    NFLOAT cpm_record_max_bpwr;                   //个人记录-最大功率
    NUINT8 cpm_record_max_angle;                  //个人记录-最大倾角
    NFLOAT cpm_record_max_rising_distance;        //个人记录-最大上升距离
    NFLOAT cpm_record_max_decline_distance;       //个人记录-最大下降距离
    NFLOAT cpm_record_max_kcal;                   //个人记录-最大消耗热量
    NUINT32 cpm_cumulative_start_time;            //累计骑行总起始时间
    NUINT32 cpm_cumulative_total_cycling_counts;  //累计骑行次数
    NFLOAT cpm_cumulative_total_mileage;          //累计骑行总里程
    NUINT32 cpm_cumulative_total_duration;        //累计骑行总时间
    NFLOAT cpm_cumulative_total_rising_distance;  //累计总上升距离
    NFLOAT cpm_cumulative_total_decline_distance; //累计总下降距离
    NFLOAT cpm_cumulative_total_kcal;             //累计总消耗热量
    NUINT32 cpm_timestamp;                        //时间戳
    NUINT32 cpm_b9watch_server_cmd;               //服务器命令解析
    NUINT16 cpm_ant_dev_conn_state;                 //ANT设备连接状态
    NCHAR cpm_nordic_verno[4];                      //nordic设备版本号
    NUINT8 cpm_cycling_state;                       //骑行状态
    NCHAR cpm_add_user[64];                         //添加用户信息
    NUINT8 cpm_delete_user;                         //删除用户信息
    NCHAR cpm_add_bike[64];                         //添加车辆信息
    NUINT8 cpm_delete_bike;                         //删除车辆信息
    NUINT8 cpm_bound_app_state;                     //码表和app绑定
    NCHAR cpm_conn_ble_name[32];                    //连接的蓝牙名字
    NUINT8 cpm_gprs_data_state;                     //蜂窝数据开关
    NUINT8 cpm_cyc_history_auto_upload;             //骑行记录自动上传
    NUINT8 cpm_cyc_history_auto_delete;             //自动删除历史旧数据
    NCHAR cpm_get_bike_list[64];                    //获取车辆列表
    NUINT8 cpm_cycling_angle;                       //当前骑行倾角
    NUINT8 cpm_cycling_slope;                       //当前骑行坡度
    NUINT32 cpm_app_read_cyc_counts;                //APP已经读取多少条骑行数据
    NUINT32 cpm_current_cycling_index;              //当前骑行记录唯一标识
} NIU_DATA_BODY_CPM;

typedef union _ndata_value_db_ {
    NIU_DATA_BODY_DB data;
    NCHAR buff[sizeof(NIU_DATA_BODY_DB)];
} NDATA_VALUE_DB;

typedef union _ndata_value_lockboard_ {
    NIU_DATA_BODY_LOCKBOARD data;
    NCHAR buff[sizeof(NIU_DATA_BODY_LOCKBOARD)];
} NDATA_VALUE_LOCKBOARD;

typedef union _ndata_value_foc_ {
    NIU_DATA_BODY_FOC data;
    NCHAR buff[sizeof(NIU_DATA_BODY_FOC)];
} NDATA_VALUE_FOC;

typedef union _ndata_value_bms_ {
    NIU_DATA_BODY_BMS data;
    NCHAR buff[sizeof(NIU_DATA_BODY_BMS)];
} NDATA_VALUE_BMS;

typedef union _ndata_value_lcu_ {
    NIU_DATA_BODY_LCU data;
    NCHAR buff[sizeof(NIU_DATA_BODY_LCU)];
} NDATA_VALUE_LCU;

typedef union _ndata_value_ecu_ {
    NIU_DATA_BODY_ECU data;
    NCHAR buff[sizeof(NIU_DATA_BODY_ECU)];
} NDATA_VALUE_ECU;

typedef union _ndata_value_alarm_ {
    NIU_DATA_BODY_ALARM data;
    NCHAR buff[sizeof(NIU_DATA_BODY_ALARM)];
} NDATA_VALUE_ALARM;

// new add, refer to V3data.xlsx
typedef union _ndata_value_bcs_ {
    NIU_DATA_BODY_BCS data;
    NCHAR buff[sizeof(NIU_DATA_BODY_BCS)];
} NDATA_VALUE_BCS;

typedef union _ndata_value_bms2_ {
    NIU_DATA_BODY_BMS2 data;
    NCHAR buff[sizeof(NIU_DATA_BODY_BMS2)];
} NDATA_VALUE_BMS2;

typedef union _ndata_value_QC_ {
    NIU_DATA_BODY_QC data;
    NCHAR buff[sizeof(NIU_DATA_BODY_QC)];
} NDATA_VALUE_QC;

typedef union _ndata_value_emcu_ {
    NIU_DATA_BODY_EMCU data;
    NCHAR buff[sizeof(NIU_DATA_BODY_EMCU)];
} NDATA_VALUE_EMCU;

typedef union _ndata_value_cpm {
    NIU_DATA_BODY_CPM data;
    NCHAR buff[sizeof(NIU_DATA_BODY_CPM)];
}NDATA_VALUE_CPM;
//end add

typedef struct _ndata_value_
{
    NDATA_VALUE_DB db;
    NDATA_VALUE_FOC foc;
    NDATA_VALUE_BMS bms;
    NDATA_VALUE_LCU lcu;
    NDATA_VALUE_ECU ecu;
    NDATA_VALUE_ALARM alarm;
    NDATA_VALUE_LOCKBOARD lockboard;
    //new add, refer to V3data.xlsx
    NDATA_VALUE_BCS bcs;
    NDATA_VALUE_BMS2 bms2;
    NDATA_VALUE_QC qc;
    NDATA_VALUE_EMCU emcu;
    NDATA_VALUE_CPM cpm;
    //end add
} NDATA_VALUE;

JSTATIC NDATA_VALUE g_niu_data = {
    .db = {.buff = {0}},
    .foc = {.buff = {0}},
    .bms = {.buff = {0}},
    .lcu = {.buff = {0}},
    .ecu = {.buff = {0}},
    .alarm = {.buff = {0}},
    .lockboard = {.buff = {0}},
    //new add , refer to V3data.xlsx
    .bcs = {.buff = {0}},
    .bms2 = {.buff = {0}},
    .qc = {.buff = {0}},
    .emcu = {.buff = {0}},
    .cpm = {.buff = {0}}
    //end add
};

JSTATIC NIU_SERVER_UPL_TEMPLATE g_niu_upl_cmd[NIU_SERVER_UPL_CMD_NUM];

#define NIU_DATA_ITEM(m, t) \
    {                       \
        &m, t, sizeof(m)    \
    }

static NIU_DATA_VALUE niu_data_value_ecu[] =
{
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_url, NIU_STRING),                  //服务器域名
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_port, NIU_UINT32),                 //服务器端口
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_serial_number, NIU_STRING),           //中控序列号
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_soft_ver, NIU_STRING),                //中控软件版本
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_hard_ver, NIU_STRING),                //中控硬件版本
        NIU_DATA_ITEM(g_niu_data.ecu.data.imei, NIU_STRING),                        //IMEI
        NIU_DATA_ITEM(g_niu_data.ecu.data.iccid, NIU_STRING),                       //ICCID
        NIU_DATA_ITEM(g_niu_data.ecu.data.imsi, NIU_STRING),                        //IMSI
        NIU_DATA_ITEM(g_niu_data.ecu.data.eid, NIU_STRING),                         //eid
        NIU_DATA_ITEM(g_niu_data.ecu.data.moved_length, NIU_UINT16),                //异动判断距离
        NIU_DATA_ITEM(g_niu_data.ecu.data.dealer_code, NIU_STRING),                 //经销商编码
        NIU_DATA_ITEM(g_niu_data.ecu.data.utc, NIU_INT32),                          //时区
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_type, NIU_UINT8),                     //车型编号
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_speed_index, NIU_FLOAT),              //仪表速度系数
        NIU_DATA_ITEM(g_niu_data.ecu.data.tize_size, NIU_FLOAT),                    //轮胎周长
        NIU_DATA_ITEM(g_niu_data.ecu.data.speed_loc_soc, NIU_UINT8),                //低电量限速开始电量
        NIU_DATA_ITEM(g_niu_data.ecu.data.lost_enery, NIU_UINT32),                  //当日消耗电量wh
        NIU_DATA_ITEM(g_niu_data.ecu.data.lost_soc, NIU_UINT32),                    //当日消耗电量
        NIU_DATA_ITEM(g_niu_data.ecu.data.last_mileage, NIU_UINT32),                //上次骑行里程
        NIU_DATA_ITEM(g_niu_data.ecu.data.total_mileage, NIU_UINT32),               //总行驶里程
        NIU_DATA_ITEM(g_niu_data.ecu.data.charge_value, NIU_UINT8),                 //充电开启电压
        NIU_DATA_ITEM(g_niu_data.ecu.data.discharge_value, NIU_UINT8),              //结束充电电压
        NIU_DATA_ITEM(g_niu_data.ecu.data.shaking_value, NIU_UINT8),                //震动检测阀值
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_cfg, NIU_UINT16),                     //中控配置
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_cmd, NIU_UINT16),                     //中控命令
        NIU_DATA_ITEM(g_niu_data.ecu.data.bms_cmd, NIU_UINT16),                     //BMS命令
        NIU_DATA_ITEM(g_niu_data.ecu.data.foc_cmd, NIU_UINT16),                     //控制器命令
        NIU_DATA_ITEM(g_niu_data.ecu.data.db_cmd, NIU_UINT16),                      //仪表命令
        NIU_DATA_ITEM(g_niu_data.ecu.data.lcu_cmd, NIU_UINT16),                     //灯控命令
        NIU_DATA_ITEM(g_niu_data.ecu.data.alarm_cmd, NIU_UINT32),                   //报警器命令
        NIU_DATA_ITEM(g_niu_data.ecu.data.display_random_code, NIU_UINT32),         //仪表显示随机码
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_1, NIU_UINT16),         //包组合编号1
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_2, NIU_UINT16),         //包组合编号2
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_3, NIU_UINT16),         //包组合编号3
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_4, NIU_UINT16),         //包组合编号4
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_5, NIU_UINT16),         //包组合编号5
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_6, NIU_UINT16),         //包组合编号6
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_7, NIU_UINT16),         //包组合编号7
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_8, NIU_UINT16),         //包组合编号8
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_9, NIU_UINT16),         //包组合编号9
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_10, NIU_UINT16),        //包组合编号10
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_11, NIU_UINT16),        //包组合编号11
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_12, NIU_UINT16),        //包组合编号12
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_13, NIU_UINT16),        //包组合编号13
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_14, NIU_UINT16),        //包组合编号14
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_15, NIU_UINT16),        //包组合编号15
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_16, NIU_UINT16),        //包组合编号16
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_17, NIU_UINT16),        //包组合编号17
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_18, NIU_UINT16),        //包组合编号18
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_19, NIU_UINT16),        //包组合编号19
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_20, NIU_UINT16),        //包组合编号20
        NIU_DATA_ITEM(g_niu_data.ecu.data.longitude, NIU_DOUBLE),                   //经度
        NIU_DATA_ITEM(g_niu_data.ecu.data.latitude, NIU_DOUBLE),                    //纬度
        NIU_DATA_ITEM(g_niu_data.ecu.data.hdop, NIU_FLOAT),                         //定位精度
        NIU_DATA_ITEM(g_niu_data.ecu.data.phop, NIU_FLOAT),                         //定位精度
        NIU_DATA_ITEM(g_niu_data.ecu.data.heading, NIU_FLOAT),                      //航向
        NIU_DATA_ITEM(g_niu_data.ecu.data.gps_speed, NIU_FLOAT),                    //GPS速度速度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_battary, NIU_UINT16),                 //中控电池电量
        NIU_DATA_ITEM(g_niu_data.ecu.data.now_bts, NIU_ARRAY),                      //当前基站信息
        NIU_DATA_ITEM(g_niu_data.ecu.data.near_bts1, NIU_ARRAY),                    //周边基站信息
        NIU_DATA_ITEM(g_niu_data.ecu.data.near_bts2, NIU_ARRAY),                    //周边基站信息
        NIU_DATA_ITEM(g_niu_data.ecu.data.near_bts3, NIU_ARRAY),                    //周边基站信息
        NIU_DATA_ITEM(g_niu_data.ecu.data.near_bts4, NIU_ARRAY),                    //周边基站信息
        NIU_DATA_ITEM(g_niu_data.ecu.data.ap_ip, NIU_UINT32),                       //接入点IP
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_no_gps_signal_time, NIU_UINT32),      //没有GPS定位计时
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_no_gsm_signal_time, NIU_UINT32),      //没有GSM信号计时
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_sleep_time, NIU_UINT32),              //车辆休眠了多少时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.x_acc, NIU_INT16),                        //X轴加速度
        NIU_DATA_ITEM(g_niu_data.ecu.data.y_acc, NIU_INT16),                        //Y轴加速度
        NIU_DATA_ITEM(g_niu_data.ecu.data.z_acc, NIU_INT16),                        //Z轴加速度
        NIU_DATA_ITEM(g_niu_data.ecu.data.x_ang, NIU_INT16),                        //X轴角加速度
        NIU_DATA_ITEM(g_niu_data.ecu.data.y_ang, NIU_INT16),                        //Y轴角加速度
        NIU_DATA_ITEM(g_niu_data.ecu.data.z_ang, NIU_INT16),                        //Z轴角加速度
        NIU_DATA_ITEM(g_niu_data.ecu.data.x_mag, NIU_INT16),                        //X轴磁力值
        NIU_DATA_ITEM(g_niu_data.ecu.data.y_mag, NIU_INT16),                        //Y轴磁力值
        NIU_DATA_ITEM(g_niu_data.ecu.data.z_mag, NIU_INT16),                        //Z轴磁力值
        NIU_DATA_ITEM(g_niu_data.ecu.data.air_pressure, NIU_UINT32),                //气压
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_state, NIU_UINT32),                   //中控状态
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_sta, NIU_UINT32),                     //机车状态
        NIU_DATA_ITEM(g_niu_data.ecu.data.timestamp, NIU_UINT32),                   //时间戳
        NIU_DATA_ITEM(g_niu_data.ecu.data.gps_singal, NIU_UINT8),                   //GPS信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.csq, NIU_UINT8),                          //GSM信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_running_time, NIU_UINT32),            //车辆行驶了多少时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_stoping_time, NIU_UINT32),            //车辆停车了多少时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.ota_exec_eqp, NIU_UINT8),                 //固件升级设备
        NIU_DATA_ITEM(g_niu_data.ecu.data.ota_rate, NIU_UINT8),                     //固件升级设备进度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ota_bin_url, NIU_STRING),                 //固件URL
        NIU_DATA_ITEM(g_niu_data.ecu.data.ota_soft_ver, NIU_STRING),                //固件升级软件版本
        NIU_DATA_ITEM(g_niu_data.ecu.data.ota_hard_ver, NIU_STRING),                //固件升级硬件版本
        NIU_DATA_ITEM(g_niu_data.ecu.data.ota_pack_size, NIU_UINT32),               //固件大小
        NIU_DATA_ITEM(g_niu_data.ecu.data.ota_pack_num, NIU_UINT32),                //固件分包数量
        NIU_DATA_ITEM(g_niu_data.ecu.data.ota_eqp, NIU_UINT8),                      //要升级的设备
        NIU_DATA_ITEM(g_niu_data.ecu.data.stop_longitude, NIU_DOUBLE),              //停车时经度
        NIU_DATA_ITEM(g_niu_data.ecu.data.stop_latitude, NIU_DOUBLE),               //停车时纬度
        NIU_DATA_ITEM(g_niu_data.ecu.data.db_code_dis_time, NIU_UINT16),            //仪表随机码显示时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_mcc, NIU_UINT16),                    //移动国家号
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_mnc, NIU_UINT16),                    //移动网络号码
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_lac_1, NIU_UINT16),                  //位置区码?
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_cid_1, NIU_UINT32),                  //小区编号
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_rssi_1, NIU_UINT16),                 //信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_lac_2, NIU_UINT16),                  //位置区码?
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_cid_2, NIU_UINT32),                  //小区编号
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_rssi_2, NIU_UINT16),                 //信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_lac_3, NIU_UINT16),                  //位置区码?
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_cid_3, NIU_UINT32),                  //小区编号
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_rssi_3, NIU_UINT16),                 //信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_lac_4, NIU_UINT16),                  //位置区码?
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_cid_4, NIU_UINT32),                  //小区编号
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_rssi_4, NIU_UINT16),                 //信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_lac_5, NIU_UINT16),                  //位置区码?
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_cid_5, NIU_UINT32),                  //小区编号
        NIU_DATA_ITEM(g_niu_data.ecu.data.cell_rssi_5, NIU_UINT16),                 //信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.pole_pairs, NIU_UINT16),                  //极对数
        NIU_DATA_ITEM(g_niu_data.ecu.data.radius, NIU_UINT16),                      //速度校准配置
        NIU_DATA_ITEM(g_niu_data.ecu.data.calu_speed_index, NIU_FLOAT),             //速度计算系数
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi1_bssid_1, NIU_UINT8),                //wifi1_bssid_1
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi1_bssid_2, NIU_UINT8),                //wifi1_bssid_2
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi1_bssid_3, NIU_UINT8),                //wifi1_bssid_3
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi1_bssid_4, NIU_UINT8),                //wifi1_bssid_4
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi1_bssid_5, NIU_UINT8),                //wifi1_bssid_5
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi1_bssid_6, NIU_UINT8),                //wifi1_bssid_6
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi1_rssi, NIU_UINT8),                   //wifi1_rssi
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi2_bssid_1, NIU_UINT8),                //wifi2_bssid_1
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi2_bssid_2, NIU_UINT8),                //wifi2_bssid_2
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi2_bssid_3, NIU_UINT8),                //wifi2_bssid_3
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi2_bssid_4, NIU_UINT8),                //wifi2_bssid_4
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi2_bssid_5, NIU_UINT8),                //wifi2_bssid_5
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi2_bssid_6, NIU_UINT8),                //wifi2_bssid_6
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi2_rssi, NIU_UINT8),                   //wifi2_rssi
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi3_bssid_1, NIU_UINT8),                //wifi3_bssid_1
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi3_bssid_2, NIU_UINT8),                //wifi3_bssid_2
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi3_bssid_3, NIU_UINT8),                //wifi3_bssid_3
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi3_bssid_4, NIU_UINT8),                //wifi3_bssid_4
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi3_bssid_5, NIU_UINT8),                //wifi3_bssid_5
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi3_bssid_6, NIU_UINT8),                //wifi3_bssid_6
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi3_rssi, NIU_UINT8),                   //wifi3_rssi
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi4_bssid_1, NIU_UINT8),                //wifi4_bssid_1
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi4_bssid_2, NIU_UINT8),                //wifi4_bssid_2
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi4_bssid_3, NIU_UINT8),                //wifi4_bssid_3
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi4_bssid_4, NIU_UINT8),                //wifi4_bssid_4
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi4_bssid_5, NIU_UINT8),                //wifi4_bssid_5
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi4_bssid_6, NIU_UINT8),                //wifi4_bssid_6
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi4_rssi, NIU_UINT8),                   //wifi4_rssi
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi5_bssid_1, NIU_UINT8),                //wifi5_bssid_1
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi5_bssid_2, NIU_UINT8),                //wifi5_bssid_2
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi5_bssid_3, NIU_UINT8),                //wifi5_bssid_3
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi5_bssid_4, NIU_UINT8),                //wifi5_bssid_4
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi5_bssid_5, NIU_UINT8),                //wifi5_bssid_5
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi5_bssid_6, NIU_UINT8),                //wifi5_bssid_6
        NIU_DATA_ITEM(g_niu_data.ecu.data.wifi5_rssi, NIU_UINT8),                   //wifi5_rssi
        NIU_DATA_ITEM(g_niu_data.ecu.data.smt_test, NIU_UINT16),                    //smt_test
        NIU_DATA_ITEM(g_niu_data.ecu.data.app_control, NIU_UINT32),                 //服务器控制字段
        NIU_DATA_ITEM(g_niu_data.ecu.data.fota_download_percent, NIU_UINT8),        //FOTA进度
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_eqp_cfg, NIU_UINT32),                 //车内设备列表
        NIU_DATA_ITEM(g_niu_data.ecu.data.car_eqp_state, NIU_UINT32),               //车内可读状态
        NIU_DATA_ITEM(g_niu_data.ecu.data.fail_value, NIU_UINT16),                  //倾倒检测阈值
        NIU_DATA_ITEM(g_niu_data.ecu.data.fall_pitch, NIU_FLOAT),                   //校正后pitch
        NIU_DATA_ITEM(g_niu_data.ecu.data.fall_roll, NIU_FLOAT),                    //校正后roll
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_state2, NIU_UINT32),                  //中控状态2
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_adups_ver, NIU_STRING),               //FOTA版本号
        NIU_DATA_ITEM(g_niu_data.ecu.data.move_gsensor_count, NIU_UINT8),           //检测到震动次数
        NIU_DATA_ITEM(g_niu_data.ecu.data.move_gsensor_time, NIU_UINT8),            //检测时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.pdu_message_current_number, NIU_UINT16),   //实时缓存报文数量
        NIU_DATA_ITEM(g_niu_data.ecu.data.max_snr_1, NIU_UINT8),                    //最大GPS信号值
        NIU_DATA_ITEM(g_niu_data.ecu.data.max_snr_2, NIU_UINT8),                    //最大GPS信号值
        NIU_DATA_ITEM(g_niu_data.ecu.data.eye_settings, NIU_UINT8),                 //天眼配置值
        NIU_DATA_ITEM(g_niu_data.ecu.data.imei_4g, NIU_STRING),                     //IMEI
        NIU_DATA_ITEM(g_niu_data.ecu.data.temperature_4g, NIU_INT8),               //4G模块温度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_velocity, NIU_UINT16),                //hall转速
        NIU_DATA_ITEM(g_niu_data.ecu.data.ble_mac, NIU_STRING),                      //蓝牙mac地址
        NIU_DATA_ITEM(g_niu_data.ecu.data.ble_password, NIU_STRING),                //蓝牙密码
        NIU_DATA_ITEM(g_niu_data.ecu.data.ble_aes, NIU_STRING),                     //蓝牙秘钥
        NIU_DATA_ITEM(g_niu_data.ecu.data.ble_name, NIU_STRING),                    //蓝牙名称
        NIU_DATA_ITEM(g_niu_data.ecu.data.smt_test2, NIU_UINT16),                   //SMT测试结果
        NIU_DATA_ITEM(g_niu_data.ecu.data.limiting_velocity_max, NIU_UINT32),       //进入双限速转速
        NIU_DATA_ITEM(g_niu_data.ecu.data.limiting_velocity_min, NIU_UINT32),       //退出双限速转速
        NIU_DATA_ITEM(g_niu_data.ecu.data.front_tire_size, NIU_FLOAT),              //前轮周长
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_url2, NIU_STRING),                 //备用服务器域名
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_port2, NIU_UINT32),                //备用服务器端口
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_error_url, NIU_STRING),            //err上报服务器域名
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_error_port, NIU_UINT32),           //err上报服务器端口
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_error_url2, NIU_STRING),           //err上报备用服务器域名
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_error_port2, NIU_UINT32),          //err上报备用服务器端口
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_lbs_url, NIU_STRING),              //LBS服务器域名
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_lbs_port, NIU_UINT32),             //LBS服务器端口
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_lbs_url2, NIU_STRING),             //LBS备用服务器域名
        NIU_DATA_ITEM(g_niu_data.ecu.data.server_lbs_port2, NIU_UINT32),            //LBS备用服务器端口
        NIU_DATA_ITEM(g_niu_data.ecu.data.v35_alarm_state, NIU_UINT32),             //V35报警器状态字段
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_keepalive, NIU_UINT32),               //心跳包间隔
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_gps_max_cnr, NIU_UINT32),             //最大GPS信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_gps_min_cnr, NIU_UINT32),             //最小GPS信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_gps_avg_cnr, NIU_UINT32),             //平均GPS信号强度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_gps_seill_num, NIU_UINT32),           //定位卫星数量
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_no_cdma_location, NIU_STRING),        //非cdma时基站信息
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_cdma_location, NIU_STRING),           //cdma时基站信息
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_gps_sleep_time, NIU_UINT32),          //电门关闭时，gps休眠间隔
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_gps_awake_time, NIU_UINT32),          //电门关闭时，gps自动唤醒时长
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_avg_longitude, NIU_DOUBLE),           //电门关闭时GPS平均经度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_avg_latitude, NIU_DOUBLE),            //电门关闭时GPS平均纬度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_v35_config, NIU_UINT32),              //V35报警器配置
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_pitch, NIU_FLOAT),                    //俯仰角
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_roll, NIU_FLOAT),                     //翻滚角
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_yaw, NIU_FLOAT),                      //俯仰角
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_server_error_code, NIU_UINT32),       //最近一次断网原因
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_server_error_code_time, NIU_UINT32),  //最近一次断网时间戳
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_server_connected_time, NIU_UINT32),   //最近一次中控联网登录成功时间戳
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_server_ka_timeout_time, NIU_UINT32),  //最近一次中控检测KA_TIMEOUT时间戳
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_server_reset_4g_time, NIU_UINT32),    //最近中控复位4G模块时间戳
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_server_do_connect_count, NIU_UINT32), //socket connect操作的次数，即几次连上
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_server_reset_4g_count, NIU_UINT32),   //中控复位4G模块的累积次数
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_realtime_longitude, NIU_DOUBLE),      //实时经度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_realtime_latitude, NIU_DOUBLE),       //实时纬度
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_app_contorl, NIU_UINT32),             //服务器控制字段1
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_app_contorl2, NIU_UINT32),            //服务器控制字段2
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_app_contorl3, NIU_UINT32),            //服务器控制字段3
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_app_contorl10, NIU_STRING),           //服务器控制字段4
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_app_contorl11, NIU_STRING),           //服务器控制字段5
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_app_contorl12, NIU_STRING),           //服务器控制字段6
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_cmd_id, NIU_STRING),                  //命令ID
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_cmd_result, NIU_UINT32),              //命令结果
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_cmd_expiry_timestamp, NIU_UINT32),    //命令失效时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_cmd_receive_timestamp, NIU_UINT32),   //命令接收时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_cmd_exec_timestamp, NIU_UINT32),      //命令执行完毕时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_auto_close_time, NIU_UINT32),         //自动关机时间
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_ex_model_version, NIU_STRING),         //外置模块版本
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_gps_version, NIU_STRING),              //GPS模板版本
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_21, NIU_UINT16),        //包组合编号21
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_22, NIU_UINT16),        //包组合编号22
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_23, NIU_UINT16),        //包组合编号23
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_24, NIU_UINT16),        //包组合编号24
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_25, NIU_UINT16),        //包组合编号25
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_26, NIU_UINT16),        //包组合编号26
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_27, NIU_UINT16),        //包组合编号27
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_28, NIU_UINT16),        //包组合编号28
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_29, NIU_UINT16),        //包组合编号29
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_30, NIU_UINT16),        //包组合编号30
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_31, NIU_UINT16),        //包组合编号31
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_32, NIU_UINT16),        //包组合编号32
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_33, NIU_UINT16),        //包组合编号33
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_34, NIU_UINT16),        //包组合编号34
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_35, NIU_UINT16),        //包组合编号35
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_36, NIU_UINT16),        //包组合编号36
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_37, NIU_UINT16),        //包组合编号37
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_38, NIU_UINT16),        //包组合编号38
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_39, NIU_UINT16),        //包组合编号39
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_40, NIU_UINT16),        //包组合编号40
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_41, NIU_UINT16),        //包组合编号41
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_42, NIU_UINT16),        //包组合编号42
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_43, NIU_UINT16),        //包组合编号43
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_44, NIU_UINT16),        //包组合编号44
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_45, NIU_UINT16),        //包组合编号45
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_46, NIU_UINT16),        //包组合编号46
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_47, NIU_UINT16),        //包组合编号47
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_48, NIU_UINT16),        //包组合编号48
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_49, NIU_UINT16),        //包组合编号49
        NIU_DATA_ITEM(g_niu_data.ecu.data.uploading_template_50, NIU_UINT16),         //包组合编号50
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_inherit_mileage, NIU_UINT32),       //继承里程
        NIU_DATA_ITEM(g_niu_data.ecu.data.ecu_reboot_count, NIU_UINT32),       //中控重启计数
        NIU_DATA_ITEM(g_niu_data.ecu.data.serverUsername, NIU_STRING),
        NIU_DATA_ITEM(g_niu_data.ecu.data.serverPassword, NIU_STRING),
        NIU_DATA_ITEM(g_niu_data.ecu.data.aesKey, NIU_STRING),
        NIU_DATA_ITEM(g_niu_data.ecu.data.mqttSubscribe, NIU_STRING),
        NIU_DATA_ITEM(g_niu_data.ecu.data.mqttPublish, NIU_STRING)
};

static NIU_DATA_VALUE niu_data_value_emcu[] =
{
    NIU_DATA_ITEM(g_niu_data.emcu.data.emcu_soft_ver, NIU_STRING),   //软件版本
    NIU_DATA_ITEM(g_niu_data.emcu.data.emcu_hard_ver, NIU_STRING),   //硬件版本
    NIU_DATA_ITEM(g_niu_data.emcu.data.emcu_sn, NIU_STRING),         //设备序列号
    NIU_DATA_ITEM(g_niu_data.emcu.data.emcu_compiletime, NIU_STRING), //编译时间
    NIU_DATA_ITEM(g_niu_data.emcu.data.emcu_cmd, NIU_UINT32),         //emcu指令
    NIU_DATA_ITEM(g_niu_data.emcu.data.emcu_alarmkeyid, NIU_UINT32),   //远程对码
    NIU_DATA_ITEM(g_niu_data.emcu.data.emcu_smt_test, NIU_UINT32),   //SMT测试结果
    NIU_DATA_ITEM(g_niu_data.emcu.data.emcu_remote_group_code, NIU_STRING)   //遥控器组合代码
};

static NIU_DATA_VALUE niu_data_value_bms[] =
{
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_user, NIU_UINT8),        //用户权限
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_bms_id, NIU_UINT8),      //设备ID
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_s_ver, NIU_UINT8),       //软件版本
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_h_ver, NIU_UINT8),       //硬件版本
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_p_pwrd, NIU_STRING),      //参数保护密码
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_bat_type, NIU_UINT8),    //电芯型号代码
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_sn_id, NIU_STRING),      //设备序列号
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_oc_vlt_p, NIU_UINT8),    //过充保护电压
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_roc_vlt, NIU_UINT8),     //过充恢复电压
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_dc_vlt_p, NIU_UINT8),    //过放保护电压
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_rdc_vlt_p, NIU_UINT8),   //过放恢复电压
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_c_b_vlt, NIU_UINT8),     //充电均衡起始电压
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_c_cur_p, NIU_UINT8),     //充电保护电流
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_dc_cur_p, NIU_UINT8),    //放电保护电流
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_isc_p, NIU_UINT8),       //短路保护电流
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_dcotp_p, NIU_INT8),     //放电高温保护
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_rdcotp_p, NIU_INT8),    //放电恢复高温保护
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_cotp_p, NIU_INT8),      //充电高温保护
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_rcotp_p, NIU_INT8),     //充电恢复高温保护
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_dcltp_p, NIU_INT8),     //放电低温保护
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_rdcltp_p, NIU_INT8),    //放电恢复低温保护
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_ncom_dc_en, NIU_UINT8),  //额定电压固定
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_rated_vlt, NIU_UINT8),   //额定电压
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_bat_to_cap, NIU_UINT16), //电池容量
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_c_cont, NIU_UINT16),     //电池循环次数
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_to_vlt_rt, NIU_UINT16),  //总电压
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_c_cur_rt, NIU_UINT16),   //充电电流
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_dc_cur_rt, NIU_UINT16),  //放电电流
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_soc_rt, NIU_UINT8),      //SOC估算
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_bat_sta_rt, NIU_UINT16), //实时电池状态
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_dc_fl_t_rt, NIU_UINT8),  //充满电剩余时间
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_tp1_rt, NIU_INT8),      //当前温度1
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_tp2_rt, NIU_INT8),      //当前温度2
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_tp3_rt, NIU_INT8),      //当前温度3
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_tp4_rt, NIU_INT8),      //当前温度4
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_tp5_rt, NIU_INT8),      //当前温度5
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt1, NIU_UINT16),  //电池组电压1
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt2, NIU_UINT16),  //电池组电压2
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt3, NIU_UINT16),  //电池组电压3
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt4, NIU_UINT16),  //电池组电压4
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt5, NIU_UINT16),  //电池组电压5
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt6, NIU_UINT16),  //电池组电压6
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt7, NIU_UINT16),  //电池组电压7
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt8, NIU_UINT16),  //电池组电压8
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt9, NIU_UINT16),  //电池组电压9
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt10, NIU_UINT16), //电池组电压10
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt11, NIU_UINT16), //电池组电压11
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt12, NIU_UINT16), //电池组电压12
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt13, NIU_UINT16), //电池组电压13
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt14, NIU_UINT16), //电池组电压14
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt15, NIU_UINT16), //电池组电压15
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt16, NIU_UINT16), //电池组电压16
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt17, NIU_UINT16), //电池组电压17
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt18, NIU_UINT16), //电池组电压18
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt19, NIU_UINT16), //电池组电压19
    NIU_DATA_ITEM(g_niu_data.bms.data.bms_g_vlt_rt20, NIU_UINT16)  //电池组电压20
};

static NIU_DATA_VALUE niu_data_value_lcu[] =
{
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_soft_ver, NIU_STRING),     //软件版本
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_hard_ver, NIU_STRING),     //硬件版本
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_sn, NIU_STRING),           //序列号
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_state, NIU_UINT16),        //整机状态
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_switch_state, NIU_UINT16), //组合开关状态
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_light_state, NIU_UINT16),  //灯具状态
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_x_acc, NIU_INT16),         //X轴加速度foc_acc_mode_stat
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_y_acc, NIU_INT16),         //Y轴加速度
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_z_acc, NIU_INT16),         //Z轴加速度
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_x_ang, NIU_INT16),         //X轴角加速度
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_y_ang, NIU_INT16),         //Y轴角加速度
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_z_ang, NIU_INT16),         //Y轴角加速度
    NIU_DATA_ITEM(g_niu_data.lcu.data.lcu_car_state, NIU_UINT16)     //车辆状态
};

static NIU_DATA_VALUE niu_data_value_db[] =
{
    NIU_DATA_ITEM(g_niu_data.db.data.db_status, NIU_UINT16),     //机车状态
    NIU_DATA_ITEM(g_niu_data.db.data.db_gears, NIU_UINT8),       //档位
    NIU_DATA_ITEM(g_niu_data.db.data.db_speed, NIU_UINT8),       //速度
    NIU_DATA_ITEM(g_niu_data.db.data.db_mileage, NIU_UINT32),    //里程数
    NIU_DATA_ITEM(g_niu_data.db.data.db_c_cur, NIU_UINT8),       //充电电流
    NIU_DATA_ITEM(g_niu_data.db.data.db_dc_cur, NIU_UINT8),      //放电电流
    NIU_DATA_ITEM(g_niu_data.db.data.db_bat_soc, NIU_UINT8),     //电池SOC
    NIU_DATA_ITEM(g_niu_data.db.data.db_full_c_t, NIU_UINT8),    //充电剩余时间
    NIU_DATA_ITEM(g_niu_data.db.data.db_hour, NIU_UINT8),        //当前小时
    NIU_DATA_ITEM(g_niu_data.db.data.db_min, NIU_UINT8),         //当前分钟
    NIU_DATA_ITEM(g_niu_data.db.data.db_f_code, NIU_UINT8),      //故障代码
    NIU_DATA_ITEM(g_niu_data.db.data.db_bat2_soc, NIU_UINT8),    //电池2SOC
    NIU_DATA_ITEM(g_niu_data.db.data.db_status2, NIU_UINT16),    //车辆状态2
    NIU_DATA_ITEM(g_niu_data.db.data.db_total_soc, NIU_UINT8),   //总电池SOC
    NIU_DATA_ITEM(g_niu_data.db.data.db_rgb_set, NIU_STRING),     //RGB指令
    NIU_DATA_ITEM(g_niu_data.db.data.db_rgb_config, NIU_UINT32), //RGB配色命令
    NIU_DATA_ITEM(g_niu_data.db.data.db_soft_ver, NIU_STRING),   //软件版本
    NIU_DATA_ITEM(g_niu_data.db.data.db_hard_ver, NIU_STRING),   //硬件版本
    NIU_DATA_ITEM(g_niu_data.db.data.db_sn, NIU_STRING)          //序列号
};

static NIU_DATA_VALUE niu_data_value_alarm[] =
{
    NIU_DATA_ITEM(g_niu_data.alarm.data.alarm_soft_ver, NIU_STRING),  //软件版本
    NIU_DATA_ITEM(g_niu_data.alarm.data.alarm_hard_ver, NIU_STRING),  //硬件版本
    NIU_DATA_ITEM(g_niu_data.alarm.data.alarm_sn, NIU_STRING),        //序列号
    NIU_DATA_ITEM(g_niu_data.alarm.data.alarm_state, NIU_UINT8),      //报警器状态
    NIU_DATA_ITEM(g_niu_data.alarm.data.alarm_control_cmd, NIU_UINT8) //报警器控制命令
};

static NIU_DATA_VALUE niu_data_value_lockboard[] =
{
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_hard_ver, NIU_STRING),      //硬件版本号
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_soft_ver, NIU_STRING),      //型号
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_sn, NIU_STRING),            //序列号
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_cfg, NIU_UINT32),           //配置表
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_status, NIU_UINT16),        //锁控状态
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_alarm_state, NIU_UINT16),   //报警器状态
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock1_state, NIU_UINT8),    //坐垫锁状态
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock2_state, NIU_UINT8),    //尾箱锁状态
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock3_state, NIU_UINT8),    //龙头锁状态
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_helmet_state, NIU_UINT8),   //头盔状态
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_alarm_cmd, NIU_UINT8),      //报警器命令
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock1_cmd, NIU_UINT8),      //坐垫锁命令
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock2_cmd, NIU_UINT8),      //尾箱锁命令
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock3_cmd, NIU_UINT8),      //龙头锁命令
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_batterysta, NIU_UINT8),     //内电池电量
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_powersta, NIU_STRING),      //所有电池状态
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock1cur, NIU_UINT16),      //最后一次开解锁坐垫锁电流
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock2cur, NIU_UINT16),      //最后一次开解锁尾箱锁电流
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock3cur, NIU_UINT16),      //最后一次开解锁龙头锁电流
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_vlocksta, NIU_UINT16),      //最后一次开解锁锁供电电压
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_vin_status, NIU_UINT16),    //外电电压
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_vlock_status, NIU_UINT16),  //锁供电电压
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_vbat_status, NIU_UINT16),   //内电电压
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_vdd_status, NIU_UINT16),    //MCU供电电压
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock1_timeout, NIU_UINT16), //坐垫锁开解锁超时时间
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock2_timeout, NIU_UINT16), //尾箱锁开解锁超时时间
    NIU_DATA_ITEM(g_niu_data.lockboard.data.lock_board_lock3_timeout, NIU_UINT16)  //龙头锁开解锁超时时间
};

static NIU_DATA_VALUE niu_data_value_bms2[] =
{
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_user, NIU_UINT8),        //用户权限
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_id, NIU_UINT8),          //设备ID
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_s_ver, NIU_UINT8),       //软件版本
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_h_ver, NIU_UINT8),       //硬件版本
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_p_pwrd, NIU_STRING),      //参数保护密码
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_bat_type, NIU_UINT8),    //电芯型号代码
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_sn_id, NIU_STRING),      //设备序列号
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_oc_vlt_p, NIU_UINT8),    //过充保护电压
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_roc_vlt, NIU_UINT8),     //过充恢复电压
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_dc_vlt_p, NIU_UINT8),    //过放保护电压
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_rdc_vlt_p, NIU_UINT8),   //过放恢复电压
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_c_b_vlt, NIU_UINT8),     //充电均衡起始电压
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_c_cur_p, NIU_UINT8),     //充电保护电流
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_dc_cur_p, NIU_UINT8),    //放电保护电流
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_isc_p, NIU_UINT8),       //短路保护电流
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_dcotp_p, NIU_INT8),     //放电高温保护
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_rdcotp_p, NIU_INT8),    //放电恢复高温保护
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_cotp_p, NIU_INT8),      //充电高温保护
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_rcotp_p, NIU_INT8),     //充电恢复高温保护
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_dcltp_p, NIU_INT8),     //放电低温保护
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_rdcltp_p, NIU_INT8),    //放电恢复低温保护
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_ncom_dc_en, NIU_UINT8),  //额定电压固定
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_rated_vlt, NIU_UINT8),   //额定电压
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_bat_to_cap, NIU_UINT16), //电池容量
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_c_cont, NIU_UINT16),     //电池循环次数
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_to_vlt_rt, NIU_UINT16),  //总电压
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_c_cur_rt, NIU_UINT16),   //充电电流
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_dc_cur_rt, NIU_UINT16),  //放电电流
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_soc_rt, NIU_UINT8),      //SOC估算
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_bat_sta_rt, NIU_UINT16), //实时电池状态
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_dc_fl_t_rt, NIU_UINT8),  //充满电剩余时间
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_tp1_rt, NIU_INT8),      //当前温度1
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_tp2_rt, NIU_INT8),      //当前温度2
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_tp3_rt, NIU_INT8),      //当前温度3
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_tp4_rt, NIU_INT8),      //当前温度4
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_tp5_rt, NIU_INT8),      //当前温度5
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt1, NIU_UINT16),  //电池组电压1
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt2, NIU_UINT16),  //电池组电压2
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt3, NIU_UINT16),  //电池组电压3
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt4, NIU_UINT16),  //电池组电压4
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt5, NIU_UINT16),  //电池组电压5
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt6, NIU_UINT16),  //电池组电压6
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt7, NIU_UINT16),  //电池组电压7
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt8, NIU_UINT16),  //电池组电压8
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt9, NIU_UINT16),  //电池组电压9
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt10, NIU_UINT16), //电池组电压10
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt11, NIU_UINT16), //电池组电压11
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt12, NIU_UINT16), //电池组电压12
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt13, NIU_UINT16), //电池组电压13
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt14, NIU_UINT16), //电池组电压14
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt15, NIU_UINT16), //电池组电压15
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt16, NIU_UINT16), //电池组电压16
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt17, NIU_UINT16), //电池组电压17
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt18, NIU_UINT16), //电池组电压18
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt19, NIU_UINT16), //电池组电压19
    NIU_DATA_ITEM(g_niu_data.bms2.data.bms2_g_vlt_rt20, NIU_UINT16)  //电池组电压20
};

static NIU_DATA_VALUE niu_data_value_bcs[] =
{
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_user, NIU_UINT8),                 //用户权限
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_hard_id, NIU_UINT8),              //设备ID
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_s_ver, NIU_STRING),               //软件版本
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_h_ver, NIU_STRING),               //硬件版本
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_p_pwrd, NIU_STRING),               //参数保护密码
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_sn_id, NIU_STRING),               //设备序列号
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_charge_cur_p, NIU_UINT32),        //充电保护电流
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_discharge_cur_p, NIU_UINT32),     //放电保护电流
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_cur_cut_state, NIU_UINT32),       //切换电流值
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_charge_vol_p, NIU_UINT32),        //充电保护电压
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_discharge_vol_p, NIU_UINT32),     //放电保护电压
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_cmp_vol, NIU_UINT32),             //电压差值设定值
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_time_charge_cur_p, NIU_UINT8),    //充电过流保护恢复时间设定
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_time_discharge_cur_p, NIU_UINT8), //放电过流保护恢复时间设定
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_temp_het_in_p, NIU_INT8),        //高温保护值
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_temp_cold_in_p, NIU_INT8),       //低温保护值
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_temp_het_out_p, NIU_INT8),       //高温保护退出值
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_temp_cold_out_p, NIU_INT8),      //低温保护退出值
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bat_to_cap, NIU_UINT16),          //电池容量
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_state, NIU_UINT32),               //工作状态
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_cmd_sel_bt, NIU_UINT8),           //电池选择
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bt1_mos_temp, NIU_INT8),         //一号电池MOS温度
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bt2_mos_temp, NIU_INT8),         //二号电池MOS温度
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bcs_mos_temp, NIU_INT8),         //干路MOS温度
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_charge_cur, NIU_UINT32),          //充电电流
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_discharge_cur, NIU_UINT32),       //放电电流
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_soc_rt, NIU_UINT8),               //SOC估算
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_dc_fl_t_rt, NIU_UINT8),           //充满电剩余时间
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_to_charger_cur, NIU_UINT16),      //需要的充电电流
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_to_charger_vol, NIU_UINT32),      //需要的充电压
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bms1_soc, NIU_UINT8),             //电池1SOC估算
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bms2_soc, NIU_UINT8),             //电池2SOC估算
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bcs_output_vlt, NIU_UINT16),      //BCS输出电压
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bms1_input_vlt, NIU_UINT16),      //电池包1接入电压
    NIU_DATA_ITEM(g_niu_data.bcs.data.bcs_bms2_input_vlt, NIU_UINT16)       //电池包2接入电压
};

static NIU_DATA_VALUE niu_data_value_foc[] =
{
    NIU_DATA_ITEM(g_niu_data.foc.data.reserved0, NIU_UINT8),                   //reserved
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_id, NIU_UINT8),                      //ID号
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_mode, NIU_STRING),                   //型号
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_sn, NIU_STRING),                     //序列号
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_version, NIU_UINT8),                 //版本号
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_ratedvlt, NIU_UINT8),                //额定电压
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_minvlt, NIU_UINT8),                  //最低电压
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_maxvlt, NIU_UINT8),                  //最高电压
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_gears, NIU_UINT8),                   //档位
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_ratedcur, NIU_UINT16),               //额定电流
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_barcur, NIU_INT16),                 //母线电流
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_motorspeed, NIU_INT16),             //电机转速
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_cotlsta, NIU_UINT16),                //FOC状态
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_ctrlsta2, NIU_UINT16),               //控制器状态2
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_acc_mode_stat, NIU_UINT8),           //控制器加速度模式
    NIU_DATA_ITEM(g_niu_data.foc.data.reserved2, NIU_UINT8),                   //reserved
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_bat_soft_uv_en, NIU_UINT8),          //电池软欠压保护使能
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_bat_soft_uv_vlt, NIU_UINT8),         //电池软欠压电压
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_max_phase_cur, NIU_UINT16),          //最大相电流
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_min_phase_cur, NIU_UINT16),          //最小相电流
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_avg_phase_cur, NIU_UINT16),          //平均电流
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_throttle_mode_set, NIU_UINT8),       //转把模式选择
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_foc_accele, NIU_UINT16),             //加速度
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_foc_dccele, NIU_UINT16),             //减速度
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_ebs_en, NIU_UINT8),                  //EBS使能
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_ebs_re_vlt, NIU_UINT8),              //EBS反充电压
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_ebs_re_cur, NIU_UINT16),             //EBS反充电流
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_high_speed_ratio, NIU_UINT8),        //高档位速度
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_mid_speed_ratio, NIU_UINT8),         //中档位速度
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_low_speed_ratio, NIU_UINT8),         //低档位速度
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_high_ger_cur_limt_ratio, NIU_UINT8), //高档位限流比例
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_mid_ger_cur_limt_ratio, NIU_UINT8),  //中档位限流比例
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_low_ger_cur_limt_ratio, NIU_UINT8),  //低档位限流比例
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_flux_weaken_en, NIU_UINT8),          //弱磁使能
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_enter_flux_speed_ratio, NIU_UINT8),  //弱磁进入速度比例
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_exit_flux_speed_ratio, NIU_UINT8),   //弱磁退出速度比例
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_flux_w_max_cur, NIU_UINT16),         //弱磁最大电流
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_enter_flux_throttle_vlt, NIU_UINT8), //弱磁进入转把电压
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_boost_en, NIU_UINT8),                //Boost使能
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_boost_mode, NIU_UINT8),              //Boost模式
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_boost_cur, NIU_UINT16),              //Boost电流
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_boost_keep_time, NIU_UINT8),         //Boost保持时间
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_boost_interval_time, NIU_UINT8),     //Boost间隔时间
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_start_auto_en, NIU_UINT8),           //暴起模式使能
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_start_gear, NIU_UINT8),              //暴起档位
    NIU_DATA_ITEM(g_niu_data.foc.data.foc_start_keep_time, NIU_UINT8)          //暴起持续时间
};

static NIU_DATA_VALUE niu_data_value_qc[] =
{
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_led_state, NIU_UINT8),               //充电器指示灯状态
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_fault1, NIU_UINT32),                 //充电器故障代码1
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_fault2, NIU_UINT32),                 //充电器故障代码2
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltage1_adc, NIU_UINT16),           //电压1ADC
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltage1_100mv, NIU_UINT16),         //电压1
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltage2_adc, NIU_UINT16),           //电压2ADC
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltage2_100mv, NIU_UINT16),         //电压2
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_current_adc, NIU_UINT16),            //电流ADC
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_current_100ma, NIU_UINT16),          //电流
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_reference_adc, NIU_UINT16),          //参考基准电压ADC
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_temperature_adc, NIU_UINT16),        //温度采集ADC
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_temperature_01degree, NIU_INT16),    //温度
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_pfc_in, NIU_UINT8),                  //PFC状态
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_work_mode, NIU_UINT8),               //电源模式
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_work_strategy, NIU_UINT8),           //当前执行策略号
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_work_phases, NIU_UINT8),             //当前充电阶段
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_en_out, NIU_UINT8),                  //使能信号状态
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_mos_out, NIU_UINT8),                 //输出MOS状态
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_goalvoltage_100mv, NIU_UINT16),      //输出目标电压
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltagepwm_0001percent, NIU_UINT16), //电压PWM占空比
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_goalcurrent_100ma, NIU_UINT16),      //输出目标电流
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_currentpwm_0001percent, NIU_UINT16), //电流PWM占空比
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_charge_voltage_p_100mv, NIU_UINT16), //充电保护电压
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_charge_current_p_100ma, NIU_UINT16), //充电保护电流
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved0, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved1, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved2, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved3, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved4, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved5, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved6, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved7, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved8, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved9, NIU_UINT8),                  //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved10, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved11, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved12, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved13, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved14, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved15, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved16, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved17, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved18, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_software_version, NIU_STRING),       //软件版本
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_hardware_version, NIU_STRING),       //硬件版本
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_frequencygrade_10, NIU_UINT8),       //PWM输出频率等级
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_temperature_p_01degree, NIU_INT16),  //温度保护值
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_temperature_r_01degree, NIU_INT16),  //撤销温度保护值
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_outvoltage_adj, NIU_UINT16),         //输出电压校准
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_outcurrent_adj, NIU_UINT16),         //输出电流校准
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_involtage_adj, NIU_UINT16),          //电压采集校准
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_incurrent_adj, NIU_UINT16),          //电流采集校准
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltagepwm_p_001percent, NIU_INT16), //电压控制P
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltagepwm_i_001percent, NIU_INT16), //电压控制I
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltagepwm_d_001percent, NIU_INT16), //电压控制D
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_voltage_p_100mv, NIU_UINT16),        //电源保护电压
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_currentpwm_p_001percent, NIU_INT16), //电流控制P
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_currentpwm_i_001percent, NIU_INT16), //电流控制I
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_currentpwm_d_001percent, NIU_INT16), //电流控制D
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_current_p_100ma, NIU_UINT16),        //电源保护电流
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved19, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved20, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved21, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved22, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved23, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved24, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved25, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved26, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved27, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved28, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved29, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved30, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved31, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved32, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.reserved33, NIU_UINT8),                 //reserved
    NIU_DATA_ITEM(g_niu_data.qc.data.qc_charge_sn_code, NIU_STRING)          //SN
};

static NIU_DATA_VALUE niu_data_value_cpm[] =
{
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_dev_name, NIU_STRING),                         //码表名称
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_dev_sw_verno, NIU_STRING),                     //码表软件版本
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_dev_hw_verno, NIU_STRING),                     //码表硬件版本
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_dev_space, NIU_UINT32),                        //码表总空间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_dev_free_space, NIU_UINT32),                   //码表剩余空间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_dev_model, NIU_STRING),                        //码表型号
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_dev_sn, NIU_STRING),                           //码表序列号
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_dev_mac, NIU_STRING),                          //码表MAC地址
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_ble_state, NIU_UINT8),                //BLE状态
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_gps_state, NIU_UINT8),                //GPS状态
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_bike_mode, NIU_UINT8),                //骑行模式
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_bike_state, NIU_UINT16),              //车辆在使用过程中的状态设置
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_brightness, NIU_UINT8),               //亮度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_backlight, NIU_UINT8),                //背光时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_colormode, NIU_UINT8),                //颜色模式
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_alert_volume, NIU_UINT8),             //提示音音量
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_key_volume, NIU_UINT8),               //按键音音量
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_data_config, NIU_UINT8),              //踏频数据记录配置
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_auto_poweroff, NIU_UINT8),            //自动关机
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_time_format, NIU_UINT8),              //时间制式
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_language, NIU_UINT8),                 //语言
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_mileage_unit, NIU_UINT8),             //里程单位
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_temperature_unit, NIU_UINT8),         //温度单位
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_weight_unit, NIU_UINT8),              //重量单位
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_bound_bsc_id, NIU_UINT32),            //绑定踏频速度计ID
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_bound_bs_id, NIU_UINT32),             //绑定速度计ID
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_bound_bc_id, NIU_UINT32),             //绑定踏频计ID
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_bound_hrm_id, NIU_UINT32),            //绑定心率计ID
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_settings_bound_bpwr_id, NIU_UINT32),           //绑定功率计ID
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_user_name, NIU_STRING),                        //用户名
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_user_sex, NIU_UINT8),                          //性别
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_user_height, NIU_UINT8),                       //身高
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_user_weight, NIU_FLOAT),                       //体重
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_user_birthday, NIU_UINT32),                    //出生日期
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_user_cur_id, NIU_UINT32),                       //当前用户ID
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_user_count, NIU_UINT8),                        //用户个数
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_cur_id, NIU_UINT32),                       //当前车辆ID
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_count, NIU_UINT8),                        //车辆数
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_name, NIU_STRING),                        //车辆名称
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_size, NIU_UINT8),                         //车辆大小类型
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_suit, NIU_UINT8),                         //套件配置   标配 高配
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_display, NIU_UINT8),                      //骑行显示
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_frame_size, NIU_UINT16),                  //车架尺寸
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_wheelsize, NIU_UINT16),                   //车轮尺寸
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_handlelength, NIU_FLOAT),                 //曲柄长度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_weight, NIU_FLOAT),                       //车重
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bike_mileage, NIU_FLOAT),                      //车辆里程
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_total_cyc_count, NIU_UINT32),                  //当前骑行数据标志
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_del_cyc_count, NIU_UINT32),                     //当前骑行数据删除到哪标志
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_start_time, NIU_UINT32),               //骑行开始时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_stop_time, NIU_UINT32),                //骑行结束时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_latitude, NIU_FLOAT),                 //纬度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_longitude, NIU_FLOAT),                //经度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_mileage, NIU_FLOAT),                   //骑行里程
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_duration, NIU_UINT32),                 //骑行时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_v_max, NIU_FLOAT),                     //骑行最大速度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_v_ave, NIU_FLOAT),                     //骑行平均速度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_v, NIU_FLOAT),                         //骑行实时速度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_rpm_max, NIU_UINT16),                  //骑行最大踏频
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_rpm_ave, NIU_FLOAT),                  //骑行平均踏频
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_rpm, NIU_UINT16),                      //骑行实时踏频
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_hrm_max, NIU_UINT8),                   //骑行最大心率
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_hrm_ave, NIU_FLOAT),                   //骑行平均心率
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_hrm, NIU_UINT8),                       //骑行实时心率
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_bpwr_max, NIU_FLOAT),                  //最大功率
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_bpwr_ave, NIU_FLOAT),                  //平均功率
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_bpwr, NIU_FLOAT),                      //实时功率
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_elevation, NIU_FLOAT),                 //海拔
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_rising_distance, NIU_FLOAT),           //上升距离
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_decline_distance, NIU_FLOAT),          //下降距离
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_max_left_angle, NIU_UINT8),            //最大左倾角
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_max_right_angle, NIU_UINT8),           //最大右倾角
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_kcal, NIU_FLOAT),                      //消耗热量
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_temperature, NIU_FLOAT),               //温度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_als, NIU_FLOAT),                       //环境光数值
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_pressure, NIU_FLOAT),                  //大气压强
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_sub_index, NIU_UINT8),                 //当前骑行段数
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_sub_start_time, NIU_UINT32),           //分段骑行开始时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_sub_end_time, NIU_UINT32),             //分段骑行结束时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_sub_mileage, NIU_FLOAT),               //分段骑行里程
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_sub_v_ave, NIU_FLOAT),                 //分段骑行平均速度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_start_time, NIU_UINT32),                //个人记录-开始于
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_mileage, NIU_FLOAT),                //个人记录-最远距离
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_duration, NIU_UINT32),              //个人记录-最久骑行时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_fastest_10km_duration, NIU_UINT32),     //个人记录-最快10公里骑行时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_v, NIU_FLOAT),                      //个人记录-最大速度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_rpm, NIU_UINT16),                   //个人记录-最大踏频
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_hrm, NIU_UINT8),                    //个人记录-最大心率
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_bpwr, NIU_FLOAT),                   //个人记录-最大功率
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_angle, NIU_UINT8),                  //个人记录-最大倾角
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_rising_distance, NIU_FLOAT),        //个人记录-最大上升距离
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_decline_distance, NIU_FLOAT),       //个人记录-最大下降距离
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_record_max_kcal, NIU_FLOAT),                   //个人记录-最大消耗热量
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cumulative_start_time, NIU_UINT32),            //累计骑行总起始时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cumulative_total_cycling_counts, NIU_UINT32),  //累计骑行次数
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cumulative_total_mileage, NIU_FLOAT),          //累计骑行总里程
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cumulative_total_duration, NIU_UINT32),        //累计骑行总时间
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cumulative_total_rising_distance, NIU_FLOAT),  //累计总上升距离
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cumulative_total_decline_distance, NIU_FLOAT), //累计总下降距离
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cumulative_total_kcal, NIU_FLOAT),             //累计总消耗热量
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_timestamp, NIU_UINT32),                        //时间戳
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_b9watch_server_cmd, NIU_UINT32),               //服务器命令解析
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_ant_dev_conn_state, NIU_UINT16),              //ANT设备连接状态
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_nordic_verno, NIU_STRING),                    //nordic设备版本号
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_state, NIU_UINT8),                    //骑行状态
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_add_user, NIU_STRING),                        //添加用户信息
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_delete_user, NIU_UINT8),                      //删除用户信息
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_add_bike, NIU_STRING),                        //添加车辆信息
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_delete_bike, NIU_UINT8),                      //删除车辆信息
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_bound_app_state, NIU_UINT8),                  //码表和app绑定
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_conn_ble_name, NIU_STRING),                   //连接的蓝牙名字
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_gprs_data_state, NIU_UINT8),                  //蜂窝数据开关
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cyc_history_auto_upload, NIU_UINT8),          //骑行记录自动上传
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cyc_history_auto_delete, NIU_UINT8),          //自动删除历史旧数据
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_get_bike_list, NIU_STRING),                   //获取车辆列表
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_angle, NIU_UINT8),                    //当前骑行倾角
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_cycling_slope, NIU_UINT8),                    //当前骑行坡度
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_app_read_cyc_counts, NIU_UINT32),             //APP已经读取多少条骑行数据
    NIU_DATA_ITEM(g_niu_data.cpm.data.cpm_current_cycling_index, NIU_UINT32)            //当前骑行记录唯一标识
};

void niu_data_value_trace(NIU_DATA_ADDR_BASE base, NUINT32 id, NIU_DATA_VALUE *value)
{
    LOG_D("base:%u, id:%u, value:%p", base, id, value);
    if (value && value->addr && value->len)
    {
        NCHAR title[20] = {0};
        snprintf(title, 20, "D%u_%u", base, id);
        LOG_D("D%u_%u: value->addr:%p, value->type:%u, value->len:%u", base, id, value->addr, value->type, value->len);
        niu_trace_dump_hex(title, value->addr, value->len);
    }
}

JSTATIC JBOOL niu_data_check()
{
    NUINT32 db_count = 0;
    NUINT32 bms_count = 0;
    NUINT32 ecu_count = 0;
    NUINT32 lcu_count = 0;
    NUINT32 alarm_count = 0;
    NUINT32 foc_count = 0;
    NUINT32 lock_count = 0;
    NUINT32 bcs_count = 0;
    NUINT32 bms2_count = 0;
    NUINT32 qc_count = 0;
    NUINT32 emcu_count = 0;
    NUINT32 cpm_count = 0;

    JBOOL check_ret = JTRUE;

    db_count = sizeof(niu_data_value_db) / sizeof(NIU_DATA_VALUE);
    bms_count = sizeof(niu_data_value_bms) / sizeof(NIU_DATA_VALUE);
    ecu_count = sizeof(niu_data_value_ecu) / sizeof(NIU_DATA_VALUE);
    lcu_count = sizeof(niu_data_value_lcu) / sizeof(NIU_DATA_VALUE);
    alarm_count = sizeof(niu_data_value_alarm) / sizeof(NIU_DATA_VALUE);
    foc_count = sizeof(niu_data_value_foc) / sizeof(NIU_DATA_VALUE);
    lock_count = sizeof(niu_data_value_lockboard) / sizeof(NIU_DATA_VALUE);
    bcs_count = sizeof(niu_data_value_bcs) / sizeof(NIU_DATA_VALUE);
    bms2_count = sizeof(niu_data_value_bms2) / sizeof(NIU_DATA_VALUE);
    qc_count = sizeof(niu_data_value_qc) / sizeof(NIU_DATA_VALUE);
    emcu_count = sizeof(niu_data_value_emcu) / sizeof(NIU_DATA_VALUE);
    cpm_count = sizeof(niu_data_value_cpm) / sizeof(NIU_DATA_VALUE);

    
    check_ret = (db_count == NIU_ID_DB_MAX) && (bms_count == NIU_ID_BMS_MAX) &&
                (ecu_count == NIU_ID_ECU_MAX) && (lcu_count == NIU_ID_LCU_MAX) &&
                (alarm_count == NIU_ID_ALARM_MAX) && (foc_count == NIU_ID_FOC_MAX) &&
                (bcs_count == NIU_ID_BCS_MAX) && (bms2_count == NIU_ID_BMS2_MAX) &&
                (qc_count == NIU_ID_QC_MAX) && (emcu_count == NIU_ID_EMCU_MAX) &&
                (cpm_count == NIU_ID_CPM_MAX);
    if (!check_ret)
    {
        LOG_D("check_ret:%d\n", check_ret);
        LOG_D("db_count:%u,db_max:%u, db_size:%u", db_count, NIU_ID_DB_MAX, sizeof(NDATA_VALUE_DB));
        LOG_D("bms_count:%u,bms_max:%u, db_size:%u", bms_count, NIU_ID_BMS_MAX, sizeof(NDATA_VALUE_BMS));
        LOG_D("ecu_count:%u,ecu_max:%u, db_size:%u", ecu_count, NIU_ID_ECU_MAX, sizeof(NDATA_VALUE_ECU));
        LOG_D("lcu_count:%u,lcu_max:%u, db_size:%u", lcu_count, NIU_ID_LCU_MAX, sizeof(NDATA_VALUE_LCU));
        LOG_D("alarm_count:%u,alarm_max:%u, db_size:%u", alarm_count, NIU_ID_ALARM_MAX, sizeof(NDATA_VALUE_ALARM));
        LOG_D("foc_count:%u,foc_max:%u, db_size:%u", foc_count, NIU_ID_FOC_MAX, sizeof(NDATA_VALUE_FOC));
        LOG_D("lock_count:%u,lock_max:%u, db_size:%u", lock_count, NIU_ID_LOCKBOARD_MAX, sizeof(NDATA_VALUE_LOCKBOARD));
        LOG_D("bcs_count:%u,bcs_max:%u, db_size:%u", bcs_count, NIU_ID_BCS_MAX, sizeof(NDATA_VALUE_BCS));
        LOG_D("bms2_count:%u,bms2_max:%u, db_size:%u", bms2_count, NIU_ID_BMS2_MAX, sizeof(NDATA_VALUE_BMS2));
        LOG_D("qc_count:%u,qc_max:%u, db_size:%u", qc_count, NIU_ID_QC_MAX, sizeof(NDATA_VALUE_QC));
        LOG_D("emcu_count:%u,emcu_max:%u, db_size:%u", emcu_count, NIU_ID_EMCU_MAX, sizeof(NDATA_VALUE_EMCU));
        LOG_D("cpm_count:%u,cpm_max:%u, db_size:%u", cpm_count, NIU_ID_CPM_MAX, sizeof(NDATA_VALUE_CPM));
    }
    return check_ret;
}

JSTATIC JBOOL niu_ecu_data_init()
{
    unsigned int value = 0;
    JBOOL ret = JTRUE;
    struct tm tm;
    char fota_version[48];

    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_1, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_1].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_1].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_2, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_2].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_2].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_3, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_3].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_3].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_4, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_4].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_4].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_5, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_5].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_5].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_6, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_6].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_6].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_7, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_7].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_7].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_8, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_8].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_8].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_9, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_9].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_9].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_10, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_10].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_10].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_11, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_11].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_11].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_12, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_12].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_12].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_13, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_13].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_13].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_14, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_14].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_14].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_15, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_15].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_15].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_16, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_16].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_16].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_17, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_17].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_17].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_18, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_18].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_18].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_19, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_19].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_19].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_20, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_20].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_20].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_21, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_21].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_21].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_22, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_22].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_22].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_23, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_23].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_23].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_24, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_24].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_24].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_25, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_25].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_25].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_26, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_26].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_26].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_27, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_27].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_27].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_28, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_28].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_28].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_29, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_29].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_29].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_30, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_30].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_30].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_31, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_31].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_31].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_32, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_32].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_32].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_33, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_33].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_33].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_34, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_34].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_34].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_35, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_35].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_35].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_36, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_36].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_36].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_37, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_37].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_37].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_38, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_38].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_38].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_39, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_39].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_39].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_40, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_40].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_40].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_41, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_41].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_41].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_42, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_42].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_42].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_43, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_43].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_43].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_44, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_44].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_44].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_45, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_45].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_45].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_46, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_46].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_46].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_47, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_47].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_47].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_48, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_48].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_48].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_49, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_49].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_49].len);
    niu_data_rw_read(NIU_DATA_RW_TEMPLATE_50, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_50].addr, niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_50].len);
    
    cmd_imsi_info_get(g_niu_data.ecu.data.imsi, sizeof(g_niu_data.ecu.data.imsi));
    cmd_iccid_info_get(g_niu_data.ecu.data.iccid, sizeof(g_niu_data.ecu.data.iccid));
    cmd_imei_info_get(g_niu_data.ecu.data.imei_4g, sizeof(g_niu_data.ecu.data.imei_4g));
    cmd_imei_info_get(g_niu_data.ecu.data.imei, sizeof(g_niu_data.ecu.data.imei));

    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SOFT_VER, NIU_DATA_DEFAULT_SW_VER, jmin(8, strlen(NIU_DATA_DEFAULT_SW_VER)));
    //LOG_D("imei:%s, imsi:%s, issci: %s", g_niu_data.ecu.data.imei, g_niu_data.ecu.data.imsi, g_niu_data.ecu.data.iccid);
    
    memset(&tm, 0, sizeof(struct tm));
    strptime(NIU_BUILD_TIMESTAMP, "%Y-%m-%d %H:%M:%S", &tm);
    snprintf(fota_version, sizeof(fota_version), "EC25_%s_%04d_%02d%02d_%02d%02d%02d", 
                            NIU_VERSION, tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    LOG_D("FOTA_VERSION: %s", fota_version);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_ADUPS_VER, fota_version, sizeof(fota_version));

    value = time(NULL);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SERVER_RESET_4G_TIME, (NUINT8 *)&value, 4);
	
    strncpy(niu_data_value_ecu[NIU_ID_ECU_ECU_ADUPS_VER].addr, NIU_VERSION, niu_data_value_ecu[NIU_ID_ECU_ECU_ADUPS_VER].len);

    return ret;
}

JSTATIC JVOID niu_templ_data_init()
{
    //LOG_D("niu_templ_data_init");
    NUINT32 i = 0;
    for (i = 0; i < NIU_SERVER_UPL_CMD_NUM; i++)
    {
        //NCHAR prompt[32] = {0};
        //snprintf(prompt, 32, "template%d, ecu_index:%d ", i, NIU_ID_ECU_UPLOADING_TEMPLATE_1 + i);
        niu_data_rw_template_read(i, &g_niu_upl_cmd[i]);
        //niu_trace_dump_data(prompt, (NUINT8 *)&g_niu_upl_cmd[i], sizeof(NIU_SERVER_UPL_TEMPLATE));
        if(i < 20)
        {
            memcpy(niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_1 + i].addr, &g_niu_upl_cmd[i].packed_name, 2);
        }
        else
        {
            memcpy(niu_data_value_ecu[NIU_ID_ECU_UPLOADING_TEMPLATE_20 + i - 20].addr, &g_niu_upl_cmd[i].packed_name, 2);
        }
    }
}

void niu_ecu_data_sync()
{
    // sync soft_ver, imsi, iccid, imei to mcu
    mcu_send_write(NIU_ECU, NIU_ID_ECU_ECU_SOFT_VER, niu_data_value_ecu[NIU_ID_ECU_ECU_SOFT_VER].addr, niu_data_value_ecu[NIU_ID_ECU_ECU_SOFT_VER].len);
    //mcu_send_write(NIU_ECU, NIU_ID_ECU_ECU_HARD_VER, niu_data_value_ecu[NIU_ID_ECU_ECU_HARD_VER].addr, niu_data_value_ecu[NIU_ID_ECU_ECU_HARD_VER].len);

    //mcu_send_write(NIU_ECU, NIU_ID_ECU_EID, niu_data_value_ecu[NIU_ID_ECU_EID].addr, niu_data_value_ecu[NIU_ID_ECU_EID].len);
    mcu_send_write(NIU_ECU, NIU_ID_ECU_IMSI, niu_data_value_ecu[NIU_ID_ECU_IMSI].addr, niu_data_value_ecu[NIU_ID_ECU_IMSI].len);
    mcu_send_write(NIU_ECU, NIU_ID_ECU_ICCID, niu_data_value_ecu[NIU_ID_ECU_ICCID].addr, niu_data_value_ecu[NIU_ID_ECU_ICCID].len);
    mcu_send_write(NIU_ECU, NIU_ID_ECU_IMEI, niu_data_value_ecu[NIU_ID_ECU_IMEI].addr, niu_data_value_ecu[NIU_ID_ECU_IMEI].len);
    mcu_send_write(NIU_ECU, NIU_ID_ECU_4G_IMEI, niu_data_value_ecu[NIU_ID_ECU_4G_IMEI].addr, niu_data_value_ecu[NIU_ID_ECU_4G_IMEI].len);
    //mcu_send_write(NIU_ECU, NIU_ID_ECU_UTC, niu_data_value_ecu[NIU_ID_ECU_UTC].addr, niu_data_value_ecu[NIU_ID_ECU_UTC].len);
    // mcu_send_read(NIU_ECU, NIU_ID_ECU_CAR_STA);
    // mcu_send_read(NIU_ECU, NIU_ID_ECU_ECU_BATTARY);
    // mcu_send_read(NIU_EMCU, NIU_ID_EMCU_EMCU_SOFT_VER);

    // mcu_send_read(NIU_EMCU, NIU_ID_ECU_ECU_AVG_LATITUDE);
    // mcu_send_read(NIU_EMCU, NIU_ID_ECU_ECU_AVG_LONGITUDE);
    // mcu_send_read(NIU_EMCU, NIU_ID_ECU_ECU_V35_CONFIG);
    // mcu_send_read(NIU_EMCU, NIU_ID_ECU_AUTO_CLOSE_TIME);
    int total = NIU_ID_ECU_MAX;
    int max_count = 0x20;
    while(1)
    {
        if(total > max_count)
        {
            mcu_send_seqread(NIU_ECU, total - max_count, max_count);
            total -= max_count;
        }
        else
        {
            mcu_send_seqread(NIU_ECU, 0, total);
            total = 0;
            break;
        }
        usleep(100);
    }
    LOG_D("send seq read to mcu[total fields: %d]", total);
}

JBOOL niu_data_init()
{
    JBOOL ret;
//    NUINT16 ecu_cfg = 0;

    memset(&g_niu_upl_cmd, 0, (sizeof(NIU_SERVER_UPL_TEMPLATE) * NIU_SERVER_UPL_CMD_NUM));

    ret = niu_data_check();
    if (!ret)
    {
        LOG_D("jwsdk niu data check failed.");
        return JFALSE;
    }

    ret = niu_data_config_init();
    
    /*  
        if json config file not exist, set ecu_cfg with NIU_ECU_CONFIG_NO_PROF flag
        else reset ecu_cfg NIU_ECU_CONFIG_NO_PROF flag.
    */
   #if 0
    NIU_DATA_VALUE *value = niu_data_read(NIU_ECU, NIU_ID_ECU_ECU_CFG);
    if(value)
    {
        memcpy(&ecu_cfg, value->addr, value->len);
        if(ret)
        {
            ecu_cfg &= ~NIU_ECU_CONFIG_NO_PROF;
        }
        else
        {
            ecu_cfg |= NIU_ECU_CONFIG_NO_PROF;
        }
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_CFG, (NUINT8*)&ecu_cfg, sizeof(NUINT16));
    }
    #endif

    niu_ecu_data_init();
    niu_templ_data_init();
    return JTRUE;
}

/**
 * update g_config_rw value, base and id are from the niu_data 
 * @base: in niu_data, means device type
 * @id: the device's field
 * @value: update value
 * @len: length of value
 *  
 */
JSTATIC JVOID niu_data_rw_update(NIU_DATA_ADDR_BASE base, NUINT32 id, NUINT8 *value, NUINT32 len)
{
    NIU_DATA_RW_INDEX index = 0xFFFF;

    if (NIU_ECU == base)
    {
        switch (id)
        {
        case NIU_ID_ECU_UPLOADING_TEMPLATE_1:
            index = NIU_DATA_RW_TEMPLATE_1;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_2:
            index = NIU_DATA_RW_TEMPLATE_2;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_3:
            index = NIU_DATA_RW_TEMPLATE_3;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_4:
            index = NIU_DATA_RW_TEMPLATE_4;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_5:
            index = NIU_DATA_RW_TEMPLATE_5;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_6:
            index = NIU_DATA_RW_TEMPLATE_6;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_7:
            index = NIU_DATA_RW_TEMPLATE_7;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_8:
            index = NIU_DATA_RW_TEMPLATE_8;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_9:
            index = NIU_DATA_RW_TEMPLATE_9;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_10:
            index = NIU_DATA_RW_TEMPLATE_10;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_11:
            index = NIU_DATA_RW_TEMPLATE_11;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_12:
            index = NIU_DATA_RW_TEMPLATE_12;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_13:
            index = NIU_DATA_RW_TEMPLATE_13;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_14:
            index = NIU_DATA_RW_TEMPLATE_14;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_15:
            index = NIU_DATA_RW_TEMPLATE_15;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_16:
            index = NIU_DATA_RW_TEMPLATE_16;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_17:
            index = NIU_DATA_RW_TEMPLATE_17;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_18:
            index = NIU_DATA_RW_TEMPLATE_18;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_19:
            index = NIU_DATA_RW_TEMPLATE_19;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_20:
            index = NIU_DATA_RW_TEMPLATE_20;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_21:
            index = NIU_DATA_RW_TEMPLATE_21;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_22:
            index = NIU_DATA_RW_TEMPLATE_22;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_23:
            index = NIU_DATA_RW_TEMPLATE_23;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_24:
            index = NIU_DATA_RW_TEMPLATE_24;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_25:
            index = NIU_DATA_RW_TEMPLATE_25;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_26:
            index = NIU_DATA_RW_TEMPLATE_26;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_27:
            index = NIU_DATA_RW_TEMPLATE_27;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_28:
            index = NIU_DATA_RW_TEMPLATE_28;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_29:
            index = NIU_DATA_RW_TEMPLATE_29;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_30:
            index = NIU_DATA_RW_TEMPLATE_30;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_31:
            index = NIU_DATA_RW_TEMPLATE_31;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_32:
            index = NIU_DATA_RW_TEMPLATE_32;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_33:
            index = NIU_DATA_RW_TEMPLATE_33;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_34:
            index = NIU_DATA_RW_TEMPLATE_34;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_35:
            index = NIU_DATA_RW_TEMPLATE_35;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_36:
            index = NIU_DATA_RW_TEMPLATE_36;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_37:
            index = NIU_DATA_RW_TEMPLATE_37;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_38:
            index = NIU_DATA_RW_TEMPLATE_38;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_39:
            index = NIU_DATA_RW_TEMPLATE_39;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_40:
            index = NIU_DATA_RW_TEMPLATE_40;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_41:
            index = NIU_DATA_RW_TEMPLATE_41;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_42:
            index = NIU_DATA_RW_TEMPLATE_42;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_43:
            index = NIU_DATA_RW_TEMPLATE_43;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_44:
            index = NIU_DATA_RW_TEMPLATE_44;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_45:
            index = NIU_DATA_RW_TEMPLATE_45;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_46:
            index = NIU_DATA_RW_TEMPLATE_46;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_47:
            index = NIU_DATA_RW_TEMPLATE_47;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_48:
            index = NIU_DATA_RW_TEMPLATE_48;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_49:
            index = NIU_DATA_RW_TEMPLATE_49;
            break;
        case NIU_ID_ECU_UPLOADING_TEMPLATE_50:
            index = NIU_DATA_RW_TEMPLATE_50;
            break;
        default:
            break;
        }
    }
    else if (NIU_BMS == base)
    {
        switch (id)
        {
#ifdef __NIU_530_SUPPORT__
        case NIU_ID_BMS_C_CONT:
            index = NIU_DATA_RW_BMS_C_CONT;
            break;
        case NIU_ID_BMS_RATED_VLT:
            index = NIU_DATA_RW_BMS_RATED_VLT;
            break;
#endif
        }
    }

    if((NIU_ECU == base) && (NIU_ID_ECU_ECU_CMD == id)) //in niu_data_write_value function, the value have written in
    {
        if(g_niu_data.ecu.data.ecu_cmd & 0x0001)    //factory reset
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x0001);
            printf("factory reset!!!!\n");
            NCHAR imei[20] = {0};
            NCHAR rw_file_path[255];

			cmd_imei_info_get(imei, 19);
			snprintf(rw_file_path, RW_FILE_PATH_LEN_MAX, "%s/%s_rw.dat", RW_FILE_PREFIX, imei);
            if(ql_com_file_exist(rw_file_path) == 0)
            {
                tmr_handler_tmr_del(TIMER_ID_NIU_UPDATE_RW_DATA);
                remove(rw_file_path);
                sync();
                LOG_D("remove %s success", rw_file_path);
            }
            //reset mcu config use config.json
            if(ql_com_file_exist(CONFIG_UPGRADE_FILE_EC25) == 0)
            {
                LOG_D("file exist, %s", CONFIG_UPGRADE_FILE_EC25);
                int ret, mask = 0;
                int retry = 50; //timeout 5 seconds
                niu_data_config_sync_flag_set(1);
                niu_data_config_sync_mask_reset();
                //Synchronize all fields in v3 table which contained in configuration files to MCU
                ret = niu_config_data_sync_to_niu_data(CONFIG_UPGRADE_FILE_EC25);
                if(ret)
                {
                    LOG_E("sync json config fields to mcu failed.");
                    return;
                }
                sleep(1);
                niu_data_all_config_fields_req();

                while(retry > 0)
                {
                    mask = niu_data_config_sync_mask_get();
                    if(mask == NIU_MASK_SYNC_FLAG_ALL)
                    {
                        LOG_D("sync config success, mask=0x%04x", mask);
                        break;
                    }
                    usleep(100*1000);
                    retry--;
                }
                niu_data_config_sync_flag_set(0);
                
                if(retry == 0)
                {
                    LOG_E("Synchronize all config fields to MCU failed[mask: 0x%04x].", mask);
                }
                LOG_D("restart app.");
                sleep(1);
                exit(1);
            }
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0002)    //FOTA
        {
            printf("%s\n", __FUNCTION__);
            g_fota_method = FOTA_CHECK_METHOD_MANUAL;
            wakeup_fota_thread(FOTA_CHECK);
            g_niu_data.ecu.data.ecu_cmd &= (~0x0002);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0004)    //Reset
        {
            LOG_D("recv reboot from IOT.");
            mcu_send_write(NIU_ECU, NIU_ID_ECU_ECU_CMD, (NUINT8*)&g_niu_data.ecu.data.ecu_cmd, sizeof(NUINT16));
            g_niu_data.ecu.data.ecu_cmd &= (~0x0004);
            sleep(3);
            Ql_Powerdown(1);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0008)    //Start ACC
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x0008);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0010)    //Stop ACC
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x0010);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0020)    //SMT test
        {
            start_niu_smt_test(0x0020);   //juson.zhang-2019/04/09: add for niu smt test
            //g_niu_data.ecu.data.ecu_cmd &= (~0x0020);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0040)    //GSensor calibration
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x0040);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0080)    //Wake up the system
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x0080);
            //niu_work_mode_do_active();
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0100)    //clear the PDU cache
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x0100);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0200)    //Open GPS power supply
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x0200);
            g_niu_data.ecu.data.ecu_cfg &= (~0x1000);
            //niu_data_rw_write(NIU_DATA_RW_ECU_CFG, (NUINT8 *)&g_niu_data.ecu.data.ecu_cfg, 2);
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_CFG, (NUINT8 *)&g_niu_data.ecu.data.ecu_cfg, 2);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0400)    //Close GPS power supply
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x0400);
            g_niu_data.ecu.data.ecu_cfg |= (0x1000);
            //niu_data_rw_write(NIU_DATA_RW_ECU_CFG, (NUINT8 *)&g_niu_data.ecu.data.ecu_cfg, 2);
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_CFG, (NUINT8 *)&g_niu_data.ecu.data.ecu_cfg, 2);
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x0800)    //Special GSensor calibration
        {
            start_niu_smt_test(0x800);   //juson.zhang-2019/04/09: add for niu smt test
            // g_niu_data.ecu.data.ecu_cmd &= (~0x0200);    //set the open gps power supply bit 0
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x1000)    //Open GPS transparent transmission
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x2000);  //
            g_gps_upload_flag = 1;
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x2000)    //Close GPS transparent transmission
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x1000);
            g_gps_upload_flag = 0;
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x4000)    //Open log upload
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x8000);
            g_log_upload_flag = 1;
        }
        if(g_niu_data.ecu.data.ecu_cmd & 0x8000)    //Close log upload
        {
            g_niu_data.ecu.data.ecu_cmd &= (~0x4000);
            g_log_upload_flag = 0;
        }
    }
    if((NIU_ECU == base) && (NIU_ID_ECU_ECU_CFG == id))
    {
        if(g_niu_data.ecu.data.ecu_cfg & 0x0004)    //no prof file
        {
            error_upload(NIU_ERROR_NO_PROF);
            ql_leds_gpio_ctrl("net_status", LEDS_STATUS_FLICKER, 100, 1000);
        }
    }
    if (index < NIU_DATA_RW_MAX)
    {
        niu_data_rw_write(index, value, len);    //write the param to data table
    }
    if ((NIU_ECU == base && NIU_ID_ECU_ECU_CFG == id) || (NIU_ECU == base && NIU_ID_ECU_ECU_STATE == id) || 
        (NIU_BMS == base && NIU_ID_BMS_BMS_BAT_STA_RT == id) || (NIU_FOC == base && NIU_ID_FOC_FOC_COTLSTA == id) || 
        (NIU_ALARM == base && NIU_ID_ALARM_ALARM_STATE == id) || (NIU_ECU == base && NIU_ID_ECU_CAR_STA == id))
    {
        LOG_V("states update, base: %d, id: %d", base, id);
        //tmr_handler_tmr_add(TIMER_ID_NIU_UPDATE_TEMPLATE, niu_update_trigger_template, NULL, 1000, 0);
        niu_update_trigger_template();
    }
    return;
}

JBOOL niu_data_write(NIU_DATA_ADDR_BASE base, NUINT32 id, NIU_DATA_VALUE *value)
{
    JBOOL ret = JFALSE;
    NUINT8 write_data[100] = {0};
    NUINT8 write_size = 0;

    niu_data_value_trace(base, id, value);

    if (value && value->addr && value->len)
    {
        switch (base)
        {
        case NIU_DB:
            if (id < NIU_ID_DB_MAX && value->len <= niu_data_value_db[id].len && value->type == niu_data_value_db[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_db[id].len;
                memcpy(niu_data_value_db[id].addr, write_data, write_size);
            }
            break;
        case NIU_FOC:
            if (id < NIU_ID_FOC_MAX && value->len <= niu_data_value_foc[id].len && value->type == niu_data_value_foc[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_foc[id].len;
                memcpy(niu_data_value_foc[id].addr, write_data, write_size);
            }
            break;
        case NIU_LOCKBOARD:
            if (id < NIU_ID_LOCKBOARD_MAX && value->len <= niu_data_value_lockboard[id].len && value->type == niu_data_value_lockboard[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_lockboard[id].len;
                memcpy(niu_data_value_lockboard[id].addr, write_data, write_size);
            }
            break;
        case NIU_BMS:
            if (id < NIU_ID_BMS_MAX && value->len <= niu_data_value_bms[id].len && value->type == niu_data_value_bms[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_bms[id].len;
                memcpy(niu_data_value_bms[id].addr, write_data, write_size);
            }
            break;
        case NIU_ECU:
            if (id < NIU_ID_ECU_MAX && value->len <= niu_data_value_ecu[id].len && value->type == niu_data_value_ecu[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_ecu[id].len;
                memcpy(niu_data_value_ecu[id].addr, write_data, write_size);
            }
            break;
        case NIU_ALARM:
            if (id < NIU_ID_ALARM_MAX && value->len <= niu_data_value_alarm[id].len && value->type == niu_data_value_alarm[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_alarm[id].len;
                memcpy(niu_data_value_alarm[id].addr, write_data, write_size);
            }
            break;
        case NIU_LCU:
            if (id < NIU_ID_LCU_MAX)
            {
                if (id < NIU_ID_LCU_MAX && value->len <= niu_data_value_lcu[id].len && value->type == niu_data_value_lcu[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_lcu[id].len;
                    memcpy(niu_data_value_lcu[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_BCS:
            if (id < NIU_ID_BCS_MAX)
            {
                if (id < NIU_ID_BCS_MAX && value->len <= niu_data_value_bcs[id].len && value->type == niu_data_value_bcs[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_bcs[id].len;
                    memcpy(niu_data_value_bcs[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_BMS2:
            if (id < NIU_ID_BMS2_MAX)
            {
                if (id < NIU_ID_BMS2_MAX && value->len <= niu_data_value_bms2[id].len && value->type == niu_data_value_bms2[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_bms2[id].len;
                    memcpy(niu_data_value_bms2[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_QC:
            if (id < NIU_ID_QC_MAX)
            {
                if (id < NIU_ID_QC_MAX && value->len <= niu_data_value_qc[id].len && value->type == niu_data_value_qc[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_qc[id].len;
                    memcpy(niu_data_value_qc[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_EMCU:
            if (id < NIU_ID_EMCU_MAX)
            {
                if (id < NIU_ID_EMCU_MAX && value->len <= niu_data_value_emcu[id].len && value->type == niu_data_value_emcu[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_emcu[id].len;
                    memcpy(niu_data_value_emcu[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_CPM:
            if (id < NIU_ID_CPM_MAX)
            {
                if (id < NIU_ID_CPM_MAX && value->len <= niu_data_value_cpm[id].len && value->type == niu_data_value_cpm[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_cpm[id].len;
                    memcpy(niu_data_value_cpm[id].addr, write_data, write_size);
                }
            }
            break;
        default:
            break;
        }

        if (write_size)
        {
            niu_data_rw_update(base, id, write_data, write_size);
        }
    }

    LOG_D("base:%d, id:%d, ret:%d", base, id, ret);

    return ret;
}

/**
 * write value to g_niu_data from serial
 * NIU_ECU have some fields must write by EC25, so we must have a filter when serial write data to g_niu_data
 * now we list these fields must write by EC25 bellow(given by xiao NIU):
 * 1. base: NIU_ECU, field id: ecu_state
 *          bit offset:
                    (0x00000001) //电门开触发 NIU_TRIGGER_EVENT_ACC_ON
                    (0x00000002) //电门关触发 NIU_TRIGGER_EVENT_ACC_OFF
                    (0x00000004) //外电接入触发 NIU_TRIGGER_EVENT_BATTER_IN
                    (0x00000008) //外电断开触发 NIU_TRIGGER_EVENT_BATTER_OUT
                    (0x00000010) //震动触发 NIU_TRIGGER_EVENT_SHAKE
                    (0x00000020) //震动停止触发 NIU_TRIGGER_EVENT_SHAKE_STOP
                    (0x00000040) //异动触发 NIU_TRIGGER_EVENT_MOVE
                    (0x00000080) //异动恢复触发 NIU_TRIGGER_EVENT_MOVE_STOP
                    (0x00000100) //倾倒触发 NIU_TRIGGER_EVENT_FALL
                    (0x00000200) //倾倒恢复触发 NIU_TRIGGER_EVENT_FALL_STOP
                    (0x00000400) //行驶状态 NIU_TRIGGER_STATE_RUNNING_Y
                    (0x00000800) //非行驶状态 NIU_TRIGGER_STATE_RUNNING_N
                    (0x00001000) //电门开状态 NIU_TRIGGER_STATE_ACC_Y
                    (0x00002000) //电门关状态 NIU_TRIGGER_STATE_ACC_N
                    (0x00004000) //外电接入状态 NIU_TRIGGER_STATE_BATTERY_Y
                    (0x00008000) //外电断开状态 NIU_TRIGGER_STATE_BATTERY_N
                    (0x00040000) //震动状态 NIU_TRIGGER_STATE_SHAKE_Y
                    (0x00080000) //非震动状态 NIU_TRIGGER_STATE_SHAKE_N
                    (0x00100000) //异动状态 NIU_TRIGGER_STATE_MOVE_Y
                    (0x00200000) //非异动状态 NIU_TRIGGER_STATE_MOVE_N
                    (0x00400000) //倾倒状态 NIU_TRIGGER_STATE_FALL_Y
                    (0x00800000) //非倾倒状态 NIU_TRIGGER_STATE_FALL_N
                    (0x04000000) //GPS定位成功 NIU_TRIGGER_STATE_GPS_Y
                    (0x08000000) //GPS定位失败 NIU_TRIGGER_STATE_GPS_N
 * 2. base: NIU_ECU, field id: car_sta
 *          bits offset:
 *                  0x0010 NIU_STATE_SHAKE
                    0x0020 NIU_STATE_MOVE
                    0x0040 NIU_STATE_FALL
                    0x0100 NIU_STATE_GPS_LOCATION
                    0x0200 NIU_STATE_GPS_ON
                    0x4000 NIU_STATE_FOTA
                    0x8000 NIU_STATE_MOVE_EX
                    0x80000 NIU_STATE_MQTT_CONNECTED
    3. base: NIU_ECU, field id: ecu_state2
            bits offset:
                    (0x00000001)  //移动触发  NIU_TRIGGER_ENENT_MOVE_GSENSOR
                    (0x00000002)  //移动停止触发  NIU_TRIGGER_EVENT_MOVE_GSENSOR_STOP
                    (0x00000400)  //被移动状态 NIU_TRIGGER_STATE_MOVE_GSENSOR_Y
                    (0x00000800)  //非被移动状态 NIU_TRIGGER_STATE_MOVE_GSENSOR_N
    4. base: NIU_ECU, field id: car_eqp_cfg
            bits offset:
                    0x10 WIFI_EXSIT
                    0x80 GPS_EXSIT

    5. base: NIU_ECU, field id: car_eqp_state
            bits offset:
                    0x10 WIFI_READABLE
                    0x80 GPS_ON

 * @param: [in] base, device type
 * @param: [in] id, the fields in device
 * @param: [in] value, the value which want to set
 * @param: [in] len, the value length which want to set
 * 
 * @return:
 *  if success return JTRUE, else return JFALSE
 * 
 */
JBOOL  niu_data_write_value_from_stm32(NIU_DATA_ADDR_BASE base, NUINT32 id, NUINT8 *value, NUINT8 size)
{
    JBOOL ret = JFALSE;
    NUINT8 write_data[256] = {0};
    NUINT8 write_size = 0;

    if (value && size)
    {

        switch (base)
        {
        case NIU_DB:
            if (id < NIU_ID_DB_MAX && size <= niu_data_value_db[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_db[id].len;
                memcpy(niu_data_value_db[id].addr, write_data, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_FOC:
            if (id < NIU_ID_FOC_MAX && size <= niu_data_value_foc[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_foc[id].len;
                memcpy(niu_data_value_foc[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_LOCKBOARD:
            if (id < NIU_ID_LOCKBOARD_MAX && size <= niu_data_value_lockboard[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_lockboard[id].len;
                memcpy(niu_data_value_lockboard[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_BMS:
            if (id < NIU_ID_BMS_MAX && size <= niu_data_value_bms[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_bms[id].len;
                memcpy(niu_data_value_bms[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_ECU:
            if (id < NIU_ID_ECU_MAX && size <= niu_data_value_ecu[id].len)
            {

                if (id == NIU_ID_ECU_CAR_STA && size == sizeof(NUINT32))
                {
                    NUINT32 car_state, car_state_reset;
                    memcpy(&car_state, niu_data_value_ecu[id].addr, sizeof(NUINT32));

                    memcpy(&car_state_reset, value, size);
                    //acc callback
                    if((car_state & NIU_STATE_ACC) == JFALSE &&  (car_state_reset & NIU_STATE_ACC ) == JTRUE )
                    {
                        // on
                        niu_acc_callback(JTRUE);
                        printf("%s\n", __FUNCTION__);
                        g_fota_method = FOTA_CHECK_METHOD_AUTO;
                        wakeup_fota_thread(FOTA_CHECK);
                    }
                    else if((car_state & NIU_STATE_ACC) == JTRUE &&  (car_state_reset & NIU_STATE_ACC ) == JFALSE)
                    {
                        // off
                        niu_acc_callback(JFALSE);
                    }
                    //将传过来的值中 由EC25更新的位 置0，保证单片机不会胡乱更新
                    car_state_reset &= (~NIU_MASK_ECU_CAR_STA_EC25);
                    //将总表中 由单片机负责更新的位 置0，保证单片机更新的数据正确
                    car_state &= (~NIU_MASK_ECU_CAR_STA_STM32);

                    car_state |= car_state_reset;

                    write_size = niu_data_value_ecu[id].len;
                    memcpy(write_data, (NUINT8*)&car_state, size);
                    memcpy(niu_data_value_ecu[id].addr, (NUINT8 *)&car_state, write_size);
                    
                    if(niu_car_state_get(NIU_STATE_SHAKE) == JTRUE)
                    {
                        LOG_D("gsensor wakeup handler begin, shake: %d", niu_car_state_get(NIU_STATE_SHAKE));
                        gsensor_wakeup_handler(60);
                    }
                    ret = JTRUE;
                }
                else if (id == NIU_ID_ECU_CAR_EQP_CFG && size == sizeof(NUINT32))
                {
                    NUINT32 eqp_cfg, eqp_cfg_reset;
                    memcpy(&eqp_cfg, niu_data_value_ecu[id].addr, sizeof(NUINT32));
                    memcpy(&eqp_cfg_reset, value, size);

                    eqp_cfg_reset &= (~NIU_MASK_ECU_CAR_EQP_CFG_EC25);
                    eqp_cfg &= (~NIU_MASK_ECU_CAR_EQP_CFG_STM32);
                    eqp_cfg |= eqp_cfg_reset;
                    write_size = niu_data_value_ecu[id].len;
                    memcpy(write_data, (NUINT8*)&eqp_cfg, size);
                    memcpy(niu_data_value_ecu[id].addr, (NUINT8 *)&eqp_cfg, write_size);
                    ret = JTRUE;
                }
                else if (id == NIU_ID_ECU_CAR_EQP_STATE && size == sizeof(NUINT32))
                {
                    NUINT32 eqp_state, eqp_state_reset;
                    memcpy(&eqp_state, niu_data_value_ecu[id].addr, sizeof(NUINT32));
                    memcpy(&eqp_state_reset, value, size);

                    eqp_state_reset &= (~NIU_MASK_ECU_CAR_EQP_STATE_EC25);
                    eqp_state &= (~NIU_MASK_ECU_CAR_EQP_STATE_STM32);
                    eqp_state |= eqp_state_reset;
                    write_size = niu_data_value_ecu[id].len;
                    memcpy(write_data, (NUINT8*)&eqp_state, size);
                    memcpy(niu_data_value_ecu[id].addr, (NUINT8 *)&eqp_state, write_size);
                    ret = JTRUE;
                }
                else if((id == NIU_ID_ECU_SERVER_URL || id == NIU_ID_ECU_SERVER_PORT ||
                            id == NIU_ID_ECU_ECU_SERIAL_NUMBER ||
                            id == NIU_ID_ECU_ECU_HARD_VER || id == NIU_ID_ECU_EID || 
                            id == NIU_ID_ECU_BLE_AES || id == NIU_ID_ECU_BLE_MAC ||
                            id == NIU_ID_ECU_BLE_NAME || id == NIU_ID_ECU_BLE_PASSWORD ||
                            id == NIU_ID_ECU_SERVER_URL2 || id == NIU_ID_ECU_SERVER_PORT2 ||
                            id == NIU_ID_ECU_SERVER_ERROR_URL || id == NIU_ID_ECU_SERVER_ERROR_PORT ||
                            id == NIU_ID_ECU_SERVER_ERROR_URL2 || id == NIU_ID_ECU_SERVER_ERROR_PORT2 ||
                            id == NIU_ID_ECU_SERVER_LBS_URL || id == NIU_ID_ECU_SERVER_LBS_PORT || 
                            id == NIU_ID_ECU_SERVER_LBS_URL2 || id == NIU_ID_ECU_SERVER_LBS_PORT2 ||
                            id == NIU_ID_ECU_SERVERUSERNAME || id == NIU_ID_ECU_SERVERPASSWORD ||
                            id == NIU_ID_ECU_AESKEY || id == NIU_ID_ECU_MQTTPUBLISH ||
                            id == NIU_ID_ECU_MQTTSUBSCRIBE) && size == niu_data_value_ecu[id].len)
                {
                    
                    memcpy(write_data, value, size);
                    write_size = niu_data_value_ecu[id].len;
                    if(g_flag_config_fields_sync)
                    {
                        if(memcmp(write_data, niu_data_value_ecu[id].addr, write_size) == 0)
                        {
                            //check config field received from  mcu, the value must be equal to v3 table
                            //printf("check sync: id=0x%x\n", id);
                            //niu_trace_dump_hex("check", niu_data_value_ecu[id].addr, niu_data_value_ecu[id].len);
                            niu_data_config_sync_mask_set(id);
                            NIU_DATA_VALUE *value = NIU_DATA_ECU(id);
                            if(value)
                            {
                                if(value->type == NIU_STRING)
                                {
                                    if(strlen(write_data) == 0)
                                    {
                                        LOG_D("sync data error[id=%02x]", id);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        niu_data_config_sync_mask_set(id);
                        memcpy(niu_data_value_ecu[id].addr, value, write_size);
                    }
                    
                    ret = JTRUE;
                }
                else if(id == NIU_ID_ECU_ECU_SOFT_VER || id == NIU_ID_ECU_IMSI ||
                        id == NIU_ID_ECU_ICCID || id == NIU_ID_ECU_IMEI || 
                        id == NIU_ID_ECU_4G_IMEI || 
                        (id >= NIU_ID_ECU_UPLOADING_TEMPLATE_1 && id <= NIU_ID_ECU_UPLOADING_TEMPLATE_20) ||
                        (id >= NIU_ID_ECU_UPLOADING_TEMPLATE_21 && id <= NIU_ID_ECU_UPLOADING_TEMPLATE_50))
                {
                    //some fields can not writed by mcu
                    ret = JTRUE;
                }
                else
                {
                    memcpy(write_data, value, size);
                    write_size = niu_data_value_ecu[id].len;
                    memcpy(niu_data_value_ecu[id].addr, value, write_size);
                    ret = JTRUE;
                }
                //niu_item_trace(base, id);
            }
            if(id != NIU_ID_ECU_CAR_STA)
            {
                //niu_item_trace(base, id);
                // LOG_D("=======car_sta: shake: %d, move_ex: %d, fail: %d===========", 
                //                         niu_car_state_get(NIU_STATE_SHAKE),
                //                         niu_car_state_get(NIU_STATE_MOVE_EX),
                //                         niu_car_state_get(NIU_STATE_FALL));
            }
            
            break;
        case NIU_ALARM:
            if (id < NIU_ID_ALARM_MAX && size <= niu_data_value_alarm[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_alarm[id].len;
                memcpy(niu_data_value_alarm[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_LCU:
            if (id < NIU_ID_LCU_MAX && size <= niu_data_value_lcu[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_lcu[id].len;
                memcpy(niu_data_value_lcu[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_BCS:
            if (id < NIU_ID_BCS_MAX && size <= niu_data_value_bcs[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_bcs[id].len;
                memcpy(niu_data_value_bcs[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_BMS2:
            if (id < NIU_ID_BMS2_MAX && size <= niu_data_value_bms2[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_bms2[id].len;
                memcpy(niu_data_value_bms2[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_QC:
            if (id < NIU_ID_QC_MAX && size <= niu_data_value_qc[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_qc[id].len;
                memcpy(niu_data_value_qc[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_EMCU:
            if (id < NIU_ID_EMCU_MAX && size <= niu_data_value_emcu[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_emcu[id].len;
                memcpy(niu_data_value_emcu[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_CPM:
            if (id < NIU_ID_CPM_MAX && size <= niu_data_value_cpm[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_cpm[id].len;
                memcpy(niu_data_value_cpm[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        default:
            break;
        }

        if (write_size)
        {
            niu_data_rw_update(base, id, write_data, write_size);
        }
    }
    
    return ret;
}

JBOOL niu_data_only_write(NIU_DATA_ADDR_BASE base, NUINT32 id, NIU_DATA_VALUE *value)
{
    JBOOL ret = JFALSE;
    NUINT8 write_data[100] = {0};
    NUINT8 write_size = 0;

    niu_data_value_trace(base, id, value);

    if (value && value->addr && value->len)
    {
        switch (base)
        {
        case NIU_DB:
            if (id < NIU_ID_DB_MAX && value->len <= niu_data_value_db[id].len && value->type == niu_data_value_db[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_db[id].len;
                memcpy(niu_data_value_db[id].addr, write_data, write_size);
            }
            break;
        case NIU_FOC:
            if (id < NIU_ID_FOC_MAX && value->len <= niu_data_value_foc[id].len && value->type == niu_data_value_foc[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_foc[id].len;
                memcpy(niu_data_value_foc[id].addr, write_data, write_size);
            }
            break;
        case NIU_LOCKBOARD:
            if (id < NIU_ID_LOCKBOARD_MAX && value->len <= niu_data_value_lockboard[id].len && value->type == niu_data_value_lockboard[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_lockboard[id].len;
                memcpy(niu_data_value_lockboard[id].addr, write_data, write_size);
            }
            break;
        case NIU_BMS:
            if (id < NIU_ID_BMS_MAX && value->len <= niu_data_value_bms[id].len && value->type == niu_data_value_bms[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_bms[id].len;
                memcpy(niu_data_value_bms[id].addr, write_data, write_size);
            }
            break;
        case NIU_ECU:
            if (id < NIU_ID_ECU_MAX && value->len <= niu_data_value_ecu[id].len && value->type == niu_data_value_ecu[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_ecu[id].len;
                memcpy(niu_data_value_ecu[id].addr, write_data, write_size);
            }
            break;
        case NIU_ALARM:
            if (id < NIU_ID_ALARM_MAX && value->len <= niu_data_value_alarm[id].len && value->type == niu_data_value_alarm[id].type)
            {
                memcpy(write_data, value->addr, value->len);
                write_size = niu_data_value_alarm[id].len;
                memcpy(niu_data_value_alarm[id].addr, write_data, write_size);
            }
            break;
        case NIU_LCU:
            if (id < NIU_ID_LCU_MAX)
            {
                if (id < NIU_ID_LCU_MAX && value->len <= niu_data_value_lcu[id].len && value->type == niu_data_value_lcu[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_lcu[id].len;
                    memcpy(niu_data_value_lcu[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_BCS:
            if (id < NIU_ID_BCS_MAX)
            {
                if (id < NIU_ID_BCS_MAX && value->len <= niu_data_value_bcs[id].len && value->type == niu_data_value_bcs[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_bcs[id].len;
                    memcpy(niu_data_value_bcs[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_BMS2:
            if (id < NIU_ID_BMS2_MAX)
            {
                if (id < NIU_ID_BMS2_MAX && value->len <= niu_data_value_bms2[id].len && value->type == niu_data_value_bms2[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_bms2[id].len;
                    memcpy(niu_data_value_bms2[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_QC:
            if (id < NIU_ID_QC_MAX)
            {
                if (id < NIU_ID_QC_MAX && value->len <= niu_data_value_qc[id].len && value->type == niu_data_value_qc[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_qc[id].len;
                    memcpy(niu_data_value_qc[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_EMCU:
            if (id < NIU_ID_EMCU_MAX)
            {
                if (id < NIU_ID_EMCU_MAX && value->len <= niu_data_value_emcu[id].len && value->type == niu_data_value_emcu[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_emcu[id].len;
                    memcpy(niu_data_value_emcu[id].addr, write_data, write_size);
                }
            }
            break;
        case NIU_CPM:
            if (id < NIU_ID_CPM_MAX)
            {
                if (id < NIU_ID_CPM_MAX && value->len <= niu_data_value_cpm[id].len && value->type == niu_data_value_cpm[id].type)
                {
                    memcpy(write_data, value->addr, value->len);
                    write_size = niu_data_value_cpm[id].len;
                    memcpy(niu_data_value_cpm[id].addr, write_data, write_size);
                }
            }
            break;
        default:
            break;
        }
    }
    //LOG_D("base:%d, id:%d, ret:%d", base, id, ret);
    return ret;
}

JBOOL niu_data_clean_value(NIU_DATA_ADDR_BASE base, NUINT32 id)
{
    JBOOL ret = JFALSE;
    NUINT8 *value = NULL;
    NUINT32 len = 0;

    switch (base)
    {
    case NIU_DB:
        if (id < NIU_ID_DB_MAX)
        {
            memset(niu_data_value_db[id].addr, 0, niu_data_value_db[id].len);
            value = niu_data_value_db[id].addr;
            len = niu_data_value_db[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_FOC:
        if (id < NIU_ID_FOC_MAX)
        {
            memset(niu_data_value_foc[id].addr, 0, niu_data_value_foc[id].len);
            value = niu_data_value_foc[id].addr;
            len = niu_data_value_foc[id].len;
            ret = JTRUE;
        }
        break;
        if (id < NIU_ID_LOCKBOARD_MAX)
        {
            memset(niu_data_value_lockboard[id].addr, 0, niu_data_value_lockboard[id].len);
            value = niu_data_value_lockboard[id].addr;
            len = niu_data_value_lockboard[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_BMS:
        if (id < NIU_ID_BMS_MAX)
        {
            memset(niu_data_value_bms[id].addr, 0, niu_data_value_bms[id].len);
            value = niu_data_value_bms[id].addr;
            len = niu_data_value_bms[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_ECU:
        if (id < NIU_ID_ECU_MAX) //只可以清空非保存文件系统的
        {
            memset(niu_data_value_ecu[id].addr, 0, niu_data_value_ecu[id].len);
            value = niu_data_value_ecu[id].addr;
            len = niu_data_value_ecu[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_ALARM:
        if (id < NIU_ID_ALARM_MAX)
        {
            memset(niu_data_value_alarm[id].addr, 0, niu_data_value_alarm[id].len);
            value = niu_data_value_alarm[id].addr;
            len = niu_data_value_alarm[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_LCU:
        if (id < NIU_ID_LCU_MAX)
        {
            memset(niu_data_value_lcu[id].addr, 0, niu_data_value_lcu[id].len);
            value = niu_data_value_lcu[id].addr;
            len = niu_data_value_lcu[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_BCS:
        if (id < NIU_ID_BCS_MAX)
        {
            memset(niu_data_value_bcs[id].addr, 0, niu_data_value_bcs[id].len);
            value = niu_data_value_bcs[id].addr;
            len = niu_data_value_bcs[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_BMS2:
        if (id < NIU_ID_BMS2_MAX)
        {
            memset(niu_data_value_bms2[id].addr, 0, niu_data_value_bms2[id].len);
            value = niu_data_value_bms2[id].addr;
            len = niu_data_value_bms2[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_QC:
        if (id < NIU_ID_QC_MAX)
        {
            memset(niu_data_value_qc[id].addr, 0, niu_data_value_qc[id].len);
            value = niu_data_value_qc[id].addr;
            len = niu_data_value_qc[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_EMCU:
        if (id < NIU_ID_EMCU_MAX)
        {
            memset(niu_data_value_emcu[id].addr, 0, niu_data_value_emcu[id].len);
            value = niu_data_value_emcu[id].addr;
            len = niu_data_value_emcu[id].len;
            ret = JTRUE;
        }
        break;
    case NIU_CPM:
        if (id < NIU_ID_CPM_MAX)
        {
            memset(niu_data_value_cpm[id].addr, 0, niu_data_value_cpm[id].len);
            value = niu_data_value_cpm[id].addr;
            len = niu_data_value_cpm[id].len;
            ret = JTRUE;
        }
        break;
    default:
        break;
    }
    
    if(JTRUE == ret )
    {
        niu_data_rw_update( base,  id,  value,  len);
    }
    
    LOG_D("clean:%d, id:%d, ret:%d, value:%p, len: %d", base, id, ret, value, len);

    return ret;
}

JBOOL niu_data_write_value(NIU_DATA_ADDR_BASE base, NUINT32 id, NUINT8 *value, NUINT8 size)
{
    JBOOL ret = JFALSE;
    NUINT8 write_data[100] = {0};
    NUINT8 write_size = 0;

    if (value && size)
    {
        switch (base)
        {
        case NIU_DB:
            if (id < NIU_ID_DB_MAX && size <= niu_data_value_db[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_db[id].len;
                memcpy(niu_data_value_db[id].addr, write_data, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_FOC:
            if (id < NIU_ID_FOC_MAX && size <= niu_data_value_foc[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_foc[id].len;
                memcpy(niu_data_value_foc[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_LOCKBOARD:
            if (id < NIU_ID_LOCKBOARD_MAX && size <= niu_data_value_lockboard[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_lockboard[id].len;
                memcpy(niu_data_value_lockboard[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_BMS:
            if (id < NIU_ID_BMS_MAX && size <= niu_data_value_bms[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_bms[id].len;
                memcpy(niu_data_value_bms[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_ECU:
            if (id < NIU_ID_ECU_MAX && size <= niu_data_value_ecu[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_ecu[id].len;
                //sync value to mcu, without these fields
                /*
                if(id != NIU_ID_ECU_CAR_TYPE && id != NIU_ID_ECU_TIZE_SIZE &&
                    id != NIU_ID_ECU_LOST_ENERY && id != NIU_ID_ECU_LOST_SOC &&
                    id != NIU_ID_ECU_LAST_MILEAGE && id != NIU_ID_ECU_TOTAL_MILEAGE)
                    */
                {
                    if(mcu_send_write(NIU_ECU, id, write_data, write_size) != 0)
                    {
                        LOG_E("mcu_send_write error.[base: 0x%02X, id: 0x%04X]", base, id);
                    }
                }
                memcpy(niu_data_value_ecu[id].addr, write_data, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_ALARM:
            if (id < NIU_ID_ALARM_MAX && size <= niu_data_value_alarm[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_alarm[id].len;
                memcpy(niu_data_value_alarm[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_LCU:
            if (id < NIU_ID_LCU_MAX && size <= niu_data_value_lcu[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_lcu[id].len;
                memcpy(niu_data_value_lcu[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_BCS:
            if (id < NIU_ID_BCS_MAX && size <= niu_data_value_bcs[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_bcs[id].len;
                memcpy(niu_data_value_bcs[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_BMS2:
            if (id < NIU_ID_BMS2_MAX && size <= niu_data_value_bms2[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_bms2[id].len;
                memcpy(niu_data_value_bms2[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_QC:
            if (id < NIU_ID_QC_MAX && size <= niu_data_value_qc[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_qc[id].len;
                memcpy(niu_data_value_qc[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_EMCU:
            if (id < NIU_ID_EMCU_MAX && size <= niu_data_value_emcu[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_emcu[id].len;
                memcpy(niu_data_value_emcu[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        case NIU_CPM:
            if (id < NIU_ID_CPM_MAX && size <= niu_data_value_cpm[id].len)
            {
                memcpy(write_data, value, size);
                write_size = niu_data_value_cpm[id].len;
                memcpy(niu_data_value_cpm[id].addr, value, write_size);
                ret = JTRUE;
            }
            break;
        default:
            break;
        }
        
       if(write_size)
       {
	        niu_data_rw_update(base, id, write_data, write_size);
       }
    
    }
    //LOG_D("base:%d, id:%d, ret:%d", base, id, ret);
    return ret;
}


JSTATIC JVOID niu_data_update(NIU_DATA_ADDR_BASE base, NUINT32 id)
{
    if (NIU_ECU == base && NIU_ID_ECU_TIMESTAMP == id)
    {
        g_niu_data.ecu.data.timestamp = niu_device_get_timestamp();
    }
    else if (NIU_ECU == base && NIU_ID_ECU_CAR_NO_GPS_SIGNAL_TIME == id)
    {
        g_niu_data.ecu.data.car_no_gps_signal_time = niu_car_nogps_time();
    }
    else if (NIU_ECU == base && NIU_ID_ECU_CAR_NO_GSM_SIGNAL_TIME == id)
    {
        g_niu_data.ecu.data.car_no_gsm_signal_time = niu_car_nogsm_time();
    }
    else if (NIU_ECU == base && NIU_ID_ECU_ICCID == id)
    {
        if (0 == strlen(g_niu_data.ecu.data.iccid))
        {
            cmd_iccid_info_get(g_niu_data.ecu.data.iccid, sizeof(g_niu_data.ecu.data.iccid));
        }
        LOG_D("iccid:%s", g_niu_data.ecu.data.iccid);
    }
    else if (NIU_ECU == base && NIU_ID_ECU_IMSI == id)
    {
        if (0 == g_niu_data.ecu.data.imsi[0])
        {
            cmd_imsi_info_get(g_niu_data.ecu.data.imsi, sizeof(g_niu_data.ecu.data.imsi));
        }
        LOG_D("imsi:%s", g_niu_data.ecu.data.imsi);
    }
    else if (NIU_ECU == base && NIU_ID_ECU_FOTA_DOWNLOAD_PERCENT == id)
    {
        g_niu_data.ecu.data.ota_rate = get_fota_download_percent();
    }
    else if (NIU_ECU == base && NIU_ID_ECU_CAR_EQP_CFG == id)
    {
        //
    }
    else if (NIU_ECU == base && NIU_ID_ECU_CAR_EQP_STATE == id)
    {
        //
    }
    else if (NIU_ECU == base && NIU_ID_ECU_4G_IMEI == id)
    {
        cmd_imei_info_get(g_niu_data.ecu.data.imei, sizeof(g_niu_data.ecu.data.imei));
        LOG_D("imei:%s", g_niu_data.ecu.data.imei);
    }
    else if (NIU_ECU == base && NIU_ID_ECU_4G_TEMPERATURE == id)
    {
        g_niu_data.ecu.data.temperature_4g = get_4g_temperature();
    }
    return;
}

NIU_DATA_VALUE *niu_data_read(NIU_DATA_ADDR_BASE base, NUINT32 id)
{
    NIU_DATA_VALUE *ret = NULL;

    //LOG_D("base:%d, id:%d", base, id);

    niu_data_update(base,id);
    switch (base)
    {
    case NIU_DB:
        if (id < NIU_ID_DB_MAX)
        {
            ret = &niu_data_value_db[id];
        }
        break;
    case NIU_FOC:
        if (id < NIU_ID_FOC_MAX)
        {
            ret = &niu_data_value_foc[id];
        }
        break;
    case NIU_LOCKBOARD:
        if (id < NIU_ID_LOCKBOARD_MAX)
        {
            ret = &niu_data_value_lockboard[id];
        }
        break;
    case NIU_BMS:
        if (id < NIU_ID_BMS_MAX)
        {
            ret = &niu_data_value_bms[id];
        }
        break;
    case NIU_ECU:
        if (id < NIU_ID_ECU_MAX)
        {
            ret = &niu_data_value_ecu[id];
        }
        break;
    case NIU_ALARM:
        if (id < NIU_ID_ALARM_MAX)
        {
            ret = &niu_data_value_alarm[id];
        }
        break;
    case NIU_LCU:
        if (id < NIU_ID_LCU_MAX)
        {
            ret = &niu_data_value_lcu[id];
        }
        break;
    case NIU_BCS:
        if (id < NIU_ID_BCS_MAX)
        {
            ret = &niu_data_value_bcs[id];
        }
        break;
    case NIU_BMS2:
        if (id < NIU_ID_BMS2_MAX)
        {
            ret = &niu_data_value_bms2[id];
        }
        break;
    case NIU_QC:
        if (id < NIU_ID_QC_MAX)
        {
            ret = &niu_data_value_qc[id];
        }
        break;
    case NIU_EMCU:
        if (id < NIU_ID_EMCU_MAX)
        {
            ret = &niu_data_value_emcu[id];
        }
        break;
    case NIU_CPM:
        if (id < NIU_ID_CPM_MAX)
        {
            ret = &niu_data_value_cpm[id];
        }
        break;
    default:
        break;
    }
    //    if(ret)
    //    {
    //         niu_data_value_trace( base,  id, ret);
    //    }
    return ret;
}

/**
 * dump template
 */
JSTATIC JVOID niu_data_template_dump(NIU_SERVER_UPL_TEMPLATE *upl_cmd)
{
    if (upl_cmd)
    {
        NUINT32 i = 0;
        LOG_D("index:%d", upl_cmd->index);
        LOG_D("trigger1:%d", upl_cmd->trigger1);
        LOG_D("trigger2:%d", upl_cmd->trigger2);
        LOG_D("trigger3:%d", upl_cmd->trigger1);
        LOG_D("trigger4:%d", upl_cmd->trigger2);
        LOG_D("trigger5:%d", upl_cmd->trigger1);
        LOG_D("trigger6:%d", upl_cmd->trigger2);
        LOG_D("trigger7:%d", upl_cmd->trigger1);
        LOG_D("trigger8:%d", upl_cmd->trigger2);
        LOG_D("trigger9:%d", upl_cmd->trigger1);
        LOG_D("trigger10:%d", upl_cmd->trigger2);
        LOG_D("trigger11:%d", upl_cmd->trigger1);
        LOG_D("trigger12:%d", upl_cmd->trigger2);
        LOG_D("packed_name:%d", upl_cmd->packed_name);
        LOG_D("list_num:%d", upl_cmd->addr_num);
        for (i = 0; i < upl_cmd->addr_num; i++)
        {
            LOG_D("addr=%p: base=%d, id=%d", upl_cmd, upl_cmd->addr_array[i].base, upl_cmd->addr_array[i].id);
        }

        LOG_D("interval:%d", upl_cmd->interval);
        LOG_D("curtime:%d", upl_cmd->cur_time);
    }
}

NIU_SERVER_UPL_TEMPLATE *niu_data_read_template(NUINT8 index)
{
    NIU_SERVER_UPL_TEMPLATE *ret = NULL;

    if (index < NIU_SERVER_UPL_CMD_NUM)
    {
        ret = &g_niu_upl_cmd[index];
        if (g_niu_upl_cmd[index].index != index)
        {
            niu_data_rw_default();
            niu_templ_data_init();
        }

    }
    return ret;
}

JBOOL niu_data_write_template(NIU_SERVER_UPL_TEMPLATE *upl_cmd, NUINT8 index)
{
    JBOOL ret = JFALSE;

    LOG_D("upl_cmd:%p, index:%d", upl_cmd, index);

    if (upl_cmd && index < NIU_SERVER_UPL_CMD_NUM)
    {
        LOG_D("upl_cmd_index:%d, index:%d, cmd_name:%d", upl_cmd->index, index, upl_cmd->packed_name);
        if (upl_cmd->index == index)
        {
            memcpy(&g_niu_upl_cmd[upl_cmd->index], upl_cmd, sizeof(NIU_SERVER_UPL_TEMPLATE));
            
            niu_data_rw_template_write(upl_cmd->index, upl_cmd);
            if(index < 20)
            {
                niu_data_write_value(NIU_ECU, NIU_ID_ECU_UPLOADING_TEMPLATE_1 + index, (NUINT8 *)&upl_cmd->packed_name, sizeof(upl_cmd->packed_name));
            }
            else
            {
                niu_data_write_value(NIU_ECU, NIU_ID_ECU_UPLOADING_TEMPLATE_20 + index - 20, (NUINT8 *)&upl_cmd->packed_name, sizeof(upl_cmd->packed_name));
            }
        }
        ret = JTRUE;
    }
    return ret;
}

JVOID show_car_state(NUINT32 state)
{
    LOG_D("%-30s = %s", "NIU_STATE_ACC", state & NIU_STATE_ACC ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_RUNNING", state & NIU_STATE_RUNNING ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_BATTERY", state & NIU_STATE_BATTERY ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_SLAVE", state & NIU_STATE_SLAVE ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_SHAKE", state & NIU_STATE_SHAKE ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_MOVE", state & NIU_STATE_MOVE ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_FALL", state & NIU_STATE_FALL ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_INCHARGE", state & NIU_STATE_INCHARGE ? "True" : "False");
    LOG_D("%-30s = %n", "NIU_STATE_GPS_LOCATION", state & NIU_STATE_GPS_LOCATION ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_GPS_ON", state & NIU_STATE_GPS_ON ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_FOC_ERR", state & NIU_STATE_FOC_ERR ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_BMS_ERR", state & NIU_STATE_BMS_ERR ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_DB_ERR", state & NIU_STATE_DB_ERR ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_LCU_ERR", state & NIU_STATE_LCU_ERR ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_FOTA", state & NIU_STATE_FOTA ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_MOVE_EX", state & NIU_STATE_MOVE_EX ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_BMS2_ERR", state & NIU_STATE_BMS2_ERR ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_BCS_ERR", state & NIU_STATE_BCS_ERR ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_DUL_BATTERY", state & NIU_STATE_DUL_BATTERY ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_STATE_MQTT_CONNECTED", state & NIU_STATE_MQTT_CONNECTED ? "True" : "False");
}

JVOID show_ecu_state(NUINT32 state)
{
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_ACC_OFF", state & NIU_TRIGGER_EVENT_ACC_OFF ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_BATTER_IN", state & NIU_TRIGGER_EVENT_BATTER_IN ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_BATTER_OUT", state & NIU_TRIGGER_EVENT_BATTER_OUT ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_SHAKE", state & NIU_TRIGGER_EVENT_SHAKE ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_SHAKE_STOP", state & NIU_TRIGGER_EVENT_SHAKE_STOP ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_MOVE", state & NIU_TRIGGER_EVENT_MOVE ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_MOVE_STOP", state & NIU_TRIGGER_EVENT_MOVE_STOP ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_FALL", state & NIU_TRIGGER_EVENT_FALL ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_EVENT_FALL_STOP", state & NIU_TRIGGER_EVENT_FALL_STOP ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_RUNNING_Y", state & NIU_TRIGGER_STATE_RUNNING_Y ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_RUNNING_N", state & NIU_TRIGGER_STATE_RUNNING_N ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_ACC_Y", state & NIU_TRIGGER_STATE_ACC_Y ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_ACC_N", state & NIU_TRIGGER_STATE_ACC_N ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_BATTERY_Y", state & NIU_TRIGGER_STATE_BATTERY_Y ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_BATTERY_N", state & NIU_TRIGGER_STATE_BATTERY_N ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_SLAVE_Y", state & NIU_TRIGGER_STATE_SLAVE_Y ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_SLAVE_N", state & NIU_TRIGGER_STATE_SLAVE_N ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_SHAKE_Y", state & NIU_TRIGGER_STATE_SHAKE_Y ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_SHAKE_N", state & NIU_TRIGGER_STATE_SHAKE_N ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_MOVE_Y", state & NIU_TRIGGER_STATE_MOVE_Y ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_MOVE_N", state & NIU_TRIGGER_STATE_MOVE_N ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_FALL_Y", state & NIU_TRIGGER_STATE_FALL_Y ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_FALL_N", state & NIU_TRIGGER_STATE_FALL_N ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_INCHARGE", state & NIU_TRIGGER_STATE_INCHARGE ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_DISCHARGE", state & NIU_TRIGGER_STATE_DISCHARGE ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_GPS_Y", state & NIU_TRIGGER_STATE_GPS_Y ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_GPS_N", state & NIU_TRIGGER_STATE_GPS_N ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_FOC_DISCONNECT", state & NIU_TRIGGER_STATE_FOC_DISCONNECT ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_BMS_DISCONNECT", state & NIU_TRIGGER_STATE_BMS_DISCONNECT ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_DB_DISCONNECT", state & NIU_TRIGGER_STATE_DB_DISCONNECT ? "True" : "False");
    LOG_D("%-30s = %s", "NIU_TRIGGER_STATE_LCU_DISCONNECT", state & NIU_TRIGGER_STATE_LCU_DISCONNECT ? "True" : "False");
}


JVOID niu_data_config_sync_flag_set(int flag)
{
    g_flag_config_fields_sync = flag;
}

int niu_data_config_sync_mask_get()
{
    return g_flag_config_fields_sync_mask;
}

JVOID niu_data_config_sync_mask_reset()
{
    g_flag_config_fields_sync_mask = 0;
}

JVOID niu_data_all_config_fields_req()
{
    //send read command to mcu, when mcu response,
    //make a flag mask at g_flag_config_fields_sync_mask
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_URL);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_PORT);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_ECU_SERIAL_NUMBER);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_ECU_HARD_VER);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_EID);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_BLE_AES);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_BLE_MAC);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_BLE_NAME);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_BLE_PASSWORD);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_URL2);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_PORT2);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_ERROR_URL);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_ERROR_PORT);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_ERROR_URL2);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_ERROR_PORT2);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_LBS_URL);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_LBS_PORT);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_LBS_URL2);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_LBS_PORT2);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVERUSERNAME);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVERPASSWORD);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_AESKEY);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_MQTTPUBLISH);
    mcu_send_read(NIU_ECU, NIU_ID_ECU_MQTTSUBSCRIBE);
    //others
    mcu_send_read(NIU_ECU, NIU_ID_ECU_ECU_KEEPALIVE);
}

JVOID niu_data_config_sync_mask_set(NUINT32 id)
{
    switch (id)
    {
        case NIU_ID_ECU_SERVER_URL:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_URL;
        }
        break;
        case NIU_ID_ECU_SERVER_PORT:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_PORT;
        }
        break;
        case NIU_ID_ECU_ECU_SERIAL_NUMBER:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERIAL_NUMBER;
        }
        break;
        case NIU_ID_ECU_EID:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_ECU_EID;
        }
        break;
        case NIU_ID_ECU_BLE_AES:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_BLE_AES;
        }
        break;
        case NIU_ID_ECU_BLE_MAC:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_BLE_MAC;
        }
        break;
        case NIU_ID_ECU_BLE_NAME:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_BLE_NAME;
        }
        break;
        case NIU_ID_ECU_BLE_PASSWORD:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_BLE_PASSWORD;
        }
        break;
        case NIU_ID_ECU_SERVER_URL2:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_URL2;
        }
        break;
        case NIU_ID_ECU_SERVER_PORT2:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_PORT2;
        }
        break;
        case NIU_ID_ECU_SERVER_ERROR_URL:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_ERROR_URL;
        }
        break;
        case NIU_ID_ECU_SERVER_ERROR_URL2:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_ERROR_URL2;
        }
        break;
        case NIU_ID_ECU_SERVER_ERROR_PORT:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_ERROR_PORT;
        }
        break;
        case NIU_ID_ECU_SERVER_ERROR_PORT2:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_ERROR_PORT2;
        }
        break;
        case NIU_ID_ECU_SERVER_LBS_URL:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_LBS_URL;
        }
        break;
        case NIU_ID_ECU_SERVER_LBS_URL2:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_LBS_URL2;
        }
        break;
        case NIU_ID_ECU_SERVER_LBS_PORT:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_LBS_PORT;
        }
        break;
        case NIU_ID_ECU_SERVER_LBS_PORT2:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVER_LBS_PORT2;
        }
        break;
        case NIU_ID_ECU_SERVERUSERNAME:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVERUSERNAME;
        }
        break;
        case NIU_ID_ECU_SERVERPASSWORD:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_SERVERPASSWORD;
        }
        break;
        case NIU_ID_ECU_AESKEY:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_AESKEY;
        }
        break;
        case NIU_ID_ECU_MQTTPUBLISH:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_MQTTPUBLISH;
        }
        break;
        case NIU_ID_ECU_MQTTSUBSCRIBE:
        {
            g_flag_config_fields_sync_mask |= NIU_MASK_SYNC_FLAG_MQTTSUBSCRIBE;
        }
        break;
        default:
        break;
    }
    return;
}
