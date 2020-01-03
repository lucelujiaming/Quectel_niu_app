#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ql_oe.h>
#include "cJSON.h"
#include "niu_data.h"
#include "utils.h"
#include "service.h"
#include "sys_core.h"
#include "niu_device.h"
#include "niu_data_pri.h"
#include "niu_version.h"


typedef struct _niu_data_config_
{
    NUINT8          device_type[32];        //device type
    NUINT8          device[32];             //device
    NUINT8          product_type[32];       //product type
    NUINT8          eid[16];                //eid
    NUINT8          server_url[32];         //服务器域名
    NUINT32         server_port;            //服务器端口
    NUINT8          serial_number[16];      //中控序列号
    NUINT8          soft_ver[8];            //中控软件版本
    NUINT8          hard_ver[8];            //中控硬件版本
    NUINT8          client_id[16];          //MQTT Connect :  client_id
    NUINT8          server_username[16];    //MQTT Connect:   user_name
    NUINT8          server_password[16];    //MQTT Connect:   password
    NUINT8          aes_key[16];            //AES 加密
    NUINT8          mqtt_subscribe[8][32];  
    NUINT8              mqtt_subscribe_items;   //subscribe count read, max = 8
    NUINT8          mqtt_publish[8][32];
    NUINT8              mqtt_publish_items;     //publish count read, max = 8
    NUINT8          ble_mac[8];
    NUINT8          ble_password[16];
    NUINT8          ble_aes[16];
    NUINT8          ble_name[32];
    NUINT8          server_url2[32];
    NUINT32         server_port2;            //服务器端口
    NUINT8          server_error_url[32];
    NUINT32         server_error_port;
    NUINT8          server_error_url2[32];
    NUINT32         server_error_port2;
    NUINT8          server_lbs_url[32];
    NUINT32         server_lbs_port;
    NUINT8          server_lbs_url2[32];
    NUINT32         server_lbs_port2;
    NUINT8          remark[64];
    NUINT32         ecu_id;
    NUINT8          imei[16];
}NIU_DATA_CONFIG;

typedef struct _niu_data_config_rw_
{
    NUINT32 template_version;
    NUINT16 uploading_template_1;  //包组合编号1
    NUINT16 uploading_template_2;  //包组合编号2
    NUINT16 uploading_template_3;  //包组合编号3
    NUINT16 uploading_template_4;  //包组合编号4
    NUINT16 uploading_template_5;  //包组合编号5
    NUINT16 uploading_template_6;  //包组合编号6
    NUINT16 uploading_template_7;  //包组合编号7
    NUINT16 uploading_template_8;  //包组合编号8
    NUINT16 uploading_template_9;  //包组合编号9
    NUINT16 uploading_template_10; //包组合编号10
    NUINT16 uploading_template_11; //包组合编号1
    NUINT16 uploading_template_12; //包组合编号2
    NUINT16 uploading_template_13; //包组合编号3
    NUINT16 uploading_template_14; //包组合编号4
    NUINT16 uploading_template_15; //包组合编号5
    NUINT16 uploading_template_16; //包组合编号6
    NUINT16 uploading_template_17; //包组合编号7
    NUINT16 uploading_template_18; //包组合编号8
    NUINT16 uploading_template_19; //包组合编号9
    NUINT16 uploading_template_20; //包组合编号10
    NUINT16 uploading_template_21; //包组合编号1
    NUINT16 uploading_template_22; //包组合编号2
    NUINT16 uploading_template_23; //包组合编号3
    NUINT16 uploading_template_24; //包组合编号4
    NUINT16 uploading_template_25; //包组合编号5
    NUINT16 uploading_template_26; //包组合编号6
    NUINT16 uploading_template_27; //包组合编号7
    NUINT16 uploading_template_28; //包组合编号8
    NUINT16 uploading_template_29; //包组合编号9
    NUINT16 uploading_template_30; //包组合编号10
    NUINT16 uploading_template_31; //包组合编号1
    NUINT16 uploading_template_32; //包组合编号2
    NUINT16 uploading_template_33; //包组合编号3
    NUINT16 uploading_template_34; //包组合编号4
    NUINT16 uploading_template_35; //包组合编号5
    NUINT16 uploading_template_36; //包组合编号6
    NUINT16 uploading_template_37; //包组合编号7
    NUINT16 uploading_template_38; //包组合编号8
    NUINT16 uploading_template_39; //包组合编号9
    NUINT16 uploading_template_40; //包组合编号10
    NUINT16 uploading_template_41; //包组合编号1
    NUINT16 uploading_template_42; //包组合编号2
    NUINT16 uploading_template_43; //包组合编号3
    NUINT16 uploading_template_44; //包组合编号4
    NUINT16 uploading_template_45; //包组合编号5
    NUINT16 uploading_template_46; //包组合编号6
    NUINT16 uploading_template_47; //包组合编号7
    NUINT16 uploading_template_48; //包组合编号8
    NUINT16 uploading_template_49; //包组合编号9
    NUINT16 uploading_template_50; //包组合编号10
    NIU_SERVER_UPL_TEMPLATE template_data[NIU_SERVER_UPL_CMD_NUM];
} NIU_DATA_CONFIG_RW;

//JSTATIC NIU_DATA_CONFIG g_niu_data_config;
JSTATIC NIU_DATA_CONFIG_RW g_niu_data_rw = {0};
JSTATIC JBOOL g_niu_imei_ok = JFALSE;
JSTATIC NUINT64 g_niu_rw_flush_timstamp = 0;
NCHAR g_niu_rw_file_path[CONFIG_FILE_PATH_LEN_MAX] = {0};

#if 1
JSTATIC JVOID niu_data_config_dump(NIU_DATA_CONFIG *cfg)
{
    int items;
    NUINT8 print_buff[100];

    if(cfg == NULL)
    {
        LOG_E("cfg is null.");
        return ;
    }
    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->device_type, sizeof(cfg->device_type));
    LOG_D("device_type: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->device, sizeof(cfg->device));
    //LOG_D("device: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->product_type, sizeof(cfg->product_type));
    LOG_D("product_type: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->eid, sizeof(cfg->eid));
    LOG_D("eid: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->server_url, sizeof(cfg->server_url));
    LOG_D("server_url: %s", print_buff);

    LOG_D("server_port: %d", cfg->server_port);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->serial_number, sizeof(cfg->serial_number));
    LOG_D("serial_number: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->soft_ver, sizeof(cfg->soft_ver));
    LOG_D("soft_ver: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->hard_ver, sizeof(cfg->hard_ver));
    LOG_D("hard_ver: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->client_id, sizeof(cfg->client_id));
    LOG_D("client_id: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->server_username, sizeof(cfg->server_username));
    LOG_D("server_username: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->server_password, sizeof(cfg->server_password));
    LOG_D("server_password: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->aes_key, sizeof(cfg->aes_key));
    LOG_D("aes_key: %s", print_buff);

    for(items = 0; items < cfg->mqtt_subscribe_items; items++)
    {
        memset(print_buff, 0, sizeof(print_buff));
        memcpy(print_buff, cfg->mqtt_subscribe[items], sizeof(cfg->mqtt_subscribe[items]));
        LOG_D("mqtt_subscribe[%d]: %s", items, print_buff);
    }

    for(items = 0; items < cfg->mqtt_publish_items; items++)
    {
        memset(print_buff, 0, sizeof(print_buff));
        memcpy(print_buff, cfg->mqtt_publish[items], sizeof(cfg->mqtt_publish[items]));
        LOG_D("mqtt_publish[%d]: %s", items, print_buff);
    }

    LOG_D("ble_mac: %02x:%02x:%02x:%02x:%02x:%02x", cfg->ble_mac[0], cfg->ble_mac[1], cfg->ble_mac[2], cfg->ble_mac[3], cfg->ble_mac[4], cfg->ble_mac[5]);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->ble_password, sizeof(cfg->ble_password));
    LOG_D("ble_password: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->ble_aes, sizeof(cfg->ble_aes));
    LOG_D("ble_aes: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->ble_name, sizeof(cfg->ble_name));
    LOG_D("ble_name: %s", print_buff);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->server_url2, sizeof(cfg->server_url2));
    LOG_D("server_url2: %s", print_buff);
    
    LOG_D("server_port2: %d", cfg->server_port2);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->server_error_url2, sizeof(cfg->server_error_url2));
    LOG_D("server_error_url2: %s", print_buff);

    LOG_D("server_error_port2: %d", cfg->server_error_port2);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->server_lbs_url, sizeof(cfg->server_lbs_url));
    LOG_D("server_lbs_url: %s", print_buff);

    LOG_D("server_lbs_port: %d", cfg->server_lbs_port);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->server_lbs_url2, sizeof(cfg->server_lbs_url2));
    LOG_D("server_lbs_url2: %s", print_buff);

    LOG_D("server_lbs_port2: %d", cfg->server_lbs_port2);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->remark, sizeof(cfg->remark));
    LOG_D("remark: %s", print_buff);

    LOG_D("ecu_id: %d", cfg->ecu_id);

    memset(print_buff, 0, sizeof(print_buff));
    memcpy(print_buff, cfg->imei, sizeof(cfg->imei));
    LOG_D("imei: %s", print_buff);
}
#endif



int niu_config_data_check(char *fname)
{
    int ret, fsize = 0, pos = 0;
    struct stat st;
    FILE *fp = NULL;
    char *content = NULL;
    cJSON *root = NULL, *tmp = NULL;

    if(fname == NULL)
    {
        LOG_E("fname not set.");
        return -1;
    }

    ret = stat(fname, &st);
    if(ret)
    {
        LOG_E("stat error.");
        return -1;
    }
    if(!S_ISREG(st.st_mode))
    {
        LOG_E("fname is not a regular file.");
        return -1;
    }

    fsize = st.st_size;
    content = QL_MEM_ALLOC(fsize);
    if(content == NULL)
    {
        LOG_E("malloc error.");
        return -1;
    }

    fp = fopen(fname, "r");
    if(fp == NULL)
    {
        LOG_E("fopen error.");
        QL_MEM_FREE(content);
        return -1;
    }

    for(;;)
    {
        ret = fread(content + pos, 1, 128, fp);
        if(ret <= 0)
        {
            break;
        }
        else
        {
            pos += ret;
            continue;
        }
    }
    fclose(fp);

    root = cJSON_Parse(content);
    if(root == NULL)
    {
        LOG_E("cJSON_Parse error.");
        QL_MEM_FREE(content);
        return -1;
    }
    ret = -1;
    tmp = cJSON_GetObjectItem(root, "eid");
    if(tmp == NULL)
    {
        LOG_E("eid field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("eid field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverUrl");
    if(tmp == NULL)
    {
        LOG_E("serverUrl field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("serverUrl field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverPort");
    if(tmp == NULL)
    {
        LOG_E("serverPort field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("serverPort field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverUsername");
    if(tmp == NULL)
    {
        LOG_E("serverUsername field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("serverUsername field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverPassword");
    if(tmp == NULL)
    {
        LOG_E("serverPassword field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("serverPassword field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "aesKey");
    if(tmp == NULL)
    {
        LOG_E("aesKey field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("aesKey field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverErrorUrl");
    if(tmp == NULL)
    {
        LOG_E("serverErrorUrl field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("serverErrorUrl field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverErrorPort");
    if(tmp == NULL)
    {
        LOG_E("serverErrorPort field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("serverErrorPort field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverLbsUrl");
    if(tmp == NULL)
    {
        LOG_E("serverLbsUrl field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("serverLbsUrl field invalid.");
            goto _out;
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverLbsPort");
    if(tmp == NULL)
    {
        LOG_E("serverLbsPort field not exist.");
        goto _out;
    }
    else
    {
        if(tmp->type != cJSON_String && strlen(tmp->valuestring) == 0)
        {
            LOG_E("serverLbsPort field invalid.");
            goto _out;
        }
    }
    ret = 0;
_out:
    cJSON_Delete(root);
    QL_MEM_FREE(content);
    return ret;

}

int niu_config_data_load(char *fname, NIU_DATA_CONFIG *cfg)
{
    struct stat st;
    int fsize = 0, pos = 0, ret, i;
    FILE *fp = NULL;
    char *content = NULL;
    cJSON *items = NULL, *root = NULL, *tmp = NULL;

    if(fname == NULL || cfg == NULL)
    {
        LOG_E("input param error.");
        return -1;
    }
    
    ret = stat(fname, &st);
    if(ret)
    {
        LOG_E("stat error.");
        return -1;
    }

    if(!S_ISREG(st.st_mode))
    {
        LOG_E("fname is not a regular file.");
        return -1;
    }

    fsize = st.st_size;
    content = QL_MEM_ALLOC(fsize);
    if(content == NULL)
    {
        LOG_E("malloc error.");
        return -1;
    }

    fp = fopen(fname, "r");
    if(fp == NULL)
    {
        LOG_E("fopen error.");
        QL_MEM_FREE(content);
        return -1;
    }

    for(;;)
    {
        ret = fread(content + pos, 1, 128, fp);
        if(ret <= 0)
        {
            break;
        }
        else
        {
            pos += ret;
            continue;
        }
    }
    fclose(fp);

    root = cJSON_Parse(content);
    if(root == NULL)
    {
        LOG_E("cJSON_Parse error.");
        QL_MEM_FREE(content);
        return -1;
    }
    //device_type
    tmp = cJSON_GetObjectItem(root, "deviceType");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->device_type, tmp->valuestring, sizeof(cfg->device_type));
        }
    }

    tmp = cJSON_GetObjectItem(root, "device");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->device, tmp->valuestring, sizeof(cfg->device));
        }
    }

    tmp = cJSON_GetObjectItem(root, "productType");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->product_type, tmp->valuestring, sizeof(cfg->product_type));
        }
    }

    tmp = cJSON_GetObjectItem(root, "eid");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->eid, tmp->valuestring, sizeof(cfg->eid));
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverUrl");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->server_url, tmp->valuestring, sizeof(cfg->server_url));
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverPort");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            cfg->server_port = atoi(tmp->valuestring);
        }
    }

    tmp = cJSON_GetObjectItem(root, "serialNumber");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->serial_number, tmp->valuestring, sizeof(cfg->serial_number));
        }
    }

    tmp = cJSON_GetObjectItem(root, "softVer");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->soft_ver, tmp->valuestring, sizeof(cfg->soft_ver));
        }
    }

    tmp = cJSON_GetObjectItem(root, "hardVer");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->hard_ver, tmp->valuestring, sizeof(cfg->hard_ver));
        }
    }

    tmp = cJSON_GetObjectItem(root, "clientId");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->client_id, tmp->valuestring, sizeof(cfg->client_id));
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverUsername");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->server_username, tmp->valuestring, sizeof(cfg->server_username));
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverPassword");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->server_password, tmp->valuestring, sizeof(cfg->server_password));
        }
    }

    tmp = cJSON_GetObjectItem(root, "aesKey");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->aes_key, tmp->valuestring, sizeof(cfg->aes_key));
        }
    }

    tmp = cJSON_GetObjectItem(root, "mqttSubscribe");
    if(tmp->type == cJSON_Array)
    {
        cfg->mqtt_subscribe_items = cJSON_GetArraySize(tmp);
        for(i = 0; i < cfg->mqtt_subscribe_items; i++)
        {
            items = cJSON_GetArrayItem(tmp, i);
            if(items != NULL)
            {
                if(items->type == cJSON_String)
                {
                    strncpy((char*)cfg->mqtt_subscribe[i], items->valuestring, sizeof(cfg->mqtt_subscribe[i]));
                }
            }
        }
    }

    tmp = cJSON_GetObjectItem(root, "mqttPublish");
    if(tmp->type == cJSON_Array)
    {
        cfg->mqtt_publish_items = cJSON_GetArraySize(tmp);
        for(i = 0; i < cfg->mqtt_publish_items; i++)
        {
            items = cJSON_GetArrayItem(tmp, i);
            if(items != NULL)
            {
                if(items->type == cJSON_String)
                {
                    strncpy((char*)cfg->mqtt_publish[i], items->valuestring, sizeof(cfg->mqtt_publish[i]));
                }
            }
        }
    }

    tmp = cJSON_GetObjectItem(root, "bleMac");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            sscanf(tmp->valuestring, "%02x:%02x:%02x:%02x:%02x:%02x",
                                                (NUINT32*)&cfg->ble_mac[0], (NUINT32*)&cfg->ble_mac[1], (NUINT32*)&cfg->ble_mac[2],
                                                (NUINT32*)&cfg->ble_mac[3], (NUINT32*)&cfg->ble_mac[4], (NUINT32*)&cfg->ble_mac[5]);
        }
    }

    tmp = cJSON_GetObjectItem(root, "blePassword");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->ble_password, tmp->valuestring, sizeof(cfg->ble_password));
        }
    }

    tmp = cJSON_GetObjectItem(root, "bleAes");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->ble_aes, tmp->valuestring, sizeof(cfg->ble_aes));
        }
    }

    tmp = cJSON_GetObjectItem(root, "bleName");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->ble_name, tmp->valuestring, sizeof(cfg->ble_name));
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverUrl2");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->server_url2, tmp->valuestring, sizeof(cfg->server_url2));
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverPort2");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            cfg->server_port2 = atoi(tmp->valuestring);
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverErrorUrl");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->server_error_url, tmp->valuestring, sizeof(cfg->server_error_url));
        }
    }
    
    tmp = cJSON_GetObjectItem(root, "serverErrorPort");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            cfg->server_error_port = atoi(tmp->valuestring);
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverErrorUrl2");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->server_error_url2, tmp->valuestring, sizeof(cfg->server_error_url2));
        }
    }
    
    tmp = cJSON_GetObjectItem(root, "serverErrorPort2");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            cfg->server_error_port2 = atoi(tmp->valuestring);
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverLbsUrl");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->server_lbs_url, tmp->valuestring, sizeof(cfg->server_lbs_url));
        }
    }
    
    tmp = cJSON_GetObjectItem(root, "serverLbsPort");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            cfg->server_lbs_port = atoi(tmp->valuestring);
        }
    }

    tmp = cJSON_GetObjectItem(root, "serverLbsUrl2");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            strncpy((char*)cfg->server_lbs_url2, tmp->valuestring, sizeof(cfg->server_lbs_url2));
        }
    }
    
    tmp = cJSON_GetObjectItem(root, "serverLbsPort2");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
            cfg->server_lbs_port2 = atoi(tmp->valuestring);
        }
    }


    tmp = cJSON_GetObjectItem(root, "remark");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
             strncpy((char*)cfg->remark, tmp->valuestring, sizeof(cfg->remark));
        }
    }

    tmp = cJSON_GetObjectItem(root, "ecuId");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_Number)
        {
             cfg->ecu_id = tmp->valueint;
        }
    }
    tmp = cJSON_GetObjectItem(root, "imei");
    if(tmp != NULL)
    {
        if(tmp->type == cJSON_String && strlen(tmp->valuestring) > 0)
        {
             strncpy((char*)cfg->imei, tmp->valuestring, sizeof(cfg->imei));
        }
    }
    cJSON_Delete(root);
    QL_MEM_FREE(content);
    return 0;
}

