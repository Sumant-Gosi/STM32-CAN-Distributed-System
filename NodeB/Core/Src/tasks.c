/*
 * tasks.c
 *
 *  Created on: Feb 23, 2026
 *      Author: sumanthgosi
 */


#include "tasks.h"
#include "main.h"
#include <stdbool.h>

/* ── Log Queue ───────────────────────────────── */
osMessageQueueId_t logQueueHandle;

/* ── Latest received data ────────────────────── */
static uint16_t g_rpm    = 0;
static int16_t  g_temp   = 0;

/* ── ACK tracking ────────────────────────────── */
static volatile bool ackReceived = false;

/* ─────────────────────────────────────────────────
 * vHeartbeatTask
 * ───────────────────────────────────────────────── */
void vHeartbeatTask(void *argument)
{
    UART_Log("HEARTBEAT", "Task started");

    for(;;)
    {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        osDelay(500);
    }
}

/* ─────────────────────────────────────────────────
 * vCANReceiveTask
 * Node B receives data, evaluates, sends commands
 * ───────────────────────────────────────────────── */
void vCANReceiveTask(void *argument)
{
    UART_Log("CAN_RX", "Task started");

    typedef struct {
        uint32_t id;
        uint8_t  data[8];
        uint8_t  dlc;
    } CAN_Frame_t;

    CAN_Frame_t frame;

    for(;;)
    {
        if(osMessageQueueGet(canRxQueueHandle, &frame, NULL, osWaitForever) == osOK)
        {
            switch(frame.id)
            {
                case CAN_ID_RPM:
                {
                    g_rpm = ((uint16_t)frame.data[0] << 8) | frame.data[1];
                    UART_Log_Int("CAN_RX", "RPM", g_rpm);

                    /* Threshold check */
                    if(g_rpm > 5000)
                    {
                        UART_Log("WARNING", "RPM threshold exceeded!");
                        CAN_App_TransmitCommand(CMD_WARNING_HIGH_RPM);

                        /* Optional: Wait for ACK with timeout */
                        ackReceived = false;
                        uint32_t startTime = osKernelGetTickCount();

                        while(!ackReceived && (osKernelGetTickCount() - startTime) < 200)
                        {
                            osDelay(10);
                        }

                        if(!ackReceived)
                        {
                            UART_Log("ERROR", "Node A did not ACK command!");
                        }
                    }
                    break;
                }

                case CAN_ID_TEMP:
                {
                    g_temp = ((int16_t)frame.data[0] << 8) | frame.data[1];
                    UART_Log_Int("CAN_RX", "TEMP", g_temp);

                    /* Threshold check */
                    if(g_temp > 80)
                    {
                        UART_Log("WARNING", "Temperature threshold exceeded!");
                        CAN_App_TransmitCommand(CMD_WARNING_HIGH_TEMP);

                        /* Optional: Wait for ACK with timeout */
                        ackReceived = false;
                        uint32_t startTime = osKernelGetTickCount();

                        while(!ackReceived && (osKernelGetTickCount() - startTime) < 200)
                        {
                            osDelay(10);
                        }

                        if(!ackReceived)
                        {
                            UART_Log("ERROR", "Node A did not ACK command!");
                        }
                    }
                    break;
                }

                case CAN_ID_HEARTBEAT:
                {
                    UART_Log("CAN_RX", "Heartbeat from Node A");
                    break;
                }

                case CAN_ID_ACK:
                {
                    uint8_t ackedCmd = frame.data[0];
                    UART_Log_Int("CAN_RX", "ACK received for command", ackedCmd);
                    ackReceived = true;  // Signal to waiting task
                    break;
                }

                default:
                {
                    UART_Log_Int("CAN_RX", "Unknown ID", frame.id);
                    break;
                }
            }
        }
    }
}

/* ─────────────────────────────────────────────────
 * vCANTransmitTask
 * Node B doesn't send periodic data, only commands
 * This task can be used for periodic health checks
 * ───────────────────────────────────────────────── */
void vCANTransmitTask(void *argument)
{
    UART_Log("CAN_TX", "Task started");

    for(;;)
    {
        /* Node B could send its own heartbeat if needed */
        // CAN_App_TransmitHeartbeat();
        osDelay(2000);
    }
}

/* ─────────────────────────────────────────────────
 * vUARTLogTask
 * ───────────────────────────────────────────────── */
void vUARTLogTask(void *argument)
{
    UART_Log("LOG", "Task started");

    char msg[128];

    for(;;)
    {
        if(osMessageQueueGet(logQueueHandle, msg, NULL, osWaitForever) == osOK)
        {
            UART_Log("LOG", msg);
        }
    }
}
