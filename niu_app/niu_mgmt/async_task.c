/*-----------------------------------------------------------------------------------------------*/
/**
  @file async_task.c
  @brief asynchronous task simulation 
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

#include <stdlib.h>
#include <stdarg.h>
#include "service.h"
#include "sys_core.h"

typedef struct {
    async_task_t *task;
    int progress;
    void *param;
} async_task_progress_t;

typedef struct {
    async_task_t *task;
    int result;
    void *param;
} async_task_result_t;


static void cmd_async_handler(void *param)
{
    async_task_t *task = param;
    task->on_handler(task, task->param);
}

static void * thread_async_proc(void *param)
{
    async_task_t *task = param;
    pthread_detach(pthread_self());
    task->on_handler(task, task->param);
    return NULL;
}

void async_task_handler(SYS_EVENT_E evt, void *param, int len)
{
    if(!param) {
        return;
    }
    if(evt == SYS_EVENT_ASYNC_ON_PROGRESS) {
        async_task_progress_t *p = param;
        if(p->task->on_progress) {
            p->task->on_progress(p->task, p->progress, p->param);
        }
        QL_MEM_FREE(p);

    }
    else if(evt == SYS_EVENT_ASYNC_ON_RESULT) {
        async_task_result_t *p = param;
        if(p->task->on_result) {
            p->task->on_result(p->task, p->result, p->param);
        }

        /** release task */
        QL_MEM_FREE(p->task);
        QL_MEM_FREE(p);
    }
}

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Push task current progress, on_progress will be called in main thread 
  @param[in] task 
  @param[in] progress User defined 
  @param[in] param Extended paramter 
  @return 0-succeed, other-failed
  */
/*-----------------------------------------------------------------------------------------------*/
int async_task_progress_push(async_task_t *task, int progress, void *param)
{
    async_task_progress_t *p;
    p = QL_MEM_ALLOC(sizeof(*p));
    if(p == NULL) {
        LOG_F("Failed to maloc");
        sys_abort(-1, "Failed to maloc");
        return -1;
    }

    p->task = task;
    p->progress = progress;
    p->param = param;
    sys_event_push(SYS_EVENT_ASYNC_ON_PROGRESS, p, sizeof(*p));

    return 0;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Push task result, on_result will be called in main thread 
  @param[in] task 
  @param[in] result User defined 
  @param[in] param Extended paramter 
  @return 0-succeed, other-failed
  */
/*-----------------------------------------------------------------------------------------------*/
int async_task_result_push(async_task_t *task, int result, void *param)
{
    async_task_result_t *p;
    p = QL_MEM_ALLOC(sizeof(*p));
    if(p == NULL) {
        LOG_F("Failed to maloc");
        sys_abort(-1, "Failed to maloc");
        return -1;
    }

    p->task = task;
    p->result = result;
    p->param = param;
    sys_event_push(SYS_EVENT_ASYNC_ON_RESULT, p, sizeof(*p));
    return 0;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Start a asynchronous task, on_progress and on_result will be called in main thread 
  @param[in] type Task mode
  @param[in] on_handler Hanlder
  @param[in] on_progress Rate of progress callback
  @param[in] on_result Resule callback
  @param[in] param Parameter invoked by on_handler
  @return 0-succeed, other-failed
  */
/*-----------------------------------------------------------------------------------------------*/
int async_task_start(ASYNC_TASK_TYPE_E type, 
        async_task_handler_f on_handler, 
        async_task_progress_f on_progress, 
        async_task_result_f on_result, 
        void *param)
{
    int ret = 0;
    pthread_t th;
    async_task_t *task = QL_MEM_ALLOC(sizeof(*task));

    memset(task, 0, sizeof(*task));
    task->type = type;
    task->on_handler = on_handler;
    task->on_progress = on_progress;
    task->on_result = on_result;
    task->param = param;
    if( type==ASYNC_TASK_TYPE_CMD ) {
        cmd_handler_push(cmd_async_handler, task);
    }
    else {
        ret = pthread_create(&th, NULL, thread_async_proc, task);
        if(ret != 0) {
            LOG_F("Failed to pthread_create, err=%s", strerror(errno));
            sys_abort(-1, "Failed to pthread_create");
            return -1;
        }
    }

    return 0;
}