int niu_config_data_sync_to_niu_data(char *fname)
{
    int ret;
    NIU_DATA_CONFIG cfg;

    if(fname == NULL)
    {
        LOG_E("fname is null.");
        return -1;
    }

    ret = niu_config_data_load(fname, &cfg);
    if(ret)
    {
        LOG_E("niu config data load failed.\n");
        return -1;
    }
    niu_data_config_dump(&cfg);
    //write config fields to v3 table, and sync to mcu
	niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_URL,            (NUINT8*)cfg.server_url,           sizeof(cfg.server_url));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_PORT,           (NUINT8*)&cfg.server_port,         sizeof(cfg.server_port));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_SERIAL_NUMBER,     (NUINT8*)cfg.serial_number,        sizeof(cfg.serial_number));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_ECU_HARD_VER,          (NUINT8*)cfg.hard_ver,             sizeof(cfg.hard_ver));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_EID,                   (NUINT8*)cfg.eid,                  sizeof(cfg.eid));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_BLE_AES,               (NUINT8*)cfg.ble_aes,              sizeof(cfg.ble_aes));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_BLE_MAC,               (NUINT8*)cfg.ble_mac,              sizeof(cfg.ble_mac));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_BLE_NAME,              (NUINT8*)cfg.ble_name,             sizeof(cfg.ble_name));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_BLE_PASSWORD,          (NUINT8*)cfg.ble_password,         sizeof(cfg.ble_password));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_URL2,           (NUINT8*)cfg.server_url2,          sizeof(cfg.server_url2));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_PORT2,          (NUINT8*)&cfg.server_port2,        sizeof(cfg.server_port2));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_ERROR_URL,      (NUINT8*)cfg.server_error_url,     sizeof(cfg.server_error_url));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_ERROR_PORT,     (NUINT8*)&cfg.server_error_port,   sizeof(cfg.server_error_port));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_ERROR_URL2,     (NUINT8*)cfg.server_error_url2,    sizeof(cfg.server_error_url2));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_ERROR_PORT2,    (NUINT8*)&cfg.server_error_port2,  sizeof(cfg.server_error_port2));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_LBS_URL,        (NUINT8*)cfg.server_lbs_url,       sizeof(cfg.server_lbs_url));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_LBS_PORT,       (NUINT8*)&cfg.server_lbs_port,     sizeof(cfg.server_lbs_port));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_LBS_URL2,       (NUINT8*)cfg.server_lbs_url2,      sizeof(cfg.server_lbs_url2));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVER_LBS_PORT2,      (NUINT8*)&cfg.server_lbs_port2,    sizeof(cfg.server_lbs_port2));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVERUSERNAME,        (NUINT8*)cfg.server_username,      sizeof(cfg.server_username));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_SERVERPASSWORD,        (NUINT8*)cfg.server_password,      sizeof(cfg.server_password));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_AESKEY,                (NUINT8*)cfg.aes_key,              sizeof(cfg.aes_key));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_MQTTPUBLISH,           (NUINT8*)&cfg.mqtt_publish[0],     sizeof(cfg.mqtt_publish[0]));
    niu_data_write_value(NIU_ECU, NIU_ID_ECU_MQTTSUBSCRIBE,         (NUINT8*)&cfg.mqtt_subscribe[0],   sizeof(cfg.mqtt_subscribe[0]));
    return 0;
}

#if 0
int niu_config_data_sync_to_rw(char *fname)
{
    int ret;
    NIU_DATA_CONFIG cfg;

    if(fname == NULL)
    {
        LOG_E("fname is null.");
        return -1;
    }

    ret = niu_config_data_load(fname, &cfg);
    if(ret)
    {
        LOG_E("niu config data load failed.\n");
        return -1;
    }
    niu_data_config_dump(&cfg);
    /*
    update rw
    server_url, server_port, serial_number, ble_mac, ble_password, ble_name, ble_aes
    */
    niu_data_rw_write(NIU_DATA_RW_SERVER_URL,           (NUINT8*)&cfg.server_url,           sizeof(cfg.server_url));
    niu_data_rw_write(NIU_DATA_RW_SERVER_PORT,          (NUINT8*)&cfg.server_port,          sizeof(cfg.server_port));
    niu_data_rw_write(NIU_DATA_RW_ECU_SERIAL_NUMBER,    (NUINT8*)&cfg.serial_number,        sizeof(cfg.serial_number));
    niu_data_rw_write(NIU_DATA_RW_ECU_BLE_MAC,          (NUINT8*)&cfg.ble_mac,              sizeof(cfg.ble_mac));
    niu_data_rw_write(NIU_DATA_RW_ECU_BLE_PASSWORD,     (NUINT8*)&cfg.ble_password,         sizeof(cfg.ble_password));
    niu_data_rw_write(NIU_DATA_RW_ECU_BLE_NAME,         (NUINT8*)&cfg.ble_name,             sizeof(cfg.ble_name));
    niu_data_rw_write(NIU_DATA_RW_ECU_BLE_AES,          (NUINT8*)&cfg.ble_aes,              sizeof(cfg.ble_aes));
    niu_data_rw_write(NIU_DATA_RW_SERVER_URL2,          (NUINT8*)&cfg.server_url2,          sizeof(cfg.server_url2));
    niu_data_rw_write(NIU_DATA_RW_SERVER_PORT2,         (NUINT8*)&cfg.server_port2,         sizeof(cfg.server_port2));
    niu_data_rw_write(NIU_DATA_RW_SERVER_ERROR_URL,     (NUINT8*)&cfg.server_error_url,     sizeof(cfg.server_error_url));
    niu_data_rw_write(NIU_DATA_RW_SERVER_ERROR_PORT,    (NUINT8*)&cfg.server_error_port,    sizeof(cfg.server_error_port));
    niu_data_rw_write(NIU_DATA_RW_SERVER_ERROR_URL2,    (NUINT8*)&cfg.server_error_url2,    sizeof(cfg.server_error_url2));
    niu_data_rw_write(NIU_DATA_RW_SERVER_ERROR_PORT2,   (NUINT8*)&cfg.server_error_port2,   sizeof(cfg.server_error_port2));
    niu_data_rw_write(NIU_DATA_RW_SERVER_LBS_URL,       (NUINT8*)&cfg.server_lbs_url,       sizeof(cfg.server_lbs_url));
    niu_data_rw_write(NIU_DATA_RW_SERVER_LBS_PORT,      (NUINT8*)&cfg.server_lbs_port,      sizeof(cfg.server_lbs_port));
    niu_data_rw_write(NIU_DATA_RW_SERVER_LBS_URL2,      (NUINT8*)&cfg.server_lbs_url2,      sizeof(cfg.server_lbs_url2));
    niu_data_rw_write(NIU_DATA_RW_SERVER_LBS_PORT2,     (NUINT8*)&cfg.server_lbs_port2,     sizeof(cfg.server_lbs_port2));
    return 0;
}
#endif
JSTATIC JVOID niu_data_rw_dump()
{
    NUINT8 print_info[64] = {0};

    LOG_D("template_version: %d", g_niu_data_rw.template_version);
    LOG_D("uploading_template_1:%d", g_niu_data_rw.uploading_template_1);   //包组合编号1
    LOG_D("uploading_template_2:%d", g_niu_data_rw.uploading_template_2);   //包组合编号2
    LOG_D("uploading_template_3:%d", g_niu_data_rw.uploading_template_3);   //包组合编号3
    LOG_D("uploading_template_4:%d", g_niu_data_rw.uploading_template_4);   //包组合编号4
    LOG_D("uploading_template_5:%d", g_niu_data_rw.uploading_template_5);   //包组合编号5
    LOG_D("uploading_template_6:%d", g_niu_data_rw.uploading_template_6);   //包组合编号6
    LOG_D("uploading_template_7:%d", g_niu_data_rw.uploading_template_7);   //包组合编号7
    LOG_D("uploading_template_8:%d", g_niu_data_rw.uploading_template_8);   //包组合编号8
    LOG_D("uploading_template_9:%d", g_niu_data_rw.uploading_template_9);   //包组合编号9
    LOG_D("uploading_template_10:%d", g_niu_data_rw.uploading_template_10); //包组合编号10
    LOG_D("uploading_template_11:%d", g_niu_data_rw.uploading_template_11); //包组合编号1
    LOG_D("uploading_template_12:%d", g_niu_data_rw.uploading_template_12); //包组合编号2
    LOG_D("uploading_template_13:%d", g_niu_data_rw.uploading_template_13); //包组合编号3
    LOG_D("uploading_template_14:%d", g_niu_data_rw.uploading_template_14); //包组合编号4
    LOG_D("uploading_template_15:%d", g_niu_data_rw.uploading_template_15); //包组合编号5
    LOG_D("uploading_template_16:%d", g_niu_data_rw.uploading_template_16); //包组合编号6
    LOG_D("uploading_template_17:%d", g_niu_data_rw.uploading_template_17); //包组合编号7
    LOG_D("uploading_template_18:%d", g_niu_data_rw.uploading_template_18); //包组合编号8
    LOG_D("uploading_template_19:%d", g_niu_data_rw.uploading_template_19); //包组合编号9
    LOG_D("uploading_template_20:%d", g_niu_data_rw.uploading_template_20); //包组合编号10
    LOG_D("uploading_template_21:%d", g_niu_data_rw.uploading_template_21);   //包组合编号1
    LOG_D("uploading_template_22:%d", g_niu_data_rw.uploading_template_22);   //包组合编号2
    LOG_D("uploading_template_23:%d", g_niu_data_rw.uploading_template_23);   //包组合编号3
    LOG_D("uploading_template_24:%d", g_niu_data_rw.uploading_template_24);   //包组合编号4
    LOG_D("uploading_template_25:%d", g_niu_data_rw.uploading_template_25);   //包组合编号5
    LOG_D("uploading_template_26:%d", g_niu_data_rw.uploading_template_26);   //包组合编号6
    LOG_D("uploading_template_27:%d", g_niu_data_rw.uploading_template_27);   //包组合编号7
    LOG_D("uploading_template_28:%d", g_niu_data_rw.uploading_template_28);   //包组合编号8
    LOG_D("uploading_template_29:%d", g_niu_data_rw.uploading_template_29);   //包组合编号9
    LOG_D("uploading_template_30:%d", g_niu_data_rw.uploading_template_30);   //包组合编号10
    LOG_D("uploading_template_31:%d", g_niu_data_rw.uploading_template_31);   //包组合编号1
    LOG_D("uploading_template_32:%d", g_niu_data_rw.uploading_template_32);   //包组合编号2
    LOG_D("uploading_template_33:%d", g_niu_data_rw.uploading_template_33);   //包组合编号3
    LOG_D("uploading_template_34:%d", g_niu_data_rw.uploading_template_34);   //包组合编号4
    LOG_D("uploading_template_35:%d", g_niu_data_rw.uploading_template_35);   //包组合编号5
    LOG_D("uploading_template_36:%d", g_niu_data_rw.uploading_template_36);   //包组合编号6
    LOG_D("uploading_template_37:%d", g_niu_data_rw.uploading_template_37);   //包组合编号7
    LOG_D("uploading_template_38:%d", g_niu_data_rw.uploading_template_38);   //包组合编号8
    LOG_D("uploading_template_39:%d", g_niu_data_rw.uploading_template_39);   //包组合编号9
    LOG_D("uploading_template_40:%d", g_niu_data_rw.uploading_template_40);   //包组合编号10
    LOG_D("uploading_template_41:%d", g_niu_data_rw.uploading_template_41);   //包组合编号1
    LOG_D("uploading_template_42:%d", g_niu_data_rw.uploading_template_42);   //包组合编号2
    LOG_D("uploading_template_43:%d", g_niu_data_rw.uploading_template_43);   //包组合编号3
    LOG_D("uploading_template_44:%d", g_niu_data_rw.uploading_template_44);   //包组合编号4
    LOG_D("uploading_template_45:%d", g_niu_data_rw.uploading_template_45);   //包组合编号5
    LOG_D("uploading_template_46:%d", g_niu_data_rw.uploading_template_46);   //包组合编号6
    LOG_D("uploading_template_47:%d", g_niu_data_rw.uploading_template_47);   //包组合编号7
    LOG_D("uploading_template_48:%d", g_niu_data_rw.uploading_template_48);   //包组合编号8
    LOG_D("uploading_template_49:%d", g_niu_data_rw.uploading_template_49);   //包组合编号9
    LOG_D("uploading_template_50:%d", g_niu_data_rw.uploading_template_50);   //包组合编号10
}

