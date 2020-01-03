#include "upload_error_lbs.h"
#include "niu_fota.h"
#include "sys_core.h"
#include "niu_thread.h"

server_info_t error_info = {
        NIU_ID_ECU_SERVER_ERROR_URL,
        NIU_ID_ECU_SERVER_ERROR_URL2,
        NIU_ID_ECU_SERVER_ERROR_PORT,
        NIU_ID_ECU_SERVER_ERROR_PORT2,
        150*1000,
        10*60*1000*1000,
        -1,
        0,
        {0},
        NULL,
        PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_COND_INITIALIZER
    };

server_info_t lbs_info = {
        NIU_ID_ECU_SERVER_LBS_URL,
        NIU_ID_ECU_SERVER_LBS_URL2,
        NIU_ID_ECU_SERVER_LBS_PORT,
        NIU_ID_ECU_SERVER_LBS_PORT2,
        150*1000,
        10*60*1000*1000,
        -1,
        0,
        {0},
        NULL,
        PTHREAD_MUTEX_INITIALIZER,
        PTHREAD_COND_INITIALIZER
    };

char g_http_protocal[] = "POST /ecu/%s HTTP/1.1\r\nContent-Length: %d\r\nContent-Type: application/json\r\nHost: %s:%d\r\nCache-Control: no-cache\r\n\r\n%s";

int error_lbs_init()
{
    int rc = 0;
    pthread_t thread_error_upload = 0;
    pthread_t thread_lbs_upload = 0;
        
    rc = pthread_create(&thread_error_upload, NULL, (void *)error_upload_proc, (void *)&error_info);
    if(0 != rc) {
        LOG_D("error upload ghread create fails");
        return rc;
    }

    rc = pthread_create(&thread_lbs_upload, NULL, (void *)lbs_upload_proc, (void *)&lbs_info);
    if(0 != rc) {
        LOG_D("lbs upload ghread create fails");
        return rc;
    }
    return 0;
}

