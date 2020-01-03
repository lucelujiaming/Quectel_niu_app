/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_cmdq.h
  @brief Command queue API (FIFO), all APIs are thread safe
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

#ifndef __QL_CMDQ_H__
#define __QL_CMDQ_H__
#include "com_list.h"
#include "ql_mutex.h"     


#define QL_CMDQ_MAX_LEN_DEFAULT 100


typedef struct ql_cmdq_handle_struct {
    ql_mutex_t mutex;
    com_list_head_t list;
    int fd_push;
    int fd_pull;
    int max_len;
} ql_cmdq_handle_t;

typedef struct {
    void * cmd;
    void * param;
    int  param_len;
    com_list_head_t next;
} ql_cmdq_item_t;


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Initializes the command queue
  @param[out] h Command queue handler 
  @param[in] max_len Max length of the command queue
  @return
  0 - succeed \n
  -1 - error ocurred
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_cmdq_init(ql_cmdq_handle_t *h, int max_len);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Push a command to the queue 
  @param[in] h Command queue handler 
  @param[in] cmd Command, user defined
  @param[in] param Parameter, user defined 
  @param[in] param_len Length of the parameter, user defined
  @return
  0 - succeed \n
  -1 - error ocurred
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_cmdq_cmd_push(ql_cmdq_handle_t *h, void *cmd, void *param, int param_len);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Pull a command from the queue(FIFO), maybe block
  @param[in] h Command queue handler 
  @param[in] timeout_ms Timeout in millisecond 
  @param[out] pcmd Storage for command item, must call ql_cmdq_item_free to release the item
  @return
  0 - timeout \n
  -1 - error ocurred
  1 - get a valid command item, parameter pcmd is valid
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_cmdq_pull(ql_cmdq_handle_t *h, int timeout_ms, ql_cmdq_item_t **pcmd);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get the file descriptors of the queue 
  @param[in] h Command queue handler 
  @return file descriptors of the queue
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_cmdq_get_fd(ql_cmdq_handle_t *h);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Rlease a command item 
  @param[in] h Command queue handler 
  @param[in] item Getting from ql_cmdq_pull 
  @return always 0 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_cmdq_item_free(ql_cmdq_handle_t *h, ql_cmdq_item_t *item);

#endif

#ifdef __cplusplus
} 
#endif

