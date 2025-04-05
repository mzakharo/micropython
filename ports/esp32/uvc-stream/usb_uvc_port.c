// Copyright 2019-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "py/obj.h"

#include "esp_system.h"
#include "soc/efuse_reg.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "uvc_stream.h"

#include "usb_uvc_port.h"
//#include "esp_camera.h"

#if CONFIG_IDF_TARGET_ESP32
#include "esp32/spiram.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/spiram.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/spiram.h"
#endif

/* USB PIN fixed in esp32-s2, can not use io matrix */
#define BOARD_USB_DP_PIN 20
#define BOARD_USB_DN_PIN 19

/* USB Camera Descriptors Related MACROS,
the quick demo skip the standred get descriptors process,
users need to get params from camera descriptors from PC side,
eg. run `lsusb -v` in linux,
then hardcode the related MACROS below
*/

#define DESCRIPTOR_FRAME_5FPS_INTERVAL  2000000
#define DESCRIPTOR_FRAME_10FPS_INTERVAL 1000000
#define DESCRIPTOR_FRAME_15FPS_INTERVAL 666666
#define DESCRIPTOR_FRAME_30FPS_INTERVAL 333333

#define DESCRIPTOR_STREAM_ISOC_ENDPOINT_ADDR 0x85

#define XFER_BUFFER_SIZE (65 * 1024) //Double buffer

/* max packet size of esp32-s2 is 1*512, bigger is not supported*/
#define ISOC_EP_MPS 512

#define BIT1_NEW_FRAME_START (0x01 << 1)
#define BIT2_NEW_FRAME_END (0x01 << 2)

static const char *TAG = "uvc_demo";
static EventGroupHandle_t s_evt_handle;
static uvcam_fb_t s_fb = {0};

static void *_malloc(size_t size)
{
  return m_malloc(size);
}

/***************************************************************************************
 * Wi-Fi HTTP Transfer Implements
 *  
*/
uvcam_fb_t* uvc_camera_fb_get()
{
    xEventGroupWaitBits(s_evt_handle, BIT1_NEW_FRAME_START, true, true, portMAX_DELAY);
    ESP_LOGV(TAG, "peek frame = %ld", s_fb.timestamp.tv_sec);
    return &s_fb;
}

void uvc_camera_fb_return(uvcam_fb_t * fb)
{
    ESP_LOGV(TAG, "release frame = %ld", fb->timestamp.tv_sec);
    xEventGroupSetBits(s_evt_handle, BIT2_NEW_FRAME_END);
    return;
}

/* *******************************************************************************************
 * This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
static void frame_cb(uvc_frame_t *frame, void *ptr)
{
    // ESP_LOGV(TAG, "callback! frame_format = %d, seq = %u, width = %d, height = %d, length = %u, ptr = %d",
            // frame->frame_format, frame->sequence, frame->width, frame->height, frame->data_bytes, (int) ptr);

    ESP_LOGI(TAG, "callback! frame_format = %d, seq = %u, width = %d, height = %d, length = %u, ptr = %d",
            frame->frame_format, frame->sequence, frame->width, frame->height, frame->data_bytes, (int) ptr);
			
    switch (frame->frame_format) {
        case UVC_FRAME_FORMAT_MJPEG:
            s_fb.buf = frame->data;
            s_fb.len = frame->data_bytes;
            s_fb.width = frame->width;
            s_fb.height = frame->height;
            s_fb.buf = frame->data;
            s_fb.format = UVCFORMAT_JPEG;
            s_fb.timestamp.tv_sec = frame->sequence;
            xEventGroupSetBits(s_evt_handle, BIT1_NEW_FRAME_START);
            ESP_LOGV(TAG, "send frame = %u",frame->sequence);
            xEventGroupWaitBits(s_evt_handle, BIT2_NEW_FRAME_END, true, true, portMAX_DELAY);
            ESP_LOGV(TAG, "send frame done = %u",frame->sequence);

            break;
        default:
            ESP_LOGW(TAG, "Format not supported");
            assert(0);
            break;
    }
}

#if CONFIG_IDF_TARGET_ESP32S3
static void usb_otg_router_to_internal_phy()
{
    uint32_t *usb_phy_sel_reg = (uint32_t *)(0x60008000 + 0x120);
    *usb_phy_sel_reg |= BIT(19) | BIT(20);
}
#endif

esp_err_t uvc_app_init(uint16_t width, uint16_t height, uint8_t FormatIndex, uint8_t FrameIndex)
{
#if CONFIG_IDF_TARGET_ESP32S3
    usb_otg_router_to_internal_phy();
#endif
    /* create eventgroup for task sync */
    s_evt_handle = xEventGroupCreate();
    if (s_evt_handle == NULL) {
        ESP_LOGE(TAG, "line-%u event group create faild", __LINE__);
        assert(0);
    }

    /* malloc double buffer for usb payload, xfer_buffer_size >= frame_buffer_size*/
    uint8_t *xfer_buffer_a = (uint8_t *)_malloc(XFER_BUFFER_SIZE);
    assert(xfer_buffer_a != NULL);
    uint8_t *xfer_buffer_b = (uint8_t *)_malloc(XFER_BUFFER_SIZE);
    assert(xfer_buffer_b != NULL);

    /* malloc frame buffer for a jpeg frame*/
    uint8_t *frame_buffer = (uint8_t *)_malloc(XFER_BUFFER_SIZE);
    assert(frame_buffer != NULL);

    /* the quick demo skip the standred get descriptors process,
    users need to get params from camera descriptors from PC side,
    eg. run `lsusb -v` in linux, then modify related MACROS */

		uvc_config_t uvc_config = {
			.dev_speed = USB_SPEED_FULL,
			.configuration = 1, /*!< bConfigurationValue */
			.format_index = FormatIndex,  /*!< bFormatIndex */
			.frame_width = width, /*!< wWidth */
			.frame_height = height, /*!< wHeight */
			.frame_index = FrameIndex, /*!< bFrameIndex */
			.frame_interval = DESCRIPTOR_FRAME_15FPS_INTERVAL, /*!< dwFrameInterval */
			.interface = 1,   /*!< bInterfaceNumber */
			.interface_alt = 4,   /*!< bAlternateSetting, ep MPS must =< 512 */
			.isoc_ep_addr = 0x82, /*!< bEndpointAddress */
			.isoc_ep_mps = ISOC_EP_MPS,  /*!< MPS size of bAlternateSetting */
			.xfer_buffer_size = XFER_BUFFER_SIZE, 
			.xfer_buffer_a = xfer_buffer_a,
			.xfer_buffer_b = xfer_buffer_b,
			.frame_buffer_size = XFER_BUFFER_SIZE,
			.frame_buffer = frame_buffer,
		};

    /* pre-config UVC driver with params from known USB Camera Descriptors*/
    esp_err_t ret = uvc_streaming_config(&uvc_config);

    /* Start camera IN stream with pre-configs, uvc driver will create multi-tasks internal
    to handle usb data from different pipes, and user's callback will be called after new frame ready. */
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uvc streaming config failed");
    } else {
        uvc_streaming_start(frame_cb, NULL);
    }

	return ret;
}
