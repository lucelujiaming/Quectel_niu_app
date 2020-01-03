#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ql_oe.h>
#include "service.h"
#include "utils.h"
#include "niu_data.h"
#include "gps_data.h"
#include "gsensor_data.h"
#include "mcu_data.h"
#include "niu_device.h"
#include "niu_data_pri.h"
#include "niu_work_mode.h"
#include "niu_entry.h"
#include "niu_thread.h"
#include "low_power.h"

typedef struct _niu_gps_data_
{
    double Lon; //121.34567; //经度
    double Lat; //31.7654321; //纬度
} NIU_GPS_DATA;

#define J_NIU_GPS_DATA_MAX_NUM (100)
JSTATIC NUINT8 g_gps_data_count = 0;
JSTATIC NUINT8 g_gps_data_pos = 0;

JSTATIC NUINT16 g_bus_v = 0;
JSTATIC NUINT64 g_niu_gps_lost_time = 0;
JSTATIC NUINT32 g_niu_no_gps_time = 0;
JSTATIC NUINT32 g_niu_gsm_lost_time = 0;
JSTATIC NUINT32 g_niu_no_gsm_time = 0;
JSTATIC NUINT32 g_niu_restart_gps_time = 0;
JSTATIC NUINT64 g_niu_sleep_start_time = 0;

JSTATIC NDOUBLE g_niu_prev_lon = 0;
JSTATIC NDOUBLE g_niu_prev_lat = 0;

JSTATIC NDOUBLE g_niu_avg_lon = 0;
JSTATIC NDOUBLE g_niu_avg_lat = 0;
JSTATIC NUINT32 g_niu_avg_index = 0;

JSTATIC NDOUBLE g_niu_off_lon = 0;
JSTATIC NDOUBLE g_niu_off_lat = 0;

#define J_NIU_GSENSOR_INT_COUNT (3)
#define J_NIU_GSENSOR_INT_INTERVAL (10)
#if 0 
//baron 2019/06/15 start: remove gsensor process to mcu
JSTATIC NUINT8 g_gsensor_int_count = 0;
JSTATIC JBOOL g_gsensor_shake_enable = JTRUE;
#endif
void entry_niu_gsm_data_update()
{
    if(0 == is_mqtt_connected()) {
        if(0 == g_niu_gsm_lost_time) {
            g_niu_gsm_lost_time = time(NULL);
            g_niu_no_gsm_time = 0;
        }
        else {
            g_niu_no_gsm_time = time(NULL) - g_niu_gsm_lost_time; 
        }
    }
    else {
        g_niu_gsm_lost_time = 0;
    }
}


JSTATIC JVOID niu_bus_v_set_zero(int tmr_id, void *param)
{
    //LOG_D("g_bus_v:%d", g_bus_v);
    g_bus_v = 0;
}

JSTATIC NUINT16 niu_read_v()
{
    NIU_DATA_VALUE* foc_speed = NIU_DATA_FOC(NIU_ID_FOC_FOC_MOTORSPEED);
    NUINT16 foc_speed_v = 0;

    if(foc_speed)
    {
         memcpy(&foc_speed_v, foc_speed->addr, sizeof(NUINT16));
    }

    //LOG_D("foc_speed:%d", foc_speed_v);

    return  foc_speed_v;
}

JSTATIC JVOID niu_gps_data_history(double lon, double lat)
{
    LOG_D("pos:%d, count:%d", g_gps_data_pos, g_gps_data_count);
    LOG_D("{fLon:%lf, fLat:%lf }", lon, lat);

    if (lon >= (-180) && lon <= (180) && lat >= (-90) && lat <= (90))
    {
        double avg_lon_prev = g_niu_avg_lon;
        double avg_lat_prev = g_niu_avg_lat;

        if (g_niu_avg_index == 0)
        {
            g_niu_avg_lon = lon;
            g_niu_avg_lat = lat;
        }
        else
        {
            g_niu_avg_lon = ((g_niu_avg_lon * g_niu_avg_index) / (g_niu_avg_index + 1)) + (lon / (g_niu_avg_index + 1));
            g_niu_avg_lat = ((g_niu_avg_lat * g_niu_avg_index) / (g_niu_avg_index + 1)) + (lat / (g_niu_avg_index + 1));
        }

        LOG_D("{i:%d, avg_pos:(%lf, %lf)+(%lf,%lf)->(%lf,%lf) }", g_niu_avg_index, avg_lon_prev, avg_lat_prev, lon, lat, g_niu_avg_lon, g_niu_avg_lat);

        if (g_niu_avg_index < (0xFFFFFFFF))
        {
            g_niu_avg_index++;
        }

        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_AVG_LONGITUDE, (NUINT8 *)&g_niu_avg_lon, sizeof(g_niu_avg_lon));
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_AVG_LATITUDE, (NUINT8 *)&g_niu_avg_lat, sizeof(g_niu_avg_lat));
    }

}

