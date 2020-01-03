#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ql_oe.h>
#include "ql_mem.h"
#include "ql_common.h"
#include "nmea_parse.h"


#define ARRAY_LENGTH(a) (sizeof a / sizeof (a[0]))



NINT8 nmea_get_checksum(const char *sentence)
{
	const char *n = sentence + 1;
	NINT8 chk = 0;

	/* While current char isn't '*' or sentence ending (newline) */
	while ('*' != *n && NMEA_END_CHAR_1 != *n && '\0' != *n) {
		chk ^= (NINT8) *n;
		n++;
	}

	return chk;
}


int nmea_has_checksum(const char *sentence, int length)
{
	if ('*' == sentence[length - 5]) {
		return 0;
	}

	return -1;
}

int nmea_validate(const char *sentence, int length, int check_checksum)
{
	const char *n;

	/* should have atleast 9 characters */
	if (9 > length) {
		return -1;
	}

	/* should be less or equal to 82 characters */
	if (NMEA_MAX_LENGTH < length) {
		return -1;
	}

	/* should start with $ */
	if ('$' != *sentence) {
		return -1;
	}

	/* should end with \r\n, or other... */
	if (NMEA_END_CHAR_2 != sentence[length - 1] || NMEA_END_CHAR_1 != sentence[length - 2]) {
		return -1;
	}

	/* should have a 5 letter, uppercase word */
	n = sentence;
	while (++n < sentence + 6) {
		if (*n < 'A' || *n > 'Z') {
			/* not uppercase letter */
			return -1;
		}
	}

	/* should have a comma after the type word */
	if (',' != sentence[6]) {
		return -1;
	}

	/* check for checksum */
	if (1 == check_checksum && 0 == nmea_has_checksum(sentence, length)) {
		NINT8 actual_chk;
		NINT8 expected_chk;
		char checksum[3];

		checksum[0] = sentence[length - 4];
		checksum[1] = sentence[length - 3];
		checksum[2] = '\0';
		actual_chk = nmea_get_checksum(sentence);
		expected_chk = (NINT8) strtol(checksum, NULL, 16);
		if (expected_chk != actual_chk) {
			return -1;
		}
	}

	return 0;
}

/**
 * Crop a sentence from the type word and checksum.
 *
 * The type word at the beginning along with the dollar sign ($) will be
 * removed. If there is a checksum, it will also be removed. The two end
 * characters (usually <CR><LF>) will not be included in the new string.
 *
 * sentence is a validated NMEA sentence string.
 * length is the char length of the sentence string.
 *
 * Returns pointer (char *) to the new string.
 */
static char * _crop_sentence(char *sentence, size_t length)
{
	/* Skip type word, 7 characters (including $ and ,) */

	sentence += NMEA_PREFIX_LENGTH + 2;

	/* Null terminate before end of line/sentence, 2 characters */
	sentence[length - 9] = '\0';

	/* Remove checksum, if there is one */

	if ('*' == sentence[length - 12]) {
		sentence[length - 12] = '\0';
	}

	return sentence;
}



/**
 * Splits a string by comma.
 *
 * string is the string to split, will be manipulated. Needs to be
 *        null-terminated.
 * values is a char pointer array that will be filled with pointers to the
 *        splitted values in the string.
 * max_values is the maximum number of values to be parsed.
 *
 * Returns the number of values found in string.
 */
static int _split_string_by_comma(char *string, char **values, int max_values)
{
	int i = 0;

	values[i++] = string;
	while (i < max_values && NULL != (string = strchr(string, ','))) {
		*string = '\0';
		values[i++] = ++string;
	}

	return i;
}

