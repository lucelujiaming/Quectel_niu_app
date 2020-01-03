/********************************************************************
 * @File name: niu_data.h
 * @AUthor: Baron.qian
 * @Version: 1.0
 * @Date: 2018-10-21
 * @Description: version define
 * @Copyright: NIU technology
 * @ niu.com
 ********************************************************************/

#ifndef NIU_VERSION_H_
	#define NIU_VERSION_H_
#define NIU_APP		"niu_mgmtd"

#define NIU_DATA_DEFAULT_SW_VER "C4Q01V08"
#define NIU_DATA_DEFAULT_HW_VER "V35LTE01"
#define NIU_VERSION NIU_DATA_DEFAULT_SW_VER
/*FOTA VERSION is defined at niu_data.c line 1808*/

/*add NIU_BUILD_TIMESTAMP in Makefile, update when compile.*/
//#define NIU_BUILD_TIMESTAMP "2019-01-31 14:43:09"
#endif /* !NIU_VERSION_H_ */
