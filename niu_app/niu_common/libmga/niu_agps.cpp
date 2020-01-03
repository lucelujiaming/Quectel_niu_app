/******************************************************************************
 * Copyright (C) u-blox AG
 * u-blox AG, Thalwil, Switzerland
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY. IN PARTICULAR, NEITHER THE AUTHOR NOR U-BLOX MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ******************************************************************************
 *
 * Project: libMga v18.07
 * Purpose: Example host application using the libMga library to download
 *          MGA assistance data and pass it on to a u-blox GNSS receiver.
 *
 *****************************************************************************/

#include <stdio.h>      // standard input/output definitions
#include <assert.h>     // defines the C preprocessor macro assert()
#include <stdlib.h>     // numeric conversion functions, memory allocation
#include <string.h>     // string handling
#include <ctype.h>
#include <fcntl.h>   // File control definitions
#include <errno.h>   // Error number definitions
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#include "libMga.h"
#include "niu_agps.h"
#include "parserbuffer.h"
#include "protocolubx.h"
#include "protocolnmea.h"
#include "protocolunknown.h"

//#include "arguments.h"  // argument handling

#ifdef _WIN32
#include <winsock.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#endif

#define TIMEOUT                 1


///////////////////////////////////////////////////////////////////////////////
extern void CleanUpThread();
extern void mySleep(int seconds);
extern bool openSerial(const char* port, unsigned int baudrate);
extern int readSerial(void *pBuffer, unsigned int size);
extern int writeSerial(const void *pBuffer, unsigned int size);
extern void closeSerial();
const char* exename = "";
void startDeviceReadThread(void);

///////////////////////////////////////////////////////////////////////////////

static bool s_keepReadingDevice = false;
static bool s_ActiveMgaTransfer = false;
static bool s_readThreadFinished = false;
static time_t s_nextCheck = 0;
// static char* s_serverToken = NULL;

// global variable for the verbosity
//extern unsigned int verbosity;
unsigned int verbosity = 0;

///////////////////////////////////////////////////////////////////////////////
#ifndef _WIN32
static int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        static struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
#endif
extern void* DeviceReadThread(void *);

static int s_fd = 0;
static pthread_t id;

///////////////////////////////////////////////////////////////////////////////
void mySleep(int seconds)
{
    sleep(seconds);
}

void startDeviceReadThread(void)
{
    pthread_create(&id, NULL, DeviceReadThread, NULL);
}

void CleanUpThread()
{
    pthread_join(id, NULL);
}

bool openSerial(const char* port, unsigned int baudrate)
{
    int br;

    // convert the baud rate
    switch(baudrate)
    {
        case 460800:    br=B460800; break;
        case 230400:    br=B230400; break;
        case 115200:    br=B115200; break;
        case 57600:     br=B57600;  break;
        case 38400:     br=B38400;  break;
        case 19200:     br=B19200;  break;
        case 9600:      br=B9600;   break;
        default:        br=B9600;
    }

    // change to you preferred serial port
    s_fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (s_fd != -1)
    {
        fcntl(s_fd, F_SETFL, FNDELAY);

        struct termios options;
        tcgetattr( s_fd, &options );

        cfsetispeed( &options, br );
        cfsetospeed( &options, br);
        cfmakeraw(&options);
        options.c_cflag |= ( CLOCAL | CREAD );  // ignore modem control lines and enable receiver

        // set the character size
        options.c_cflag &= ~CSIZE; // mask the character size bits
        options.c_cflag |= CS8;    // select 8 data bits

        // set no parity (8N1)
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;

        // disable Hardware flow control
        //  options.c_cflag &= ~CNEW_RTSCTS;  -- not supported

        // enable raw input
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

        // disable software flow control
        options.c_iflag &= ~(IXON | IXOFF | IXANY);

        // chose raw (not processed) output
        options.c_oflag &= ~OPOST;
        if ( tcsetattr( s_fd, TCSANOW, &options ) == -1 )
            printf ("Error with tcsetattr = %s\n", strerror ( errno ) );

        fcntl(s_fd, F_SETFL, 0);
    }

    return (s_fd > -1);

}

