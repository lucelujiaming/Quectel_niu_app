#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ql_oe.h>
#include "gps_data.h"
#include "gsensor_data.h"
#include "service.h"
#include "utils.h"
#include "niu_work_mode.h"


#define J_GPS_V_ARRAY_LEN (40)
JSTATIC J_WORK_MODE_INIT_PARA g_init_para = {0};
JSTATIC J_WORK_MODE g_work_mode = J_WORK_MODE_ACTIVE;
JSTATIC JBOOL g_keep_active = JFALSE;
JSTATIC JBOOL g_workmode_enable = JTRUE;

JSTATIC NUINT32 g_gsensor_move_count = 0;
JSTATIC NUINT32 g_gsensor_still_count = 0;
JSTATIC NUINT32 g_gps_v[J_GPS_V_ARRAY_LEN] = {0};
JSTATIC NUINT8 g_gps_v_index = 0;
JSTATIC JBOOL g_gps_v_valid = JFALSE;
JSTATIC NUINT16 g_gps_max_v = 0;
JSTATIC JBOOL g_gps_data_valid = FALSE;

JSTATIC NUINT16 g_gsensor_max_x = 0;
JSTATIC NUINT16 g_gsensor_max_y = 0;
JSTATIC NUINT16 g_gsensor_max_z = 0;

JSTATIC NUINT16 g_gsensor_prev_x = 0;
JSTATIC NUINT16 g_gsensor_prev_y = 0;
JSTATIC NUINT16 g_gsensor_prev_z = 0;

JSTATIC NINT16 g_gsensor_value_x = 0;
JSTATIC NINT16 g_gsensor_value_y = 0;
JSTATIC NINT16 g_gsensor_value_z = 0;

JSTATIC NUINT32 g_gsensor_delta_x = 0;
JSTATIC NUINT32 g_gsensor_delta_y = 0;
JSTATIC NUINT32 g_gsensor_delta_z = 0;


JSTATIC JVOID niu_work_mode_gps_data_valid(int tmr_id, void *param)
{
    if(tmr_id != TIMER_ID_GPSDATA_VALID)
    {
        LOG_E("tmr_id(%d) != TIMER_ID_GPSDATA_VALID", tmr_id);
        return;
    }
    LOG_D("g_gps_data_valid:%d", g_gps_data_valid);
    g_gps_data_valid = JTRUE;
}


JSTATIC JVOID niu_work_mode_to_sleep(int tmr_id, void *param)
{

    if(tmr_id != TIMER_ID_WORKMODE_TO_SLEEP)
    {
        LOG_E("tmr_id(%d) != TIMER_ID_WORKMODE_TO_SLEEP", tmr_id);
        return;
    }

    LOG_D("g_work_mode:%d, g_keep_active:%d, use_gps:%d", g_work_mode, g_keep_active, g_init_para.use_gps);

    if (JFALSE == g_keep_active)
    {
        if (g_init_para.use_gps && g_workmode_enable)
        {
            ql_gps_power_off();
            LOG_D("ql_gps_power_off.");
            g_gps_v_index = 0;
            g_gps_v_valid = JFALSE;
            g_gps_data_valid = JFALSE;
        }

        g_work_mode = J_WORK_MODE_SLEEP;

        if (g_init_para.notify_func && g_workmode_enable)
        {
            g_init_para.notify_func(g_work_mode);
        }
    }
}

JSTATIC JVOID niu_work_mode_to_active()
{
    LOG_D("g_work_mode:%d, g_keep_active:%d, use_gps:%d", g_work_mode, g_keep_active, g_init_para.use_gps);
    tmr_handler_tmr_del(TIMER_ID_WORKMODE_TO_SLEEP);
    if (g_init_para.use_gps && JFALSE == ql_gps_get_onoff() && g_workmode_enable)
    {
        ql_gps_power_on();
    }
}


