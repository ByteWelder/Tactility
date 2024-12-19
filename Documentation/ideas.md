# Bugs
- I2C Scanner is on M5Stack devices is broken
- Fix screenshot app on ESP32: it currently blocks when allocating memory (its cmakelists.txt also needs a fix, see TODO in there)
- WiFi bug: when pressing disconnect while between `WIFI_EVENT_STA_START` and `IP_EVENT_STA_GOT_IP`, then auto-connect becomes active again.
- ESP32 (CYD) memory issues (or any device without PSRAM):
  - Boot app doesn't show logo 
  - WiFi is on and navigating back to Desktop makes desktop icons disappear
  - WiFi might fail quiety when trying to enable it: this shows no feedback (force it by increasing LVGL buffers to 100kB)
  Possible mitigations: 
  - When no PSRAM is available, use simplified desktop buttons
  - Add statusbar icon for memory pressure.
  - Show error in WiFi screen (e.g. AlertDialog when SPI is not enabled and available memory is below a certain amount)
- WiFi details "forget" button should be hidden when WiFi credentials are not stores yet.
- Clean up static_cast when casting to base class.

# TODOs
- Call tt::lvgl::isSyncSet after HAL init and show error (and crash?) when it is not set.
- Create different partitions files for different ESP flash size targets (N4, N8, N16, N32)
- Rewrite `sdcard` HAL to class
- Attach ELF data to wrapper app (as app data) (check that app state is "running"!) so you can run more than 1 external apps at a time.
  We'll need to keep track of all manifest instances, so that the wrapper can look up the relevant manifest for the relevant callbacks.
- T-Deck: Clear screen before turning on blacklight
- Audio player app
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
- Explore LVGL9's FreeRTOS functionality
- Replace M5Unified and M5GFX with custom drivers (so we can fix the Core2 SD card mounting bug, and so we regain some firmware space)
- External app loading: Check version of Tactility and check ESP target hardware, to check for compatibility.
- Scanning SD card for external apps and auto-register them (in a temporary register?)
- tt::app::start() and similar functions as proxies for Loader app start/stop/etc.
- Support hot-plugging SD card

# Nice-to-haves
- T-Deck Plus: Create separate board config?
- Support for displays with different DPI. Consider the layer-based system like on Android.
- Make firmwares available via web serial website
- If present, use LED to show boot/wifi status
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
