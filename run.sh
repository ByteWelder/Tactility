#!/bin/sh
if [[ -v ESP_IDF_VERSION ]]; then
  idf.py flash monitor $@
else
  cmake -S ./ -B build-sim
  make -C build-sim all
  build-sim/app-sim/app-sim
fi