void closeSerial()
{
    if (s_fd)
        close(s_fd);
}

int readSerial(void *pBuffer, unsigned int size)
{
    return read(s_fd, pBuffer, size);
}

int writeSerial(const void *pBuffer, unsigned int size)
{
    if (s_fd <= 0)
        return -1;

   return write(s_fd, pBuffer, size);
};
static void stopDeviceReadThread(void)
{
    s_keepReadingDevice = false;
    int count = 60;

    while ((!s_readThreadFinished) && (count > 0))
    {
        mySleep(TIMEOUT);
        count--;
    }

    CleanUpThread();
}

static bool waitForFinish(int retries)
{
    int count = (retries > 0) ? retries : 1;
    while ((s_ActiveMgaTransfer) && (count > 0))
    {
        mySleep(TIMEOUT);

        if (retries > 0)
        {
            count--;
        }
    }

    if (!s_ActiveMgaTransfer)
        return true;
    else
        return false;
}


#ifdef _WIN32
void DeviceReadThread(void *)
#else
void *DeviceReadThread(void *)
#endif
{
    s_keepReadingDevice = true;
    s_readThreadFinished = false;

    CProtocolUBX  protocolUBX;
    CProtocolNMEA protocolNmea;
    CProtocolUnknown protocolUnknown;
    CParserBuffer parser;               // declare after protocols, so destructor called before protocol destructor

    // setup parser buffer
    parser.Register(&protocolUBX);
    parser.Register(&protocolNmea);
    parser.RegisterUnknown(&protocolUnknown);

    while (s_keepReadingDevice)
    {
        unsigned char *ptr = parser.GetPointer();
        unsigned int space = (unsigned int)parser.GetSpace();
        int size = readSerial(ptr, space);

        if (size)
        {
            // printf("%i bytes read\n", size);
            int msgSize;
            unsigned char* pMsg;
            CProtocol* pProtocol;

            // we read it so update the size
            parser.Append(size);
            // parse ... only interested breaking the message stream into individual UBX message chunks
            while (parser.Parse(pProtocol, pMsg, msgSize))
            {

                if (pProtocol == &protocolUnknown)
                {
                    printf("%s: MSG UNKNOWN size %d\n", __FUNCTION__, msgSize);
                }


                mgaProcessReceiverMessage(pMsg, msgSize);
                parser.Remove(msgSize);
            }

            parser.Compact();
        }
        else
        {
            // printf("%s: read error %d\n", __FUNCTION__, size);
        }

        time_t now = time(NULL);
        if (now > s_nextCheck)
        {
            mgaCheckForTimeOuts();
            s_nextCheck = now + 1;
        }
    }

    s_readThreadFinished = true;
#ifdef _WIN32
#else
    return NULL;
#endif
}

///////////////////////////////////////////////////////////////////////////////