void *error_upload_proc(void *param)
{
    int i = 0, write_length = 0, content_length = 0;
    struct timeval tm = {0};
    int reconnect_count = 0;
    NIU_DATA_VALUE *temp_niu_data = NULL;
    NIU_DATA_ID_ECU url_index = 0, port_index = 0;
    char tempbuf_url[32] = {0};
    unsigned char tempbuf_port[4] = {0};
    char read_buf[HTTP_BUF_LEN] = {0};

    server_info_t *info = (server_info_t *)param;
    tm.tv_sec = 3;
    tm.tv_usec = 0;
    pthread_detach(pthread_self());
    while(1)
    {
        pthread_mutex_lock(&(info->mut));
        while(0 == info->wakeup_flag) {
            LOG_D("error upload thread suspend");
            pthread_cond_wait(&(info->cond), &(info->mut));
        }
        pthread_mutex_unlock(&(info->mut));
        LOG_D("error upload thread run");
        if(0 == ((reconnect_count / 3) % 2)) {
            url_index = info->server;
            port_index = info->port;
        }
        else {
            url_index = info->server2;
            port_index = info->port2;
        }
        memset(tempbuf_url, 0, 32);
        temp_niu_data = NIU_DATA_ECU(url_index);
        memcpy(tempbuf_url, temp_niu_data->addr, temp_niu_data->len);
        
        memset(tempbuf_port, 0, 4);
        temp_niu_data = NIU_DATA_ECU(port_index);
        memcpy(tempbuf_port, temp_niu_data->addr, temp_niu_data->len);
        
        reconnect_count++;
        LOG_D("error upload thread connect %d number", reconnect_count);
        LOG_D("error upload thread connect to %s at %d", tempbuf_url, *((unsigned int *)tempbuf_port));
        if(0 != network_connect(&(info->socket_handler), tempbuf_url, *((unsigned int *)tempbuf_port))) {
            LOG_D("error upload thread connect %d number failure", reconnect_count);
            if((reconnect_count >= 6) && (0 == (reconnect_count % 3))) {
                usleep(info->interval_exchange_time);
            }
            else {
                usleep(info->interval_time);
            }
            continue;
        }
        else {
            LOG_D("error upload thread connect %d number successful", reconnect_count);
            memset(info->data, 0, HTTP_BUF_LEN);
            snprintf(info->data, HTTP_BUF_LEN, g_http_protocal, "error", strlen(info->json_data), tempbuf_url, *((int *)tempbuf_port), info->json_data);
            content_length = strlen(info->data);
            for(i = 0; i < 3; i++) {
	            setsockopt(info->socket_handler, SOL_SOCKET, SO_SNDTIMEO, (char *)&tm, sizeof(struct timeval));
                write_length = write(info->socket_handler, info->data, content_length);
                if(content_length == write_length) {   //send data successful
                    LOG_D("error upload thread send data successful at %d number", i + 1);
                    memset(read_buf, 0, HTTP_BUF_LEN);
	                setsockopt(info->socket_handler, SOL_SOCKET, SO_RCVTIMEO, (char *)&tm, sizeof(struct timeval));
                    if(read(info->socket_handler, read_buf, HTTP_BUF_LEN) > 0) {        //recive data successful, http successful
                        LOG_D("error upload thread read response data successful");
                        break;
                    }
                    else {          //receive data failure
                        LOG_D("error upload thread read response data failure");
                        continue;
                    }
                }
                else {          //send data failure
                    LOG_D("error upload thread send data failure at %d number", i + 1);
                    continue;
                }
            }
            if(3 == i) {      //send error info failure
                continue;
            }
            else {            //send error info successful
                LOG_D("error upload data thread end and close socket");
                close(info->socket_handler);
                free(info->json_data);
                info->json_data = NULL;
                reconnect_count = 0;
                pthread_mutex_lock(&(info->mut));
                info->wakeup_flag = 0;
                pthread_mutex_unlock(&(info->mut));
            }
        }
    }
    return NULL;
}

