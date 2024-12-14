#!/bin/bash

function build() {
    Buildscripts/build.sh $1
}

function release() {
    Buildscripts/release.sh $1
}

function releaseSdk() {
    Buildscripts/release-sdk.sh $1
}

SECONDS=0

build lilygo_tdeck
release lilygo_tdeck

releaseSdk release/Tactility-ESP32S3-SDK/TactilitySDK

build yellow_board
release yellow_board

releaseSdk release/Tactility-ESP32-SDK/TactilitySDK

build m5stack_core2
release m5stack_core2

build m5stack_cores3
release m5stack_cores3

duration=$SECONDS

echo "Finished in $((duration / 60)) minutes and $((duration % 60)) seconds."