NUINT32 niu_car_nogps_time()
{
    LOG_D("g_niu_no_gps_time:%d", g_niu_no_gps_time);
    return g_niu_no_gps_time;
}
NUINT32 niu_car_nogsm_time()
{
    return g_niu_no_gsm_time;
}

int niu_data_get_acc_status()
{
    NUINT32 car_sta = 0;
    NIU_DATA_VALUE *value = NIU_DATA_ECU(NIU_ID_ECU_CAR_STA);
    if (value)
    {
        memcpy(&car_sta, value->addr, sizeof(NUINT32));
        return car_sta & NIU_STATE_ACC;
    }
    return 0;
}

JSTATIC JBOOL g_niu_acc_prev = JFALSE;
JVOID niu_acc_callback(JBOOL on_off)
{
    if (on_off)
    {
        niu_car_state_set(NIU_STATE_SHAKE, JFALSE);
        niu_car_state_set(NIU_STATE_MOVE, JFALSE);
        niu_car_state_set(NIU_STATE_FALL, JFALSE);
    }

    if (on_off == JTRUE)
    {        
        niu_car_state_set(NIU_STATE_SHAKE, JFALSE);
        g_niu_off_lon = 0;
        g_niu_off_lat = 0;
        if (JFALSE == ql_gps_get_onoff())   //juson.zhang-2019/05/17:when acc on, open gps
        {
            ql_gps_power_on();
        }
        gsensor_wakeup_handler(60);
    }
    else
    {
        NIU_DATA_VALUE *lon = NIU_DATA_ECU(NIU_ID_ECU_LONGITUDE);
        NIU_DATA_VALUE *lat = NIU_DATA_ECU(NIU_ID_ECU_LATITUDE);
        struct timespec interval_time;

        if (lon && lat)
        {
            memcpy(&g_niu_off_lon, lon->addr, lon->len);
            memcpy(&g_niu_off_lat, lat->addr, lon->len);

            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_AVG_LATITUDE, (NUINT8 *)&g_niu_off_lat, sizeof(double));
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_AVG_LONGITUDE, (NUINT8 *)&g_niu_off_lon, sizeof(double));
        }
        //baron.qian-2019/06/20: Requirements change, and this function is implemented on the MCU side
        //set acc off time
        //niu_acc_off_time_update();

        memset(&interval_time, 0, sizeof(struct timespec));
        low_power_check(LOCK_TYPE_GSENSOR, interval_time);
    }

    g_niu_acc_prev = on_off;
}

