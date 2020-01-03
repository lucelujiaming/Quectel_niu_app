#include <ql_oe.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <ql_lpm.h>
#include <time.h>
#include "low_power.h"
#include "sys_core.h"
#include "ql_customer.h"
#include "niu_data_pri.h"
#include "MQTTClient.h"
#include "niu_thread.h"
#include "niu_agps.h"
#include "gps_data.h"

#define CLOCKID             (8)	//CLOCK_REALTIME_ALARM, support wakeup system
#define WAKELOCK_NUM	    6
#define GSENSOR_NETLINK     30
#define MAX_PAYLOAD		    24
#define LPM_WAKEUPOUT_PIN	23	//GPIO26
#define LPM_WAKEUPIN_PIN	1	//GPIO25
#define GSENSOR_HANDLE_TIME 20
static int g_gps_position_flag = 0;

lock_timer_t g_lock_time[WAKELOCK_NUM] = {
    {
        .lock_name = "mcu_lock",
        .lock_flag = 0
    },
    {
        .lock_name = "watchdog",
        .lock_flag = 0
    },
    {
        .lock_name = "heartbeat",
        .lock_flag = 0
    },
    {
        .lock_name = "agps_update",
        .lock_flag = 0
    },
    {
        .lock_name = "template_trigger",
        .lock_flag = 0
    },
    {
        .lock_name = "gsensor_lock",
        .lock_flag = 0
    }
};

void timeout_handler_cb(union sigval val)
{
    struct timespec interval_time;
    struct niu_gps_data gps_data;
    
    if((LOCK_TYPE_MCU != val.sival_int) && (LOCK_TYPE_GSENSOR != val.sival_int)) {
        if(0 == g_lock_time[val.sival_int].lock_flag) {
            if(0 != Ql_SLP_WakeLock_Lock(g_lock_time[val.sival_int].lock_fd)) {
                Ql_SLP_WakeLock_Destroy(g_lock_time[val.sival_int].lock_fd);
                LOG_D("lock system wakeup state failed, source is %d", val.sival_int);
                return;
            }            
            cmd_set_lowpower(0);
            g_lock_time[val.sival_int].lock_flag = 1;
        }
    }

    if(LOCK_TYPE_MCU == val.sival_int) {
        memset(&gps_data, 0, sizeof(struct niu_gps_data));
        gps_data_read(&gps_data);
        if (0 == gps_data.valid) {  //gps position failed        
            gsensor_wakeup_handler(60);  //keep system wakeup 1min    
            LOG_D("wakeup system in order to gps position success"); 
        }  
    }
    else if(LOCK_TYPE_WATCHDOG == val.sival_int) {
        if(1 == g_lock_time[LOCK_TYPE_WATCHDOG].lock_flag) {
            ql_gpio_feed_dog(100);
            interval_time.tv_sec = 60;
            interval_time.tv_nsec = 0;
            LOG_D("watch dog wakeup at %d", time(NULL));
            low_power_check(LOCK_TYPE_WATCHDOG, interval_time);
        }
    }
    else if(LOCK_TYPE_HEARTBEAT == val.sival_int) {
        if(1 == g_lock_time[LOCK_TYPE_HEARTBEAT].lock_flag) {
            mqtt_client_keep_alive(0, NULL);
            LOG_D("heart beat wakeup at %d", time(NULL));
        }
    }
    else if(LOCK_TYPE_AGPS == val.sival_int) {
        if(1 == g_lock_time[LOCK_TYPE_AGPS].lock_flag) {
            if(!ql_gps_get_onoff()) {
                ql_gps_power_on();
            }
            get_agps_infomation();
            low_power_check(LOCK_TYPE_AGPS,get_agps_time_out());
            LOG_D("agps wakeup at %d", time(NULL));
        }
    }
    else if(LOCK_TYPE_TEMPLATE == val.sival_int) {
        if(1 == g_lock_time[LOCK_TYPE_TEMPLATE].lock_flag) {
            LOG_D("template trigger wakeup at %d", time(NULL));
            //check_template_send();
            tmr_handler_tmr_add(TIMER_ID_TEMPLATE_EMPTY_LOW_POWER, empyt_template_low_power, NULL, 30 * 1000, 0);
        }
    }
    else if(LOCK_TYPE_GSENSOR == val.sival_int) {
        if(1 == g_lock_time[LOCK_TYPE_GSENSOR].lock_flag) {
            LOG_D("gsensor set sleep down flag at %d", time(NULL));
            memset(&interval_time, 0, sizeof(struct timespec));
            low_power_check(LOCK_TYPE_GSENSOR, interval_time);
        }        
    }
}

