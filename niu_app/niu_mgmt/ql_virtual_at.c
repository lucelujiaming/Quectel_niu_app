/*-----------------------------------------------------------------------------------------------*/
/**
  @file ql_virtual_at.c 
  @brief Virtual AT device  
*/
/*-----------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------
  EDIT HISTORY
  This section contains comments describing changes made to the file.
  Notice that changes are listed in reverse chronological order.
  $Header: $
  when       who          what, where, why
  --------   ---          ----------------------------------------------------------
  20180908   tyler.kuang  Created .
-------------------------------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <stdlib.h>
#include "ql_common.h"
#include "ql_mutex.h"
#include "ql_virtual_at.h"



static int g_fd_master = -1;
static ql_mutex_t g_mutex;

static void _disable_echo(int ttyfd) {
    struct termios  ios;

    /* disable echo on serial ports */
    memset(&ios, 0, sizeof(ios));
    tcgetattr( ttyfd, &ios );
    cfmakeraw(&ios);
    ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */
    cfsetispeed(&ios, B115200);
    cfsetospeed(&ios, B115200);
    tcsetattr( ttyfd, TCSANOW, &ios );
    tcflush(ttyfd, TCIOFLUSH);
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Initialization function 
  @return  On success, the number of bytes written is returned, On error, -1 is returned
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_init(void)
{
    const char *slave_name = NULL;
    g_fd_master = open("/dev/ptmx", O_RDWR | O_NOCTTY);
    if(g_fd_master < 0) {
        LOG_F("Failed to open tty master device, err=%s", strerror(errno));
        return -1;
    }

    if (grantpt(g_fd_master) < 0) {
        LOG_E("Failed to grantpt, err=%s\n", strerror(errno));
        return -1;
    }

    if(unlockpt(g_fd_master) < 0)
    {
        LOG_E("Failed to unlockkpt, err=%s\n", strerror(errno));
        return -1;
    }

    _disable_echo(g_fd_master);

    slave_name = (const char *)ptsname(g_fd_master);

    if(QL_COM_STR_EMPTY(slave_name)) {
        LOG_F("AT tty device is null");
        return -1;
    }

    unlink(QL_VIRTUAL_AT_DEVICE);
    ql_com_system("ln -s %s %s", slave_name, QL_VIRTUAL_AT_DEVICE);

    ql_com_fd_noblock(g_fd_master, 1);

    ql_mutex_init(&g_mutex, "AT device");
    
    return 0;    
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief writes up to count bytes from the buffer pointed buf to the AT device 
  @param[in] buf Buffer
  @param[in] len Length
  @return  On success, the number of bytes written is returned, On error, -1 is returned
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_write(const void *buf, int len)
{
    int ret = 0;

    if(g_fd_master < 0) {
        return -1;
    }
    
    QL_MUTEX_LOCK(&g_mutex);
    ret = write(g_fd_master, buf, len); 
    if(ret < 0) {
        LOG_E("Failed to write, err=%s", strerror(errno));
    }
    QL_MUTEX_UNLOCK(&g_mutex);

    return ret;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Wapper of ql_at_write, formatted output conversion, max length is 256 
  @return  On success, the number of bytes written is returned, On error, -1 is returned
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_print(const char *fmt, ...)
{
    va_list ap;
    char buf[256];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return ql_at_write(buf, strlen(buf));
}



/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Check wheather the AT device is ready for reading
  @return 
  0 - not ready
  1 - ready for reading 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_has_data(void)
{
    int ret = 0;

    if(g_fd_master < 0) {
        return -1;
    }

    QL_MUTEX_LOCK(&g_mutex);
    ret = ql_com_fd_has_data(g_fd_master); 
    QL_MUTEX_UNLOCK(&g_mutex);

    return ret;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Attempts to read up to count bytes from AT device into the buffer starting at buf
  @param[in] buf
  @param[in] len
  @return  On success, the number of bytes read is returned, On error, -1 is returned
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_read(void *buf, int len)
{
    int ret = 0;

    if(g_fd_master < 0) {
        return -1;
    }

    QL_MUTEX_LOCK(&g_mutex);
    ret = read(g_fd_master, buf, len);
    if(ret < 0) {
        LOG_E("Failed to read, err=%s", strerror(errno));
    }
    QL_MUTEX_UNLOCK(&g_mutex);

    return ret;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Get file descriptor of tht AT device 
  @return The file descriptor 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_get_fd(void)
{
    return g_fd_master;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  @brief Uninit function 
  @return Always 0 is returned 
  */
/*-----------------------------------------------------------------------------------------------*/
int ql_at_uinit(void)
{
    if(g_fd_master >= 0) {
        close(g_fd_master);
        g_fd_master = -1;

        ql_mutex_destroy(&g_mutex);
    }

    return 0;
}
