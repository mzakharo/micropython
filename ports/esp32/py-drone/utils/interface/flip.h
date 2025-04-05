#ifndef __FLIP_H
#define __FLIP_H 
#include "stabilizer_types.h"


void flyerFlipCheck(setpoint_t* setpoint,control_t* control,state_t* state);	/* Flyer 翻滚检测*/

void setFlipDir(uint8_t dir);

#endif 

