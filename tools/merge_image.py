import os

Import("env")


def merge_bin(target, source, env):
    """
    Build merged_image.bin from the ESP32 flash images.

    target:
        [merged_image.bin]

    source:
        [firmware.bin]

    env:
        PlatformIO/SCons construction environment
    """

    board_config = env.BoardConfig()

    app_bin = source[0].get_abspath()
    merged_image = target[0].get_abspath()

    # The list contains all extra images:
    # bootloader, partitions, eboot, etc.,
    # plus the final application binary.
    flash_images = (
        env.Flatten(env.get("FLASH_EXTRA_IMAGES", []))
        + [
            "${ESP32_APP_OFFSET}",
            app_bin,
        ]
    )

    # Figure out flash frequency and mode.
    flash_freq = board_config.get("build.f_flash", "40000000L")
    flash_freq = str(flash_freq).replace("L", "")
    flash_freq = str(int(int(flash_freq) / 1000000)) + "m"

    flash_mode = board_config.get("build.flash_mode", "dio")
    memory_type = board_config.get("build.arduino.memory_type", "qio_qspi")

    if flash_mode == "qio" or flash_mode == "qout":
        flash_mode = "dio"

    if memory_type == "opi_opi" or memory_type == "opi_qspi":
        flash_mode = "dout"

    print(f"Creating merged image: {merged_image}")

    # Run esptool to merge images into a single binary.
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


build_dir = env.subst("$BUILD_DIR")
progname = env.subst("$PROGNAME")

app_bin = os.path.join(build_dir, f"{progname}.bin")
merged_image = os.path.join(build_dir, env.GetProjectOption("custom_merged_image", "merged_image.bin"))

merged_target = env.Command(
    target=merged_image,
    source=app_bin,
    action=merge_bin,
)

# Make `pio run` build merged_image.bin as part of the normal build.
env.Depends("buildprog", merged_target)

# Make the target/path available to later extra scripts.
env.Replace(
    MERGED_IMAGE=merged_image,
    MERGED_IMAGE_TARGET=merged_target,
)