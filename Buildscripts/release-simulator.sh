#!/usr/bin/bash

#
# Usage: release-sdk.sh [target_path]
# Example: release.sh release/TactilitySDK
# Description: Releases the current build files as an SDK in the specified folder.
#

target_path=$1

mkdir -p $target_path

build_dir=`pwd`

cp version.txt $target_path

cp buildsim/App/AppSim $target_path/
cp -r Data/data $target_path/
cp -r Data/system $target_path/
