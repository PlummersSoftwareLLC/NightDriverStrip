Import("env")

import os
import shutil

def copy_firmware_to_webinstaller(target, source, env):
    env_name = env["PIOENV"]

    src = target[0].get_abspath()

    if not os.path.isfile(src):
        print(f"copy_installer_firmware: {src} not found, skipping.")
        return

    dst_dir = os.path.join("WebInstaller", "firmware", env_name)
    dst = os.path.join(dst_dir, os.path.basename(src))

    os.makedirs(dst_dir, exist_ok=True)
    shutil.copy2(src, dst)

    print(f"copy_installer_firmware: {src} -> {dst}")


merged_target = env.get("MERGED_IMAGE_TARGET")

if not merged_target:
    raise RuntimeError(
        "MERGED_IMAGE_TARGET is not defined. "
        "Make sure merge_image.py appears before "
        "copy_firmware_to_webinstaller.py in extra_scripts."
    )

env.AddPostAction(merged_target, copy_firmware_to_webinstaller)