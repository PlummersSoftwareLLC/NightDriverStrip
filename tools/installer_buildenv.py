#!/usr/bin/env python

#--------------------------------------------------------------------------
#
# File:        bake_vars.py
#
# NightDriverStrip - (c) 2024 Plummer's Software LLC.  All Rights Reserved.
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
#    Build one environment for inclusion in the Web Installer
#
# History:     Jul-12-2024         Rbergen      Created
#
#---------------------------------------------------------------------------

import glob
import json
import os
import shutil
import subprocess
import sys
import installer_vars
import show_features

def build_env(tag: str, version: str):
    project = installer_vars.get_project(tag)

    if project is None:
        print("Environment not found: " + tag, file=sys.stderr)
        sys.exit(1)

    # Build the firmware and the merged image
    subprocess.run(['pio', 'run', '-e', tag])

    project_dir = os.path.join(installer_vars.Dirs.build, tag)

    firmware_target_dir = os.path.join(project_dir, installer_vars.Dirs.firmware)
    if not os.path.exists(firmware_target_dir):
        os.makedirs(firmware_target_dir)

    build_dir = os.path.join('.pio', 'build', tag)

    if project['merge']:
        image_source_path = os.path.join(build_dir, installer_vars.merged_image)
        # If the merged image doesn't exist that means the build was effectively executed from cache. Which
        # means in turn that the merged image should already be in its target location.
        if (os.path.exists(image_source_path)):
            print('=== Copying merged firmware file ' + installer_vars.merged_image)
            shutil.copy(image_source_path, firmware_target_dir)

        with open(os.path.join(installer_vars.Dirs.config, installer_vars.Manifest.merged_template), "r", encoding='utf-8') as f:
            template = f.read()
    else:
        # Copy all .bin files from the build directory, EXCEPT the merged one
        bin_files = glob.glob(os.path.join(build_dir, '*.bin'))

        for bin_file in bin_files:
            if not bin_file.endswith(installer_vars.merged_image):
                print('=== Copying binary file ' + bin_file)
                shutil.copy(bin_file, firmware_target_dir)

        with open(os.path.join(installer_vars.Dirs.config, installer_vars.Manifest.unmerged_template), "r", encoding='utf-8') as f:
            template = f.read()

    print('=== Writing manifest file')
    manifest = template.replace('<name>', project['name'] + ' for ' + project['device'])
    manifest = manifest.replace('<version>', version)
    manifest = manifest.replace('<chipfamily>', project['chipfamily'])
    manifest = manifest.replace('<tag>', tag)

    with open(os.path.join(project_dir, installer_vars.Files.manifest), 'w', encoding='utf-8') as f:
        f.write(manifest)

    print('=== Computing feature flags')

    with open(os.path.join(project_dir, installer_vars.Files.features), 'w', encoding='utf-8') as f:
        f.write(json.dumps(show_features.get_features(tag)))

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: " + sys.argv[0] + " <environment> <version>", file=sys.stderr)
        sys.exit(1)

    build_env(sys.argv[1], sys.argv[2])
