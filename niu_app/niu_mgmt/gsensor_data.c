#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ql_oe.h>
#include "utils.h"
#include "gsensor_data.h"

BMI_SENSOR_DATA g_sensor_data = {0};

void vector3f_negation(Vector3f *ptrV)
{
    ptrV->x = -ptrV->x;
    ptrV->y = -ptrV->y;
    ptrV->z = -ptrV->z;
}

Vector3f vector3f_add(const Vector3f *ptrA, const Vector3f *ptrB)
{
    Vector3f result;
    result.x = ptrA->x + ptrB->x;
    result.y = ptrA->y + ptrB->y;
    result.z = ptrA->z + ptrB->z;

    return result;
}

Vector3f vector3f_sub(const Vector3f *ptrA, const Vector3f *ptrB)
{
    Vector3f result;
    result.x = ptrA->x - ptrB->x;
    result.y = ptrA->y - ptrB->y;
    result.z = ptrA->z - ptrB->z;

    return result;
}

NFLOAT vector3f_length(const Vector3f *ptrA)
{
    NFLOAT temp;

    temp = sqrtf((ptrA->x * ptrA->x) + (ptrA->y * ptrA->y) + (ptrA->z * ptrA->z));

    return temp;
}

Vector3f vector3f_uniform_scaling(const Vector3f *ptrA, const NUINT16 i)
{
	Vector3f result;
    result.x = ptrA->x / i;
    result.y = ptrA->y / i;
    result.z = ptrA->z / i;

    return result;
}

void vector3f_zero(Vector3f *ptrA)
{
    ptrA->x = 0;
    ptrA->y = 0;
    ptrA->z = 0;
}

// vector cross product
Vector3f vector3f_cross_product(const Vector3f *ptrV1, const Vector3f *ptrV2)
{
	Vector3f result;
    result.x = ptrV1->y * ptrV2->z - ptrV1->z * ptrV2->y;
    result.y = ptrV1->z * ptrV2->x - ptrV1->x * ptrV2->z;
    result.z = ptrV1->x * ptrV2->y - ptrV1->y * ptrV2->x;

    return result;
}

// vector dot product
NFLOAT vector3f_dot_product(const Vector3f *ptrV1, const Vector3f *ptrV2)
{
    return ptrV1->x * ptrV2->x + ptrV1->y * ptrV2->y + ptrV1->z * ptrV2->z;
}

void vector3f_normalize(Vector3f *ptrV)
{
    NFLOAT length = vector3f_length(ptrV);

    ptrV->x = ptrV->x / length;
    ptrV->y = ptrV->y / length;
    ptrV->z = ptrV->z / length;
}

NFLOAT invSqrt_(NFLOAT number)
{
    volatile long i;
    volatile NFLOAT x, y;
    volatile const NFLOAT f = 1.5f;

    x = number * 0.5f;
    y = number;

    i = *((long *)&y);
    i = 0x5f375a86 - (i >> 1);
    y = *((NFLOAT *)&i);
    y = y * (f - (x * y * y));
    return y;
}

NFLOAT constrain_float(NFLOAT amt, NFLOAT low, NFLOAT high)
{
    if (isnan(amt))
    {
        return (low + high) * 0.5f;
    }
    return ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)));
}

NFLOAT safe_asin(NFLOAT v)
{
    if (isnan(v))
    {
        return 0.0;
    }
    if (v >= 1.0f)
    {
        return M_PI_NIU / 2;
    }
    if (v <= -1.0f)
    {
        return -M_PI_NIU / 2;
    }
    return asinf(v);
}


