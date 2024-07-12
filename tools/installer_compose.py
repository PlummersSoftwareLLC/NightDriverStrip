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
#    Compose the Web Installer from previously built firmware and manifests
#
# History:     Jul-12-2024         Rbergen      Created
#
#---------------------------------------------------------------------------

import glob
import json
import os
import sys
import shutil
import installer_vars

def compose_installer(release_name: str):

    print('=== Setting up base directory and file structure for web installer')

    # Do some ground work to set up the web installer directory, starting with the installer image assets...
    assets_target_dir = os.path.join(installer_vars.Dirs.webinstaller, installer_vars.Dirs.assets)
    if not os.path.exists(assets_target_dir):
        os.makedirs(assets_target_dir)
    shutil.copy(os.path.join(installer_vars.Dirs.assets, 'favicon.ico'), assets_target_dir)
    shutil.copy(os.path.join(installer_vars.Dirs.assets, 'NightDriverLogo-small.png'), assets_target_dir)

    # ...then the firmware and manifest directories
    firmware_target_root = os.path.join(installer_vars.Dirs.webinstaller, installer_vars.Dirs.firmware)
    if not os.path.exists(firmware_target_root):
        os.makedirs(firmware_target_root)

    manifest_target_dir = os.path.join(installer_vars.Dirs.webinstaller, installer_vars.Dirs.manifests)
    if not os.path.exists(manifest_target_dir):
        os.makedirs(manifest_target_dir)

    with open(os.path.join(installer_vars.Dirs.config, installer_vars.Files.feature_flags), "r", encoding='utf-8') as f:
        known_features = json.load(f)

    # Now read the device and project config
    with open(os.path.join(installer_vars.Dirs.config, installer_vars.Files.webprojects), "r", encoding='utf-8') as f:
        webprojects = json.load(f)
        devices = webprojects['devices']

    used_features = []

    # Process output for each project
    for device in devices:
        for project in device['projects']:
            tag = project['tag']
            project_dir = os.path.join(installer_vars.Dirs.build, tag)

            print('===')
            print('=' * 79)
            print('=== Processing Web Installer project ' + tag)
            print('=' * 79)
            print('===', flush = True)

            firmware_target_dir = os.path.join(firmware_target_root, tag)
            if not os.path.exists(firmware_target_dir):
                os.makedirs(firmware_target_dir)

            print('=== Copying firmware file(s)')

            firmware_files = glob.glob(os.path.join(project_dir, installer_vars.Dirs.firmware, '*'))
            for firmware_file in firmware_files:
                shutil.copy(firmware_file, firmware_target_dir)

            print('=== Copying manifest file')

            shutil.copy(os.path.join(project_dir, installer_vars.Files.manifest), os.path.join(manifest_target_dir, installer_vars.Manifest.base + tag + installer_vars.Manifest.ext))

            print('=== Processing feature flags')

            feature_letters = []

            with open(os.path.join(project_dir, installer_vars.Files.features), "r", encoding='utf-8') as f:
                for feature_tag in json.load(f):
                    feature = known_features[feature_tag]
                    # Only report features we should
                    if feature['show']:
                        feature_letters.append(feature['letter'])
                        if feature_tag not in used_features:
                            used_features.append(feature_tag)

            if len(feature_letters) > 0:
                project['name'] = project['name'] + ' (' + ','.join(feature_letters) + ')'

    print('===')
    print('=' * 79)
    print('===')

    print('=== Writing web projects JSON file')
    with open(os.path.join(installer_vars.Dirs.webinstaller, installer_vars.Files.webprojects), 'w', encoding='utf-8') as f:
        json.dump(webprojects, fp=f)

    print('=== Writing index.html')
    legend_entries = []
    for feature_tag, feature in known_features.items():
        if feature_tag in used_features:
            legend_entries.append(feature['letter'] + ': ' + feature['label'])

    # Load template for index.html...
    with open(os.path.join(installer_vars.Dirs.config, installer_vars.Files.index_template), "r", encoding='utf-8') as f:
        index_template = f.read()

    # ...then inject release name...
    index_template = index_template.replace('$$RELEASE_NAME$$', release_name)

    # ...and write it with the feature legend injected
    with open(os.path.join(installer_vars.Dirs.webinstaller, installer_vars.Files.index), 'w', encoding='utf-8') as f:
        f.write(index_template.replace('$$FEATURE_LEGEND$$', ', '.join(legend_entries)))

if __name__ == '__main__':
    compose_installer(sys.argv[1] if len(sys.argv) > 1 else 'unnamed')