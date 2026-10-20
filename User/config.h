#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f10x.h"

/* ================= 舵机参数 ================= */
#define SERVO_MIN_US       500
#define SERVO_MAX_US       2500
#define SERVO_ANGLE_SPAN   180

/* ================= 轴编号 ================= */
#define AXIS_YAW    1      /* 偏航：PA0 / TIM2_CH1 */
#define AXIS_PITCH  2      /* 俯仰：PA1 / TIM2_CH2 */
#define AXIS_COUNT  2

/* ================= 轴的状态与范围 ================= */
typedef struct
{
    int16_t current;       /* 当前角度 */
    int16_t target;        /* 目标角度 */
    int16_t minDeg;        /* 安全下限 */
    int16_t maxDeg;        /* 安全上限 */
} Axis_t;

extern Axis_t g_axis[AXIS_COUNT + 1];    /* 下标 1、2 对应两个轴 */

/* ================= 串口参数 ================= */
#define UART_BAUDRATE   115200
#define RX_BUF_SIZE     32

#endif