JSTATIC JVOID niu_work_mode_data_update(int tmr_id, void *param)
{
    struct niu_gps_data gps_data = {0};
    BMI_SENSOR_DATA gsensor_data;
    JBOOL gps_driving_flag = JFALSE;
    J_WORK_MODE new_work_mode = g_work_mode;
    NUINT32 delta_x = 0;
    NUINT32 delta_y = 0;
    NUINT32 delta_z = 0;

    NUINT32 x, y, z, range_mid = 2048;

    if(tmr_id != TIMER_ID_WORKMODE_DATA_UPDATE)
    {
        LOG_E("tmr_id(%d) != TIMER_ID_WORKMODE_DATA_UPDATE", tmr_id);
        return;
    }
    gsensor_data_read(&gsensor_data);

    x = gsensor_data.accdata[X_AXIS];
    y = gsensor_data.accdata[Y_AXIS];
    z = gsensor_data.accdata[Z_AXIS];

    gps_data_read(&gps_data);

    if(JFALSE == gps_data.valid)
    {
        memset(g_gps_v, 0, J_GPS_V_ARRAY_LEN * sizeof(NUINT32));
        g_gps_v_index = 0;
        g_gps_v_valid = JFALSE;
        g_gps_data_valid = JFALSE;
        tmr_handler_tmr_del(TIMER_ID_GPSDATA_VALID);
    }
    else if (JTRUE == g_gps_data_valid)
    {
        g_gps_max_v = jmax(g_gps_max_v, gps_data.gps_speed);
        g_gps_v[g_gps_v_index] = gps_data.gps_speed;
        g_gps_v_index++;
        if (J_GPS_V_ARRAY_LEN <= g_gps_v_index)
        {
            g_gps_v_index = 0;
            g_gps_v_valid = JTRUE;
        }
    }
    else if (JFALSE == tmr_handler_tmr_is_exist(TIMER_ID_GPSDATA_VALID))
    {
        tmr_handler_tmr_add(TIMER_ID_GPSDATA_VALID, niu_work_mode_gps_data_valid, NULL, 30 * 1000, 0);
    }

    if (x != 0 && y != 0 && z != 0 && g_gsensor_prev_x != 0 && g_gsensor_prev_y != 0 && g_gsensor_prev_z != 0)
    {
        NUINT16 gsensor_x = 0;
        NUINT16 gsensor_y = 0;
        NUINT16 gsensor_z = 0;

        delta_x = (g_gsensor_prev_x > x) ? (g_gsensor_prev_x - x) : (x - g_gsensor_prev_x);
        delta_y = (g_gsensor_prev_y > y) ? (g_gsensor_prev_y - y) : (y - g_gsensor_prev_y);
        delta_z = (g_gsensor_prev_z > z) ? (g_gsensor_prev_z - z) : (z - g_gsensor_prev_z);
        gsensor_x = (range_mid > x) ? (range_mid - x) : (x - range_mid);
        gsensor_y = (range_mid > y) ? (range_mid - y) : (y - range_mid);
        gsensor_z = (range_mid > z) ? (range_mid - z) : (z - range_mid);

        g_gsensor_max_x = jmax(gsensor_x, g_gsensor_max_x);
        g_gsensor_max_y = jmax(gsensor_y, g_gsensor_max_y);
        g_gsensor_max_z = jmax(gsensor_z, g_gsensor_max_z);

        g_gsensor_value_x = x - range_mid;
        g_gsensor_value_y = y - range_mid;
        g_gsensor_value_z = z - range_mid;

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

    g_gsensor_delta_x = delta_x;
    g_gsensor_delta_y = delta_y;
    g_gsensor_delta_z = delta_z;

    if (40 <= delta_x || 40 <= delta_y || 80 <= delta_z)
    {
        g_gsensor_move_count++;
        if (10 <= g_gsensor_move_count)
        {
            g_gsensor_still_count = 0;
        }
        else if (g_gsensor_still_count)
        {
            g_gsensor_still_count++;
        }
    }
    else
    {
        g_gsensor_still_count++;
        if (10 <= g_gsensor_still_count)
        {
            g_gsensor_move_count = 0;
        }
        else if (g_gsensor_move_count)
        {
            g_gsensor_move_count++;
        }
    }
    if (g_gps_v_valid)
    {
        NUINT32 total_v = 0;
        NUINT32 max_v = 0;
        NUINT32 min_v = 0;
        NUINT32 i = 0;

        for (i = 0; i < J_GPS_V_ARRAY_LEN; i++)
        {
            total_v += g_gps_v[i];
            max_v = jmax(max_v, g_gps_v[i]);
            min_v = jmin(min_v, g_gps_v[i]);
        }
        //baron:　平均速度超过3，　则判定 处于　gps_driving
        if ((total_v - max_v - min_v) >= ((J_GPS_V_ARRAY_LEN - 2) * 3))
        {
            gps_driving_flag = JTRUE;
        }
    }
    if (JTRUE == gps_driving_flag && (g_gsensor_still_count < 60))
    {
        new_work_mode = J_WORK_MODE_DRIVING;
        g_gsensor_still_count = 0;
        g_gsensor_move_count = 60;
        tmr_handler_tmr_del(TIMER_ID_WORKMODE_TO_SLEEP);
    }
    else if (J_WORK_MODE_DRIVING == g_work_mode)
    {
        if (g_gsensor_still_count >= 60)
        {
            new_work_mode = J_WORK_MODE_ACTIVE;

            if (JFALSE == tmr_handler_tmr_is_exist(TIMER_ID_WORKMODE_TO_SLEEP))
            {
                if (g_init_para.time_to_sleep)
                {
                    tmr_handler_tmr_add(TIMER_ID_WORKMODE_TO_SLEEP, niu_work_mode_to_sleep, NULL, g_init_para.time_to_sleep * 1000, 0);
                }
                else
                {
                    tmr_handler_tmr_add(TIMER_ID_WORKMODE_TO_SLEEP, niu_work_mode_to_sleep, NULL, g_init_para.time_to_sleep * 1000, 0);
                }
            }
        }
        else
        {
            tmr_handler_tmr_del(TIMER_ID_WORKMODE_TO_SLEEP);
        }
    }
    else if (J_WORK_MODE_ACTIVE == g_work_mode)
    {
        if (g_gsensor_move_count >= 60)
        {
            new_work_mode = J_WORK_MODE_DRIVING;
            tmr_handler_tmr_del(TIMER_ID_WORKMODE_TO_SLEEP);
        }
        else if (g_gsensor_still_count >= 60)
        {
            if (JFALSE == tmr_handler_tmr_is_exist(TIMER_ID_WORKMODE_TO_SLEEP))
            {
                if (g_init_para.time_to_sleep)
                {
                    tmr_handler_tmr_add(TIMER_ID_WORKMODE_TO_SLEEP, niu_work_mode_to_sleep, NULL, g_init_para.time_to_sleep * 1000, 0);
                }
                else
                {
                    tmr_handler_tmr_add(TIMER_ID_WORKMODE_TO_SLEEP, niu_work_mode_to_sleep, NULL, 60 * 1000, 0);
                }
            }
        }
        else
        {
            tmr_handler_tmr_del(TIMER_ID_WORKMODE_TO_SLEEP);
        }
    }
    else if (J_WORK_MODE_SLEEP == g_work_mode)
    {
        if (g_gsensor_move_count >= 10)
        {
            new_work_mode = J_WORK_MODE_ACTIVE;
            tmr_handler_tmr_del(TIMER_ID_WORKMODE_TO_SLEEP);
        }
    }

   // LOG_D("driving_flag:%d, move_count:%d, static_count:%d, nw_mode:%d,gw_mode:%d,k_active:%d",
    //            gps_driving_flag, g_gsensor_move_count, g_gsensor_still_count, new_work_mode, g_work_mode, g_keep_active);

    if (g_work_mode != new_work_mode)
    {
        if (J_WORK_MODE_SLEEP == g_work_mode)
        {
            niu_work_mode_to_active();
        }

        g_work_mode = new_work_mode;

        if (g_init_para.notify_func && g_workmode_enable)
        {
            g_init_para.notify_func(g_work_mode);
        }
    }

    //LOG_D("g_work_mode:%d", g_work_mode);

    tmr_handler_tmr_add(TIMER_ID_WORKMODE_DATA_UPDATE, niu_work_mode_data_update, NULL, 500, 0);

}

J_WORK_MODE niu_work_mode_value()
{
    LOG_V("g_work_mode:%d", g_work_mode);
    if (JFALSE == tmr_handler_tmr_is_exist(TIMER_ID_WORKMODE_DATA_UPDATE) && (J_WORK_MODE_SLEEP != g_work_mode))
    {
        tmr_handler_tmr_add(TIMER_ID_WORKMODE_DATA_UPDATE, niu_work_mode_data_update, NULL, 500, 0);
    }
    return g_work_mode;
}

JVOID niu_work_mode_do_active()
{
    tmr_handler_tmr_del(TIMER_ID_WORKMODE_TO_SLEEP);
    g_gsensor_still_count = 0;
    if (g_work_mode == J_WORK_MODE_SLEEP)
    {
        niu_work_mode_to_active();
        g_gsensor_move_count = 5;
        g_work_mode = J_WORK_MODE_ACTIVE;
        if (g_init_para.notify_func && g_workmode_enable)
        {
            g_init_para.notify_func(g_work_mode);
        }
        if ( JFALSE == tmr_handler_tmr_is_exist(TIMER_ID_WORKMODE_DATA_UPDATE))
        {
            tmr_handler_tmr_add(TIMER_ID_WORKMODE_DATA_UPDATE, niu_work_mode_data_update, NULL, 500, 0);
        }
    }
}

JBOOL niu_work_mode_init(J_WORK_MODE_INIT_PARA* para)
{
    memcpy(&g_init_para, para, sizeof(J_WORK_MODE_INIT_PARA));
    g_work_mode = J_WORK_MODE_ACTIVE;
    g_workmode_enable = JTRUE;

    //gsensor switch

    if (g_init_para.use_gps)
    {
        ql_gps_power_on();
    }

    niu_work_mode_data_update(TIMER_ID_WORKMODE_DATA_UPDATE, NULL);
    return JTRUE;
}

NUINT32 niu_work_mode_still_count()
{
    LOG_D("g_gsensor_still_count:%d", g_gsensor_still_count);
    return g_gsensor_still_count;
}

NUINT32 niu_work_mode_move_count()
{
    LOG_D("g_gsensor_move_count:%d", g_gsensor_move_count);
    return g_gsensor_move_count;
}