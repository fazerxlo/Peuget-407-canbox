#ifndef CAN_H
#define CAN_H

#include "main.h" // For HAL and other common defines

#ifndef USE_QEMU
#include "stm32f1xx_hal_can.h"
extern CAN_HandleTypeDef hcan; // Declare hcan as external
#endif

void CAN_Init(void); // Declaration for hardware
#ifdef USE_QEMU
int CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len); // QEMU version prototype
#else
void CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len); // STM32 version prototype
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan); // For hardware only
#endif
void ProcessCanMessage(uint32_t can_id, uint8_t *data, uint8_t data_len);


#endif // CAN_H