#ifndef __JPEGD2_H
#define __JPEGD2_H 

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "cdjpeg.h" 
// #include <sys.h> 
#include <setjmp.h>

#ifdef __cplusplus 
extern "C" {
#endif

void mjpegdraw(uint8_t *mjpegbuffer, uint32_t size, uint16_t *outbuffer);

#ifdef __cplusplus 
}
#endif

#endif