JVOID entry_niu_gps_data_update()
{
    struct niu_gps_data gps_data = {0};
    NUINT16 ecu_cfg_value = 0;
    NUINT16 move_length = 100;
    NIU_DATA_VALUE *ecu_cfg = NIU_DATA_ECU(NIU_ID_ECU_ECU_CFG);
    NDOUBLE Lon = 0;//121.34567; //经度
    NDOUBLE Lat = 0;//31.7654321; //纬度
    NUINT8  cnr = 0;

    if (0 < niu_read_v())
    {
        g_bus_v = niu_read_v();
        tmr_handler_tmr_del(TIMER_ID_NIU_V_TO_ZERO);
    }
    else if (JFALSE == tmr_handler_tmr_is_exist(TIMER_ID_NIU_V_TO_ZERO) && 0 < g_bus_v)
    {
        tmr_handler_tmr_add(TIMER_ID_NIU_V_TO_ZERO, niu_bus_v_set_zero, NULL, 3 * 1000, 0);
    }
    //LOG_D("g_bus_v:%d", g_bus_v);

    if (ecu_cfg)
    {
        memcpy(&ecu_cfg_value, ecu_cfg->addr, sizeof(NUINT16));
    }
    //LOG_D("gps state: %d", ecu_cfg_value & 0x1000);
    if (ecu_cfg_value & 0x1000) //关闭GPS
    {
        ql_gps_power_off();
        LOG_D("ql_gps_power_off.");
        g_niu_gps_lost_time = 0;
        g_niu_no_gps_time = 0;
        g_niu_restart_gps_time = 0;
        g_niu_prev_lon = 0;
        g_niu_prev_lat = 0;
    }
    else
    {
        // if (JFALSE == ql_gps_get_onoff())    //juson.zhang-2019/05/17:close gps
        // {
        //     ql_gps_power_on();
        // }
        Lon = g_niu_prev_lon;
        Lat = g_niu_prev_lat;

        gps_data_read(&gps_data);
        if(gps_data.fwver[0] != 0)
        {
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_GPS_VERSION, (NUINT8*)gps_data.fwver, strlen(gps_data.fwver));
        }
        LOG_V("valid: %s, latitude: %f, cardinal: %d, lontitude: %f, cardinal: %d, heading: %f, gps_speed: %f",
                gps_data.valid == 1 ? "TRUE":"FALSE",gps_data.latitude, gps_data.latitude_cardinal, gps_data.longitude, gps_data.longitude_cardinal,
            gps_data.heading, gps_data.gps_speed);
        LOG_V("max_cnr: %d, min_cnr: %d, avg_cnr: %d, gps_signal: %d, altitude: %f, hdop: %f, pdop: %f, fwver: %d",
            gps_data.max_cnr, gps_data.min_cnr, gps_data.avg_cnr, gps_data.gps_signal,
            gps_data.altitude, gps_data.hdop, gps_data.pdop, gps_data.fwver);

        NUINT32 curr_time = niu_device_get_timestamp();
        if (JTRUE == ql_gps_get_onoff() && JFALSE == gps_data.valid)
        {
            if (g_niu_gps_lost_time == 0)
            {
                g_niu_gps_lost_time = curr_time;
            }
            else
            {
                g_niu_no_gps_time = curr_time - g_niu_gps_lost_time;
            }
            if (g_niu_restart_gps_time == 0)
            {
                g_niu_restart_gps_time = curr_time;
            }
        }
        else
        {
            g_niu_gps_lost_time = 0;
            g_niu_no_gps_time = 0;
            g_niu_restart_gps_time = 0;
        }
        LOG_V("curr_time:%d,no_gps:%d,lost_gps:%llu,restart_gps:%d",curr_time,g_niu_no_gps_time,g_niu_gps_lost_time,g_niu_restart_gps_time);

        // if (g_niu_restart_gps_time && ((curr_time >= ((3 * 60) + g_niu_restart_gps_time)) || ((curr_time >= ((60) + g_niu_restart_gps_time)) && (15 < niu_read_v()))))
        // {
        //     LOG_D("RESTART GPS");
        //     g_niu_restart_gps_time = curr_time;
        //     ql_gps_power_off();
        // }

        if (gps_data.valid && gps_data.longitude > 0 && gps_data.latitude > 0)
        {
            double lon_v = 0;
            double lat_v = 0;
            lon_v = gps_data.longitude;
            lat_v = gps_data.latitude;

            if (gps_data.longitude_cardinal == 'W')
            {
                lon_v = lon_v * (-1);
            }

            if (gps_data.latitude_cardinal == 'S')
            {
                lat_v = lat_v * (-1);
            }

            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_REALTIME_LONGITUDE, (NUINT8 *)&lon_v, sizeof(lon_v));
            niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_REALTIME_LATITUDE, (NUINT8 *)&lat_v, sizeof(lat_v));

            if (lon_v >= (-180) && lon_v <= (180) && lat_v >= (-90) && lat_v <= (90))
            {
                Lon = lon_v;
                Lat = lat_v;
                if ((JFALSE == niu_data_get_acc_status() || 0 == g_bus_v))
                {
                    niu_gps_data_history(lon_v, lat_v);
                }
                else
                {
                    g_gps_data_count = 0;
                    g_gps_data_pos = 0;
                    g_niu_avg_index = 0;
                    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_AVG_LONGITUDE, (NUINT8 *)&Lon, sizeof(Lon));
                    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_AVG_LATITUDE, (NUINT8 *)&Lat, sizeof(Lat));
                }
            }
        }
        else
        {
            g_niu_avg_index = 0;
            g_gps_data_count = 0;
            g_gps_data_pos = 0;
        }

        g_niu_prev_lon = Lon;
        g_niu_prev_lat = Lat;
        //LOG_D("year:%d, mon:%d, day:%d, hour:%d, min:%d, sec:%d", gps_data.tm.year, gps_data.tm.mon, gps_data.tm.day, gps_data.tm.hour, gps_data.tm.min, gps_data.tm.sec);
        // LOG_D("{fLon:%lf, fLat:%lf }", Lon, Lat);
        // LOG_D("heading:%f", gps_data.heading);
        // LOG_D("speed:%f", gps_data.gps_speed);
        // LOG_D("gps_signal:%d", gps_data.gps_signal);
        // LOG_D("hdop:%f", gps_data.hdop);
        // LOG_D("pdop:%f", gps_data.pdop);
        // LOG_D("gps_snr(%d):%d-%d-%d, gps_satellite:%d", gps_data.gps_signal, gps_data.max_cnr,
        //                                                     gps_data.avg_cnr, gps_data.min_cnr, gps_data.satellites_num);

        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_GPS_MAX_CNR, (NUINT8 *)&gps_data.max_cnr, 4);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_GPS_MIN_CNR, (NUINT8 *)&gps_data.min_cnr, 4);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_GPS_AVG_CNR, (NUINT8 *)&gps_data.avg_cnr, 4);
        //2019-05-08 wujing.geng Requirements max_snr1,max_snr2 are set same values with gps_max_cnr
        cnr = gps_data.max_cnr;
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_MAX_SNR_1, (NUINT8*)&cnr, sizeof(NUINT8));
        cnr = gps_data.max_cnr2;
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_MAX_SNR_2, (NUINT8*)&cnr, sizeof(NUINT8));
        //printf("write max_snr1 %d, max_snr2: %d\n", gps_data.max_cnr, gps_data.max_cnr2);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_GPS_SEILL_NUM, (NUINT8 *)&gps_data.satellites_num, 4);
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_LONGITUDE, (NUINT8 *)&Lon, sizeof(Lon));
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_LATITUDE, (NUINT8 *)&Lat, sizeof(Lat));
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_HEADING, (NUINT8 *)&gps_data.heading, sizeof(gps_data.heading));
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_GPS_SPEED, (NUINT8 *)&gps_data.gps_speed, sizeof(gps_data.gps_speed));
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_HDOP, (NUINT8 *)&gps_data.hdop, sizeof(gps_data.hdop));
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_PHOP, (NUINT8 *)&gps_data.pdop, sizeof(gps_data.pdop));
        niu_data_write_value(NIU_ECU, NIU_ID_ECU_GPS_SINGAL, (NUINT8 *)&gps_data.gps_signal, sizeof(gps_data.gps_signal));
    }

    niu_car_state_set(NIU_STATE_GPS_ON, ql_gps_get_onoff());
    if (gps_data.valid)
    {
        niu_car_state_set(NIU_STATE_GPS_LOCATION, gps_data.valid);
    }
    else if (g_niu_no_gps_time >= 5 || JFALSE == ql_gps_get_onoff())
    {
        niu_car_state_set(NIU_STATE_GPS_LOCATION, gps_data.valid);
    }

    if (JFALSE == niu_car_state_get(NIU_STATE_ACC) && 0 != Lat && 0 != Lon && 0 != g_niu_off_lat && 0 != g_niu_off_lon)
    {
        double distance = 0;
        NIU_DATA_VALUE *move_value = NIU_DATA_ECU(NIU_ID_ECU_MOVED_LENGTH);
        if (move_value)
        {
            memcpy(&move_length, move_value->addr, 2);
        }

        distance = niu_get_distance(Lat, Lon, g_niu_off_lat, g_niu_off_lon);

        LOG_D("{distance:%lf, fLon:%lf, fLat:%lf, off_Lon:%lf, off_Lat:%lf }", distance, Lon, Lat, g_niu_off_lon, g_niu_off_lat);

        if (distance > (jmax(100, move_length)))
        {
            niu_car_state_set(NIU_STATE_MOVE, JTRUE);
        }
        else
        {
            niu_car_state_set(NIU_STATE_MOVE, JFALSE);
        }
    }
    else
    {
        if (JFALSE == niu_car_state_get(NIU_STATE_ACC) && 0 != Lat && 0 != Lon)
        {
            g_niu_off_lat = Lat;
            g_niu_off_lon = Lon;
        }

        niu_car_state_set(NIU_STATE_MOVE, JFALSE);
    }
}
#if 0
JSTATIC NUINT8 g_gsensor_fall_count = 0;
JVOID entry_niu_gsensor_data_update()
{
    NUINT16 fall_value = 500;
    NUINT16 x, y, z;
    NUINT16 gx, gy, gz;
    NFLOAT pitch = 0.0f;
    BMI_SENSOR_DATA gsensor_data;

    gsensor_data_read(&gsensor_data);

    x = gsensor_data.accdata[X_AXIS];
    y = gsensor_data.accdata[Y_AXIS];
    z = gsensor_data.accdata[Z_AXIS];
    pitch = gsensor_data.pitch;
    
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_X_ACC, (NUINT8 *)&x, 2);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_Y_ACC, (NUINT8 *)&y, 2);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_Z_ACC, (NUINT8 *)&z, 2);

    gx = gsensor_data.gyrodata[X_AXIS];
    gy = gsensor_data.gyrodata[Y_AXIS];
    gz = gsensor_data.gyrodata[Z_AXIS];
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_X_ANG, (NUINT8 *)&gx, 2);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_Y_ANG, (NUINT8 *)&gy, 2);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_Z_ANG, (NUINT8 *)&gz, 2);

    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_ROLL, (NUINT8 *)&gsensor_data.roll, 4);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_PITCH, (NUINT8 *)&gsensor_data.pitch, 4);
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_YAW, (NUINT8 *)&gsensor_data.yaw, 4);

    NIU_DATA_VALUE *niu_fall_value = NIU_DATA_ECU(NIU_ID_ECU_FAIL_VALUE);

    if (niu_fall_value)
    {
        memcpy(&fall_value, niu_fall_value->addr, 2);
    }
    y = (y < 0) ? (-y) : (y);
    x = (x < 0) ? (-x) : (x);
    pitch = (pitch < 0.0f) ? (-pitch) : (pitch);

    if (fall_value - pitch > 0 || 0 == fall_value)
    {
        g_gsensor_fall_count = 0;
    }
    else
    {
        if (g_gsensor_fall_count < 10)
        {
            g_gsensor_fall_count++;
        }
        printf("roll %f, fall_count %d\n", pitch, g_gsensor_fall_count);
    }

    //LOG_D("g_gsensor_fall_count:%d", g_gsensor_fall_count);

    if (g_gsensor_fall_count >= 10)
    {
        niu_car_state_set(NIU_STATE_FALL, JTRUE);
    }
    else
    {
        niu_car_state_set(NIU_STATE_FALL, JFALSE);
    }
}

