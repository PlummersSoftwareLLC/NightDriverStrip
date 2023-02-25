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

webprojects_path = os.path.join(config_dir, webprojects_file)

shutil.copy(webprojects_path, webinstaller_dir)

firmware_path = os.path.join(webinstaller_dir, firmware_dir)

if not os.path.exists(firmware_path):
    os.makedirs(firmware_path)

f = open(webprojects_path, "r", encoding='utf-8')
json_data = json.load(f)
devices = json_data['devices']
f.close()

f = open(os.path.join(config_dir, manifest_template), "r", encoding='utf-8')
manifest_text = f.read()
f.close()

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

        project_manifest = manifest_text.replace('<name>', project['name'] + ' for ' + device['name'])
        project_manifest = project_manifest.replace('<chipfamily>', device['chipfamily'])
        project_manifest = project_manifest.replace('<tag>', tag)

        f = open(os.path.join(manifest_target_path, manifest_prefix + tag + '.json'), 'w', encoding='utf-8')
        f.write(project_manifest)
        f.close()
