## Overview

NanoBake is a front-end application platform for ESP32.
It provides an application framework that is based on code from the [Flipper Zero](https://github.com/flipperdevices/flipperzero-firmware/) project.

Nanobake provides:
- A hardware abstraction layer
- UI capabilities (via LVGL)
- An application platform that can run apps and services

Requirements:
- ESP32 (any?)
- [esp-idf 5.1.x](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/get-started/index.html)
- a display (connected via SPI or I2C)

**Status: pre-alpha**

## Technologies

UI is created with [lvgl](https://github.com/lvgl/lvgl) via [esp_lvgl_port](https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port).

LCD and input drivers are based on [esp_lcd](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/lcd.html)
and [esp_lcd_touch](https://components.espressif.com/components/espressif/esp_lcd_touch).

## Supported Hardware

**NOTE**: `sdkconfig.defaults` currently contains `CONFIG_LV_COLOR_16_SWAP=y`. 
You might have to remove this setting if you're not using the Yellow Board described below.

### Devices

Predefined configurations are available for:
- Yellow Board: 2.4" with capacitive touch (2432S024) (sources: AliExpress [1](https://www.aliexpress.com/item/1005005902429049.html), [2](https://www.aliexpress.com/item/1005005865107357.html))
- LilyGo T-Deck
- (more will follow)

Other configurations can be supported, but they require you to set up the drivers yourself:

- Display drivers: [esp-bsp](https://github.com/espressif/esp-bsp/blob/master/LCD.md) and [Espressif Registry](https://components.espressif.com/components?q=esp_lcd).
- Touch drivers: [Espressif Registry](https://components.espressif.com/components?q=esp_lcd_touch).

## Guide

Until there is proper documentation, here are some pointers:
- Sample application: [bootstrap](main/src/main.c) and [app](main/src/hello_world/hello_world.c)
- [NanoBake](./components/nanobake/inc)
- [Furi](./components/furi/src)

## Building Firmware

First we have to select the correct device:

1. If you use CLion, close it and delete the `cmake-build-debug` folder.
2. If you have a `build` folder, then delete it or run `idf.py fullclean`
3. Copy the `sdkconfig.board.YOUR_BOARD` into `sdkconfig`

Now you can run `idf.py flash monitor`

## License

[GNU General Public License Version 3](LICENSE.md)

