#!/bin/sh  
if [[ -v ESP_IDF_VERSION ]]; then
  idf.py build
else
  cmake -S ./ -B build-sim
  cmake --build build-sim -j 12
fi

