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

release() {
    target_path=$1
    echoNewPhase "Making release at '$target_path'"

    bin_path=$target_path/Binaries
    mkdir -p $bin_path
    mkdir -p $bin_path/partition_table
    mkdir -p $bin_path/bootloader
    cp build/*.bin $bin_path/
    cp build/bootloader/*.bin $bin_path/bootloader/
    cp build/partition_table/*.bin $bin_path/partition_table/
    cp build/flash_args $bin_path/
    cp build/flasher_args.json $bin_path/

    cp Buildscripts/Flashing/* $target_path
}

releaseSymbols() {
    target_path=$1
    echoNewPhase "Making symbols release at '$target_path'"
    mkdir $target_path
    cp build/*.elf $target_path/
}

releaseAll() {
    board=$1
    board_clean=${board/_/-}
    build sdkconfig.board.$board
    release $release_path/Tactility-$board_clean
    releaseSymbols $release_path/Tactility-$board_clean-symbols
}

release_path=release

releaseAll lilygo_tdeck

./Buildscripts/release-sdk.sh $release_path/Tactility-ESP32S3-SDK/TactilitySDK

releaseAll yellow_board

./Buildscripts/release-sdk.sh $release_path/Tactility-ESP32-SDK/TactilitySDK

releaseAll m5stack_core2

releaseAll m5stack_cores3

exit 1
