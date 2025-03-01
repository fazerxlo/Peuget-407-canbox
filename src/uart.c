#include "uart.h"
#include "commands.h" // For ProcessAndroidCommand

#ifndef USE_QEMU
UART_HandleTypeDef huart1; // Define huart1 here
#endif

void UART_Init(void) {
// ... (All the UART initialization code from the original main.c) ...
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUD_RATE; // Now defined in main.h
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    // Enable UART RX interrupt
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}
#ifndef USE_QEMU
void USART1_IRQHandler(void) {
 // ... (All the UART RX interrupt handler code) ...
     if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        static uint8_t rx_buffer[64];
        static uint8_t rx_index = 0;
        uint8_t received_byte = (uint8_t)(huart1.Instance->DR & 0xFF);

        if (received_byte == '\n') {
            rx_buffer[rx_index] = '\0'; // Null-terminate
            char command[4] = {0};
            char value[32] = {0};
            char *token;
            char *saveptr;

            token = strtok_r((char *)rx_buffer, ":\n", &saveptr);
            if (token != NULL && token[0] == '!') {
                strncpy(command, token + 1, 3);
                token = strtok_r(NULL, ":\n", &saveptr);
                if (token != NULL) {
                    strncpy(value, token, sizeof(value) - 1);
                }
                ProcessAndroidCommand(command, value);
            }
            rx_index = 0; // Reset for the next message.
        } else {
            if (rx_index < sizeof(rx_buffer) - 1) {
                rx_buffer[rx_index++] = received_byte;
            } // else:  Buffer overflow - handle or ignore
        }
        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
    }
    HAL_UART_IRQHandler(&huart1);
}
#endif

void SendToAndroid(const char *response, const char *value) {
#ifdef USE_QEMU
    printf("!%s:%s\n", response, value);
    fflush(stdout);
#else
    char message[64];
    snprintf(message, sizeof(message), "!%s:%s\n", response, value);
    HAL_UART_Transmit(&huart1, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
#endif
}

void ReceiveFromAndroid(void) {
#ifndef USE_QEMU
    // UART RX is interrupt-driven, so this function is empty in the real HW build
#else
 // QEMU version - Reads from stdin
    // ... (QEMU stdin handling - Remains unchanged) ...
     char buffer[256];
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        char command[4] = {0};
        char value[32] = {0};
        char *token;
        char *saveptr;

        token = strtok_r(buffer, ":\n", &saveptr);
        if (token != NULL && token[0] == '!') {
            strncpy(command, token + 1, 3);
            token = strtok_r(NULL, ":\n", &saveptr);
            if (token != NULL) {
                strncpy(value, token, sizeof(value) - 1);
            }
            ProcessAndroidCommand(command, value);
        }
    }
#endif
}