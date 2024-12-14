#!/bin/bash

echoNewPhase() {
    echo -e "⏳ \e[36m${1}\e[0m"
}

fatalError() {
    echo -e "⚠️ \e[31m${1}\e[0m"
    exit 0
}

build() {
    sdkconfig_file=$1
    echoNewPhase "Building $sdkconfig_file"

    rm -rf build

    cp $sdkconfig_file sdkconfig
    if not idf.py build; then
        fatalError "Failed to build esp32s3 SDK"
    fi
}

releaseBuild() {
    target_path=$1
    target_name=$2
    echoNewPhase "Making release at '$target_path' for '$target_name'"

    mkdir -p $target_path
    cp build/Tactility.bin $target_path/$target_name.bin
    cp build/Tactility.elf $target_path/$target_name.elf
}

release_path=release
release_version=`cat ../version.txt`

build sdkconfig.board.lilygo_tdeck
releaseBuild $release_path/ Tactility-Lilygo-Tdeck-$release_version
./Buildscripts/release-sdk.sh $release_path/Tactility-ESP32S3-SDK-$release_version/TactilitySDK

build sdkconfig.board.sdkconfig.board.yellow_board
releaseBuild $release_path/ Tactility-Yellow-Board-$release_version
./Buildscripts/release-sdk.sh $release_path/Tactility-ESP32-SDK-$release_version/TactilitySDK

exit 1