void mcu_suspend_resume_handler_cb(ql_lpm_edge_t edge_state)
{
    struct timespec interval_time;

	switch(edge_state)
	{
        case E_QL_LPM_RISING:
        if (0 == g_lock_time[LOCK_TYPE_MCU].lock_flag) {
            if(0 != Ql_SLP_WakeLock_Lock(g_lock_time[LOCK_TYPE_MCU].lock_fd)){
                Ql_SLP_WakeLock_Destroy(g_lock_time[LOCK_TYPE_MCU].lock_fd);
                LOG_D("mcu wakeup system fialed");
            }
            Ql_GPIO_SetLevel(LPM_WAKEUPOUT_PIN, PINLEVEL_HIGH);
            cmd_set_lowpower(0); 
            LOG_D("===mcu wake up system===");
            g_lock_time[LOCK_TYPE_MCU].lock_flag = 1;
        }		
		break;
	case E_QL_LPM_FALLING:    
        if(1 == g_lock_time[LOCK_TYPE_MCU].lock_flag) {
            LOG_D("===mcu suspend system==="); 
            Ql_GPIO_SetLevel(LPM_WAKEUPOUT_PIN, PINLEVEL_LOW);
            g_lock_time[LOCK_TYPE_MCU].lock_flag = 0;
            interval_time.tv_sec = 0;
            interval_time.tv_nsec = 0;            
            low_power_check(LOCK_TYPE_MCU, interval_time);
        }
		break;
	default:
		break;
	}
}

static void *thread_read_gps_valid(void *arg)
{
    struct niu_gps_data gps_data;
    struct timespec interval_time;
    static unsigned char gps_valid_flag = 0;

    while (1)
    {
        if (1 == g_gps_position_flag) {            
            memset(&gps_data, 0, sizeof(struct niu_gps_data));
            gps_data_read(&gps_data);
            if (1 == gps_data.valid) {
                if (0 == gps_valid_flag) {
                    gps_valid_flag = 1;
                    LOG_D("GPS Positioning success, lock_flag: %d", g_lock_time[LOCK_TYPE_GSENSOR].lock_flag);
                    sleep(3);
                    if(1 == g_lock_time[LOCK_TYPE_GSENSOR].lock_flag) {
                        memset(&interval_time, 0, sizeof(struct timespec));
                        low_power_check(LOCK_TYPE_GSENSOR, interval_time);
                    } 
                }
            }
            else {
                if (1 == gps_valid_flag) {
                    LOG_D("GPS Positioning failed");
                    gps_valid_flag = 0;
                }
            }
        }

        sleep(1);
    }

    return (void *)1;
}

