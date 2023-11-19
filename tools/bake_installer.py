#!/usr/bin/env python

#--------------------------------------------------------------------------
#
# File:        bake_installer.py
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
#    Script to bake the Web Installer, including firmware and manifests
#
# History:     Aug-08-2023         Rbergen      Added header
#
#---------------------------------------------------------------------------

import os
import glob
import json
import shutil
import subprocess
import show_features

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
feature_flags_file = 'feature_flags.json'
merged_image = 'merged_image.bin'
globals_h = 'globals.h'
index_template_file = 'installer_index.html'
index_file = 'index.html'

# Do some ground work to set up the web installer directory, starting with the installer image assets...
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

with open(os.path.join(Dirs.config, feature_flags_file), "r", encoding='utf-8') as f:
    known_features = json.load(f)

# Now read the device and project config
with open(os.path.join(Dirs.config, webprojects_file), "r", encoding='utf-8') as f:
    webprojects = json.load(f)
    devices = webprojects['devices']

# Find out how many projects we're going to build
projectCount = 0

for device in devices:
    projectCount += len(device['projects'])

currentProject = 0
used_features = []

# Start building images!
for device in devices:
    device_name = device['name']
    chip_family = device['chipfamily']

    # If the merge firmware flag was set at the device level then pick it up, otherwise default to true
    device_merge_firmware = device['merge'] if 'merge' in device else True

    for project in device['projects']:
        tag = project['tag']

        currentProject += 1

        print('===')
        print('=' * 79)
        print('=== Building Web Installer project ' + tag + ' (' + str(currentProject) + ' of ' + str(projectCount) + ')')
        print('=' * 79)
        print('===', flush = True)

        # Build the firmware and the merged image
        subprocess.run(['pio', 'run', '-e', tag])

        # If the merge firmware flag was set at the project level then pick it up, otherwise default to the device flag
        merge_firmware = project['merge'] if 'merge' in project else device_merge_firmware

        firmware_target_dir = os.path.join(firmware_target_root, tag)

        if not os.path.exists(firmware_target_dir):
            os.makedirs(firmware_target_dir)

        build_dir = os.path.join('.pio', 'build', tag)

        if merge_firmware:
            image_source_path = os.path.join(build_dir, merged_image)
            # If the merged image doesn't exist that means the build was effectively executed from cache. Which
            # means in turn that the merged image should already be in its target location.
            if (os.path.exists(image_source_path)):
                print('=== Copying merged firmware file ' + merged_image)
                shutil.copy(image_source_path, firmware_target_dir)

            template = merged_template
        else:
            # Copy all .bin files from the build directory, EXCEPT the merged one
            bin_files = glob.glob(os.path.join(build_dir, '*.bin'))

            for bin_file in bin_files:
                if not bin_file.endswith(merged_image):
                    print('=== Copying binary file ' + bin_file)
                    shutil.copy(bin_file, firmware_target_dir)

            template = unmerged_template

        manifest_file = Manifest.base + tag + Manifest.ext
        print('=== Writing manifest file ' + manifest_file)
        manifest = template.replace('<name>', project['name'] + ' for ' + device_name)
        manifest = manifest.replace('<version>', version)
        manifest = manifest.replace('<chipfamily>', chip_family)
        manifest = manifest.replace('<tag>', tag)

        with open(os.path.join(manifest_target_dir, manifest_file), 'w', encoding='utf-8') as f:
            f.write(manifest)

        print('=== Computing feature flags')
        feature_letters = []
        for feature_tag in show_features.get_features(tag):
            feature = known_features[feature_tag]
            # Only report features we should
            if feature['show']:
                feature_letters.append(feature['letter'])
                if feature_tag not in used_features:
                    used_features.append(feature_tag)

        if len(feature_letters) > 0:
            project['name'] = project['name'] + ' (' + ','.join(feature_letters) + ')'

        print('=== Removing build directory ' + build_dir, flush = True)
        shutil.rmtree(build_dir)

print('===')
print('=' * 79)
print('===')

print('=== Writing web projects JSON file')
with open(os.path.join(Dirs.webinstaller, webprojects_file), 'w', encoding='utf-8') as f:
    json.dump(webprojects, fp=f)

print('=== Writing index.html')
legend_entries = []
for feature_tag, feature in known_features.items():
    if feature_tag in used_features:
        legend_entries.append(feature['letter'] + ': ' + feature['label'])

# Load template for index.html...
with open(os.path.join(Dirs.config, index_template_file), "r", encoding='utf-8') as f:
    index_template = f.read()

# ...and write it with the feature legend injected
with open(os.path.join(Dirs.webinstaller, index_file), 'w', encoding='utf-8') as f:
    f.write(index_template.replace('$$FEATURE_LEGEND$$', ', '.join(legend_entries)))