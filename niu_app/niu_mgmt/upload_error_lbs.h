#ifndef __UPLOAD_ERROR_LBS_H__
#define __UPLOAD_ERROR_LBS_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "service.h"
#include "niu_data.h"
#include "niu_data_pri.h"
#include "niu_types.h"
#include "cJSON.h"

#define HTTP_BUF_LEN 4096

typedef struct server_info_struct {
    NIU_DATA_ID_ECU server;
    NIU_DATA_ID_ECU server2;
    NIU_DATA_ID_ECU port;
    NIU_DATA_ID_ECU port2;
    int interval_time;
    int interval_exchange_time;
    int socket_handler;
    int wakeup_flag;
    char data[HTTP_BUF_LEN];
    char *json_data;
    pthread_mutex_t mut;
    pthread_cond_t cond;
}server_info_t;

int error_lbs_init();
void* error_upload_proc(void *param);
void* lbs_upload_proc(void *param);
void lbs_upload();
void error_upload(NIU_ERROR_CODE error_code);
int network_connect(int *socket_handler, char* addr, int port);

#endif
