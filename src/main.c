#include "main.h"
#include "uart.h"
#include "can.h"
#include "commands.h"
#include "config.h"
#include "signals.h"

void SystemClock_Config(void);
void Error_Handler(void);

void SystemClock_Config(void) {
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

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    // Disable interrupts.  This prevents any further interrupts from
    // interfering with the reset process.
    __disable_irq();

    // Optional:  Send an error message to the Android head unit *before*
    // resetting.  This is useful for debugging.
    SendToAndroid(RESP_ERR, "FATAL_ERROR");

    // Add a short delay to allow the UART transmission to complete.
    // This is important; otherwise, the message might not be sent before
    // the reset.
    HAL_Delay(100);

    // Re-initialize peripherals.  This is crucial for a clean restart.
    // We call the same initialization functions as in main().

#ifndef USE_QEMU  // Only re-initialize hardware peripherals on the real device
    HAL_DeInit(); // Deinitialize HAL. Reset all peripherals to default state.
    HAL_Init();   // Reinitialize HAL
    SystemClock_Config(); // Reconfigure the system clock
    CAN_Init();         // Re-initialize the CAN peripheral
    UART_Init();        // Re-initialize the UART peripheral
    GPIO_Status_Init(); // Re-initialize GPIO pins
#endif

    // Perform a software reset.
    HAL_NVIC_SystemReset();

    // The code should never reach here, but it's good practice to have
    // an infinite loop, just in case.
    while (1) {}
}

int main(void) {
    HAL_Init();

    SystemClock_Config();

    UART_Init();
    CAN_Init();

    while (1) {
        CheckStatusSignals();
    }
}