void sensor_data_init(BMI_SENSOR_DATA *data)
{
    BMI_SENSOR_DATA *data_init = data;
    if(data_init == NULL)
    {
        return;
    }
    data_init->q0 = 1.0f;
    data_init->q1 = 0.0f;
    data_init->q2 = 0.0f;
    data_init->q3 = 0.0f;
    data_init->dq0 = 0.0f;
    data_init->dq1 = 0.0f;
    data_init->dq2 = 0.0f;
    data_init->dq3 = 0.0f;
    data_init->gyro_bias[0] = 0.0f;
    data_init->gyro_bias[1] = 0.0f;
    data_init->gyro_bias[2] = 0.0f;
    data_init->roll = 0;
    data_init->pitch = 0;
    data_init->yaw = 0;
    data_init->preYaw = 0;
    data_init->curYaw = 0;
    data_init->errYaw = 0;
    data_init->dcm_matrix.a.x = 1.0f;
    data_init->dcm_matrix.a.y = 0.0f;
    data_init->dcm_matrix.a.z = 0.0f;
    data_init->dcm_matrix.b.x = 0.0f;
    data_init->dcm_matrix.b.y = 1.0f;
    data_init->dcm_matrix.b.z = 0.0f;
    data_init->dcm_matrix.c.x = 0.0f;
    data_init->dcm_matrix.c.y = 0.0f;
    data_init->dcm_matrix.c.z = 1.0f;
    data_init->dcm_matrix2.a.x = 1.0f;
    data_init->dcm_matrix2.a.y = 0.0f;
    data_init->dcm_matrix2.a.z = 0.0f;
    data_init->dcm_matrix2.b.x = 0.0f;
    data_init->dcm_matrix2.b.y = 1.0f;
    data_init->dcm_matrix2.b.z = 0.0f;
    data_init->dcm_matrix2.c.x = 0.0f;
    data_init->dcm_matrix2.c.y = 0.0f;
    data_init->dcm_matrix2.c.z = 1.0f;
    data_init->dcm_matrix3.a.x = 1.0f;
    data_init->dcm_matrix3.a.y = 0.0f;
    data_init->dcm_matrix3.a.z = 0.0f;
    data_init->dcm_matrix3.b.x = 0.0f;
    data_init->dcm_matrix3.b.y = 1.0f;
    data_init->dcm_matrix3.b.z = 0.0f;
    data_init->dcm_matrix3.c.x = 0.0f;
    data_init->dcm_matrix3.c.y = 0.0f;
    data_init->dcm_matrix3.c.z = 1.0f;
}

int sensor_recv_data(int fd, char *buff, int len)
{
        int ret, pos = 0;
    fd_set fdset;
    struct timeval timeout = {0, 1000 * 50};

    ret = read(fd, buff, len);
    if(ret > 0)
    {
        if(ret == len)
        {
            return ret;
        }
        else
        {
            pos = ret;
        }
    }
    else if(ret == 0)
    {
        printf("read error.[ret=%d]\n", ret);
        return -1;
    }
    else if(ret == -1)
    {
        if(errno != EAGAIN)
        {
            printf("read error.[ret=%d, errno=%d]\n", ret, errno);
            return -1;
        }
    }

    while(1)
    {
        FD_ZERO(&fdset); 
        FD_SET(fd, &fdset); 
	timeout.tv_sec  = 0;
        timeout.tv_usec = 50 * 1000;
        ret = select(fd + 1, &fdset, NULL, NULL, &timeout);
        if (ret == -1)
        {
            printf("failed to select\n");
            return -1;
        }
        else if (ret == 0)
        {
            continue;
        }
        else
        {
            if (FD_ISSET(fd, &fdset))
            {
                ret = read(fd, buff + pos, len - pos);
                if (ret > 0)
                {
                    pos += ret;
                    if (pos == len)
                    {
                        return pos;
                    }
                }
                else if (ret == 0)
                {
                    printf("read error.[ret=%d]\n", ret);
                    return -1;
                }
                else if (ret == -1)
                {
                    if (errno != EAGAIN)
                    {
                        printf("read error.[ret=%d, errno=%d]\n", ret, errno);
                        return -1;
                    }
                }
            }
        }
    }

    return -1;
}

