import subprocess
import os
from distutils.spawn import find_executable

if (os.name == "posix") & (find_executable("npm") is not None):
    os.chdir('site')
    subprocess.check_output("npm run build", shell=True)
else:
    print("Please follow the instructions in the README.md of the site folder to build the site")