/**
 * used to check gsensor value , and update ecu_car_sta->NIU_STATE_MOVE_EX
 * read v3 table move_gsensor_count and move_gsensor_time value
 */ 
#define MOVE_EX_INT_NUM_MAX (60)

JSTATIC NUINT8 g_move_ex_int_value[MOVE_EX_INT_NUM_MAX + 1] = {0};
JSTATIC NUINT8 g_move_ex_int_pos = 0;

JSTATIC NUINT16 g_gsensor_prev_x = 0;
JSTATIC NUINT16 g_gsensor_prev_y = 0;
JSTATIC NUINT16 g_gsensor_prev_z = 0;

JSTATIC JVOID niu_gsensor_active()
{
    LOG_V("work_mode:%d, g_gsensor_int_count: %d, shake:%d", niu_work_mode_value(), g_gsensor_int_count, niu_car_state_get(NIU_STATE_SHAKE));

    LOG_V("g_gsensor_shake_enable:%d", g_gsensor_shake_enable);

    if (JTRUE == g_gsensor_shake_enable)
    {
        niu_car_state_set(NIU_STATE_SHAKE, JTRUE);
    }

    niu_work_mode_do_active();
}

JSTATIC JVOID niu_gsensor_reset()
{
    tmr_handler_tmr_del(TIMER_ID_GSENSOR_INT_RESET);
    g_gsensor_int_count = 0;
}

