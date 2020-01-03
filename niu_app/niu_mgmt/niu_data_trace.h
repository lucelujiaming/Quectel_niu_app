/********************************************************************
 * @File name: niu_data_trace.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: niu data trace function, used to debug niu data.
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef NIU_DATA_TRACE_H_
#define NIU_DATA_TRACE_H_
#include "niu_types.h"
#include "niu_data_pri.h"



JVOID niu_item_trace(NIU_DEVICE_TYPE base, NUINT32 id);

JVOID niu_device_trace(NIU_DEVICE_TYPE base);

JVOID niu_all_trace();
#endif /* !NIU_DATA_TRACE_H_ */
