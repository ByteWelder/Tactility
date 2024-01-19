#!/bin/sh  
if [[ -v ESP_IDF_VERSION ]]; then
  idf.py build
else
  cmake -S ./ -B build
  make -C build all
fi

