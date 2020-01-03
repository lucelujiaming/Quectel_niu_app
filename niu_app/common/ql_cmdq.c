/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_cmdq.c
  @brief Implementation of command queue API (FIFO)
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
#include "ql_cmdq.h"
#include "ql_mem.h"


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
int ql_cmdq_init(ql_cmdq_handle_t *h, int max_len)
{
    int ret;
    int fd[2] = {-1, -1};

    LOG_V("enter");
    
    if(!h) {
        LOG_E("Invalid param");
        return -1;
    }

    memset(h, 0, sizeof(*h));
    COM_INIT_LIST_HEAD(&h->list);

    ret = ql_mutex_init(&h->mutex, "cmdq");
    if(ret < 0) {
        LOG_E("Faield to ql_mutex_init, err=%s", strerror(errno));
        return -1;
    }

    if(max_len <= 0) {
        h->max_len = QL_CMDQ_MAX_LEN_DEFAULT;
    }
    else {
        h->max_len = QL_CMDQ_MAX_LEN_DEFAULT;
    }

    socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    h->fd_push = fd[0];
    h->fd_pull = fd[1];

    ql_com_fd_noblock(h->fd_push, 1);
    ql_com_fd_noblock(h->fd_pull, 1);

    LOG_V("exit");

    return 0;
}


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
int ql_cmdq_cmd_push(ql_cmdq_handle_t *h, void *cmd, void *param, int param_len)
{
    int ret = 0;
    ql_cmdq_item_t *item;
    uint8_t note = 1;

    LOG_V("enter");

    if(!h) {
        LOG_E("Invalid param");
        return -1;
    }

    QL_MUTEX_LOCK(&h->mutex);
    do {
        if(!com_list_empty(&h->list) && com_list_size(&h->list) >= h->max_len) {
            LOG_E("Error ocurred, Message list is full %p", cmd);
            ret = 1;
            break;
        }

        item = QL_MEM_ALLOC(sizeof(*item)); 
        if(!item) {
            LOG_F("Failed to alloc ql_cmdq_item_t");
            ret = -1;
            break;
        }

        memset(item, 0, sizeof(*item));

        item->cmd = cmd;
        item->param = param;
        item->param_len = param_len;

        ret = write(h->fd_push, &note, sizeof(note));
        com_list_add_tail(&item->next, &h->list);

        if(ret != sizeof(note)) {
            LOG_E("Failed to push a message, err=%s", strerror(errno));
            ret = -1;
        }
        else {
            ret = 0;
        }
    } while(0);
    QL_MUTEX_UNLOCK(&h->mutex);

    LOG_V("exit, ret=%d", ret);

    return ret;
}


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
int ql_cmdq_pull(ql_cmdq_handle_t *h, int timeout_ms, ql_cmdq_item_t **pcmd)
{
    int ret = 0;
    fd_set rfds; 
    ql_cmdq_item_t *item;
    struct timeval tm;
    uint8_t note;

    if(!h || !pcmd) {
        LOG_F("Invalid param");
        return -1;
    }

    pcmd[0] = NULL;

    tm.tv_sec  = timeout_ms/1000;
    tm.tv_usec = (timeout_ms%1000) * 1000;

    FD_ZERO(&rfds);
    FD_SET(h->fd_pull, &rfds);

    ret = select(h->fd_pull+1, &rfds, NULL, NULL, &tm);
    if(ret<0) { // error ocurred
        if(errno != EINTR) {
            LOG_E("Failed to select, error=%s", strerror(errno));
            return -1;
        }
        else {
            LOG_V("need retry");
            return 0;   // need retry
        }
    }
    else if(ret == 0) { // timeout
        return 0;
    }

    ret = read(h->fd_pull, &note, sizeof(note));
    if(ret != sizeof(note)) {
        LOG_E("Failed to read, ret=%d, err=%s", ret, strerror(errno));
        return -1;
    }
    
    QL_MUTEX_LOCK(&h->mutex);
    do {
        if(com_list_empty(&h->list)) {
            ret = 0;
            break;
        }
        item = com_list_first_entry(&h->list, ql_cmdq_item_t, next);
        com_list_del(&item->next);
        pcmd[0] = item;
        ret = 1;
    } while(0);
    QL_MUTEX_UNLOCK(&h->mutex);

    return ret;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get the file descriptors of the queue 
  @param[in] h Command queue handler 
  @return file descriptors of the queue
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_cmdq_get_fd(ql_cmdq_handle_t *h)
{
    return h->fd_pull;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Rlease a command item 
  @param[in] item Getting from ql_cmdq_pull 
  @return always 0 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_cmdq_item_free(ql_cmdq_handle_t *h, ql_cmdq_item_t *item)
{
    if(!h || !item) {
        return 0;
    }
    QL_MEM_FREE(item);
    return 0;
}

