#include "config.h"
#include "servo.h"
#include "uart.h"

/* ================= TIM3：10ms 中断驱动运动 ================= */
static void TIM3_Init_10ms(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef        NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseStructure.TIM_Prescaler     = 7199;   /* 72MHz / 7200 = 10kHz */
    TIM_TimeBaseStructure.TIM_Period        = 99;     /* 100 个 = 10ms */
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel                   = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM3, ENABLE);
}

void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        Servo_Update();          /* 每 10ms 让两个轴各走 1 度 */
    }
}

/* ================= 主函数 ================= */
int main(void)
{
    Servo_Init();
    UART_Init(UART_BAUDRATE);
    TIM3_Init_10ms();

    /* 让两轴平滑回到中位（不是瞬间跳过去） */
    Servo_MoveTo(AXIS_YAW,   90);
    Servo_MoveTo(AXIS_PITCH, 90);

    UART_SendString("Gimbal ready\r\n");

    while (1)
    {
        /* 主循环空闲：运动由 TIM3 中断推进，指令由串口中断接收 */
    }
}
