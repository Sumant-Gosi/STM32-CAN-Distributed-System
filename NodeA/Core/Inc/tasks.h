/*
 * tasks.h
 *
 *  Created on: Feb 17, 2026
 *      Author: sumanthgosi
 */

#ifndef INC_TASKS_H_
#define INC_TASKS_H_

#include "cmsis_os.h"
#include "can_app.h"
#include "uart_log.h"

/* ── Task Function Declarations ──────────────── */
void vCANTransmitTask(void *argument);
void vCANReceiveTask(void *argument);
void vHeartbeatTask(void *argument);
void vUARTLogTask(void *argument);

/* ── Log Queue ───────────────────────────────── */
extern osMessageQueueId_t logQueueHandle;

#endif /* INC_TASKS_H_ */
