# Architecture: Build System

The `tactility_add_module()` CMake macro (in `Buildscripts/module.cmake`) wraps ESP-IDF's `idf_component_register` on ESP32 and standard `add_library` on POSIX, allowing the same source to build for both targets.

`device.py` reads `Devices/<id>/device.properties` and generates the `sdkconfig` file with all necessary ESP-IDF config (target chip, flash size, SPIRAM, LVGL fonts, Bluetooth, USB, etc.).
