#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <ql_oe.h>
#include "ql_mem.h"
#include "niu_types.h"
#include "utils.h"

#define MAX_CHAR_ONE_LINE (60)

static unsigned char g_ec25_tea_key[16] = {0x17,0xC6,0xB9,0x7A,0xCE,0xFC,0xAD,0xAA,0x66,0xF2,0x2E,0x83,0x9E,0xD2,0x97,0xA2};
static unsigned char g_stm32_tea_key[16] = {0xE9,0x8D,0x86,0x6B,0x0A,0x83,0xB3,0x01,0xB5,0x62,0xE3,0xF0,0x26,0xEC,0x1A,0xFB};





JSTATIC JCONST NCHAR hex_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void get_ec25_tea_key(unsigned char *key, int length)
{
    if((NULL == key) || (length < 16)) {
        LOG_D("get ec25 tea key failed while param is invalid");
        return;
    }
    memset(key, 0, length);
    memcpy(key, g_ec25_tea_key, 16);
}

void get_stm32_tea_key(unsigned char *key, int length)
{
    if((NULL == key) || (length < 16)) {
        LOG_D("get ec25 tea key failed while param is invalid");
        return;
    }
    memset(key, 0, length);
    memcpy(key, g_stm32_tea_key, 16);
}

NCHAR jhex_low(NUINT8 v)
{
    return (char)hex_table[(v & 0x0F)];
}

NCHAR jhex_high(NUINT8 v)
{
    return (char)hex_table[((v & 0xF0) >> 4)];
}

JVOID niu_trace_dump_hex(NCHAR *prompt, NUINT8 *buf, NUINT32 len)
{
    int i;

    if (len > 0)
    {
        printf("%s:\n", prompt);
        for (i = 0; i < len; i++)
        {
            printf("%02X ", buf[i]);
            if ((i + 1) % 16 == 0)
            {
                printf("\n");
            }
        }
        if (i % 16 != 0)
        {
            printf("\n");
        }
    }
}

JVOID niu_trace_dump_data(NCHAR *prompt, NUINT8 *buf, NUINT32 len)
{
    NCHAR *disp_str = NULL;
    NUINT32 i = 0, j = 0;
    NUINT32 prompt_len = 0;
    NUINT32 lines = 0;
    NUINT32 line_len = 0;

    if (!buf || 0 == len)
    {
        LOG_D("%s error:{buf:0x%p, len:%d}", prompt, buf, len);
    }

    if (prompt)
    {
        prompt_len = strlen(prompt);
    }

    if (prompt && prompt_len >= (MAX_CHAR_ONE_LINE - 2))
    {
        LOG_D("error:{len=%d, prompt:%s}", prompt_len, prompt);
    }

    line_len = (MAX_CHAR_ONE_LINE - prompt_len) / 2;

    disp_str = (NCHAR *)QL_MEM_ALLOC(line_len * 2 + 1);

    if (disp_str)
    {
        lines = (len + line_len - 1) / line_len;

        for (i = 0; i < lines; i++)
        {
            NUINT8 l = line_len;

            if (i == (lines - 1))
            {
                l = (len % line_len) ? (len % line_len) : line_len;
            }

            memset(disp_str, 0, (line_len * 2 + 1));
            for (j = 0; j < l; j++)
            {
                NUINT8 v = buf[i * line_len + j];
                disp_str[j * 2] = jhex_high(v);
                disp_str[j * 2 + 1] = jhex_low(v);
            }

            LOG_D("%s[%d]{%s}", prompt, i, disp_str);
        }
        QL_MEM_FREE(disp_str);
    }
}

unsigned char CRC8(unsigned char *ptr, int len)
{
    unsigned char crc;
    unsigned char i;
    crc = 0;

    while(len--)
    {
       crc ^= *ptr++;
       for(i = 0;i < 8;i++)
       {
           if(crc & 0x80)
           {
               crc = (crc << 1) ^ 0x07;
           }
           else
           {
               crc <<= 1;
           }
        }
    }
    return crc;
}