int mpu_acc_gryo_get(BMI_SENSOR_DATA *sensor_data)
{
	char acclbuf_16[16] = {0};
	char gyrobuf_16[16] = {0};
	int ret;

    if(sensor_data == NULL)
    {
        LOG_E("sensor data point is null\n");
        return -1;
    }

	ret = sensor_recv_data(sensor_data->acclfd, acclbuf_16, sizeof(acclbuf_16));
    if(ret != sizeof(acclbuf_16))
    {
        LOG_E("sensor recv data error.[ret=%d]\n", ret);
        return -1;
    }
	Vector3s *acc_tmp = (Vector3s *)acclbuf_16;

#ifndef IMU_ROTATE_X_90
	sensor_data->accdata[X_AXIS] = acc_tmp->x;
	sensor_data->accdata[Y_AXIS] = acc_tmp->y;
	sensor_data->accdata[Z_AXIS] = acc_tmp->z;
#else
	// sensor_data->accdata[X_AXIS] = acc_tmp->z;
	// sensor_data->accdata[Y_AXIS] = acc_tmp->x;
	// sensor_data->accdata[Z_AXIS] = acc_tmp->y;
	sensor_data->accdata[X_AXIS] = acc_tmp->x;
	sensor_data->accdata[Y_AXIS] = -acc_tmp->z;
	sensor_data->accdata[Z_AXIS] = acc_tmp->y;
#endif
	
	//printf("***********acc_tmp %d, %d,%d\n",acc_tmp->x,acc_tmp->y,acc_tmp->z);
    sensor_data->acc_mpu.x = sensor_data->accdata[0] * ACCL_SCALE - sensor_data->accl_offset.x;
	sensor_data->acc_mpu.y = sensor_data->accdata[1] * ACCL_SCALE - sensor_data->accl_offset.y;
	sensor_data->acc_mpu.z = sensor_data->accdata[2] * ACCL_SCALE - sensor_data->accl_offset.z;
    ret = sensor_recv_data(sensor_data->gyrofd, gyrobuf_16, sizeof(gyrobuf_16));
    if(ret != sizeof(gyrobuf_16))
    {
        LOG_E("sensor recv data error.[ret=%d]\n", ret);
        return -1;
    }

    Vector3s *gyro_tmp = (Vector3s *)gyrobuf_16;

#ifndef IMU_ROTATE_X_90
	sensor_data->gyrodata[X_AXIS] = gyro_tmp->x;
	sensor_data->gyrodata[Y_AXIS] = gyro_tmp->y;
	sensor_data->gyrodata[Z_AXIS] = gyro_tmp->z;
#else
	// sensor_data->gyrodata[X_AXIS] = gyro_tmp->z;
	// sensor_data->gyrodata[Y_AXIS] = gyro_tmp->x;
	// sensor_data->gyrodata[Z_AXIS] = gyro_tmp->y;
	sensor_data->gyrodata[X_AXIS] = gyro_tmp->x;
	sensor_data->gyrodata[Y_AXIS] = -gyro_tmp->z;
	sensor_data->gyrodata[Z_AXIS] = gyro_tmp->y;
#endif
	

    sensor_data->gyr_mpu.x = sensor_data->gyrodata[0] * GYRO_SCALE - sensor_data->gyro_offset.x;
	sensor_data->gyr_mpu.y = sensor_data->gyrodata[1] * GYRO_SCALE - sensor_data->gyro_offset.y;
	sensor_data->gyr_mpu.z = sensor_data->gyrodata[2] * GYRO_SCALE - sensor_data->gyro_offset.z;
	//printf("***********gyro_tmp %d, %d,%d\n",gyro_tmp->x,gyro_tmp->y,gyro_tmp->z);
    return 0;
}

void Delay_MPU(int ms)
{
	usleep(1 * ms);
}

