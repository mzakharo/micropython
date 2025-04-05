
#ifndef _WHO_COLOR_DETECTION_H_
#define _WHO_COLOR_DETECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


typedef enum
{
    COLOR_DETECTION_IDLE = 0,
    OPEN_REGISTER_COLOR_BOX,  //录入模式  MENU
    CLOSE_REGISTER_COLOR_BOX,  //识别模式 MENU
    REGISTER_COLOR,       //录入指示框内颜色PLAY 录入模式
    DELETE_COLOR,		//删除最后一个录入的颜色 检测模式
    INCREASE_COLOR_AREA,  //UP
    DECREASE_COLOR_AREA,  //DOWN
    SWITCH_RESULT,   //显示切换
} color_detection_state_t;

void register_color_detection(QueueHandle_t frame_i,
                                     QueueHandle_t event,
                                     QueueHandle_t result,
                                     QueueHandle_t frame_o,
                                     const bool camera_fb_return);
void setColorIndex(uint16_t index);
void color_deinit_task(void);
#ifdef __cplusplus
}
#endif

#endif /* _WHO_COLOR_DETECTION_H_ */
