#--------------------------------------------------------------------------
#
# File:        merge_image.py
#
# NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights Reserved.
#
# This file is part of the NightDriver software project.
#
#    NightDriver is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    NightDriver is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Nightdriver.  It is normally found in copying.txt
#    If not, see <https://www.gnu.org/licenses/>.
#
# Description:
#
#    PlatformIO post-build script to create a merged firmware file
#
# History:     Aug-08-2023         Rbergen      Added header
#
#---------------------------------------------------------------------------

import string, os

Import("env")

def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["${ESP32_APP_OFFSET}", target[0].get_abspath()]
    board_config = env.BoardConfig()
    merged_image = os.path.join("${BUILD_DIR}", "merged_image.bin")

    # Figure out flash frequency and mode
    flash_freq = board_config.get("build.f_flash", "40000000L")
    flash_freq = str(flash_freq).replace("L", "")
    flash_freq = str(int(int(flash_freq) / 1000000)) + "m"
    flash_mode = board_config.get("build.flash_mode", "dio")
    memory_type = board_config.get("build.arduino.memory_type", "qio_qspi")

    if flash_mode == "qio" or flash_mode == "qout":
        flash_mode = "dio"
    if memory_type == "opi_opi" or memory_type == "opi_qspi":
        flash_mode = "dout"

    # Run esptool to merge images into a single binary
    env.Execute(
        " ".join(
            [
                "${PYTHONEXE}",
                "${OBJCOPY}",
                "--chip",
                board_config.get("build.mcu", "esp32"),
                "merge_bin",
                "--flash_size",
                board_config.get("upload.flash_size", "4MB"),
                "--flash_mode",
                flash_mode,
                "--flash_freq",
                flash_freq,
                "-o",
                merged_image,
            ]
            + flash_images
        )
    )

# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction("${BUILD_DIR}/${PROGNAME}.bin", merge_bin)
