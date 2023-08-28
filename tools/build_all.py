#!/usr/bin/env python

#--------------------------------------------------------------------------
#
# File:        build_all.py
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
#    This script builds all the environment names defined in platformio.ini.
#
#    Note that it expects to be executed from the project root directory. That is,
#    it needs to be run like this:
#
#    $ tools/build_all.py
#
#    Instead of:
#
#    $ cd tools
#    $ ./build_all.py
#
# History:     Aug-27-2023         Rbergen      Created
#
#---------------------------------------------------------------------------

import show_envs
import subprocess

def buildenvs():
    errors = []

    for env in show_envs.getenvs():
        try:
            subprocess.run(['pio', 'run', '-e', env], check=True)
        except subprocess.CalledProcessError as cpe:
            errors.append('Process \'' + ' '.join(cpe.cmd) + '\' completed with error code ' + str(cpe.returncode))

    return errors

if __name__ == '__main__':
    errors = buildenvs()

    print()
    print('=' * 79)
    print()

    if len(errors) > 0:
        print('Builds completed with errors:')
        for error in errors:
            print('* ' + error)

        exit(1)

    else:
        print('Builds completed successfully.')

        exit()