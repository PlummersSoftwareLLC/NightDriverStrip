#!/usr/bin/env python

# This script outputs a JSON array with all the environment names defined in platformio.ini.
# It is used by this project's GitHub CI workflow.
#
# Note that it expects to be executed from the project root directory. That is, it needs to be
# run like this:
#
# $ tools/show_envs.py
#
# Instead of:
#
# $ cd tools
# $ ./show_envs.py
#

import configparser
import json

config = configparser.ConfigParser()
config.read('platformio.ini')

envs = []

for section in config.sections():
    if section.startswith('env:') and len(section) > 4:
        envs.append(section[4::])

print(json.dumps(envs))