JBOOL niu_data_config_init()
{
    niu_data_rw_init();
    niu_data_rw_dump();
    return JTRUE;
}
#if 0
JBOOL niu_data_config_read(NIU_DATA_CONFIG_INDEX index, NUINT8 *addr, NUINT32 len)
{
    JBOOL ret = JFALSE;
    //LOG_D("index:%d, addr:%p, len:%d", index, addr, len);
    if (addr && len)
    {
        switch (index)
        {
        case NIU_DATA_CONFIG_SERVER_URL:
        {
            memcpy(addr, g_niu_data_config.server_url, jmin(sizeof(g_niu_data_config.server_url), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_PORT:
        {
            if (len == sizeof(NUINT32))
            {
                memcpy(addr, &g_niu_data_config.server_port, sizeof(NUINT32));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_CONFIG_SERIAL_NUMBER:
        {
            memcpy(addr, g_niu_data_config.serial_number, jmin(sizeof(g_niu_data_config.serial_number), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SOFT_VER:
        {
            memcpy(addr, g_niu_data_config.soft_ver, jmin(sizeof(g_niu_data_config.soft_ver), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_HARD_VER:
        {
            memcpy(addr, g_niu_data_config.hard_ver, jmin(sizeof(g_niu_data_config.hard_ver), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_EID:
        {
            memcpy(addr, g_niu_data_config.eid, jmin(sizeof(g_niu_data_config.eid), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_USERNAME:
        {
            memcpy(addr, g_niu_data_config.server_username, jmin(sizeof(g_niu_data_config.server_username), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_PASSWORD:
        {
            memcpy(addr, g_niu_data_config.server_password, jmin(sizeof(g_niu_data_config.server_password), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_CLIENT_ID:
        {
            memcpy(addr, g_niu_data_config.client_id, jmin(sizeof(g_niu_data_config.client_id), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_AES_KEY:
        {
            memcpy(addr, g_niu_data_config.aes_key, jmin(sizeof(g_niu_data_config.aes_key), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_BLE_MAC:
        {
            memcpy(addr, g_niu_data_config.ble_mac, jmin(sizeof(g_niu_data_config.ble_mac), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_BLE_PASSWORD:
        {
            memcpy(addr, g_niu_data_config.ble_password, jmin(sizeof(g_niu_data_config.ble_password), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_BLE_AES:
        {
            memcpy(addr, g_niu_data_config.ble_aes, jmin(sizeof(g_niu_data_config.ble_aes), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_BLE_NAME:
        {
            memcpy(addr, g_niu_data_config.ble_name, jmin(sizeof(g_niu_data_config.ble_name), len));
            ret = JTRUE;
        }
        break;
        //baron: add new json config fields
        case NIU_DATA_CONFIG_SERVER_URL2:
        {
            memcpy(addr, g_niu_data_config.server_url2, jmin(sizeof(g_niu_data_config.server_url2), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_PORT2:
        {
            memcpy(addr, &g_niu_data_config.server_port2, jmin(sizeof(g_niu_data_config.server_port2), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_ERROR_URL:
        {
            memcpy(addr, g_niu_data_config.server_error_url, jmin(sizeof(g_niu_data_config.server_error_url), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_ERROR_PORT:
        {
            memcpy(addr, &g_niu_data_config.server_error_port, jmin(sizeof(g_niu_data_config.server_error_port), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_ERROR_URL2:
        {
            memcpy(addr, g_niu_data_config.server_error_url2, jmin(sizeof(g_niu_data_config.server_error_url2), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_ERROR_PORT2:
        {
            memcpy(addr, &g_niu_data_config.server_error_port2, jmin(sizeof(g_niu_data_config.server_error_port2), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_LBS_URL:
        {
            memcpy(addr, g_niu_data_config.server_lbs_url, jmin(sizeof(g_niu_data_config.server_lbs_url), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_LBS_PORT:
        {
            memcpy(addr, &g_niu_data_config.server_lbs_port, jmin(sizeof(g_niu_data_config.server_lbs_port), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_LBS_URL2:
        {
            memcpy(addr, g_niu_data_config.server_lbs_url2, jmin(sizeof(g_niu_data_config.server_lbs_url2), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_SERVER_LBS_PORT2:
        {
            memcpy(addr, &g_niu_data_config.server_lbs_port2, jmin(sizeof(g_niu_data_config.server_lbs_port2), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_REMARK:
        {
            memcpy(addr, g_niu_data_config.remark, jmin(sizeof(g_niu_data_config.remark), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_ECU_ID:
        {
            memcpy(addr, &g_niu_data_config.ecu_id, jmin(sizeof(g_niu_data_config.ecu_id), len));
            ret = JTRUE;
        }
        break;
        case NIU_DATA_CONFIG_IMEI:
        {
            memcpy(addr, g_niu_data_config.imei, jmin(sizeof(g_niu_data_config.imei), len));
            ret = JTRUE;
        }
        break;
        default:
            break;
        }
    }

    return ret;
}
#endif
/**
 * init template in data_rw
 * auto created by template.xls
 * 
 */
JVOID niu_data_rw_default()
{
    NUINT8 i = 0;
    memset(&g_niu_data_rw, 0, sizeof(NIU_DATA_CONFIG_RW));

    for (i = 0; i < NIU_SERVER_UPL_CMD_NUM; i++)
    {
        memset(&g_niu_data_rw.template_data[i], 0, sizeof(NIU_SERVER_UPL_TEMPLATE));
        g_niu_data_rw.template_data[i].index = i;
    }

    // 电池插入/ 开电门
    g_niu_data_rw.template_data[0].index = 0;                                                         //序列编号
    g_niu_data_rw.template_data[0].trigger0 = NIU_TRIGGER_EVENT_ACC_ON | NIU_TRIGGER_EVENT_BATTER_IN; //触发条件1
    g_niu_data_rw.template_data[0].trigger1 = 0;                                                      //BMS状态触发掩码
    g_niu_data_rw.template_data[0].trigger2 = 0;                                                      //FOC状态触发掩码
    g_niu_data_rw.template_data[0].trigger3 = 0;                                                      //ECU状态触发掩码
    g_niu_data_rw.template_data[0].trigger4 = 0;                                                      //ALARM状态触发掩码
    g_niu_data_rw.template_data[0].trigger5 = 0;                                                      //LCU状态触发掩码
    g_niu_data_rw.template_data[0].trigger6 = 0;                                                      //DB状态触发掩码
    g_niu_data_rw.template_data[0].trigger7 = 0;                                                      //保留位掩码位
    g_niu_data_rw.template_data[0].trigger8 = 0;                                                      //保留位掩码位
    g_niu_data_rw.template_data[0].trigger9 = 0;                                                      //保留位掩码位
    g_niu_data_rw.template_data[0].packed_name = 15046;                                               //包组合编号
    g_niu_data_rw.template_data[0].trigged_count = 0xFFFF;                                            //
    g_niu_data_rw.template_data[0].send_max = 1;                                                      //使用次数
    g_niu_data_rw.template_data[0].interval = 1;                                                      //发送间隔
    g_niu_data_rw.template_data[0].cur_time = 1;                                                      //当前时间
    g_niu_data_rw.template_data[0].addr_num = 42;                                                     //地址序列长度

    g_niu_data_rw.template_data[0].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[0].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[0].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[0].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[0].addr_array[4].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[4].id = NIU_ID_ECU_ECU_SOFT_VER; // 中控软件版本

    g_niu_data_rw.template_data[0].addr_array[5].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[5].id = NIU_ID_ECU_ECU_HARD_VER; // 中控硬件版本

    g_niu_data_rw.template_data[0].addr_array[6].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[6].id = NIU_ID_ECU_IMEI; // IMEI

    g_niu_data_rw.template_data[0].addr_array[7].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[7].id = NIU_ID_ECU_ICCID; // ICCID

    g_niu_data_rw.template_data[0].addr_array[8].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[8].id = NIU_ID_ECU_IMSI; // IMSI

    g_niu_data_rw.template_data[0].addr_array[9].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[9].id = NIU_ID_ECU_LOST_ENERY; // 当日消耗电量wh

    g_niu_data_rw.template_data[0].addr_array[10].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[10].id = NIU_ID_ECU_LOST_SOC; // 当日消耗电量

    g_niu_data_rw.template_data[0].addr_array[11].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[11].id = NIU_ID_ECU_LAST_MILEAGE; // 上次骑行里程

    g_niu_data_rw.template_data[0].addr_array[12].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[12].id = NIU_ID_ECU_TOTAL_MILEAGE; // 总行驶里程

    g_niu_data_rw.template_data[0].addr_array[13].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[13].id = NIU_ID_BMS_BMS_BAT_TO_CAP; // 电池容量

    g_niu_data_rw.template_data[0].addr_array[14].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[14].id = NIU_ID_BMS_BMS_C_CONT; // 电池循环次数

    g_niu_data_rw.template_data[0].addr_array[15].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[15].id = NIU_ID_BMS_BMS_TO_VLT_RT; // 总电压

    g_niu_data_rw.template_data[0].addr_array[16].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[16].id = NIU_ID_BMS_BMS_C_CUR_RT; // 充电电流

    g_niu_data_rw.template_data[0].addr_array[17].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[17].id = NIU_ID_BMS_BMS_DC_CUR_RT; // 放电电流

    g_niu_data_rw.template_data[0].addr_array[18].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[18].id = NIU_ID_BMS_BMS_SOC_RT; // SOC估算

    g_niu_data_rw.template_data[0].addr_array[19].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[19].id = NIU_ID_BMS_BMS_BAT_STA_RT; // 实时电池状态

    g_niu_data_rw.template_data[0].addr_array[20].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[20].id = NIU_ID_BMS_BMS_DC_FL_T_RT; // 充满电剩余时间

    g_niu_data_rw.template_data[0].addr_array[21].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[21].id = NIU_ID_BMS_BMS_TP1_RT; // 当前温度1

    g_niu_data_rw.template_data[0].addr_array[22].base = NIU_BMS;
    g_niu_data_rw.template_data[0].addr_array[22].id = NIU_ID_BMS_BMS_SN_ID; // 设备序列号

    g_niu_data_rw.template_data[0].addr_array[23].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[23].id = NIU_ID_ECU_LONGITUDE; // 经度

    g_niu_data_rw.template_data[0].addr_array[24].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[24].id = NIU_ID_ECU_LATITUDE; // 纬度

    g_niu_data_rw.template_data[0].addr_array[25].base = NIU_FOC;
    g_niu_data_rw.template_data[0].addr_array[25].id = NIU_ID_FOC_FOC_ID; // ID号

    g_niu_data_rw.template_data[0].addr_array[26].base = NIU_FOC;
    g_niu_data_rw.template_data[0].addr_array[26].id = NIU_ID_FOC_FOC_MODE; // 型号

    g_niu_data_rw.template_data[0].addr_array[27].base = NIU_FOC;
    g_niu_data_rw.template_data[0].addr_array[27].id = NIU_ID_FOC_FOC_SN; // 序列号

    g_niu_data_rw.template_data[0].addr_array[28].base = NIU_FOC;
    g_niu_data_rw.template_data[0].addr_array[28].id = NIU_ID_FOC_FOC_VERSION; // 版本号

    g_niu_data_rw.template_data[0].addr_array[29].base = NIU_FOC;
    g_niu_data_rw.template_data[0].addr_array[29].id = NIU_ID_FOC_FOC_RATEDVLT; // 额定电压

    g_niu_data_rw.template_data[0].addr_array[30].base = NIU_FOC;
    g_niu_data_rw.template_data[0].addr_array[30].id = NIU_ID_FOC_FOC_MINVLT; // 最低电压

    g_niu_data_rw.template_data[0].addr_array[31].base = NIU_FOC;
    g_niu_data_rw.template_data[0].addr_array[31].id = NIU_ID_FOC_FOC_MAXVLT; // 最高电压

    g_niu_data_rw.template_data[0].addr_array[32].base = NIU_FOC;
    g_niu_data_rw.template_data[0].addr_array[32].id = NIU_ID_FOC_FOC_MOTORSPEED; // 电机转速

    g_niu_data_rw.template_data[0].addr_array[33].base = NIU_EMCU;
    g_niu_data_rw.template_data[0].addr_array[33].id = NIU_ID_EMCU_EMCU_SOFT_VER; // 软件版本

    g_niu_data_rw.template_data[0].addr_array[34].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[34].id = NIU_ID_ECU_AUTO_CLOSE_TIME; // 自动关机时间

    g_niu_data_rw.template_data[0].addr_array[35].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[35].id = NIU_ID_ECU_EX_MODEL_VERSION; // 外置模块版本

    g_niu_data_rw.template_data[0].addr_array[36].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[36].id = NIU_ID_ECU_SHAKING_VALUE; // 震动检测阀值

    g_niu_data_rw.template_data[0].addr_array[37].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[37].id = NIU_ID_ECU_BLE_MAC; // 蓝牙mac地址

    g_niu_data_rw.template_data[0].addr_array[38].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[38].id = NIU_ID_ECU_BLE_PASSWORD; // 蓝牙密码

    g_niu_data_rw.template_data[0].addr_array[39].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[39].id = NIU_ID_ECU_BLE_AES; // 蓝牙秘钥

    g_niu_data_rw.template_data[0].addr_array[40].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[40].id = NIU_ID_ECU_BLE_NAME; // 蓝牙名称

    g_niu_data_rw.template_data[0].addr_array[41].base = NIU_ECU;
    g_niu_data_rw.template_data[0].addr_array[41].id = NIU_ID_ECU_INHERIT_MILEAGE; // 继承里程

    // 电池拔出定位 基站定位
    g_niu_data_rw.template_data[1].index = 1;                              //序列编号
    g_niu_data_rw.template_data[1].trigger0 = NIU_TRIGGER_STATE_BATTERY_N; //触发条件1
    g_niu_data_rw.template_data[1].trigger1 = 0;                           //BMS状态触发掩码
    g_niu_data_rw.template_data[1].trigger2 = 0;                           //FOC状态触发掩码
    g_niu_data_rw.template_data[1].trigger3 = 0;                           //ECU状态触发掩码
    g_niu_data_rw.template_data[1].trigger4 = 0;                           //ALARM状态触发掩码
    g_niu_data_rw.template_data[1].trigger5 = 0;                           //LCU状态触发掩码
    g_niu_data_rw.template_data[1].trigger6 = 0;                           //DB状态触发掩码
    g_niu_data_rw.template_data[1].trigger7 = 0;                           //保留位掩码位
    g_niu_data_rw.template_data[1].trigger8 = 0;                           //保留位掩码位
    g_niu_data_rw.template_data[1].trigger9 = 0;                           //保留位掩码位
    g_niu_data_rw.template_data[1].packed_name = 15021;                    //包组合编号
    g_niu_data_rw.template_data[1].trigged_count = 0xFFFF;                 //
    g_niu_data_rw.template_data[1].send_max = 65535;                       //使用次数
    g_niu_data_rw.template_data[1].interval = 2304;                        //发送间隔
    g_niu_data_rw.template_data[1].cur_time = 2304;                        //当前时间
    g_niu_data_rw.template_data[1].addr_num = 29;                          //地址序列长度

    g_niu_data_rw.template_data[1].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[1].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[1].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[1].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[1].addr_array[4].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[4].id = NIU_ID_ECU_CELL_MCC; // 移动国家号

    g_niu_data_rw.template_data[1].addr_array[5].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[5].id = NIU_ID_ECU_CELL_MNC; // 移动网络号码

    g_niu_data_rw.template_data[1].addr_array[6].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[6].id = NIU_ID_ECU_CELL_LAC_1; // 位置区码 

    g_niu_data_rw.template_data[1].addr_array[7].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[7].id = NIU_ID_ECU_CELL_CID_1; // 小区编号

    g_niu_data_rw.template_data[1].addr_array[8].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[8].id = NIU_ID_ECU_CELL_RSSI_1; // 信号强度

    g_niu_data_rw.template_data[1].addr_array[9].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[9].id = NIU_ID_ECU_CELL_LAC_2; // 位置区码 

    g_niu_data_rw.template_data[1].addr_array[10].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[10].id = NIU_ID_ECU_CELL_CID_2; // 小区编号

    g_niu_data_rw.template_data[1].addr_array[11].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[11].id = NIU_ID_ECU_CELL_RSSI_2; // 信号强度

    g_niu_data_rw.template_data[1].addr_array[12].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[12].id = NIU_ID_ECU_CELL_LAC_3; // 位置区码 

    g_niu_data_rw.template_data[1].addr_array[13].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[13].id = NIU_ID_ECU_CELL_CID_3; // 小区编号

    g_niu_data_rw.template_data[1].addr_array[14].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[14].id = NIU_ID_ECU_CELL_RSSI_3; // 信号强度

    g_niu_data_rw.template_data[1].addr_array[15].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[15].id = NIU_ID_ECU_CELL_LAC_4; // 位置区码 

    g_niu_data_rw.template_data[1].addr_array[16].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[16].id = NIU_ID_ECU_CELL_CID_4; // 小区编号

    g_niu_data_rw.template_data[1].addr_array[17].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[17].id = NIU_ID_ECU_CELL_RSSI_4; // 信号强度

    g_niu_data_rw.template_data[1].addr_array[18].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[18].id = NIU_ID_ECU_CELL_LAC_5; // 位置区码 

    g_niu_data_rw.template_data[1].addr_array[19].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[19].id = NIU_ID_ECU_CELL_CID_5; // 小区编号

    g_niu_data_rw.template_data[1].addr_array[20].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[20].id = NIU_ID_ECU_CELL_RSSI_5; // 信号强度

    g_niu_data_rw.template_data[1].addr_array[21].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[21].id = NIU_ID_ECU_LONGITUDE; // 经度

    g_niu_data_rw.template_data[1].addr_array[22].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[22].id = NIU_ID_ECU_LATITUDE; // 纬度

    g_niu_data_rw.template_data[1].addr_array[23].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[23].id = NIU_ID_ECU_HDOP; // 定位精度

    g_niu_data_rw.template_data[1].addr_array[24].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[24].id = NIU_ID_ECU_PHOP; // 定位精度

    g_niu_data_rw.template_data[1].addr_array[25].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[25].id = NIU_ID_ECU_HEADING; // 航向

    g_niu_data_rw.template_data[1].addr_array[26].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[26].id = NIU_ID_ECU_GPS_SINGAL; // GPS信号强度

    g_niu_data_rw.template_data[1].addr_array[27].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[27].id = NIU_ID_ECU_ECU_BATTARY; // 中控电池电量

    g_niu_data_rw.template_data[1].addr_array[28].base = NIU_ECU;
    g_niu_data_rw.template_data[1].addr_array[28].id = NIU_ID_ECU_CSQ; // GSM信号强度

    // 行驶
    g_niu_data_rw.template_data[2].index = 2;                              //序列编号
    g_niu_data_rw.template_data[2].trigger0 = NIU_TRIGGER_STATE_RUNNING_Y; //触发条件1
    g_niu_data_rw.template_data[2].trigger1 = 0;                           //BMS状态触发掩码
    g_niu_data_rw.template_data[2].trigger2 = 0;                           //FOC状态触发掩码
    g_niu_data_rw.template_data[2].trigger3 = 0;                           //ECU状态触发掩码
    g_niu_data_rw.template_data[2].trigger4 = 0;                           //ALARM状态触发掩码
    g_niu_data_rw.template_data[2].trigger5 = 0;                           //LCU状态触发掩码
    g_niu_data_rw.template_data[2].trigger6 = 0;                           //DB状态触发掩码
    g_niu_data_rw.template_data[2].trigger7 = 0;                           //保留位掩码位
    g_niu_data_rw.template_data[2].trigger8 = 0;                           //保留位掩码位
    g_niu_data_rw.template_data[2].trigger9 = 0;                           //保留位掩码位
    g_niu_data_rw.template_data[2].packed_name = 15038;                    //包组合编号
    g_niu_data_rw.template_data[2].trigged_count = 0xFFFF;                 //
    g_niu_data_rw.template_data[2].send_max = 65535;                       //使用次数
    g_niu_data_rw.template_data[2].interval = 60;                          //发送间隔
    g_niu_data_rw.template_data[2].cur_time = 0;                           //当前时间
    g_niu_data_rw.template_data[2].addr_num = 48;                          //地址序列长度

    g_niu_data_rw.template_data[2].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[2].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[2].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[2].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[2].addr_array[4].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[4].id = NIU_ID_BMS_BMS_TO_VLT_RT; // 总电压

    g_niu_data_rw.template_data[2].addr_array[5].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[5].id = NIU_ID_BMS_BMS_C_CUR_RT; // 充电电流

    g_niu_data_rw.template_data[2].addr_array[6].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[6].id = NIU_ID_BMS_BMS_DC_CUR_RT; // 放电电流

    g_niu_data_rw.template_data[2].addr_array[7].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[7].id = NIU_ID_BMS_BMS_SOC_RT; // SOC估算

    g_niu_data_rw.template_data[2].addr_array[8].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[8].id = NIU_ID_BMS_BMS_BAT_STA_RT; // 实时电池状态

    g_niu_data_rw.template_data[2].addr_array[9].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[9].id = NIU_ID_BMS_BMS_DC_FL_T_RT; // 充满电剩余时间

    g_niu_data_rw.template_data[2].addr_array[10].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[10].id = NIU_ID_BMS_BMS_TP1_RT; // 当前温度1

    g_niu_data_rw.template_data[2].addr_array[11].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[11].id = NIU_ID_BMS_BMS_TP2_RT; // 当前温度2

    g_niu_data_rw.template_data[2].addr_array[12].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[12].id = NIU_ID_BMS_BMS_TP3_RT; // 当前温度3

    g_niu_data_rw.template_data[2].addr_array[13].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[13].id = NIU_ID_BMS_BMS_TP4_RT; // 当前温度4

    g_niu_data_rw.template_data[2].addr_array[14].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[14].id = NIU_ID_BMS_BMS_TP5_RT; // 当前温度5

    g_niu_data_rw.template_data[2].addr_array[15].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[15].id = NIU_ID_BMS_BMS_G_VLT_RT1; // 电池组电压1

    g_niu_data_rw.template_data[2].addr_array[16].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[16].id = NIU_ID_BMS_BMS_G_VLT_RT2; // 电池组电压2

    g_niu_data_rw.template_data[2].addr_array[17].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[17].id = NIU_ID_BMS_BMS_G_VLT_RT3; // 电池组电压3

    g_niu_data_rw.template_data[2].addr_array[18].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[18].id = NIU_ID_BMS_BMS_G_VLT_RT4; // 电池组电压4

    g_niu_data_rw.template_data[2].addr_array[19].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[19].id = NIU_ID_BMS_BMS_G_VLT_RT5; // 电池组电压5

    g_niu_data_rw.template_data[2].addr_array[20].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[20].id = NIU_ID_BMS_BMS_G_VLT_RT6; // 电池组电压6

    g_niu_data_rw.template_data[2].addr_array[21].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[21].id = NIU_ID_BMS_BMS_G_VLT_RT7; // 电池组电压7

    g_niu_data_rw.template_data[2].addr_array[22].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[22].id = NIU_ID_BMS_BMS_G_VLT_RT8; // 电池组电压8

    g_niu_data_rw.template_data[2].addr_array[23].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[23].id = NIU_ID_BMS_BMS_G_VLT_RT9; // 电池组电压9

    g_niu_data_rw.template_data[2].addr_array[24].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[24].id = NIU_ID_BMS_BMS_G_VLT_RT10; // 电池组电压10

    g_niu_data_rw.template_data[2].addr_array[25].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[25].id = NIU_ID_BMS_BMS_G_VLT_RT11; // 电池组电压11

    g_niu_data_rw.template_data[2].addr_array[26].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[26].id = NIU_ID_BMS_BMS_G_VLT_RT12; // 电池组电压12

    g_niu_data_rw.template_data[2].addr_array[27].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[27].id = NIU_ID_BMS_BMS_G_VLT_RT13; // 电池组电压13

    g_niu_data_rw.template_data[2].addr_array[28].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[28].id = NIU_ID_BMS_BMS_G_VLT_RT14; // 电池组电压14

    g_niu_data_rw.template_data[2].addr_array[29].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[29].id = NIU_ID_BMS_BMS_G_VLT_RT15; // 电池组电压15

    g_niu_data_rw.template_data[2].addr_array[30].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[30].id = NIU_ID_BMS_BMS_G_VLT_RT16; // 电池组电压16

    g_niu_data_rw.template_data[2].addr_array[31].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[31].id = NIU_ID_BMS_BMS_G_VLT_RT17; // 电池组电压17

    g_niu_data_rw.template_data[2].addr_array[32].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[32].id = NIU_ID_BMS_BMS_G_VLT_RT18; // 电池组电压18

    g_niu_data_rw.template_data[2].addr_array[33].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[33].id = NIU_ID_BMS_BMS_G_VLT_RT19; // 电池组电压19

    g_niu_data_rw.template_data[2].addr_array[34].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[34].id = NIU_ID_BMS_BMS_G_VLT_RT20; // 电池组电压20

    g_niu_data_rw.template_data[2].addr_array[35].base = NIU_FOC;
    g_niu_data_rw.template_data[2].addr_array[35].id = NIU_ID_FOC_FOC_GEARS; // 档位

    g_niu_data_rw.template_data[2].addr_array[36].base = NIU_FOC;
    g_niu_data_rw.template_data[2].addr_array[36].id = NIU_ID_FOC_FOC_RATEDCUR; // 额定电流

    g_niu_data_rw.template_data[2].addr_array[37].base = NIU_FOC;
    g_niu_data_rw.template_data[2].addr_array[37].id = NIU_ID_FOC_FOC_BARCUR; // 母线电流

    g_niu_data_rw.template_data[2].addr_array[38].base = NIU_FOC;
    g_niu_data_rw.template_data[2].addr_array[38].id = NIU_ID_FOC_FOC_MOTORSPEED; // 电机转速

    g_niu_data_rw.template_data[2].addr_array[39].base = NIU_FOC;
    g_niu_data_rw.template_data[2].addr_array[39].id = NIU_ID_FOC_FOC_COTLSTA; // FOC状态

    g_niu_data_rw.template_data[2].addr_array[40].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[40].id = NIU_ID_ECU_TOTAL_MILEAGE; // 总行驶里程

    g_niu_data_rw.template_data[2].addr_array[41].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[41].id = NIU_ID_ECU_LOST_ENERY; // 当日消耗电量wh

    g_niu_data_rw.template_data[2].addr_array[42].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[42].id = NIU_ID_ECU_LOST_SOC; // 当日消耗电量

    g_niu_data_rw.template_data[2].addr_array[43].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[43].id = NIU_ID_ECU_ECU_BATTARY; // 中控电池电量

    g_niu_data_rw.template_data[2].addr_array[44].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[44].id = NIU_ID_BMS_BMS_C_CONT; // 电池循环次数

    g_niu_data_rw.template_data[2].addr_array[45].base = NIU_BMS;
    g_niu_data_rw.template_data[2].addr_array[45].id = NIU_ID_BMS_BMS_SN_ID; // 设备序列号

    g_niu_data_rw.template_data[2].addr_array[46].base = NIU_FOC;
    g_niu_data_rw.template_data[2].addr_array[46].id = NIU_ID_FOC_FOC_CTRLSTA2; // 控制器状态2

    g_niu_data_rw.template_data[2].addr_array[47].base = NIU_ECU;
    g_niu_data_rw.template_data[2].addr_array[47].id = NIU_ID_ECU_ECU_V35_CONFIG; // V35报警器配置

    // 行驶定位
    g_niu_data_rw.template_data[3].index = 3;                                                        //序列编号
    g_niu_data_rw.template_data[3].trigger0 = 0;                                                     //触发条件1
    g_niu_data_rw.template_data[3].trigger1 = 0;                                                     //BMS状态触发掩码
    g_niu_data_rw.template_data[3].trigger2 = 0;                                                     //FOC状态触发掩码
    g_niu_data_rw.template_data[3].trigger3 = NIU_TRIGGER_STATE_RUNNING_Y | NIU_TRIGGER_STATE_GPS_Y; //ECU状态触发掩码
    g_niu_data_rw.template_data[3].trigger4 = 0;                                                     //ALARM状态触发掩码
    g_niu_data_rw.template_data[3].trigger5 = 0;                                                     //LCU状态触发掩码
    g_niu_data_rw.template_data[3].trigger6 = 0;                                                     //DB状态触发掩码
    g_niu_data_rw.template_data[3].trigger7 = 0;                                                     //保留位掩码位
    g_niu_data_rw.template_data[3].trigger8 = 0;                                                     //保留位掩码位
    g_niu_data_rw.template_data[3].trigger9 = 0;                                                     //保留位掩码位
    g_niu_data_rw.template_data[3].packed_name = 15042;                                              //包组合编号
    g_niu_data_rw.template_data[3].trigged_count = 0xFFFF;                                           //
    g_niu_data_rw.template_data[3].send_max = 65535;                                                 //使用次数
    g_niu_data_rw.template_data[3].interval = 5;                                                     //发送间隔
    g_niu_data_rw.template_data[3].cur_time = 0;                                                     //当前时间
    g_niu_data_rw.template_data[3].addr_num = 17;                                                    //地址序列长度

    g_niu_data_rw.template_data[3].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[3].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[3].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[3].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[3].addr_array[4].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[4].id = NIU_ID_ECU_LONGITUDE; // 经度

    g_niu_data_rw.template_data[3].addr_array[5].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[5].id = NIU_ID_ECU_LATITUDE; // 纬度

    g_niu_data_rw.template_data[3].addr_array[6].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[6].id = NIU_ID_ECU_HDOP; // 定位精度

    g_niu_data_rw.template_data[3].addr_array[7].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[7].id = NIU_ID_ECU_PHOP; // 定位精度

    g_niu_data_rw.template_data[3].addr_array[8].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[8].id = NIU_ID_ECU_HEADING; // 航向

    g_niu_data_rw.template_data[3].addr_array[9].base = NIU_DB;
    g_niu_data_rw.template_data[3].addr_array[9].id = NIU_ID_DB_DB_SPEED; // 速度

    g_niu_data_rw.template_data[3].addr_array[10].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[10].id = NIU_ID_ECU_TOTAL_MILEAGE; // 总行驶里程

    g_niu_data_rw.template_data[3].addr_array[11].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[11].id = NIU_ID_ECU_ECU_BATTARY; // 中控电池电量

    g_niu_data_rw.template_data[3].addr_array[12].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[12].id = NIU_ID_ECU_CSQ; // GSM信号强度

    g_niu_data_rw.template_data[3].addr_array[13].base = NIU_ECU;
    g_niu_data_rw.template_data[3].addr_array[13].id = NIU_ID_ECU_GPS_SINGAL; // GPS信号强度

    g_niu_data_rw.template_data[3].addr_array[14].base = NIU_BMS;
    g_niu_data_rw.template_data[3].addr_array[14].id = NIU_ID_BMS_BMS_SOC_RT; // SOC估算

    g_niu_data_rw.template_data[3].addr_array[15].base = NIU_BMS;
    g_niu_data_rw.template_data[3].addr_array[15].id = NIU_ID_BMS_BMS_TO_VLT_RT; // 总电压

    g_niu_data_rw.template_data[3].addr_array[16].base = NIU_FOC;
    g_niu_data_rw.template_data[3].addr_array[16].id = NIU_ID_FOC_FOC_COTLSTA; // FOC状态

    // 行驶无定位
    g_niu_data_rw.template_data[4].index = 4;                                                        //序列编号
    g_niu_data_rw.template_data[4].trigger0 = 0;                                                     //触发条件1
    g_niu_data_rw.template_data[4].trigger1 = 0;                                                     //BMS状态触发掩码
    g_niu_data_rw.template_data[4].trigger2 = 0;                                                     //FOC状态触发掩码
    g_niu_data_rw.template_data[4].trigger3 = NIU_TRIGGER_STATE_RUNNING_Y | NIU_TRIGGER_STATE_GPS_N; //ECU状态触发掩码
    g_niu_data_rw.template_data[4].trigger4 = 0;                                                     //ALARM状态触发掩码
    g_niu_data_rw.template_data[4].trigger5 = 0;                                                     //LCU状态触发掩码
    g_niu_data_rw.template_data[4].trigger6 = 0;                                                     //DB状态触发掩码
    g_niu_data_rw.template_data[4].trigger7 = 0;                                                     //保留位掩码位
    g_niu_data_rw.template_data[4].trigger8 = 0;                                                     //保留位掩码位
    g_niu_data_rw.template_data[4].trigger9 = 0;                                                     //保留位掩码位
    g_niu_data_rw.template_data[4].packed_name = 15043;                                              //包组合编号
    g_niu_data_rw.template_data[4].trigged_count = 0xFFFF;                                           //
    g_niu_data_rw.template_data[4].send_max = 65535;                                                 //使用次数
    g_niu_data_rw.template_data[4].interval = 20;                                                    //发送间隔
    g_niu_data_rw.template_data[4].cur_time = 0;                                                     //当前时间
    g_niu_data_rw.template_data[4].addr_num = 28;                                                    //地址序列长度

    g_niu_data_rw.template_data[4].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[4].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[4].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[4].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[4].addr_array[4].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[4].id = NIU_ID_ECU_CELL_MCC; // 移动国家号

    g_niu_data_rw.template_data[4].addr_array[5].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[5].id = NIU_ID_ECU_CELL_MNC; // 移动网络号码

    g_niu_data_rw.template_data[4].addr_array[6].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[6].id = NIU_ID_ECU_CELL_LAC_1; // 位置区码 

    g_niu_data_rw.template_data[4].addr_array[7].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[7].id = NIU_ID_ECU_CELL_CID_1; // 小区编号

    g_niu_data_rw.template_data[4].addr_array[8].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[8].id = NIU_ID_ECU_CELL_RSSI_1; // 信号强度

    g_niu_data_rw.template_data[4].addr_array[9].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[9].id = NIU_ID_ECU_CELL_LAC_2; // 位置区码 

    g_niu_data_rw.template_data[4].addr_array[10].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[10].id = NIU_ID_ECU_CELL_CID_2; // 小区编号

    g_niu_data_rw.template_data[4].addr_array[11].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[11].id = NIU_ID_ECU_CELL_RSSI_2; // 信号强度

    g_niu_data_rw.template_data[4].addr_array[12].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[12].id = NIU_ID_ECU_CELL_LAC_3; // 位置区码 

    g_niu_data_rw.template_data[4].addr_array[13].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[13].id = NIU_ID_ECU_CELL_CID_3; // 小区编号

    g_niu_data_rw.template_data[4].addr_array[14].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[14].id = NIU_ID_ECU_CELL_RSSI_3; // 信号强度

    g_niu_data_rw.template_data[4].addr_array[15].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[15].id = NIU_ID_ECU_CELL_LAC_4; // 位置区码 

    g_niu_data_rw.template_data[4].addr_array[16].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[16].id = NIU_ID_ECU_CELL_CID_4; // 小区编号

    g_niu_data_rw.template_data[4].addr_array[17].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[17].id = NIU_ID_ECU_CELL_RSSI_4; // 信号强度

    g_niu_data_rw.template_data[4].addr_array[18].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[18].id = NIU_ID_ECU_CELL_LAC_5; // 位置区码 

    g_niu_data_rw.template_data[4].addr_array[19].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[19].id = NIU_ID_ECU_CELL_CID_5; // 小区编号

    g_niu_data_rw.template_data[4].addr_array[20].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[20].id = NIU_ID_ECU_CELL_RSSI_5; // 信号强度

    g_niu_data_rw.template_data[4].addr_array[21].base = NIU_DB;
    g_niu_data_rw.template_data[4].addr_array[21].id = NIU_ID_DB_DB_SPEED; // 速度

    g_niu_data_rw.template_data[4].addr_array[22].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[22].id = NIU_ID_ECU_TOTAL_MILEAGE; // 总行驶里程

    g_niu_data_rw.template_data[4].addr_array[23].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[23].id = NIU_ID_ECU_CSQ; // GSM信号强度

    g_niu_data_rw.template_data[4].addr_array[24].base = NIU_ECU;
    g_niu_data_rw.template_data[4].addr_array[24].id = NIU_ID_ECU_GPS_SINGAL; // GPS信号强度

    g_niu_data_rw.template_data[4].addr_array[25].base = NIU_BMS;
    g_niu_data_rw.template_data[4].addr_array[25].id = NIU_ID_BMS_BMS_SOC_RT; // SOC估算

    g_niu_data_rw.template_data[4].addr_array[26].base = NIU_BMS;
    g_niu_data_rw.template_data[4].addr_array[26].id = NIU_ID_BMS_BMS_TO_VLT_RT; // 总电压

    g_niu_data_rw.template_data[4].addr_array[27].base = NIU_FOC;
    g_niu_data_rw.template_data[4].addr_array[27].id = NIU_ID_FOC_FOC_COTLSTA; // FOC状态

    // 停车日常
    g_niu_data_rw.template_data[5].index = 5;                                                            //序列编号
    g_niu_data_rw.template_data[5].trigger0 = 0;                                                         //触发条件1
    g_niu_data_rw.template_data[5].trigger1 = 0;                                                         //BMS状态触发掩码
    g_niu_data_rw.template_data[5].trigger2 = 0;                                                         //FOC状态触发掩码
    g_niu_data_rw.template_data[5].trigger3 = NIU_TRIGGER_STATE_RUNNING_N | NIU_TRIGGER_STATE_BATTERY_Y; //ECU状态触发掩码
    g_niu_data_rw.template_data[5].trigger4 = 0;                                                         //ALARM状态触发掩码
    g_niu_data_rw.template_data[5].trigger5 = 0;                                                         //LCU状态触发掩码
    g_niu_data_rw.template_data[5].trigger6 = 0;                                                         //DB状态触发掩码
    g_niu_data_rw.template_data[5].trigger7 = 0;                                                         //保留位掩码位
    g_niu_data_rw.template_data[5].trigger8 = 0;                                                         //保留位掩码位
    g_niu_data_rw.template_data[5].trigger9 = 0;                                                         //保留位掩码位
    g_niu_data_rw.template_data[5].packed_name = 15044;                                                  //包组合编号
    g_niu_data_rw.template_data[5].trigged_count = 0xFFFF;                                               //
    g_niu_data_rw.template_data[5].send_max = 65535;                                                     //使用次数
    g_niu_data_rw.template_data[5].interval = 1200;                                                      //发送间隔
    g_niu_data_rw.template_data[5].cur_time = 1536;                                                      //当前时间
    g_niu_data_rw.template_data[5].addr_num = 38;                                                        //地址序列长度

    g_niu_data_rw.template_data[5].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[5].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[5].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[5].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[5].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[5].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[5].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[5].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[5].addr_array[4].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[4].id = NIU_ID_BMS_BMS_TO_VLT_RT; // 总电压

    g_niu_data_rw.template_data[5].addr_array[5].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[5].id = NIU_ID_BMS_BMS_C_CUR_RT; // 充电电流

    g_niu_data_rw.template_data[5].addr_array[6].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[6].id = NIU_ID_BMS_BMS_DC_CUR_RT; // 放电电流

    g_niu_data_rw.template_data[5].addr_array[7].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[7].id = NIU_ID_BMS_BMS_SOC_RT; // SOC估算

    g_niu_data_rw.template_data[5].addr_array[8].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[8].id = NIU_ID_BMS_BMS_BAT_STA_RT; // 实时电池状态

    g_niu_data_rw.template_data[5].addr_array[9].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[9].id = NIU_ID_BMS_BMS_DC_FL_T_RT; // 充满电剩余时间

    g_niu_data_rw.template_data[5].addr_array[10].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[10].id = NIU_ID_BMS_BMS_TP1_RT; // 当前温度1

    g_niu_data_rw.template_data[5].addr_array[11].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[11].id = NIU_ID_BMS_BMS_TP2_RT; // 当前温度2

    g_niu_data_rw.template_data[5].addr_array[12].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[12].id = NIU_ID_BMS_BMS_TP3_RT; // 当前温度3

    g_niu_data_rw.template_data[5].addr_array[13].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[13].id = NIU_ID_BMS_BMS_TP4_RT; // 当前温度4

    g_niu_data_rw.template_data[5].addr_array[14].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[14].id = NIU_ID_BMS_BMS_TP5_RT; // 当前温度5

    g_niu_data_rw.template_data[5].addr_array[15].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[15].id = NIU_ID_BMS_BMS_G_VLT_RT1; // 电池组电压1

    g_niu_data_rw.template_data[5].addr_array[16].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[16].id = NIU_ID_BMS_BMS_G_VLT_RT2; // 电池组电压2

    g_niu_data_rw.template_data[5].addr_array[17].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[17].id = NIU_ID_BMS_BMS_G_VLT_RT3; // 电池组电压3

    g_niu_data_rw.template_data[5].addr_array[18].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[18].id = NIU_ID_BMS_BMS_G_VLT_RT4; // 电池组电压4

    g_niu_data_rw.template_data[5].addr_array[19].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[19].id = NIU_ID_BMS_BMS_G_VLT_RT5; // 电池组电压5

    g_niu_data_rw.template_data[5].addr_array[20].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[20].id = NIU_ID_BMS_BMS_G_VLT_RT6; // 电池组电压6

    g_niu_data_rw.template_data[5].addr_array[21].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[21].id = NIU_ID_BMS_BMS_G_VLT_RT7; // 电池组电压7

    g_niu_data_rw.template_data[5].addr_array[22].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[22].id = NIU_ID_BMS_BMS_G_VLT_RT8; // 电池组电压8

    g_niu_data_rw.template_data[5].addr_array[23].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[23].id = NIU_ID_BMS_BMS_G_VLT_RT9; // 电池组电压9

    g_niu_data_rw.template_data[5].addr_array[24].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[24].id = NIU_ID_BMS_BMS_G_VLT_RT10; // 电池组电压10

    g_niu_data_rw.template_data[5].addr_array[25].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[25].id = NIU_ID_BMS_BMS_G_VLT_RT11; // 电池组电压11

    g_niu_data_rw.template_data[5].addr_array[26].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[26].id = NIU_ID_BMS_BMS_G_VLT_RT12; // 电池组电压12

    g_niu_data_rw.template_data[5].addr_array[27].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[27].id = NIU_ID_BMS_BMS_G_VLT_RT13; // 电池组电压13

    g_niu_data_rw.template_data[5].addr_array[28].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[28].id = NIU_ID_BMS_BMS_G_VLT_RT14; // 电池组电压14

    g_niu_data_rw.template_data[5].addr_array[29].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[29].id = NIU_ID_BMS_BMS_G_VLT_RT15; // 电池组电压15

    g_niu_data_rw.template_data[5].addr_array[30].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[30].id = NIU_ID_BMS_BMS_G_VLT_RT16; // 电池组电压16

    g_niu_data_rw.template_data[5].addr_array[31].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[31].id = NIU_ID_BMS_BMS_G_VLT_RT17; // 电池组电压17

    g_niu_data_rw.template_data[5].addr_array[32].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[32].id = NIU_ID_BMS_BMS_G_VLT_RT18; // 电池组电压18

    g_niu_data_rw.template_data[5].addr_array[33].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[33].id = NIU_ID_BMS_BMS_G_VLT_RT19; // 电池组电压19

    g_niu_data_rw.template_data[5].addr_array[34].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[34].id = NIU_ID_BMS_BMS_G_VLT_RT20; // 电池组电压20

    g_niu_data_rw.template_data[5].addr_array[35].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[35].id = NIU_ID_BMS_BMS_C_CONT; // 电池循环次数

    g_niu_data_rw.template_data[5].addr_array[36].base = NIU_BMS;
    g_niu_data_rw.template_data[5].addr_array[36].id = NIU_ID_BMS_BMS_SN_ID; // 设备序列号

    g_niu_data_rw.template_data[5].addr_array[37].base = NIU_FOC;
    g_niu_data_rw.template_data[5].addr_array[37].id = NIU_ID_FOC_FOC_COTLSTA; // FOC状态

    // 停车定位
    g_niu_data_rw.template_data[6].index = 6;                                                            //序列编号
    g_niu_data_rw.template_data[6].trigger0 = 0;                                                         //触发条件1
    g_niu_data_rw.template_data[6].trigger1 = 0;                                                         //BMS状态触发掩码
    g_niu_data_rw.template_data[6].trigger2 = 0;                                                         //FOC状态触发掩码
    g_niu_data_rw.template_data[6].trigger3 = NIU_TRIGGER_STATE_RUNNING_N | NIU_TRIGGER_STATE_BATTERY_Y; //ECU状态触发掩码
    g_niu_data_rw.template_data[6].trigger4 = 0;                                                         //ALARM状态触发掩码
    g_niu_data_rw.template_data[6].trigger5 = 0;                                                         //LCU状态触发掩码
    g_niu_data_rw.template_data[6].trigger6 = 0;                                                         //DB状态触发掩码
    g_niu_data_rw.template_data[6].trigger7 = 0;                                                         //保留位掩码位
    g_niu_data_rw.template_data[6].trigger8 = 0;                                                         //保留位掩码位
    g_niu_data_rw.template_data[6].trigger9 = 0;                                                         //保留位掩码位
    g_niu_data_rw.template_data[6].packed_name = 15045;                                                  //包组合编号
    g_niu_data_rw.template_data[6].trigged_count = 0xFFFF;                                               //
    g_niu_data_rw.template_data[6].send_max = 65535;                                                     //使用次数
    g_niu_data_rw.template_data[6].interval = 300;                                                       //发送间隔
    g_niu_data_rw.template_data[6].cur_time = 0;                                                         //当前时间
    g_niu_data_rw.template_data[6].addr_num = 32;                                                        //地址序列长度

    g_niu_data_rw.template_data[6].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[6].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[6].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[6].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[6].addr_array[4].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[4].id = NIU_ID_ECU_LONGITUDE; // 经度

    g_niu_data_rw.template_data[6].addr_array[5].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[5].id = NIU_ID_ECU_LATITUDE; // 纬度

    g_niu_data_rw.template_data[6].addr_array[6].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[6].id = NIU_ID_ECU_HDOP; // 定位精度

    g_niu_data_rw.template_data[6].addr_array[7].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[7].id = NIU_ID_ECU_PHOP; // 定位精度

    g_niu_data_rw.template_data[6].addr_array[8].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[8].id = NIU_ID_ECU_HEADING; // 航向

    g_niu_data_rw.template_data[6].addr_array[9].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[9].id = NIU_ID_ECU_CELL_MCC; // 移动国家号

    g_niu_data_rw.template_data[6].addr_array[10].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[10].id = NIU_ID_ECU_CELL_MNC; // 移动网络号码

    g_niu_data_rw.template_data[6].addr_array[11].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[11].id = NIU_ID_ECU_CELL_LAC_1; // 位置区码 

    g_niu_data_rw.template_data[6].addr_array[12].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[12].id = NIU_ID_ECU_CELL_CID_1; // 小区编号

    g_niu_data_rw.template_data[6].addr_array[13].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[13].id = NIU_ID_ECU_CELL_RSSI_1; // 信号强度

    g_niu_data_rw.template_data[6].addr_array[14].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[14].id = NIU_ID_ECU_CELL_LAC_2; // 位置区码 

    g_niu_data_rw.template_data[6].addr_array[15].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[15].id = NIU_ID_ECU_CELL_CID_2; // 小区编号

    g_niu_data_rw.template_data[6].addr_array[16].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[16].id = NIU_ID_ECU_CELL_RSSI_2; // 信号强度

    g_niu_data_rw.template_data[6].addr_array[17].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[17].id = NIU_ID_ECU_CELL_LAC_3; // 位置区码 

    g_niu_data_rw.template_data[6].addr_array[18].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[18].id = NIU_ID_ECU_CELL_CID_3; // 小区编号

    g_niu_data_rw.template_data[6].addr_array[19].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[19].id = NIU_ID_ECU_CELL_RSSI_3; // 信号强度

    g_niu_data_rw.template_data[6].addr_array[20].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[20].id = NIU_ID_ECU_CELL_LAC_4; // 位置区码 

    g_niu_data_rw.template_data[6].addr_array[21].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[21].id = NIU_ID_ECU_CELL_CID_4; // 小区编号

    g_niu_data_rw.template_data[6].addr_array[22].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[22].id = NIU_ID_ECU_CELL_RSSI_4; // 信号强度

    g_niu_data_rw.template_data[6].addr_array[23].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[23].id = NIU_ID_ECU_CELL_LAC_5; // 位置区码 

    g_niu_data_rw.template_data[6].addr_array[24].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[24].id = NIU_ID_ECU_CELL_CID_5; // 小区编号

    g_niu_data_rw.template_data[6].addr_array[25].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[25].id = NIU_ID_ECU_CELL_RSSI_5; // 信号强度

    g_niu_data_rw.template_data[6].addr_array[26].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[26].id = NIU_ID_ECU_ECU_BATTARY; // 中控电池电量

    g_niu_data_rw.template_data[6].addr_array[27].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[27].id = NIU_ID_ECU_CSQ; // GSM信号强度

    g_niu_data_rw.template_data[6].addr_array[28].base = NIU_ECU;
    g_niu_data_rw.template_data[6].addr_array[28].id = NIU_ID_ECU_GPS_SINGAL; // GPS信号强度

    g_niu_data_rw.template_data[6].addr_array[29].base = NIU_BMS;
    g_niu_data_rw.template_data[6].addr_array[29].id = NIU_ID_BMS_BMS_SOC_RT; // SOC估算

    g_niu_data_rw.template_data[6].addr_array[30].base = NIU_BMS;
    g_niu_data_rw.template_data[6].addr_array[30].id = NIU_ID_BMS_BMS_TO_VLT_RT; // 总电压

    g_niu_data_rw.template_data[6].addr_array[31].base = NIU_FOC;
    g_niu_data_rw.template_data[6].addr_array[31].id = NIU_ID_FOC_FOC_COTLSTA; // FOC状态

    // 异动/触动事件 电池拔出事件 电门关闭事件
    g_niu_data_rw.template_data[7].index = 7;                                                                                                                                                                                                                                  //序列编号
    g_niu_data_rw.template_data[7].trigger0 = NIU_TRIGGER_EVENT_SHAKE | NIU_TRIGGER_EVENT_SHAKE_STOP | NIU_TRIGGER_EVENT_MOVE | NIU_TRIGGER_EVENT_MOVE_STOP | NIU_TRIGGER_EVENT_FALL | NIU_TRIGGER_EVENT_FALL_STOP | NIU_TRIGGER_EVENT_BATTER_OUT | NIU_TRIGGER_EVENT_ACC_OFF; //触发条件1
    g_niu_data_rw.template_data[7].trigger1 = 0;                                                                                                                                                                                                                               //BMS状态触发掩码
    g_niu_data_rw.template_data[7].trigger2 = 0;                                                                                                                                                                                                                               //FOC状态触发掩码
    g_niu_data_rw.template_data[7].trigger3 = 0;                                                                                                                                                                                                                               //ECU状态触发掩码
    g_niu_data_rw.template_data[7].trigger4 = NIU_TRIGGER_ALARM_ALERT | NIU_TRIGGER_ALARM_LOCK | NIU_TRIGGER_ALARM_FIND | NIU_TRIGGER_ALARM_ACC | NIU_TRIGGER_ALARM_AUTO_LOCK | NIU_TRIGGER_ALARM_EVENT_CHANGE;                                                                //ALARM状态触发掩码
    g_niu_data_rw.template_data[7].trigger5 = NIU_TRIGGER_EVENT_MOVE_GSENSOR | NIU_TRIGGER_EVENT_MOVE_GSENSOR_STOP;                                                                                                                                                            //LCU状态触发掩码
    g_niu_data_rw.template_data[7].trigger6 = 0;                                                                                                                                                                                                                               //DB状态触发掩码
    g_niu_data_rw.template_data[7].trigger7 = 0;                                                                                                                                                                                                                               //保留位掩码位
    g_niu_data_rw.template_data[7].trigger8 = 0;                                                                                                                                                                                                                               //保留位掩码位
    g_niu_data_rw.template_data[7].trigger9 = 0;                                                                                                                                                                                                                               //保留位掩码位
    g_niu_data_rw.template_data[7].packed_name = 15039;                                                                                                                                                                                                                        //包组合编号
    g_niu_data_rw.template_data[7].trigged_count = 0xFFFF;                                                                                                                                                                                                                     //
    g_niu_data_rw.template_data[7].send_max = 1;                                                                                                                                                                                                                               //使用次数
    g_niu_data_rw.template_data[7].interval = 1;                                                                                                                                                                                                                               //发送间隔
    g_niu_data_rw.template_data[7].cur_time = 1;                                                                                                                                                                                                                               //当前时间
    g_niu_data_rw.template_data[7].addr_num = 21;                                                                                                                                                                                                                              //地址序列长度

    g_niu_data_rw.template_data[7].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[7].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[7].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[7].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[7].addr_array[4].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[4].id = NIU_ID_ECU_LONGITUDE; // 经度

    g_niu_data_rw.template_data[7].addr_array[5].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[5].id = NIU_ID_ECU_LATITUDE; // 纬度

    g_niu_data_rw.template_data[7].addr_array[6].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[6].id = NIU_ID_ECU_HDOP; // 定位精度

    g_niu_data_rw.template_data[7].addr_array[7].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[7].id = NIU_ID_ECU_PHOP; // 定位精度

    g_niu_data_rw.template_data[7].addr_array[8].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[8].id = NIU_ID_ECU_HEADING; // 航向

    g_niu_data_rw.template_data[7].addr_array[9].base = NIU_DB;
    g_niu_data_rw.template_data[7].addr_array[9].id = NIU_ID_DB_DB_SPEED; // 速度

    g_niu_data_rw.template_data[7].addr_array[10].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[10].id = NIU_ID_ECU_TOTAL_MILEAGE; // 总行驶里程

    g_niu_data_rw.template_data[7].addr_array[11].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[11].id = NIU_ID_ECU_ECU_BATTARY; // 中控电池电量

    g_niu_data_rw.template_data[7].addr_array[12].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[12].id = NIU_ID_ECU_CSQ; // GSM信号强度

    g_niu_data_rw.template_data[7].addr_array[13].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[13].id = NIU_ID_ECU_GPS_SINGAL; // GPS信号强度

    g_niu_data_rw.template_data[7].addr_array[14].base = NIU_ALARM;
    g_niu_data_rw.template_data[7].addr_array[14].id = NIU_ID_ALARM_ALARM_STATE; // 报警器状态

    g_niu_data_rw.template_data[7].addr_array[15].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[15].id = NIU_ID_ECU_STOP_LONGITUDE; // 停车时经度

    g_niu_data_rw.template_data[7].addr_array[16].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[16].id = NIU_ID_ECU_STOP_LATITUDE; // 停车时纬度

    g_niu_data_rw.template_data[7].addr_array[17].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[17].id = NIU_ID_ECU_ECU_AVG_LONGITUDE; // 电门关闭时GPS平均经度

    g_niu_data_rw.template_data[7].addr_array[18].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[18].id = NIU_ID_ECU_ECU_AVG_LATITUDE; // 电门关闭时GPS平均纬度

    g_niu_data_rw.template_data[7].addr_array[19].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[19].id = NIU_ID_ECU_ECU_REALTIME_LONGITUDE; // 实时经度

    g_niu_data_rw.template_data[7].addr_array[20].base = NIU_ECU;
    g_niu_data_rw.template_data[7].addr_array[20].id = NIU_ID_ECU_ECU_REALTIME_LATITUDE; // 实时纬度

    // 异常事件
    g_niu_data_rw.template_data[8].index = 8;                                                                                                                                                                                                //序列编号
    g_niu_data_rw.template_data[8].trigger0 = NIU_TRIGGER_STATE_FOC_DISCONNECT | NIU_TRIGGER_STATE_BMS_DISCONNECT | NIU_TRIGGER_STATE_DB_DISCONNECT | NIU_TRIGGER_STATE_LCU_DISCONNECT;                                                      //触发条件1
    g_niu_data_rw.template_data[8].trigger1 = BMS_INCHARGE_PTK | BMS_BAT_LOW | BMS_INCHARGE_OVER_CUR | BMS_DISCHARGE_OVER_CUR | BMS_BAT_TEMP_HIGH | BMS_BAT_TEMP_LOW | BMS_BAR_OTHER_ERR | BMS_BAR_DUANLU | BMS_BAR_WATER | BMS_BAR_MOS_ERR; //BMS状态触发掩码
    g_niu_data_rw.template_data[8].trigger2 = DRIVER_MOS_ERR | DRIVER_OVERCUR | DRIVER_PHASE_ERR | DRIVER_HALL_ERR | DRIVER_STALL | DRIVER_POWER_ERR | DRIVER_OVERVOT | DRIVER_LOWVOT | DRIVER_OVERTEMP | DRIVER_HANDLE_ERR;                 //FOC状态触发掩码
    g_niu_data_rw.template_data[8].trigger3 = 0;                                                                                                                                                                                             //ECU状态触发掩码
    g_niu_data_rw.template_data[8].trigger4 = 0;                                                                                                                                                                                             //ALARM状态触发掩码
    g_niu_data_rw.template_data[8].trigger5 = 0;                                                                                                                                                                                             //LCU状态触发掩码
    g_niu_data_rw.template_data[8].trigger6 = 0;                                                                                                                                                                                             //DB状态触发掩码
    g_niu_data_rw.template_data[8].trigger7 = 0;                                                                                                                                                                                             //保留位掩码位
    g_niu_data_rw.template_data[8].trigger8 = 0;                                                                                                                                                                                             //保留位掩码位
    g_niu_data_rw.template_data[8].trigger9 = 0;                                                                                                                                                                                             //保留位掩码位
    g_niu_data_rw.template_data[8].packed_name = 15028;                                                                                                                                                                                      //包组合编号
    g_niu_data_rw.template_data[8].trigged_count = 0xFFFF;                                                                                                                                                                                   //
    g_niu_data_rw.template_data[8].send_max = 65535;                                                                                                                                                                                         //使用次数
    g_niu_data_rw.template_data[8].interval = 20;                                                                                                                                                                                            //发送间隔
    g_niu_data_rw.template_data[8].cur_time = 1;                                                                                                                                                                                             //当前时间
    g_niu_data_rw.template_data[8].addr_num = 24;                                                                                                                                                                                            //地址序列长度

    g_niu_data_rw.template_data[8].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[8].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[8].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[8].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[8].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[8].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[8].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[8].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[8].addr_array[4].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[4].id = NIU_ID_BMS_BMS_TO_VLT_RT; // 总电压

    g_niu_data_rw.template_data[8].addr_array[5].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[5].id = NIU_ID_BMS_BMS_C_CUR_RT; // 充电电流

    g_niu_data_rw.template_data[8].addr_array[6].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[6].id = NIU_ID_BMS_BMS_DC_CUR_RT; // 放电电流

    g_niu_data_rw.template_data[8].addr_array[7].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[7].id = NIU_ID_BMS_BMS_SOC_RT; // SOC估算

    g_niu_data_rw.template_data[8].addr_array[8].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[8].id = NIU_ID_BMS_BMS_BAT_STA_RT; // 实时电池状态

    g_niu_data_rw.template_data[8].addr_array[9].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[9].id = NIU_ID_BMS_BMS_DC_FL_T_RT; // 充满电剩余时间

    g_niu_data_rw.template_data[8].addr_array[10].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[10].id = NIU_ID_BMS_BMS_TP1_RT; // 当前温度1

    g_niu_data_rw.template_data[8].addr_array[11].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[11].id = NIU_ID_BMS_BMS_TP2_RT; // 当前温度2

    g_niu_data_rw.template_data[8].addr_array[12].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[12].id = NIU_ID_BMS_BMS_TP3_RT; // 当前温度3

    g_niu_data_rw.template_data[8].addr_array[13].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[13].id = NIU_ID_BMS_BMS_TP4_RT; // 当前温度4

    g_niu_data_rw.template_data[8].addr_array[14].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[14].id = NIU_ID_BMS_BMS_TP5_RT; // 当前温度5

    g_niu_data_rw.template_data[8].addr_array[15].base = NIU_FOC;
    g_niu_data_rw.template_data[8].addr_array[15].id = NIU_ID_FOC_FOC_GEARS; // 档位

    g_niu_data_rw.template_data[8].addr_array[16].base = NIU_FOC;
    g_niu_data_rw.template_data[8].addr_array[16].id = NIU_ID_FOC_FOC_RATEDCUR; // 额定电流

    g_niu_data_rw.template_data[8].addr_array[17].base = NIU_FOC;
    g_niu_data_rw.template_data[8].addr_array[17].id = NIU_ID_FOC_FOC_BARCUR; // 母线电流

    g_niu_data_rw.template_data[8].addr_array[18].base = NIU_FOC;
    g_niu_data_rw.template_data[8].addr_array[18].id = NIU_ID_FOC_FOC_MOTORSPEED; // 电机转速

    g_niu_data_rw.template_data[8].addr_array[19].base = NIU_FOC;
    g_niu_data_rw.template_data[8].addr_array[19].id = NIU_ID_FOC_FOC_COTLSTA; // FOC状态

    g_niu_data_rw.template_data[8].addr_array[20].base = NIU_ECU;
    g_niu_data_rw.template_data[8].addr_array[20].id = NIU_ID_ECU_ECU_BATTARY; // 中控电池电量

    g_niu_data_rw.template_data[8].addr_array[21].base = NIU_ECU;
    g_niu_data_rw.template_data[8].addr_array[21].id = NIU_ID_ECU_CSQ; // GSM信号强度

    g_niu_data_rw.template_data[8].addr_array[22].base = NIU_ECU;
    g_niu_data_rw.template_data[8].addr_array[22].id = NIU_ID_ECU_GPS_SINGAL; // GPS信号强度

    g_niu_data_rw.template_data[8].addr_array[23].base = NIU_BMS;
    g_niu_data_rw.template_data[8].addr_array[23].id = NIU_ID_BMS_BMS_SN_ID;                                                 // 设备序列号
                                                                                                                         // 升级相关
    g_niu_data_rw.template_data[9].index = 9;                                                                            //序列编号
    g_niu_data_rw.template_data[9].trigger0 = 0;                                                                         //触发条件1
    g_niu_data_rw.template_data[9].trigger1 = 0;                                                                         //BMS状态触发掩码
    g_niu_data_rw.template_data[9].trigger2 = 0;                                                                         //FOC状态触发掩码
    g_niu_data_rw.template_data[9].trigger3 = 0;                                                                         //ECU状态触发掩码
    g_niu_data_rw.template_data[9].trigger4 = 0;                                                                         //ALARM状态触发掩码
    g_niu_data_rw.template_data[9].trigger5 = 0;                                                                         //LCU状态触发掩码
    g_niu_data_rw.template_data[9].trigger6 = 0;                                                                         //DB状态触发掩码
    g_niu_data_rw.template_data[9].trigger7 = NIU_TRIGGER_OTA_READY | NIU_TRIGGER_FOTA_READY | NIU_TRIGGER_FOTA_WORKING; //保留位掩码位
    g_niu_data_rw.template_data[9].trigger8 = 0;                                                                         //保留位掩码位
    g_niu_data_rw.template_data[9].trigger9 = 0;                                                                         //保留位掩码位
    g_niu_data_rw.template_data[9].packed_name = 15029;                                                                  //包组合编号
    g_niu_data_rw.template_data[9].trigged_count = 0xFFFF;                                                               //
    g_niu_data_rw.template_data[9].send_max = 65535;                                                                     //使用次数
    g_niu_data_rw.template_data[9].interval = 20;                                                                        //发送间隔
    g_niu_data_rw.template_data[9].cur_time = 2;                                                                         //当前时间
    g_niu_data_rw.template_data[9].addr_num = 9;                                                                         //地址序列长度

    g_niu_data_rw.template_data[9].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[9].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[9].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[9].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[9].addr_array[4].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[4].id = NIU_ID_ECU_ECU_BATTARY; // 中控电池电量

    g_niu_data_rw.template_data[9].addr_array[5].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[5].id = NIU_ID_ECU_CSQ; // GSM信号强度

    g_niu_data_rw.template_data[9].addr_array[6].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[6].id = NIU_ID_ECU_GPS_SINGAL; // GPS信号强度

    g_niu_data_rw.template_data[9].addr_array[7].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[7].id = NIU_ID_ECU_ECU_CFG; // 中控配置

    g_niu_data_rw.template_data[9].addr_array[8].base = NIU_ECU;
    g_niu_data_rw.template_data[9].addr_array[8].id = NIU_ID_ECU_FOTA_DOWNLOAD_PERCENT; // FOTA进度

    // 电门关闭事件
    g_niu_data_rw.template_data[10].index = 10;                         //序列编号
    g_niu_data_rw.template_data[10].trigger0 = 0;                       //触发条件1
    g_niu_data_rw.template_data[10].trigger1 = 0;                       //BMS状态触发掩码
    g_niu_data_rw.template_data[10].trigger2 = 0;                       //FOC状态触发掩码
    g_niu_data_rw.template_data[10].trigger3 = NIU_TRIGGER_STATE_ACC_N; //ECU状态触发掩码
    g_niu_data_rw.template_data[10].trigger4 = 0;                       //ALARM状态触发掩码
    g_niu_data_rw.template_data[10].trigger5 = 0;                       //LCU状态触发掩码
    g_niu_data_rw.template_data[10].trigger6 = 0;                       //DB状态触发掩码
    g_niu_data_rw.template_data[10].trigger7 = 0;                       //保留位掩码位
    g_niu_data_rw.template_data[10].trigger8 = 0;                       //保留位掩码位
    g_niu_data_rw.template_data[10].trigger9 = 0;                       //保留位掩码位
    g_niu_data_rw.template_data[10].packed_name = 15037;                //包组合编号
    g_niu_data_rw.template_data[10].trigged_count = 0xFFFF;             //
    g_niu_data_rw.template_data[10].send_max = 36;                      //使用次数
    g_niu_data_rw.template_data[10].interval = 5;                       //发送间隔
    g_niu_data_rw.template_data[10].cur_time = 0;                       //当前时间
    g_niu_data_rw.template_data[10].addr_num = 21;                      //地址序列长度

    g_niu_data_rw.template_data[10].addr_array[0].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[0].id = NIU_ID_ECU_ECU_STATE; // 中控状态

    g_niu_data_rw.template_data[10].addr_array[1].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[1].id = NIU_ID_ECU_CAR_STA; // 机车状态

    g_niu_data_rw.template_data[10].addr_array[2].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[2].id = NIU_ID_ECU_TIMESTAMP; // 时间戳

    g_niu_data_rw.template_data[10].addr_array[3].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[3].id = NIU_ID_ECU_ECU_STATE2; // 中控状态2

    g_niu_data_rw.template_data[10].addr_array[4].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[4].id = NIU_ID_ECU_LONGITUDE; // 经度

    g_niu_data_rw.template_data[10].addr_array[5].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[5].id = NIU_ID_ECU_LATITUDE; // 纬度

    g_niu_data_rw.template_data[10].addr_array[6].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[6].id = NIU_ID_ECU_HDOP; // 定位精度

    g_niu_data_rw.template_data[10].addr_array[7].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[7].id = NIU_ID_ECU_PHOP; // 定位精度

    g_niu_data_rw.template_data[10].addr_array[8].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[8].id = NIU_ID_ECU_HEADING; // 航向

    g_niu_data_rw.template_data[10].addr_array[9].base = NIU_DB;
    g_niu_data_rw.template_data[10].addr_array[9].id = NIU_ID_DB_DB_SPEED; // 速度

    g_niu_data_rw.template_data[10].addr_array[10].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[10].id = NIU_ID_ECU_TOTAL_MILEAGE; // 总行驶里程

    g_niu_data_rw.template_data[10].addr_array[11].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[11].id = NIU_ID_ECU_ECU_BATTARY; // 中控电池电量

    g_niu_data_rw.template_data[10].addr_array[12].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[12].id = NIU_ID_ECU_CSQ; // GSM信号强度

    g_niu_data_rw.template_data[10].addr_array[13].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[13].id = NIU_ID_ECU_GPS_SINGAL; // GPS信号强度

    g_niu_data_rw.template_data[10].addr_array[14].base = NIU_ALARM;
    g_niu_data_rw.template_data[10].addr_array[14].id = NIU_ID_ALARM_ALARM_STATE; // 报警器状态

    g_niu_data_rw.template_data[10].addr_array[15].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[15].id = NIU_ID_ECU_STOP_LONGITUDE; // 停车时经度

    g_niu_data_rw.template_data[10].addr_array[16].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[16].id = NIU_ID_ECU_STOP_LATITUDE; // 停车时纬度

    g_niu_data_rw.template_data[10].addr_array[17].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[17].id = NIU_ID_ECU_ECU_AVG_LONGITUDE; // 电门关闭时GPS平均经度

    g_niu_data_rw.template_data[10].addr_array[18].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[18].id = NIU_ID_ECU_ECU_AVG_LATITUDE; // 电门关闭时GPS平均纬度

    g_niu_data_rw.template_data[10].addr_array[19].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[19].id = NIU_ID_ECU_ECU_REALTIME_LONGITUDE; // 实时经度

    g_niu_data_rw.template_data[10].addr_array[20].base = NIU_ECU;
    g_niu_data_rw.template_data[10].addr_array[20].id = NIU_ID_ECU_ECU_REALTIME_LATITUDE; // 实时纬度
}

/**
 * g_niu_data_rw flush to disk 
 * 
 * 
 * 
 * 
 * 
 */
JSTATIC JVOID niu_data_rw_flush(int tmr_id, void *param)
{
    FILE *fp = NULL;
    NUINT32 write_size = 0;
    NUINT64 cur_time_stamp = 0;

    if (JFALSE == g_niu_imei_ok)
    {
        return;
    }

    if(tmr_id != TIMER_ID_NIU_UPDATE_RW_DATA)
    {
        LOG_E("niu_data_rw_flush tmr_id error.");
        return;
    }

    cur_time_stamp = niu_device_get_timestamp();
    //niu_trace_dump_data("flush rw_path", (NUINT8 *)g_niu_rw_file_path, RW_FILE_PATH_LEN_MAX);

    if ((cur_time_stamp > (g_niu_rw_flush_timstamp + 5)) || (cur_time_stamp < g_niu_rw_flush_timstamp))
    {
        fp = fopen(g_niu_rw_file_path, "wb");
        if (fp)
        {
            write_size = fwrite(&g_niu_data_rw, 1, sizeof(g_niu_data_rw), fp);
            if(write_size != sizeof(g_niu_data_rw))
            {
                LOG_E("niu_data_rw flush error.");
            }
            fclose(fp);
        }
        //LOG_D("write_size:%d,sizeof(g_niu_data_rw):%d", write_size, sizeof(g_niu_data_rw));
        // LOG_D("file_handle:%x, write_size:%d", file_handle, write_size);
        g_niu_rw_flush_timstamp = niu_device_get_timestamp();
    }
    else
    {
        tmr_handler_tmr_add(TIMER_ID_NIU_UPDATE_RW_DATA, niu_data_rw_flush, NULL, 2000, 0);
    }
}

/**
 * 
 * 1. read imei
 * 2. ret = read ($(IMEI)_rw.dat, 'r')
 * 3. sizeof(struct) , readsize, filesize compare
 * 4. template_version compare
 * 5. set uploading_template_N = template_data[N].packed_name
 */
JVOID niu_data_rw_init()
{
    int ret;
    NCHAR imei[20] = {0};
    FILE *fp = NULL;
    NUINT32 file_size = 0;
    NUINT32 read_size = 0;
    struct stat st;

    LOG_D("niu_data_rw_init");
    cmd_imei_info_get(imei, 19);

    if (strlen(imei))
    {
        g_niu_imei_ok = JTRUE;
    }

    LOG_D("imei_ok:%d,imei:%s", g_niu_imei_ok, imei);

    if (g_niu_imei_ok)
    {
        snprintf(g_niu_rw_file_path, RW_FILE_PATH_LEN_MAX, "%s/%s_rw.dat", RW_FILE_PREFIX, imei);

        fp = fopen(g_niu_rw_file_path, "r");
        if (fp)
        {
            ret = stat(g_niu_rw_file_path, &st);
            if (ret)
            {
                file_size = 0;
            }
            file_size = st.st_size;
            if (file_size == sizeof(NIU_DATA_CONFIG_RW))
            {
                ret = fread(&g_niu_data_rw, 1, file_size, fp);
                if (ret != file_size)
                {
                    LOG_E("fread error.[file_size %u, ret %u]", file_size, ret);
                }
                else
                {
                    read_size = ret;
                }
            }
            fclose(fp);
        }
    }
    LOG_D(" file_size:%d, read_size:%d, sizeof(NIU_DATA_CONFIG_RW):%d", file_size, read_size, sizeof(NIU_DATA_CONFIG_RW));

    if (file_size != sizeof(NIU_DATA_CONFIG_RW) || read_size != sizeof(NIU_DATA_CONFIG_RW))
    {
        if (g_niu_imei_ok)
        {
            remove(g_niu_rw_file_path);
        }

        LOG_D("load rw_data from file error. use default data to initialize.");
        niu_data_rw_default();
    }

    NUINT8 i = 0;

    for (i = 0; i < NIU_SERVER_UPL_CMD_NUM; i++)
    {
        //LOG_D("i:%d, index:%d", i, g_niu_data_rw.template_data[i].index);
        if (g_niu_data_rw.template_data[i].index != i)
        {
            niu_data_rw_default();
            break;
        }
    }

    LOG_D("template_version:%d, NIU_DATA_TEMPLATE_VER:%d", g_niu_data_rw.template_version, NIU_DATA_TEMPLATE_VER);
    if (g_niu_data_rw.template_version != NIU_DATA_TEMPLATE_VER)
    {
        niu_data_rw_default();
    }
    g_niu_data_rw.uploading_template_1 = g_niu_data_rw.template_data[0].packed_name;
    g_niu_data_rw.uploading_template_2 = g_niu_data_rw.template_data[1].packed_name;
    g_niu_data_rw.uploading_template_3 = g_niu_data_rw.template_data[2].packed_name;
    g_niu_data_rw.uploading_template_4 = g_niu_data_rw.template_data[3].packed_name;
    g_niu_data_rw.uploading_template_5 = g_niu_data_rw.template_data[4].packed_name;
    g_niu_data_rw.uploading_template_6 = g_niu_data_rw.template_data[5].packed_name;
    g_niu_data_rw.uploading_template_7 = g_niu_data_rw.template_data[6].packed_name;
    g_niu_data_rw.uploading_template_8 = g_niu_data_rw.template_data[7].packed_name;
    g_niu_data_rw.uploading_template_9 = g_niu_data_rw.template_data[8].packed_name;
    g_niu_data_rw.uploading_template_10 = g_niu_data_rw.template_data[9].packed_name;
    g_niu_data_rw.uploading_template_11 = g_niu_data_rw.template_data[10].packed_name;
    g_niu_data_rw.uploading_template_12 = g_niu_data_rw.template_data[11].packed_name;
    g_niu_data_rw.uploading_template_13 = g_niu_data_rw.template_data[12].packed_name;
    g_niu_data_rw.uploading_template_14 = g_niu_data_rw.template_data[13].packed_name;
    g_niu_data_rw.uploading_template_15 = g_niu_data_rw.template_data[14].packed_name;
    g_niu_data_rw.uploading_template_16 = g_niu_data_rw.template_data[15].packed_name;
    g_niu_data_rw.uploading_template_17 = g_niu_data_rw.template_data[16].packed_name;
    g_niu_data_rw.uploading_template_18 = g_niu_data_rw.template_data[17].packed_name;
    g_niu_data_rw.uploading_template_19 = g_niu_data_rw.template_data[18].packed_name;
    g_niu_data_rw.uploading_template_20 = g_niu_data_rw.template_data[19].packed_name;
    g_niu_data_rw.uploading_template_21 = g_niu_data_rw.template_data[20].packed_name;
    g_niu_data_rw.uploading_template_22 = g_niu_data_rw.template_data[21].packed_name;
    g_niu_data_rw.uploading_template_23 = g_niu_data_rw.template_data[22].packed_name;
    g_niu_data_rw.uploading_template_24 = g_niu_data_rw.template_data[23].packed_name;
    g_niu_data_rw.uploading_template_25 = g_niu_data_rw.template_data[24].packed_name;
    g_niu_data_rw.uploading_template_26 = g_niu_data_rw.template_data[25].packed_name;
    g_niu_data_rw.uploading_template_27 = g_niu_data_rw.template_data[26].packed_name;
    g_niu_data_rw.uploading_template_28 = g_niu_data_rw.template_data[27].packed_name;
    g_niu_data_rw.uploading_template_29 = g_niu_data_rw.template_data[28].packed_name;
    g_niu_data_rw.uploading_template_30 = g_niu_data_rw.template_data[29].packed_name;
    g_niu_data_rw.uploading_template_31 = g_niu_data_rw.template_data[30].packed_name;
    g_niu_data_rw.uploading_template_32 = g_niu_data_rw.template_data[31].packed_name;
    g_niu_data_rw.uploading_template_33 = g_niu_data_rw.template_data[32].packed_name;
    g_niu_data_rw.uploading_template_34 = g_niu_data_rw.template_data[33].packed_name;
    g_niu_data_rw.uploading_template_35 = g_niu_data_rw.template_data[34].packed_name;
    g_niu_data_rw.uploading_template_36 = g_niu_data_rw.template_data[35].packed_name;
    g_niu_data_rw.uploading_template_37 = g_niu_data_rw.template_data[36].packed_name;
    g_niu_data_rw.uploading_template_38 = g_niu_data_rw.template_data[37].packed_name;
    g_niu_data_rw.uploading_template_39 = g_niu_data_rw.template_data[38].packed_name;
    g_niu_data_rw.uploading_template_40 = g_niu_data_rw.template_data[39].packed_name;
    g_niu_data_rw.uploading_template_41 = g_niu_data_rw.template_data[40].packed_name;
    g_niu_data_rw.uploading_template_42 = g_niu_data_rw.template_data[41].packed_name;
    g_niu_data_rw.uploading_template_43 = g_niu_data_rw.template_data[42].packed_name;
    g_niu_data_rw.uploading_template_44 = g_niu_data_rw.template_data[43].packed_name;
    g_niu_data_rw.uploading_template_45 = g_niu_data_rw.template_data[44].packed_name;
    g_niu_data_rw.uploading_template_46 = g_niu_data_rw.template_data[45].packed_name;
    g_niu_data_rw.uploading_template_47 = g_niu_data_rw.template_data[46].packed_name;
    g_niu_data_rw.uploading_template_48 = g_niu_data_rw.template_data[47].packed_name;
    g_niu_data_rw.uploading_template_49 = g_niu_data_rw.template_data[48].packed_name;
    g_niu_data_rw.uploading_template_50 = g_niu_data_rw.template_data[49].packed_name;
}

/**
 * read value from g_config_nw, and write to addr
 * index is used to mark which field to read
 */
JBOOL niu_data_rw_read(NIU_DATA_RW_INDEX index, NUINT8 *addr, NUINT32 len)
{
    JBOOL ret = JFALSE;

    //LOG_D("index:%d, addr:%p, len:%d", index, addr, len);

    if (addr && len)
    {
        switch (index)
        {
        case NIU_DATA_RW_TEMPLATE_1:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_1))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_1, sizeof(g_niu_data_rw.uploading_template_1));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_2:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_2))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_2, sizeof(g_niu_data_rw.uploading_template_2));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_3:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_3))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_3, sizeof(g_niu_data_rw.uploading_template_3));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_4:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_4))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_4, sizeof(g_niu_data_rw.uploading_template_4));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_5:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_5))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_5, sizeof(g_niu_data_rw.uploading_template_5));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_6:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_6))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_6, sizeof(g_niu_data_rw.uploading_template_6));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_7:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_7))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_7, sizeof(g_niu_data_rw.uploading_template_7));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_8:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_8))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_8, sizeof(g_niu_data_rw.uploading_template_8));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_9:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_9))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_9, sizeof(g_niu_data_rw.uploading_template_9));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_10:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_10))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_10, sizeof(g_niu_data_rw.uploading_template_10));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_11:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_11))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_11, sizeof(g_niu_data_rw.uploading_template_11));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_12:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_12))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_12, sizeof(g_niu_data_rw.uploading_template_12));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_13:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_13))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_13, sizeof(g_niu_data_rw.uploading_template_13));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_14:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_14))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_14, sizeof(g_niu_data_rw.uploading_template_14));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_15:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_15))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_15, sizeof(g_niu_data_rw.uploading_template_15));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_16:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_16))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_16, sizeof(g_niu_data_rw.uploading_template_16));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_17:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_17))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_17, sizeof(g_niu_data_rw.uploading_template_17));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_18:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_18))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_18, sizeof(g_niu_data_rw.uploading_template_18));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_19:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_19))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_19, sizeof(g_niu_data_rw.uploading_template_19));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_20:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_20))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_20, sizeof(g_niu_data_rw.uploading_template_20));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_21:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_21))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_21, sizeof(g_niu_data_rw.uploading_template_21));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_22:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_22))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_22, sizeof(g_niu_data_rw.uploading_template_22));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_23:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_23))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_23, sizeof(g_niu_data_rw.uploading_template_23));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_24:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_24))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_24, sizeof(g_niu_data_rw.uploading_template_24));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_25:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_25))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_25, sizeof(g_niu_data_rw.uploading_template_25));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_26:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_26))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_26, sizeof(g_niu_data_rw.uploading_template_26));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_27:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_27))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_27, sizeof(g_niu_data_rw.uploading_template_27));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_28:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_28))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_28, sizeof(g_niu_data_rw.uploading_template_28));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_29:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_29))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_29, sizeof(g_niu_data_rw.uploading_template_29));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_30:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_30))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_30, sizeof(g_niu_data_rw.uploading_template_30));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_31:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_31))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_31, sizeof(g_niu_data_rw.uploading_template_31));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_32:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_32))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_32, sizeof(g_niu_data_rw.uploading_template_32));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_33:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_33))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_33, sizeof(g_niu_data_rw.uploading_template_33));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_34:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_34))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_34, sizeof(g_niu_data_rw.uploading_template_34));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_35:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_35))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_35, sizeof(g_niu_data_rw.uploading_template_35));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_36:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_36))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_36, sizeof(g_niu_data_rw.uploading_template_36));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_37:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_37))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_37, sizeof(g_niu_data_rw.uploading_template_37));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_38:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_38))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_38, sizeof(g_niu_data_rw.uploading_template_38));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_39:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_39))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_39, sizeof(g_niu_data_rw.uploading_template_39));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_40:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_40))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_40, sizeof(g_niu_data_rw.uploading_template_40));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_41:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_41))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_41, sizeof(g_niu_data_rw.uploading_template_41));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_42:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_42))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_42, sizeof(g_niu_data_rw.uploading_template_42));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_43:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_43))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_43, sizeof(g_niu_data_rw.uploading_template_43));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_44:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_44))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_44, sizeof(g_niu_data_rw.uploading_template_44));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_45:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_45))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_45, sizeof(g_niu_data_rw.uploading_template_45));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_46:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_46))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_46, sizeof(g_niu_data_rw.uploading_template_46));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_47:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_47))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_47, sizeof(g_niu_data_rw.uploading_template_47));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_48:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_48))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_48, sizeof(g_niu_data_rw.uploading_template_48));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_49:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_49))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_49, sizeof(g_niu_data_rw.uploading_template_49));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_50:
        {
            if (addr && len >= sizeof(g_niu_data_rw.uploading_template_50))
            {
                memcpy(addr, &g_niu_data_rw.uploading_template_50, sizeof(g_niu_data_rw.uploading_template_50));
                ret = JTRUE;
            }
        }
        break;
        default:
            break;
        }
    }
    //LOG_D("index:%d, addr:%p, len:%d, ret:%d", index, addr, len, ret);
    return ret;
}

