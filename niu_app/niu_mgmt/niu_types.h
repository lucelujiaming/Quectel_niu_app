/********************************************************************
 * @File name: niu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: niu type define
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef NIU_TYPES_H_
#define NIU_TYPES_H_

#ifdef __cplusplus 
extern "C" {
#endif

typedef char                         NCHAR;

typedef unsigned short          NWCHAR;

typedef unsigned char           NUINT8;

typedef signed char              NINT8;

typedef unsigned short int     NUINT16;

typedef signed short int        NINT16;

typedef unsigned int             NUINT32;

typedef signed int                NINT32;

typedef unsigned long long   NUINT64;

typedef signed long long       NINT64;

typedef float                 NFLOAT;
typedef double                NDOUBLE;

#ifndef JFALSE
#define  JFALSE  0
#endif

#ifndef JTRUE
#define JTRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef JVOID
#define JVOID void
#endif

#ifndef JBOOL
#define JBOOL NUINT8
#endif

#ifndef JSTATIC
#define JSTATIC static
#endif 

#ifndef JCONST
#define JCONST const
#endif

typedef struct _tagJstring
{
   NCHAR* str;
   NUINT32 len;
}JSTRING;

typedef struct _tagJbuffer
{
  NUINT8* buf;
  NUINT32 len;
}JBUFFER;


#define J_BIT0     (1)
#define J_BIT1     (1<<1)
#define J_BIT2     (1<<2)
#define J_BIT3     (1<<3)
#define J_BIT4     (1<<4)
#define J_BIT5     (1<<5)
#define J_BIT6     (1<<6)
#define J_BIT7     (1<<7)

#ifdef __cplusplus 
}
#endif
#endif /* !NIU_TYPES_H_ */