void low_power_init()
{
    int i = 0, ret = 0;
    struct itimerspec rtc_it;
    pthread_t ptd;
    
	QL_Lpm_Cfg_T ql_mcu_cfg ={
		.wakeupin.wakeupin_pin = LPM_WAKEUPIN_PIN,
		.wakeupin.wakeupin_edge = E_QL_LPM_RISING,
		.wakeupout.wakeupout_pin = LPM_WAKEUPOUT_PIN,
		.wakeupout.wakeupout_edge = E_QL_LPM_RISING,
	};

    for(i = 0; i < WAKELOCK_NUM; i++) {
        if(i == LOCK_TYPE_MCU) {
            QL_Lpm_Deinit();
            system("rmmod ql_lpm && sync");
            usleep(100*1000);
	        ret = QL_Lpm_Init(mcu_suspend_resume_handler_cb, &ql_mcu_cfg);
	        if(0 != ret) {
		        LOG_D("lock power init failed while mcu gpio init failed");
		        return;
	        }
	        Ql_GPIO_Init(LPM_WAKEUPOUT_PIN, PINDIRECTION_OUT, PINLEVEL_HIGH, PINPULLSEL_DISABLE);	
        }

        g_lock_time[i].lock_fd=Ql_SLP_WakeLock_Create(g_lock_time[i].lock_name, strlen(g_lock_time[i].lock_name));

        if((i != LOCK_TYPE_MCU) && (i != LOCK_TYPE_AGPS)) {
            ret = Ql_SLP_WakeLock_Lock(g_lock_time[i].lock_fd);
            if(0 != ret) {
		        Ql_SLP_WakeLock_Destroy(g_lock_time[i].lock_fd);
		        LOG_D("lock power init failed while initialize lock %s, index is %d, ret is %d", g_lock_time[i].lock_name, i + 1, ret);
		        return;
	        }
            g_lock_time[i].lock_flag = 1;
        }
        else {
            ret = Ql_SLP_WakeLock_Unlock(g_lock_time[i].lock_fd);
            if(0 != ret) {
		        Ql_SLP_WakeLock_Destroy(g_lock_time[i].lock_fd);
		        LOG_D("lock power init failed while initialize lock %s, index is %d, ret is %d", g_lock_time[i].lock_name, i + 1, ret);
		        return;
	        }
            g_lock_time[i].lock_flag = 0;
        }
        memset(&g_lock_time[i].rtc_evp, 0, sizeof(struct sigevent));
	    g_lock_time[i].rtc_evp.sigev_value.sival_int = i;
	    g_lock_time[i].rtc_evp.sigev_notify = SIGEV_THREAD;
	    g_lock_time[i].rtc_evp.sigev_notify_function = timeout_handler_cb;
    	
        ret = timer_create(CLOCKID, &g_lock_time[i].rtc_evp, &g_lock_time[i].timer_id);
	    if(0 != ret) {
		    LOG_D("timer create failed, lock %s, index is %d, ret is %d", g_lock_time[i].lock_name, i + 1, ret);
		    return;
	    }
        if(i == LOCK_TYPE_AGPS) {
            low_power_check(LOCK_TYPE_AGPS, get_agps_time_out());
        }
    }
   
    ret = pthread_create(&ptd, NULL, thread_read_gps_valid, NULL);
    if(ret)
    {
        LOG_E("thread create thread_read_gps_valid error.");
        return;
    }

    rtc_it.it_interval.tv_sec = 20*60;  //once 20min power on gps
    rtc_it.it_interval.tv_nsec = 0;
    rtc_it.it_value.tv_sec = 20*60;     //20min
    rtc_it.it_value.tv_nsec = 0;    
    if (-1 == timer_settime(g_lock_time[LOCK_TYPE_MCU].timer_id, 0, &rtc_it, NULL)) {
        LOG_D("failed to set time at %s, errno is %d, err is %s", g_lock_time[LOCK_TYPE_MCU].lock_name, errno, strerror(errno));
    }

    gsensor_wakeup_handler(3*60);  //keep system wakeup 3min 

    LOG_D("lock and timer init succeed");
    Ql_Autosleep_Enable(1);
}