/**
 * write value to g_config_rw
 * @addr: the value of addr
 * @len: write len
 * 
 */
JBOOL niu_data_rw_write(NIU_DATA_RW_INDEX index, NUINT8 *addr, NUINT32 len)
{
    JBOOL ret = JFALSE;

    // if (addr && len)
    // {
    //     NCHAR prompt[20] = {0};
    //     snprintf(prompt, 20, "rw_data[%d]", index);

        //niu_trace_dump_data(prompt, addr, len);
    //}

    if (addr && len)
    {
        switch (index)
        {
        case NIU_DATA_RW_TEMPLATE_1:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_1))
            {
                memcpy(&g_niu_data_rw.uploading_template_1, addr, sizeof(g_niu_data_rw.uploading_template_1));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_2:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_2))
            {
                memcpy(&g_niu_data_rw.uploading_template_2, addr, sizeof(g_niu_data_rw.uploading_template_2));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_3:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_3))
            {
                memcpy(&g_niu_data_rw.uploading_template_3, addr, sizeof(g_niu_data_rw.uploading_template_3));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_4:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_4))
            {
                memcpy(&g_niu_data_rw.uploading_template_4, addr, sizeof(g_niu_data_rw.uploading_template_4));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_5:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_5))
            {
                memcpy(&g_niu_data_rw.uploading_template_5, addr, sizeof(g_niu_data_rw.uploading_template_5));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_6:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_6))
            {
                memcpy(&g_niu_data_rw.uploading_template_6, addr, sizeof(g_niu_data_rw.uploading_template_6));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_7:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_7))
            {
                memcpy(&g_niu_data_rw.uploading_template_7, addr, sizeof(g_niu_data_rw.uploading_template_7));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_8:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_8))
            {
                memcpy(&g_niu_data_rw.uploading_template_8, addr, sizeof(g_niu_data_rw.uploading_template_8));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_9:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_9))
            {
                memcpy(&g_niu_data_rw.uploading_template_9, addr, sizeof(g_niu_data_rw.uploading_template_9));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_10:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_10))
            {
                memcpy(&g_niu_data_rw.uploading_template_10, addr, sizeof(g_niu_data_rw.uploading_template_10));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_11:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_11))
            {
                memcpy(&g_niu_data_rw.uploading_template_11, addr, sizeof(g_niu_data_rw.uploading_template_11));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_12:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_12))
            {
                memcpy(&g_niu_data_rw.uploading_template_12, addr, sizeof(g_niu_data_rw.uploading_template_12));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_13:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_13))
            {
                memcpy(&g_niu_data_rw.uploading_template_13, addr, sizeof(g_niu_data_rw.uploading_template_13));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_14:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_14))
            {
                memcpy(&g_niu_data_rw.uploading_template_14, addr, sizeof(g_niu_data_rw.uploading_template_14));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_15:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_15))
            {
                memcpy(&g_niu_data_rw.uploading_template_15, addr, sizeof(g_niu_data_rw.uploading_template_15));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_16:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_16))
            {
                memcpy(&g_niu_data_rw.uploading_template_16, addr, sizeof(g_niu_data_rw.uploading_template_16));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_17:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_17))
            {
                memcpy(&g_niu_data_rw.uploading_template_17, addr, sizeof(g_niu_data_rw.uploading_template_17));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_18:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_18))
            {
                memcpy(&g_niu_data_rw.uploading_template_18, addr, sizeof(g_niu_data_rw.uploading_template_18));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_19:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_19))
            {
                memcpy(&g_niu_data_rw.uploading_template_19, addr, sizeof(g_niu_data_rw.uploading_template_19));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_20:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_20))
            {
                memcpy(&g_niu_data_rw.uploading_template_20, addr, sizeof(g_niu_data_rw.uploading_template_20));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_21:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_21))
            {
                memcpy(&g_niu_data_rw.uploading_template_21, addr, sizeof(g_niu_data_rw.uploading_template_21));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_22:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_22))
            {
                memcpy(&g_niu_data_rw.uploading_template_22, addr, sizeof(g_niu_data_rw.uploading_template_22));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_23:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_23))
            {
                memcpy(&g_niu_data_rw.uploading_template_23, addr, sizeof(g_niu_data_rw.uploading_template_23));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_24:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_24))
            {
                memcpy(&g_niu_data_rw.uploading_template_24, addr, sizeof(g_niu_data_rw.uploading_template_24));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_25:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_25))
            {
                memcpy(&g_niu_data_rw.uploading_template_25, addr, sizeof(g_niu_data_rw.uploading_template_25));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_26:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_26))
            {
                memcpy(&g_niu_data_rw.uploading_template_26, addr, sizeof(g_niu_data_rw.uploading_template_26));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_27:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_27))
            {
                memcpy(&g_niu_data_rw.uploading_template_27, addr, sizeof(g_niu_data_rw.uploading_template_27));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_28:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_28))
            {
                memcpy(&g_niu_data_rw.uploading_template_28, addr, sizeof(g_niu_data_rw.uploading_template_28));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_29:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_29))
            {
                memcpy(&g_niu_data_rw.uploading_template_29, addr, sizeof(g_niu_data_rw.uploading_template_29));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_30:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_30))
            {
                memcpy(&g_niu_data_rw.uploading_template_30, addr, sizeof(g_niu_data_rw.uploading_template_30));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_31:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_31))
            {
                memcpy(&g_niu_data_rw.uploading_template_31, addr, sizeof(g_niu_data_rw.uploading_template_31));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_32:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_32))
            {
                memcpy(&g_niu_data_rw.uploading_template_32, addr, sizeof(g_niu_data_rw.uploading_template_32));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_33:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_33))
            {
                memcpy(&g_niu_data_rw.uploading_template_33, addr, sizeof(g_niu_data_rw.uploading_template_33));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_34:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_34))
            {
                memcpy(&g_niu_data_rw.uploading_template_34, addr, sizeof(g_niu_data_rw.uploading_template_34));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_35:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_35))
            {
                memcpy(&g_niu_data_rw.uploading_template_35, addr, sizeof(g_niu_data_rw.uploading_template_35));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_36:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_36))
            {
                memcpy(&g_niu_data_rw.uploading_template_36, addr, sizeof(g_niu_data_rw.uploading_template_36));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_37:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_37))
            {
                memcpy(&g_niu_data_rw.uploading_template_37, addr, sizeof(g_niu_data_rw.uploading_template_37));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_38:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_38))
            {
                memcpy(&g_niu_data_rw.uploading_template_38, addr, sizeof(g_niu_data_rw.uploading_template_38));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_39:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_39))
            {
                memcpy(&g_niu_data_rw.uploading_template_39, addr, sizeof(g_niu_data_rw.uploading_template_39));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_40:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_40))
            {
                memcpy(&g_niu_data_rw.uploading_template_40, addr, sizeof(g_niu_data_rw.uploading_template_40));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_41:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_41))
            {
                memcpy(&g_niu_data_rw.uploading_template_41, addr, sizeof(g_niu_data_rw.uploading_template_41));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_42:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_42))
            {
                memcpy(&g_niu_data_rw.uploading_template_42, addr, sizeof(g_niu_data_rw.uploading_template_42));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_43:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_43))
            {
                memcpy(&g_niu_data_rw.uploading_template_43, addr, sizeof(g_niu_data_rw.uploading_template_43));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_44:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_44))
            {
                memcpy(&g_niu_data_rw.uploading_template_44, addr, sizeof(g_niu_data_rw.uploading_template_44));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_45:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_45))
            {
                memcpy(&g_niu_data_rw.uploading_template_45, addr, sizeof(g_niu_data_rw.uploading_template_45));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_46:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_46))
            {
                memcpy(&g_niu_data_rw.uploading_template_46, addr, sizeof(g_niu_data_rw.uploading_template_46));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_47:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_47))
            {
                memcpy(&g_niu_data_rw.uploading_template_47, addr, sizeof(g_niu_data_rw.uploading_template_47));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_48:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_48))
            {
                memcpy(&g_niu_data_rw.uploading_template_48, addr, sizeof(g_niu_data_rw.uploading_template_48));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_49:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_49))
            {
                memcpy(&g_niu_data_rw.uploading_template_49, addr, sizeof(g_niu_data_rw.uploading_template_49));
                ret = JTRUE;
            }
        }
        break;
        case NIU_DATA_RW_TEMPLATE_50:
        {
            if (addr && len == sizeof(g_niu_data_rw.uploading_template_50))
            {
                memcpy(&g_niu_data_rw.uploading_template_50, addr, sizeof(g_niu_data_rw.uploading_template_50));
                ret = JTRUE;
            }
        }
        break;
        default:
            break;
        }
    }
    //LOG_D("index:%d, addr:%p, len:%d, ret:%d", index, addr, len, ret);

    if (JTRUE == ret && JFALSE == tmr_handler_tmr_is_exist(TIMER_ID_NIU_UPDATE_RW_DATA))
    {
        tmr_handler_tmr_add(TIMER_ID_NIU_UPDATE_RW_DATA, niu_data_rw_flush, NULL, 1000, 0);
    }
    return ret;
}

