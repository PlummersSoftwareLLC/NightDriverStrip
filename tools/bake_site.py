#!/usr/bin/env python

import os
import sys
import glob
import shutil

destFolder=os.path.join('site', 'dist')
backupAddr="espaddr.tsx.orig"
srcFolder = os.path.join('site', 'src')
htmlFile = 'index.html.gz'
icoFile = 'favicon.ico'
espAddr= 'espaddr.tsx'

shutil.copy(os.path.join(srcFolder, espAddr), os.path.join(srcFolder, backupAddr))

jsPath = os.path.join(srcFolder, espAddr)
js = open(jsPath, 'w', encoding='utf-8')
js.write("export const httpPrefix='';")
js.close()

os.chdir('site')
os.system('yarn')
os.system('yarn build')
os.chdir('..')

shutil.copy(os.path.join(srcFolder, backupAddr), os.path.join(srcFolder, espAddr))
os.remove(os.path.join(srcFolder, backupAddr))

htmlBytes = os.stat(os.path.join(destFolder, htmlFile)).st_size
jsBytes = os.stat(jsPath).st_size
icoBytes = os.stat(os.path.join(destFolder, icoFile)).st_size
totalBytes = htmlBytes + jsBytes + icoBytes
print('Build completed, html: %d B, js: %d B, ico: %d B, total: %d KB' % (htmlBytes, jsBytes, icoBytes, totalBytes / 1024))
