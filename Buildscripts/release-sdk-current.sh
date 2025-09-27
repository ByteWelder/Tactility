#!/bin/sh

#
# Description: Releases the current build files as an SDK in release/TactilitySDK
# This deployment is used when compiling apps in ./ExternalApps
#

rm -rf release/TactilitySDK
./Buildscripts/release-sdk.sh release/TactilitySDK

