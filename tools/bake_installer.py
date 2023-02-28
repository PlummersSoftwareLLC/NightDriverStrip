#!/usr/bin/env python

import os
import glob
import json
import shutil

config_dir = 'config'
webinstaller_dir = 'WebInstaller'
firmware_dir = 'firmware'
manifests_dir = 'manifests'
manifest_prefix = 'manifest_'
manifest_template = 'manifest_template.json'
webprojects_file = 'web_projects.json'
include_dir = 'include'
globals_h = 'globals.h'

webprojects_path = os.path.join(config_dir, webprojects_file)

shutil.copy(webprojects_path, webinstaller_dir)

firmware_path = os.path.join(webinstaller_dir, firmware_dir)

if not os.path.exists(firmware_path):
    os.makedirs(firmware_path)

with open(webprojects_path, "r", encoding='utf-8') as f:
    json_data = json.load(f)
    devices = json_data['devices']

with open(os.path.join(config_dir, manifest_template), "r", encoding='utf-8') as f:
    manifest_text = f.read()

version = ''
with open(os.path.join(include_dir, globals_h), "r", encoding='utf-8') as f:
    while (line := f.readline()):
        if '#define FLASH_VERSION' in line:
            version = line.split()[2]
            break

version = '0' * (3 - len(version)) + version

manifest_target_path = os.path.join(webinstaller_dir, manifests_dir)
if not os.path.exists(manifest_target_path):
    os.makedirs(manifest_target_path)

for device in devices:
    device_name = device['name']

    for project in device['projects']:
        tag = project['tag']
        project_firmware_path = os.path.join(firmware_path, tag)

        if not os.path.exists(project_firmware_path):
            os.makedirs(project_firmware_path)

        bin_files = glob.glob(os.path.join('.pio', 'build', tag, '*.bin'))

        for bin_file in bin_files:
            shutil.copy(bin_file, project_firmware_path)

        project_manifest = manifest_text.replace('<name>', project['name'] + ' for ' + device_name)
        project_manifest = project_manifest.replace('<version>', version)
        project_manifest = project_manifest.replace('<chipfamily>', device['chipfamily'])
        project_manifest = project_manifest.replace('<tag>', tag)

        with open(os.path.join(manifest_target_path, manifest_prefix + tag + '.json'), 'w', encoding='utf-8') as f:
            f.write(project_manifest)
