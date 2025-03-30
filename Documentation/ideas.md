# TODOs
- rename kernel::systemEventAddListener() etc to subscribe/unsubscribe
- Split up boot stages, so the last stage can be done from the splash screen
- Start using non_null (either via MS GSL, or custom)
- `hal/Configuration.h` defines C function types: Use C++ std::function instead
- Fix system time to not be 1980 (use build year as minimum)
- Use std::span or string_view in StringUtils https://youtu.be/FRkJCvHWdwQ?t=2754 
- Fix bug in T-Deck/etc: esp_lvgl_port settings has a large stack size (~9kB) to fix an issue where the T-Deck would get a stackoverflow. This sometimes happens when WiFi is auto-enabled and you open the app while it is still connecting.
- Clean up static_cast when casting to base class.
- Mutex: Implement give/take from ISR support (works only for non-recursive ones)
- Extend unPhone power driver: add charging status, usb connection status, etc.
- Expose app::Paths to TactilityC
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
- All drivers (e.g. display, touch, etc.) should call stop() in their destructor, or at least assert that they should not be running.
- Use GPS time to set/update the current time
- Investigate EEZ Studio
- Remove flex_flow from app_container in Gui.cpp

# Nice-to-haves
- Give external app a different icon. Allow an external app update their id, icon, type and name once they are running(and persist that info?). Loader will need to be able to find app by (external) location.
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
- Support more than 1 hardware keyboard (see lvgl::hardware_keyboard_set_indev()). LVGL init currently calls keyboard init, but that part should probably be done from the KeyboardDevice base class.
 
# App Ideas
- Map widget:
  https://github.com/portapack-mayhem/mayhem-firmware/blob/b66d8b1aa178d8a9cd06436fea788d5d58cb4c8d/firmware/application/ui/ui_geomap.cpp
  https://github.com/portapack-mayhem/mayhem-firmware/blob/b66d8b1aa178d8a9cd06436fea788d5d58cb4c8d/firmware/tools/generate_world_map.bin.py
  https://github.com/portapack-mayhem/mayhem-firmware/releases
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
- Calculator
- Text editor
- Todo list
- Calendar