unsigned short CRC16(unsigned char* start_address, int len)
{
    unsigned short CRC_KEY = 0xE32A;
    unsigned short crc = 0xFFFF;
    int i;
    unsigned char j;

    if (len > 0)
    {
        for (i = 0; i < len; i++)
        {
            crc = (unsigned short)(crc ^ (unsigned char)((unsigned char*)start_address)[i]);
            for (j = 0; j < 8; j++)
            {
                crc = (crc & 1) != 0 ? (unsigned short)((crc >> 1) ^ CRC_KEY) : (unsigned short)(crc >> 1);
            }
        }

        return (crc);
    }
    return 0;
}


NUINT16 CRC16_file(char *fname)
{
    int ret, fsize = 0;
    FILE *fp = NULL;
    NUINT8 *content = NULL;
    NUINT16 verify = 0;

    if(fname == NULL)
    {
        LOG_E("fname is null.");
        return 0;
    }
    if(fname[0] == 0)
    {
        LOG_E("fname is error.");
        return 0;
    }
    if(strlen(fname) > MAX_UPGRADE_FILE_NAME_SIZE)
    {
        LOG_E("fname is error.");
        return 0;
    }

    if(ql_com_file_exist(fname))
    {
        LOG_E("fname is not exist[%s].", fname);
        return 0;
    }
    fsize = ql_com_file_get_size(fname);
    if(fsize < 0 || fsize > MAX_UPGRADE_FILE_SIZE)
    {
        LOG_E("fsize error[%d].", fsize);
        return 0;
    }

    fp = fopen(fname, "r");
    if(fp == NULL)
    {
        LOG_E("fopen error[%s].", fname);
        return 0;
    }

    content = QL_MEM_ALLOC(fsize);
    if(content == NULL)
    {
        LOG_E("ql_mem_alloc failed[size=%d].", fsize);
        goto _error;
    }

    ret = fread(content, 1, fsize, fp);
    if(ret != fsize)
    {
        LOG_E("fread error[ret=%d, size=%d].", ret, fsize);
        goto _error;
    }

    fclose(fp);
    fp = NULL;

    verify = CRC16(content, fsize);
    if(verify == 0)
    {
        LOG_E("crc16 error[verify=%d].", verify);
        goto _error;
    }
    QL_MEM_FREE(content);
    return verify;

_error:
    if(fp)
    {
        fclose(fp);
        fp = NULL;
    }
    if(content)
    {
        QL_MEM_FREE(content);
        content = NULL;
    }

    return 0;
}


int uart_close(int fd)
{
    if(fd)
    {
        return Ql_UART_Close(fd);
    }
    return -1;
}

int uart_connect(char *dev, int boundrate, int flowctrl, int parity)
{
    int fd_uart, ret;

    if(dev == NULL || boundrate < 0 || flowctrl < 0 || parity < 0)
    {
        LOG_E("uart connect param error.");
        return -1;
    }

    //open aurt 
    fd_uart = Ql_UART_Open(dev, boundrate, FC_NONE);
    if(fd_uart <= 0)
    {
        LOG_E("uart open failed.[dev: %s, bound: %d, flowctrl: %d][fd=%d]", 
                                        dev, boundrate, flowctrl, fd_uart);
        return -1;
    }

    ST_UARTDCB dcb = {
		.flowctrl = flowctrl,	//none flow control
		.databit = DB_CS8,	    //databit: 8
		.stopbit = SB_1,	    //stopbit: 1
		.parity = parity,	    //parity check: none
		.baudrate = boundrate,	//baudrate
	};

    ret = Ql_UART_SetDCB(fd_uart, &dcb);
    if(ret)
    {
        uart_close(fd_uart);
        return -1;
    }
    return fd_uart;
}

#define EARTH_RADIUS (6378137)
#define MATH_PI (3.141592653589793)
#define MOVE_DISTANCE (100.0) 

JSTATIC double rad(double d)
{
    return (d * MATH_PI / 180.0);
}

