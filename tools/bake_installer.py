import os
import glob
import json
import shutil

config_dir = 'config'
webinstaller_dir = 'WebInstaller'
firmware_dir = 'firmware'
manifests_dir = 'manifests'
manifest_prefix = 'manifest_'
webprojects_file = 'web_projects.json'

webprojects_path = os.path.join(config_dir, webprojects_file)

shutil.copy(webprojects_path, webinstaller_dir)

firmware_path = os.path.join(webinstaller_dir, firmware_dir)

if not os.path.exists(firmware_path):
    os.makedirs(firmware_path)

f = open(webprojects_path, "r", encoding='utf-8')
devices = json.load(f)
f.close()

manifest_source_path = os.path.join(config_dir, manifests_dir)
manifest_target_path = os.path.join(webinstaller_dir, manifests_dir)
if not os.path.exists(manifest_target_path):
    os.makedirs(manifest_target_path)

for device in devices:
    for project in device['projects']:
        tag = project['tag']
        project_firmware_path = os.path.join(firmware_path, tag)

        if not os.path.exists(project_firmware_path):
            os.makedirs(project_firmware_path)

        bin_files = glob.glob(os.path.join('.pio', 'build', tag, '*.bin'))

        for bin_file in bin_files:
            shutil.copy(bin_file, project_firmware_path)

        manifest_file_path = os.path.join(manifest_source_path, manifest_prefix + tag + '.json')
        if os.path.exists(manifest_file_path):
            shutil.copy(manifest_file_path, manifest_target_path)