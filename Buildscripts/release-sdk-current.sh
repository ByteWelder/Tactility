#!/bin/sh

#
# Description: Releases the current build files as an SDK in release/TactilitySDK-[platform]
# This deployment is used when compiling new SDK features for apps.
#

config_idf_target=`cat sdkconfig | grep CONFIG_IDF_TARGET=`
idf_target=${config_idf_target:19:-1}
sdk_path=release/TactilitySDK-${idf_target}
rm -rf ${sdk_path}
./Buildscripts/release-sdk.sh ${sdk_path}

