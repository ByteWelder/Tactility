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

build elecrow-crowpanel-advance-35
release elecrow-crowpanel-advance-35

build elecrow-crowpanel-basic-28
release elecrow-crowpanel-basic-28

build elecrow-crowpanel-basic-35
release elecrow-crowpanel-basic-35

build lilygo-tdeck
release lilygo-tdeck

releaseSdk release/TactilitySDK-esp32s3

build cyd-2432s024c
release cyd-2432s024c

build cyd-8048s043c
release cyd-8048s043c

build cyd-jc2432w328c
release cyd-jc2432w328c

build cyd-jc8048w550c
release cyd-jc8048w550c

releaseSdk release/TactilitySDK-esp32

build m5stack-core2
release m5stack-core2

build m5stack-cores3
release m5stack-cores3

duration=$SECONDS

echo "Finished in $((duration / 60)) minutes and $((duration % 60)) seconds."