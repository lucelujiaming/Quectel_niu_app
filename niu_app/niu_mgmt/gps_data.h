/********************************************************************
 * @File name: gps_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: read nmea data and parse value to niu_gps_data.
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef GPS_DATA_H_
#define GPS_DATA_H_
#ifdef __cplusplus 
extern "C" {
#endif

#include "niu_types.h"

#define UART_DEV_GPS_NAME               "/dev/ttyHSL0"
#define UART_DEV_GPS_BOUNDRATE          (9600)
#define UART_DEV_GPS_FLOWCTRL           (0)
#define UART_DEV_GPS_PARITY             (0)

#define KNOTS_CONVERSION_FACTOR         (1.852)

#define QL_GSV_MAX_SATS                 (64)

struct niu_gps_data {
    /*是否有效定位*/
    NUINT8  valid;
    /*经度*/
    NDOUBLE longitude;
    /*经度E(东经)或W(西经)*/
    NUINT8  longitude_cardinal;
    /*纬度*/
    NDOUBLE latitude;
    /*纬度N(北纬)或S(南纬)*/
    NUINT8  latitude_cardinal;
    /*hdop精度*/
    NFLOAT  hdop;
    /*phop精度*/
    NFLOAT  pdop;
    /*航向 0-360*/
    NFLOAT  heading;
    /*速度 km/h*/
    NFLOAT  gps_speed;
    /*信号强度,max=5, data from avg_snr*/
    NUINT8  gps_signal;
    /*最大信号值 db*/
    NUINT32 max_cnr;
    NUINT32 max_cnr2;
    /*最小信号值 db*/
    NUINT32 min_cnr;
    /*平均信号值 db*/
    NUINT32 avg_cnr;
    /*信号值数组*/
    NUINT32 cnrs[QL_GSV_MAX_SATS];
    NUINT32 cnrs_index;
    /*定位卫星数量*/
    NUINT32 satellites_num;
    /*海拔高度*/
    NFLOAT altitude;
    /*FWVER*/
    NCHAR fwver[32];
};

/**
 * @brief init uart of gps, and create a pthread to recv nmea data
 * 
 * @return:
 *  if success return 0, else return -1
 */ 
int gps_init(void);

/**
 * copy struct niu_gps_data to data
 * 
 * @return:
 *  if success return 0, else return -1
 * 
 */ 
int gps_data_read(struct niu_gps_data *data);

/**
 * send gps config use uart
 * 
 * @return:
 *  if success return 0, else return -1
 * 
 */ 
int gps_set_config(char *buff, int len);



#ifdef __cplusplus 
}
#endif
#endif /* !GPS_DATA_H_ */
