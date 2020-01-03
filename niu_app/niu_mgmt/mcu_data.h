/********************************************************************
 * @File name: mcu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: communication between ec25 and stm32
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/
#ifndef __MCU_DATA_H__
#define __MCU_DATA_H__

#ifdef __cplusplus 
extern "C" {
#endif

#include "niu_types.h"

#define FOTA_UPGRADE_PACKAGE_SIZE (1024)


struct mcu_file_trans {
    char        fname[255];     //file name
    int         fsize;          //file size
    NUINT32     package_index;  //package index
    int         package_size;   //package size
    int         deal_size;      //already send or recv size
    int         is_start;       //fota start flag
    int         fd;
    int         retry;
    NUINT16     device_id;
    char        device_type[16];
    NUINT32     last_time;
};


enum {
    NIU_DEVICE_ID_DB = 0x10,
    NIU_DEVICE_ID_FOC = 0x20,
    NIU_DEVICE_ID_BMS = 0x31,
    NIU_DEVICE_ID_BMS2 = 0x32,
    NIU_DEVICE_ID_ALARM = 0X50,
    NIU_DEVICE_ID_LCU = 0x70,
    NIU_DEVICE_ID_QC = 0x35,
    NIU_DEVICE_ID_LKU = 0xA0,
    NIU_DEVICE_ID_BCS = 0xB0,
    NIU_DEVICE_ID_EMCU = 0x41
};

//enable define

#define _NIU_SERIAL_RESP_ENABLE_                (0)


#define UART_DEV_STM_NAME               "/dev/ttyHSL1"
#define UART_DEV_STM_BOUNDRATE          (115200)
#define UART_DEV_STM_FLOWCTRL           (0)
#define UART_DEV_STM_PARITY             (2)

int mcu_data_init();

int mcu_send_read(NUINT8 base, NUINT16 id);
int mcu_send_seqread(NUINT8 base, NUINT16 id, NUINT8 count);
int mcu_send_write(NUINT8 base, NUINT16 id, NUINT8 *value, NUINT32 len);


//fota upgrade
int mcu_upgrade_to_stm32(char *fname, char *device_type);

#ifdef __cplusplus 
}
#endif
#endif //__MCU_DATA_H__
