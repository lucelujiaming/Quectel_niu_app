/*-----------------------------------------------------------------------------------------------*/
/**
  @file thread_main.c
  @brief Processing center
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
#include "ql_mutex.h"

#define SYS_HANDLER_MAX_LEN_DEFAULT 20


int g_tmr_cnt = 0;
void echo_tmr_proc(int tmr_id, void *param) 
{
    printf("tmr_cnt = %d\n", g_tmr_cnt);
    g_tmr_cnt++;
}

int g_cmd_cnt = 0;
void cmd1_proc(void *param)
{
    printf("cmd_cnt = %d\n", g_cmd_cnt);
    g_cmd_cnt++;
}

int main_thread(void)
{
    int i;
    int ret;
    ql_mutex_t mux1, mux2;

    ret = cmd_handler_init();
    if(ret != 0) {
        LOG_F("Failed to cmd_handler_init");
        return -1;
    }

    ret = tmr_handler_init();
    if(ret != 0) {
        LOG_F("Failed to tmr_handler_init");
        return -1;
    }

    QL_MEM_ALLOC(2000);
    QL_MEM_ALLOC(20);

    for(i=0; i<100; i++) {
        usleep(10000);
        printf("test cnt = %d\n", i);
        ret = tmr_handler_tmr_add(echo_tmr_proc, NULL, 2, 0);
        if(ret < 0) {
            printf("Failed to create tmr, i=%d\n", i);
            break;
        }
        ret = cmd_handler_push(cmd1_proc, NULL);
        if(ret != 0) {
            printf("Failed to create cmd, i=%d\n", i);
            break;
        }
    }

    ql_mutex_init(&mux1, "mux1");
    ql_mutex_init(&mux2, "mux2");

    QL_MUTEX_LOCK(&mux1);
    sleep(1);
    QL_MUTEX_LOCK(&mux2);
    sleep(2);
    ql_mutex_dump(stdout);

    QL_MUTEX_UNLOCK(&mux1);

    sleep(2);

    ql_mem_dump(stdout);
    ql_mutex_dump(stdout);

    while(1) {
        sleep(100);
    }

    return 0;
}

void sys_cmd_push(SYS_CMD_E cmd, void *data)
{

}
