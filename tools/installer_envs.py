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
#    Collect/show the projects configured for inclusion in the Web Installer
#
# History:     Jul-12-2024         Rbergen      Created
#
#---------------------------------------------------------------------------

import installer_vars
import json

def get_envs():
    projects = []

    for device in installer_vars.get_project_tree():
        for project in device['projects']:
            projects.append(project['tag'])

    return projects

if __name__ == '__main__':
    print(json.dumps(get_envs()))