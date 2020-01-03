/*-----------------------------------------------------------------------------------------------*/
/**
  @file thread_cmd.c
  @brief Implementation of framework command queue 
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
#include "ql_cmdq.h"

#define CMD_HANDLER_MAX_LEN_DEFAULT 100


static pthread_t g_thread_cmd;
static ql_cmdq_handle_t g_cmd_handler;
static volatile int g_is_peeding = 0;
static volatile uint64_t g_tmr_ms_peeding_start = 0;

void * thread_cmd_handler_proc(void * param)
{
    int ret;
    ql_cmdq_item_t *item;

    LOG_V("start, enter main");
    while(1) {
        ret = ql_cmdq_pull(&g_cmd_handler, 10000, &item);
        if(ret<0) {
            LOG_F("Failed to thread_cmd_handler_proc, ret=%d", ret);
            break;
        }

        if(item==NULL) {
            continue;
        }

        if(item->cmd) {
            g_is_peeding = 1;
            LOG_V("start to handler cmd %p", item->cmd);
            g_tmr_ms_peeding_start = ql_com_get_uptime_ms(); 
            ((cmd_handler_func_f)item->cmd)(item->param);
            LOG_V("end to handler cmd %p, time lease = %d ms", item->cmd, ql_com_get_uptime_ms() - g_tmr_ms_peeding_start);
            g_is_peeding = 0;
        }
        ql_cmdq_item_free(&g_cmd_handler, item);
    }

    return 0;
}

int cmd_handler_init(void)
{
    int ret;

    ret = ql_cmdq_init(&g_cmd_handler, CMD_HANDLER_MAX_LEN_DEFAULT);
    if(ret != 0) {
        LOG_F("Failed to ql_cmdq_init, ret=%d", ret);
        return -1;
    }

    ret = pthread_create(&g_thread_cmd, NULL, thread_cmd_handler_proc, NULL);
    if(ret != 0) {
        LOG_F("Failed to pthread_create, err=%s", strerror(errno));
        return -1;
    }

    return 0;
}

int cmd_handler_push(cmd_handler_func_f func, void *param)
{
    return ql_cmdq_cmd_push(&g_cmd_handler, (void*)func, param, 0);
}

int cmd_handler_peeding_time_ms(void)
{
   if(g_is_peeding) {
        return ql_com_get_uptime_ms() - g_tmr_ms_peeding_start;
   }

   return 0;
}