void init_gyr(BMI_SENSOR_DATA *sensor_data)
{
	int i, j, k;
	float best_diff = 0, diff_norm = 0;
    Vector3f last_average, best_avg;
    Vector3f gyro_sum, gyro_avg, gyro_diff;

	vector3f_zero(&sensor_data->gyro_offset);

	for (i = 0; i < 20; i++)
	{
		Delay_MPU(1);
		mpu_acc_gryo_get(sensor_data);
		Delay_MPU(1);
	}

	vector3f_zero(&last_average);
	for (j = 0; j <= 5; j++)
	{
		vector3f_zero(&gyro_sum);
		for (k = 0; k < 100; k++)
		{
			mpu_acc_gryo_get(sensor_data);
			//取数据，然后累加
            gyro_sum = vector3f_add(&gyro_sum, &sensor_data->gyr_mpu);
			Delay_MPU(1);
		}
		//计算gyro的平均值，连续取100次数据的平均值
		gyro_avg = vector3f_uniform_scaling(&gyro_sum, k);

		gyro_diff = vector3f_sub(&last_average, &gyro_avg);

		diff_norm = vector3f_length(&gyro_diff);

		if (j == 0)
		{
			best_diff = diff_norm;
			best_avg = gyro_avg;
		}
		else if (vector3f_length(&gyro_diff) < ToRad(0.04f))
		{
			last_average.x = (gyro_avg.x * 0.5f) + (last_average.x * 0.5f);
			last_average.y = (gyro_avg.y * 0.5f) + (last_average.y * 0.5f);
			last_average.z = (gyro_avg.z * 0.5f) + (last_average.z * 0.5f);
			sensor_data->gyro_offset = last_average;
			//printf("gyro_offset2: x: %f, y: %f, z: %f\n", sensor_data->gyro_offset.x, sensor_data->gyro_offset.y, sensor_data->gyro_offset.z);
			return;
		}
		else if (diff_norm < best_diff)
		{
			best_diff = diff_norm;
			best_avg.x = (gyro_avg.x * 0.5f) + (last_average.x * 0.5f);
			best_avg.y = (gyro_avg.y * 0.5f) + (last_average.y * 0.5f);
			best_avg.z = (gyro_avg.z * 0.5f) + (last_average.z * 0.5f);
		}
		last_average = gyro_avg;
	}
	sensor_data->gyro_offset = best_avg;
	//printf("gyro_offset: x: %f, y: %f, z: %f\n", sensor_data->gyro_offset.x, sensor_data->gyro_offset.y, sensor_data->gyro_offset.z);
}

void AHRS_Init(BMI_SENSOR_DATA *sensor_data, Vector3f* Acc, Vector3f* Mag)
{
	float initialRoll, initialPitch;
	float cosRoll, sinRoll, cosPitch, sinPitch;
	float magX, magY;
	float initialHdg, cosHeading, sinHeading;

	initialRoll = atan2(Acc->y, Acc->z);
	initialPitch = atan2(-Acc->x, Acc->z);
	cosRoll = cosf(initialRoll);
	sinRoll = sinf(initialRoll);
	cosPitch = cosf(initialPitch);
	sinPitch = sinf(initialPitch);
	magX = Mag->x * cosPitch + Mag->y * sinRoll * sinPitch + Mag->z * cosRoll * sinPitch;
	magY = Mag->y * cosRoll - Mag->z * sinRoll;
	initialHdg = atan2f(-magY, magX);

	cosRoll = cosf(initialRoll * 0.5f);

	sinRoll = sinf(initialRoll * 0.5f);

	cosPitch = cosf(initialPitch * 0.5f);

	sinPitch = sinf(initialPitch * 0.5f);

	cosHeading = cosf(initialHdg * 0.5f);

	sinHeading = sinf(initialHdg * 0.5f);

	sensor_data->q0 = cosRoll * cosPitch * cosHeading + sinRoll * sinPitch * sinHeading;

	sensor_data->q1 = sinRoll * cosPitch * cosHeading - cosRoll * sinPitch * sinHeading;

	sensor_data->q2 = cosRoll * sinPitch * cosHeading + sinRoll * cosPitch * sinHeading;

	sensor_data->q3 = cosRoll * cosPitch * sinHeading - sinRoll * sinPitch * cosHeading;

	sensor_data->q0q0 = sensor_data->q0 * sensor_data->q0;

	sensor_data->q0q1 = sensor_data->q0 * sensor_data->q1;

	sensor_data->q0q2 = sensor_data->q0 * sensor_data->q2;

	sensor_data->q0q3 = sensor_data->q0 * sensor_data->q3;

	sensor_data->q1q1 = sensor_data->q1 * sensor_data->q1;

	sensor_data->q1q2 = sensor_data->q1 * sensor_data->q2;

	sensor_data->q1q3 = sensor_data->q1 * sensor_data->q3;

	sensor_data->q2q2 = sensor_data->q2 * sensor_data->q2;

	sensor_data->q2q3 = sensor_data->q2 * sensor_data->q3;

	sensor_data->q3q3 = sensor_data->q3 * sensor_data->q3;

	// printf("q0: %f, q1: %f, q2: %f, q3: %f\nq0q0: %f, q0q1: %f, q0q2: %f, q0q3: %f\n"
	// 		"q1q1: %f, q1q2: %f, q1q3: %f, q2q2: %f\nq2q3: %f, q3q3: %f\n",
	// 		sensor_data->q0, sensor_data->q1, sensor_data->q2, sensor_data->q3,
	// 		sensor_data->q0q0, sensor_data->q0q1, sensor_data->q0q2, sensor_data->q0q3,
	// 		sensor_data->q1q1, sensor_data->q1q2, sensor_data->q1q3,
	// 		sensor_data->q2q2, sensor_data->q2q3, sensor_data->q3q3);
}

