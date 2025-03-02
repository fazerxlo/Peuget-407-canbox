Import("env")
import subprocess

def qemu_target(env):

    print("test")
    qemu_command = []  # Initialize as an empty list

    # VERY IMPORTANT: We *don't* get debug_extra_args here.  We use debug_server.
    # Get the debug_server command, handling potential missing definition.
    debug_server_command = env.GetProjectOption("debug_server", [])

    if isinstance(debug_server_command, str):
        #If provided as a single command.
        import shlex
        qemu_command.extend(shlex.split(debug_server_command))
    elif isinstance(debug_server_command, list):
        #If already provided as list
        qemu_command.extend(debug_server_command)
    else:
        #Error
        print("Error: debug_server in platformio.ini must be set as list or as string")
        return


    # Add the custom target.
    env.AddCustomTarget(
        "qemu",
        None,
        qemu_command,  # Pass the command list directly
        title="Run in QEMU",
        description="Launch QEMU emulator",
    )

# Check QEMU version (optional, but good for diagnostics)
try:
    result = subprocess.run(["qemu-system-arm", "--version"], capture_output=True, text=True, check=True)
    print(result.stdout)  # Print QEMU version information
except FileNotFoundError:
    print("Error: qemu-system-arm not found.  Ensure it's installed and in your PATH.")
except subprocess.CalledProcessError as e:
    print(f"Error checking QEMU version: {e}")

qemu_target(env) # Call the function to add the target