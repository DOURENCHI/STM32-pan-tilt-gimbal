#ifndef __SERVO_H
#define __SERVO_H

#include "config.h"

void Servo_Init(void);
void Servo_SetAngle(uint8_t axis, int16_t deg);
void Servo_MoveTo(uint8_t axis, int16_t deg);
void Servo_Update(void);        /* 由定时器中断周期调用 */

#endif
