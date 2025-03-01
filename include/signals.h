#ifndef SIGNALS_H
#define SIGNALS_H

#include "main.h"

// --- GPIO Pins for Status Signals (only for real hardware) ---
//Moved from main.h
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

void GPIO_Status_Init(void);
void CheckStatusSignals(void);

#endif // SIGNALS_H
