/********************************************************************
 * @File name: niu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: work mode update
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef NIU_WORK_MODE_H_
	#define NIU_WORK_MODE_H_

#include "niu_types.h"

typedef enum  _J_WORK_MODE_
{
   J_WORK_MODE_DRIVING = 0,
   J_WORK_MODE_ACTIVE,
   J_WORK_MODE_SLEEP,
}J_WORK_MODE;


typedef JVOID (*J_WORK_MODE_NOTIFY)(J_WORK_MODE mode);


typedef struct _J_WORK_MODE_INIT_PARA_
{
    J_WORK_MODE_NOTIFY  notify_func;
    NUINT32 time_to_sleep;
    JBOOL use_gps;
}J_WORK_MODE_INIT_PARA;

JBOOL niu_work_mode_init(J_WORK_MODE_INIT_PARA* para);
JVOID niu_work_mode_do_active();
J_WORK_MODE niu_work_mode_value();
#endif /* !NIU_WORK_MODE_H_ */
