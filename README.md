## Overview

NanoBake is a front-end application platform for ESP32. It provides an application framework that is based on code from the [Flipper Zero](https://github.com/flipperdevices/flipperzero-firmware/) project.

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

LCD and input drivers are based on [esp_lcd](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/lcd.html)
and [esp_lcd_touch](https://components.espressif.com/components/espressif/esp_lcd_touch).

UI is created with [lvgl](https://github.com/lvgl/lvgl) via [esp_lvgl_port](https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port).

## Supported Hardware

**NOTE**: `sdkconfig.defaults` currently contains `CONFIG_LV_COLOR_16_SWAP=y`. 
You might have to remove this setting if you're not using the Yellow Board described below.

### Devices

See below for the supported hardware.
Predefined configurations are available for:
- Yellow Board / 2432S024 (capacitive touch variant)
- (more will follow)

### Drivers

**Displays** (see [esp-bsp](https://github.com/espressif/esp-bsp/blob/master/LCD.md) and [Espressif Registry](https://components.espressif.com/components?q=esp_lcd)):
- GC9503
- GC9A01
- ILI9341
- RA8875
- RM68120
- SH1107
- SSD1306
- SSD1963
- ST7262E43
- ST7789
- ST7796 
 
**Touch** (see [Espressif Registry](https://components.espressif.com/components?q=esp_lcd_touch)):
- CST8xx
- FT5X06
- GT1151
- GT911
- STMPE610
- TT2100

## Guide

Until there is proper documentation, here are some pointers:
- Sample application: [bootstrap](main/src/main.c) and [app](main/src/hello_world/hello_world.c)
- [NanoBake](./components/nanobake/inc)
- [Furi](./components/furi/src)

## License

[GNU General Public License Version 3](LICENSE.md)

