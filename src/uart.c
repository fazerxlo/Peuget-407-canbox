#include "uart.h"
#include "commands.h" // For ProcessAndroidCommand
#include "main.h"  // For Error_Handler
#include <stdio.h>    // Required for QEMU build (printf, fflush)
#include <string.h>   // Required for string functions.

UART_HandleTypeDef huart1; // Define huart1 here

// --- Circular Buffer ---
#define RX_BUFFER_SIZE 128 // Increased buffer size for more robustness
typedef struct {
    uint8_t buffer[RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile bool overflow;
} CircularBuffer;

CircularBuffer rx_buffer;

// --- Circular Buffer Functions ---

/**
 * @brief Initializes the circular buffer.
 */
void CircularBuffer_Init(CircularBuffer *cb) {
    cb->head = 0;
    cb->tail = 0;
}

/**
 * @brief Adds a byte to the circular buffer.
 * @param cb Pointer to the circular buffer.
 * @param data Byte to add.
 * @return true if successful, false if buffer is full.
 */
bool CircularBuffer_Push(CircularBuffer *cb, uint8_t data) {
    uint16_t next_head = (cb->head + 1) % RX_BUFFER_SIZE;
    if (next_head == cb->tail) {
        // Buffer full
        return false;
    }
    cb->buffer[cb->head] = data;
    cb->head = next_head;
    return true;
}

/**
 * @brief Retrieves a byte from the circular buffer.
 * @param cb Pointer to the circular buffer.
 * @param data Pointer to store the retrieved byte.
 * @return true if successful, false if buffer is empty.
 */
bool CircularBuffer_Pop(CircularBuffer *cb, uint8_t *data) {
    if (cb->head == cb->tail) {
        // Buffer empty
        return false;
    }
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % RX_BUFFER_SIZE;
    return true;
}

// --- UART Initialization ---

void UART_Init(void) {
    // Initialize the circular buffer
    CircularBuffer_Init(&rx_buffer);

    // Enable USART1 and GPIO clocks
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

    // Initialize UART parameters
    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUD_RATE; // Defined in main.h
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    // Initialize UART with HAL
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    // Enable UART RX interrupt
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}

// --- UART Interrupt Handler (STM32) ---
#ifndef USE_QEMU
void USART1_IRQHandler(void) {
    // Check if there is RX data available and no error occurred.
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) &&
        !(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) ||
          __HAL_UART_GET_FLAG(&huart1, UART_FLAG_NE) ||
          __HAL_UART_GET_FLAG(&huart1, UART_FLAG_FE))) {
        uint8_t received_byte = (uint8_t)(huart1.Instance->DR & 0xFF);

        // Push the received byte into the circular buffer
        if (!CircularBuffer_Push(&rx_buffer, received_byte)) {
            // Buffer overflow, handle error (e.g., log error, discard data)
            // You can add custom error handling here, e.g.:
             // Error_Handler(); // Or a custom error handler
        }
    }
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart1); //Clear Overrun error flag
    }
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_NE)) {
       __HAL_UART_CLEAR_NEFLAG(&huart1); //Clear Noise Error flag
    }
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_FE)) {
        __HAL_UART_CLEAR_FEFLAG(&huart1); //Clear Framing Error flag
    }
    // Process the buffer
    ProcessReceivedData();

    HAL_UART_IRQHandler(&huart1);
}
#endif
// --- Data Processing ---
void ProcessReceivedData(void) {
    static uint8_t process_buffer[RX_BUFFER_SIZE]; // Buffer to store message to process
    static uint16_t process_index = 0;            // Index for message to process

    uint8_t byte;
    while (CircularBuffer_Pop(&rx_buffer, &byte)) {
        if (byte == '\n') {
            process_buffer[process_index] = '\0'; // Null-terminate
            // Process the received message
            char command[4] = {0};
            char value[32] = {0};
            char *token;
            char *saveptr;
            token = strtok_r((char *)process_buffer, ":\n", &saveptr);
            if (token != NULL && token[0] == '!') {
                strncpy(command, token + 1, 3);
                token = strtok_r(NULL, ":\n", &saveptr);
                if (token != NULL) {
                    strncpy(value, token, sizeof(value) - 1);
                }
                ProcessAndroidCommand(command, value);
            }

            // Reset for the next message
            process_index = 0;
        } else {
            // Add byte to process buffer
            if (process_index < sizeof(process_buffer) - 1) {
                process_buffer[process_index++] = byte;
            } else {
                // Process buffer overflow, reset and discard received message
                process_index = 0;
            }
        }
    }
}
#ifdef USE_QEMU
void QEMU_UART_Transmit(uint8_t *data, uint16_t len){
     printf("%s",data);
     fflush(stdout);
}
void QEMU_UART_Receive(uint8_t *data, uint16_t len){
    //This function is not used in this version
    //it can be used to emulate UART receive in QEMU
}
#endif


void ReceiveFromAndroid(void) {
    #ifndef USE_QEMU // Real Hardware implementation
        static char command_buffer[64]; // Buffer to store the complete command
        static uint8_t command_index = 0;
        int16_t received_byte;
    
    
        while (is_data_available()) { // Keep reading until the buffer is empty
            received_byte = read_byte();
    
            if (received_byte == -1) break; // Should not occur, since is_data_available checked
    
            if (received_byte == '\n') {
                // End of command.  Process it.
                command_buffer[command_index] = '\0'; // Null-terminate
                char command[4] = {0};
                char value[32] = {0};
                char *token;
                char *saveptr;
    
                token = strtok_r(command_buffer, ":\n", &saveptr);
                if (token != NULL && token[0] == '!') {
                    strncpy(command, token + 1, 3); // Extract command
                    token = strtok_r(NULL, ":\n", &saveptr);  //Extract Value
                    if (token != NULL) {
                        strncpy(value, token, sizeof(value) - 1);
                    }
                    ProcessAndroidCommand(command, value);
                }
                command_index = 0; // Reset for the next command
            } else {
                // Add the byte to the command buffer (if there's space)
                if (command_index < sizeof(command_buffer) - 1) {
                    command_buffer[command_index++] = (char)received_byte;
                } else {
                    // Command buffer overflow.  Handle this (e.g., reset index, send error)
                    command_index = 0; // Simple reset.  A better approach might be to send an error.
                }
            }
            if(rx_buffer.overflow == true){
                //You can handle overflow here.
                rx_buffer.overflow = false; //Reset flag
                command_index = 0; // Clear the command buffer in case of an overflow.
            }
        }
    #else
       // QEMU version - Reads from stdin - NO CHANGES
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