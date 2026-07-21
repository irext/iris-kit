"""
PlatformIO post-build script to rename firmware binary files
based on the custom firmware_rename environment variable.
"""

import os
import shutil
from SCons.Script import Import

Import("env")

# Get the firmware rename target from environment variable or build_flags
# Check if FIRMWARE_RENAME is defined in build_flags
build_flags = env.get('BUILD_FLAGS', [])
firmware_rename = None

# Parse build_flags to find -DFIRMWARE_RENAME=value
if isinstance(build_flags, list):
    for flag in build_flags:
        if flag.startswith('-DFIRMWARE_RENAME='):
            firmware_rename = flag.split('=')[1]
            break
elif isinstance(build_flags, str):
    for flag in build_flags.split():
        if flag.startswith('-DFIRMWARE_RENAME='):
            firmware_rename = flag.split('=')[1]
            break

if firmware_rename:
    # Get the build directory and firmware path
    build_dir = env.subst("$BUILD_DIR")
    firmware_src = os.path.join(build_dir, "firmware.bin")
    
    # Define the new firmware name
    firmware_dst = os.path.join(build_dir, f"{firmware_rename}.bin")
    
    # Rename the firmware file if it exists
    if os.path.exists(firmware_src):
        print(f"Renaming firmware: {firmware_src} -> {firmware_dst}")
        shutil.copy2(firmware_src, firmware_dst)
        print(f"Firmware renamed successfully: {firmware_rename}.bin")
    else:
        print(f"Warning: firmware.bin not found at {firmware_src}")
else:
    print("No firmware_rename specified, keeping default name")
