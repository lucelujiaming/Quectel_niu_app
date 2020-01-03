/*-----------------------------------------------------------------------------------------------*/
/**
  @file thread_tmr.c
  @brief Implementation of framework timer
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
#include "ql_tmrq.h"

#define TMR_HANDLER_MAX_LEN_DEFAULT 100


static pthread_t g_thread_tmr;
static ql_tmrq_handle_t g_tmr_handler;
static volatile int g_is_peeding = 0;
static volatile uint32_t g_tmr_ms_peeding_start = 0;

void thread_tmr_handler_proc(void * param)
{
    int ret;
    ql_tmrq_item_t *item;

    LOG_V("start, enter main");
    while(1) {

        item = NULL;
        ret = ql_tmrq_pull(&g_tmr_handler, &item); 
        if(ret < 0) {
            LOG_F("Failed to pull a timer item");
        }
        
        if(item != NULL) {
            if(item->cmd) {
               ((tmr_handler_func_f)item->cmd)(item->id, item->param);
            }
            else {
                LOG_W("tmr item cmd is null");
            }
            ql_tmrq_item_free(&g_tmr_handler, item);
        }
    }
}

int tmr_handler_init(void)
{
    int ret;

    ret = ql_tmrq_init(&g_tmr_handler, TMR_HANDLER_MAX_LEN_DEFAULT);
    if(ret != 0) {
        LOG_F("Failed to ql_tmrq_init, ret=%d", ret);
        return -1;
    }

    ret = pthread_create(&g_thread_tmr, NULL, (void*)thread_tmr_handler_proc, NULL);
    if(ret != 0) {
        LOG_F("Failed to pthread_create, err=%s", strerror(errno));
        return -1;
    }

    return 0;
}

int tmr_handler_tmr_add(int tmr_id, tmr_handler_func_f func, void *param, uint32_t interval_ms, int is_cycle)
{
    return ql_tmrq_tmr_add(tmr_id, &g_tmr_handler, (void*)func, param, 0, interval_ms, is_cycle);
}

int tmr_handler_tmr_set(int tmr_id, tmr_handler_func_f func, void *param, uint32_t interval_ms, int is_cycle)
{
    return ql_tmrq_tmr_set(&g_tmr_handler, tmr_id, (void*)func, param, 0, interval_ms, is_cycle);
}

int tmr_handler_tmr_del(int tmr_id)
{
    return ql_tmrq_tmr_del(&g_tmr_handler, tmr_id);
}

int tmr_handler_tmr_is_exist(int tmr_id)
{
    return ql_tmrq_tmr_is_exist(&g_tmr_handler, tmr_id);
}

int tmr_handler_tmr_time_remaining(int tmr_id, int *remaining_sec, int *remaining_usec)
{
    uint32_t curr_sec = 0;
    uint32_t curr_usec = 0;

    ql_tmrq_item_t *item = NULL;

    if((!remaining_sec) || (!remaining_usec)) {
        LOG_E("Invalid param");
        return -1;
    }

    QL_MUTEX_LOCK(&g_tmr_handler.mutex);
    if(com_list_empty(&g_tmr_handler.list)) {
        QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
        return -1;
    }
    else {
        if(0 == tmr_handler_tmr_is_exist(tmr_id)) {
            QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
            return -1;
        }
        else {
            com_list_for_each_entry(item, &g_tmr_handler.list, next) {
                if(item->id == tmr_id) {
                    ql_com_get_uptime(&curr_sec, &curr_usec);
                    *remaining_sec = item->expire_sec - curr_sec;
                    *remaining_usec = item->expire_usec - curr_usec;
                    QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
                    return 0;
                }
            }
            QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
            return -1;
        }
    }
}

int tmr_handler_tmr_min_time_remaining(int *remaining_sec, int *remaining_usec)
{
    uint32_t curr_sec = 0;
    uint32_t curr_usec = 0;

    ql_tmrq_item_t *item = NULL;
    
    if((!remaining_sec) || (!remaining_usec)) {
        LOG_E("Invalid param");
        return -1;
    }

    QL_MUTEX_LOCK(&g_tmr_handler.mutex);
    if(com_list_empty(&g_tmr_handler.list)) {
        QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
        return -1;
    }

    ql_com_get_uptime(&curr_sec, &curr_usec);
    item = com_list_first_entry(&g_tmr_handler.list, ql_tmrq_item_t, next);
    *remaining_sec = item->expire_sec - curr_sec;
    *remaining_usec = item->expire_usec - curr_usec;
    QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
    return item->id;
}

int tmr_handler_tmr_template_min_time_remaining(int *remaining_sec, int *remaining_usec)
{
    uint32_t curr_sec = 0;
    uint32_t curr_usec = 0;

    ql_tmrq_item_t *item = NULL;

    if((!remaining_sec) || (!remaining_usec)) {
        LOG_E("Invalid param");
        return -1;
    }

    QL_MUTEX_LOCK(&g_tmr_handler.mutex);
    if(com_list_empty(&g_tmr_handler.list)) {
        QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
        return -1;
    }
    else {
        com_list_for_each_entry(item, &g_tmr_handler.list, next) {
            if((item->id >= TIMER_ID_TEMPLATE_NIU_TRIGGER_1) && (item->id <= TIMER_ID_TEMPLATE_NIU_TRIGGER_50)) {
                ql_com_get_uptime(&curr_sec, &curr_usec);
                *remaining_sec = item->expire_sec - curr_sec;
                *remaining_usec = item->expire_usec - curr_usec;
                QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
                return item->id;
            }
        }
        QL_MUTEX_UNLOCK(&g_tmr_handler.mutex);
        return -1;
    }
}