double niu_get_distance(double lat1, double longt1, double lat2, double longt2)
{
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(longt1) - rad(longt2);
    //double sin_a = sin(a/2);
    //double sin_b = sin(b/2);
    //double cos_1 = cos(radLat1);
    //double cos_2 = cos(radLat2);
    //double pow1 = pow(sin(a/2), 2);
    //double pow2 = pow(sin(b/2), 2);
    //double sqrt_input = ( pow(sin(a/2), 2) + cos(radLat1)*cos(radLat2)*pow(sin(b/2), 2) );
    //double sqrt_value = ( sqrt( pow(sin(a/2), 2) + cos(radLat1)*cos(radLat2)*pow(sin(b/2), 2) ) );
    double s = asin( sqrt( pow(sin(a/2), 2) + cos(radLat1)*cos(radLat2)*pow(sin(b/2), 2) ) ) * 2;

    s *= EARTH_RADIUS;
    return s;
}

//加密函数
void encrypt(NUINT32 *v, NUINT32 *k)
{
    NUINT32 v0 = v[0], v1 = v[1], sum = 0, i;           /* set up */
    NUINT32 delta = 0x9e3779b9;                         /* a key schedule constant */
    NUINT32 k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3]; /* cache key */
    for (i = 0; i < 32; i++)
    { /* basic cycle start */
        sum += delta;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    } /* end cycle */
    v[0] = v0;
    v[1] = v1;
}

//解密函数
void decrypt(NUINT32 *v, NUINT32 *k)
{
    NUINT32 v0 = v[0], v1 = v[1], sum = 0xC6EF3720, i;  /* set up */
    NUINT32 delta = 0x9e3779b9;                         /* a key schedule constant */
    NUINT32 k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3]; /* cache key */
    for (i = 0; i < 32; i++)
    { /* basic cycle start */
        v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        sum -= delta;
    } /* end cycle */
    v[0] = v0;
    v[1] = v1;
}

#define TEA_BLOCK_SIZE (8)


/**
 * decrypt upgrade file, use tea decrypt.
 * padding method: in encrypt, if file length is times of BLOCK SIZE, padding BLOCK_SIZE in the end,
 * the padding bytes of value is BLOCK_SIZE.
 * else we padding BLOCK_SIZE - length % BLOCK_SIZE bytes in the end, the padding bytes of value is 
 * BLOCK_SIZE - length % BLOCK_SIZE.
 * #param
 *  @fname: input file
 *  @outfile: decrypt file name, where to write the decrypt file
 *  @key: 128bit key
 * 
 * #return
 *  if success , return 0 or more than 0,
 *  if failed, return -1
 */ 

int TEA_decrypt_file(char *fname, char *outfile, NUINT8 *key)
{
    int ret = -1;
    int file_size = 0, pos = 0, write_size;
    NUINT8 buffer[8];
    FILE *in_fp = NULL, *out_fp = NULL;

    if(fname == NULL || outfile == NULL)
    {
        LOG_E("decrypt file or out file path is not set.\n");
        return -1;
    }

    if(ql_com_file_exist(fname))
    {
        LOG_E("decrypt file `%s` is not exist.\n", fname);
        return -1;
    }

    file_size = ql_com_file_get_size(fname);

    if((file_size <= 0)  && (file_size % TEA_BLOCK_SIZE != 0))
    {
        LOG_E("file `%s` size error[size = %d].\n", fname, file_size);
        return -1;
    }

    in_fp = fopen(fname, "r+");
    if(in_fp == NULL)
    {
        LOG_E("fopen `%s` error.\n", fname);
        return -1;
    }

    out_fp = fopen(outfile, "wb");
    if(out_fp == NULL)
    {
        LOG_E("fopen `%s` error.\n", outfile);
        fclose(in_fp);
        return -1;
    }

    while(pos < file_size)
    {
        ret = fread(buffer, 1, TEA_BLOCK_SIZE, in_fp);
        if(ret <= 0)
        {
            ret = 0;
            break;
        }

        decrypt((NUINT32*)buffer, (NUINT32*)key);

        if(file_size - pos <= 8)
        {
            //remove padding
            NUINT8 padding = buffer[TEA_BLOCK_SIZE - 1];

            if(padding > 0 && padding <= TEA_BLOCK_SIZE)
            {
                write_size = 8 - padding;
                if(write_size == 0)
                {
                    ret = 0;
                    break;
                }
                ret = fwrite(buffer, 1, write_size, out_fp);
                if(ret != write_size)
                {
                    LOG_E("fwrite error.\n");
                    ret = -1;
                    break;
                }
            }
            else
            {
                LOG_E("padding error.\n");
                ret = -1;
            }
        }
        else
        {
            ret = fwrite(buffer, 1, TEA_BLOCK_SIZE, out_fp);
            if(ret != TEA_BLOCK_SIZE)
            {
                LOG_E("fwrite error.\n");
                ret = -1;
                break;
            }
        }
        pos += 8;
    }
    if(in_fp != NULL)
    {
        fclose(in_fp);
    }
    if(out_fp != NULL)
    {
        fclose(out_fp);
    }
    return ret;
}


