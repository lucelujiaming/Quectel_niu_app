/********************************************************************
 * @File name: gsensor_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: read gsensor data and calculate roll,pitch,yaw
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef GSENSOR_DATA_H_
	#define GSENSOR_DATA_H_

#include "niu_types.h"

#define EULAR               3
#define NUMAXIS             3
#define POLE_NUM            6
#define SINARRAYSIZE        4096
#define SINARRAYSCALE       32767
#define PWM_PERIODE         1000
#define M_PI_NIU            3.1415926535898f
#define M_TWOPI             (M_PI_NIU * 2)
#define D2R(x)              (x * 0.01745329252f)
#define R2D(x)              (x * 57.2957795131f)
#define twoKp               4.0f  //0.2f
#define twoKi               0.01f //0.001f
#define ACCL_SCALE          (1.0f / 4096)
#define GYRO_SCALE          ((1 / 16.4f) * (M_PI_NIU / 180.0f))
#define ToRad(x)            (x * 0.01745329252f)
#define ToDeg(x)            (x * 57.2957795131f)


#define NIU_DEVICE_ACCL "/dev/iio:device0"
#define NIU_DEVICE_GYRO "/dev/iio:device1"

#define USE_CHANGE_I_RATE (0)
#define USE_CHANGE_P_RATE (0)
#define IMU_ROTATE_X_90

typedef struct
{
    NFLOAT x;
    NFLOAT y;
    NFLOAT z;
} Vector3f;

typedef struct
{
    Vector3f a;
    Vector3f b;
    Vector3f c;
} Matrix3f;

typedef struct bmi_sensor_data
{
    NINT32 acclfd;
    NINT32 gyrofd;
    Vector3f gyr_mpu;
    Vector3f acc_mpu;
    Vector3f mag_mpu;
    Vector3f omega_mpu;
    Vector3f accl_offset;
    Vector3f gyro_offset;
    NINT32 ahrs_init_count;
    NFLOAT q0;
    NFLOAT q1;
    NFLOAT q2;
    NFLOAT q3;
    NFLOAT dq0, dq1, dq2, dq3;
    NFLOAT q0q0, q0q1, q0q2, q0q3;
    NFLOAT q1q1, q1q2, q1q3;
    NFLOAT q2q2, q2q3;
    NFLOAT q3q3;
    NFLOAT gyro_bias[3];
    Matrix3f dcm_matrix;
    Matrix3f dcm_matrix2;
    Matrix3f dcm_matrix3;
    NFLOAT roll;
    NFLOAT pitch;
    NFLOAT yaw;
    NFLOAT preYaw;
    NFLOAT curYaw;
    NFLOAT errYaw;
    NINT16 accdata[3];
    NINT16 gyrodata[3];
}BMI_SENSOR_DATA;

typedef enum
{
    X_AXIS,
    Y_AXIS,
    Z_AXIS
} AXIS;

typedef enum
{
    ROLL,
    PITCH,
    YAW
} tAxis;

typedef struct
{
    NUINT16 x;
    NUINT16 y;
    NUINT16 z;
} Vector3u;

typedef struct
{
    NINT16 x;
    NINT16 y;
    NINT16 z;
} Vector3s;

typedef struct
{
    NUINT16 x;
    NUINT16 y;
} Vector2u;

typedef struct
{
    NINT16 x;
    NINT16 y;
} Vector2s;

typedef struct
{
    NFLOAT x;
    NFLOAT y;
} Vector2f;

typedef struct
{
    NINT32 x;
    NINT32 y;
    NINT32 z;
} AxesRaw_TypeDef;


int gsensor_init();

int gsensor_data_read(BMI_SENSOR_DATA *sensor_data);
#endif /* !GSENSOR_DATA_H_ */
