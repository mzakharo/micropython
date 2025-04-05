
#ifndef _WHO_HUMAN_FACE_RECOGNITION_H_
#define _WHO_HUMAN_FACE_RECOGNITION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

typedef enum
{
    IDLE = 0,
    DETECT,
    ENROLL,
    RECOGNIZE,
    DELETE, 
} recognizer_state_t;

void register_human_face_recognition(QueueHandle_t frame_i,
                                     QueueHandle_t event,
                                     QueueHandle_t result,
                                     QueueHandle_t frame_o,
                                     QueueHandle_t delteId);

#ifdef __cplusplus
}
#endif

#endif /* _WHO_HUMAN_FACE_DETECTION_H_ */
