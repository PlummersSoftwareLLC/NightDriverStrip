#!/usr/bin/env python

#--------------------------------------------------------------------------
#
# File:        installer_vars.py
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
#    Global variables and functions for the installer_*.py scripts
#
# History:     Jul-12-2024         Rbergen      Created
#
#---------------------------------------------------------------------------

import json
import os

class Dirs:
    build = 'build'
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

class Files:
    webprojects = 'web_projects.json'
    feature_flags = 'feature_flags.json'
    features = 'features.json'
    manifest = 'manifest.json'
    merged_image = 'merged_image.bin'
    globals_h = 'globals.h'
    index_template = 'installer_index.html'
    index = 'index.html'

def get_project_tree():
    with open(os.path.join(Dirs.config, Files.webprojects), "r", encoding='utf-8') as f:
        return json.load(f).get('devices')

def get_project(tag: str):
    for device in get_project_tree():
        merge = device['merge'] if 'merge' in device else True
        for project in device['projects']:
            if project['tag'] == tag:
                project['device'] = device['name']
                project['chipfamily'] = device['chipfamily']
                if 'merge' not in project:
                    project['merge'] = merge
                return project

    return None