#!/bin/sh

#
# Usage: release-simulator.sh [builddir] [target_path]
# Example: release-simulator.sh buildsim release/Simulator-linux-amd64
# Description: Releases the current simulator build files in the specified folder.
#

build_path=$1
target_path=$2

mkdir -p $target_path

cp version.txt $target_path
cp $build_path/Firmware/FirmwareSim $target_path/
cp -r Data/data $target_path/
cp -r Data/system $target_path/
