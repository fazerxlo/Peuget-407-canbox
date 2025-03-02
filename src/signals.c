#include "signals.h"
#ifndef USE_QEMU
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

void CheckStatusSignals(void) {
    // --- State variables are now static within this function ---
    static bool ign_state_local = false;
    static bool illum_state_local = false;
    static bool park_state_local = false;
    static bool rear_state_local = false;
    //Removed, because door is handled in can.c
    //static bool door_state_local = false;

    // --- Set IGN/ACC ---
    HAL_GPIO_WritePin(IGN_PORT, IGN_PIN, ign_state_local ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // --- Set ILLUM ---
    HAL_GPIO_WritePin(ILLUM_PORT, ILLUM_PIN, illum_state_local ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // --- Set PARK ---
    HAL_GPIO_WritePin(PARK_PORT, PARK_PIN, park_state_local ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // --- Set REAR ---
    HAL_GPIO_WritePin(REAR_PORT, REAR_PIN, rear_state_local ? GPIO_PIN_SET : GPIO_PIN_RESET);

    //No need to set door status, it is just for information.
}
#endif // Not USE_QEMU

#ifdef USE_QEMU

void CheckStatusSignals(void) {
    
}

#endif // USE_QEMU


