#!/bin/sh
if [[ -v ESP_IDF_VERSION ]]; then
  idf.py flash monitor $@
else
  cmake -S ./ -B build
  make -C build all
  build/main_sim/main_sim
fi

