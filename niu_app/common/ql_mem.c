/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_mem.h
  @brief Simplified macro for memory allocation and release (support memory track).
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

#include "ql_mem.h"
#include "ql_common.h"

#ifdef QL_DEBUG_MEM_TRACK
#include <pthread.h>
#include "uthash.h"
#endif



#ifdef QL_DEBUG_MEM_TRACK
typedef struct ql_mem_track_struct {
    void* ptr;
    const char * call_func;
    int  call_line;
    int  size;
    UT_hash_handle hh;
} ql_mem_track_t;

static ql_mem_track_t  *g_hash_mem = NULL;
static pthread_mutex_t g_mutex_mem = PTHREAD_MUTEX_INITIALIZER;

#endif

void * _ql_mem_alloc(const char *func, int line, int size)
{
    void *ptr;
    if(size <= 0) {
        return NULL;
    }

    ptr = malloc(size);
    if(!ptr) {
        LOG_F("Failed to malloc, size=%d", size);
        return NULL;
    }
#ifdef QL_DEBUG_MEM_TRACK
    do {
        ql_mem_track_t *item;
        pthread_mutex_lock(&g_mutex_mem);
        HASH_FIND_PTR(g_hash_mem, &ptr, item); 
        if(item != NULL)  {
            LOG_F("Unknow error ocurred, %p is used", ptr);
        }
        else
        {
            item = malloc(sizeof(*item));
            memset(item, 0, sizeof(*item));
            item->ptr = ptr;
            item->call_func = func;
            //strncpy(item->call_func, func, sizeof(item->call_func));
            item->call_line = line;
            item->size = size;
            HASH_ADD_PTR(g_hash_mem, ptr, item);
        }
        pthread_mutex_unlock(&g_mutex_mem);
    } while(0);
#endif

    return ptr;
}

void _ql_mem_free(const char *func, int line, void *ptr)
{
    if(!ptr) {
        return;
    }

#ifdef QL_DEBUG_MEM_TRACK
    do {
        pthread_mutex_lock(&g_mutex_mem);
        ql_mem_track_t *item;
        HASH_FIND_PTR(g_hash_mem, &ptr, item);
        if(item == NULL) {
            LOG_E("Error curred, free unknow memory"); 
        }
        else {
            HASH_DEL(g_hash_mem, item);
            free(item);
        }

        pthread_mutex_unlock(&g_mutex_mem);
    } while(0);
#endif
    
    free(ptr);
}

void ql_mem_dump(FILE *fp) 
{
#ifndef QL_DEBUG_MEM_TRACK
    fprintf(fp, "mem tracking is not supported\n");
#else
    do {
        ql_mem_track_t *item;
        int cnt = 1;
        fprintf(fp, "%-6s %-10s %-9s   %s\n", "idx", "mem", "size", "func");
        for(item=g_hash_mem; item != NULL; item=(ql_mem_track_t*)(item->hh.next)) {
            fprintf(fp, "%-6d %-10p %-9d    %s:%d\n", cnt, item->ptr, item->size, item->call_func, item->call_line);
            cnt++;
        } 
    } while(0);
#endif
}

 
