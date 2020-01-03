#ifndef __LOW_POWER_H__
#define __LOW_POWER_H__

#include <ql_oe.h>
#include <ql_lpm.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "service.h"
typedef enum LOCK_ID {
    LOCK_TYPE_MCU = 0,
    LOCK_TYPE_WATCHDOG,
    LOCK_TYPE_HEARTBEAT,
    LOCK_TYPE_AGPS,
    LOCK_TYPE_TEMPLATE,
    LOCK_TYPE_GSENSOR
}LOCK_TYPE_ID;

typedef struct lock_timer_struct {
    timer_t timer_id;
    char lock_name[MAX_CHAR];
    int lock_fd;
    struct sigevent rtc_evp;
    int lock_flag;
}lock_timer_t;

void gsensor_wakeup_handler(int gsensor_handle_time);
void low_power_init();
void low_power_check(LOCK_TYPE_ID lock_id, struct timespec interval_time);
void set_template_lock();
void set_gsensor_sleep(int flag);
void set_leds(int net_light);
void gsensor_entry_suspend();
struct timespec get_agps_time_out();
void get_agps_infomation();
void gsensor_wakeup_handler(int gsensor_handle_time);
#endif
