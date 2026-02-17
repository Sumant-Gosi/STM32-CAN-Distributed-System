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

/* ─────────────────────────────────────────────────
 * vHeartbeatTask
 * Blinks onboard LED every 500ms
 * Proves the FreeRTOS scheduler is alive
 * ───────────────────────────────────────────────── */
void vHeartbeatTask(void *argument)
{
    UART_Log("HEARTBEAT", "Task started");

    for(;;)
    {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        UART_Log("HEARTBEAT", "Alive");
        osDelay(500);
    }
}

/* ─────────────────────────────────────────────────
 * vCANTransmitTask
 * Sends RPM, Temperature, Status over CAN
 * every 100ms — simulates a real ECU
 * ───────────────────────────────────────────────── */
void vCANTransmitTask(void *argument)
{
    UART_Log("CAN_TX", "Task started");

    uint16_t rpm  = 800;       /* Idle RPM */
    int16_t  temp = 25;        /* Room temp */
    uint8_t  status = NODE_STATUS_OK;

    for(;;)
    {
        /* Simulate slowly rising RPM */
        rpm += 100;
        if(rpm > 6000) rpm = 800;

        /* Simulate slowly rising temperature */
        temp += 1;
        if(temp > 100) temp = 25;

        /* Transmit all values over CAN */
        CAN_App_TransmitRPM(rpm);
        CAN_App_TransmitTemp(temp);
        CAN_App_TransmitStatus(status);

        osDelay(100);   /* Send every 100ms */
    }
}

/* ─────────────────────────────────────────────────
 * vCANReceiveTask
 * Waits for frames from the RX queue
 * Queue is populated by the CAN RX interrupt
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
        /* Block here until a frame arrives in the queue */
        if(osMessageQueueGet(canRxQueueHandle, &frame, NULL, osWaitForever) == osOK)
        {
            switch(frame.id)
            {
                case CAN_ID_RPM:
                {
                    uint16_t rpm = ((uint16_t)frame.data[0] << 8) | frame.data[1];
                    UART_Log_Int("CAN_RX", "RPM received", rpm);
                    break;
                }
                case CAN_ID_TEMP:
                {
                    int16_t temp = ((int16_t)frame.data[0] << 8) | frame.data[1];
                    UART_Log_Int("CAN_RX", "TEMP received", temp);
                    break;
                }
                case CAN_ID_STATUS:
                {
                    UART_Log_Int("CAN_RX", "STATUS received", frame.data[0]);
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
 * vUARTLogTask
 * Lowest priority task
 * Handles any extra log messages from a queue
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
