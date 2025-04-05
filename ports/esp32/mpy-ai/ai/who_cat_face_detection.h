
#ifndef _WHO_CAT_FACE_DETECTION_H_
#define _WHO_CAT_FACE_DETECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

void register_cat_face_detection(QueueHandle_t frame_i,
                                   TaskHandle_t *Task_handle,
                                   QueueHandle_t result,
                                   QueueHandle_t frame_o,
                                   const bool camera_fb_return);

#ifdef __cplusplus
}
#endif

#endif /* _WHO_CAT_FACE_DETECTION_H_ */
