/********************************************************************
 * @File name: niu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: niu program entry, process gps and gsensor service logic, 
 * 				like move check, move_ex check, etc...
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef NIU_ENTRY_H_
	#define NIU_ENTRY_H_
#include "niu_types.h"


NUINT32 niu_car_nogps_time();
NUINT32 niu_car_nogsm_time();




JVOID niu_entry();

JVOID entry_niu_gps_data_update();
JVOID niu_acc_callback(JBOOL on_off);
#endif /* !NIU_ENTRY_H_ */
