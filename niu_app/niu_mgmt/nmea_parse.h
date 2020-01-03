/********************************************************************
 * @File name: niu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: nmea data parse
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef NMEA_PARSE_H_
	#define NMEA_PARSE_H_
#ifdef __cplusplus 
extern "C" {
#endif
#include "niu_types.h"
#include <time.h>


#define NMEA_MAX_LENGTH  (82)
#define NMEA_END_CHAR_1 '\r'
#define NMEA_END_CHAR_2 '\n'


typedef enum {
	NMEA_UNKNOWN,
	NMEA_RMC,
    NMEA_GGA,
    NMEA_GSA,
    NMEA_GSV,
    NMEA_TXT,
} nmea_type;

typedef char nmea_cardinal_t;
#define NMEA_CARDINAL_DIR_NORTH		(nmea_cardinal_t) 'N'
#define NMEA_CARDINAL_DIR_EAST		(nmea_cardinal_t) 'E'
#define NMEA_CARDINAL_DIR_SOUTH		(nmea_cardinal_t) 'S'
#define NMEA_CARDINAL_DIR_WEST		(nmea_cardinal_t) 'W'
#define NMEA_CARDINAL_DIR_UNKNOWN	(nmea_cardinal_t) '\0'

#define NMEA_PREFIX_LENGTH          (5)
#define NMEA_TIME_FORMAT	"%H%M%S"
#define NMEA_TIME_FORMAT_LEN	6

#define NMEA_DATE_FORMAT	"%d%m%y"
#define NMEA_DATE_FORMAT_LEN	6

struct nmea_s{
    nmea_type type;
    void *data;
};

/* GPS position struct */
struct nmea_position{
	float minutes;
	int degrees;
	nmea_cardinal_t cardinal;
};

/* nmea rmc packet define */
/* Value indexes */
#define NMEA_GPRMC_TIME			            0
#define NMEA_GPRMC_STATUS                   1
#define NMEA_GPRMC_LATITUDE		            2
#define NMEA_GPRMC_LATITUDE_CARDINAL	    3
#define NMEA_GPRMC_LONGITUDE		        4
#define NMEA_GPRMC_LONGITUDE_CARDINAL	    5
#define NMEA_GPRMC_SPEED		            6
#define NMEA_GPRMC_COURSE                   7
#define NMEA_GPRMC_DATE			            8

struct nmea_gprmc_s{
    /*1: valid, 0: invalid*/
    int valid;
	struct nmea_position longitude;
	struct nmea_position latitude;
    /*航向*/
    float course;
	float speed;
	struct tm time;
} ;


/* nmea gsv packet define */
#define NMEA_GPGSV_TOTAL_MSGS               0
#define NMEA_GPGSV_MSG_NR                   1
#define NMEA_GPGSV_SATS                     2
#define NMEA_GPGSV_SAT_INFO_NR1             3
#define NMEA_GPGSV_SAT_INFO_ELEV1           4
#define NMEA_GPGSV_SAT_INFO_AZIMUTH1        5
#define NMEA_GPGSV_SAT_INFO_SNR1            6
#define NMEA_GPGSV_SAT_INFO_NR2             7
#define NMEA_GPGSV_SAT_INFO_ELEV2           8
#define NMEA_GPGSV_SAT_INFO_AZIMUTH2        9
#define NMEA_GPGSV_SAT_INFO_SNR2            10
#define NMEA_GPGSV_SAT_INFO_NR3             11
#define NMEA_GPGSV_SAT_INFO_ELEV3           12
#define NMEA_GPGSV_SAT_INFO_AZIMUTH3        13
#define NMEA_GPGSV_SAT_INFO_SNR3            14
#define NMEA_GPGSV_SAT_INFO_NR4             15
#define NMEA_GPGSV_SAT_INFO_ELEV4           16
#define NMEA_GPGSV_SAT_INFO_AZIMUTH4        17
#define NMEA_GPGSV_SAT_INFO_SNR4            18

struct nmea_sat_info {
    /* PRN */
    int nr;
    /*elevation of satellite*/
    int elevation;
    /*azimuth of satellite*/
    int azimuth;
    /*Signal Noise Ratio*/
    int snr;
};

struct nmea_gpgsv_s {
    /*total msg count*/
    int total_msgs;
    /*current msg index*/
    int msg_nr;
    /*total satellite count*/
    int total_sats;
    /*satellites info*/
    struct nmea_sat_info sats[4];
};

/*nmea gga packet define*/
//2,3,4,5,6,7
#define NMEA_GPGGA_LATITUDE                     1
#define NMEA_GPGGA_LATITUDE_CARDINAL            2
#define NMEA_GPGGA_LONGITUDE                    3
#define NMEA_GPGGA_LONGITUDE_CARDINAL           4
#define NMEA_GPGGA_STATUS                       5
#define NMEA_GPGGA_SATELLITES_TRACKED           6
#define NMEA_GPGGA_HDOP                         7
#define NMEA_GPGGA_ALTITUDE                     8

struct nmea_gpgga_s{
    struct nmea_position longitude;
	struct nmea_position latitude;
    /*GPS状态，0=未定位，1=非差分定位，2=差分定位，3=无效PPS，6=正在估算*/
    NUINT8 status;
    NUINT32 satellites_tracked;
    NFLOAT hdop;
    NFLOAT altitude;
} ;


/*nmea gsa packet define*/
#define NMEA_GPGSA_MODE                         0
#define NMEA_GPGSA_TYPE                         1
#define NMEA_GPGSA_PDOP                         14
#define NMEA_GPGSA_HDOP                         15
#define NMEA_GPGSA_VDOP                         16
struct nmea_gpgsa_s {
    NUINT8 mode;
    NUINT8 type;
    NFLOAT pdop;
    NFLOAT hdop;
    NFLOAT vdop;
};

/*nmea txt packet define*/
#define NMEA_GNTXT_FWVER                        3
struct nmea_gntxt_s {
    NCHAR fwver[32];
};

nmea_type nmea_get_type(const char *sentence);
struct nmea_s* nmea_parse(char *sentence, int length, int check_checksum);


#ifdef __cplusplus 
}
#endif
#endif /* !NMEA_PARSE_H_ */
