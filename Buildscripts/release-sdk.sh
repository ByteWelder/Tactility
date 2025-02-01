#!/bin/sh

#
# Usage: release-sdk.sh [target_path]
# Example: release.sh release/TactilitySDK
# Description: Releases the current build files as an SDK in the specified folder.
#

target_path=$1

mkdir -p $target_path

build_dir=`pwd`
library_path=$target_path/Libraries

cp version.txt $target_path

# TactilityC
tactility_library_path=$library_path/TactilityC
mkdir -p $tactility_library_path/Binary
cp build/esp-idf/TactilityC/libTactilityC.a $tactility_library_path/Binary/
mkdir -p $tactility_library_path/Include
find_target_dir=$build_dir/$tactility_library_path/Include/
cp TactilityC/Include/* $find_target_dir
cd -
cp Documentation/license-tactilitysdk.md $build_dir/$tactility_library_path/LICENSE.md

# lvgl
lvgl_library_path=$library_path/lvgl
mkdir -p $lvgl_library_path/Binary
mkdir -p $lvgl_library_path/Include
cp build/esp-idf/lvgl/liblvgl.a $lvgl_library_path/Binary/
find_target_dir=$build_dir/$lvgl_library_path/Include/
cd Libraries/lvgl
find src/ -name '*.h' | cpio -pdm $find_target_dir
cd -
cp Libraries/lvgl/lvgl.h $find_target_dir
cp Libraries/lvgl/lv_version.h $find_target_dir
cp Libraries/lvgl/LICENCE.txt $lvgl_library_path/LICENSE.txt
cp Libraries/lvgl_conf/lv_conf_kconfig.h $lvgl_library_path/Include/lv_conf.h

# elf_loader
elf_loader_library_path=$library_path/elf_loader
mkdir -p $elf_loader_library_path
cp Libraries/elf_loader/elf_loader.cmake $elf_loader_library_path/
cp Libraries/elf_loader/license.txt $elf_loader_library_path/

cp Buildscripts/CMake/TactilitySDK.cmake $target_path/
cp Buildscripts/CMake/CMakeLists.txt $target_path/
echo -n $ESP_IDF_VERSION >> $target_path/idf-version.txt
