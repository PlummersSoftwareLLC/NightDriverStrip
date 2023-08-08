#!/usr/bin/env python

# Copyright (c) 2017 José Luis Aracil Gómez del Campo
# Copyright (c) 2023 Plummer's Software LLC
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#================================================================================
#
# Generate a JSON object with timezone information from Linux tzdata and write it
# to a file.
#
# This script is based on the one in https://github.com/nayarsystems/posix_tz_db.
#
# The script expects to be started from the project root, i.e. by executing
# "tools/gen-tz-json.py". As it relies on the presence of the Linux
# /usr/share/zoneinfo tzdata tree, it can only be used on a (virtualized) Linux
# environment.

import json
import os
import platform

ZONES_ROOT = "/usr/share/zoneinfo"
ZONES_DIRS = [
    "Africa",
    "America",
    "Antarctica",
    "Arctic",
    "Asia",
    "Atlantic",
    "Australia",
    "Europe",
    "Indian",
    "Pacific",
    "Etc"
]

def traverse_directory_trees(parent_directory, directory_list):
    contents = []

    def traverse_directory(directory):
        absolute_directory = os.path.join(parent_directory, directory)
        items = sorted(os.listdir(absolute_directory))
        for item in items:
            item_path = os.path.join(directory, item)
            absolute_item_path = os.path.join(absolute_directory, item)
            if os.path.isfile(absolute_item_path):
                contents.append(item_path)
            else:
                traverse_directory(item_path)

    for directory in directory_list:
        traverse_directory(directory)

    return contents


def get_tz_string(timezone):
    data = open(os.path.join(ZONES_ROOT, timezone), "rb").read().split(b"\n")[-2]
    return data.decode("utf-8")


def make_timezones_dict(timezones):
    result = {}
    for timezone in timezones:
        timezone = timezone.strip()
        result[timezone] = get_tz_string(timezone)
    return result


if __name__ == "__main__":

    if platform.system() == 'Windows':
        print("This script can only be run in a Linux environment. You can probably use a WSL distribution under Windows.")
        exit()
    elif platform.system() != 'Linux':
        print("This script can only be run in a Linux environment. You may be able to use a virtual machine for this.")
        exit()

    timezones_list = traverse_directory_trees(ZONES_ROOT, ZONES_DIRS)
    timezones_dict = make_timezones_dict(timezones_list)

    json_file_path = os.path.join("config", "timezones.json")

    with open(json_file_path, "w") as json_file:
        json.dump(timezones_dict, json_file, indent=0, sort_keys=False, separators=(",", ":"))

    print(f"JSON tzdata file written to {json_file_path}.")
