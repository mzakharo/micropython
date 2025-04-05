// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*
 * Example Use
 *
*/

#pragma once

#include "esp_err.h"
#include "sys/time.h"
//#include "esp_camera.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UVCFORMAT_RGB565,    // 2BPP/RGB565
    UVCFORMAT_YUV422,    // 2BPP/YUV422
    UVCFORMAT_GRAYSCALE, // 1BPP/GRAYSCALE
    UVCFORMAT_JPEG,      // JPEG/COMPRESSED
    UVCFORMAT_RGB888,    // 3BPP/RGB888
    UVCFORMAT_RAW,       // RAW
    UVCFORMAT_RGB444,    // 3BP2P/RGB444
    UVCFORMAT_RGB555,    // 3BP2P/RGB555
} uvc_format_t;

/**
 * @brief Data structure of camera frame buffer
 */
typedef struct {
    uint8_t * buf;              /*!< Pointer to the pixel data */
    size_t len;                 /*!< Length of the buffer in bytes */
    size_t width;               /*!< Width of the buffer in pixels */
    size_t height;              /*!< Height of the buffer in pixels */
    uvc_format_t format;         /*!< Format of the pixel data */
    struct timeval timestamp;   /*!< Timestamp since boot of the first DMA buffer of the frame */
} uvcam_fb_t;

/************************************** Functions user implement ************************************************/
/**
 * @brief Obtain pointer to a frame buffer.
 *
 * @return pointer to the frame buffer
 */
uvcam_fb_t* uvc_camera_fb_get();

/**
 * @brief Return the frame buffer to be reused again.
 *
 * @param fb    Pointer to the frame buffer
 */
void uvc_camera_fb_return(uvcam_fb_t * fb);
esp_err_t uvc_app_init(uint16_t width, uint16_t height, uint8_t FormatIndex, uint8_t FrameIndex);
#ifdef __cplusplus
}
#endif
