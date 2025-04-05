#include "who_human_face_recognition.h"

#include "esp_log.h"
#include "esp_camera.h"
#include <stdio.h>
#include "dl_image.hpp"
#include "fb_gfx.h"

#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#include "face_recognition_tool.hpp"

#include "py/obj.h"

#define CONFIG_MFN_V1 1
#define CONFIG_S16 1

#if CONFIG_MFN_V1
#if CONFIG_S8
#include "face_recognition_112_v1_s8.hpp"
#elif CONFIG_S16
#include "face_recognition_112_v1_s16.hpp"
#endif
#endif

#include "who_ai_utils.hpp"

#if MICROPY_HW_ESPAI
#include "modespai.h"
#endif

using namespace std;
using namespace dl;

static const char *TAG = "face_recognition";

static QueueHandle_t xQueueFrameI = NULL;
static QueueHandle_t xQueueEvent = NULL;
static QueueHandle_t xQueueFrameO = NULL;
static QueueHandle_t xQueueResult = NULL;
static QueueHandle_t xQueueDeID = NULL;


static TaskHandle_t process_handler = NULL;
static TaskHandle_t event_handler = NULL;

static recognizer_state_t gEvent = DETECT;
static bool gReturnFB = true;
static face_info_t recognize_result;

static uint16_t setMOde_result;
static uint16_t delte_ID = 0;

SemaphoreHandle_t xMutex;

typedef enum
{
    SHOW_STATE_IDLE,
    SHOW_STATE_DELETE,
    SHOW_STATE_RECOGNIZE,
    SHOW_STATE_ENROLL,
} show_state_t;

#define RGB565_MASK_RED 0xF800
#define RGB565_MASK_GREEN 0x07E0
#define RGB565_MASK_BLUE 0x001F
#define FRAME_DELAY_NUM 16
#if MICROPY_HW_ESPAI

