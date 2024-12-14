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

build lilygo-tdeck
release lilygo-tdeck

releaseSdk release/Tactility-ESP32S3-SDK/TactilitySDK

build yellow-board
release yellow-board

releaseSdk release/Tactility-ESP32-SDK/TactilitySDK

build m5stack-core2
release m5stack-core2

build m5stack-cores3
release m5stack-cores3

duration=$SECONDS

echo "Finished in $((duration / 60)) minutes and $((duration % 60)) seconds."