void lock_release(LOCK_TYPE_ID lock_id, struct timespec interval_time, unsigned char release_flag)
{
    struct itimerspec rtc_it;
	rtc_it.it_interval.tv_sec = 0;
	rtc_it.it_interval.tv_nsec = 0;

	rtc_it.it_value.tv_sec = interval_time.tv_sec;
	rtc_it.it_value.tv_nsec = 0;
    if(rtc_it.it_value.tv_sec != 0 || rtc_it.it_value.tv_nsec != 0) {
        if(-1 == timer_settime(g_lock_time[lock_id].timer_id, 0, &rtc_it, NULL)) {
		LOG_D("failed to set time at %s, while release lock, errno is %d, err is %s, time_sec is %d, time_nsec is %d", 
                g_lock_time[lock_id].lock_name, errno, strerror(errno), 
                rtc_it.it_value.tv_sec, rtc_it.it_value.tv_nsec);
	    }
    }

    if (1 == release_flag) {
        g_lock_time[lock_id].lock_flag = 0;
        if(0 != Ql_SLP_WakeLock_Unlock(g_lock_time[lock_id].lock_fd)) {
            Ql_SLP_WakeLock_Destroy(g_lock_time[lock_id].lock_fd);
            LOG_D("failed to release lock, lock name is %s", g_lock_time[lock_id].lock_name);
        }
    }    
}

void low_power_check(LOCK_TYPE_ID lock_id, struct timespec interval_time)
{
    int i = 0;
    int acc_on_flag = 0;
    NUINT32 car_sta = 0;

    if(lock_id > LOCK_TYPE_GSENSOR || lock_id < LOCK_TYPE_MCU)
    {
        LOG_E("lock id invalid[id=%d].", lock_id);
        return;
    } 
    if (LOCK_TYPE_GSENSOR == lock_id) {
        g_gps_position_flag = 0;
    }      

    acc_on_flag = niu_car_state_get(NIU_STATE_ACC);
    NIU_DATA_VALUE *value = NIU_DATA_ECU(NIU_ID_ECU_CAR_STA);
    if(value)
    {
        memcpy(&car_sta, value->addr, value->len);
    }
    //LOG_D("acc: %d, car_sta: 0x%04X.", acc_on_flag, car_sta);
    LOG_D("release lock_name: %s", g_lock_time[lock_id].lock_name);

    // for(i = LOCK_TYPE_MCU; i <= LOCK_TYPE_GSENSOR; i++) {  
    //     LOG_D("name: %s, lock_flag: %d.", g_lock_time[i].lock_name, g_lock_time[i].lock_flag);
    // }
    for(i = LOCK_TYPE_MCU; i <= LOCK_TYPE_GSENSOR; i++) {        
        if (lock_id != i) {
            if (1 == g_lock_time[i].lock_flag) {
                lock_release(lock_id, interval_time, 1);
                return;
            }
        }        
    }

    if (0 == acc_on_flag) {
        LOG_D("system suspend in low_power_check.");
        cmd_set_lowpower(1);
        if(ql_gps_get_onoff()) {
            LOG_D("ql_gps_power_off.");
            ql_gps_power_off();
        }
        lock_release(lock_id, interval_time, 1);
    }
    else {
        lock_release(lock_id, interval_time, 0);
    }
}

void gsensor_entry_suspend()
{
    if(g_lock_time[LOCK_TYPE_GSENSOR].lock_flag == 1) {
		g_lock_time[LOCK_TYPE_GSENSOR].lock_flag = 0;
        //set_leds(0);
		if(0 != Ql_SLP_WakeLock_Unlock(g_lock_time[LOCK_TYPE_GSENSOR].lock_fd)) {
			Ql_SLP_WakeLock_Destroy(g_lock_time[LOCK_TYPE_GSENSOR].lock_fd);
			LOG_D("unlock g_gsensor wakelock failed");
			return;
		}
        LOG_D("gsensor lock release.");
	}
}