static void EvtProgressHandler(MGA_PROGRESS_EVENT_TYPE evtType, const void* pContext, const void* pEvtInfo, UBX_I4 evtInfoSize)
{
    // unused variables
    (void)pContext;
    (void)evtInfoSize;

    static int s_max = 0;
    static UBX_U2 s_activeFileId = 0;

    switch (evtType)
    {
    case MGA_PROGRESS_EVT_MSG_SENT:
    {
        MgaMsgInfo* pMsgInfo = (MgaMsgInfo*)pEvtInfo;
        if (verbosity == 1)
        {
            printf("Message sent [%i/%i] size [%i] reties[%i]\n",
                   pMsgInfo->sequenceNumber + 1,
                   s_max,
                   pMsgInfo->msgSize,
                   pMsgInfo->retryCount);
        }
    }
    break;

    case MGA_PROGRESS_EVT_TERMINATED:
    {
        EVT_TERMINATION_REASON* stopReason = (EVT_TERMINATION_REASON*)pEvtInfo;
        printf("Terminated - Reason %i\n", *stopReason);
        s_ActiveMgaTransfer = false;
    }
    break;

    case MGA_PROGRESS_EVT_FINISH:
    {
        // pEvtInfo is NULL
        printf("Finish\n");
        s_ActiveMgaTransfer = false;
    }
    break;

    case MGA_PROGRESS_EVT_START:
    {
        s_max = *((UBX_U4*)pEvtInfo);
        printf("Start. Expecting %i messages\n", s_max);
    }
    break;

    case MGA_PROGRESS_EVT_MSG_TRANSFER_FAILED:
    {
        MgaMsgInfo* pMsgInfo = (MgaMsgInfo*)pEvtInfo;
        printf("Transfer failed - Message [%i]  Reason [%i]\n",
               pMsgInfo->sequenceNumber + 1,
               pMsgInfo->mgaFailedReason);
    }
    break;

    case MGA_PROGRESS_EVT_MSG_TRANSFER_COMPLETE:
    {
        MgaMsgInfo* pMsgInfo = (MgaMsgInfo*)pEvtInfo;
        printf("Transfer complete [%i]\n", pMsgInfo->sequenceNumber + 1);
    }
    break;

    case MGA_PROGRESS_EVT_SERVER_CONNECT:
    {
        printf("Connected to server '%s'\n", (char *)pEvtInfo);
    }
    break;

    case MGA_PROGRESS_EVT_REQUEST_HEADER:
    {
        printf("Requesting header\n");
    }
    break;

    case MGA_PROGRESS_EVT_RETRIEVE_DATA:
    {
        // pEvtInfo is NULL
        printf("Retrieve data\n");
    }
    break;

    case MGA_PROGRESS_EVT_SERVICE_ERROR:
    {
        EvtInfoServiceError* pError = (EvtInfoServiceError*)pEvtInfo;
        printf("Service error - code [%i] '%s'\n", pError->errorType, pError->errorMessage);
    }
    break;

    case MGA_PROGRESS_EVT_SERVER_CONNECTING:
    {
        const char* pServerName = (const char*)pEvtInfo;
        printf("Connecting to server '%s'\n", pServerName);
    }
    break;

    case MGA_PROGRESS_EVT_UNKNOWN_SERVER:
    {
        const char* pServerName = (const char*)pEvtInfo;
        printf("Unknown server '%s'\n", pServerName);
    }
    break;

    case MGA_PROGRESS_EVT_SERVER_CANNOT_CONNECT:
    {
        const char* pServerName = (const char*)pEvtInfo;
        printf("Can not connect to server '%s'\n", pServerName);
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_STARTUP:
        s_max = *((UBX_U4*)pEvtInfo);
        printf("Initializing legacy aiding flash data transfer\n");
        break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_STARTUP_FAILED:
    {
        UBX_I4 reason = *(UBX_I4*)pEvtInfo;
        printf("Initialization of legacy aiding flash data transfer. Reason [%i]\n", reason);

        switch (reason)
        {
        case MGA_FAILED_REASON_LEGACY_NO_ACK:
            printf("\tNo Ack\n");
            break;

        default:
            assert(false);
            break;
        }
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_STARTUP_COMPLETED:
        printf("Completed the initialization of legacy aiding flash data transfer\n");
        break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_FLASH_BLOCK_SENT:
    {
        MgaMsgInfo* pInfo = (MgaMsgInfo*)pEvtInfo;

        printf("Legacy aiding flash data block sent [%i/%i] size [%i]\n",
               pInfo->sequenceNumber + 1,
               s_max,
               pInfo->msgSize);
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_FLASH_BLOCK_FAILED:
    {
        MgaMsgInfo* pInfo = (MgaMsgInfo*)pEvtInfo;

        printf("Legacy aiding flash data block [%i], transfer failed. Reason [%i] - ",
               pInfo->sequenceNumber + 1,
               pInfo->mgaFailedReason);
        switch (pInfo->mgaFailedReason)
        {
        case MGA_FAILED_REASON_LEGACY_NO_ACK:
            printf("No Ack\n");
            break;

        default:
            assert(false);
            break;
        }

    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_FLASH_BLOCK_COMPLETE:
    {
        MgaMsgInfo* pInfo = (MgaMsgInfo*)pEvtInfo;
        printf("Legacy aiding flash data block received [%i]\n", pInfo->sequenceNumber + 1);
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_FINALIZE_START:
        printf("Finalizing legacy aiding flash data transfer\n");
        break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_FINALIZE_FAILED:
        printf("Finalization of legacy aiding flash data transfer failed (No Ack)\n");
        break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_FINALIZE_COMPLETED:
        printf("Finalizing legacy aiding flash data transfer done\n");
        break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_SERVER_STARTED:
    {
        // pEvtInfo is a pointer to the payload of the UBX-AID-ALPSRV message just sent to inform
        // the receiver the host based aiding server is active.
        // This payload is a LegacyAidingRequestHeader structure followed by a LegacyAidingDataHeader
        // structure.
        // evtInfoSize is the size of these two structures.
        assert(evtInfoSize == (sizeof(LegacyAidingRequestHeader) + sizeof(LegacyAidingDataHeader)));

        LegacyAidingRequestHeader *pAidingRequestHeader = (LegacyAidingRequestHeader *)pEvtInfo;
        LegacyAidingDataHeader *pAidingData = (LegacyAidingDataHeader *)&pAidingRequestHeader[1];

        printf("Legacy aiding server started: Magic: 0x%x, Duration: %i %s\n",
               pAidingData->magic,
               (pAidingData->duration * 600) / 86400,
               ((pAidingData->duration * 600) / 86400) == 1 ? "day" : "days");
        s_activeFileId = pAidingRequestHeader->fileId;
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_SERVER_STOPPED:
    {
        // pEvtInfo is NULL
        // evtInfoSize is 0
        assert(pEvtInfo == NULL);
        assert(evtInfoSize == 0);
        printf("Legacy aiding server stopped\n");
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_SERVER_REQUEST_RECEIVED:
    {
        // pEvtInfo is a pointer to the to the payload of the UBX-AID-ALPSRV message just received.
        // This payload is a LegacyAidingRequestHeader structure.
        // evtInfoSize is the size of this LegacyAidingRequestHeader structure.
        LegacyAidingRequestHeader *pRequestEvtInfo = (LegacyAidingRequestHeader *)pEvtInfo;
        assert(evtInfoSize == sizeof(LegacyAidingRequestHeader));

        printf("Req rcvd: Header size: %i, Type: %x, Offs: %i, Size: %i\n",
               pRequestEvtInfo->idSize,
               pRequestEvtInfo->type,
               pRequestEvtInfo->ofs * 2,
               pRequestEvtInfo->size * 2
               );
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_SERVER_REQUEST_COMPLETED:
    {
        // pEvtInfo is a pointer to the to the payload of the UBX-AID-ALPSRV reply message just sent.
        // This payload is a LegacyAidingRequestHeader structure followed the requested aiding data.
        // evtInfoSize is the size of this LegacyAidingRequestHeader structure plus the requested data.
        // The requested data length can be found in the LegacyAidingRequestHeader 'dataSize' field.
        // If pEvtInfo is NULL, evtInfoSize will be 0. This means no reply message was sent.
        LegacyAidingRequestHeader *pRequestEvtInfo = (LegacyAidingRequestHeader *)pEvtInfo;
        if (pRequestEvtInfo)
        {
            assert(evtInfoSize == (UBX_I4)(sizeof(LegacyAidingRequestHeader) + pRequestEvtInfo->dataSize));

            printf("Repl sent: Header size: %i, Type: %x, Offs: %i, Size: %i\n",
                   pRequestEvtInfo->idSize,
                   pRequestEvtInfo->type,
                   pRequestEvtInfo->ofs * 2,
                   pRequestEvtInfo->size * 2
                   );
        }
        else
        {
            assert(evtInfoSize == 0);
            printf("Reply not sent: Request outside data bounds.\n");
        }
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_SERVER_UPDATE_RECEIVED:
    {
        // pEvtInfo is a pointer to the to the payload of the UBX-AID-ALPSRV update message just sent.
        // This payload is a LegacyAidingUpdateDataHeader structure followed by the new aiding data.
        // evtInfoSize is the size of this LegacyAidingUpdateDataHeader structure plus the new aiding data.
        // The new aiding data length can be calculated by taking the LegacyAidingUpdateDataHeader 'size' field.
        // and multiplying by 2.
        LegacyAidingUpdateDataHeader *pAidingUpdateHeader = (LegacyAidingUpdateDataHeader *)pEvtInfo;
        assert(pAidingUpdateHeader != NULL);
        assert(sizeof(LegacyAidingUpdateDataHeader) == pAidingUpdateHeader->idSize);

        printf("Update received: Offs: %i, Data size: %i\n",
               pAidingUpdateHeader->ofs * 2,
               pAidingUpdateHeader->size * 2);
        printf(" - Don't access aiding data directly\n");
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_SERVER_UPDATE_COMPLETED:
    {
        // pEvtInfo is NULL
        // evtInfoSize is 0
        assert(pEvtInfo == NULL);
        assert(evtInfoSize == 0);

        printf("Update completed: Can now access aiding data directly\n");
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_REQUEST_FAILED_NO_MEMORY:
    {
        // pEvtInfo is a pointer to the to the payload of the UBX-AID-ALPSRV update message just sent.
        // This payload is a LegacyAidingRequestHeader structure.
        // evtInfoSize is the size of this LegacyAidingRequestHeader structure.
        LegacyAidingRequestHeader *pAidingRequestHeader = (LegacyAidingRequestHeader *)pEvtInfo;
        assert(evtInfoSize == sizeof(LegacyAidingRequestHeader));

        printf("Request response not sent: Not enough memory. Offs: %i, Data size: %i\n",
               pAidingRequestHeader->ofs * 2,
               pAidingRequestHeader->dataSize * 2);
    }
    break;

    case MGA_PROGRESS_EVT_LEGACY_AIDING_REQUEST_FAILED_ID_MISMATCH:
    {
        // pEvtInfo is a pointer to the to the payload of the UBX-AID-ALPSRV update message just sent.
        // This payload is a LegacyAidingUpdateDataHeader structure.
        // evtInfoSize is the size of this LegacyAidingUpdateDataHeader structure.
        LegacyAidingUpdateDataHeader *pLegacyAidingheader = (LegacyAidingUpdateDataHeader *)pEvtInfo;
        assert(s_activeFileId != pLegacyAidingheader->fileId);

        printf("Request resoonse not sent: File Ids do not match. Expected: %i, Received: %i\n",
               s_activeFileId,
               pLegacyAidingheader->fileId);
    }
    break;

    default:
        assert(0);
        break;
    }
}


static void EvtWriteDeviceHandler(const void* pContext, const UBX_U1* pData, UBX_I4 iSize)
{
    //printf("Writing %d bytes of data\n", iSize);
    (void)pContext;

    writeSerial(pData, iSize);
}


///////////////////////////////////////////////////////////////////////////////
const CL_ARGUMENTS_t defaultArgs =
{
    "/dev/ttyHSL0",                     //!< COM port on Linux
    9600,                               //!< Baud rate
    "u-blox-iot.niu.com",               //!< MGA server 1 name
    "",                                 //!< MGA server 2 name
    "vREokKbyY0GHcqZJ9od7nA",           //!< MGA token
    "gps,glo,bds,gal",                  //!< GNSS systems, comma separated
    "eph,alm,aux",                      //!< Data type
    "mga",                              //!< Data format
    "gps",                              //!< GNSS systems from which to get almanac data, comma separated
    0,                                  //!< Use reference position (0 or 1)
    0,                                  //!< Position accuracy in m
    0,                                  //!< Altitude
    0,                                  //!< Latitude
    0,                                  //!< Longitude
    0,                                  //!< Use latency (0 or 1)
    0,                                  //!< Latency in s
    0,                                  //!< Use time accuracy
    0,                                  //!< Time accuracy in s
    4,                                  //!< Period
    1,                                  //!< Resolution
    28,                                 //!< Days (28 for mga, 14 for aid)
    "online",                           //!< MGA test to perform
    2000,                               //!< Timeout
    2,                                  //!< Retry count
    "smart",                            //!< Flow control
    24 * 3600,                            //!< Offline host server duration in s
    0,                                  //!< Verbosity level
    0,                                  //!< Server certificate verification
};
// MGA online test (MGA and legacy)
//static void onlineExample(CL_ARGUMENTS_pt clArgs)
void niu_agps_mga_online(double longitude, double latitude)
{
    CL_ARGUMENTS_t clArgs;


    memcpy(&clArgs, (void*)&defaultArgs, sizeof(CL_ARGUMENTS_t));
    clArgs.posLatitude = latitude;
    clArgs.posLongitude = longitude;

    if (!openSerial(clArgs.port, clArgs.baudrate))
    {
        printf("Serial port '%s' could not be opened\n", clArgs.port);
        exit(EXIT_FAILURE);
    }

    startDeviceReadThread();

    // setup callback for progress and writing to receiver
    MgaEventInterface evtInterface;
    evtInterface.evtProgress = EvtProgressHandler;
    evtInterface.evtWriteDevice = EvtWriteDeviceHandler;

    // setup flow control to the receiver like this
    MgaFlowConfiguration flowConfig;
    memset(&flowConfig, 0, sizeof(flowConfig));
    flowConfig.msgTimeOut = clArgs.timeout;
    flowConfig.msgRetryCount = clArgs.retry;

    // determine the flow control from the command line arguments
    flowConfig.mgaFlowControl = MGA_FLOW_NONE;
    if (strstr(clArgs.flow, "simple"))
        flowConfig.mgaFlowControl = MGA_FLOW_SIMPLE;
    if (strstr(clArgs.flow, "smart"))
        flowConfig.mgaFlowControl = MGA_FLOW_SMART;

    MGA_API_RESULT mgaResult = mgaConfigure(&flowConfig, &evtInterface, NULL);
    assert(mgaResult == MGA_API_OK);

    // setup what Online servers to communicate with, and what parameters to send
    MgaOnlineServerConfig mgaServerConfig;
    memset(&mgaServerConfig, 0, sizeof(mgaServerConfig));

    mgaServerConfig.strPrimaryServer = clArgs.server1;
    mgaServerConfig.strSecondaryServer = clArgs.server2;
    mgaServerConfig.strServerToken = clArgs.token;

    // specify what GNSS systems we need data for, like this
    strstr(clArgs.gnss, "gps") ? mgaServerConfig.gnssTypeFlags |= MGA_GNSS_GPS : 0;
    strstr(clArgs.gnss, "glo") ? mgaServerConfig.gnssTypeFlags |= MGA_GNSS_GLO : 0;
    strstr(clArgs.gnss, "gal") ? mgaServerConfig.gnssTypeFlags |= MGA_GNSS_GALILEO : 0;
    strstr(clArgs.gnss, "bds") ? mgaServerConfig.gnssTypeFlags |= MGA_GNSS_BEIDOU : 0;
    strstr(clArgs.gnss, "qzss") ? mgaServerConfig.gnssTypeFlags |= MGA_GNSS_QZSS : 0;

    // specify what type of online data we need, like this
    strstr(clArgs.dataType, "eph") ? mgaServerConfig.dataTypeFlags |= MGA_DATA_EPH : 0;
    strstr(clArgs.dataType, "alm") ? mgaServerConfig.dataTypeFlags |= MGA_DATA_ALM : 0;
    strstr(clArgs.dataType, "aux") ? mgaServerConfig.dataTypeFlags |= MGA_DATA_AUX : 0;
    strstr(clArgs.dataType, "pos") ? mgaServerConfig.dataTypeFlags |= MGA_DATA_POS : 0;

    // specify position like this
    if (clArgs.usePosition)
    {
        mgaServerConfig.useFlags |= MGA_FLAGS_USE_POSITION;
        mgaServerConfig.dblAccuracy = clArgs.posAccuracy;
        mgaServerConfig.dblAltitude = clArgs.posAltitude;
        mgaServerConfig.dblLatitude = clArgs.posLatitude;
        mgaServerConfig.dblLongitude = clArgs.posLongitude;
    }

    // specify latency like this
    if (clArgs.useLatency)
    {
        mgaServerConfig.useFlags |= MGA_FLAGS_USE_LATENCY;
        mgaServerConfig.latency = clArgs.latency;
    }

    // specify time accuracy like this
    if (clArgs.useTimeAccuracy)
    {
        mgaServerConfig.useFlags |= MGA_FLAGS_USE_TIMEACC;
        mgaServerConfig.timeAccuracy = clArgs.timeAccuracy;
    }

    // check for legacy aiding
    if (strstr(clArgs.format, "aid"))
    {
        mgaServerConfig.useFlags |= MGA_FLAGS_USE_LEGACY_AIDING;

        // reset the flow control
        flowConfig.mgaFlowControl = MGA_FLOW_NONE;
        if (strstr(clArgs.flow, "simple"))
            flowConfig.mgaFlowControl = MGA_FLOW_SIMPLE;
    }

    //check if server certificate should be verified
    if (clArgs.serverVerify)
    {
        mgaServerConfig.bValidateServerCert = true;
    }

    // set the filter on position like this
    mgaServerConfig.bFilterOnPos = true;

    printf("Connecting to u-blox MGA Online Service\n");

    UBX_U1* pMgaData = NULL;
    UBX_I4 iMgaDataSize = 0;
    mgaResult = mgaGetOnlineData(&mgaServerConfig, &pMgaData, &iMgaDataSize);

    printf("Got %d bytes from MGA\n", iMgaDataSize);

    if (mgaResult == MGA_API_OK)
    {
        printf("Retrieved Online data successfully\n");
        mgaResult = mgaSessionStart();
        assert(mgaResult == MGA_API_OK);


        printf("Sending Online data to receiver\n");
        
        s_ActiveMgaTransfer = true;

        // setup time adjust like this
        time_t now = time(NULL);
        tm* pNowAsTm = gmtime(&now);

        MgaTimeAdjust time;
        time.mgaAdjustType = MGA_TIME_ADJUST_ABSOLUTE;
        time.mgaYear = (UBX_U2)pNowAsTm->tm_year + 1900;
        time.mgaMonth = (UBX_U1)pNowAsTm->tm_mon + 1;
        time.mgaDay = (UBX_U1)pNowAsTm->tm_mday;
        time.mgaHour = (UBX_U1)pNowAsTm->tm_hour;
        time.mgaMinute = (UBX_U1)pNowAsTm->tm_min;
        time.mgaSecond = (UBX_U1)pNowAsTm->tm_sec;
        time.mgaAccuracyS = (UBX_U2)0;
        time.mgaAccuracyMs = (UBX_U2)0;

        mgaResult = mgaSessionSendOnlineData(pMgaData, iMgaDataSize, &time);
        if (mgaResult == MGA_API_OK)
        {
            // now wait until transfer has finished
            waitForFinish(300);
            printf("Online transfer to receiver is complete\n");

        }
        else
        {
            // failed to send, so stop session
            mgaSessionStop();
            switch (mgaResult)
            {
            case MGA_API_NO_DATA_TO_SEND:
                printf("Data form AssistNow Online service does not contain any MGA online data.");
                break;

            case MGA_API_BAD_DATA:
                printf("Bad data received from AssistNow Online service.");
                break;

            case MGA_API_NO_MGA_INI_TIME:
                printf("No MGA-INI-TIME message in data from AssistNow Online service.");
                break;

            case MGA_API_OUT_OF_MEMORY:
                printf("Out of memory.");
                break;

            default:
                assert(0);
                break;
            }
        }

        free(pMgaData);
    }
    else
    {
        // Failed to get online data from service
        switch (mgaResult)
        {
        case MGA_API_CANNOT_CONNECT:
            printf("Failed to connect to AssistNow Online server.\n");
            break;

        case MGA_API_CANNOT_GET_DATA:
            printf("Failed to get online data from service.\n");
            break;

        default:
            assert(0);
            break;
        }
    }
    stopDeviceReadThread();
    //if (mgaResult != MGA_API_OK)
    //    exit(EXIT_FAILURE);
    mgaDeinit();
}




