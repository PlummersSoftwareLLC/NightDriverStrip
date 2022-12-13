#!/bin/bash
#
# Quick and dirty shell script to build a bunch of targets as a smoke-test before CI

set -e
platformio run -e demo
platformio run -e ledstrip
platformio run -e ledstrip_feather
platformio run -e spectrum
platformio run -e mesmerizer
platformio run -e umbrella
platformio run -e generic
platformio run -e m5demo
platformio run -e m5plusdemo
platformio run -e panlee




