/********************************************************************
 * @File name: niu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: utils define
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef UTILS_H_
#define UTILS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "niu_types.h"
#include "ql_common.h"
#include "niu_version.h"

#define jmin(a, b) (((a) > (b)) ? (b) : (a))
#define jmax(a, b) (((a) > (b)) ? (a) : (b))

JVOID niu_trace_dump_data(NCHAR* prompt, NUINT8* buf, NUINT32 len);

JVOID niu_trace_dump_hex(NCHAR *prompt, NUINT8 *buf, NUINT32 len);


#if 0
#define PRINT_DEBUG_MSG_
#ifdef PRINT_DEBUG_MSG_
	#define printDebugFormatMsg(format, ...) \
				printf("[DEBUG] "format,  ##__VA_ARGS__)
#else
	#define printDebugFormatMsg(format, ...)
#endif

#define PRINT_ERROR_MSG_
#ifdef PRINT_ERROR_MSG_
	#define printErrFormatMsg(format, ...) \
				printf("[%s:%d][ERROR] "format,  __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
	#define printErrFormatMsg(format, ...)
#endif
#endif

#define CONFIG_FILE_PATH_LEN_MAX 		(64)
#define RW_FILE_PATH_LEN_MAX			(64)
#define CONFIG_FILE_PREFIX 				"/home/root/xn_config"
#define CONFIG_FILE_PREFIX2 			"/usrdata/xn_config"
#define RW_FILE_PREFIX					"/usrdata/xn_app"


#define MAX_UPGRADE_FILE_NAME_SIZE    	(255)
#define MAX_UPGRADE_FILE_SIZE         	(5*1024*1024)

//just for test FOTA_UPGRADE_FILE_STM32
#define APP_PATH												"/usrdata/xn_app"
#define APP_BACKUP_PATH											"/usr/bin"
#define APP_EXEC												APP_PATH"/"NIU_APP
#define FOTA_BACKUP_PATH 										APP_PATH "/backup"
#define FOTA_BACKUP_EXEC										FOTA_BACKUP_PATH"/"NIU_APP".old"
#define FOTA_UPGRADE_PATH 										APP_PATH"/fota_download"
#define FOTA_UPGRADE_FILE_STM32 								FOTA_UPGRADE_PATH "/stm32.bin"
#define FOTA_UPGRADE_FILE_ENCRYPT_STM32 						FOTA_UPGRADE_PATH "/niuEncryptstm32.bin"
#define FOTA_UPGRADE_FILE_EC25 									FOTA_UPGRADE_PATH "/upgrade.bin"
#define FOTA_UPGRADE_FILE_EC25_DECRYPT 							FOTA_UPGRADE_PATH "/niu_mgmtd.n"
#define CONFIG_UPGRADE_FILE_EC25								FOTA_UPGRADE_PATH "/config.json"
void get_ec25_tea_key(unsigned char *key, int length);
void get_stm32_tea_key(unsigned char *key, int length);

int uart_close(int fd);
int uart_connect(char *dev, int boundrate, int flowctrl, int parity);

double niu_get_distance(double lat1, double longt1, double lat2, double longt2);

unsigned char CRC8(unsigned char *ptr, int len);
unsigned short CRC16(unsigned char *start_address, int len);

NUINT16 CRC16_file(char *fname);
int TEA_decrypt_file(char *fname, char *outfile, NUINT8 *key);

int copyfile(char *src, char *dest);


unsigned char *base64_encode(unsigned char *str, int str_len, int *ret_len);
unsigned char *base64_decode(unsigned char *str, int str_len, int *ret_len);
void StrToHex(char *dest, char *src, int len);

void bubbleSort(NUINT32 *a, int len);
#ifdef __cplusplus
}
#endif
#endif /* !UTILS_H_ */
