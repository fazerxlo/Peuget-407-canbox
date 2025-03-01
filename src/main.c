#include "main.h"
#include "can.h"
#include "uart.h"
#include "config.h"
#include "signals.h"
#include "commands.h"
#include <stdio.h>

int main(void) {
    HAL_Init();
    SystemClock_Config();

#ifndef USE_QEMU
    CAN_Init();
    UART_Init();
    GPIO_Status_Init();
    load_config(); // Load configuration from flash
#endif

    send_version();  // Send version at startup (defined in commands.c)
    SendToAndroid(RESP_OK, "INIT");

    while (1) {
        ReceiveFromAndroid(); // UART reception is interrupt-driven
#ifndef USE_QEMU
        CheckStatusSignals(); // Update output signals
#endif
    }
}

void SystemClock_Config(void) {
    // ... (Clock configuration code - Remains unchanged) ...
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

void Error_Handler(void) {
    __disable_irq();
#ifndef USE_QEMU
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // Turn off LED

    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(100);
    }
#else
    fprintf(stderr, "Error occurred!\n");
    while (1) {}
#endif
}