int copyfile(char *src, char *dest)
{
    FILE *fp_r, *fp_w;
    int len_r, len_w;
    char buff[256];
    if(src == NULL || dest == NULL)
    {
        LOG_E("src or dest not set.");
        return -1;
    }

    if ((fp_r = fopen(src, "r")) == NULL)
	{
		LOG_E("file `%s` open failed.", src);
		return -1;
	}
	if ((fp_w = fopen(dest, "w")) == NULL)
	{
		LOG_E("file '%s' open failed.", dest);
		fclose(fp_r);
		return -1;
	}

    memset(buff, 0, 256);
	while ((len_r = fread(buff, 1, 256, fp_r)) > 0)
	{
		if ((len_w = fwrite(buff, 1, len_r, fp_w)) != len_r)
		{
			LOG_E("Write to file `%s` failed.", dest);
			fclose(fp_r);
			fclose(fp_w);
			return -1;
		}
		memset(buff, 0, 256);
	}
 
	fclose(fp_r);
	fclose(fp_w);
	return 0;
}

static char *base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char *base64_encode(unsigned char *str, int str_len, int *ret_len)
{
    int i = 0;
    long len = 0;
    unsigned char *res = NULL;
    
    if((NULL == str) || (NULL == ret_len )) {
        LOG_D("base64 encode while param is NULL");
        return NULL;
    }

    if(0 == str_len % 3) {
        len = str_len / 3 * 4;
    }
    else {
        len = (str_len / 3 + 1) * 4;
    }

    res = (unsigned char *)QL_MEM_ALLOC(len + 1);
    if(NULL == res) {
        LOG_D("base64 encode while malloc failure");
        return NULL;
    }
    *ret_len = len;
    
    memset(res, 0, len + 1);
    for(i = 0; i < str_len / 3; i++) {
        res[i * 4 + 0] = base64_table[str[i * 3 + 0] >> 2];
        res[i * 4 + 1] = base64_table[((str[i * 3 + 0] & 0x03) << 4) | (str[i * 3 + 1] >> 4)];
        res[i * 4 + 2] = base64_table[((str[i * 3 + 1] & 0x0f) << 2) | (str[i * 3 + 2] >> 6)];
        res[i * 4 + 3] = base64_table[str[i * 3 + 2] & 0x3f];
    }
    if(1 == str_len % 3) {
        res[i * 4 + 0] = base64_table[str[i * 3 + 0] >> 2];
        res[i * 4 + 1] = base64_table[(str[i * 3 + 0] & 0x03) << 4];
        res[i * 4 + 2] = '=';
        res[i * 4 + 3] = '=';
    }
    else if(2 ==  str_len % 3) {
        res[i * 4 + 0] = base64_table[str[i * 3 + 0] >> 2];
        res[i * 4 + 1] = base64_table[((str[i * 3 + 0] & 0x03) << 4) | (str[i * 3 + 1] >> 4)];
        res[i * 4 + 2] = base64_table[((str[i * 3 + 1] & 0x0f) << 2)];
        res[i * 4 + 3] = '=';
    }
    return res;
}