void *lbs_upload_proc(void *param)
{
    int i = 0, write_length = 0, content_length = 0;
    struct timeval tm = {0};
    int reconnect_count = 0;
    NIU_DATA_VALUE *temp_niu_data = NULL;
    NIU_DATA_ID_ECU url_index = 0, port_index = 0;
    char tempbuf_url[32] = {0};
    unsigned char tempbuf_port[4] = {0};
    char read_buf[HTTP_BUF_LEN] = {0};
    pthread_detach(pthread_self());
    server_info_t *info = (server_info_t *)param;
    tm.tv_sec = 3;
    tm.tv_usec = 0;
    while(1) {
        pthread_mutex_lock(&(info->mut));
        while(0 == info->wakeup_flag) {
            LOG_D("lbs upload thread suspend");
            pthread_cond_wait(&(info->cond), &(info->mut));
        }
        pthread_mutex_unlock(&(info->mut));
        LOG_D("lbs upload thread run");

        if(0 == ((reconnect_count / 3) % 2)) {
            url_index = info->server;
            port_index = info->port;
        }
        else {
            url_index = info->server2;
            port_index = info->port2;
        }
        memset(tempbuf_url, 0, 32);
        temp_niu_data = NIU_DATA_ECU(url_index);
        memcpy(tempbuf_url, temp_niu_data->addr, temp_niu_data->len);
        
        memset(tempbuf_port, 0, 4);
        temp_niu_data = NIU_DATA_ECU(port_index);
        memcpy(tempbuf_port, temp_niu_data->addr, temp_niu_data->len);
     
        reconnect_count++;
        LOG_D("lbs upload thread connect %d number", reconnect_count);
        LOG_D("lbs upload thread connect to %s at %d", tempbuf_url, *((int *)tempbuf_port));
        if(0 != network_connect(&(info->socket_handler), tempbuf_url, *((int *)tempbuf_port))) {
            LOG_D("lbs upload thread connect %d number failure", reconnect_count);
            if((reconnect_count >= 6) && (0 == (reconnect_count % 3))) {
                usleep(info->interval_exchange_time);
            }
            else {
                usleep(info->interval_time);
            }
            continue;
        }
        else {
            LOG_D("lbs upload thread connect %d number successful", reconnect_count);
            memset(info->data, 0, HTTP_BUF_LEN);
            snprintf(info->data, HTTP_BUF_LEN, g_http_protocal, "boot", strlen(info->json_data), tempbuf_url, *((int *)tempbuf_port), info->json_data);
            content_length = strlen(info->data);
            for(i = 0; i < 3; i++) {
	            setsockopt(info->socket_handler, SOL_SOCKET, SO_SNDTIMEO, (char *)&tm, sizeof(struct timeval));
                write_length = write(info->socket_handler, info->data, content_length);
              
                if(content_length == write_length) {   //send data successful
                    LOG_D("lbs upload thread send data successful at %d number", i + 1);
                    memset(read_buf, 0, HTTP_BUF_LEN);
	                setsockopt(info->socket_handler, SOL_SOCKET, SO_RCVTIMEO, (char *)&tm, sizeof(struct timeval));
                    if(read(info->socket_handler, read_buf, HTTP_BUF_LEN) > 0) {        //recive data successful, http successful
                        LOG_D("lbs upload thread read response data successful");
                        break;
                    }
                    else {          //receive data failure
                        LOG_D("lbs upload thread read response data failure");
                        continue;
                    }
                }
                else {          //send data failure
                    LOG_D("lbs upload thread send data failure at %d number", i + 1);
                    continue;
                }
            }
            if(3 == i) {      //send lbs info failure
                continue;
            }
            else {            //send lbs info successful
                LOG_D("lbs upload data thread end and close socket");
                close(info->socket_handler);
                free(info->json_data);
                info->json_data = NULL;
                reconnect_count = 0;
                pthread_mutex_lock(&(info->mut));
                info->wakeup_flag = 0;
                pthread_mutex_unlock(&(info->mut));
            }
        }
    }
    return NULL;
}


void error_upload(NIU_ERROR_CODE error_code)
{
    cJSON *root = cJSON_CreateObject();
    add_niu_data_to_json(root, "eid", NIU_ECU, NIU_ID_ECU_EID);
    add_niu_data_to_json(root, "imsi", NIU_ECU, NIU_ID_ECU_IMSI);
    add_niu_data_to_json(root, "imei", NIU_ECU, NIU_ID_ECU_IMEI);
    add_niu_data_to_json(root, "iccid", NIU_ECU, NIU_ID_ECU_ICCID);
    cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(error_code));
    cJSON_AddItemToObject(root, "timestamp", cJSON_CreateNumber(time(NULL) - DIFF_TIME));

    pthread_mutex_lock(&(error_info.mut));
    error_info.json_data = cJSON_PrintUnformatted(root);
    LOG_D("wake up error upload thread, error code is %d", error_code);
    error_info.wakeup_flag = 1;
    pthread_cond_signal(&(error_info.cond));
    pthread_mutex_unlock(&(error_info.mut));

    cJSON_Delete(root);
}

