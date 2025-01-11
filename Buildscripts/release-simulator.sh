#!/usr/bin/bash

#
# Usage: release-sdk.sh [target_path]
# Example: release.sh release/TactilitySDK
# Description: Releases the current build files as an SDK in the specified folder.
#

build_path=$1
target_path=$2

mkdir -p $target_path

build_dir=`pwd`

cp version.txt $target_path

cp $build_path/App/AppSim $target_path/
cp -r Data/data $target_path/
cp -r Data/system $target_path/
