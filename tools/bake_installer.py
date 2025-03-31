#!/usr/bin/env python

#--------------------------------------------------------------------------
#
# File:        bake_installer.py
#
# NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights Reserved.
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
#    Script to bake the Web Installer, including firmware and manifests
#
# History:     Aug-08-2023         Rbergen      Added header
#              Jul-12-2024         Rbergen      Refactored to support
#                                               parallel builds
#
#---------------------------------------------------------------------------

import os
import shutil
import sys
import installer_buildenv
import installer_compose
import installer_vars
import installer_version

release_name = sys.argv[1] if len(sys.argv) > 1 else 'unnamed'
version = installer_version.get_version()
devices = installer_vars.get_project_tree()

# Find out how many projects we're going to build
projectCount = 0

for device in devices:
    projectCount += len(device['projects'])

currentProject = 0

# Start building images!
for device in devices:
    for project in device['projects']:
        tag = project['tag']

        currentProject += 1

        print('===')
        print('=' * 79)
        print('=== Building Web Installer project ' + tag + ' (' + str(currentProject) + ' of ' + str(projectCount) + ')')
        print('=' * 79)
        print('===', flush = True)

        installer_buildenv.build_env(tag, version)

# Now compose the installer
installer_compose.compose_installer(release_name)