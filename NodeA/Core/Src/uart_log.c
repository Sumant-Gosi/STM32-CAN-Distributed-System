/*
 * uart_log.c
 *
 *  Created on: Feb 17, 2026
 *      Author: sumanthgosi
 */


#include "uart_log.h"

static UART_HandleTypeDef *_huart;

void UART_Log_Init(UART_HandleTypeDef *huart)
{
    _huart = huart;
}

void UART_Log(const char *tag, const char *message)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "[%s] %s\r\n", tag, message);
    HAL_UART_Transmit(_huart, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
}

void UART_Log_Int(const char *tag, const char *message, int value)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "[%s] %s: %d\r\n", tag, message, value);
    HAL_UART_Transmit(_huart, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
}
