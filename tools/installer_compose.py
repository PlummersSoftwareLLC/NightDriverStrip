#!/usr/bin/env python

#--------------------------------------------------------------------------
#
# File:        installer_compose.py
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
import argparse
import sys
import shutil
import installer_vars

GUIDED_PROJECT_TAGS = ['installer_strip', 'installer_matrix', 'installer_m5']

def project_artifacts_exist(project_dir: str) -> bool:
    return os.path.exists(os.path.join(project_dir, installer_vars.Files.manifest))

def read_project_features(project_dir: str, tag: str, allow_missing: bool):
    features_path = os.path.join(project_dir, installer_vars.Files.features)
    try:
        with open(features_path, "r", encoding='utf-8') as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError) as error:
        if allow_missing:
            print('=== Skipping feature flags for ' + tag + ' because ' + features_path + ' could not be read: ' + str(error))
            return []

        raise RuntimeError(
            'Invalid Web Installer feature artifact for ' + tag + ': ' + features_path +
            '. Rebuild it with tools/installer_buildenv.py before composing the full installer.') from error

def compose_installer(release_name: str, project_tags = None, allow_missing: bool = False):

    print('=== Setting up base directory and file structure for web installer')

    # Do some ground work to set up the web installer directory, starting with the installer image assets...
    assets_target_dir = os.path.join(installer_vars.Dirs.webinstaller, installer_vars.Dirs.assets)
    if not os.path.exists(assets_target_dir):
        os.makedirs(assets_target_dir)
    shutil.copy(os.path.join(installer_vars.Dirs.assets, 'favicon.ico'), assets_target_dir)
    shutil.copy(os.path.join(installer_vars.Dirs.assets, installer_vars.Files.logo), assets_target_dir)

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

    requested_tags = set(project_tags) if project_tags else None
    used_features = []
    missing_projects = []

    # Process output for each project
    for device in devices:
        composed_projects = []
        for project in device['projects']:
            tag = project['tag']
            if requested_tags is not None and tag not in requested_tags:
                continue

            project_dir = os.path.join(installer_vars.Dirs.build, tag)

            print('===')
            print('=' * 79)
            print('=== Processing Web Installer project ' + tag)
            print('=' * 79)
            print('===', flush = True)

            if not project_artifacts_exist(project_dir):
                missing_projects.append(tag)
                if allow_missing:
                    print('=== Skipping Web Installer project ' + tag + ' because ' + os.path.join(project_dir, installer_vars.Files.manifest) + ' does not exist')
                    continue

                continue

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

            for feature_tag in read_project_features(project_dir, tag, allow_missing):
                feature = known_features[feature_tag]
                # Only report features we should
                if feature['show']:
                    feature_letters.append(feature['letter'])
                    if feature_tag not in used_features:
                        used_features.append(feature_tag)

            if len(feature_letters) > 0:
                project['name'] = project['name'] + ' (' + ','.join(feature_letters) + ')'
            composed_projects.append(project)

        if requested_tags is not None or allow_missing:
            device['projects'] = composed_projects

    if len(missing_projects) > 0 and not allow_missing:
        raise FileNotFoundError(
            'Missing Web Installer build artifacts for: ' + ', '.join(missing_projects) +
            '. Build each environment with tools/installer_buildenv.py, or use --allow-missing for a local partial installer.')

    if requested_tags is not None or allow_missing:
        devices[:] = [device for device in devices if len(device['projects']) > 0]

    print('===')
    print('=' * 79)
    print('===')

    print('=== Writing web projects JSON file')
    with open(os.path.join(installer_vars.Dirs.webinstaller, installer_vars.Files.webprojects), 'w', encoding='utf-8') as f:
        json.dump(webprojects, fp=f)

    print('=== Copying installer profiles JSON file')
    shutil.copy(
        os.path.join(installer_vars.Dirs.config, installer_vars.Files.installer_profiles),
        os.path.join(installer_vars.Dirs.webinstaller, installer_vars.Files.installer_profiles))

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
    parser = argparse.ArgumentParser(description='Compose the Web Installer from previously built firmware and manifests.')
    parser.add_argument('release_name', nargs='?', default='unnamed')
    parser.add_argument(
        '--guided',
        action='store_true',
        help='Include only the guided setup starter images: installer_strip, installer_matrix, and installer_m5.')
    parser.add_argument(
        '--projects',
        help='Comma-separated environment tags to include, for example installer_strip,installer_matrix,installer_m5.')
    parser.add_argument(
        '--allow-missing',
        action='store_true',
        help='Skip projects whose build/<tag>/manifest.json does not exist. Intended for local partial installer testing.')
    args = parser.parse_args()

    if args.guided and args.projects:
        parser.error('--guided and --projects cannot be used together.')

    selected_projects = None
    if args.guided:
        selected_projects = GUIDED_PROJECT_TAGS
    elif args.projects:
        selected_projects = [tag.strip() for tag in args.projects.split(',') if tag.strip()]

    try:
        compose_installer(args.release_name, selected_projects, args.allow_missing)
    except (FileNotFoundError, RuntimeError) as error:
        print('ERROR: ' + str(error), file=sys.stderr)
        print('', file=sys.stderr)
        print('For local guided setup testing, run:', file=sys.stderr)
        print('  python3 tools/installer_compose.py local --guided', file=sys.stderr)
        print('', file=sys.stderr)
        print('If you intentionally want a partial installer from whatever artifacts exist, run:', file=sys.stderr)
        print('  python3 tools/installer_compose.py local --guided --allow-missing', file=sys.stderr)
        sys.exit(2)
