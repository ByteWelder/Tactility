# TODOs

## Higher Priority

- Show a warning in the web installer when flashing CYD 28R board regarding v1/v2/v3
- Fix Development service: when no SD card is present, the app fails to install. Consider installing to `/data`
  Note: Change app install to "transfer file" functionality. We can have a proper install when we have app packaging.
  Note: Consider installation path option in interface
- External app loading: Check the version of Tactility and check ESP target hardware to check for compatibility.
- Make a URL handler. Use it for handling local files. Match file types with apps.
  Create some kind of "intent" handler like on Android.
  The intent can have an action (e.g. view), a URL and an optional bundle.
  The manifest can provide the intent handler
- Update ILI934x to v2.0.1
- Apps with update timer in onCreate() should check `lvgl::isStarted()`
- CrowPanel Basic 3.5": check why GraphicsDemo fails
- CrowPanel Basic 3.5": check why System Info doesn't show storage info
- Update to LVGL v9.3 stable
- Create `app::getSettingsPath()` to get paths to properties files by first trying sd card and then trying `/data`

## Medium Priority

- Improve EspLcdDisplay to contain all the standard configuration options, and implement a default init function. Add a configuration class.
- Statusbar icon that shows low/critical memory warnings
- Make WiFi setup app that starts an access point and hosts a webpage to set up the device.
  This will be useful for devices without a screen, a small screen or a non-touch screen.
- Unify the way displays are dimmed. Some implementations turn off the display when it's fully dimmed. Make this a separate functionality.
- Try out ILI9342 https://github.com/jbrilha/esp_lcd_ili9342
- All drivers (e.g. display, touch, etc.) should call stop() in their destructor, or at least assert that they should not be running.
- Bug: Turn on WiFi (when testing it wasn't connected/connecting - just active). Open chat. Observe crash.
- Bug: Crash handling app cannot be exited with an EncoderDevice. (current work-around is to manually reset the device)
- I2C app should show error when I2C port is disabled when the scan button was manually pressed

## Lower Priority

- Rename `filebrowser` to `files` and `FileBrowser.cpp` to `Files.cpp`
- Implement system suspend that turns off the screen
- The boot button on some devices can be used as GPIO_NUM_0 at runtime
- Localize all apps
- Support hot-plugging SD card (note: this is not possible if they require the CS pin hack)
- Explore LVGL9's FreeRTOS functionality
- CrashHandler: use "corrupted" flag
- CrashHandler: process other types of crashes (WDT?)
- Use GPS time to set/update the current time
- Fix bug in T-Deck/etc: esp_lvgl_port settings has a large stack size (~9kB) to fix an issue where the T-Deck would get a stackoverflow. This sometimes happens when WiFi is auto-enabled and you open the app while it is still connecting.
- Consider using non_null (either via MS GSL, or custom)
- Fix system time to not be 1980 (use build year as a minimum). Consider keeping track of the last known time.
- Use std::span or string_view in StringUtils https://youtu.be/FRkJCvHWdwQ?t=2754 
- Mutex: Implement give/take from ISR support (works only for non-recursive ones)
- Extend unPhone power driver: add charging status, usb connection status, etc.
- Clear screen before turning on blacklight (e.g. T-Deck, CYD 2432S028R, etc.)
- T-Deck: Use trackball as input device (with optional mouse functionality for LVGL)
- Show a warning screen if firmware encryption or secure boot are off when saving WiFi credentials.
- Remove flex_flow from app_container in Gui.cpp
- Files app: copy/cut/paste actions
- ElfAppManifest: change name (remove "manifest" as it's confusing), remove icon and title, publish snapshot SDK on CDN
- `UiScale` implementation for devices like the CYD 2432S032C
- Bug: CYD 2432S032C screen rotation fails due to touch driver issue
- Calculator app should show regular text input field on non-touch devices that have a keyboard (Cardputer, T-Lora Pager)

# Nice-to-haves

- Considering the lack of callstack debugging for external apps: allow for some debugging to be exposed during a device crash. Apps could report their state (e.g. an integer value) which can be stored during app operation and retrieve after crash. The same can be done for various OS apps and states. We can keep an array of these numbers to keep track of the last X states, to get an idea of what's going on.
- Audio player app
- Audio recording app
- OTA updates
- T-Deck Plus: Create separate board config?
- Support for displays with different DPI. Consider the layer-based system like on Android.
- If present, use LED to show boot/wifi status
- Capacity based on voltage: estimation for various devices uses a linear voltage curve, but it should use a battery discharge curve.
- Wrapper for lvgl slider widget that shows "+" and "-" buttons, and also the value in a label.
  Note: consider Spinbox
- On crash, try to save the current log to flash or SD card? (this is risky, though, so ask in Discord first)
- Support more than 1 hardware keyboard (see lvgl::hardware_keyboard_set_indev()). LVGL init currently calls keyboard init, but that part should probably be done from the KeyboardDevice base class.

# App Ideas

- Revisit TinyUSB mouse idea: the bugs related to cleanup seem to be fixed in the library.
- Map widget:
  https://github.com/portapack-mayhem/mayhem-firmware/blob/b66d8b1aa178d8a9cd06436fea788d5d58cb4c8d/firmware/application/ui/ui_geomap.cpp
  https://github.com/portapack-mayhem/mayhem-firmware/blob/b66d8b1aa178d8a9cd06436fea788d5d58cb4c8d/firmware/tools/generate_world_map.bin.py
  https://github.com/portapack-mayhem/mayhem-firmware/releases
- Weather app: https://lab.flipper.net/apps/flip_weather
- wget app: https://lab.flipper.net/apps/web_crawler (add profiles for known public APIs?)
- BlueTooth keyboard app
- Chip 8 emulator
- BadUSB (in December 2024, TinyUSB has a bug where uninstalling and re-installing the driver fails)
- Discord bot
- IR transceiver app
- GPS app
- Investigate CSI https://stevenmhernandez.github.io/ESP32-CSI-Tool/
- Compile unix tools to ELF apps?
- Todo list
- Calendar
- Display touch calibration
- RSS reader
- Static file web server (with option to specify path and port)
