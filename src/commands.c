#include "commands.h"
#include "uart.h" // For SendToAndroid
#include "config.h" // For configuration
#include <string.h>
#include <stdio.h>

void ProcessAndroidCommand(const char *command, const char *value) {
 // ... (ProcessAndroidCommand code - Moved here) ...
     if (strcmp(command, CMD_RESET) == 0) {
        HAL_NVIC_SystemReset();
    } else if (strcmp(command, CMD_KEY) == 0) {
        // fazerxlo uses the same key naming, no need for simulation in this example
        // If you wanted to *simulate* key presses on the *car* side, you'd
        // use CAN_Transmit here, sending the appropriate CAN message.
    } else if (strcmp(command, CMD_GET_VER) == 0) {
        send_version();
    } else if (strcmp(command, CMD_CFG) == 0) {
        char parameter[32] = {0};
        char val[32] = {0};
        char *token;
        char *saveptr;

        token = strtok_r((char *)value, ":", &saveptr);
        if (token != NULL) {
            strncpy(parameter, token, sizeof(parameter) - 1);
            token = strtok_r(NULL, ":", &saveptr);
            if (token != NULL) {
                strncpy(val, token, sizeof(val) - 1);
                ProcessConfigCommand(parameter, val); //Moved to config.c
            } else {
                SendToAndroid(RESP_ERR, ERR_INVALID_CONFIG);
            }
        } else {
            SendToAndroid(RESP_ERR, ERR_INVALID_CONFIG);
        }
    } else {
        SendToAndroid(RESP_ERR, ERR_INVALID_COMMAND); // Unknown command
    }
}
void send_version(void){
#ifdef USE_QEMU
    SendToAndroid(RESP_VER, "QEMU_SIM_1.0"); // Example version string
#else
    SendToAndroid(RESP_VER, "HW_VER_1.0");
#endif
}
