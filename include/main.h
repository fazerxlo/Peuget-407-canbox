#ifndef MAIN_H
#define MAIN_H

#include "stm32f1xx_hal.h"  // Include the HAL
#include <stdbool.h>
#include <stdint.h>

// --- Defines (Moved from main.c) ---


// --- Android -> CANBox Commands ---
#define CMD_RESET       "RST"
#define CMD_KEY         "KEY"
#define CMD_GET_VER     "VER"
#define CMD_CFG         "CFG"

// --- CANBox -> Android Responses ---
#define RESP_KEY        "KEY"
#define RESP_OK         "OK"
#define RESP_IGN        "IGN"
#define RESP_ILLUM      "ILL"
#define RESP_REV        "REV"
#define RESP_PARK       "PARK"
#define RESP_DOOR       "DOOR"
#define RESP_VER        "VER"
#define RESP_ERR        "ERR"
#define RESP_CFG        "CFG"

// --- Error Codes ---
#define ERR_INVALID_COMMAND  "INVALID_CMD"
#define ERR_INVALID_CONFIG   "INVALID_CFG"
#define ERR_CAN_ERROR       "CAN_ERROR" // Generic CAN error

// --- Ignition Modes (from 0x036) ---
#define IGN_MODE_OFF        0x00
#define IGN_MODE_ACCESSORY  0x01
#define IGN_MODE_ON         0x02
#define IGN_MODE_START      0x03
#define IGN_MODE_UNKNOWN    0x07

// --- UART Baud Rate ---
#define UART_BAUD_RATE 38400 // Moved here from main.c

// --- Extern Declarations (for global variables) ---
extern uint32_t config_ign_src;
extern uint32_t config_illum_src;
extern uint32_t config_rev_src;
extern uint32_t config_park_src;
extern uint32_t config_door_src;

void Error_Handler(void);
void LoadConfig(void); // Assuming LoadConfig is in main.c

#endif // MAIN_H