void AHRS_Update(BMI_SENSOR_DATA *sensor_data, NFLOAT dtt)
{
    NFLOAT recipNorm;
    Vector3f* Gyr = &sensor_data->gyr_mpu;
    Vector3f* Acc = &sensor_data->acc_mpu;
    Vector3f* Mag = &sensor_data->mag_mpu;

	float dt = 0.01f;

	NFLOAT ixTerm = 0.0f, iyTerm = 0.0f, izTerm = 0.0f;

	NFLOAT halfex = 0.0f, halfey = 0.0f, halfez = 0.0f;

	NFLOAT eular[3];

	if (sensor_data->ahrs_init_count == 0)
	{
		AHRS_Init(sensor_data, Acc, Mag);
		sensor_data->ahrs_init_count = 1;
	}

	if (!((Mag->x == 0.0f) && (Mag->y == 0.0f) && (Mag->z == 0.0f)))
	{
		float hx, hy, hz, bx, bz;
		float halfwx, halfwy, halfwz;
		// Normalise magnetometer measurement
		recipNorm = invSqrt_(Mag->x * Mag->x + Mag->y * Mag->y + Mag->z * Mag->z);

		Mag->x *= recipNorm;
		Mag->y *= recipNorm;
		Mag->z *= recipNorm;
		// Reference direction of Earth's magnetic field
		// Vmag(earth) = dcmMatrix[3X3] * Vmag(body)
		hx = 2.0f * (Mag->x * (0.5f - sensor_data->q2q2 - sensor_data->q3q3) + Mag->y * (sensor_data->q1q2 - sensor_data->q0q3) +
					 Mag->z * (sensor_data->q1q3 + sensor_data->q0q2));
		hy = 2.0f * (Mag->x * (sensor_data->q1q2 + sensor_data->q0q3) + Mag->y * (0.5f - sensor_data->q1q1 - sensor_data->q3q3) +
					 Mag->z * (sensor_data->q2q3 - sensor_data->q0q1));
		hz = 2.0f * (Mag->x * (sensor_data->q1q3 - sensor_data->q0q2) + Mag->y * (sensor_data->q2q3 + sensor_data->q0q1) +
					 Mag->z * (0.5f - sensor_data->q1q1 - sensor_data->q2q2));
		bx = sqrt(hx * hx + hy * hy);
		bz = hz;

		// Estimated direction of magnetic field
		// Vmag(body) = dcmMatrix[3X3]' * Vmag(earth)
		halfwx = bx * (0.5f - sensor_data->q2q2 - sensor_data->q3q3) + bz * (sensor_data->q1q3 - sensor_data->q0q2);
		halfwy = bx * (sensor_data->q1q2 - sensor_data->q0q3) + bz * (sensor_data->q0q1 + sensor_data->q2q3);
		halfwz = bx * (sensor_data->q0q2 + sensor_data->q1q3) + bz * (0.5f - sensor_data->q1q1 - sensor_data->q2q2);
		// Error is sum of cross product between estimated direction and measured direction of field vectors
		halfex += (Mag->y * halfwz - Mag->z * halfwy);
		halfey += (Mag->z * halfwx - Mag->x * halfwz);
		halfez += (Mag->x * halfwy - Mag->y * halfwx);
	}
	// Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if (!((Acc->x == 0.0f) && (Acc->y == 0.0f) && (Acc->z == 0.0f)))
	{
		NFLOAT halfvx, halfvy, halfvz;
		// Normalise accelerometer measurement
		recipNorm = invSqrt_(Acc->x * Acc->x + Acc->y * Acc->y + Acc->z * Acc->z);
		Acc->x *= recipNorm;
		Acc->y *= recipNorm;
		Acc->z *= recipNorm;
		// Estimated direction of gravity and magnetic field
		// Vg(body) = dcmMatrix[3X3]' * Vg(earth)
		// Vg(earth) = [0 0 1]
		halfvx = sensor_data->q1q3 - sensor_data->q0q2;
		halfvy = sensor_data->q0q1 + sensor_data->q2q3;
		halfvz = sensor_data->q0q0 - 0.5f + sensor_data->q3q3;
		// Error is sum of cross product between estimated direction and measured direction of field vectors,
		halfex = Acc->y * halfvz - Acc->z * halfvy;
		halfey = Acc->z * halfvx - Acc->x * halfvz;
		halfez = Acc->x * halfvy - Acc->y * halfvx;
		ixTerm += halfex;
		iyTerm += halfey;
		izTerm += halfez;
	}
	// Apply feedback only when valid data has been gathered from the accelerometer or magnetometer
	if (ixTerm != 0.0f && iyTerm != 0.0f && izTerm != 0.0f)
	{
		// Compute and apply integral feedback if enabled
		if (twoKi > 0.0f)
		{
			// integral error scaled by Ki
#if USE_CHANGE_I_RATE
			sensor_data->gyro_bias[0] += twoKi * ixTerm * change_integration_rate(halfex) * dt;
			sensor_data->gyro_bias[1] += twoKi * iyTerm * change_integration_rate(halfey) * dt;
			sensor_data->gyro_bias[2] += twoKi * izTerm * change_integration_rate(halfez) * dt;
#else
			sensor_data->gyro_bias[0] += twoKi * ixTerm * dt;
			sensor_data->gyro_bias[1] += twoKi * iyTerm * dt;
			sensor_data->gyro_bias[2] += twoKi * izTerm * dt;
#endif
			// apply integral feedback
			Gyr->x += sensor_data->gyro_bias[0];
			Gyr->y += sensor_data->gyro_bias[1];
			Gyr->z += sensor_data->gyro_bias[2];
		}
		else
		{
			sensor_data->gyro_bias[0] = 0.0f; // prevent integral windup
			sensor_data->gyro_bias[1] = 0.0f;
			sensor_data->gyro_bias[2] = 0.0f;
		}
		// Apply proportional feedback

#if USE_CHANGE_P_RATE
		Gyr->x += twoKp * variable_parameter(halfex) * halfex;
		Gyr->y += twoKp * variable_parameter(halfey) * halfey;
		Gyr->z += twoKp * variable_parameter(halfez) * halfez;
#else
		Gyr->x += twoKp * ixTerm;
		Gyr->y += twoKp * iyTerm;
		Gyr->z += twoKp * izTerm;
#endif
	}
	// Time derivative of quaternion. q_dot = 0.5*q\otimes omega.
	//! q_k = q_{k-1} + dt*\dot{q}
	//! \dot{q} = 0.5*q \otimes P(\omega)
	sensor_data->dq0 = 0.5f * (-sensor_data->q1 * Gyr->x - sensor_data->q2 * Gyr->y - sensor_data->q3 * Gyr->z);
	sensor_data->dq1 = 0.5f * (sensor_data->q0 * Gyr->x + sensor_data->q2 * Gyr->z - sensor_data->q3 * Gyr->y);
	sensor_data->dq2 = 0.5f * (sensor_data->q0 * Gyr->y - sensor_data->q1 * Gyr->z + sensor_data->q3 * Gyr->x);
	sensor_data->dq3 = 0.5f * (sensor_data->q0 * Gyr->z + sensor_data->q1 * Gyr->y - sensor_data->q2 * Gyr->x);
	sensor_data->q0 += dt * sensor_data->dq0;
	sensor_data->q1 += dt * sensor_data->dq1;
	sensor_data->q2 += dt * sensor_data->dq2;
	sensor_data->q3 += dt * sensor_data->dq3;
	// Normalise quaternion
	recipNorm = invSqrt_(sensor_data->q0 * sensor_data->q0 + sensor_data->q1 * sensor_data->q1 + sensor_data->q2 * sensor_data->q2 +
						 sensor_data->q3 * sensor_data->q3);
	sensor_data->q0 *= recipNorm;
	sensor_data->q1 *= recipNorm;
	sensor_data->q2 *= recipNorm;
	sensor_data->q3 *= recipNorm;
	// Auxiliary variables to avoid repeated arithmetic
	sensor_data->q0q0 = sensor_data->q0 * sensor_data->q0;
	sensor_data->q0q1 = sensor_data->q0 * sensor_data->q1;
	sensor_data->q0q2 = sensor_data->q0 * sensor_data->q2;
	sensor_data->q0q3 = sensor_data->q0 * sensor_data->q3;
	sensor_data->q1q1 = sensor_data->q1 * sensor_data->q1;
	sensor_data->q1q2 = sensor_data->q1 * sensor_data->q2;
	sensor_data->q1q3 = sensor_data->q1 * sensor_data->q3;
	sensor_data->q2q2 = sensor_data->q2 * sensor_data->q2;
	sensor_data->q2q3 = sensor_data->q2 * sensor_data->q3;
	sensor_data->q3q3 = sensor_data->q3 * sensor_data->q3;
	// Convert q->R, This R converts inertial frame to body frame.
	sensor_data->dcm_matrix.a.x = sensor_data->q0q0 + sensor_data->q1q1 - sensor_data->q2q2 - sensor_data->q3q3; // 11
	sensor_data->dcm_matrix.a.y = 2.f * (sensor_data->q1q2 - sensor_data->q0q3); // 12
	sensor_data->dcm_matrix.a.z = 2.f * (sensor_data->q1q3 + sensor_data->q0q2); // 13
	sensor_data->dcm_matrix.b.x = 2.f * (sensor_data->q1q2 + sensor_data->q0q3); // 21
	sensor_data->dcm_matrix.b.y = sensor_data->q0q0 - sensor_data->q1q1 + sensor_data->q2q2 - sensor_data->q3q3; // 22
	sensor_data->dcm_matrix.b.z = 2.f * (sensor_data->q2q3 - sensor_data->q0q1); // 23
	sensor_data->dcm_matrix.c.x = 2.f * (sensor_data->q1q3 - sensor_data->q0q2); // 31
	sensor_data->dcm_matrix.c.y = 2.f * (sensor_data->q2q3 + sensor_data->q0q1); // 32
	sensor_data->dcm_matrix.c.z = sensor_data->q0q0 - sensor_data->q1q1 - sensor_data->q2q2 + sensor_data->q3q3; // 33
	eular[0] = atan2f(sensor_data->dcm_matrix.c.y, sensor_data->dcm_matrix.c.z); //! Roll
	eular[1] = -safe_asin(sensor_data->dcm_matrix.c.x); //! Pitch
	eular[2] = atan2f(sensor_data->dcm_matrix.b.x, sensor_data->dcm_matrix.a.x); //! Yaw
	//printf("[0]: %f, [1]: %f, [2]: %f\n", eular[0], eular[1], eular[2]);
	if (isfinite(eular[0]) && isfinite(eular[1]) && isfinite(eular[2]))
	{
		// roll pitch
		//		pitch	=	R2D(eular[0]);
		//		roll	=	R2D(eular[1]);
		//roll pitch
		sensor_data->pitch = R2D(eular[1]);
		sensor_data->roll = R2D(eular[0]);
		//sensor_data->yaw	=	1.75f*R2D(eular[2]);
		sensor_data->curYaw = 1.54f * R2D(eular[2]);
		sensor_data->errYaw = sensor_data->curYaw - sensor_data->preYaw;
		if ((sensor_data->errYaw > 100) || (sensor_data->errYaw < -100))
		{
			sensor_data->yaw += sensor_data->curYaw + sensor_data->preYaw;
		}
		else
		{
			sensor_data->yaw += sensor_data->curYaw - sensor_data->preYaw;
		}
		if (sensor_data->yaw > 360)
        {
			sensor_data->yaw -= 2 * 360.0f;
        }
		else if (sensor_data->yaw < -360)
        {
			sensor_data->yaw += 2 * 360.0f;
        }
		sensor_data->preYaw = sensor_data->curYaw;
		//
	}
}

