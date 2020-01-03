#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ql_oe.h>
#include "ql_mem.h"
#include "ql_common.h"
#include "gps_data.h"
#include "nmea_parse.h"
#include "niu_packet.h"
#include "service.h"
#include "utils.h"
#include "niu_thread.h"

static struct niu_gps_data g_gps_data = {0};
static int g_gps_fd = -1;
static int g_gps_gsv_total = 0;

int nmea_value_update(struct nmea_s *nmea, struct niu_gps_data *gps_data)
{
    int i = 0;
    struct nmea_gprmc_s *rmc = NULL;
    struct nmea_gpgsv_s *gsv = NULL;
    struct nmea_gpgga_s *gga = NULL;
    struct nmea_gpgsa_s *gsa = NULL;
    struct nmea_gntxt_s *txt = NULL;

    if(nmea == NULL || gps_data == NULL)
    {
        LOG_E("param invalid.");
        return -1;
    }
    if(nmea->type == NMEA_UNKNOWN)
    {
        LOG_E("gps nmea type is unknown!");
        return -1;
    }
    switch(nmea->type)
    {
        case NMEA_RMC:
            rmc = (struct nmea_gprmc_s*)nmea->data;
            if(rmc)
            {
                gps_data->valid = rmc->valid;
                gps_data->longitude = rmc->longitude.degrees + (rmc->longitude.minutes / 60);
                gps_data->longitude_cardinal = rmc->longitude.cardinal;
                gps_data->latitude = rmc->latitude.degrees + (rmc->latitude.minutes / 60);
                gps_data->latitude_cardinal = rmc->latitude.cardinal;
                gps_data->heading = rmc->course;
                gps_data->gps_speed = rmc->speed * KNOTS_CONVERSION_FACTOR;
            }
        break;
        case NMEA_GSV:
            gsv = (struct nmea_gpgsv_s*)nmea->data;
            if(gsv)
            {
                #if 1
                if(gsv->total_sats < QL_GSV_MAX_SATS)
                {
                    for(i = 0; i < 4; i++)
                    {
                        gps_data->cnrs[gps_data->cnrs_index] = gsv->sats[i].snr;
                        gps_data->cnrs_index++;
                    }
                    // LOG_D("gps_data->cnrs_index : %d", gps_data->cnrs_index);
                    if(gsv->total_msgs == gsv->msg_nr)
                    {
                        char buff_snrs[256];
                        int pos = 0, total = 0;
                        bubbleSort(gps_data->cnrs, QL_GSV_MAX_SATS);
                        for(i = 0; i < gps_data->cnrs_index; i++)
                        {
                            pos += sprintf(buff_snrs + pos, "%d ", gps_data->cnrs[i]);
                        }
                        //LOG_D("snrs: %s", buff_snrs);
                        gps_data->max_cnr = gps_data->cnrs[0];
                        gps_data->max_cnr2 = gps_data->cnrs[1];

                        total += gps_data->cnrs[0];
                        total += gps_data->cnrs[1];
                        total += gps_data->cnrs[2];
                        total += gps_data->cnrs[3];
                        gps_data->avg_cnr = total / 4;
                        gps_data->gps_signal = gps_data->avg_cnr / 7;
                        if(gps_data->gps_signal > 5)
                        {
                            gps_data->gps_signal = 5;
                        }

                        gps_data->min_cnr = 0;
                        for(i = gps_data->cnrs_index - 1; i >= 0; i--)
                        {
                            if(gps_data->cnrs[i] > 0)
                            {
                                gps_data->min_cnr = gps_data->cnrs[i];
                                break;
                            }
                        }
                        gps_data->satellites_num = gps_data->cnrs_index;


                        gps_data->cnrs_index = 0;
                    }
                }
                else
                {
                    LOG_E("gsv total sats more than QL_GSV_MAX_SATS!!!!![current: %d, max: %d]", gsv->total_sats, QL_GSV_MAX_SATS);
                }
                #else
                NUINT32 max_cnr = 0, max_cnr2 = 0, min_cnr = 0, avg_cnr = 0;
                NUINT32 cnr_value, total_cnr = 0, snr_num = 0;

                if(g_gps_gsv_total > 10)
                {
                    g_gps_gsv_total = 0;
                    max_cnr = 0;
                    max_cnr2 = 0;
                    avg_cnr = 0;
                }
                else
                {
                    //g_gps_gsv_total++;
                    max_cnr = gps_data->max_cnr;
                    max_cnr2 = gps_data->max_cnr2;
                    avg_cnr = gps_data->avg_cnr;
                }
                gps_data->satellites_num = gsv->total_sats;
                for(i = 0; i < 4; i++)
                {
                    cnr_value = gsv->sats[i].snr;
                    max_cnr = jmax(max_cnr, cnr_value);
                    if(cnr_value > max_cnr2 && cnr_value < max_cnr)
                    {
                        max_cnr2 = cnr_value;
                    }
                    if(cnr_value > 0)
                    {
                        min_cnr = min_cnr > 0 ? (jmin(min_cnr, cnr_value)) : (cnr_value);
                        total_cnr += cnr_value;
                        snr_num ++;
                    }
                }
                
                if(snr_num > 0)
                {
                    avg_cnr = (avg_cnr * g_gps_gsv_total +  total_cnr / snr_num) / (g_gps_gsv_total + 1);
                }
                g_gps_gsv_total++;
                gps_data->max_cnr = max_cnr;
                gps_data->max_cnr2 = max_cnr2;
                gps_data->min_cnr = min_cnr;
                gps_data->avg_cnr = avg_cnr;

                gps_data->gps_signal = avg_cnr / 7;
                if(gps_data->gps_signal > 5)
                {
                    gps_data->gps_signal = 5;
                }
                // if(max_cnr > 99 || max_cnr == 0)
                // {
                //     LOG_D("snr1: %d, snr2: %d, snr3: %d, snr4: %d", gsv->sats[0].snr,gsv->sats[1].snr,gsv->sats[2].snr,gsv->sats[3].snr);
                //     return -1;
                // }
                #endif
            }
        break;
        case NMEA_GGA:
            gga = (struct nmea_gpgga_s*)nmea->data;
            if(gga != NULL)
            {
                gps_data->altitude = gga->altitude;
            }
        break;
        case NMEA_GSA:
            gsa = (struct nmea_gpgsa_s*)nmea->data;
            if(gsa != NULL)
            {
                gps_data->hdop = gsa->hdop;
                gps_data->pdop = gsa->pdop;
            }
        break;
        case NMEA_TXT:
            txt = (struct nmea_gntxt_s*)nmea->data;
            if(txt != NULL)
            {
                if(txt->fwver[0] != 0)
                {
                    strncpy(gps_data->fwver, txt->fwver, sizeof(gps_data->fwver));
                }
            }
		break;
        default:
            break;
    }
    return 0;
}

