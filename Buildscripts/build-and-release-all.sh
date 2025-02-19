#!/bin/sh

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

build elecrow-crowpanel-advance-28
release elecrow-crowpanel-advance-28

build lilygo-tdeck
release lilygo-tdeck

releaseSdk release/TactilitySDK-esp32s3

build yellow-board
release yellow-board

releaseSdk release/TactilitySDK-esp32

build m5stack-core2
release m5stack-core2

build m5stack-cores3
release m5stack-cores3

duration=$SECONDS

echo "Finished in $((duration / 60)) minutes and $((duration % 60)) seconds."