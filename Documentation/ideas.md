# Bugs
- I2C Scanner is on M5Stack devices is broken
- Fix screenshot app on ESP32: it currently blocks when allocating memory (its cmakelists.txt also needs a fix, see TODO in there)
- In LVGL9 with M5Core2, crash when bottom item is clicked without scrolling first
- Commit fix to esp_lvgl_port to have `esp_lvgl_port_disp.c` user driver_data instead of user_data
- WiFi bug: when pressing disconnect while between `WIFI_EVENT_STA_START` and `IP_EVENT_STA_GOT_IP`, then auto-connect becomes activate again.

# TODOs
- Rewrite `sdcard` HAL to class
- Attach ELF data to wrapper app (as app data) (check that app state is "running"!) so you can run more than 1 external apps at a time.
  We'll need to keep track of all manifest instances, so that the wrapper can look up the relevant manifest for the relevant callbacks.
- T-Deck: Clear screen before turning on blacklight
- Single wire audio
- Audio recording app
- Files app: file operations: rename, delete, copy, paste (long press?), create folder
- T-Deck: Use knob for UI selection
- Logging to disk/etc.
- Crash monitoring: Keep track of which system phase the app crashed in (e.g. which app in which state)
- AppContext's onResult should pass the app id (or launch request id!) that was started, so we can differentiate between multiple types of apps being launched
- Loader: Use main dispatcher instead of Thread, and move API to `tt::app::`
- Create more unit tests for `tactility-core` and `tactility` (PC-only for now)
- Show a warning screen if firmware encryption or secure boot are off when saving WiFi credentials.
- Show a warning screen when a user plugs in the SD card on a device that only supports mounting at boot.
- Localisation of texts (load in boot app from sd?)
- Portrait support for GPIO app
- Explore LVGL9's FreeRTOS functionality
- Explore LVGL9's ILI93414 driver for 2.4" Yellow Board
- Replace M5Unified and M5GFX with custom drivers (so we can fix the Core2 SD card mounting bug, and so we regain some firmware space)
- External app loading: Check version of Tactility and check ESP target hardware, to check for compatibility.
- Scanning SD card for external apps and auto-register them (in a temporary register?)
- tt::app::start() and similar functions as proxies for Loader app start/stop/etc.
- Support hot-plugging SD card

# Nice-to-haves
- T-Deck Plus: Create separate board config?
- Support for displays with different DPI. Consider the layer-based system like on Android.
- Make firmwares available via web serial website
- If present, use LED to show boot status
- T-Deck Power: capacity estimation uses linear voltage curve, but it should use some sort of battery discharge curve.
 
# App Ideas
- USB implementation to make device act as mass storage device.
- System logger
- BlueTooth keyboard app
- Chip 8 emulator
- BadUSB (in December 2024, TinyUSB has a bug where uninstalling and re-installing the driver fails)
- Discord bot
- IR transceiver app
- GPS app
- Investigate CSI https://stevenmhernandez.github.io/ESP32-CSI-Tool/
- Compile unix tools to ELF apps?
