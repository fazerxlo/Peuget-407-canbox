#include "can.h"
#include "config.h" // For configuration parameters
#include "commands.h" // For SendToAndroid
#include "signals.h" //For defines

#ifndef USE_QEMU
CAN_HandleTypeDef hcan; // Define hcan here
#endif

#ifndef USE_QEMU
// ... (Rest of CAN_Init and HAL_CAN_RxFifo0MsgPendingCallback - No Changes) ...
void CAN_Init(void) {
    // ... existing code ...
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
// ... existing code
}
#else
// QEMU CAN Transmit (using SocketCAN)
#include <stdio.h>      // For perror
#include <string.h>     // For memset, memcpy, strncpy
#include <unistd.h>     // For close
#include <sys/socket.h> // For socket functions
#include <sys/ioctl.h>  // For ioctl
#include <net/if.h>     // For network interface
#include <linux/can.h>  // For CAN structures
#include <linux/can/raw.h> // For raw CAN sockets

int CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len) {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame; // Moved declaration inside the #ifdef

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return -1;
    }

    strncpy(ifr.ifr_name, "can0", IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0'; // Ensure null termination
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        close(s);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        close(s);
        return -1;
    }

    frame.can_id = id;
    frame.can_dlc = len;
    if (len > 8) len = 8;
    memcpy(frame.data, data, len);

    if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Write");
        close(s);
        return -1;
    }

    close(s);
    return 0;
}
#endif

void CAN_Transmit(uint32_t id, uint8_t *data, uint8_t len) {
#ifndef USE_QEMU
    // ... (Existing STM32 CAN Transmit code - No Changes) ...
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
#endif
}

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