/**
 * read value from default template
 * @index: the template index, 0~19
 * @templ: the template value point, see struct NIU_SERVER_UPL_TEMPLATE
 * 
 * @return: void
 */
JVOID niu_data_rw_template_read(NUINT32 index, NIU_SERVER_UPL_TEMPLATE *templ)
{
    //LOG_D("index:%d", index);
    if (index < NIU_SERVER_UPL_CMD_NUM && templ)
    {
        memcpy(templ, &g_niu_data_rw.template_data[index], sizeof(NIU_SERVER_UPL_TEMPLATE));
    }
}

/**
 * write to default template, it will flush delay
 * @index: the template index, 0~19
 * @templ: the template value point, see struct NIU_SERVER_UPL_TEMPLATE
 * 
 * @return: void
 */
JVOID niu_data_rw_template_write(NUINT32 index, NIU_SERVER_UPL_TEMPLATE *templ)
{
    //LOG_D("index:%d", index);
    if (index < NIU_SERVER_UPL_CMD_NUM && templ)
    {
        memcpy(&g_niu_data_rw.template_data[index], templ, sizeof(NIU_SERVER_UPL_TEMPLATE));
        tmr_handler_tmr_add(TIMER_ID_NIU_UPDATE_RW_DATA, niu_data_rw_flush, NULL, 1000, 0);
    }
}
