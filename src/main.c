#include "stm32f1xx_hal.h"

#ifdef USE_QEMU
// These headers are ONLY for the QEMU build (using SocketCAN)
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#else
// These headers are for the REAL STM32 hardware
#include "stm32f1xx_hal_can.h"
#include "stm32f1xx_hal_uart.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#endif

// --- Defines ---
#ifdef USE_QEMU
//  No need to define CAN speed here, as SocketCAN handles it.
#else
#define CAN_BAUD_RATE 125000 // 125 kbps
#endif

#define UART_BAUD_RATE 38400 // 38400 baud
#define STEERING_WHEEL_CONTROLS_ID 0x165

// --- Button Codes (from DRIVE2.md) ---
#define BTN_SOURCE      0x01
#define BTN_VOL_UP      0x02
#define BTN_VOL_DOWN    0x04
#define BTN_SEEK_FWD    0x08
#define BTN_SEEK_BWD    0x10
#define BTN_OK          0x40
#define BTN_END_CALL    0x80

// --- Android -> CANBox Commands ---
#define CMD_RESET       "RST"
#define CMD_KEY         "KEY"
#define CMD_STEER_WHEEL "SWH"

// --- CANBox -> Android Responses ---
#define RESP_KEY        "KEY"
#define RESP_OK         "OK"
#define RESP_IGN        "IGN" // Ignition/ACC status
#define RESP_ILLUM      "ILL" // Illumination status
#define RESP_PARK       "PARK" // Parking brake status
#define RESP_REAR       "REAR" // Reverse gear status

// --- GPIO Pins for Status Signals (only for real hardware) ---
#ifndef USE_QEMU
#define IGN_PIN         GPIO_PIN_1
#define IGN_PORT        GPIOB
#define ILLUM_PIN       GPIO_PIN_2
#define ILLUM_PORT      GPIOB
#define PARK_PIN        GPIO_PIN_3
#define PARK_PORT       GPIOB
#define REAR_PIN        GPIO_PIN_4
#define REAR_PORT       GPIOB
#endif

// --- CAN ID for Reverse Camera Power (DETERMINE THIS!) ---
#define REAR_CAMERA_POWER_ID 0xXXX // Replace with the correct CAN ID (Real Hardware)

// --- Function Prototypes ---
void SystemClock_Config(void);
#ifndef USE_QEMU
void CAN_Init(void);
void UART_Init(void);
void GPIO_Status_Init(void);
void CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan); // Only for real hardware
void USART1_IRQHandler(void); // Only for real hardware
#else
int CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len);
void ProcessCanMessage(uint32_t can_id, uint8_t *data, uint8_t data_len); // QEMU version
#endif
void ProcessCanMessage(uint32_t can_id, uint8_t *data, uint8_t data_len);
void SendToAndroid(const char *response, const char *value);
void ProcessAndroidCommand(const char *command, const char *value);
void ReceiveFromAndroid(void);
void CheckStatusSignals(void);
void Error_Handler(void);

// --- Global Variables ---
#ifndef USE_QEMU
CAN_HandleTypeDef hcan;
UART_HandleTypeDef huart1;
#endif

int main(void) {
    HAL_Init();
    SystemClock_Config();
#ifndef USE_QEMU
    CAN_Init();
    UART_Init();
    GPIO_Status_Init();
#endif

    SendToAndroid(RESP_OK, "INIT");

    while (1) {
        ReceiveFromAndroid();
#ifndef USE_QEMU
        CheckStatusSignals();
#endif
    }
}

