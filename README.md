## Overview

Tactility is a front-end application platform for ESP32. It is mainly intended for touchscreen devices.
It provides an application framework that is based on code from the [Flipper Zero](https://github.com/flipperdevices/flipperzero-firmware/) project.

**Status: Alpha**

![Tactility shown on a Lilygo T-Deck device and on PC](docs/pics/tactility-showcase.jpg)

Tactility features:
- A hardware abstraction layer
- UI capabilities (via LVGL)
- An application platform that can run apps and services
- PC app support to speed up development for ESP32 apps

Requirements:
- ESP32 (any?) with a display that has touch capability
- [esp-idf 5.1.2](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/get-started/index.html) or a newer v5.1.x

## Technologies

UI is created with [lvgl](https://github.com/lvgl/lvgl).
Any LVGL-capable device is supported.

In general, [esp_lvgl_port](https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port)
is the preferred solution if it supports your hardware:
Those LCD and input drivers are based on [esp_lcd](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/lcd.html)
and [esp_lcd_touch](https://components.espressif.com/components/espressif/esp_lcd_touch).
They are generally available via the Espressif Registry: [here](https://components.espressif.com/components?q=esp_lcd)
and [here](https://components.espressif.com/components?q=esp_lcd_touch)

## Supported Hardware

### Devices

Most hardware configurations should work, but it might require you to set up the drivers yourself.

Predefined configurations are available for:

| Device                                   | Screen&Touch | SD card | Other    |
|------------------------------------------|--------------|---------|----------|
| [LilyGo T-Deck][tdeck]                   | ✅            | ✅       | Keyboard | 
| [Waveshare S3 Touch][waveshare_s3_touch] | ✅            | ⏳       |          |
| Yellow Board 2432S024C (\*)              | ✅            | ✅       |          |

- ✅: Capable and implemented
- ⏳: Capable but not yet implemented
- ❌: Not capable

(*) Note: Only the capacitive version is supported. See AliExpress [here][2432s024c_1] and [here][2432s024c_2].

[tdeck]: https://www.lilygo.cc/products/t-deck
[waveshare_s3_touch]: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3
[2432s024c_1]: https://www.aliexpress.com/item/1005005902429049.html
[2432s024c_2]: https://www.aliexpress.com/item/1005005865107357.html

## Guide

### Build environment setup

Ensure you have [esp-idf 5.1.2](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/get-started/index.html) installed, then select the correct device:

Copy the `sdkconfig.board.YOUR_BOARD` into `sdkconfig`. Use `sdkconfig.defaults` if you are setting up a custom board.

### Building firmware

You can run `idf.py flash monitor`, but there are some helpers available too:

`./build.sh` - build the ESP-IDF or the PC version of Tactility (*)
`./build.sh -p /dev/ttyACM0` - optional: you can pass on extra parameters for esp-idf builds
`./run.sh` - Does `flash` and `monitor` for ESP-IDF and simply builds and starts it for PC

The build scripts will detect if ESP-IDF is available. They will adapter if you ran `${IDF_PATH}/export.sh`.

### Development

Directories explained:

- `app-esp`: The ESP32 application example
- `app-sim`: The PC/simulator application example
- `boards`: Contains ESP modules with drivers
- `tactility`: The main application platform code ([src/](./tactility/src))
- `tactility-esp`: ESP-specific application code (e.g. wifi app and service)
- `tactility-core`: Core functionality regarding threads, stdlib, etc. ([src/](./tactility-core/src))
- `libs`: Contains a mix of regular libraries and ESP modules

Until there is proper documentation, here are some pointers:

## License

[GNU General Public License Version 3](LICENSE.md)

