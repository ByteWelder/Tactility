#!/bin/sh  
if [[ -v ESP_IDF_VERSION ]]; then
  idf.py build
else
  cmake -S ./ -B build-sim
  make -C build-sim all
fi

