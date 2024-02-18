# TODOs
- Update `view_port` to use `ViewPort` as handle externally and `ViewPortData` internally
- Create more unit tests for `tactility-core` and `tactility` (PC-only for now)
- Have a way to deinit LVGL drivers that are created from `HardwareConfig`
- Show a warning screen if firmware encryption or secure boot are off when saving WiFi credentials.
- Show a warning screen when a user plugs in the SD card on a device that only supports mounting at boot.
- Try out Waveshare S3 120MHz mode for PSRAM (see "enabling 120M PSRAM is necessary" in [docs](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3#Other_Notes))
- T-Deck has random sdcard SPI crashes due to sharing bus with screen SPI: make it use the LVGL lock for sdcard operations?
- Wi-Fi connect app should show info about connection result
- Check service/app id on registration to see if it is a duplicate id
- Fix screenshot app on ESP32: it currently blocks when allocating memory
- Localisation of texts
- Portrait support for GPIO app
- App lifecycle docs mention on_create/on_destroy but app lifecycle is on_start/on_stop
- Explore LVGL9's FreeRTOS functionality
- Explore LVGL9's ILI93414 driver for 2.4" Yellow Board
 
# Core Ideas
- Make a HAL? It would mainly be there to support PC development. It's a lot of effort for supporting what's effectively a dev-only feature.
- Support for displays with different DPI. Consider the layer-based system like on Android.
- If present, use LED to show boot status
- 2 wire speaker support
- tt_app_start() and similar functions as proxies for Loader app start/stop/etc.
- tt_app_set_result() for apps that need to return data to other apps (e.g. file selection)
- Introduce co-routines for calling wifi/lvgl/etc functionality.

# App Improvement Ideas
- Sort desktop apps by name.
- Light/dark mode selection in Display settings app.

# App Ideas
- File viewer (images, text... binary?)
- GPIO status viewer
- BlueTooth keyboard app
- Chip 8 emulator
- BadUSB
- Discord bot
- IR transceiver app
- Investigate CSI https://stevenmhernandez.github.io/ESP32-CSI-Tool/