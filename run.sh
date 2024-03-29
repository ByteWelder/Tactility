#!/bin/sh
if [[ -v ESP_IDF_VERSION ]]; then
  idf.py flash monitor $@
else
  set -e
  cmake -S ./ -B build-sim
  cmake --build build-sim -j 12
  cd data
  ../build-sim/app-sim/app-sim
  cd -
fi

