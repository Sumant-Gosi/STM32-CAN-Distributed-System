/*
 * can_app.c
 *
 *  Created on: Feb 23, 2026
 *      Author: sumanthgosi
 */


#include "can_app.h"
#include "uart_log.h"

/* ── Private Variables ───────────────────────── */
static CAN_HandleTypeDef *_hcan;
static CAN_TxHeaderTypeDef TxHeader;
static uint32_t TxMailbox;

/* ── RX Queue ────────────────────────────────── */
osMessageQueueId_t canRxQueueHandle;

/* ── Received Frame Structure ────────────────── */
typedef struct {
    uint32_t id;
    uint8_t  data[8];
    uint8_t  dlc;
} CAN_Frame_t;

/* ─────────────────────────────────────────────────
 * CAN_App_Init
 * ───────────────────────────────────────────────── */
void CAN_App_Init(CAN_HandleTypeDef *hcan)
{
    _hcan = hcan;

    /* Create the RX queue */
    canRxQueueHandle = osMessageQueueNew(10, sizeof(CAN_Frame_t), NULL);

    /* Configure RX Filter — accept ALL messages */
    CAN_FilterTypeDef filter;
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = 0x0000;
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = 0x0000;
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(_hcan, &filter);

    /* Start CAN */
    HAL_CAN_Start(_hcan);

    /* Enable RX interrupt */
    HAL_CAN_ActivateNotification(_hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

    UART_Log("CAN", "Initialized OK");
}

/* ─────────────────────────────────────────────────
 * Internal helper — sends a CAN frame
 * ───────────────────────────────────────────────── */
static void CAN_Send(uint32_t id, uint8_t *data, uint8_t len)
{
    TxHeader.StdId              = id;
    TxHeader.IDE                = CAN_ID_STD;
    TxHeader.RTR                = CAN_RTR_DATA;
    TxHeader.DLC                = len;
    TxHeader.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(_hcan, &TxHeader, data, &TxMailbox) != HAL_OK)
    {
        UART_Log("CAN", "TX ERROR");
    }
}

/* ─────────────────────────────────────────────────
 * Public TX Functions
 * ───────────────────────────────────────────────── */
void CAN_App_TransmitRPM(uint16_t rpm)
{
    uint8_t data[2];
    data[0] = (rpm >> 8) & 0xFF;
    data[1] = rpm & 0xFF;
    CAN_Send(CAN_ID_RPM, data, 2);
    UART_Log_Int("CAN_TX", "RPM", rpm);
}

void CAN_App_TransmitTemp(int16_t temp)
{
    uint8_t data[2];
    data[0] = (temp >> 8) & 0xFF;
    data[1] = temp & 0xFF;
    CAN_Send(CAN_ID_TEMP, data, 2);
    UART_Log_Int("CAN_TX", "TEMP", temp);
}

void CAN_App_TransmitHeartbeat(void)
{
    uint8_t data[1] = {0xAA};  // Arbitrary alive signal
    CAN_Send(CAN_ID_HEARTBEAT, data, 1);
    UART_Log("CAN_TX", "Heartbeat");
}

void CAN_App_TransmitCommand(uint8_t cmdCode)
{
    uint8_t data[1];
    data[0] = cmdCode;
    CAN_Send(CAN_ID_COMMAND, data, 1);
    UART_Log_Int("CAN_TX", "COMMAND", cmdCode);
}

void CAN_App_TransmitAck(uint8_t ackedCmd)
{
    uint8_t data[1];
    data[0] = ackedCmd;
    CAN_Send(CAN_ID_ACK, data, 1);
    UART_Log_Int("CAN_TX", "ACK", ackedCmd);
}

/* ─────────────────────────────────────────────────
 * CAN RX Interrupt Callback
 * ───────────────────────────────────────────────── */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    CAN_Frame_t frame;

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, frame.data) == HAL_OK)
    {
        frame.id  = RxHeader.StdId;
        frame.dlc = RxHeader.DLC;

        osMessageQueuePut(canRxQueueHandle, &frame, 0, 0);
    }
}