void SystemClock_Config(void)
{
// This clock configuration *should* work in both QEMU and on real hardware.

  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

#ifndef USE_QEMU
void CAN_Init(void) {
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // --- GPIO Configuration ---
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_11; // CAN RX (PA11)
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12; // CAN TX (PA12)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // --- CAN Peripheral Configuration ---
    hcan.Instance = CAN1;
    hcan.Init.Mode = CAN_MODE_NORMAL;
    hcan.Init.AutoBusOff = DISABLE;
    hcan.Init.AutoWakeUp = DISABLE;
    hcan.Init.AutoRetransmission = ENABLE;
    hcan.Init.ReceiveFifoLocked = DISABLE;
    hcan.Init.TransmitFifoPriority = DISABLE;
     hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;

    // --- Bit Timing for 125kbps (CRITICAL!) ---
    hcan.Init.Prescaler = 36;     // 72MHz / 36 = 2MHz , Tq = 0.5 us
    hcan.Init.TimeSeg1 = CAN_BS1_13TQ;  //  (1 + 13) * 0.5 us = 7 us
    hcan.Init.TimeSeg2 = CAN_BS2_2TQ;   // 2 * 0.5  us= 1 us
    // Total time = 7 us + 1 us = 8 us,  1 / 8 us = 125 kHz.

    if (HAL_CAN_Init(&hcan) != HAL_OK) {
        Error_Handler();
    }
    // --- CAN Filter Configuration ---
    CAN_FilterTypeDef canfilterconfig;

    canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig.FilterBank = 0;
    canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canfilterconfig.FilterIdHigh = STEERING_WHEEL_CONTROLS_ID << 5;
    canfilterconfig.FilterIdLow = 0x0000;
    canfilterconfig.FilterMaskIdHigh = 0xFFFF << 5;
    canfilterconfig.FilterMaskIdLow = 0x0000;
    canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
    canfilterconfig.SlaveStartFilterBank = 14; // Not relevant for single CAN

    if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig) != HAL_OK) {
      Error_Handler();
    }

    // --- Start CAN ---
    if(HAL_CAN_Start(&hcan) != HAL_OK){
        Error_Handler();
    }
    if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK){
        Error_Handler();
    }
}

void UART_Init(void) {
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
    huart1.Init.BaudRate = UART_BAUD_RATE;
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

void GPIO_Status_Init(void) {
      __HAL_RCC_GPIOB_CLK_ENABLE(); // Enable GPIOB clock

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // --- IGN/ACC Pin ---
    GPIO_InitStruct.Pin = IGN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; //  OUTPUT
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(IGN_PORT, &GPIO_InitStruct);

    // --- ILLUM Pin ---
    GPIO_InitStruct.Pin = ILLUM_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; //  OUTPUT
    GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ILLUM_PORT, &GPIO_InitStruct);

    // --- PARK Pin ---
    GPIO_InitStruct.Pin = PARK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; //  OUTPUT
    GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PARK_PORT, &GPIO_InitStruct);

    // --- REAR Pin ---
    GPIO_InitStruct.Pin = REAR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; //  OUTPUT
    GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(REAR_PORT, &GPIO_InitStruct);
}

void CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len) {
     CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;

    TxHeader.StdId = id;
    TxHeader.ExtId = 0;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = len;
    TxHeader.TransmitGlobalTime = DISABLE;

    if (len > 8) len = 8;
    memcpy(TxData, data, len);

    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
        Error_Handler();
    }
    while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan) != 3);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
     CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK) {
        Error_Handler();
    }

    ProcessCanMessage(RxHeader.StdId, RxData, RxHeader.DLC); // Pass StdId and DLC
}

void USART1_IRQHandler(void) {
  if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {

        static uint8_t rx_buffer[64];
        static uint8_t rx_index = 0;
        uint8_t received_byte = (uint8_t)(huart1.Instance->DR & 0xFF);

        if (received_byte == '\n') {
            rx_buffer[rx_index] = '\0'; // Null-terminate
            // Now using strtok_r directly here, processing immediately
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
            } //else overflow case should be handled

        }
         __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
    }
    HAL_UART_IRQHandler(&huart1);
}
#else
// QEMU CAN Transmit (using SocketCAN)
int CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len) {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Create a socket
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return -1;
    }

    // Set up the interface name
    strncpy(ifr.ifr_name, "can0", IFNAMSIZ - 1); // Use strncpy for safety
    ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // Ensure null termination
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        close(s);
        return -1;
    }

    // Set up the address
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Bind the socket
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        close(s);
        return -1;
    }

    // Prepare the CAN frame
    frame.can_id = id;
    frame.can_dlc = len;
    if (len > 8) len = 8; // Ensure we don't write past the buffer
    memcpy(frame.data, data, len);

    // Send the frame
    if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Write");
        close(s);
        return -1;
    }

    close(s);
    return 0;
}
#endif