nmea_type nmea_get_type(const char *sentence)
{
    if (strncmp(sentence + 3, "RMC", 3) == 0)
	{
        return NMEA_RMC;
	}
    else if (strncmp(sentence + 3, "GGA", 3) == 0)
	{
        return NMEA_GGA;
	}
    else if (strncmp(sentence + 3, "GSA", 3) == 0)
	{
        return NMEA_GSA;
	}
    else if (strncmp(sentence + 3, "GSV", 3) == 0)
	{
        return NMEA_GSV;
	}
	else if (strncmp(sentence + 3, "TXT", 3) == 0)
	{
		return NMEA_TXT;
	}
    return NMEA_UNKNOWN;
}

/**
 * Check if a value is not NULL and not empty.
 *
 * Returns 0 if set, otherwise -1.
 */
static int _is_value_set(const char *value)
{
	if (NULL == value || '\0' == *value) {
		return -1;
	}

	return 0;
}

int nmea_position_parse(char *s, struct nmea_position *pos)
{
	char *cursor;

	pos->degrees = 0;
	pos->minutes = 0;

	if (s == NULL || *s == '\0') {
		return -1;
	}

	/* decimal minutes */
	if (NULL == (cursor = strchr(s, '.'))) {
		return -1;
	}

	/* minutes starts 2 digits before dot */
	cursor -= 2;
	pos->minutes = atof(cursor);
	*cursor = '\0';

	/* integer degrees */
	cursor = s;
	pos->degrees = atoi(cursor);

	return 0;
}

nmea_cardinal_t nmea_cardinal_direction_parse(char *s)
{
	if (NULL == s || '\0'== *s) {
		return NMEA_CARDINAL_DIR_UNKNOWN;
	}

	switch (*s) {
	case NMEA_CARDINAL_DIR_NORTH:
		return NMEA_CARDINAL_DIR_NORTH;
	case NMEA_CARDINAL_DIR_EAST:
		return NMEA_CARDINAL_DIR_EAST;
	case NMEA_CARDINAL_DIR_SOUTH:
		return NMEA_CARDINAL_DIR_SOUTH;
	case NMEA_CARDINAL_DIR_WEST:
		return NMEA_CARDINAL_DIR_WEST;
	default:
		break;
	}

	return NMEA_CARDINAL_DIR_UNKNOWN;
}

int nmea_validate_parse(char *s)
{
	if (NULL == s || '\0'== *s) {
		return 0;
	}
	return (*s == 'A');
}

int nmea_time_parse(char *s, struct tm *time)
{
	//char *rv;

	memset(time, 0, sizeof (struct tm));

	if (s == NULL || *s == '\0') {
		return -1;
	}

	strptime(s, NMEA_TIME_FORMAT, time);
	// if (NULL == rv || (int) (rv - s) != NMEA_TIME_FORMAT_LEN) {
	// 	return -1;
	// }

	return 0;
}

int nmea_date_parse(char *s, struct tm *time)
{
	//char *rv;

	// Assume it has been already cleared
	// memset(time, 0, sizeof (struct tm));

	if (s == NULL || *s == '\0') {
		return -1;
	}

	strptime(s, NMEA_DATE_FORMAT, time);
	// if (NULL == rv || (int) (rv - s) != NMEA_DATE_FORMAT_LEN) {
	// 	return -1;
	// }

	return 0;
}

/**
 * parse nmea $GNTXT for fwver
 * $GNTXT,01,01,02,FWVER=UDR 1.00*42
 */ 
int nmea_parse_txt(struct nmea_s *nmea, char *sentence)
{
	unsigned int n_vals, val_index;
	char *value;
	char *values[255];
	struct nmea_gntxt_s *data = NULL;

	/* Split the sentence into values */
	n_vals = _split_string_by_comma(sentence, values, ARRAY_LENGTH(values));
	if (0 == n_vals)
	{
		return -1;
	}
	data = QL_MEM_ALLOC(sizeof(struct nmea_gntxt_s));
	if (data == NULL)
	{
		LOG_E("nmea data malloc error.");
		return -1;
	}
	memset(data, 0, sizeof(struct nmea_gntxt_s));
	for (val_index = 0; val_index < n_vals; val_index++)
	{
		value = values[val_index];
		if (-1 == _is_value_set(value))
		{
			continue;
		}

		switch (val_index)
		{
			case NMEA_GNTXT_FWVER:
			{
				if(strncasecmp(value, "FWVER", 5)  == 0)
				{
					strncpy(data->fwver, value + 6, sizeof(data->fwver));
				}
			}
			break;
			default:
			break;
		}
	}
	nmea->data = data;
	return 0;
}

