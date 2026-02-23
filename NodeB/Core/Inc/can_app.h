/*
 * can_app.h
 *
 *  Created on: Feb 17, 2026
 *      Author: sumanthgosi
 */

#ifndef INC_CAN_APP_H_
#define INC_CAN_APP_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

/* ── CAN Message IDs ─────────────────────────── */
#define CAN_ID_RPM          0x100
#define CAN_ID_TEMP         0x101
#define CAN_ID_STATUS       0x102
#define CAN_ID_ACK          0x200

/* ── Node Status Values ──────────────────────── */
#define NODE_STATUS_OK      0x01
#define NODE_STATUS_WARN    0x02
#define NODE_STATUS_FAULT   0x03

/* ── Data Structure shared between tasks ─────── */
typedef struct {
    uint16_t rpm;
    int16_t  temperature;
    uint8_t  status;
} CANNodeData_t;

/* ── Queue Handle (ISR → Task communication) ─── */
extern osMessageQueueId_t canRxQueueHandle;

/* ── Function Declarations ───────────────────── */
void CAN_App_Init(CAN_HandleTypeDef *hcan);
void CAN_App_TransmitRPM(uint16_t rpm);
void CAN_App_TransmitTemp(int16_t temp);
void CAN_App_TransmitStatus(uint8_t status);
void CAN_App_TransmitAck(uint8_t ackCode);

#endif /* INC_CAN_APP_H_ */
