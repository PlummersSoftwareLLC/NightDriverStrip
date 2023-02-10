#!/bin/python

import subprocess
import os
import sys
import glob
from distutils.spawn import find_executable

localBuild=False
for i, arg in enumerate(sys.argv):
    if (arg == 'local'):
        localBuild=True

def minimize(content):
    if (localBuild):
        return content
    else:
        return ' '.join(map(lambda line: line.strip(), content.splitlines()))

def getJsx(folder,mask,exclude=""):
    files = glob.glob(folder + '/**/' + mask, recursive=True)
    ret = ""
    for i, file in enumerate(files):
        with open(file,'r') as reader:
            if ((exclude == "") or (file.endswith(exclude) == False)):
                ret+=reader.read()
    return ret

destFolder="data/"

if (localBuild):
    destFolder="site/local/"
    if (os.path.exists(destFolder) == False):
        print("Creating local build folder")
        os.mkdir(destFolder)
else:
    if (os.path.exists(destFolder) == False):
        print("Creating chip build folder")
        os.mkdir(destFolder)

jsx = open(destFolder + "main.jsx", "w")
html = open(destFolder + "index.html", "w")

html.write(open('site/src/index.html').read())
html.close()

if (localBuild):
    print("Building site in local development code")
    jsx.write(open('site/src/espaddr.jsx').read())
else:
    print("Building site in Chip model")
    jsx.write("const httpPrefix='';")

jsx.write(minimize(open('site/src/modules/modules.jsx').read()))
jsx.write(minimize(getJsx('site/src/theme',"*.jsx")))
jsx.write(minimize(getJsx('site/src/components',"style.jsx")))
jsx.write(minimize(getJsx('site/src/components',"*.jsx","style.jsx")))
jsx.write(minimize(open('site/src/main.jsx').read()))
jsx.close()

htmlBytes = os.stat(destFolder + "index.html").st_size
jsxBytes = os.stat(destFolder + "main.jsx").st_size
print('Build completed, html: %d bytes, jsx: %d total: %dK' % (htmlBytes, jsxBytes,(htmlBytes + jsxBytes)/1024))
