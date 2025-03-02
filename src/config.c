#include "config.h"
#include "commands.h" // For SendToAndroid
#include <string.h>  // Added for strcmp
#include <stdio.h>     // For sscanf

// --- Configuration Variables (Defaults) ---
uint32_t config_ign_src = IGNITION_STATUS_ID;
uint32_t config_illum_src = DASHBOARD_LIGHTS_ID;
uint32_t config_rev_src = REVERSE_GEAR_ID;
uint32_t config_park_src = DASHBOARD_LIGHTS_ID;
uint32_t config_door_src = DOOR_STATUS_ID;


void load_config(void) {
    // No flash loading needed; defaults are already set.
}

void save_config(void) {
    // No flash saving needed; changes are already in RAM.
}

void ProcessConfigCommand(const char *parameter, const char *value) {
    if (strcmp(parameter, "GET") == 0) {
        if (strcmp(value, "ign_src") == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "ign_src:%#x", (unsigned int)config_ign_src);
            SendToAndroid(RESP_CFG, buf);
        } else if (strcmp(value, "illum_src") == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "illum_src:%#x", (unsigned int)config_illum_src);
            SendToAndroid(RESP_CFG, buf);
        } else if (strcmp(value, "rev_src") == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "rev_src:%#x", (unsigned int)config_rev_src);
            SendToAndroid(RESP_CFG, buf);
        } else if (strcmp(value, "park_src") == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "park_src:%#x", (unsigned int)config_park_src);
            SendToAndroid(RESP_CFG, buf);
        } else if (strcmp(value, "door_src") == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "door_src:%#x", (unsigned int)config_door_src);
            SendToAndroid(RESP_CFG, buf);
        } else {
            SendToAndroid(RESP_ERR, ERR_INVALID_CONFIG);
        }
    } else if (strcmp(parameter, "SET") == 0) {
        unsigned int new_value;
        if (sscanf(value, "%x", &new_value) == 1) {
            if (strcmp(value, "ign_src") == 0) {
                config_ign_src = (uint32_t)new_value;
            } else if (strcmp(value, "illum_src") == 0) {
                config_illum_src = (uint32_t)new_value;
            } else if (strcmp(value, "rev_src") == 0) {
                config_rev_src = (uint32_t)new_value;
            } else if (strcmp(value, "park_src") == 0) {
                config_park_src = (uint32_t)new_value;
            } else if (strcmp(value, "door_src") == 0) {
                config_door_src = (uint32_t)new_value;
            } else {
                SendToAndroid(RESP_ERR, ERR_INVALID_CONFIG);
            }
        } else {
            SendToAndroid(RESP_ERR, ERR_INVALID_CONFIG); // Invalid value format
        }
    } else {
        SendToAndroid(RESP_ERR, ERR_INVALID_CONFIG); // Invalid parameter (not GET or SET)
    }
}
