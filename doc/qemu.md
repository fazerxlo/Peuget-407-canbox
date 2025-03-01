# Running and Debugging the STM32 CANBox Project with QEMU

This document describes how to run and debug the STM32 CANBox project using QEMU, a machine emulator and virtualizer. This allows for development and testing without requiring the physical STM32 hardware or connection to a car.

## Prerequisites

*   **PlatformIO IDE:** Installed and configured with the `ststm32` platform.
*   **QEMU:**  PlatformIO should automatically install the necessary QEMU packages when you build for the `qemu_stm32f103` environment. If not, you may need to install it manually (e.g., `sudo apt-get install qemu-system-arm` on Debian/Ubuntu, or use your system's package manager).
*  **Project Setup:** Ensure you have completed the setup in `platformio.ini` and modified `main.c` as described in the previous responses, including the `#ifdef QEMU_MODE` sections.

## Building for QEMU

1.  **Select the QEMU Environment:** In PlatformIO, open the PlatformIO sidebar (ant icon).  Under "Project Tasks," expand the `qemu_stm32f103` environment.
2.  **Build:** Click the "Build" task (or use the checkmark icon in the PlatformIO toolbar). This will compile the code specifically for the QEMU environment, including the `-D QEMU_MODE` build flag.  The output will be in the `.pio/build/qemu_stm32f103/` directory.

## Running in QEMU (Non-Debug Mode)

1.  **Select the QEMU Environment:** (Same as for building).
2.  **"Upload":** Click the "Upload" task (or use the right-arrow icon in the PlatformIO toolbar).  *Important:* This does *not* actually upload anything to a device.  Because we've configured a custom `upload_command` in `platformio.ini`, this will execute QEMU with the compiled `.elf` file.

    The command executed (from `platformio.ini`) is:

    ```bash
    qemu-system-arm -M netduinoplus2 -cpu cortex-m3 -kernel .pio/build/qemu_stm32f103/firmware.elf -nographic -serial stdio -netdev user,id=net0 -device can-bus,netdev=net0,canbus=can0

    ```

3.  **Terminal Output:** You should see output from your program in the PlatformIO terminal.  This is because of the `-serial stdio` option, which redirects QEMU's emulated serial port (USART1) to your terminal.  You'll see messages from `printf()` calls, including the simulated CAN messages and status updates.

4. **Interacting:** Because we've redirected standard input, you can also type commands into the terminal to interact with the emulated UART.  For example, you could send commands like `!RST:OK\n` or `!KEY:SRC\n` to simulate messages from the Android head unit.

## Debugging in QEMU

1.  **Select the QEMU Environment:** (Same as for building).
2.  **Start Debugging:** Click the "Debug" button (bug icon) in the PlatformIO toolbar, or press F5. This will:
    *   Build the project (if necessary).
    *   Start QEMU in debug mode, using the `debug_server` command specified in `platformio.ini`. This command includes `-gdb tcp::1234`, which tells QEMU to listen for a GDB connection on port 1234.
    *   Launch GDB (the GNU Debugger) and connect it to QEMU.

3.  **Debugging Interface:** The VS Code debugging interface will appear. You can now:

    *   **Set Breakpoints:** Click in the left margin of the code editor to set breakpoints.
    *   **Step Through Code:** Use the debugging controls (Step Over, Step Into, Step Out, Continue) to execute the code line by line.
    *   **Inspect Variables:** View the values of variables in the "Variables" pane.
    *   **Watch Expressions:** Add expressions to the "Watch" pane to monitor their values.
    *   **Call Stack:** See the call stack in the "Call Stack" pane.
    *   **Disassembly:** View the disassembled code.
    *   **Registers:** Inspect the CPU registers.
    * **Debug Console:** The "Debug Console" pane allows you to interact directly with GDB.  You can enter GDB commands here (e.g., `print variable_name`, `info registers`, etc.).

4.  **Simulating Input:**

    *   **UART Input:** You can type commands into the PlatformIO terminal (which is connected to QEMU's serial port) to simulate input from the Android head unit.
    *   **Simulated Signals (`simulated_ign`, etc.):** You can modify the values of the `simulated_ign`, `simulated_illum`, `simulated_park`, and `simulated_rear` variables *directly* in the debugger (in the "Variables" pane). This allows you to test how your code responds to changes in these signals. You can also modify the `simulated_can_rx_data` and `simulated_can_rx_id` variables to test the CAN message processing.
    *   **Timers:** The code uses `HAL_GetTick()` and simple delays to simulate time-based events. You can observe `HAL_GetTick()` increasing in the debugger.

5. **Stopping Debugging:** Click the "Stop" button (square icon) in the debugging toolbar. This will terminate the GDB session and stop QEMU.

## Troubleshooting

*   **QEMU Not Found:** If PlatformIO can't find `qemu-system-arm`, make sure it's installed correctly on your system.  You may need to add it to your system's `PATH`.
*   **Connection Refused (Debugging):** If GDB cannot connect to QEMU, make sure QEMU is running and listening on the correct port (1234 by default). Double-check the `debug_server` settings in `platformio.ini`.
*   **No Output:**  If you don't see any output in the terminal, ensure that you've selected the correct environment (`qemu_stm32f103`) and that the `-serial stdio` option is included in the `upload_command` in `platformio.ini`. Also, check that you have `printf` statements in your code (especially in the QEMU-specific sections).
* **Incorrect Serial Input Processing:** Make sure to send the input text with `\n` at the end.
* **CAN bus issue:** If you have CAN bus related problem, ensure that `-netdev user,id=net0 -device can-bus,netdev=net0,canbus=can0` parameters exist in `platformio.ini`.

## Advantages of Using QEMU

*   **No Hardware Required:**  You can develop and test a large portion of your code without needing the STM32 hardware or a connection to the car.
*   **Faster Development:**  The build-run-debug cycle is much faster with QEMU than with uploading to real hardware.
*   **Controlled Environment:**  You have complete control over the simulated environment (input signals, CAN messages, etc.).
*   **Debugging:**  QEMU provides excellent debugging capabilities through GDB.

This guide provides a comprehensive overview of how to run and debug your STM32 CANBox project using QEMU within PlatformIO. This setup allows for efficient and convenient development and testing without relying on physical hardware.