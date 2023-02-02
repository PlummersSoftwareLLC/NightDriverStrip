import subprocess
import os
os.chdir('site')

subprocess.check_output("npm run build", shell=True)
