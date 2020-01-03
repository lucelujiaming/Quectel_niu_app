/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_tmrq.h
  @brief Timer queue, all APIs are thread safe
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

#ifdef __cplusplus
 extern "C" {
#endif


#ifndef __QL_TMRQ_H__
#define __QL_TMRQ_H__

#include <stdint.h>
#include "com_list.h"
#include "ql_mutex.h"


#define QL_TMRQ_MAX_LEN_DEFAULT 1024


typedef struct ql_tmrq_handle_struct {
    ql_mutex_t mutex;
    com_list_head_t list;
    int fd_push;
    int fd_pull;
    //int tmr_index;
    int max_len;
} ql_tmrq_handle_t;

typedef struct ql_tmrq_item_struct {
    void *cmd;
    void *param;
    int  param_len;
    int  id;
    uint32_t expire_sec;
    uint32_t expire_usec;
    uint32_t interval_ms;
    int is_cycle;
    com_list_head_t next;
} ql_tmrq_item_t;



/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Initializes the timer queue
  @param[out] h Timer queue handler 
  @param[in] max_len Max length of the timer queue, 0 meas using default (100)
  @return
  0 - succeed \n
  -1 - error ocurred
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_tmrq_init(ql_tmrq_handle_t *h, int max_len);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Add a timer to the queue 
  @param[in] tmr_id time id
  @param[in] h Timer queue handler 
  @param[in] cmd Command, user defined
  @param[in] param Parameter, user defined 
  @param[in] param_len Length of the parameter, user defined
  @param[in] interval_ms Timer timeout in millisecond
  @param[in] is_cycle Whether the timer is executed cyclically
  @return
  -1 - error ocurred \n
  other - timer id, range 1-2147483647(0x7FFFFFFF)
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_tmrq_tmr_add(int tmr_id, ql_tmrq_handle_t *h, void *cmd, void *param, int param_len, 
        uint32_t interval_ms, int is_cycle);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Check whether the timer is exist 
  @param[in] h Timer queue handler 
  @param[in] tmr_id Timer id 
  @return
  1 - timer is exist \n
  0 - timer is not exist 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_tmrq_tmr_is_exist(ql_tmrq_handle_t *h, int tmr_id);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Delete the timer 
  @param[in] h Timer queue handler 
  @param[in] tmr_id Timer id 
  @return
  0 - succeed \n
  -1 - error ocurred 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_tmrq_tmr_del(ql_tmrq_handle_t *h, int tmr_id);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Reset the timer 
  @param[in] h Timer queue handler 
  @param[in] tmr_id Timer id 
  @param[in] cmd Command, user defined
  @param[in] param Parameter, user defined 
  @param[in] param_len Length of the parameter, user defined
  @param[in] interval_ms Timer timeout in millisecond
  @param[in] is_cycle Whether the timer is executed cyclically
  @return
  0 - succeed \n
  -1 - error ocurred 
  1 - timer is not exist
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_tmrq_tmr_set(ql_tmrq_handle_t *h, int tmr_id, void *cmd, void *param, int param_len, uint32_t interval_ms, int is_cycle);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Pull a timer item from the queue, maybe block
  @param[in] h Timer queue handler 
  @param[out] pitem Storage for timer item, must call ql_tmrq_item_free to relase the item
  @return
  0 - timeout \n
  -1 - error ocurred
  1 - get a valid command item, parameter pitem is valid
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_tmrq_pull(ql_tmrq_handle_t *h, ql_tmrq_item_t **pitem);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Rlease a timer item 
  @param[in] item Getting from ql_tmrq_pull 
  @return always 0 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_tmrq_item_free(ql_tmrq_handle_t *h, ql_tmrq_item_t *item);

#endif

#ifdef __cplusplus
} 
#endif

