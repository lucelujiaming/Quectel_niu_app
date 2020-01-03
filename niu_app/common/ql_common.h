/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_common.h
  @brief Common API
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

#ifndef _QL_COMMON_H_
#define _QL_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ql_oe.h>
#include <qlsyslog/ql_sys_log.h>
extern void upload_log_info(char *format, ...);
#define QL_COM_BUF_ZERO(buf)                memset(buf, 0, sizeof(buf))
#define QL_COM_STR_EMPTY(string)            (!string || !string[0])
#define QL_COM_STR_NCPY(src, dst)           strncpy(src, dst, sizeof(src))
#define QL_COM_ARRAY_SIZE(array)  (sizeof(array)/sizeof((array)[0]))

#define LOG_V(fmt, ...) QLOGV(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_D(fmt, ...) QLOGD(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);upload_log_info(fmt, ##__VA_ARGS__);
#define LOG_I(fmt, ...) QLOGI(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);upload_log_info(fmt, ##__VA_ARGS__);
#define LOG_W(fmt, ...) QLOGW(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);upload_log_info(fmt, ##__VA_ARGS__);
#define LOG_E(fmt, ...) QLOGE(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);upload_log_info(fmt, ##__VA_ARGS__);
#define LOG_F(fmt, ...) QLOGF(QL_LOG_TAG, "[%s  %d]: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);upload_log_info(fmt, ##__VA_ARGS__);


typedef struct {
    int value;
    const char *name;
} ql_value_name_item_t;


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Debug utils, get value by name 
  @param[in] vnlist Array of ql_value_name_item_t, must end by {-1, NULL}
  @param[in] value 
  @return name, or null_unknow
  */
/*-----------------------------------------------------------------------------------------------*/
const char *ql_v2n(ql_value_name_item_t *vnlist, int value);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Debug utils, get name by value
  @param[in] vnlist Array of ql_value_name_item_t, must end by {-1, NULL}
  @param[in] name
  @return value, or -1
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_n2v(ql_value_name_item_t *vnlist, const char *name);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get file size
  @param[in] file_path file path
  @return
  0 - succeed \n
  -1 - invalid param \n
  >0 - file size
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_file_get_size(const char *file_path);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Check wheather the file is exist
  @param[in] file_path file path
  @return
  0 - file is exist \n
  -1 - invalid param \n
  1 - file is not exist
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_file_exist(const char *file_path);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Synchronize the file's in-core state with storage device.(using shell cmd : fsync -d xxx)
  @param[in] file_path file path
  @return
  0 - succeed \n
  -1 - invalid param \n
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_file_sync(const char *file_path);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Check wheather the process is running
  @param[in] app_name Process name showed in PS 
  @return
  0 - the process is running \n
  1 - the process is not running \n
  -1 - error ocurred, check errno \n
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_app_exist(const char *app_name);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Execute a shell command, max command length is 256
  @param[in] fmt
  @param[in] others 
  @return same as system
  -1 - error ocurred, check errno \n
  other - status of the command
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_system(const char *fmt, ...);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get Current system time.(using gettimeofday)
  @return Current system time in millisecond
  */
/*-----------------------------------------------------------------------------------------------*/
uint64_t ql_com_get_tmr_ms(void);

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get system uptime, it does not include any time that the system is suspended
  @return uptime in sec, 0 is error
  */
/*-----------------------------------------------------------------------------------------------*/
uint64_t ql_com_get_uptime_ms(void);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get system uptime, it does not include any time that the system is suspended
  @param[out] psec Storage for second
  @param[out] pusec Storage for microsecond
  @return 
  0 - succeed \n
  -1 - error ocurred, check errno 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_get_uptime(uint32_t *psec, uint32_t *pusec);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get system uptime, it also includes any time that the system is suspended
  @param[out] psec Storage for second
  @param[out] pusec Storage for microsecond
  @return 
  0 - succeed \n
  -1 - error ocurred, check errno 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_get_uptime_all(uint32_t *psec, uint32_t *pusec);


/*-----------------------------------------------------------------------------------------------*/
/** 
  @brief Enable or disable nonblocking 
  @param[in] fd file descriptor, returned by socket() or open()
  @param[in] is_noblock 1-enable or 0-disable
  @return 
  0 - succeed
  -1 - error ocurred
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_fd_noblock(int fd, int is_noblock);


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Check wheather the file descriptor is ready for reading
  @param[in] fd file descriptor, returned by socket() or open()
  @return 
  0 - not ready
  1 - ready for reading 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_fd_has_data(int fd); 

#endif

#ifdef __cplusplus
} 
#endif

