#include "can.h"
#include "config.h" // For configuration parameters
#include "commands.h" // For SendToAndroid
#include "signals.h" //For defines
#include "string.h"

#ifndef USE_QEMU
CAN_HandleTypeDef hcan; // Define hcan here for hardware builds
#endif

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
    // Steering Wheel Controls
    CAN_FilterTypeDef canfilterconfig;
    canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig.FilterBank = 0;
    canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canfilterconfig.FilterIdHigh = STEERING_WHEEL_CONTROLS_ID << 5;
    canfilterconfig.FilterIdLow = 0x0000;
    canfilterconfig.FilterMaskIdHigh = 0x7ff << 5;
    canfilterconfig.FilterMaskIdLow = 0x0000;
    canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
    canfilterconfig.SlaveStartFilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig) != HAL_OK) {
        Error_Handler();
    }

    // Ignition Status
    CAN_FilterTypeDef canfilterconfig_ign;
    canfilterconfig_ign.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig_ign.FilterBank = 1;
    canfilterconfig_ign.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canfilterconfig_ign.FilterIdHigh = IGNITION_STATUS_ID << 5;
    canfilterconfig_ign.FilterIdLow = 0x0000;
    canfilterconfig_ign.FilterMaskIdHigh = 0x7ff << 5;
    canfilterconfig_ign.FilterMaskIdLow = 0x0000;
    canfilterconfig_ign.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilterconfig_ign.FilterScale = CAN_FILTERSCALE_32BIT;
    canfilterconfig_ign.SlaveStartFilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig_ign) != HAL_OK) {
        Error_Handler();
    }

    // Illumination and Park
    CAN_FilterTypeDef canfilterconfig_illum;
    canfilterconfig_illum.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig_illum.FilterBank = 2;
    canfilterconfig_illum.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canfilterconfig_illum.FilterIdHigh = DASHBOARD_LIGHTS_ID << 5;
    canfilterconfig_illum.FilterIdLow = 0x0000;
    canfilterconfig_illum.FilterMaskIdHigh = 0x7ff << 5;
    canfilterconfig_illum.FilterMaskIdLow = 0x0000;
    canfilterconfig_illum.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilterconfig_illum.FilterScale = CAN_FILTERSCALE_32BIT;
    canfilterconfig_illum.SlaveStartFilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig_illum) != HAL_OK) {
        Error_Handler();
    }

    // Reverse Gear
    CAN_FilterTypeDef canfilterconfig_rear;
    canfilterconfig_rear.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig_rear.FilterBank = 3;
    canfilterconfig_rear.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canfilterconfig_rear.FilterIdHigh = REVERSE_GEAR_ID << 5;
    canfilterconfig_rear.FilterIdLow = 0x0000;
    canfilterconfig_rear.FilterMaskIdHigh = 0x7ff << 5;
    canfilterconfig_rear.FilterMaskIdLow = 0x0000;
    canfilterconfig_rear.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilterconfig_rear.FilterScale = CAN_FILTERSCALE_32BIT;
    canfilterconfig_rear.SlaveStartFilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig_rear) != HAL_OK) {
        Error_Handler();
    }

    // Door Status
    CAN_FilterTypeDef canfilterconfig_door;
    canfilterconfig_door.FilterActivation = CAN_FILTER_ENABLE;
    canfilterconfig_door.FilterBank = 4;
    canfilterconfig_door.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canfilterconfig_door.FilterIdHigh = DOOR_STATUS_ID << 5;
    canfilterconfig_door.FilterIdLow = 0x0000;
    canfilterconfig_door.FilterMaskIdHigh = 0x7ff << 5;
    canfilterconfig_door.FilterMaskIdLow = 0x0000;
    canfilterconfig_door.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilterconfig_door.FilterScale = CAN_FILTERSCALE_32BIT;
    canfilterconfig_door.SlaveStartFilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan, &canfilterconfig_door) != HAL_OK) {
        Error_Handler();
    }

    // --- Start CAN ---
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
   CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK) {
        Error_Handler(); //  CAN RX error
    }

    ProcessCanMessage(RxHeader.StdId, RxData, RxHeader.DLC);
}
#endif // #ifndef USE_QEMU

