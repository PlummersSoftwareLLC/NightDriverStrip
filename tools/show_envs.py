#!/usr/bin/env python

# This script outputs a JSON array with all the environment names defined in platformio.ini.
# It is used by this project GitHub CI workflow.
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

config = configparser.ConfigParser()
config.read('platformio.ini')

envs = ''

for env in config.sections():
    if env.startswith('env:'):
        if envs != '':
            envs += ','
        envs += '"' + env[4::] + '"'

print('[' + envs + ']')