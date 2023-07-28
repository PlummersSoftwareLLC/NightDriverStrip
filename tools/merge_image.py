import string, os

Import("env")

def merge_bin(source, target, env):
    # The list contains all extra images (bootloader, partitions, eboot) and
    # the final application binary
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["${ESP32_APP_OFFSET}", target[0].get_abspath()]
    board_config = env.BoardConfig()

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
                "${BUILD_DIR}/${PROGNAME}_merged.bin",
            ]
            + flash_images
        )
    )

# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction("${BUILD_DIR}/${PROGNAME}.bin", merge_bin)

# Patch the upload command to flash the merged binary at address 0x0
env.Replace(
    UPLOADERFLAGS=[
        ]
        + ["0x0", MERGED_BIN],
    UPLOADCMD='"$PYTHONEXE" "$UPLOADER" $UPLOADERFLAGS',
)