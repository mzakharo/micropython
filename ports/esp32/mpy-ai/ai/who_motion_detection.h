
#ifndef _WHO_MOTION_DETECTION_H_
#define _WHO_MOTION_DETECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

void register_motion_detection(QueueHandle_t frame_i, 
								TaskHandle_t *Task_handle,
								unsigned int nthreshold,
								QueueHandle_t result, 
								QueueHandle_t frame_o);
#ifdef __cplusplus
}
#endif

#endif /* _WHO_HUMAN_MOTION_DETECTION_H_ */