JSTATIC JVOID niu_gsensor_int()
{
    LOG_V("work_mode:%d,g_gsensor_int_count:%d", niu_work_mode_value(), g_gsensor_int_count);

    if (JFALSE == niu_car_state_get(NIU_STATE_ACC))
    {
        if (g_gsensor_int_count == 0)
        {
            tmr_handler_tmr_add(TIMER_ID_GSENSOR_INT_RESET, niu_gsensor_reset, NULL, J_NIU_GSENSOR_INT_INTERVAL * 1000, 0);
        }
        g_gsensor_int_count++;

        if (g_gsensor_int_count >= J_NIU_GSENSOR_INT_COUNT)
        {
            niu_gsensor_active();
            niu_gsensor_reset();
        }
    }
    else
    {
        niu_gsensor_reset();
    }
}

JSTATIC JVOID niu_gsensor_check_value(int tmr_id, void *param)
{
    NUINT16 x, y, z;
    NUINT16 delta_x = 0;
    NUINT16 delta_y = 0;
    NUINT16 delta_z = 0;
    NUINT8 shaking_value = 0;
    NIU_DATA_VALUE *value = NULL;

    BMI_SENSOR_DATA gsensor_data;

    if(tmr_id != TIMER_ID_GSENSOR_INT_VALUE)
    {
        LOG_E("tmr_id(%d) error[%d].", tmr_id, TIMER_ID_GSENSOR_INT_VALUE);
        return;
    }

    gsensor_data_read(&gsensor_data);

    x = gsensor_data.accdata[X_AXIS];
    y = gsensor_data.accdata[Y_AXIS];
    z = gsensor_data.accdata[Z_AXIS];

    if (x != 0 && y != 0 && z != 0 && g_gsensor_prev_x != 0 && g_gsensor_prev_y != 0 && g_gsensor_prev_z != 0)
    {
        delta_x = (g_gsensor_prev_x > x) ? (g_gsensor_prev_x - x) : (x - g_gsensor_prev_x);
        delta_y = (g_gsensor_prev_y > y) ? (g_gsensor_prev_y - y) : (y - g_gsensor_prev_y);
        delta_z = (g_gsensor_prev_z > z) ? (g_gsensor_prev_z - z) : (z - g_gsensor_prev_z);
        g_gsensor_prev_x = x;
        g_gsensor_prev_y = y;
        g_gsensor_prev_z = z;
    }
    else
    {
        g_gsensor_prev_x = x;
        g_gsensor_prev_y = y;
        g_gsensor_prev_z = z;
    }

    // LOG_D("g_gsensor_int_status:%d", g_gsensor_int_status);
    LOG_V("NIU_STATE_BATTERY: %d, NIU_STATE_ACC: %d, NIU_STATE_RUNNING: %d, NIU_STATE_MOVE_EX: %d, NIU_STATE_SHAKE: %d, INCHANGE: %d", 
                       niu_car_state_get(NIU_STATE_BATTERY), niu_car_state_get(NIU_STATE_ACC), niu_car_state_get(NIU_STATE_RUNNING),
                       niu_car_state_get(NIU_STATE_MOVE_EX), niu_car_state_get(NIU_STATE_SHAKE), niu_car_state_get(NIU_STATE_INCHARGE));
    // LOG_D("INCHARGE: %d, FOC_ERR: %d, BMS_ERR: %d, DB_ERR: %d, LCU_ERR: %d", 
    //                                 niu_car_state_get(NIU_STATE_INCHARGE), 
    //                                 niu_car_state_get(NIU_STATE_FOC_ERR),
    //                                 niu_car_state_get(NIU_STATE_BMS_ERR),
    //                                 niu_car_state_get(NIU_STATE_DB_ERR),
    //                                 niu_car_state_get(NIU_STATE_LCU_ERR)
    //                                 );
    /*if (g_gsensor_int_status)
    {
        niu_gsensor_int();
        g_move_ex_int_value[g_move_ex_int_pos] = 1;
    }
    else */
    /*v3table shaking_value 0-4*/
    value = NIU_DATA_ECU(NIU_ID_ECU_SHAKING_VALUE);
    if(value)
    {
        memcpy(&shaking_value, value->addr, sizeof(NUINT8));
    }

    if(shaking_value == 0)
    {
        g_move_ex_int_value[g_move_ex_int_pos] = 0;
    }
    else
    {
        if (40 * shaking_value <= delta_x || 40 * shaking_value <= delta_y || 80 * shaking_value <= delta_z)
        {
            niu_gsensor_int();
            g_move_ex_int_value[g_move_ex_int_pos] = 1;
            LOG_D("shaking_value: %d, delta_x: %d, delta_y: %d, delta_z: %d, g_move_ex_int_pos: %d",
                    shaking_value, delta_x, delta_y, delta_z, g_move_ex_int_pos);
        }
        else
        {
            g_move_ex_int_value[g_move_ex_int_pos] = 0;
        }
    }
    

    if (JTRUE == niu_car_state_get(NIU_STATE_ACC) && (JTRUE == niu_car_state_get(NIU_STATE_MOVE_EX) || JTRUE == niu_car_state_get(NIU_STATE_SHAKE)))
    {
        memset(g_move_ex_int_value, 0, MOVE_EX_INT_NUM_MAX + 1);
        g_move_ex_int_pos = 0;
        niu_car_state_set(NIU_STATE_MOVE_EX, JFALSE);
        niu_car_state_set(NIU_STATE_SHAKE, JFALSE);
    }
    else if (JFALSE == niu_car_state_get(NIU_STATE_ACC))
    {
        NIU_DATA_VALUE *niu_move_time = NIU_DATA_ECU(NIU_ID_ECU_MOVE_GSENSOR_TIME);
        NIU_DATA_VALUE *niu_move_value = NIU_DATA_ECU(NIU_ID_ECU_MOVE_GSENSOR_COUNT);
        NUINT8 move_time = 0;
        NUINT8 move_value = 0;

        if (niu_move_time)
        {
            memcpy(&move_time, niu_move_time->addr, 1);
        }

        if (niu_move_value)
        {
            memcpy(&move_value, niu_move_value->addr, 1);
        }
        LOG_V("move_time: %d, move_value: %d", move_time, move_value);
        if (move_time && move_value && move_value <= move_time && move_time <= MOVE_EX_INT_NUM_MAX)
        {
            //if (/*g_gsensor_int_status || */niu_car_state_get(NIU_STATE_MOVE_EX))
            {
                NUINT8 i = 0;
                NUINT8 value = 0;
                for (i = 0; i < move_time; i++)
                {
                    if (i <= g_move_ex_int_pos)
                    {
                        value += g_move_ex_int_value[g_move_ex_int_pos - i];
                    }
                    else
                    {
                        value += g_move_ex_int_value[MOVE_EX_INT_NUM_MAX - (i - g_move_ex_int_pos)];
                    }
                }

                if (value == 0)
                {
                    niu_car_state_set(NIU_STATE_MOVE_EX, JFALSE);
                    niu_car_state_set(NIU_STATE_SHAKE, JFALSE);
                }
                else if (value >= move_value)
                {
                    niu_car_state_set(NIU_STATE_MOVE_EX, JTRUE);
                }
            }
        }
        else
        {
            niu_car_state_set(NIU_STATE_MOVE_EX, JFALSE);
        }
    }

    //g_gsensor_int_status = JFALSE;
    g_move_ex_int_pos = (g_move_ex_int_pos + 1) % MOVE_EX_INT_NUM_MAX;




    tmr_handler_tmr_add(TIMER_ID_GSENSOR_INT_VALUE, niu_gsensor_check_value, NULL, 500, 0);
}
#endif
JVOID niu_ext_data_update(int tmr_id, void *param)
{
    //gps
    //gsensor
    //printf("niu_ext_data_update!!\n");
    entry_niu_gps_data_update();
    //entry_niu_gsensor_data_update();
    entry_niu_gsm_data_update();
}

