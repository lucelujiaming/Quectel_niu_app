/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_mutex.c
  @brief Implementation of mutex (support mutex tracking).
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
#include "ql_common.h"
#include "com_list.h"
#include "ql_mutex.h"

#ifdef QL_DEBUG_MUTEX_TRACK
typedef struct ql_mutex_track_struct {
    char name[32];
    ql_mutex_t *mutex;
    uint8_t  is_lock;
    const char *lock_func;
    int lock_line;
    uint32_t lock_sec;
    uint32_t lock_usec;
    com_list_head_t next;
} ql_mutex_track_t;

static pthread_mutex_t g_mutex_mutex = PTHREAD_MUTEX_INITIALIZER;
static com_list_head_t g_list_mutex = COM_LIST_HEAD_INIT(g_list_mutex);

#endif

int ql_mutex_init(ql_mutex_t *mutex, const char *name)
{
    int ret = 0;
    ret = pthread_mutex_init(mutex, NULL);
    if(ret != 0) {
        return ret;
    }
#ifdef QL_DEBUG_MUTEX_TRACK
    pthread_mutex_lock(&g_mutex_mutex);
    do {
        ql_mutex_track_t *item;
        item = malloc(sizeof(*item)); 
        if(!item) {
            break;
        }
        memset(item, 0, sizeof(*item));
        item->mutex = mutex;
        if(name && name[0]) {
            strncpy(item->name, name, sizeof(item->name));
        }
        else {
            snprintf(item->name, sizeof(item->name), "%p", mutex);
        }
        com_list_add(&item->next, &g_list_mutex);
    } while(0); 
    pthread_mutex_unlock(&g_mutex_mutex);
#endif

    return ret;
}

int ql_mutex_destroy(ql_mutex_t *mutex)
{
    int ret = 0;
    ret = pthread_mutex_destroy(mutex);
    if(ret != 0) {
        return ret;
    }

#ifdef QL_DEBUG_MUTEX_TRACK
    pthread_mutex_lock(&g_mutex_mutex);
    do {
        ql_mutex_track_t *pos;
        com_list_for_each_entry(pos, &g_list_mutex, next) {
            if(pos->mutex == mutex) {
                com_list_del(&pos->next);
                free(pos);
                break;
            }
        }
    } while(0);
    pthread_mutex_unlock(&g_mutex_mutex);
#endif

    return ret;
}

int _ql_mutex_lock(const char *func, int line, ql_mutex_t *mutex)
{
    int ret = 0; 

    ret = pthread_mutex_lock(mutex);
    if(ret != 0) {
        return ret;
    }

#ifdef QL_DEBUG_MUTEX_TRACK
    pthread_mutex_lock(&g_mutex_mutex);
    do {
        ql_mutex_track_t *pos;
        com_list_for_each_entry(pos, &g_list_mutex, next) {
            if(pos->mutex == mutex) {
                pos->is_lock = 1;
                pos->lock_func = func;
                pos->lock_line = line;
                ql_com_get_uptime(&pos->lock_sec, &pos->lock_usec);
                break;
            }
        }
    } while(0);
    pthread_mutex_unlock(&g_mutex_mutex);
#endif


    return ret;
}

int _ql_mutex_unlock(const char *func, int line, ql_mutex_t *mutex)
{
    int ret = 0;
    ret = pthread_mutex_unlock(mutex);
    if(ret != 0) {
        return ret;
    }

#ifdef QL_DEBUG_MUTEX_TRACK
    pthread_mutex_lock(&g_mutex_mutex);
    do {
        ql_mutex_track_t *pos;
        com_list_for_each_entry(pos, &g_list_mutex, next) {
            if(pos->mutex == mutex) {
                pos->is_lock = 0;
                pos->lock_func = func;
                pos->lock_line = line;
                ql_com_get_uptime(&pos->lock_sec, &pos->lock_usec);
                break;
            }
        }
    } while(0);
    pthread_mutex_unlock(&g_mutex_mutex);
#endif

    return ret;
}

int ql_mutex_dump(FILE *fp)
{

#ifndef QL_DEBUG_MUTEX_TRACK
    fprintf(fp, "mutex tracking is not supported\n");
#else
    pthread_mutex_lock(&g_mutex_mutex);
    do {
        ql_mutex_track_t *pos;
        uint32_t sec, lease_sec;
        uint32_t usec, lease_usec;
        ql_com_get_uptime(&sec, &usec);
        fprintf(fp, "%-32s %-6s %9s    %s\n",
                "mutex", "lock", "lease", "info");
        com_list_for_each_entry(pos, &g_list_mutex, next) {
            if(usec >= pos->lock_usec) {
                lease_sec = sec - pos->lock_sec;
                lease_usec = usec - pos->lock_usec;
            }
            else {
                lease_usec = usec + 1000000 - pos->lock_usec;
                lease_sec = sec - pos->lock_sec - 1;
            }

            fprintf(fp, "%-32s %-6s %6d.%03d   %s:%d\n",
                    pos->name, 
                    pos->is_lock?"YES":"NO",
                    lease_sec, lease_usec/1000,
                    pos->lock_func, pos->lock_line);
        }
    } while(0);
    pthread_mutex_unlock(&g_mutex_mutex);
#endif

    return 0;
}