int nmea_parse_rmc(struct nmea_s *nmea, char *sentence)
{
	unsigned int n_vals, val_index;
	char *value;
	char *values[255];
	struct nmea_gprmc_s *data = NULL;

	/* Split the sentence into values */
	n_vals = _split_string_by_comma(sentence, values, ARRAY_LENGTH(values));
	if (0 == n_vals)
	{
		return -1;
	}
	//printf("n_vals: %d\n", n_vals);
	data = QL_MEM_ALLOC(sizeof(struct nmea_gprmc_s));
	if (data == NULL)
	{
		LOG_E("rmc nmea data malloc error.");
		return -1;
	}
	memset(data, 0, sizeof(struct nmea_gprmc_s));

	for (val_index = 0; val_index < n_vals; val_index++)
	{
		value = values[val_index];
		if (-1 == _is_value_set(value))
		{
			continue;
		}
		switch (val_index)
		{
		case NMEA_GPRMC_TIME:
			/* Parse time */
			if (-1 == nmea_time_parse(value, &data->time))
			{
				goto _error;
			}
			break;
		case NMEA_GPRMC_STATUS:
			data->valid = nmea_validate_parse(value);
			break;
		case NMEA_GPRMC_LATITUDE:
			/* Parse latitude */
			if (-1 == nmea_position_parse(value, &data->latitude))
			{
				goto _error;
			}
			break;

		case NMEA_GPRMC_LATITUDE_CARDINAL:
			/* Parse cardinal direction */
			data->latitude.cardinal = nmea_cardinal_direction_parse(value);
			if (NMEA_CARDINAL_DIR_UNKNOWN == data->latitude.cardinal)
			{
				goto _error;
			}
			break;

		case NMEA_GPRMC_LONGITUDE:
			/* Parse longitude */
			if (-1 == nmea_position_parse(value, &data->longitude))
			{
				goto _error;
			}
			break;

		case NMEA_GPRMC_LONGITUDE_CARDINAL:
			/* Parse cardinal direction */
			data->longitude.cardinal = nmea_cardinal_direction_parse(value);
			if (NMEA_CARDINAL_DIR_UNKNOWN == data->longitude.cardinal)
			{
				goto _error;
			}
			break;

		case NMEA_GPRMC_DATE:
			/* Parse date */
			if (-1 == nmea_date_parse(value, &data->time))
			{
				goto _error;
			}
			break;
		case NMEA_GPRMC_COURSE:
			data->course = atof(value);
			break;
		case NMEA_GPRMC_SPEED:
			/* Parse ground speed in knots */
			data->speed = atof(value);
			break;

		default:
			break;
		}
	}
	nmea->data = data;
	return 0;
_error:
	if (data)
	{
		QL_MEM_FREE(data);
		data = NULL;
	}
	return -1;
}

