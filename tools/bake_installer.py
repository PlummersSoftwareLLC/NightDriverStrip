#!/usr/bin/env python

import os
import glob
import json
import shutil

# Directories used throughough the script
class Dirs:
    config = 'config'
    webinstaller = 'WebInstaller'
    firmware = 'firmware'
    manifests = 'manifests'
    include = 'include'
    assets = 'assets'

# Properties relating to manifest( template)s
class Manifest:
    base = 'manifest_'
    template_default = 'manifest_template.json'
    ext = '.json'

webprojects_file = 'web_projects.json'
firmware_file = 'firmware_merged.bin'
globals_h = 'globals.h'

webprojects_path = os.path.join(Dirs.config, webprojects_file)
shutil.copy(webprojects_path, Dirs.webinstaller)

assets_target_path = os.path.join(Dirs.webinstaller, Dirs.assets)
if not os.path.exists(assets_target_path):
    os.makedirs(assets_target_path)
shutil.copy(os.path.join(Dirs.assets, 'favicon.ico'), assets_target_path)
shutil.copy(os.path.join(Dirs.assets, 'NightDriverLogo-small.png'), assets_target_path)

firmware_target_dir = os.path.join(Dirs.webinstaller, Dirs.firmware)
if not os.path.exists(firmware_target_dir):
    os.makedirs(firmware_target_dir)

with open(webprojects_path, "r", encoding='utf-8') as f:
    json_data = json.load(f)
    devices = json_data['devices']

# Read the text of the "default" manifest template; we can assume we need that
with open(os.path.join(Dirs.config, Manifest.template_default), "r", encoding='utf-8') as f:
    template = f.read()

version = ''
with open(os.path.join(Dirs.include, globals_h), "r", encoding='utf-8') as f:
    while (line := f.readline()):
        if '#define FLASH_VERSION' in line:
            version = line.split()[2]
            break

# Create a neat 3-digit version number
version = '0' * (3 - len(version)) + version

manifest_target_dir = os.path.join(Dirs.webinstaller, Dirs.manifests)
if not os.path.exists(manifest_target_dir):
    os.makedirs(manifest_target_dir)

for device in devices:
    device_name = device['name']
    chip_family = device['chipfamily']

    for project in device['projects']:
        tag = project['tag']

        project_firmware_source_dir = os.path.join('.pio', 'build', tag)
        if not os.path.exists(project_firmware_source_dir):
            continue

        project_firmware_target_dir = os.path.join(firmware_target_dir, tag)

        if not os.path.exists(project_firmware_target_dir):
            os.makedirs(project_firmware_target_dir)

        project_firmware_source_path = os.path.join(project_firmware_source_dir, firmware_file)
        shutil.copy(project_firmware_source_path, project_firmware_target_dir)

        project_manifest = template.replace('<name>', project['name'] + ' for ' + device_name)
        project_manifest = project_manifest.replace('<version>', version)
        project_manifest = project_manifest.replace('<chipfamily>', chip_family)
        project_manifest = project_manifest.replace('<tag>', tag)

        with open(os.path.join(manifest_target_dir, Manifest.base + tag + Manifest.ext), 'w', encoding='utf-8') as f:
            f.write(project_manifest)
