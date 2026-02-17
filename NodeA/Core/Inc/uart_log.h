/*
 * uart_log.h
 *
 *  Created on: Feb 17, 2026
 *      Author: sumanthgosi
 */

#ifndef INC_UART_LOG_H_
#define INC_UART_LOG_H_

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

void UART_Log_Init(UART_HandleTypeDef *huart);
void UART_Log(const char *tag, const char *message);
void UART_Log_Int(const char *tag, const char *message, int value);

#endif /* INC_UART_LOG_H_ */

