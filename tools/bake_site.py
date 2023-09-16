#!/usr/bin/env python

#--------------------------------------------------------------------------
#
# File:        bake_site.py
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
#    Script to bake the on-board web application ("site")
#
# History:     Aug-08-2023         Rbergen      Added header
#
#---------------------------------------------------------------------------

import os
import sys
import subprocess

localBuild=False
for i, arg in enumerate(sys.argv):
    if arg == 'local':
        localBuild = True

if localBuild:
    subprocess.check_call('cd site && npm run local', shell=True)
else:
    subprocess.check_call('cd site && npm run build', shell=True)
destFolder = os.path.join('site', 'dist')


htmlBytes = os.stat(os.path.join(destFolder, 'index.html.gz')).st_size
jsxBytes = os.stat(os.path.join(destFolder, 'index.js.gz')).st_size
icoBytes = os.stat(os.path.join(destFolder, 'favicon.ico.gz')).st_size
totalBytes = htmlBytes + jsxBytes + icoBytes
print('Build completed, html: %d B, jsx: %d B, ico: %d B, total: %d KB' % (htmlBytes, jsxBytes, icoBytes, totalBytes / 1024))
