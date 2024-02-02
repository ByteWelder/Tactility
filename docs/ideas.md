# TODOs
- Update `view_port` to use `ViewPort` as handle externally and `ViewPortData` internally
- Replace FreeRTOS semaphore from `Loader` with internal `Mutex`
- Create more unit tests for `tactility-core` and `tactility` (PC-only for now)
- Have a way to deinit LVGL drivers that are created from `HardwareConfig`
- Show a warning screen if firmware encryption or secure boot are off when saving WiFi credentials.
- Show a warning screen when a user plugs in the SD card on a device that only supports mounting at boot.
- Try out Waveshare S3 120MHz mode for PSRAM (see "enabling 120M PSRAM is necessary" in [docs](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3#Other_Notes))
- Fix for dark theme: the wifi icons should use the colour of the theme (they remain black when dark theme is set)
 
# Core Ideas
- Make a HAL? It would mainly be there to support PC development. It's a lot of effort for supporting what's effectively a dev-only feature.
- Support for displays with different DPI. Consider the layer-based system like on Android.
- Display orientation support for Display app
- If present, use LED to show boot status

# App Improvement Ideas
- Sort desktop apps by name.
- Light/dark mode selection in Display settings app.

# App Ideas
- Chip 8 emulator
- Discord bot
- BadUSB
- IR transceiver app
- GPIO status viewer
- BlueTooth keyboard app
- Investigate CSI https://stevenmhernandez.github.io/ESP32-CSI-Tool/