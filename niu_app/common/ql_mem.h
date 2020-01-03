/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_mem.h
  @brief Simplified macro for memory allocation and release (support memory tracking).
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

#ifndef __QL_MEM_H__
#define __QL_MEM_H__
#include <stdio.h>


#define QL_MEM_ALLOC(size) _ql_mem_alloc(__FUNCTION__, __LINE__, size)
#define QL_MEM_FREE(ptr) _ql_mem_free(__FUNCTION__, __LINE__, (void*)(ptr))


void * _ql_mem_alloc(const char *func, int line, int size);
void _ql_mem_free(const char *func, int line, void *ptr);
void ql_mem_dump(FILE *fp); 


#endif

