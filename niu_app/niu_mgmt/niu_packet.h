/********************************************************************
 * @File name: niu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: communication between ec25 and stm32 define and packet generate
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef NIU_PACKET_H_
#define NIU_PACKET_H_
#ifdef __cplusplus 
extern "C" {
#endif
#include "niu_types.h"

//#define SERIAL_PACKET_LENGTH_MIN    (4)
//#define SERIAL_PACKET_LENGTH_MAX    (0xFFFF)
/**
 * packet: $CMC + frame_len + opt + data + crc8 + tail 
 *          4         2        1     XX      1      1
 * frame: opt + data + crc8 + tail
 * so frame len just include (opt + data + crc8 + tail)
 *                             1     XX      1      1
 */
#define SERIAL_PACKET_LENGTH_MIN    (10)
#define SERIAL_PACKET_LENGTH_MAX    (0xFFFF)

#define SERIAL_FRAME_LENGTH_MIN     (4)
#define SERIAL_FRAME_LENGTH_MAX     (2048)


//packet header
#define SERIAL_PACKET_BEGIN_SIZE	(1)
#define SERIAL_PACKET_BEGIN  	    "$"

#define SERIAL_PACKET_HEADER        "$CMC"
#define SERIAL_PACKET_HEADER_SIZE   (4)

#define SERIAL_PACKET_LENGTH_SIZE   (2)

#define SERIAL_PACKET_OPT_SIZE      (1)

#define SERIAL_PACKET_CRC8_SIZE     (1)
#define SERIAL_PACKET_TAIL_SIZE	    (1)
#define SERIAL_PACKET_TAIL	 	    '&'

//OFFSET
#define V35_PACKET_OFFSET_FRAMELEN     (SERIAL_PACKET_HEADER_SIZE)
#define V35_PACKET_OFFSET_OPT          (V35_PACKET_OFFSET_FRAMELEN + SERIAL_PACKET_LENGTH_SIZE)
#define V35_PACKET_OFFSET_DATA          (V35_PACKET_OFFSET_OPT + SERIAL_PACKET_OPT_SIZE)
#define V35_PACKET_OFFSET_CRC8(pkt_len)          ((pkt_len) - SERIAL_PACKET_CRC8_SIZE - SERIAL_PACKET_TAIL_SIZE)


//length
#define CRC8_CALC_LENGTH(pkt_len) ((pkt_len) - SERIAL_PACKET_HEADER_SIZE - \
                                                SERIAL_PACKET_LENGTH_SIZE - SERIAL_PACKET_CRC8_SIZE - SERIAL_PACKET_TAIL_SIZE)
#define PACKET_DATA_LENGTH(pkt_len)     ((pkt_len) - SERIAL_PACKET_HEADER_SIZE - \
                                                SERIAL_PACKET_LENGTH_SIZE - SERIAL_PACKET_OPT_SIZE - SERIAL_PACKET_CRC8_SIZE - SERIAL_PACKET_TAIL_SIZE)
#define COMMAND_SEQREAD_LENGTH_MIN       (4)
#define COMMAND_SEQWRITE_LENGTH_MIN      (4)
#define COMMAND_READ_LENGTH_MIN          (3)
#define COMMAND_WRITE_LENGTH_MIN         (4)

#if 0
#define V35_PACKET_HEADER
#define V35_PACKET_HEADER_SIZE
#define V35_FRAME_LENGTH_SIZE
#define V35
#endif

//command define
#define SERIAL_COMMAND_NONE             (0x00)
#define SERIAL_COMMAND_SRAED 			(0x01)
#define SERIAL_COMMAND_SWRITE 			(0x02)
#define SERIAL_COMMAND_READ				(0x03)
#define SERIAL_COMMAND_WRITE			(0x04)
#define SERIAL_COMMAND_FOTA_START		(0x05)
#define SERIAL_COMMAND_FOTA_PACKAGE	    (0x06)
#define SERIAL_COMMAND_FOTA_VERIFY		(0x07)
#define SERIAL_COMMAND_CONFIG_START		(0x08)
#define SERIAL_COMMAND_CONFIG_PACKAGE   (0x09)
#define SERIAL_COMMAND_CONFIG_VERIFY	(0x0A)
#define SERIAL_COMMAND_CONFIG_IMEI		(0x0B)
#define SERIAL_COMMAND_MAX              (0x0C)



//abnormal define

#define MCU_PACKET_ERRCODE_VERIFY       (0x80)
#define MCU_PAKCET_ERRCODE_ADDRESS      (0x40)
#define MCU_PACKET_ERRCODE_COMMAND      (0x20)
#define MCU_PACKET_ERRCODE_DATA         (0x10)

//packet process error
#define SERIAL_PACKET_VERIFY_ERROR      (0x80)
#define SERIAL_PACKET_ADDRESS_ERROR     (0x40)
#define SERIAL_PACKET_COMMAND_ERROR     (0x20)
#define SERIAL_PACKET_DATA_ERROR        (0x10)
#define SERIAL_PACKET_PPOCESS_OK        (0)
#define SERIAL_PACKET_HEADER_ERROR      (-1)
#define SERIAL_PACKET_TAIL_ERROR        (-2)
#define SERIAL_PACKET_OTHER_ERROR       (-7)




//resp type
#define SERIAL_RESP_TYPE_NORMAL         (0x80)
#define SERIAL_RESP_TYPE_ERROR          (0xC0)



//opt mask
#define SERIAL_RESP_TYPE_MASK           (0xF0)
#define SERIAL_COMMAND_MASK             (0x0F)


struct frame_packet {
    int fd;
    NUINT8 *buff;
    NUINT32 len;
};


NUINT8* generate_mcu_packet(NUINT8 cmd, NUINT8 resp, NUINT8 *data, NUINT16 data_len, NUINT32 *pkt_len);

void send_packet_handle(void *param);


#ifdef __cplusplus 
}
#endif
#endif /* !NIU_PACKET_H_ */
