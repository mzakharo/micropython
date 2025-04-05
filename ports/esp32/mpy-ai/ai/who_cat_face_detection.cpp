
#include "who_cat_face_detection.h"

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"
#include "cat_face_detect_mn03.hpp"

#include "who_ai_utils.hpp"

#include "py/obj.h"
#include "py/runtime.h"

#if MICROPY_HW_ESPAI
#include "modespai.h"
#endif

static const char *TAG = "cat_face_detection";

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
		}
    }
}
#endif

static void task_process_handler(void *arg)
{
    camera_fb_t *frame = NULL;
    CatFaceDetectMN03 detector(0.4F, 0.3F, 10, 0.3F);

    while (true)
    {
        if (gEvent)
        {

            if (xQueueReceive(xQueueFrameI, &frame, portMAX_DELAY))
            {
                std::list<dl::detect::result_t> &detect_results = detector.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3});
                if (detect_results.size() > 0)
                {
                    draw_detection_result((uint16_t *)frame->buf, frame->height, frame->width, detect_results);
                    // print_detection_result(detect_results);
					get_detection_result(detect_results);

                }else{
					get_detection_result(detect_results);
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

void register_cat_face_detection(const QueueHandle_t frame_i,
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