int nmea_parse_gsv(struct nmea_s *nmea, char *sentence)
{
	unsigned int n_vals, val_index;
	char *value;
	char *values[255];
	struct nmea_gpgsv_s *data = NULL;

	/* Split the sentence into values */
	n_vals = _split_string_by_comma(sentence, values, ARRAY_LENGTH(values));
	if (0 == n_vals)
	{
		return -1;
	}
	data = QL_MEM_ALLOC(sizeof(struct nmea_gpgsv_s));
	if (data == NULL)
	{
		LOG_E("nmea data malloc error.");
		return -1;
	}
	memset(data, 0, sizeof(struct nmea_gpgsv_s));
	for (val_index = 0; val_index < n_vals; val_index++)
	{
		value = values[val_index];
		if (-1 == _is_value_set(value))
		{
			continue;
		}
		switch (val_index)
		{
			case NMEA_GPGSV_TOTAL_MSGS:
				data->total_msgs = atoi(value);
				break;
			case NMEA_GPGSV_MSG_NR:
				data->msg_nr = atoi(value);
				break;
			case NMEA_GPGSV_SATS:
				data->total_sats = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_NR1:
				data->sats[0].nr = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_ELEV1:
				data->sats[0].elevation = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_AZIMUTH1:
				data->sats[0].azimuth = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_SNR1:
				data->sats[0].snr = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_NR2:
				data->sats[1].nr = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_ELEV2:
				data->sats[1].elevation = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_AZIMUTH2:
				data->sats[1].azimuth = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_SNR2:
				data->sats[1].snr = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_NR3:
				data->sats[2].nr = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_ELEV3:
				data->sats[2].elevation = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_AZIMUTH3:
				data->sats[2].azimuth = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_SNR3:
				data->sats[2].snr = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_NR4:
				data->sats[3].nr = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_ELEV4:
				data->sats[3].elevation = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_AZIMUTH4:
				data->sats[3].azimuth = atoi(value);
				break;
			case NMEA_GPGSV_SAT_INFO_SNR4:
				data->sats[3].snr = atoi(value);
				break;
			default:
				break;
		}
	}
	nmea->data = data;
	return 0;
}

int nmea_parse_gga(struct nmea_s *nmea, char *sentence)
{
	unsigned int n_vals, val_index;
	char *value;
	char *values[255];
	struct nmea_gpgga_s *data = NULL;

	/* Split the sentence into values */
	n_vals = _split_string_by_comma(sentence, values, ARRAY_LENGTH(values));
	if (0 == n_vals)
	{
		return -1;
	}

	data = QL_MEM_ALLOC(sizeof(struct nmea_gpgga_s));
	if (data == NULL)
	{
		LOG_E("gga nmea data malloc error.");
		return -1;
	}
	memset(data, 0, sizeof(struct nmea_gpgga_s));
	for (val_index = 0; val_index < n_vals; val_index++)
	{
		value = values[val_index];

		if (-1 == _is_value_set(value))
		{
			continue;
		}
		switch (val_index)
		{
			case NMEA_GPGGA_LATITUDE:
				if (-1 == nmea_position_parse(value, &data->latitude))
				{
					goto _error;
				}
				break;
			case NMEA_GPGGA_LATITUDE_CARDINAL:
				data->latitude.cardinal = nmea_cardinal_direction_parse(value);
				if (NMEA_CARDINAL_DIR_UNKNOWN == data->latitude.cardinal)
				{
					goto _error;
				}
				break;
			case NMEA_GPGGA_LONGITUDE:
				if (-1 == nmea_position_parse(value, &data->longitude))
				{
					goto _error;
				}
				break;
			case NMEA_GPGGA_LONGITUDE_CARDINAL:
				data->longitude.cardinal = nmea_cardinal_direction_parse(value);
				if (NMEA_CARDINAL_DIR_UNKNOWN == data->longitude.cardinal)
				{
					goto _error;
				}
				break;
			case NMEA_GPGGA_STATUS:
				data->status = atoi(value);
				break;
			case NMEA_GPGGA_SATELLITES_TRACKED:
				data->satellites_tracked = atoi(value);
				break;
			case NMEA_GPGGA_HDOP:
				data->hdop = atof(value);
				break;
			case NMEA_GPGGA_ALTITUDE:
				data->altitude = atof(value);
				break;
			default:
				break;
		}
	}
	nmea->data = data;
	return 0;
_error:
	if (data)
	{
		QL_MEM_FREE(data);
		data = NULL;
	}
	return -1;
}

