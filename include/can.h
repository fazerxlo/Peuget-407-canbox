#ifndef CAN_H
#define CAN_H

#include "main.h" // For HAL and other common defines

#ifndef USE_QEMU
#include "stm32f1xx_hal_can.h"
extern CAN_HandleTypeDef hcan; // Declare hcan as external
#endif

void CAN_Init(void);
void CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len);
void ProcessCanMessage(uint32_t can_id, uint8_t *data, uint8_t data_len);
#ifndef USE_QEMU
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
#endif

#endif // CAN_H