void * thread_sensor_data_update(void *arg)
{
    BMI_SENSOR_DATA *sensor_data;
    NFLOAT attUpdateTime = 0.002f;

    pthread_detach(pthread_self());

    if(arg == NULL)
    {
        LOG_E("thread argument is null.\n");
        return NULL;
    }

    sensor_data = (BMI_SENSOR_DATA*)arg;

    init_gyr(sensor_data);

    for(;;)
    {
        mpu_acc_gryo_get(sensor_data);
		attUpdateTime = 0.01f;
        AHRS_Update(sensor_data, attUpdateTime);
        // LOG_D("============================sensor pitch  %4f, roll %4f, yaw %4f", sensor_data->pitch, sensor_data->roll, sensor_data->yaw);
        // LOG_D("============x: %d, y: %d, z: %d, x1: %d, y1: %d, z1: %d", sensor_data->accdata[X_AXIS], sensor_data->accdata[Y_AXIS], sensor_data->accdata[Z_AXIS],
       	 							// sensor_data->gyrodata[X_AXIS], sensor_data->gyrodata[Y_AXIS], sensor_data->gyrodata[Z_AXIS]);
		// usleep(100*1000);
    }
    return NULL;
}

int gsensor_init()
{
    int ret;
    pthread_t ptd;

    memset(&g_sensor_data, 0, sizeof(BMI_SENSOR_DATA));
	ql_gsensor_on();


    sensor_data_init(&g_sensor_data);

    g_sensor_data.acclfd = open(NIU_DEVICE_ACCL, O_RDWR);
    if(g_sensor_data.acclfd < 0)
    {
        LOG_E("open `%s` failed.", NIU_DEVICE_ACCL);
        return -1;
    }

    g_sensor_data.gyrofd = open(NIU_DEVICE_GYRO, O_RDWR);
    if(g_sensor_data.gyrofd < 0)
    {
        LOG_E("open `%s` failed.", NIU_DEVICE_GYRO);
        return -1;
    }

    ret = pthread_create(&ptd, NULL, thread_sensor_data_update, (void*)&g_sensor_data);
    if(ret)
    {
        LOG_E("create thread thread_sensor_data_update failed.");
        return -1;
    }
    return 0;
}

int gsensor_data_read(BMI_SENSOR_DATA *sensor_data)
{
    if(sensor_data)
    {
        memcpy(sensor_data, &g_sensor_data, sizeof(BMI_SENSOR_DATA));
        return 0;
    }
    return -1;
}