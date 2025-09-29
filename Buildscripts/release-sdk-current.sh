#!/bin/sh

#
# Description: Releases the current build files as an SDK in release/TactilitySDK-[platform]
# This deployment is used when compiling new SDK features for apps.
#

idf_target=`Buildscripts/get-idf-target.sh`
version=`cat version.txt`
sdk_path="release/TactilitySDK/${version}-${idf_target}/TactilitySDK"
mkdir -p ${sdk_path}
echo Cleaning up ${sdk_path}
rm -rf ${sdk_path}
./Buildscripts/release-sdk.sh ${sdk_path}