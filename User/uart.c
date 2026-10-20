#include "uart.h"
#include "servo.h"
#include <stdlib.h>      /* atoi */

static char    s_rxBuf[RX_BUF_SIZE];
static uint8_t s_rxLen = 0;

void UART_SendChar(char c)
{
    USART_SendData(USART1, (uint8_t)c);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendString(char *s)
{
    while (*s) UART_SendChar(*s++);
}

void UART_Init(uint32_t baud)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate            = baud;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

/* ================= 解析一条指令 ================= */
static void UART_ParseCommand(char *cmd)
{
    uint8_t axis;
    int     deg;

    if (cmd[0] == 'A' || cmd[0] == 'a')      axis = AXIS_YAW;
    else if (cmd[0] == 'B' || cmd[0] == 'b') axis = AXIS_PITCH;
    else { UART_SendString("ERR\r\n"); return; }

    deg = atoi(&cmd[1]);
    if (deg < 0 || deg > 180) { UART_SendString("ERR\r\n"); return; }

    Servo_MoveTo(axis, (int16_t)deg);   /* 设定目标，中断自己走过去 */
    UART_SendString("OK\r\n");
}

/* ================= 接收中断：只负责收字节和分帧 ================= */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        char c = (char)USART_ReceiveData(USART1);

        if (c == '\n' || c == '\r')
        {
            if (s_rxLen > 0)
            {
                s_rxBuf[s_rxLen] = '\0';
                UART_ParseCommand(s_rxBuf);
                s_rxLen = 0;
            }
        }
        else if (s_rxLen < RX_BUF_SIZE - 1)
        {
            s_rxBuf[s_rxLen++] = c;
        }
        else
        {
            s_rxLen = 0;
        }
    }
}