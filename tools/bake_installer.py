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
    template_base = 'manifest_template_'
    ext = '.json'

webprojects_file = 'web_projects.json'
globals_h = 'globals.h'

webprojects_path = os.path.join(Dirs.config, webprojects_file)
shutil.copy(webprojects_path, Dirs.webinstaller)

assets_target_path = os.path.join(Dirs.webinstaller, Dirs.assets)
if not os.path.exists(assets_target_path):
    os.makedirs(assets_target_path)
shutil.copy(os.path.join(Dirs.assets, 'favicon.ico'), assets_target_path)
shutil.copy(os.path.join(Dirs.assets, 'NightDriverLogo-small.png'), assets_target_path)

firmware_path = os.path.join(Dirs.webinstaller, Dirs.firmware)
if not os.path.exists(firmware_path):
    os.makedirs(firmware_path)

with open(webprojects_path, "r", encoding='utf-8') as f:
    json_data = json.load(f)
    devices = json_data['devices']

# Read the text of the "default" manifest template; we can assume we need that
with open(os.path.join(Dirs.config, Manifest.template_default), "r", encoding='utf-8') as f:
    template = f.read()

manifest_texts = {'': template}

version = ''
with open(os.path.join(Dirs.include, globals_h), "r", encoding='utf-8') as f:
    while (line := f.readline()):
        if '#define FLASH_VERSION' in line:
            version = line.split()[2]
            break

# Create a neat 3-digit version number
version = '0' * (3 - len(version)) + version

manifest_target_path = os.path.join(Dirs.webinstaller, Dirs.manifests)
if not os.path.exists(manifest_target_path):
    os.makedirs(manifest_target_path)

for device in devices:
    device_name = device['name']
    chip_family = device['chipfamily']

    for project in device['projects']:
        tag = project['tag']

        # If the project mentions a suffix, use the relevant manifest template (after loading it if necessary).
        # Otherwise, use the default.
        if 'suffix' in project:
            suffix = project['suffix']
            if suffix in manifest_texts:
                template = manifest_texts[suffix]
            else:
                with open(os.path.join(Dirs.config, Manifest.template_base + suffix + Manifest.ext), "r", encoding='utf-8') as f:
                    template = f.read()

                manifest_texts[suffix] = template
        else:
            template = manifest_texts['']

        project_firmware_path = os.path.join(firmware_path, tag)

        if not os.path.exists(project_firmware_path):
            os.makedirs(project_firmware_path)

        bin_files = glob.glob(os.path.join('.pio', 'build', tag, '*.bin'))

        for bin_file in bin_files:
            shutil.copy(bin_file, project_firmware_path)

        project_manifest = template.replace('<name>', project['name'] + ' for ' + device_name)
        project_manifest = project_manifest.replace('<version>', version)
        project_manifest = project_manifest.replace('<chipfamily>', chip_family)
        project_manifest = project_manifest.replace('<tag>', tag)

        with open(os.path.join(manifest_target_path, Manifest.base + tag + Manifest.ext), 'w', encoding='utf-8') as f:
            f.write(project_manifest)
