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
    prefix = 'manifest_'
    ext = '.json'
    template = 'manifest_template.json'

webprojects_file = 'web_projects.json'
globals_h = 'globals.h'

webprojects_path = os.path.join(Dirs.config, webprojects_file)
shutil.copy(webprojects_path, Dirs.webinstaller)

assets_target_dir = os.path.join(Dirs.webinstaller, Dirs.assets)
if not os.path.exists(assets_target_dir):
    os.makedirs(assets_target_dir)
shutil.copy(os.path.join(Dirs.assets, 'favicon.ico'), assets_target_dir)
shutil.copy(os.path.join(Dirs.assets, 'NightDriverLogo-small.png'), assets_target_dir)

firmware_target_dir = os.path.join(Dirs.webinstaller, Dirs.firmware)
if not os.path.exists(firmware_target_dir):
    os.makedirs(firmware_target_dir)

with open(webprojects_path, "r", encoding='utf-8') as f:
    json_data = json.load(f)
    devices = json_data['devices']

with open(os.path.join(Dirs.config, Manifest.template), "r", encoding='utf-8') as f:
    manifest_text = f.read()

version = ''
with open(os.path.join(Dirs.include, globals_h), "r", encoding='utf-8') as f:
    while (line := f.readline()):
        if '#define FLASH_VERSION' in line:
            version = line.split()[2]
            break

version = '0' * (3 - len(version)) + version

manifest_target_dir = os.path.join(Dirs.webinstaller, Dirs.manifests)
if not os.path.exists(manifest_target_dir):
    os.makedirs(manifest_target_dir)

for device in devices:
    device_name = device['name']

    for project in device['projects']:
        tag = project['tag']

        subprocess.run(['pio', 'run', '-e', tag])
        subprocess.run(['pio', 'run', '-e', tag, '-t', 'buildfs'])

        project_firmware_target_dir = os.path.join(firmware_target_dir, tag)

        if not os.path.exists(project_firmware_target_dir):
            os.makedirs(project_firmware_target_dir)

        project_build_dir = os.path.join('.pio', 'build', tag)
        bin_files = glob.glob(os.path.join(project_build_dir, '*.bin'))

        for bin_file in bin_files:
            shutil.copy(bin_file, project_firmware_target_dir)

        project_manifest = manifest_text.replace('<name>', project['name'] + ' for ' + device_name)
        project_manifest = project_manifest.replace('<version>', version)
        project_manifest = project_manifest.replace('<chipfamily>', device['chipfamily'])
        project_manifest = project_manifest.replace('<tag>', tag)

        with open(os.path.join(manifest_target_dir, Manifest.prefix + tag + Manifest.ext), 'w', encoding='utf-8') as f:
            f.write(project_manifest)

        shutil.rmtree(project_build_dir)
