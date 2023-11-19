#!/usr/bin/env python

#--------------------------------------------------------------------------
#
# File:        show_features.py
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
#    This script generates JSON output with the features enabled within one or all environments defined in
#    platformio.ini.
#
#    Note that it expects to be executed from the project root directory. That is,
#    it needs to be run like this:
#
#    $ tools/show_features.py
#
#    Instead of:
#
#    $ cd tools
#    $ ./show_features.py
#
#    Execute it with the -h argument for help on usage.
#
# History:     Nov-18-2023         Rbergen      Created
#
#---------------------------------------------------------------------------

import argparse
import bisect
import glob
import json
import os
import subprocess
import shutil
import show_envs
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def get_features(environment_name):
    if not environment_name:
        return []

    cpp_file = '-defines-.cpp'
    shutil.copy(os.path.join('tools', cpp_file), 'src')
    build_path = os.path.join('.pio', 'build', environment_name, 'src')

    pio_env = os.environ.copy()
    pio_env['PLATFORMIO_BUILD_FLAGS'] = '-dM -E'

    try:
        subprocess.run(['pio', 'run', '-e', environment_name, '-t', '.pio/build/' + environment_name + '/src/' + cpp_file + '.o'],
                       check=True,
                       env=pio_env,
                       capture_output=True)
        build_succeeded = True
    except subprocess.CalledProcessError as cpe:
        eprint('Process \'' + ' '.join(cpe.cmd) + '\' completed with error code ' + str(cpe.returncode))
        build_succeeded = False

    defines = []
    if build_succeeded:
        # We sort the contents of the file, so we can binary search in it later
        with open(os.path.join(build_path, cpp_file + '.o'), "r", encoding='utf-8') as f:
            defines = sorted(f.readlines()[:-1])

    # Make sure to always clean up!
    os.remove(os.path.join('src', cpp_file))
    for file in glob.glob(os.path.join(build_path, cpp_file + '.*')):
        os.remove(file)

    if not build_succeeded:
        return []

    with open(os.path.join('config', 'feature_flags.json'), "r", encoding='utf-8') as f:
        known_features = json.load(f)

    found_features = []

    for tag, feature in known_features.items():
        feature_define = '#define ' + feature['define'] + ' ' + feature['value'] + '\n'
        # Bisect almost sneakily implements a binary search algorithm, if you look behind the curtains
        found_index = bisect.bisect_left(defines, feature_define)
        if found_index != len(defines) and defines[found_index] == feature_define:
            found_features.append(tag)

    return found_features

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Show enabled features')
    parser.add_argument('environment',
                        nargs='?',
                        help='the environment to show features for, or all if not specified')
    parser.add_argument('-q', '--quiet',
                        action='store_true',
                        help='suppress progress updates')
    parser.add_argument('-m', '--minimize',
                        action='store_true',
                        help='don\'t indent output')

    args = parser.parse_args()

    if args.environment is not None:
        print(json.dumps(get_features(args.environment)))
    else:
        platformio_envs = show_envs.getenvs()

        env_features = {}

        for env in platformio_envs:
            if not args.quiet:
                eprint('Fetching features for environment ' + env)
            env_features[env] = get_features(env)

        print(json.dumps(env_features, indent=0 if args.minimize else 2))
