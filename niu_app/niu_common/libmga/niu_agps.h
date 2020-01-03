/*
** EPITECH PROJECT, 2019
** niu_app
** File description:
** niu_agps
*/

#ifndef NIU_AGPS_H_
	#define NIU_AGPS_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//! command line arguments
typedef struct
{
    // interface configuration
    const char*     port;               //!< COM port name
    unsigned int    baudrate;           //!< Baud rate for serial port
    // server configuration
    const char*     server1;            //!< MGA server 1 address
    const char*     server2;            //!< MGA server 2 address
    const char*     token;              //!< MGA token
    const char*     gnss;               //!< GNSS systems, comma separated
    const char*     dataType;           //!< Data type
    const char*     format;             //!< Data format
    const char*     alm;                //!< GNSS systems from which to get almanac data, comma separated
    // online options
    unsigned int    usePosition;        //!< Use reference position
    double          posAccuracy;        //!< Position accuracy in m
    double          posAltitude;        //!< Altitude
    double          posLatitude;        //!< Latitude
    double          posLongitude;       //!< Longitude
    unsigned int    useLatency;         //!< Use latency
    double          latency;            //!< Latency in s
    unsigned int    useTimeAccuracy;    //!< Use time accuracy
    double          timeAccuracy;       //!< Time accuracy in s
    // offline options
    unsigned int    period;             //!< Number of weeks into the future the data should be valid for
    unsigned int    resolution;         //!< Resolution of the data
    unsigned int    days;               //!< The number of days into the future the data should be valid for
    // test configuration
    const char*     test;               //!< MGA test to perform
    // flow control configuration
    unsigned int    timeout;            //!< Timeout
    unsigned int    retry;              //!< Retry count
    const char*     flow;               //!< Flow control
    // legacy configuration
    int             legacyServerDuration; //!< Legacy server duration
    // general configuration
    unsigned int    verbosity;          //!< Verbosity level

    //server certificate verification
    int serverVerify;                  //!< Verify server certificate
} CL_ARGUMENTS_t;

typedef CL_ARGUMENTS_t *CL_ARGUMENTS_pt;//!< pointer to CL_ARGUMENTS_t type


void niu_agps_mga_online(double longitude, double latitude);
#ifdef __cplusplus
}
#endif // __cplusplus
#endif /* !NIU_AGPS_H_ */
