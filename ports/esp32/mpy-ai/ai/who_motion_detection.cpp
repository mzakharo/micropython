#include "who_motion_detection.h"

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"
#include "py/obj.h"
#include "py/runtime.h"

#if MICROPY_HW_ESPAI
#include "modespai.h"
#endif

static const char *TAG = "motion_detection";

static QueueHandle_t xQueueFrameI = NULL;
static QueueHandle_t xQueueFrameO = NULL;

static bool gEvent = true;
static unsigned int threshold = 50;

static void task_process_handler(void *arg)
{
    camera_fb_t *frame1 = NULL;
    camera_fb_t *frame2 = NULL;

    while (true)
    {
        if (gEvent)
        {
            if (xQueueReceive(xQueueFrameI, &(frame1), portMAX_DELAY))
            {
                if (xQueueReceive(xQueueFrameI, &(frame2), portMAX_DELAY))
                {
                    uint32_t moving_point_number = dl::image::get_moving_point_number((uint16_t *)frame1->buf, (uint16_t *)frame2->buf, frame1->height, frame1->width, 8, 15);
					#if MICROPY_HW_ESPAI
					pFdetection.results = (uint16_t)moving_point_number;
					#endif
					if (moving_point_number > threshold)
                    {
                        dl::image::draw_filled_rectangle((uint16_t *)frame2->buf, frame2->height, frame2->width, 0, 0, 20, 20);
                    }
                }
            }

            if (xQueueFrameO)
            {
                esp_camera_fb_return(frame1);
                xQueueSend(xQueueFrameO, &frame2, portMAX_DELAY);
            }
            else
            {
                esp_camera_fb_return(frame1);
                esp_camera_fb_return(frame2);
            }
        }
    }
}

void register_motion_detection(QueueHandle_t frame_i,
								TaskHandle_t *Task_handle,
								unsigned int nthreshold,
								QueueHandle_t result,
								QueueHandle_t frame_o)
{
    xQueueFrameI = frame_i;
    xQueueFrameO = frame_o;
	threshold = nthreshold;
	xTaskCreate( task_process_handler, TAG, (4 * 1024) / sizeof(StackType_t), NULL, 5, Task_handle );	
}