int nmea_parse_gsa(struct nmea_s *nmea, char *sentence)
{
	unsigned int n_vals, val_index;
	char *value;
	char *values[255];
	struct nmea_gpgsa_s *data = NULL;

	/* Split the sentence into values */
	n_vals = _split_string_by_comma(sentence, values, ARRAY_LENGTH(values));
	if (0 == n_vals)
	{
		return -1;
	}

	data = QL_MEM_ALLOC(sizeof(struct nmea_gpgsa_s));
	if (data == NULL)
	{
		LOG_E("gsa nmea data malloc error.");
		return -1;
	}
	memset(data, 0, sizeof(struct nmea_gpgsa_s));
	for (val_index = 0; val_index < n_vals; val_index++)
	{
		value = values[val_index];

		if (-1 == _is_value_set(value))
		{
			continue;
		}
		switch (val_index)
		{
			case NMEA_GPGSA_MODE:
				data->mode = *value;
				break;
			case NMEA_GPGSA_TYPE:
				data->type = atoi(value);
				break;
			case NMEA_GPGSA_PDOP:
				data->pdop = atof(value);
				break;
			case NMEA_GPGSA_HDOP:
				data->hdop = atof(value);
				break;
			case NMEA_GPGSA_VDOP:
				data->vdop = atof(value);
				break;
			default:
				break;
		}
	}
	nmea->data = data;
	return 0;
}

struct nmea_s* nmea_parse(char *sentence, int length, int check_checksum)
{
	int ret;
    nmea_type type;
	char *val_string;
	struct nmea_s * nmea = NULL;
    
	/* Validate sentence string */
	if (-1 == nmea_validate(sentence, length, check_checksum)) {
		//printf("nmea validate false!\n");
		return NULL;
	}

    type = nmea_get_type(sentence);
	if (NMEA_UNKNOWN == type) {
		//printf("nmea get type unknown!\n");
		return NULL;
	}

	/* Crop sentence from type word and checksum */
	val_string = _crop_sentence(sentence, length);
	if (NULL == val_string) {
		LOG_E("_crop_sentence failed!");
	    return NULL;
	}

	nmea = QL_MEM_ALLOC(sizeof(struct nmea_s));
	if(nmea == NULL)
	{
		LOG_E("nmea malloc error.");
		return NULL;
	}
	memset(nmea, 0, sizeof(struct nmea_s));
	nmea->type = type;

	switch(type)
	{
		case NMEA_GGA:
		{
			ret = nmea_parse_gga(nmea, val_string);
			if(ret)
			{
				LOG_E("nmea_parse_gga failed.");
				goto _error;
			}
		}
		break;
		case NMEA_GSA:
		{
			ret = nmea_parse_gsa(nmea, val_string);
			if(ret)
			{
				LOG_E("nmea_parse_gsa failed.");
				goto _error;
			}
		}
		break;
		case NMEA_GSV:
		{
			ret = nmea_parse_gsv(nmea, val_string);
			if(ret)
			{
				LOG_E("nmea_parse_gsv failed.");
				goto _error;
			}
		}
		break;
		case NMEA_RMC:
		{
			ret = nmea_parse_rmc(nmea, val_string);
			if(ret)
			{
				LOG_E("nmea_parse_rmc failed.");
				goto _error;
			}
		}
		break;
		case NMEA_TXT:
		{
			ret = nmea_parse_txt(nmea, val_string);
			if(ret)
			{
				LOG_E("nmea_parse_txt failed.");
				goto _error;
			}
		}
		default:
			break;
	}
	return nmea;
_error:
	if(nmea)
	{
		if(nmea->data)
		{
			QL_MEM_FREE(nmea->data);
		}
		QL_MEM_FREE(nmea);
		nmea = NULL;
	}
	return NULL;
}