int gps_data_read(struct niu_gps_data *data)
{
    if(data == NULL)
    {
        return -1;
    }
    memcpy(data, &g_gps_data, sizeof(g_gps_data));
    return 0;
}


int gps_set_config(char *buff, int len)
{
    int ret;
    struct frame_packet * sp = NULL;

    if(buff == NULL || len <= 0)
    {
        LOG_E("buff or len invalid.");
        return -1;
    }

    sp = QL_MEM_ALLOC(sizeof(struct frame_packet));
    if(sp == NULL)
    {
        LOG_E("malloc packet error.");
        ret = -1;
        goto _error;
    }
    memset(sp, 0, sizeof(struct frame_packet));
    sp->buff = QL_MEM_ALLOC(len);
    if(sp->buff == NULL)
    {
        LOG_E("malloc buff error.");
        ret = -1;
        goto _error;
    }
    memcpy(sp->buff, buff, len);
    sp->len = len;
    sp->fd = g_gps_fd;
    if(0 != cmd_handler_push(send_packet_handle, (void*)sp))
    {
        LOG_E("cmd_handler_push error.");
        ret = -1;
        goto _error;
    }
    return 0;

_error:
    if(sp)
    {
        if(sp->buff)
        {
            QL_MEM_FREE(sp->buff);
        }
        QL_MEM_FREE(sp);
    }
    return ret;
}



