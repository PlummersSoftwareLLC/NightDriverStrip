#!/bin/bash

set -e
platformio run -e demo
platformio run -e ledstrip
platformio run -e ledstrip_feather
platformio run -e spectrum
platformio run -e mesmerizer

