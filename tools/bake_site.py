#!/usr/bin/env python

import os
import sys
import glob

localBuild=False
for i, arg in enumerate(sys.argv):
    if arg == 'local':
        localBuild = True

def minimize(content):
    if localBuild:
        return content
    else:
        return '\n'.join(map(lambda line: line.strip(), content.splitlines()))

def getJsx(folder, mask, exclude=''):
    files = glob.glob(os.path.join(folder, '**', mask), recursive=True)
    ret = ''
    for i, file in enumerate(files):
        with open(file, encoding='utf-8') as reader:
            if exclude == '' or not file.endswith(exclude):
                ret += reader.read()
    return ret

destFolder='site'
buildType='chip'

if localBuild:
    destFolder = os.path.join(destFolder, 'local')
    buildType = 'local'

if not os.path.exists(destFolder):
    print('Creating ' + buildType + ' build folder')
    os.makedirs(destFolder)

jsxPath = os.path.join(destFolder, 'main.jsx')
jsx = open(jsxPath, 'w', encoding='utf-8')
htmlPath = os.path.join(destFolder, 'index.html')
html = open(htmlPath, 'w', encoding='utf-8')

srcFolder = os.path.join('site', 'src')

html.write(open(os.path.join(srcFolder, 'index.html'), encoding='utf-8').read())
html.close()

if (localBuild):
    print('Building site in local development code')
    jsx.write(open(os.path.join(srcFolder, 'espaddr.jsx'), encoding='utf-8').read())
else:
    print('Building site in Chip model')
    jsx.write("const httpPrefix='';")

jsx.write(minimize(open(os.path.join(srcFolder, 'modules', 'modules.jsx'), encoding='utf-8').read()))
jsx.write(minimize(getJsx(os.path.join(srcFolder, 'theme'), '*.jsx')))
compFolder = os.path.join(srcFolder, 'components')
jsx.write(minimize(getJsx(compFolder, 'style.jsx')))
jsx.write(minimize(getJsx(compFolder, '*.jsx', 'style.jsx')))
jsx.write(minimize(open(os.path.join(srcFolder, 'main.jsx'), encoding='utf-8').read()))
jsx.close()

htmlBytes = os.stat(htmlPath).st_size
jsxBytes = os.stat(jsxPath).st_size
print('Build completed, html: %d bytes, jsx: %d total: %dK' % (htmlBytes, jsxBytes, (htmlBytes + jsxBytes) / 1024))
