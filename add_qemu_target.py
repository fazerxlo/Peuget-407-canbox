Import("env")

# Create a custom target to run QEMU
def qemu_target(env):
    # Determine the serial port.  Modify this as needed!
    # Linux example:
    serial_port = "/dev/ttyACM0"  # Or /dev/ttyUSB0, etc. - CHECK YOUR SYSTEM!
    # Windows example (untested, adjust as necessary):
    # serial_port = "COM3"

    qemu_command = [
        "qemu-system-arm",
        "-machine", "netduinoplus2",
        "-cpu", "cortex-m3",
        "-gdb", "tcp::1234",
        "-nographic",
        "-kernel", "$BUILD_DIR/${PROGNAME}.elf",
        "-serial", f"chardev:serdev,path={serial_port},id=serdev", # Redirect to a REAL serial port
        "-monitor", "none",
         # No semihosting in this configuration
    ]

    env.AddCustomTarget(
        "qemu",
        None,
        qemu_command,
        title="Run in QEMU",
        description="Launch QEMU emulator",
        group="Platform"
    )

# Add the custom target to the current environment
qemu_target(env)