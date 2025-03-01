#ifndef CONFIG_H
#define CONFIG_H

#include "main.h"

// --- CAN IDs (Configurable - Defaults) ---
// Default values, can be overridden by loading from flash
#define STEERING_WHEEL_CONTROLS_ID 0x165
#define IGNITION_STATUS_ID         0x036
#define DASHBOARD_LIGHTS_ID        0x128
#define REVERSE_GEAR_ID            0x0F6
#define DOOR_STATUS_ID             0x220

// Function prototypes for config
void load_config(void);
void save_config(void);
void ProcessConfigCommand(const char *parameter, const char *value);

#endif // CONFIG_H
