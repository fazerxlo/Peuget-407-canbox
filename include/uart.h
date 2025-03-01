#ifndef UART_H
#define UART_H

#include "main.h"  // For HAL and other common defines

#ifndef USE_QEMU
#include "stm32f1xx_hal_uart.h"
extern UART_HandleTypeDef huart1; // Declare huart1 as external
#endif

void UART_Init(void);
void SendToAndroid(const char *response, const char *value);
void ReceiveFromAndroid(void);
#ifndef USE_QEMU
void USART1_IRQHandler(void);
#endif
#endif // UART_H