void gsensor_wakeup_handler(int gsensor_handle_time)
{
    struct itimerspec rtc_it;
    if(0 == g_lock_time[LOCK_TYPE_GSENSOR].lock_flag) {		
		if(0 != Ql_SLP_WakeLock_Lock(g_lock_time[LOCK_TYPE_GSENSOR].lock_fd)) {
			Ql_SLP_WakeLock_Destroy(g_lock_time[LOCK_TYPE_GSENSOR].lock_fd);
			LOG_D("lock g_gsensor wakelock failed");
			return;
		}
        cmd_set_lowpower(0);
        g_lock_time[LOCK_TYPE_GSENSOR].lock_flag = 1;
    }

    if(!ql_gps_get_onoff()) {
        LOG_D("gsensor wakeup system and gps power on");
        ql_gps_power_on();
    }
    
    LOG_D("gsensor wakeup system at %d", time(NULL));
    g_gps_position_flag = 1;

    rtc_it.it_interval.tv_sec = 0;
    rtc_it.it_interval.tv_nsec = 0;
    rtc_it.it_value.tv_sec = gsensor_handle_time;
    rtc_it.it_value.tv_nsec = 0;
    if(-1 == timer_settime(g_lock_time[LOCK_TYPE_GSENSOR].timer_id, 0, &rtc_it, NULL)) {
        LOG_D("failed to set time at %s, while gsensor set release time, errno is %d, err is %s", g_lock_time[LOCK_TYPE_GSENSOR].lock_name, errno, strerror(errno));
    }
}

void set_leds(int net_light)
{
    if(1 == net_light) {
        ql_leds_gpio_ctrl("net_status", LEDS_STATUS_HIGH, 0, 0);
    }
    else {
        ql_leds_gpio_ctrl("net_status", LEDS_STATUS_LOW, 0, 0);
    }
}

void set_template_lock()
{
    struct itimerspec rtc_it;
	
	rtc_it.it_interval.tv_sec = 0;
	rtc_it.it_interval.tv_nsec = 0;

	rtc_it.it_value.tv_sec = 99999;
	rtc_it.it_value.tv_nsec = 0;
	if(-1 == timer_settime(g_lock_time[LOCK_TYPE_TEMPLATE].timer_id, 0, &rtc_it, NULL)) {
		LOG_D("failed to set template lock time errno is %d, err is %s", errno, strerror(errno));
	}
    if(0 == g_lock_time[LOCK_TYPE_TEMPLATE].lock_flag) {
        g_lock_time[LOCK_TYPE_TEMPLATE].lock_flag = 1;
	    if(0 != Ql_SLP_WakeLock_Lock(g_lock_time[LOCK_TYPE_TEMPLATE].lock_fd)) {
		    Ql_SLP_WakeLock_Destroy(g_lock_time[LOCK_TYPE_TEMPLATE].lock_fd);
		    LOG_D("template failed to lock");
	    }
    }
}

// void set_gsensor_sleep(int flag)
// {
//     struct itimerspec gsensor_init_timer;
//     memset(&gsensor_init_timer, 0, sizeof(struct itimerspec));
//     gsensor_init_timer.it_interval.tv_sec = 0;
// 	gsensor_init_timer.it_interval.tv_nsec = 0;
// 	gsensor_init_timer.it_value.tv_nsec = 0;
	
//     g_gsensor_sleep = flag;
//     if(0 == flag) {
//         gsensor_init_timer.it_value.tv_sec = GSENSOR_HANDLE_TIME;
//         if(-1 == timer_settime(g_lock_time[LOCK_TYPE_GSENSOR].timer_id, 0, &gsensor_init_timer, NULL)) {
// 		    LOG_D("failed to set init gsensor time");
// 	    }
//     }
// }

struct timespec get_agps_time_out()
{ 
    time_t time_sec;
    struct tm date_time;
    struct timespec interval_time;

    time_sec = time((time_t *)NULL);
    date_time = *gmtime(&time_sec);
    interval_time.tv_sec = 24 * 3600 - date_time.tm_hour * 3600 - date_time.tm_min * 60 - date_time.tm_sec;
    interval_time.tv_nsec = 0;
    return interval_time;
}

void get_agps_infomation()
{
    double latitude = 0;
    double longitude = 0;
    NIU_DATA_VALUE *temp_niu_data = NULL;

    temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_LATITUDE);
    latitude = *((double*)temp_niu_data->addr);
    temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_LONGITUDE);
    longitude = *((double*)temp_niu_data->addr);

    niu_agps_mga_online(longitude, latitude);
}
