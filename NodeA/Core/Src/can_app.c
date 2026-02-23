/*
 * can_app.c
 *
 *  Created on: Feb 17, 2026
 *      Author: sumanthgosi
 */


#include "can_app.h"
#include "uart_log.h"

/* ── Private Variables ───────────────────────── */
static CAN_HandleTypeDef *_hcan;
static CAN_TxHeaderTypeDef TxHeader;
// static uint8_t TxData[8];
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
 * Call this once before starting the scheduler
 * ───────────────────────────────────────────────── */
void CAN_App_Init(CAN_HandleTypeDef *hcan)
{
    _hcan = hcan;

    /* Create the RX queue — holds up to 10 CAN frames */
    canRxQueueHandle = osMessageQueueNew(10, sizeof(CAN_Frame_t), NULL);

    /* Configure RX Filter — accept ALL messages for now */
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

    /* Start CAN peripheral */
    HAL_CAN_Start(_hcan);

    /* Enable RX interrupt */
    HAL_CAN_ActivateNotification(_hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

    UART_Log("CAN", "Initialized OK");
}

/* ─────────────────────────────────────────────────
 * Internal helper — builds and sends a CAN frame
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
    data[0] = (rpm >> 8) & 0xFF;   /* High byte */
    data[1] = rpm & 0xFF;           /* Low byte  */
    CAN_Send(CAN_ID_RPM, data, 2);
    UART_Log_Int("CAN", "TX RPM", rpm);
}

void CAN_App_TransmitTemp(int16_t temp)
{
    uint8_t data[2];
    data[0] = (temp >> 8) & 0xFF;
    data[1] = temp & 0xFF;
    CAN_Send(CAN_ID_TEMP, data, 2);
    UART_Log_Int("CAN", "TX TEMP", temp);
}

void CAN_App_TransmitStatus(uint8_t status)
{
    uint8_t data[1];
    data[0] = status;
    CAN_Send(CAN_ID_STATUS, data, 1);
    UART_Log_Int("CAN", "TX STATUS", status);
}

void CAN_App_TransmitAck(uint8_t ackCode)
{
    uint8_t data[1];
    data[0] = ackCode;
    CAN_Send(CAN_ID_ACK, data, 1);
    UART_Log_Int("CAN", "TX ACK", ackCode);
}

/* ─────────────────────────────────────────────────
 * CAN RX Interrupt Callback
 * Called automatically by HAL when a frame arrives
 * DO NOT call this yourself — HAL calls it
 * ───────────────────────────────────────────────── */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    CAN_Frame_t frame;

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, frame.data) == HAL_OK)
    {
        frame.id  = RxHeader.StdId;
        frame.dlc = RxHeader.DLC;

        /* Send frame to queue from ISR — task will process it */
        osMessageQueuePut(canRxQueueHandle, &frame, 0, 0);
    }
}
