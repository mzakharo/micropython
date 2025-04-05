#ifndef __ANOMAL_DETEC_H
#define __ANOMAL_DETEC_H
#include "stabilizer_types.h"

#define DETEC_ENABLED

#define DETEC_FF_THRESHOLD 	0.05f	/* accZ接近-1.0程度 表示Free Fall */
#define DETEC_FF_COUNT 		50  	/* 自由落体检测计数 1000Hz测试条件 */

#define DETEC_TU_THRESHOLD 	60		/* 碰撞检测阈值60°*/
#define DETEC_TU_COUNT 		100  	/* 碰撞检测计数 1000Hz测试条件 */

/*异常检测*/
void anomalDetec(const sensorData_t *sensorData, const state_t *state, const control_t *control);

#endif	/*__ANOMAL_DETEC_H*/

