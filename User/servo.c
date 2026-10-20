#include "servo.h"

/* 两个轴的初始状态：都在中位 */
Axis_t g_axis[AXIS_COUNT + 1] =
{
    { 0,   0,   0,   0 },      /* [0] 占位，不使用 */
    { 90,  90,  10,  170 },    /* [1] 偏航 */
    { 90,  90,  20,  160 }     /* [2] 俯仰 */
};

/* ================= 双通道 PWM 初始化 ================= */
void Servo_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,  ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_TimeBaseStructure.TIM_Prescaler     = 71;
    TIM_TimeBaseStructure.TIM_Period        = 19999;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse       = 1500;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);

    TIM_OCInitStructure.TIM_Pulse = 1500;
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);

    TIM_Cmd(TIM2, ENABLE);
}

static uint16_t Servo_AngleToPulse(int16_t deg)
{
    uint32_t us;
    us = (uint32_t)SERVO_MIN_US
       + (uint32_t)deg * (SERVO_MAX_US - SERVO_MIN_US) / SERVO_ANGLE_SPAN;
    return (uint16_t)us;
}

/* ================= 立即设置某个轴的角度（内部会限位） ================= */
void Servo_SetAngle(uint8_t axis, int16_t deg)
{
    uint16_t us;

    if (axis == 0 || axis > AXIS_COUNT) return;

    if (deg < g_axis[axis].minDeg) deg = g_axis[axis].minDeg;
    if (deg > g_axis[axis].maxDeg) deg = g_axis[axis].maxDeg;

    us = Servo_AngleToPulse(deg);

    if (axis == AXIS_YAW) TIM_SetCompare1(TIM2, us);
    else                  TIM_SetCompare2(TIM2, us);
}

/* ================= 只设定目标，运动交给中断 ================= */
void Servo_MoveTo(uint8_t axis, int16_t deg)
{
    if (axis == 0 || axis > AXIS_COUNT) return;

    if (deg < g_axis[axis].minDeg) deg = g_axis[axis].minDeg;
    if (deg > g_axis[axis].maxDeg) deg = g_axis[axis].maxDeg;

    g_axis[axis].target = deg;
}

/* ================= 每 10ms 调用一次：每个轴走 1 度 ================= */
void Servo_Update(void)
{
    uint8_t i;

    for (i = 1; i <= AXIS_COUNT; i++)
    {
        if (g_axis[i].current < g_axis[i].target)      g_axis[i].current++;
        else if (g_axis[i].current > g_axis[i].target) g_axis[i].current--;

        Servo_SetAngle(i, g_axis[i].current);
    }
}