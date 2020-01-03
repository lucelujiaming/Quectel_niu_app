#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include "ql_common.h"



/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Debug utils, get value by name 
  @param[in] vnlist Array of ql_value_name_item_t, must end by {-1, NULL}
  @param[in] value 
  @return name, or null_unknow
  */
/*-----------------------------------------------------------------------------------------------*/
const char *ql_v2n(ql_value_name_item_t *vnlist, int value)
{
    int i = 0;
    for(i=0; ; i++) {
        if(vnlist[i].name==NULL) {
            break;
        }
        if(vnlist[i].value == value) {
            return vnlist[i].name;
        }
    }

    return "null_unknow";
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Debug utils, get name by value
  @param[in] vnlist Array of ql_value_name_item_t, must end by {-1, NULL}
  @param[in] name
  @return value
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_n2v(ql_value_name_item_t *vnlist, const char *name)
{
    int i = 0;

    if(!name || !name[0]) {
        return -1;
    }

    for(i=0; ; i++) {
        if(vnlist[i].name==NULL) {
            break;
        }

        if(!strcmp(vnlist[i].name, name)) {
            return vnlist[i].value;
        }
    }

    return -1;
}


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
int ql_com_file_get_size(const char *file_path)
{
    int file_size = -1;
    struct stat statbuff;  

    if(QL_COM_STR_EMPTY(file_path)) {
        return -1;
    }

    if(stat(file_path, &statbuff) < 0){  
        return file_size;  
    }else{  
        file_size = (int)statbuff.st_size;  
    }  

    return file_size; 
}


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
int ql_com_file_exist(const char *file_path)
{
    if(QL_COM_STR_EMPTY(file_path)) {
        return -1;
    }

    if(!access(file_path, F_OK)) {
        return 0;
    }

    return 1;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Synchronize the file's in-core state with storage device.(using shell cmd : fsync -d xxx)
  @param[in] file_path file path
  @return
  0 - succeed \n
  -1 - invalid param \n
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_file_sync(const char *file_path)
{
    char cmd[256];
    if(QL_COM_STR_EMPTY(file_path)) {
        return -1;
    }
    snprintf(cmd, sizeof(cmd), "fsync -d %s", file_path);
    ql_com_system(cmd);
    return 0;
}


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
int ql_com_app_exist(const char *app_name)
{
    FILE *fp;
    char buf[256];

    if(QL_COM_STR_EMPTY(app_name)) {
        return -1;
    }

    snprintf(buf, sizeof(buf), "pidof %s", app_name); 
    fp = popen(buf, "r");
    if(!fp) {
        return -1;
    }

    QL_COM_BUF_ZERO(buf);
    fgets(buf, sizeof(buf), fp);
    pclose(fp);

    if(buf[0]<'0' || buf[0]>'9') {
        return 1;
    }

    return 0;
}


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
int ql_com_system(const char *fmt, ...)
{
    va_list ap;
    char buf[256];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return system(buf);
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get Current system time.(using gettimeofday)
  @return Current system time in millisecond
  */
/*-----------------------------------------------------------------------------------------------*/
uint64_t ql_com_get_tmr_ms(void)
{
    uint64_t tmr = 0;
    struct timeval tv;
    gettimeofday(&tv,NULL);

    tmr = tv.tv_sec * 1000ull + tv.tv_usec/1000;

    return tmr;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get system uptime, it does not include any time that the system is suspended
  @return uptime in sec, 0 is error
  */
/*-----------------------------------------------------------------------------------------------*/
uint64_t ql_com_get_uptime_ms(void)
{
    uint64_t tmr = 0;
    uint32_t sec = 0;
    uint32_t usec = 0;
    
    if(ql_com_get_uptime(&sec, &usec) != 0) {
        return 0;
    }
        
    tmr = sec * 1000ull + usec/1000;
    return tmr;
}


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
int ql_com_get_uptime(uint32_t *psec, uint32_t *pusec)
{
    struct timespec res;   
    int ret = 0;
    //ret = clock_gettime(CLOCK_MONOTONIC, &res);  //CLOCK_BOOTTIME
    ret = clock_gettime(CLOCK_BOOTTIME, &res);
    if(ret != 0) {
        return -1;
    }
    
    if(psec) {
        psec[0] = res.tv_sec;
    }

    if(pusec) {
        pusec[0] = res.tv_nsec/1000;
    }

    return 0;
}


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
int ql_com_get_uptime_all(uint32_t *psec, uint32_t *pusec)
{
    struct timespec res;   
    int ret = 0;
    ret = clock_gettime(CLOCK_BOOTTIME, &res);
    if(ret != 0) {
        return -1;
    }
    
    if(psec) {
        psec[0] = res.tv_sec;
    }

    if(pusec) {
        pusec[0] = res.tv_nsec/1000;
    }

    return 0;
}


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
int ql_com_fd_noblock(int fd, int is_noblock)
{
    int flag = 0;
    if(fd<0) {
        return -1;
    }

    flag = fcntl(fd, F_GETFL, 0);
    if(is_noblock) {
        flag |= O_NONBLOCK;
    }
    else {
        flag &= ~O_NONBLOCK;
    }

    fcntl(fd, F_SETFL, flag); 

    return 0;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Check wheather the file descriptor is ready for reading
  @param[in] fd file descriptor, returned by socket() or open()
  @return 
  0 - not ready
  1 - ready for reading 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_com_fd_has_data(int fd) 
{
    int ret;
    fd_set rfds;
    struct timeval tm;

    tm.tv_sec  = 0;
    tm.tv_usec = 0;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    ret = select(fd+1, &rfds, NULL, NULL, &tm);
    if(ret > 0)
    {
        return 1;
    }
    return 0;
}


/*-------------------------------------------------------------------------------------------------
  @brief Get rss of current process (page count, 1 page = 4K)
  @param[in] fd file descriptor, returned by socket() or open()
  @return 
     -1 - error ocurred 
     other - page count, 1 page = 4K 
  -----------------------------------------------------------------------------------------------*/
int ql_com_mem_get_rss(void)
{
    char buf[256];
    FILE *fp;
    char *ptr, *ptr_save;

    snprintf(buf, sizeof(buf), "/proc/%d/statm", getpid());
    fp = fopen(buf, "r");
    if(!fp) {
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), fp);
    fclose(fp);
    ptr = strtok_r(buf, " ", &ptr_save);
    if(!ptr) {
        return -1;
    }

    ptr = strtok_r(NULL, " ", &ptr_save);
    if(!ptr) {
        return -1;
    }

    return atoi(ptr);
}
