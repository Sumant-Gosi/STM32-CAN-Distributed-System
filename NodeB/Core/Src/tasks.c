/*
 * tasks.c
 *
 *  Created on: Feb 17, 2026
 *      Author: sumanthgosi
 */


#include "tasks.h"
#include "main.h"

/* ── Log Queue ───────────────────────────────── */
osMessageQueueId_t logQueueHandle;

/* ── Latest received data ────────────────────── */
static uint16_t g_rpm    = 0;
static int16_t  g_temp   = 0;
static uint8_t  g_status = 0;

/* ─────────────────────────────────────────────────
 * vHeartbeatTask
 * Blinks LED every 500ms — proves scheduler alive
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
 * Waits for frames from RX queue
 * Populated by CAN RX interrupt
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
        /* Block until frame arrives */
        if(osMessageQueueGet(canRxQueueHandle, &frame, NULL, osWaitForever) == osOK)
        {
            switch(frame.id)
            {
                case CAN_ID_RPM:
                {
                    g_rpm = ((uint16_t)frame.data[0] << 8) | frame.data[1];
                    UART_Log_Int("CAN_RX", "RPM", g_rpm);

                    /* Warn if RPM too high */
                    if(g_rpm > 5000)
                    {
                        UART_Log("CAN_RX", "WARNING - High RPM!");
                        CAN_App_TransmitStatus(NODE_STATUS_WARN);
                    }
                    break;
                }
                case CAN_ID_TEMP:
                {
                    g_temp = ((int16_t)frame.data[0] << 8) | frame.data[1];
                    UART_Log_Int("CAN_RX", "TEMP", g_temp);

                    /* Warn if temperature too high */
                    if(g_temp > 80)
                    {
                        UART_Log("CAN_RX", "WARNING - High Temp!");
                        CAN_App_TransmitStatus(NODE_STATUS_WARN);
                    }
                    break;
                }
                case CAN_ID_STATUS:
                {
                    g_status = frame.data[0];
                    UART_Log_Int("CAN_RX", "STATUS", g_status);

                    /* Send dedicated ACK frame back to Node A */
                    CAN_App_TransmitAck(NODE_STATUS_OK);
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
 * Node B only sends ACK and status responses
 * Not periodic like Node A
 * ───────────────────────────────────────────────── */
void vCANTransmitTask(void *argument)
{
    UART_Log("CAN_TX", "Task started");

    for(;;)
    {
        /* Node B sends keepalive status periodically */
        CAN_App_TransmitStatus(NODE_STATUS_OK);
        osDelay(2000);
    }
}

/* ─────────────────────────────────────────────────
 * vUARTLogTask
 * Lowest priority — handles log queue messages
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