void lbs_upload()
{
    base_station_info_t base_info;
    short signal = 0;
    NIU_DATA_VALUE *temp_niu_data;
    cJSON *root = NULL;
    cJSON *data = NULL;
    cmd_signal_strength_info_get(NULL);
    temp_niu_data = NIU_DATA_ECU(NIU_ID_ECU_CSQ);
    signal = *((NUINT8 *)temp_niu_data->addr);
    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    add_niu_data_to_json(root, "eid", NIU_ECU, NIU_ID_ECU_EID);
    add_niu_data_to_json(root, "imei", NIU_ECU, NIU_ID_ECU_IMEI);
    add_niu_data_to_json(root, "utc", NIU_ECU, NIU_ID_ECU_UTC);
    add_niu_data_to_json(root, "longtitude", NIU_ECU, NIU_ID_ECU_LONGITUDE);
    add_niu_data_to_json(root, "latitude", NIU_ECU, NIU_ID_ECU_LATITUDE);
    cJSON_AddItemToObject(root, "timestamp", cJSON_CreateNumber(time(NULL)));

    get_base_station_info(&base_info);
    if(1 == base_info.non_cdma_flag) {
        cJSON_AddItemToObject(root, "cdma", cJSON_CreateNumber(0));
        cJSON_AddItemToObject(root, "bts", data);
        cJSON_AddItemToObject(data, "mcc", cJSON_CreateString(base_info.non_cdma_mcc));
        cJSON_AddItemToObject(data, "mnc", cJSON_CreateString(base_info.non_cdma_mnc));
        cJSON_AddItemToObject(data, "tac", cJSON_CreateNumber(base_info.tac));
        cJSON_AddItemToObject(data, "cellid", cJSON_CreateNumber(base_info.cid));
        cJSON_AddItemToObject(data, "signal", cJSON_CreateNumber(signal * 2 -113));
    }
    else if(1 == base_info.cdma_flag) {
        cJSON_AddItemToObject(root, "cdma", cJSON_CreateNumber(1));
        cJSON_AddItemToObject(root, "bts", data);
        cJSON_AddItemToObject(data, "sid", cJSON_CreateNumber(base_info.sid));
        cJSON_AddItemToObject(data, "nid", cJSON_CreateNumber(base_info.nid));
        cJSON_AddItemToObject(data, "bid", cJSON_CreateNumber(base_info.bsid));
        //cJSON_AddItemToObject(data, "lon", cJSON_CreateNumber(0));
        //cJSON_AddItemToObject(data, "lat", cJSON_CreateNumber(0));
        add_niu_data_to_json(data, "lon", NIU_ECU, NIU_ID_ECU_LONGITUDE);
        add_niu_data_to_json(data, "lat", NIU_ECU, NIU_ID_ECU_LATITUDE);
        cJSON_AddItemToObject(data, "signal", cJSON_CreateNumber(signal * 2 -113));
    }
    else {
        LOG_D("lbs upload failed while get base station info failed");
        cJSON_Delete(root);
        return;
    }
    
    pthread_mutex_lock(&(lbs_info.mut));
    lbs_info.json_data = cJSON_PrintUnformatted(root);
    LOG_D("wake up lbs upload thread");
    lbs_info.wakeup_flag = 1;
    pthread_cond_signal(&(lbs_info.cond));
    pthread_mutex_unlock(&(lbs_info.mut));

    cJSON_Delete(root);
}

int network_connect(int *socket_handler, char* addr, int port)
{
    int rc = 0;
	sa_family_t family = AF_INET;
	int type = SOCK_STREAM;
	struct sockaddr_in address;
	struct addrinfo *result = NULL;
    struct addrinfo *res = NULL;
	struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

	if((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0) {
        res = result;
		while(res) {
			if(res->ai_family == AF_INET) {
				break;
			}
			res = res->ai_next;
		}

		if(res->ai_family == AF_INET) {
			address.sin_port = htons(port);
			address.sin_family = family = AF_INET;
			address.sin_addr = ((struct sockaddr_in*)(res->ai_addr))->sin_addr;
		}
		else {
			rc = -1;
        }
		freeaddrinfo(result);
	}
	if(rc == 0) {
		*socket_handler = socket(family, type, 0);
		if(*socket_handler != -1) {
    		rc = connect(*socket_handler, (struct sockaddr*)&address, sizeof(address));
        }
        else {
            close(*socket_handler);
            rc = -1;
        }
    }
	return rc;
}
