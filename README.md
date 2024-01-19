## Overview

Tactility is a front-end application platform for ESP32. It is mainly intended for touchscreen devices.
It provides an application framework that is based on code from the [Flipper Zero](https://github.com/flipperdevices/flipperzero-firmware/) project.

Tactility provides:
- A hardware abstraction layer
- UI capabilities (via LVGL)
- An application platform that can run apps and services

Requirements:
- ESP32 (any?)
- [esp-idf 5.1.2](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/get-started/index.html) or a newer v5.1.x
- a display (connected via SPI or I2C)

**Status: Alpha**

## Technologies

UI is created with [lvgl](https://github.com/lvgl/lvgl) via [esp_lvgl_port](https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port).

LCD and input drivers are based on [esp_lcd](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/lcd.html)
and [esp_lcd_touch](https://components.espressif.com/components/espressif/esp_lcd_touch).

## Supported Hardware

### Devices

Predefined configurations are available for:
- Yellow Board: 2.4" with capacitive touch (2432S024C) (see AliExpress [1](https://www.aliexpress.com/item/1005005902429049.html), [2](https://www.aliexpress.com/item/1005005865107357.html))
- LilyGo T-Deck (see [lilygo.cc](https://www.lilygo.cc/products/t-deck), [AliExpress](https://www.aliexpress.com/item/1005005692235592.html))
- (more will follow)

Other configurations can be supported, but they require you to set up the drivers yourself:

- Display drivers: [esp-bsp](https://github.com/espressif/esp-bsp/blob/master/LCD.md) and [Espressif Registry](https://components.espressif.com/components?q=esp_lcd).
- Touch drivers: [Espressif Registry](https://components.espressif.com/components?q=esp_lcd_touch).

## Guide

Building can be done either for the ESP-IDF target or for PC:

`./build.sh` - build the ESP-IDF or the PC version of Tactility (*)
`./run.sh` - Does `flash` and `monitor` for ESP-IDF and simply builds and starts it for PC

The build scripts will detect if ESP-IDF is available. They will know if you ran `${IDF_PATH}/export.sh`.

Until there is proper documentation, here are some pointers:
- Sample application: [bootstrap](main/src/main.c) and [app](main/src/hello_world/hello_world.c)
- [Tactility](./components/tactility): The main platform with default services and apps.
- [Tactility Core](./libs/tactility-core): The core platform code.

## Building Firmware

First we have to select the correct device:

1. If you use CLion, close it and delete the `cmake-build-debug` folder.
2. Run `idf.py fullclean` to remove any cache from previous builds (or delete `build` folder manually)
3. Copy the `sdkconfig.board.YOUR_BOARD` into `sdkconfig`. Use `sdkconfig.defaults` if you are setting up a custom board.

Now you can run `idf.py flash monitor`

## License

[GNU General Public License Version 3](LICENSE.md)

