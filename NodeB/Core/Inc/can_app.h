/*
 * can_app.h
 *
 *  Created on: Feb 23, 2026
 *      Author: sumanthgosi
 */

#ifndef INC_CAN_APP_H_
#define INC_CAN_APP_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

/* ── CAN Message IDs ─────────────────────────── */
#define CAN_ID_RPM          0x100
#define CAN_ID_TEMP         0x101
#define CAN_ID_HEARTBEAT    0x102
#define CAN_ID_COMMAND      0x200
#define CAN_ID_ACK          0x201

/* ── Command Codes ───────────────────────────── */
#define CMD_WARNING_HIGH_RPM    0x01
#define CMD_WARNING_HIGH_TEMP   0x02
#define CMD_REDUCE_POWER        0x03
#define CMD_ACTIVATE_COOLING    0x04

/* ── Queue Handle (ISR → Task communication) ─── */
extern osMessageQueueId_t canRxQueueHandle;

/* ── Function Declarations ───────────────────── */
void CAN_App_Init(CAN_HandleTypeDef *hcan);
void CAN_App_TransmitRPM(uint16_t rpm);
void CAN_App_TransmitTemp(int16_t temp);
void CAN_App_TransmitHeartbeat(void);
void CAN_App_TransmitCommand(uint8_t cmdCode);
void CAN_App_TransmitAck(uint8_t ackedCmd);

#endif /* INC_CAN_APP_H_ */
