/*
 * can.h
 *
 *  Created on: Aug 12, 2026
 *      Author: oriol
 */

#ifndef INC_CAN_H_
#define INC_CAN_H_

#include "main.h"

void CAN_Init_Custom(FDCAN_HandleTypeDef *hfdcan);
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);

#endif /* INC_CAN_H_ */
