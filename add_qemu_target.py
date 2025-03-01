Import("env")

# Create a custom target to run QEMU
def qemu_target(env):
    qemu_command = [
        "qemu-system-arm",
        "-machine", "netduinoplus2",
        "-cpu", "cortex-m3",
        "-gdb", "tcp::1234",
        "-nographic",
        "-kernel",
        "$BUILD_DIR/${PROGNAME}.elf",
        "-serial", "stdio",
        "-semihosting-config", "enable=on,target=native"
    ]

    env.AddCustomTarget(
        "qemu",
        None,
        qemu_command,
        title="Run in QEMU",
        description="Launch QEMU emulator",
    )

# Add the custom target to the current environment
qemu_target(env)