static void get_recognition_result(std::list<dl::detect::result_t> &results)
{
    int i = 0;
	pFdetection.results = 0;

    for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
    {
		if(i < 1){
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
static void rgb_print(camera_fb_t *fb, uint32_t color, const char *str)
{
    fb_gfx_print(fb, (fb->width - (strlen(str) * 14)) / 2, 10, color, str);
}

static int rgb_printf(camera_fb_t *fb, uint32_t color, const char *format, ...)
{
    char loc_buf[64];
    char *temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(copy);
    if (len >= sizeof(loc_buf))
    {
        temp = (char *)malloc(len + 1);
        if (temp == NULL)
        {
            return 0;
        }
    }
    vsnprintf(temp, len + 1, format, arg);
    va_end(arg);
    rgb_print(fb, color, temp);
    if (len > 64)
    {
        free(temp);
    }
    return len;
}

static void task_process_handler(void *arg)
{
    camera_fb_t *frame = NULL;
    HumanFaceDetectMSR01 detector(0.3F, 0.3F, 10, 0.3F);
    HumanFaceDetectMNP01 detector2(0.4F, 0.3F, 10);

#if CONFIG_MFN_V1
#if CONFIG_S8
    FaceRecognition112V1S8 *recognizer = new FaceRecognition112V1S8();
#elif CONFIG_S16
    FaceRecognition112V1S16 *recognizer = new FaceRecognition112V1S16();
#endif
#endif
    show_state_t frame_show_state = SHOW_STATE_IDLE;
    recognizer_state_t _gEvent;
    recognizer->set_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "fr");
    // int partition_result = recognizer->set_ids_from_flash();
	recognizer->set_ids_from_flash();

    while (true)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        _gEvent = gEvent;
        gEvent = DETECT;
        xSemaphoreGive(xMutex);

        if (_gEvent)
        {
            bool is_detected = false;

            if (xQueueReceive(xQueueFrameI, &frame, portMAX_DELAY))
            {
                std::list<dl::detect::result_t> &detect_candidates = detector.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3});
                std::list<dl::detect::result_t> &detect_results = detector2.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3}, detect_candidates);

                if (detect_results.size() == 1)
                    is_detected = true;
				
				if(_gEvent == DELETE){

					vTaskDelay(10);

					xQueueReceive(xQueueDeID, &delte_ID, portMAX_DELAY);
					if(delte_ID){
						recognizer->delete_id(delte_ID,true);
					}else{
						recognizer->delete_id(true);
					}
					
					ESP_LOGI("DELETE", "% d IDs left", recognizer->get_enrolled_id_num());

					if(xQueueResult){
						setMOde_result = recognizer->get_enrolled_id_num();
						xQueueSend(xQueueResult, &setMOde_result, portMAX_DELAY);
					}

					frame_show_state = SHOW_STATE_DELETE;
				}
                if (is_detected)
                {
                    switch (_gEvent)
                    {
                    case ENROLL:  //录入成功
                        recognizer->enroll_id((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3}, detect_results.front().keypoint, "", true);
                        ESP_LOGI("ENROLL", "ID %d is enrolled", recognizer->get_enrolled_ids().back().id);
                        frame_show_state = SHOW_STATE_ENROLL;
						
						if(xQueueResult){
							setMOde_result = recognizer->get_enrolled_ids().back().id;
							xQueueSend(xQueueResult, &setMOde_result, portMAX_DELAY);
						}
						
                        break;

                    case RECOGNIZE:
                        recognize_result = recognizer->recognize((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3}, detect_results.front().keypoint);
                        //print_detection_result(detect_results);

                            ESP_LOGI("RECOGNIZE", "Similarity: %f, Match ID: %d,", recognize_result.similarity, recognize_result.id);

						#if MICROPY_HW_ESPAI
							get_recognition_result(detect_results);
							if (recognize_result.id > 0){
								setMOde_result  = recognize_result.id;
								pFdetection.results = recognize_result.id;
							}else{
								pFdetection.results = 0;
								setMOde_result = 0;
							}
						#endif
							if(xQueueResult){
								xQueueSend(xQueueResult, &setMOde_result, portMAX_DELAY);	
							}

                        frame_show_state = SHOW_STATE_RECOGNIZE;
                        break;

                    // case DELETE:  //铲除ID
					
                        // vTaskDelay(10);
						
                        // recognizer->delete_id(true);
						
                        // ESP_LOGI("DELETE", "% d IDs left", recognizer->get_enrolled_id_num());
						
						// if(xQueueResult){
							// setMOde_result = recognizer->get_enrolled_id_num();
							// xQueueSend(xQueueResult, &setMOde_result, portMAX_DELAY);
						// }

                        // frame_show_state = SHOW_STATE_DELETE;
                        // break;

                    default:
					
                        break;
                    }
                }

                if (frame_show_state != SHOW_STATE_IDLE)
                {
                    static int frame_count = 0;
                    switch (frame_show_state)
                    {
                    case SHOW_STATE_DELETE:
                        rgb_printf(frame, RGB565_MASK_RED, "%d IDs left", recognizer->get_enrolled_id_num());
                        break;

                    case SHOW_STATE_RECOGNIZE:
                        if (recognize_result.id > 0){
							rgb_printf(frame, RGB565_MASK_GREEN, "ID %d,%0.2f", recognize_result.id, recognize_result.similarity);
						}
                        else{
							rgb_print(frame, RGB565_MASK_RED, "who ?");
						}
                        break;

                    case SHOW_STATE_ENROLL:
                        rgb_printf(frame, RGB565_MASK_BLUE, "Enroll: ID %d", recognizer->get_enrolled_ids().back().id);
                        break;

                    default:
                        break;
                    }

                    if (++frame_count > FRAME_DELAY_NUM)
                    {
                        frame_count = 0;
                        frame_show_state = SHOW_STATE_IDLE;
                    }
                }

                if (detect_results.size())
                {
#if !CONFIG_IDF_TARGET_ESP32S3
                    print_detection_result(detect_results);
#endif
                    draw_detection_result((uint16_t *)frame->buf, frame->height, frame->width, detect_results);
                }
            }

            if (xQueueFrameO)
            {

                xQueueSend(xQueueFrameO, &frame, portMAX_DELAY);
            }
            // else if (gReturnFB)
            // {
                // esp_camera_fb_return(frame);
            // }
            // else
            // {
                // free(frame);
            // }

            // if (xQueueResult && is_detected)
            // {
                // xQueueSend(xQueueResult, &recognize_result, portMAX_DELAY);
            // }
        }
    }
}

static void task_event_handler(void *arg)
{
    recognizer_state_t _gEvent;
    while (true)
    {
        xQueueReceive(xQueueEvent, &(_gEvent), portMAX_DELAY);
        xSemaphoreTake(xMutex, portMAX_DELAY);
        gEvent = _gEvent;
        xSemaphoreGive(xMutex);
    }
}

void register_human_face_recognition(const QueueHandle_t frame_i,
                                     const QueueHandle_t event,
                                     const QueueHandle_t result,
                                     const QueueHandle_t frame_o,
                                     const QueueHandle_t delteId)
{
    xQueueFrameI = frame_i;
    xQueueFrameO = frame_o;
    xQueueEvent = event;
    xQueueResult = result;
    xQueueDeID = delteId;
    xMutex = xSemaphoreCreateMutex();

	xTaskCreate( task_process_handler, TAG, (4 * 1024) / sizeof(StackType_t), NULL, 5, &process_handler );

    // xTaskCreatePinnedToCore(task_process_handler, TAG, 4 * 1024, NULL, 5, NULL, 0);
    if (xQueueEvent)
        // xTaskCreatePinnedToCore(task_event_handler, TAG, 4 * 1024, NULL, 5, NULL, 1);
		xTaskCreate( task_event_handler, TAG, (4 * 1024) / sizeof(StackType_t), NULL, 5, &event_handler );
}
