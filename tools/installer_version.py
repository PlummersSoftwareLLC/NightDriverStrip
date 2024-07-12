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
#    Determine the version of the firmware as to be used in the Web Installer
#
# History:     Jul-12-2024         Rbergen      Created
#
#---------------------------------------------------------------------------

import os
import installer_vars

def get_version():
    # Extract the version from globals.h
    version = ''
    with open(os.path.join(installer_vars.Dirs.include, installer_vars.Files.globals_h), "r", encoding='utf-8') as f:
        while (line := f.readline()):
            if '#define FLASH_VERSION' in line:
                version = line.split()[2]
                break

    # Create a neat 3-digit version number
    version = '0' * (3 - len(version)) + version

    return version

if __name__ == '__main__':
    print(get_version())