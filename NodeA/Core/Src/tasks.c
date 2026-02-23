/*
 * tasks.c
 *
 *  Created on: Feb 23, 2026
 *      Author: sumanthgosi
 */


#include "tasks.h"
#include "main.h"

/* ── Log Queue ───────────────────────────────── */
osMessageQueueId_t logQueueHandle;

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
 * vCANTransmitTask
 * Node A broadcasts sensor data periodically
 * ───────────────────────────────────────────────── */
void vCANTransmitTask(void *argument)
{
    UART_Log("CAN_TX", "Task started");

    uint16_t rpm  = 800;
    int16_t  temp = 25;

    for(;;)
    {
        /* Simulate slowly rising RPM */
        rpm += 100;
        if(rpm > 6000) rpm = 800;

        /* Simulate slowly rising temperature */
        temp += 1;
        if(temp > 100) temp = 25;

        /* Broadcast data - no ACK needed */
        CAN_App_TransmitRPM(rpm);
        CAN_App_TransmitTemp(temp);
        CAN_App_TransmitHeartbeat();

        osDelay(100);
    }
}

/* ─────────────────────────────────────────────────
 * vCANReceiveTask
 * Node A receives COMMANDS from Node B and ACKs them
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
                case CAN_ID_COMMAND:
                {
                    uint8_t cmd = frame.data[0];

                    /* IMMEDIATELY send ACK */
                    CAN_App_TransmitAck(cmd);

                    /* Handle the command */
                    switch(cmd)
                    {
                        case CMD_WARNING_HIGH_RPM:
                            UART_Log("COMMAND", "Node B detected HIGH RPM!");
                            // Take action: reduce throttle, log event, etc.
                            break;

                        case CMD_WARNING_HIGH_TEMP:
                            UART_Log("COMMAND", "Node B detected HIGH TEMP!");
                            // Take action: activate cooling, reduce load, etc.
                            break;

                        case CMD_REDUCE_POWER:
                            UART_Log("COMMAND", "Reducing power as requested");
                            break;

                        case CMD_ACTIVATE_COOLING:
                            UART_Log("COMMAND", "Activating cooling system");
                            break;

                        default:
                            UART_Log_Int("COMMAND", "Unknown command", cmd);
                            break;
                    }
                    break;
                }

                default:
                    // Ignore other messages (Node A doesn't care about HEARTBEAT from B, etc.)
                    break;
            }
        }
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