JVOID niu_test(int tmr_id, void *param)
{
    NIU_DATA_VALUE *value = NULL;
    
    if (tmr_id == TIMER_ID_TEST_HANDLER)
    {
    #if 0

        NUINT8 server_url[32] = {0};
        value = niu_data_read(NIU_ECU, NIU_ID_ECU_SERVER_URL);
        if(value)
        {
            memcpy(server_url, value->addr, value->len);
            //LOG_D("server_url: %s", server_url);
        }
        LOG_D("\n======================read server_url: %s=================\n", server_url);
        mcu_send_read(NIU_ECU, NIU_ID_ECU_SERVER_URL);
        //mcu_send_read(NIU_ECU, NIU_ID_ECU_CAR_STA);

    #else
        NINT32 utc = 0;
        value = niu_data_read(NIU_ECU, NIU_ID_ECU_UTC);
        if(value)
        {
            memcpy(&utc, value->addr, value->len);
        }
        LOG_D("\n======================read utc: %d=================\n", utc);
        mcu_send_read(NIU_ECU, NIU_ID_ECU_UTC);
    #endif
    }
}

/*
JVOID niu_test2(int tmr_id, void *param)
{
    int ret;
    unsigned char stm32_tea_key[16] = {0xE9,0x8D,0x86,0x6B,0x0A,0x83,0xB3,0x01,0xB5,0x62,0xE3,0xF0,0x26,0xEC,0x1A,0xFB};
    if (tmr_id == TIMER_ID_TEST_HANDLER2)
    {
        LOG_D("---------------------start upgrade --------------------");
        ret = TEA_decrypt_file(FOTA_UPGRADE_FILE_ENCRYPT_STM32, FOTA_UPGRADE_FILE_STM32, stm32_tea_key);
        if(ret != -1)
        {
            LOG_D("decrypt success[ret=%d].", ret);
            mcu_upgrade_to_stm32(FOTA_UPGRADE_FILE_STM32);
        }
        else
        {
            LOG_D("decrypt error[ret=%d].", ret);
        }
    }
}
*/