void ProcessCanMessage(uint32_t can_id, uint8_t *data, uint8_t data_len) {
    if (can_id == STEERING_WHEEL_CONTROLS_ID) {
        //  Check for data length before accessing data[0]
        if (data_len > 0) {
            switch (data[0]) {
                case BTN_SOURCE:
                    SendToAndroid(RESP_KEY, "SRC");
                    break;
                case BTN_VOL_UP:
                    SendToAndroid(RESP_KEY, "VOL+");
                    break;
                case BTN_VOL_DOWN:
                    SendToAndroid(RESP_KEY, "VOL-");
                    break;
                case BTN_SEEK_FWD:
                    SendToAndroid(RESP_KEY, "SEEK+");
                    break;
                case BTN_SEEK_BWD:
                    SendToAndroid(RESP_KEY, "SEEK-");
                    break;
                case BTN_OK:
                    SendToAndroid(RESP_KEY, "OK");
                    break;
                case BTN_END_CALL:
                    SendToAndroid(RESP_KEY, "END");
                    break;
                default:
                    break;
            }
        }
    }
}


void SendToAndroid(const char *response, const char *value) {
#ifdef USE_QEMU
    printf("!%s:%s\n", response, value); // Use standard output for QEMU
    fflush(stdout); // Ensure the output is flushed immediately
#else
    char message[64];
    snprintf(message, sizeof(message), "!%s:%s\n", response, value);
    HAL_UART_Transmit(&huart1, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
#endif
}

void ProcessAndroidCommand(const char *command, const char *value) {
 if (strcmp(command, CMD_RESET) == 0) {
        HAL_NVIC_SystemReset(); // Consider re-initializing peripherals instead
    } else if (strcmp(command, CMD_KEY) == 0) {
        if (strcmp(value, "SRC") == 0) {
            uint8_t data[1] = {BTN_SOURCE};
            CAN_Transmit(STEERING_WHEEL_CONTROLS_ID, data, 1);
        }
        // ... other key simulations ...
    }
      else if (strcmp(command, CMD_STEER_WHEEL) == 0){
        // Steering wheel command.
    }
}

void ReceiveFromAndroid(void) {
 #ifndef USE_QEMU
    // For real hardware The UART RX interrupt handler now directly processes
    // the incoming data, so this function becomes a no-op.
#else
    //For QEMU
    char buffer[256];  //Local Buffer
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        // Remove trailing newline
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

void CheckStatusSignals(void) {
#ifndef USE_QEMU
    // --- State variables are now static within this function ---
    static bool ign_state = false;
    static bool illum_state = false;
    static bool park_state = false;
    static bool rear_state = false;

    // --- IGN/ACC ---
    bool current_ign_state = HAL_GPIO_ReadPin(IGN_PORT, IGN_PIN) == GPIO_PIN_SET;
    if (current_ign_state != ign_state) {
        ign_state = current_ign_state;
         SendToAndroid(RESP_IGN, ign_state ? "ON" : "OFF");

    }
    // --- Set IGN/ACC ---
    HAL_GPIO_WritePin(IGN_PORT, IGN_PIN, ign_state ? GPIO_PIN_SET : GPIO_PIN_RESET);


    // --- ILLUM ---
    bool current_illum_state = HAL_GPIO_ReadPin(ILLUM_PORT, ILLUM_PIN) == GPIO_PIN_SET;
    if (current_illum_state != illum_state) {
        illum_state = current_illum_state;
        SendToAndroid(RESP_ILLUM, illum_state ? "ON" : "OFF");
    }
    // --- Set ILLUM ---
    HAL_GPIO_WritePin(ILLUM_PORT, ILLUM_PIN, illum_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // --- PARK ---
    bool current_park_state = HAL_GPIO_ReadPin(PARK_PORT, PARK_PIN) == GPIO_PIN_RESET; // Active low
    if (current_park_state != park_state) {
        park_state = current_park_state;
        SendToAndroid(RESP_PARK, park_state ? "ON" : "OFF");
    }
    // --- Set PARK ---
    HAL_GPIO_WritePin(PARK_PORT, PARK_PIN, park_state ? GPIO_PIN_RESET : GPIO_PIN_SET);

    // --- REAR ---
    bool current_rear_state = HAL_GPIO_ReadPin(REAR_PORT, REAR_PIN) == GPIO_PIN_SET;
    if (current_rear_state != rear_state) {
        rear_state = current_rear_state;
        SendToAndroid(RESP_REAR, rear_state ? "ON" : "OFF");
    }

    // --- Set REAR ---
    HAL_GPIO_WritePin(REAR_PORT, REAR_PIN, rear_state ? GPIO_PIN_SET : GPIO_PIN_RESET);


#endif
}

void Error_Handler(void) {
    __disable_irq();
#ifndef USE_QEMU
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // Turn off LED in error.

    while (1) {

         HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
         HAL_Delay(100); // Indicate error by blinking on board led.
    }
#else
    while (1) {}
#endif
}