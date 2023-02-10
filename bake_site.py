import subprocess
import os
import sys
import glob

def getJsx(folder,mask,exclude=""):
    files = glob.glob(folder + '/**/' + mask, recursive=True)
    ret = ""
    for i, file in enumerate(files):
        with open(file,'r') as reader:
            if ((exclude == "") or (file.endswith(exclude) == False)):
                ret+=reader.read()
    return ret

localBuild=False
for i, arg in enumerate(sys.argv):
    if (arg == 'local'):
        localBuild=True

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
    print("Local Build")
    jsx.write(open('site/src/espaddr.jsx').read())
else:
    print("Chip Build")
    jsx.write("const httpPrefix='';")

jsx.write(open('site/src/modules/modules.jsx').read())
jsx.write(getJsx('site/src/theme',"*.jsx"))
jsx.write(getJsx('site/src/components',"style.jsx"))
jsx.write(getJsx('site/src/components',"*.jsx","style.jsx"))
jsx.write(open('site/src/main.jsx').read())
print("Build completed")

