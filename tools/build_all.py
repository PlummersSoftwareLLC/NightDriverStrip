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

for env in show_envs.getenvs():
    subprocess.run(['pio', 'run', '-e', env])