void * thread_uart_gps_data_recv(void *arg)
{
    //int fd = -1;
    int read_bytes, total_bytes = 0, ret;
    fd_set fdset;
    char buffer[1024], nema_buff[256];
    char *start, *end;
    struct timeval timeout = {3, 0};	// timeout 3s
    struct nmea_s *nmea = NULL;
    //set NMEA mode to 4.1
    char nmea_mode_cfg[] = {0xB5,0x62,0x06,0x17,0x14,0x00,0x00,0x41,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
                0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x75,0x57};//28 
    //Set to GPS+Galileo+BD               
    char gps_mode_cfg[] = {0xB5, 0x62, 0x06, 0x3E, 0x3C, 0x00, 0x00, 0x00, 0x1C, 0x07, 0x00, 0x08, 0x10, 0x00, 0x01,
                   0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00,
                   0x01, 0x00, 0x01, 0x01, 0x03, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01, 0x04, 0x00, 0x08,
                   0x00, 0x00, 0x00, 0x01, 0x01, 0x05 ,0x00, 0x03, 0x00, 0x01 ,0x00, 0x01, 0x01, 0x06, 0x08,
                   0x0E, 0x00, 0x00 ,0x00, 0x01 ,0x01, 0x2C, 0xDD};//10X5+2 = 52

    pthread_detach(pthread_self());
_loop:
    if(g_gps_fd > 0)
    {
        Ql_UART_Close(g_gps_fd);
    }

    g_gps_fd = uart_connect(UART_DEV_GPS_NAME, UART_DEV_GPS_BOUNDRATE, 
                        UART_DEV_GPS_FLOWCTRL, UART_DEV_GPS_PARITY);
  
    if(g_gps_fd < 0)
    {
        LOG_E("uart connect failed.[dev:%s]", UART_DEV_GPS_NAME);
        sleep(1);
        goto _loop;
    }
    gps_set_config(nmea_mode_cfg, sizeof(nmea_mode_cfg));
    gps_set_config(gps_mode_cfg, sizeof(gps_mode_cfg));
    while(1)
    {
        FD_ZERO(&fdset); 
		FD_SET(g_gps_fd, &fdset); 
		ret = select(g_gps_fd + 1, &fdset, NULL, NULL, &timeout);
        if (ret == -1)
		{
			LOG_E("gps failed to select.");
            goto _loop;
		}
        else if (ret == 0)
		{// no data in Rx buffer
            //usleep(10*1000);
            timeout.tv_sec  = 3;
			timeout.tv_usec = 0;
			continue;
		}
        else
		{// data is in Rx data
			if (FD_ISSET(g_gps_fd, &fdset)) 
			{
_rec:
                read_bytes = Ql_UART_Read(g_gps_fd, buffer + total_bytes, 20);
                if(read_bytes == -1)
                {
                    if(errno == EAGAIN)
                    {
                        continue;
                    }
                }
                else if(read_bytes == 0)
                {
                    goto _loop;
                }
                total_bytes += read_bytes;

                if(total_bytes > sizeof(buffer) - 20)
                {
                    total_bytes = 0;
                    goto _rec;
                }

                start = memchr(buffer, '$', total_bytes);
                if (NULL == start) {
                    total_bytes = 0;
                    goto _rec;
                }
                /* find end of line */
                end = memchr(start, NMEA_END_CHAR_1, total_bytes - (start - buffer));
                if (NULL == end || NMEA_END_CHAR_2 != *(++end)) {
                    goto _rec;
                }
                /*upload gps data*/
                memset(nema_buff, 0, 256);
                /* do not include the end of nmea \r\n */
                memcpy(nema_buff, start, jmin(255, end - start - 1));
                upload_gps_info("%s", nema_buff);
                /* nmea string parse*/
                nmea = nmea_parse(start, end - start + 1, 1);
                if(nmea != NULL)
                {
                    ret = nmea_value_update(nmea, &g_gps_data);
                    if(ret)
                    {
                        LOG_D("nmea_value_update error.\n");
                        LOG_D("GSV: %s", nema_buff);
                    }
                    if(nmea->data)
                    {
                        QL_MEM_FREE(nmea->data);
                        nmea->data = NULL;
                    }
                    QL_MEM_FREE(nmea);
                    nmea = NULL;
                }

                if (end == buffer + total_bytes) {
                    total_bytes = 0;
                    goto _rec;
                }

                /* copy rest of buffer to beginning */
                if (buffer != memmove(buffer, end, total_bytes - (end - buffer))) {
                    total_bytes = 0;
                    goto _rec;
                }

                total_bytes -= end - buffer;
            }

        }
    }
    Ql_UART_Close(g_gps_fd);
    return NULL;
}


int gps_init()
{
    int ret;
    pthread_t ptd;

    ret = pthread_create(&ptd, NULL, thread_uart_gps_data_recv, NULL);
    if(ret)
    {
        LOG_E("thread create thread_uart_gps_data_recv error.");
        return -1;
    }
    
    sleep(1);
    if(0 != ql_gps_set_led_state(g_gps_fd, 3100000, 3100000, 0, 3000000)) {
        LOG_D("set gps light state failed");
    }
    
    return 0;
}