JSTATIC JVOID niu_stop_gps(int tmr_id, void *param);
JSTATIC JVOID niu_start_gps(int tmr_id, void *param)
{
    NIU_DATA_VALUE *gps_awake_time = NIU_DATA_ECU(NIU_ID_ECU_ECU_GPS_AWAKE_TIME);
    NIU_DATA_VALUE *ecu_cfg = NIU_DATA_ECU(NIU_ID_ECU_ECU_CFG);
    NUINT32 gps_awake_time_value = 60;
    NUINT16 ecu_cfg_value = 0;

    if (ecu_cfg)
    {
        memcpy(&ecu_cfg_value, ecu_cfg->addr, 2);
    }
    LOG_D("ecu_cfg_value:0x%x", ecu_cfg_value);
    if (0 == (ecu_cfg_value & 0x1000)) //关闭GPS
    {
        if (gps_awake_time)
        {
            memcpy(&gps_awake_time_value, gps_awake_time->addr, 4);
        }

        LOG_D("gps_on:%d", ql_gps_get_onoff());
        ql_gps_power_on();
        LOG_D("gps_awake_time_value=%d", gps_awake_time_value);
        tmr_handler_tmr_add(TIMER_ID_NIU_CLOSE_GPS, niu_stop_gps, NULL, gps_awake_time_value * 1000, 0);
    }
}

