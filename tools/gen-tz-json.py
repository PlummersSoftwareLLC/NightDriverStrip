#!/usr/bin/env python

# This script is based on the one in https://github.com/nayarsystems/posix_tz_db. As such, this script is subject to the
# MIT license. As the script relies on the presence of the Linux /usr/share/zoneinfo tzdata tree, it can only be used on
# a (virtualized) Linux environment.

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

    with open(os.path.join("config", "timezones.json"), "w") as jsonfile:
        json.dump(timezones_dict, jsonfile, indent=0, sort_keys=False, separators=(",", ":"))
