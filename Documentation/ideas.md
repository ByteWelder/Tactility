# TODOs
- Loader: Use Timer instead of Thread, and move API to `tt::app::`
- Gpio: Use Timer instead of Thread
- I2cScannerThread: Use Timer instead of Thread
- Bug: I2C Scanner is on M5Stack devices is broken
- WiFi AP Connect app: add "Forget" option.
- T-Deck Plus: Implement battery status
- Make firmwares available via release process
- Make firmwares available via web serial website
- Bug: When closing a top level app, there's often an error "can't stop root app"
- Create more unit tests for `tactility-core` and `tactility` (PC-only for now)
- Show a warning screen if firmware encryption or secure boot are off when saving WiFi credentials.
- Show a warning screen when a user plugs in the SD card on a device that only supports mounting at boot.
- Check service/app id on registration to see if it is a duplicate id
- Fix screenshot app on ESP32: it currently blocks when allocating memory
- Localisation of texts (load in boot app from sd?)
- Portrait support for GPIO app
- Explore LVGL9's FreeRTOS functionality
- Explore LVGL9's ILI93414 driver for 2.4" Yellow Board
- Bug: in LVGL9 with M5Core2, crash when bottom item is clicked without scrolling first
- Replace M5Unified and M5GFX with custom drivers (so we can fix the Core2 SD card mounting bug, and so we regain some firmware space)
- Commit fix to esp_lvgl_port to have `esp_lvgl_port_disp.c` user driver_data instead of user_data
- Wifi bug: when pressing disconnect while between `WIFI_EVENT_STA_START` and `IP_EVENT_STA_GOT_IP`, then auto-connect becomes activate again.
- T-Deck Plus: Create separate board config
- External app loading: Check version of Tactility and check ESP target hardware, to check for compatibility.
- hal::Configuration: Replace CreateX fields with plain instances

# Core Ideas
- Support for displays with different DPI. Consider the layer-based system like on Android.
- If present, use LED to show boot status
- 2 wire speaker support
- tt::app::start() and similar functions as proxies for Loader app start/stop/etc.
- USB implementation to make device act as mass storage device.

# App Ideas
- System logger
- Add FreeRTOS task manager functionality to System Info app
- BlueTooth keyboard app
- Chip 8 emulator
- BadUSB (in December 2024, TinyUSB has a bug where uninstalling and re-installing the driver fails)
- Discord bot
- IR transceiver app
- GPS app
- Investigate CSI https://stevenmhernandez.github.io/ESP32-CSI-Tool/