#ifdef USE_QEMU
// QEMU CAN Transmit (using SocketCAN)
#include <stdio.h>      // For perror
#include <string.h>     // For memset, memcpy, strncpy
#include <unistd.h>     // For close


void CAN_Init(void) {

}

int CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len) {
    int s;
    s = 0;
    close(s);
    return 0;
}
#endif //#ifdef USE_QEMU

#ifndef USE_QEMU
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
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) != 3);
}
#endif // #ifndef USE_QEMU

void ProcessCanMessage(uint32_t can_id, uint8_t *data, uint8_t data_len) {
// ... (Existing ProcessCanMessage code - No Changes) ...
	 static bool ign_state = false;
    static bool illum_state = false;
    static bool park_state = false;
    static bool rear_state = false;
    static bool door_state = false;

    if (can_id == STEERING_WHEEL_CONTROLS_ID) {
        if (data_len > 0) {
            // Use fazerxlo key codes directly
            switch (data[0]) {
                case 0x01: SendToAndroid(RESP_KEY, "SRC"); break;
                case 0x02: SendToAndroid(RESP_KEY, "VOL+"); break;
                case 0x04: SendToAndroid(RESP_KEY, "VOL-"); break;
                case 0x08: SendToAndroid(RESP_KEY, "SEEK+"); break;
                case 0x10: SendToAndroid(RESP_KEY, "SEEK-"); break;
                case 0x40: SendToAndroid(RESP_KEY, "OK"); break;
                case 0x80: SendToAndroid(RESP_KEY, "END"); break;
                // Add other key codes as needed, matching fazerxlo/canbox
                default: break;
            }
        }
    } else if (can_id == config_ign_src) {
        if (data_len >= 1) {
            uint8_t ign_mode = data[0] & 0x07;
            switch (ign_mode) {
                case IGN_MODE_OFF:
                    if (ign_state != false) {
                        ign_state = false;
                        SendToAndroid(RESP_IGN, "OFF");
                    }
                    break;
                case IGN_MODE_ACCESSORY:
                case IGN_MODE_ON:
                case IGN_MODE_START:
                    if (ign_state != true) {
                        ign_state = true;
                        SendToAndroid(RESP_IGN, "ON");
                    }
                    break;
                default:
                    if (ign_state != false){
                        ign_state = false;
                        SendToAndroid(RESP_IGN, "OFF");
                    }
                    break;
            }
        }
    } else if (can_id == config_illum_src) {
        if (data_len >= 1) {
            bool low_beam_on = (data[0] & 0x10) != 0; // Only low beam
            if (low_beam_on != illum_state) {
                illum_state = low_beam_on;
                SendToAndroid(RESP_ILLUM, illum_state ? "ON" : "OFF");
            }
        }
    } else if (can_id == config_park_src) {
        if (data_len >= 1) {
            bool new_park_state = (data[0] & 0x80) != 0;
            if (new_park_state != park_state) {
                park_state = new_park_state;
                SendToAndroid(RESP_PARK, park_state ? "ON" : "OFF");
            }
        }
    } else if (can_id == config_rev_src) {
        if (data_len >= 1) {
            bool new_rear_state = (data[0] & 0x04) != 0;
            if (new_rear_state != rear_state) {
                rear_state = new_rear_state;
                SendToAndroid(RESP_REV, rear_state ? "ON" : "OFF"); // Changed to REV
            }
        }
    }  else if (can_id == config_door_src) { // Door Status
        if (data_len >= 1) {
            //This is just an example how to handle door status
            //Check PSACAN.md for details, here showed only for one door
            bool front_left_door = (data[0] & 0x80) != 0; //Bit 7 for example

            //You can implement your logic here, for example if any door opened
            //send information to Android.
            if(front_left_door != door_state){
                door_state = front_left_door;
                SendToAndroid(RESP_DOOR, door_state ? "OPEN" : "CLOSE");
            }
        }
    }
}
