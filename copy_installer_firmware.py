Import("env")

import atexit
import os
import shutil

_env_name = env["PIOENV"]

# Stage the merged image that merge_image.py just produced inside
# .pio/build/<env>/ into WebInstaller/firmware/<env>/, where the WebInstaller
# manifest expects it.
def _copy_firmware_to_webinstaller():
    src = os.path.join(".pio", "build", _env_name, "merged_image.bin")
    if not os.path.isfile(src):
        print(f"copy_installer_firmware: {src} not found, skipping.")
        return

    dst_dir = os.path.join("WebInstaller", "firmware", _env_name)
    dst = os.path.join(dst_dir, "merged_image.bin")
    os.makedirs(dst_dir, exist_ok=True)
    shutil.copy2(src, dst)
    print(f"copy_installer_firmware: {src} -> {dst}")

atexit.register(_copy_firmware_to_webinstaller)
