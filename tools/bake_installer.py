#!/usr/bin/env python

import os
import glob
import json
import shutil
import subprocess

class Dirs:
    config = 'config'
    webinstaller = 'WebInstaller'
    firmware = 'firmware'
    manifests = 'manifests'
    include = 'include'
    assets = 'assets'

class Manifest:
    base = 'manifest_'
    ext = '.json'
    unmerged_template = 'manifest_template.json'
    merged_template = 'manifest_template_merged.json'

webprojects_file = 'web_projects.json'
merged_image = 'merged_image.bin'
globals_h = 'globals.h'

# Do some ground work to set up the web installer directory, starting with the project config...
webprojects_path = os.path.join(Dirs.config, webprojects_file)
shutil.copy(webprojects_path, Dirs.webinstaller)

# ...then the installer image assets...
assets_target_dir = os.path.join(Dirs.webinstaller, Dirs.assets)
if not os.path.exists(assets_target_dir):
    os.makedirs(assets_target_dir)
shutil.copy(os.path.join(Dirs.assets, 'favicon.ico'), assets_target_dir)
shutil.copy(os.path.join(Dirs.assets, 'NightDriverLogo-small.png'), assets_target_dir)

# ...then the firmware and manifest directories
firmware_target_root = os.path.join(Dirs.webinstaller, Dirs.firmware)
if not os.path.exists(firmware_target_root):
    os.makedirs(firmware_target_root)

manifest_target_dir = os.path.join(Dirs.webinstaller, Dirs.manifests)
if not os.path.exists(manifest_target_dir):
    os.makedirs(manifest_target_dir)

# Load template for unmerged and merged firmware images
with open(os.path.join(Dirs.config, Manifest.unmerged_template), "r", encoding='utf-8') as f:
    unmerged_template = f.read()

with open(os.path.join(Dirs.config, Manifest.merged_template), "r", encoding='utf-8') as f:
    merged_template = f.read()

# Extract the version from globals.h
version = ''
with open(os.path.join(Dirs.include, globals_h), "r", encoding='utf-8') as f:
    while (line := f.readline()):
        if '#define FLASH_VERSION' in line:
            version = line.split()[2]
            break

# Create a neat 3-digit version number
version = '0' * (3 - len(version)) + version

# Now read the device and project config
with open(webprojects_path, "r", encoding='utf-8') as f:
    json_data = json.load(f)
    devices = json_data['devices']

# Find out how many projects we're going to build
projectCount = 0

for device in devices:
    projectCount += len(device['projects'])

currentProject = 0

# Start building images!
for device in devices:
    device_name = device['name']
    chip_family = device['chipfamily']

    # If the merge firmware flag was set at the device level then pick it up, otherwise default to true
    if 'merge' in device:
        device_merge_firmware = device['merge']
    else:
        device_merge_firmware = True

    for project in device['projects']:
        tag = project['tag']

        currentProject += 1

        print('=' * 50)
        print('=== Building Web Installer project ' + tag + ' (' + currentProject + ' of ' + projectCount + ')')
        print('=' * 50)

        # Build the firmware and the merged image
        subprocess.run(['pio', 'run', '-e', tag])

        # If the merge firmware flag was set at the project level then pick it up, otherwise default to the device flag
        if 'merge' in project:
            merge_firmware = project['merge']
        else:
            merge_firmware = device_merge_firmware

        firmware_target_dir = os.path.join(firmware_target_root, tag)

        if not os.path.exists(firmware_target_dir):
            os.makedirs(firmware_target_dir)

        build_dir = os.path.join('.pio', 'build', tag)

        if merge_firmware:
            # Copy only the merged image from the build directory
            shutil.copy(os.path.join(build_dir, merged_image), firmware_target_dir)

            template = merged_template
        else:
            # Copy all .bin files from the build directory, EXCEPT the merged one
            bin_files = glob.glob(os.path.join(build_dir, '*.bin'))

            for bin_file in bin_files:
                if not bin_file.endswith(merged_image):
                    shutil.copy(bin_file, firmware_target_dir)

            template = unmerged_template

        manifest = template.replace('<name>', project['name'] + ' for ' + device_name)
        manifest = manifest.replace('<version>', version)
        manifest = manifest.replace('<chipfamily>', chip_family)
        manifest = manifest.replace('<tag>', tag)

        with open(os.path.join(manifest_target_dir, Manifest.base + tag + Manifest.ext), 'w', encoding='utf-8') as f:
            f.write(manifest)

        shutil.rmtree(build_dir)
