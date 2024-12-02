# TODOs
- Publish firmwares with upload tool
- Bug: When closing a top level app, there's often an error "can't stop root app"
- Bug: I2C Scanner is on M5Stack devices is broken
- Create more unit tests for `tactility-core` and `tactility` (PC-only for now)
- Show a warning screen if firmware encryption or secure boot are off when saving WiFi credentials.
- Show a warning screen when a user plugs in the SD card on a device that only supports mounting at boot.
- Try out Waveshare S3 120MHz mode for PSRAM (see "enabling 120M PSRAM is necessary" in [docs](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3#Other_Notes))
- T-Deck has random sdcard SPI crashes due to sharing bus with screen SPI: make it use the LVGL lock for sdcard operations?
- Check service/app id on registration to see if it is a duplicate id
- Fix screenshot app on ESP32: it currently blocks when allocating memory
- Localisation of texts
- Portrait support for GPIO app
- App lifecycle docs mention on_create/on_destroy but app lifecycle is on_start/on_stop
- Explore LVGL9's FreeRTOS functionality
- Explore LVGL9's ILI93414 driver for 2.4" Yellow Board
- Bug: in LVGL9 with M5Core2, crash when bottom item is clicked without scrolling first
- Replace M5Unified and M5GFX with custom drivers (so we can fix the Core2 SD card mounting bug, and so we regain some firmware space)
- Commit fix to esp_lvgl_port to have esp_lvgl_port_disp.c user driver_data instead of user_data

# Core Ideas
- Support for displays with different DPI. Consider the layer-based system like on Android.
- If present, use LED to show boot status
- 2 wire speaker support
- tt::app::start() and similar functions as proxies for Loader app start/stop/etc.
- App.setResult() for apps that need to return data to other apps (e.g. file selection)
- Wi-Fi using dispatcher to dispatch its main functionality to the dedicated Wi-Fi CPU core (to avoid main loop hack)

# App Ideas
- Add FreeRTOS task manager functionality to System Info app
- BlueTooth keyboard app
- Chip 8 emulator
- BadUSB (in December 2024, TinyUSB has a bug where uninstalling and re-installing the driver fails)
- Discord bot
- IR transceiver app
- GPS app
- Investigate CSI https://stevenmhernandez.github.io/ESP32-CSI-Tool/
