/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_mutex.h
  @brief Simplified macro for mutex (support mutex tracking).
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

#ifndef __QL_MUTEX_H__
#define __QL_MUTEX_H__
#include <pthread.h>

#define QL_MUTEX_LOCK(mutex)   _ql_mutex_lock(__FUNCTION__, __LINE__, mutex)
#define QL_MUTEX_UNLOCK(mutex) _ql_mutex_unlock(__FUNCTION__, __LINE__, mutex)

typedef pthread_mutex_t ql_mutex_t;

int ql_mutex_init(ql_mutex_t *mutex, const char *name);
int ql_mutex_destroy(ql_mutex_t *mutex);

int _ql_mutex_lock(const char *func, int line, ql_mutex_t *mutex);
int _ql_mutex_unlock(const char *func, int line, ql_mutex_t *mutex);

int ql_mutex_dump(FILE *fp);

#endif

