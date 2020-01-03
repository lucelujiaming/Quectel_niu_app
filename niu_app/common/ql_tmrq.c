/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_tmrq.c
  @brief Implementation of timer queue API (FIFO)
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
#include "ql_tmrq.h"
#include "ql_mem.h"


static int _tmr_add_curr(uint32_t *psec, uint32_t *pusec, uint32_t time_ms)
{
    uint32_t sec, usec;
    if(ql_com_get_uptime(&sec, &usec) != 0) {
        return -1;
    }
    
    usec += time_ms%1000 * 1000;
    sec += time_ms/1000 + usec/1000000;
    usec = usec%1000000;
    if(psec) {
        psec[0] = sec;
    }

    if(pusec) {
        pusec[0] = usec;
    }

    return 0;
}


int _tmr_list_insert(ql_tmrq_handle_t *h, ql_tmrq_item_t *item)
{
    int ret = 0;
    ql_tmrq_item_t *pos, *pre;
    uint8_t note = 1;

    pre = NULL;
    com_list_for_each_entry(pos, &h->list, next) {
        if((pos->expire_sec<item->expire_sec) 
                || (pos->expire_sec==item->expire_sec && pos->expire_usec<=item->expire_usec)) {
            pre = pos;
            continue;
        }
        break;
    }
    if(pre==NULL) {
        LOG_V("add head");
        com_list_add(&item->next, &h->list); 
    }
    else {
        LOG_V("add tail");
        com_list_add(&item->next, &pre->next);
    }

    ret = write(h->fd_push, &note, sizeof(note));
    if(ret != sizeof(note)) {
        LOG_F("Failed to write, ret=%s, err=%s", ret, strerror(errno));
        return -1;
    }

    return 0;
}


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
int ql_tmrq_init(ql_tmrq_handle_t *h, int max_len)
{
    int ret = 0;
    int fd[2] = {-1, -1};
    uint32_t sec = 0;
    uint32_t usec = 0;

    if(!h) {
        return -1;
    }

    if(ql_com_get_uptime(&sec, &usec)) {
        LOG_F("Failed to get current time");
        return -1;
    }

    //printf("curr boot time : sec=%d, usec=%d\n", sec, usec);

    memset(h, 0, sizeof(*h));

    COM_INIT_LIST_HEAD(&h->list);
    
    ret = ql_mutex_init(&h->mutex, "tmrq");
    if(ret < 0) {
        LOG_E("Faield to ql_mutex_init, err=%s", strerror(errno));
        return -1;
    }

    if(max_len <= 0) {
        h->max_len = QL_TMRQ_MAX_LEN_DEFAULT;
    }
    else {
        h->max_len = max_len;
    }

    socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    h->fd_push = fd[0];
    h->fd_pull = fd[1];

    ql_com_fd_noblock(h->fd_push, 1);
    ql_com_fd_noblock(h->fd_pull, 1);

    return 0;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Add a timer to the queue 

  @param[in] tmr_id, timer id
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
        uint32_t interval_ms, int is_cycle)
{
    int ret = 0;
    ql_tmrq_item_t *item;

    LOG_V("enter");

    if(!h) {
        LOG_E("Invlaid param");
        return -1;
    }

    if(ql_tmrq_tmr_is_exist(h, tmr_id))
    {
        //LOG_E("tmr_id %d already exist", tmr_id);
        return -1;
    }

    QL_MUTEX_LOCK(&h->mutex);
    do {
        if(!com_list_empty(&h->list) && com_list_size(&h->list) >= h->max_len) {
            LOG_E("Error ocurred, Message list is full[len:%d]", com_list_size(&h->list));
            ret = -1;
            break;
        }
        
        item = QL_MEM_ALLOC(sizeof(*item));
        if(!item) {
            LOG_F("Failed to alloc ql_tmrq_item_t");
            ret = -1;
            break;
        }

        memset(item, 0, sizeof(*item));
        item->cmd = cmd;
        item->param = param;
        item->param_len = param_len;
        // h->tmr_index++;
        // if(h->tmr_index<=0) {
        //     h->tmr_index = 1;
        // }
        // item->id = h->tmr_index;
        item->id = tmr_id;
        item->interval_ms = interval_ms;
        item->is_cycle = is_cycle;
        ret = item->id;

        _tmr_add_curr(&item->expire_sec, &item->expire_usec, interval_ms);

        ret = _tmr_list_insert(h, item);

        if(ret != 0) {
            LOG_E("Failed to push a message");
            ret = -1;
        }
        else {
            ret = item->id;
        }
    } while(0);

    QL_MUTEX_UNLOCK(&h->mutex);

    LOG_V("exit, ret=%d");

    return ret;
}


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
int ql_tmrq_tmr_is_exist(ql_tmrq_handle_t *h, int tmr_id)
{
    ql_tmrq_item_t *pos = NULL;
    int ret = 0;

    LOG_V("enter");

    if(!h) {
        LOG_E("Invalid param");
        return 0;
    }

    QL_MUTEX_LOCK(&h->mutex);
    com_list_for_each_entry(pos, &h->list, next) {
        if(pos->id == tmr_id) {
            ret = 1;
            break;
        }
    }
    QL_MUTEX_UNLOCK(&h->mutex);

    LOG_V("exit, ret=%d", ret);

    return ret;
}


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
int ql_tmrq_tmr_del(ql_tmrq_handle_t *h, int tmr_id)
{
    ql_tmrq_item_t *pos = NULL;
    int ret = 0;

    LOG_V("enter");

    if(!h) {
        LOG_E("Invalid param");
        return -1;
    }

    QL_MUTEX_LOCK(&h->mutex);
    com_list_for_each_entry(pos, &h->list, next) {
        if(pos->id == tmr_id) {
            if(NULL != pos->param) {
                QL_MEM_FREE(pos->param);
                pos->param = NULL;
            }
            com_list_del(&pos->next);
            QL_MEM_FREE(pos);
            break;
        }
    }
    QL_MUTEX_UNLOCK(&h->mutex);

    LOG_V("exit, ret=%d", ret);

    return ret;
}


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
int ql_tmrq_tmr_set(ql_tmrq_handle_t *h, int tmr_id, void *cmd, void *param, 
        int param_len, uint32_t interval_ms, int is_cycle)
{
    ql_tmrq_item_t *item = NULL;
    ql_tmrq_item_t *pos = NULL;
    int ret = 0;

    LOG_V("enter");

    if(!h) {
        LOG_E("Invalid param");
        return -1;
    }

    QL_MUTEX_LOCK(&h->mutex);

    com_list_for_each_entry(pos, &h->list, next) {
        if(pos->id == tmr_id) {
            item = pos;
            break;
        }
    }

    if(item != NULL) {
        item->cmd = cmd;
        item->param = param;
        item->param_len = param_len;
        item->interval_ms = interval_ms;
        _tmr_add_curr(&item->expire_sec, &item->expire_usec, interval_ms);
        item->is_cycle = is_cycle;
        com_list_del(&item->next);
        ret = _tmr_list_insert(h, item);
    }
    else {
        ret = 1;
    }
    
    QL_MUTEX_UNLOCK(&h->mutex);

    LOG_V("exit, ret=%d", ret);

    return ret;
}


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
int ql_tmrq_pull(ql_tmrq_handle_t *h, ql_tmrq_item_t **pitem)
{
    int ret = 0;
    fd_set rfds; 
    struct timeval tm;

    uint8_t  note_buf[16];
    uint32_t sleep_sec = 0;
    uint32_t sleep_usec = 0;

    uint32_t curr_sec = 0;
    uint32_t curr_usec = 0;

    ql_tmrq_item_t *exp_item = NULL;
    ql_tmrq_item_t *item = NULL;

    if(!h || !pitem) {
        LOG_E("Invvalid param");
        return -1;
    }

    pitem[0] = NULL;

    QL_MUTEX_LOCK(&h->mutex);
    if(com_list_empty(&h->list)) {
        sleep_sec = 2; // default sleep 2s
        sleep_usec = 0;
    }
    else {
        ql_com_get_uptime(&curr_sec, &curr_usec);
        item = com_list_first_entry(&h->list, ql_tmrq_item_t, next);

        if(curr_sec>item->expire_sec || (curr_sec==item->expire_sec && curr_usec>=item->expire_usec)) {
            exp_item = item;
            com_list_del(&item->next);
            if(item->is_cycle) {
                _tmr_add_curr(&item->expire_sec, &item->expire_usec, item->interval_ms);
                _tmr_list_insert(h, item);
            }
        }
        else {
            if(item->expire_usec >= curr_usec) {
                sleep_usec = item->expire_usec - curr_usec;
                sleep_sec = item->expire_sec - curr_sec;
            }
            else {
                sleep_usec = item->expire_usec+1000000 - curr_usec;
                sleep_sec = item->expire_sec - curr_sec - 1;
            }
        }
    }

    QL_MUTEX_UNLOCK(&h->mutex);
    if(exp_item != NULL)  {
        pitem[0] = exp_item;
        LOG_V("get valid item");
        return 1;
    }

    FD_ZERO(&rfds);
    FD_SET(h->fd_pull, &rfds);
    
    tm.tv_sec = sleep_sec;
    tm.tv_usec = sleep_usec;

    ret = select(h->fd_pull+1, &rfds, NULL, NULL, &tm);
    if(ret < 0 && errno!=EINTR)  {
        LOG_F("Failed to select, err=%s", strerror(errno));
        return -1;
    }

    if(ret>0) {
        read(h->fd_pull, note_buf, sizeof(note_buf));
    }
     
    return 0;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Rlease a timer item 
  @param[in] item Getting from ql_tmrq_pull 
  @return always 0 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_tmrq_item_free(ql_tmrq_handle_t *h, ql_tmrq_item_t *item)
{
    if(!h || !item) {
        return 0;
    }

    if(!item->is_cycle) {
        QL_MEM_FREE(item);
    }
    return 0;
}
