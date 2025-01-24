# Issues
- WiFi bug: when pressing disconnect while between `WIFI_EVENT_STA_START` and `IP_EVENT_STA_GOT_IP`, then auto-connect becomes active again.
- ESP32 (CYD) memory issues (or any device without PSRAM):
  - Boot app doesn't show logo 
  - WiFi is on and navigating back to Desktop makes desktop icons disappear
  - WiFi might fail quietly when trying to enable it: this shows no feedback (force it by increasing LVGL buffers to 100kB)
  Possible mitigations: 
  - When no PSRAM is available, use simplified desktop buttons
  - Add statusbar icon for memory pressure.
  - Show error in WiFi screen (e.g. AlertDialog when SPI is not enabled and available memory is below a certain amount)
- Clean up static_cast when casting to base class.
- EventFlag: Fix return value of set/get/wait (the errors are weirdly mixed in)
- Fix bug in T-Deck/etc: esp_lvgl_port settings has a large stack size (~9kB) to fix an issue where the T-Deck would get a stackoverflow. This sometimes happens when WiFi is auto-enabled and you open the app while it is still connecting.
- M5Stack Core only shows 4MB of SPIRAM in use

# TODOs
- Extend unPhone power driver: add charging status, usb connection status, etc.
- Expose app::Paths to TactilityC
- Refactor ServiceManifest into C++ class-based design like the App class
- Experiment with what happens when using C++ code in an external app (without using standard library!)
- Boards' CMakeLists.txt manually declare each source folder. Update them all to do a recursive search of all folders.
- We currently build all boards for a given platform (e.g. ESP32S3), but it's better to filter all irrelevant ones based on the Kconfig board settings:
  Projects will load and compile faster as it won't compile all the dependencies of all these other boards
- Make a ledger for setting CPU affinity of various services and tasks
- Boot hooks instead of a single boot method in config. Define different boot phases/levels in enum.
- Add toggle to Display app for sysmon overlay: https://docs.lvgl.io/master/API/others/sysmon/index.html
- CrashHandler: use "corrupted" flag
- CrashHandler: process other types of crashes (WDT?)
- Call tt::lvgl::isSyncSet after HAL init and show error (and crash?) when it is not set.
- Create different partitions files for different ESP flash size targets (N4, N8, N16, N32)
- T-Deck: Clear screen before turning on blacklight
- T-Deck: Use knob for UI selection?
- Crash monitoring: Keep track of which system phase the app crashed in (e.g. which app in which state)
- App::onResult should pass the app id (or launch request id!) that was started, so we can differentiate between multiple types of apps being launched
- Create more unit tests for `tactility-core` and `tactility` (PC-only for now)
- Show a warning screen if firmware encryption or secure boot are off when saving WiFi credentials.
- Show a warning screen when a user plugs in the SD card on a device that only supports mounting at boot.
- Localisation of texts (load in boot app from sd?)
- Explore LVGL9's FreeRTOS functionality
- External app loading: Check version of Tactility and check ESP target hardware, to check for compatibility.
- Scanning SD card for external apps and auto-register them (in a temporary register?)
- Support hot-plugging SD card

# Nice-to-haves
- CoreS3 has a hardware issue that prevents mounting SD cards while using the display too: allow USB Mass Storage to use `/data` instead? Perhaps give the USB settings app a drop down to select the root filesystem to attach.
- Give external app a different icon. Allow an external app update their id, icon, type and name once they are running(, and persist that info?). Loader will need to be able to find app by (external) location.
- Audio player app
- Audio recording app
- OTA updates
- T-Deck Plus: Create separate board config?
- Support for displays with different DPI. Consider the layer-based system like on Android.
- If present, use LED to show boot/wifi status
- Capacity based on voltage: estimation for various devices uses linear voltage curve, but it should use some sort of battery discharge curve.
- Statusbar widget to show how much memory is in use?
- Wrapper for Slider that shows "+" and "-" buttons, and also the value in a label.
- Display app: Add toggle to display performance measurement overlay (consider showing FPS in statusbar!)
- Files app: copy/paste actions
- On crash, try to save current log to flash or SD card? (this is risky, though, so ask in Discord first)
 
# App Ideas
- Weather app: https://lab.flipper.net/apps/flip_weather
- wget app: https://lab.flipper.net/apps/web_crawler (add profiles for known public APIs?)
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
