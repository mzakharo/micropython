#include "who_human_face_detection.h"

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"

#include "who_ai_utils.hpp"
#include <stdio.h>
#include <iostream>

#include "py/obj.h"
#include "py/runtime.h"

#define TWO_STAGE_ON 1

#if MICROPY_HW_ESPAI
#include "modespai.h"
#endif

static const char *TAG = "human_face_detection";

static QueueHandle_t xQueueFrameI = NULL;
static QueueHandle_t xQueueFrameO = NULL;

static bool gEvent = true;
static bool gReturnFB = true;

#if MICROPY_HW_ESPAI
static void get_detection_result(std::list<dl::detect::result_t> &results)
{
    int i = 0;
	pFdetection.results = 0;

    for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
    {
		
		if(i <= 1){
			pFdetection.results++;
			pFdetection.piont[i].x0 = (prediction->box[0]<0) ? 0:prediction->box[0];
			pFdetection.piont[i].y0 = (prediction->box[1]<0) ? 0:prediction->box[1];
			pFdetection.piont[i].x1 = (prediction->box[2]<0) ? 0:prediction->box[2];
			pFdetection.piont[i].y1 = (prediction->box[3]<0) ? 0:prediction->box[3];

			if (prediction->keypoint.size() == 10)
			{
				pFdetection.piont[i].leye_x = (prediction->keypoint[0]<0) ? 0: prediction->keypoint[0];
				pFdetection.piont[i].leye_y = (prediction->keypoint[1]<0) ? 0: prediction->keypoint[1];  // left eye
				pFdetection.piont[i].reye_x = (prediction->keypoint[6]<0) ? 0: prediction->keypoint[6];
				pFdetection.piont[i].reye_y = (prediction->keypoint[7]<0) ? 0: prediction->keypoint[7];  // right eye
				pFdetection.piont[i].nose_x = (prediction->keypoint[4]<0) ? 0: prediction->keypoint[4];
				pFdetection.piont[i].nose_y = (prediction->keypoint[5]<0) ? 0: prediction->keypoint[5];  // nose
				pFdetection.piont[i].lcorner_x = (prediction->keypoint[2]<0) ? 0: prediction->keypoint[2];
				pFdetection.piont[i].lcorner_y = (prediction->keypoint[3]<0) ? 0: prediction->keypoint[3]; // mouth left corner
				pFdetection.piont[i].rcorner_x = (prediction->keypoint[8]<0) ? 0: prediction->keypoint[8];
				pFdetection.piont[i].rcorner_y = (prediction->keypoint[9]<0) ? 0: prediction->keypoint[9]; // mouth right corner
			}
		}
    }
}
#endif
static void task_process_handler(void *arg)
{
    camera_fb_t *frame = NULL;
    HumanFaceDetectMSR01 detector(0.3F, 0.3F, 10, 0.3F);
#if TWO_STAGE_ON
    HumanFaceDetectMNP01 detector2(0.4F, 0.3F, 10);
#endif

    while (true)
    {
        if (gEvent)
        {

            if (xQueueReceive(xQueueFrameI, &frame, portMAX_DELAY))
            {
#if TWO_STAGE_ON
                std::list<dl::detect::result_t> &detect_candidates = detector.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3});
                std::list<dl::detect::result_t> &detect_results = detector2.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3}, detect_candidates);
#else
                std::list<dl::detect::result_t> &detect_results = detector.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3});
#endif
                if (detect_results.size() > 0){
                    draw_detection_result((uint16_t *)frame->buf, frame->height, frame->width, detect_results);
                    // print_detection_result(detect_results);

					#if MICROPY_HW_ESPAI
					get_detection_result(detect_results);
					#endif
                }else{
					#if MICROPY_HW_ESPAI
					pFdetection.results = 0;
					#endif
				}
			}
            if (xQueueFrameO)
            {
                xQueueSend(xQueueFrameO, &frame, portMAX_DELAY);
            }
            else if (gReturnFB)
            {
                esp_camera_fb_return(frame);
            }

        }
    }
}

void register_human_face_detection(const QueueHandle_t frame_i,
                                   TaskHandle_t *Task_handle,
                                   const QueueHandle_t result,
                                   const QueueHandle_t frame_o,
                                   const bool camera_fb_return)
{
    xQueueFrameI = frame_i;
    xQueueFrameO = frame_o;
    gReturnFB = camera_fb_return;

	xTaskCreate( task_process_handler, TAG, (4 * 1024) / sizeof(StackType_t), NULL, 5, Task_handle );		

}

