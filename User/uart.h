#ifndef __UART_H
#define __UART_H

#include "config.h"

void UART_Init(uint32_t baud);
void UART_SendChar(char c);
void UART_SendString(char *s);

#endif