JSTATIC JVOID niu_stop_gps(int tmr_id, void *param)
{
    NIU_DATA_VALUE *gps_sleep_time = NIU_DATA_ECU(NIU_ID_ECU_ECU_GPS_SLEEP_TIME);
    NUINT32 gps_sleep_time_value = 300;
    if (gps_sleep_time)
    {
        memcpy(&gps_sleep_time_value, gps_sleep_time->addr, 4);
    }
    LOG_D("gps_sleep_time:%d", gps_sleep_time_value);

    LOG_D("gps_on:%d", ql_gps_get_onoff());
    ql_gps_power_off();
    LOG_D("ql_gps_power_off.");
    tmr_handler_tmr_add(TIMER_ID_NIU_OPEN_GPS, niu_start_gps, NULL, gps_sleep_time_value * 1000, 0);
}

JSTATIC JVOID niu_workmode_callback(J_WORK_MODE mode)
{
    LOG_D("mode: %d", mode);

    if (J_WORK_MODE_SLEEP == mode)
    {
        niu_car_state_set(NIU_STATE_SHAKE, JFALSE);
    }
    else if (JFALSE == niu_car_state_get(NIU_STATE_ACC))
    {
        niu_car_state_set(NIU_STATE_SHAKE, JTRUE);
    }

    if(mode != J_WORK_MODE_SLEEP)
    {
        ql_gps_power_on();
    }
    else
    {
        ql_gps_power_off();
        LOG_D("ql_gps_power_off.");
    }

    if (J_WORK_MODE_SLEEP == mode)
    {
        NIU_DATA_VALUE *gps_sleep_time = NIU_DATA_ECU(NIU_ID_ECU_ECU_GPS_SLEEP_TIME);
        NUINT32 gps_sleep_time_value = 300;
        if (gps_sleep_time)
        {
            memcpy(&gps_sleep_time_value, gps_sleep_time->addr, 4);
        }
        LOG_D("gps_sleep_time:%d", gps_sleep_time_value);
        g_niu_sleep_start_time = niu_device_get_timestamp();

        tmr_handler_tmr_add(TIMER_ID_NIU_OPEN_GPS, niu_start_gps, NULL, gps_sleep_time_value * 1000, 0);
    }
    else
    {
        tmr_handler_tmr_del(TIMER_ID_NIU_CLOSE_GPS);
        tmr_handler_tmr_del(TIMER_ID_NIU_OPEN_GPS);
        g_niu_sleep_start_time = 0;
    }
}

JVOID niu_entry()
{
    J_WORK_MODE_INIT_PARA work_mode_para = {0};

    LOG_D("begin niu_entry!");
    niu_data_init();
    niu_data_ecu_state_init();
    /*init v3 table and config data*/
    mcu_data_init();
    /* init gps data */
    /* init gsensor data */

    //modification by pony.ma
    //gsensor_init();
    sleep(2); //wait for mcu_data_thread init
    ql_app_ready(37, PINLEVEL_LOW);
    //niu_gsensor_check_value(TIMER_ID_GSENSOR_INT_VALUE, NULL);
    tmr_handler_tmr_add(TIMER_ID_NIU_UPDATE_DATA, niu_ext_data_update, NULL, 1*1000, 1);
    //end pony.ma

    //tmr_handler_tmr_add(TIMER_ID_TEST_HANDLER, niu_test, NULL, 8*1000, 1);
    //tmr_handler_tmr_add(TIMER_ID_TEST_HANDLER2, niu_test2, NULL, 20*1000, 0);
    //tmr_handler_tmr_add(TIMER_ID_NIU_UPDATE_TEMPLATE, niu_update_trigger_template, NULL, 1000, 1);

/*     work_mode_para.notify_func = niu_workmode_callback;
    work_mode_para.time_to_sleep = 60;
    work_mode_para.use_gps = JTRUE;
    niu_work_mode_init(&work_mode_para); */
    gps_init();
}