static int table[] = {
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,    //0-11
    	0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,    //12-23
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,    //24-35
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,62,0 ,0 ,0 ,63,    //36-47
        52,53,54,55,56,57,58,59,60,61,0 ,0 ,    //48-59
        0 ,0 ,0 ,0 ,0 ,0 ,1 ,2 ,3 ,4 ,5 ,6 ,    //60-71
        7 ,8 ,9 ,10,11,12,13,14,15,16,17,18,    //72-83
        19,20,21,22,23,24,25,0 ,0 ,0 ,0 ,0 ,    //84-95
        0 ,26,27,28,29,30,31,32,33,34,35,36,    //96-107
        37,38,39,40,41,42,43,44,45,46,47,48,    //108-119
        49,50,51                                //120-122
    };
    
unsigned char *base64_decode(unsigned char *str, int str_len, int *ret_len)
{
    int i = 0;
    unsigned char *res = NULL;
    
    if((NULL == str) || (NULL == ret_len )) {
        LOG_D("base64 decode while param is NULL");
        return NULL;
    }
    if(NULL != strstr((char *)str, "==")) {
        *ret_len = str_len / 4 * 3 - 2;
    }
    else if(NULL != strstr((char *)str, "=")) {
        *ret_len = str_len / 4 * 3 - 1;  
    }
    else { 
        *ret_len = str_len / 4 * 3; 
    }
  
    res = (unsigned char *)QL_MEM_ALLOC(*ret_len + 1);  
    if(NULL == res) {
        LOG_D("base64 decode while malloc failure");
        return NULL;
    }

    for(i = 0; i < str_len / 4 - 1; i++) {
        res[i * 3 + 0] = table[str[i * 4 + 0 ]] << 2 | table[str[i * 4 + 1]] >> 4;
        res[i * 3 + 1] = table[str[i * 4 + 1 ]] << 4 | table[str[i * 4 + 2]] >> 2;
        res[i * 3 + 2] = table[str[i * 4 + 2 ]] << 6 | table[str[i * 4 + 3]];
    }

    if(NULL != strstr((char *)str, "==")) {
        res[i * 3 + 0] = table[str[i * 4 + 0 ]] << 2 | table[str[i * 4 + 1]] >> 4;
    }
    else if(NULL != strstr((char *)str, "=")) {
        res[i * 3 + 0] = table[str[i * 4 + 0 ]] << 2 | table[str[i * 4 + 1]] >> 4;
        res[i * 3 + 1] = table[str[i * 4 + 1 ]] << 4 | table[str[i * 4 + 2]] >> 2;
    }
    else { 
        res[i * 3 + 0] = table[str[i * 4 + 0 ]] << 2 | table[str[i * 4 + 1]] >> 4;
        res[i * 3 + 1] = table[str[i * 4 + 1 ]] << 4 | table[str[i * 4 + 2]] >> 2;
        res[i * 3 + 2] = table[str[i * 4 + 2 ]] << 6 | table[str[i * 4 + 3]];
    }
    return res;
} 



void StrToHex(char *dest, char *src, int len)
{
    int i;
	char h1, h2;
	char s1, s2;
	for (i = 0; i < len; i++)
	{
		h1 = src[2 * i];
		h2 = src[2 * i + 1];
 
		s1 = toupper(h1) - 0x30;
		if (s1 > 9)
			s1 -= 7;
 
		s2 = toupper(h2) - 0x30;
		if (s2 > 9)
			s2 -= 7;
 
		dest[i] = s1 * 16 + s2;
	}
}

void bubbleSort(NUINT32 *a, int len)
{
    NUINT32 tmp;
    int end = len - 1, i = 0;
    while (end > 0)
    {
        int exchange = 0;
        for (i = 0; i < end; i++)
        {
            if (a[i] < a[i + 1])
            {
                // swap(a[i], a[i + 1]);
                tmp = a[i];
                a[i] = a[i + 1];
                a[i + 1] = tmp;
                exchange = 1;
            }
        }
        if (exchange == 0)
        {
            return;
        }
        else
        {
            exchange = 0;
        }
        end--;
    }
}