# STM32F103 CANBox for Peugeot 407 and Android Head Unit (12V Output Signals)

This project implements a CAN bus interface (CANBox) using an STM32F103 microcontroller ("Blue Pill") to connect a Peugeot 407's CAN bus to an Android head unit. It performs two main functions:

1.  **CAN Bus Communication:** Reads CAN messages from the car (e.g., steering wheel controls) and translates them into a serial protocol for the Android head unit.
2.  **Signal Generation:** *Generates* 12V output signals (IGN, ILLUM, REAR) and a GND-level output (PARK) to control the Android head unit's power, backlight, parking brake status, and reverse camera input.

The project is inspired by and uses the communication protocol described in the [fazerxlo/canbox](https://github.com/fazerxlo/canbox) project, specifically the [DRIVE2.md](https://github.com/fazerxlo/canbox/blob/doc/doc/sources/DRIVE2.md) documentation.

## Features

*   **Steering Wheel Control Integration:** Reads steering wheel button presses (Source, Volume Up/Down, Seek Forward/Backward, OK, End Call) from the car's CAN bus (ID `0x165`) and sends corresponding commands to the Android head unit.
*   **Ignition (IGN/ACC) Output:** Generates a 12V signal to control the Android head unit's power (on/off).
*   **Illumination (ILLUM) Output:** Generates a 12V signal to control the Android head unit's backlight.
*   **Parking Brake (PARK) Output:** Generates a GND-level signal to indicate the parking brake status.
*   **Reverse Gear (REAR) Output:** Generates a 12V signal to switch the Android head unit to the rear-view camera input.  Also sends a CAN message to control the power to the rear-view camera (CAN ID needs to be determined and may not be required for all setups).
*   **Communication Protocol:** Uses a simple serial protocol over UART (38400 baud) to communicate with the Android head unit: `!<command>:<value>\n` (from Android) and `!<response>:<value>\n` (from CANBox).
*   **Command Handling:** Can receive and process commands from the Android head unit (e.g., simulate button presses, reset).
*   **Interrupt-Driven UART:** Uses interrupts for efficient UART reception.
*   **PlatformIO Based:** Developed using PlatformIO.

## Hardware Requirements

*   **STM32F103C8T6 "Blue Pill" Board:**
*   **CAN Transceiver:** MCP2551 (5V, may need level shifter on TX), SN65HVD230 (3.3V), TJA1050 (5V), or ISO1050 (isolated).
*   **USB-to-UART Adapter:** To connect the STM32's UART1 (PA9/PA10) to the Android head unit.
*   **N-Channel MOSFETs (x3):** Logic-level, for switching the 12V IGN, ILLUM, and REAR signals (e.g., 2N7000/2N7002, BS170, IRLZ44N).
*   **P-Channel MOSFET (x1):** For switching the GND PARK signal (e.g., IRF9Z34N, SI2301CDS).
*   **Resistors:**
    *   Gate resistors (1kΩ - 10kΩ) for the MOSFETs.
    *    Pull-Up resistor 10kOhm for P-Channel MOSFET.
    *   Pull-Down resistors (10kΩ, optional but recommended) for the N-Channel MOSFET gates.
*   **Automotive-grade DC-DC Buck Converter:** To step down the car's 12V to 3.3V or 5V.
*   **Fuse:** (e.g., 2A) in series with the 12V input to the DC-DC converter.
*   **Wiring Harness/Connectors:**
*   **Multimeter:**
*   **CAN Bus Sniffer (Optional but Highly Recommended):**

## Software Requirements

*   **PlatformIO IDE:** (VS Code extension recommended).
*   **STM32Cube HAL:** (Automatically managed by PlatformIO).
*   **Android Head Unit App (Not Included):**  You'll need to develop an Android application to communicate with the CANBox.

## Project Structure

```
canbox_project/
├── include/       <- (Optional) Your custom header files
├── lib/           <- (Optional) Your custom libraries
├── src/
│   └── main.c     <- Main source code file
├── platformio.ini <- PlatformIO configuration file
├── .pio/          <- PlatformIO's internal build directory (do not modify)
└── .vscode/       <- VS Code settings (if using VS Code)
└── README.md      <- This file
└── hardware.md    <- Hardware details.
```

## `platformio.ini` Configuration

```ini
[env:bluepill_f103c8]
platform = ststm32
board = bluepill_f103c8
framework = stm32cube
upload_protocol = stlink
debug_tool = stlink

build_flags =
    -D HSE_VALUE=8000000
    -D SYSCLK_FREQ_72MHz
    -D HAL_CAN_MODULE_ENABLED

; monitor_speed not used, UART communication used.
```

## `main.c` Overview

*   **Includes:** Header files for STM32 HAL, CAN, UART, stdio, string, and stdbool.
*   **Defines:** Constants for baud rates, CAN ID, button codes, Android commands, CANBox responses, and GPIO pins for output signals.
*   **Global Variables:** CAN and UART handles, UART RX buffer, status flags, and output signal states.
*   **`main()`:** Initializes HAL, system clock, CAN, UART, GPIO, sends an initialization message, and enters an infinite loop that checks for incoming data and updates output signals.
*   **`SystemClock_Config()`:** Configures the system clock (typically 72MHz).
*   **`CAN_Init()`:** Initializes the CAN peripheral (125kbps), configures GPIO, sets up a filter for steering wheel controls (`0x165`), and enables the RX interrupt.
*   **`UART_Init()`:** Initializes UART1 (38400 baud), configures GPIO, and enables the RX interrupt.
*   **`GPIO_Status_Init()`:** Initializes the GPIO pins for the *output* signals (IGN, ILLUM, PARK, REAR) as outputs.
*   **`CAN_Transmit()`:** Sends a CAN message.
*   **`HAL_CAN_RxFifo0MsgPendingCallback()`:** CAN RX interrupt handler. Calls `ProcessCanMessage()`.
*   **`ProcessCanMessage()`:** Decodes CAN messages (currently handles steering wheel controls).
*   **`SendToAndroid()`:** Formats and sends a message to the Android head unit over UART.
*   **`ProcessAndroidCommand()`:** Parses and handles commands from the Android head unit.
*   **`ReceiveFromAndroid()`:** Checks for incoming UART data and calls `ProcessAndroidCommand()`.
*   **`USART1_IRQHandler()`:** UART RX interrupt handler.  Receives bytes, stores them in a buffer (needs a circular buffer implementation!), and sets a flag.
*   **`CheckStatusSignals()`:**  This function is modified to *set* the output pins based on internal state variables, rather than reading inputs.  The logic for determining *when* to change these states (e.g., based on CAN messages or commands from Android) needs to be implemented.
*   **`Error_Handler()`:** Basic error handler.

## Building and Uploading

1.  **Install PlatformIO:**
2.  **Open Project:**
3.  **Connect Hardware:** Connect the STM32 via ST-Link.
4.  **Build:** Click "Build" (checkmark).
5.  **Upload:** Click "Upload" (right-arrow).

## Wiring (Example - Verify!)

| STM32 Pin | Connection                                                                                               | Notes                                                                                                                                                                                                                           |
| --------- | -------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| PA9       | USB-to-UART Adapter RX                                                                                    | UART1 TX (to Android)                                                                                                                                                                                                           |
| PA10      | USB-to-UART Adapter TX                                                                                    | UART1 RX (from Android)                                                                                                                                                                                                          |
| PA11      | CAN Transceiver RX                                                                                        | CAN1 RX                                                                                                                                                                                                                          |
| PA12      | CAN Transceiver TX                                                                                        | CAN1 TX                                                                                                                                                                                                                          |
| PB1       | N-Channel MOSFET Gate (IGN/ACC)                                                                           | Output to control 12V IGN/ACC signal.  MOSFET Drain to head unit input, Source to GND.                                                                                                                                           |
| PB2       | N-Channel MOSFET Gate (ILLUM)                                                                             | Output to control 12V ILLUM signal. MOSFET Drain to head unit input, Source to GND.                                                                                                                                            |
| PB3       | P-Channel MOSFET Gate (PARK)                                                                              | Output to control GND PARK signal. MOSFET Drain to head unit input, Source to GND.                                                                           |
| PB4       | N-Channel MOSFET Gate (REAR)                                                                              | Output to control 12V REAR signal. MOSFET Drain to head unit input, Source to GND.  Also, send a CAN message to potentially control rear camera power (if needed - CAN ID and data must be determined).                             |
| 3.3V      | DC-DC Converter Output (+), CAN Transceiver (if 3.3V), pull-up resistor for P-channel MOSFET. | Positive supply.                                                                                                                                                                                                                  |
| GND       | DC-DC Converter Output (-), Car Chassis Ground, CAN Transceiver, MOSFET Sources, USB-to-UART Adapter     | Common ground.                                                                                                                                                                                                                  |
| CANH      | CAN Transceiver CANH                                                                                      | To car's CAN High.                                                                                                                                                                                                              |
| CANL      | CAN Transceiver CANL                                                                                      | To car's CAN Low.                                                                                                                                                                                                               |
| 12V (Car) | Car Battery (or Accessory Line) -> Fuse -> DC-DC Converter Input;  Also to relay COM (if used)       |  Use a fuse.  Preferably use an accessory line that's switched with the ignition.  Also provides 12V for the MOSFETs to switch to the head unit.                                                                                     |

**Important:**

*   **MOSFETs:** Use logic-level N-channel MOSFETs for IGN, ILLUM, and REAR.  Use a P-channel MOSFET for PARK.  Include gate resistors.
*   **CAN Bus Connection:** Correctly identify CANH and CANL.
* **DC-DC Converter**: Use a robust automotive grade DC-DC converter

## Reverse Engineering

You'll likely need to reverse-engineer additional CAN IDs and data (e.g., for climate control, rear camera power).  Use a CAN bus sniffer and analyze the captured data.

## TODOs

*   **Circular Buffer:** Implement a circular buffer for UART RX.
*   **Add More CAN Handling:**
*   **Add More Commands:**
*   **Debouncing:** (Less critical for outputs, but still good practice).
*   **Error Handling:**
*   **Power Management:**
*   **Configuration:**
* **Android Application**
* **Complete Logic for Output Signals:** The `CheckStatusSignals()` function currently just sets the output pins.  You need to add the logic to determine *when* to change the signals (e.g., turn on IGN when a specific CAN message is received, or when a command is received from Android).
* **Rear Camera power control via CAN:** You need to find the correct CAN ID and data.
*   **Testing:**

## Safety

*   **Do not interfere with safety-critical systems.**
*   **Start with read-only CAN access.**
*   **Test in a safe environment.**
*   **Use a fuse.**

This revised README accurately reflects the changes required to *generate* the 12V output signals and provides a clear and comprehensive overview of the project. The most important changes are the hardware modifications (using MOSFETs as switches) and the corresponding changes to the `CheckStatusSignals()` function in the code (which now needs to *set* the output pins based on the desired logic, rather than reading input pins).
