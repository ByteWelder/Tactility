#!/bin/sh

#
# Usage: build.sh [boardname]
# Example: build.sh lilygo-tdeck
# Description: Makes a clean build for the specified board.
#

echoNewPhase() {
    echo -e "⏳ \e[36m${1}\e[0m"
}

fatalError() {
    echo -e "⚠️ \e[31m${1}\e[0m"
    exit 0
}

sdkconfig_file="sdkconfig.board.${1}"

if [ $# -lt 1 ]; then
    fatalError "Must pass board name as first argument. (e.g. lilygo_tdeck)"
fi

if [ ! -f $sdkconfig_file ]; then
    fatalError "Board not found: ${sdkconfig_file}"
fi

echoNewPhase "Cleaning build folder"

#rm -rf build

echoNewPhase "Building $sdkconfig_file"

cp $sdkconfig_file sdkconfig
idf.py build
if [[ $? != 0 ]]; then
    fatalError "Failed to build esp32s3 SDK"
fi

