# TODOs

## Higher Priority

- Fix Development service: when no SD card is present, the app fails to install. Consider installing to `/data`
  Note: Change app install to "transfer file" functionality. We can have a proper install when we have app packaging.
  Note: Consider installation path option in interface
- External app loading: Check the version of Tactility and check ESP target hardware to check for compatibility.
- Make a URL handler. Use it for handling local files. Match file types with apps.
  Create some kind of "intent" handler like on Android.
  The intent can have an action (e.g. view), a URL and an optional bundle.
  The manifest can provide the intent handler
- Bug: GraphicsDemo should check if display supports the DisplayDriver interface (and same for touch) and show an AlertDialog error if there's a problem
- Update ILI934x to v2.0.1
- App packaging
- Create an "app install paths" settings app to add/remove paths.
  Scan these paths on startup.
  Make the AppList use the scan results.

## Medium Priority

- Unify the way displays are dimmed. Some implementations turn off the display when it's fully dimmed. Make this a separate functionality.
- Try out ILI9342 https://github.com/jbrilha/esp_lcd_ili9342
- All drivers (e.g. display, touch, etc.) should call stop() in their destructor, or at least assert that they should not be running.
- Create different partition files for different ESP flash size targets (N4, N8, N16, N32)
  Consider a dev variant for quick flashing.
- Bug: Turn on WiFi (when testing it wasn't connected/connecting - just active). Open chat. Observe crash.

## Lower Priority

- Localize all apps
- Support hot-plugging SD card (note: this is not possible if they require the CS pin hack)
- Create more unit tests for `tactility`
- Explore LVGL9's FreeRTOS functionality
- CrashHandler: use "corrupted" flag
- CrashHandler: process other types of crashes (WDT?)
- Add a Keyboard setting in `keyboard.properties` to override the behaviour of soft keyboard hiding (e.g. keyboard hardware is present, but the user wants to use a soft keyboard)
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

# Nice-to-haves

- Considering the lack of callstack debugging for external apps: allow for some debugging to be exposed during a device crash. Apps could report their state (e.g. an integer value) which can be stored during app operation and retrieve after crash. The same can be done for various OS apps and states. We can keep an array of these numbers to keep track of the last X states, to get an idea of what's going on.
- Give external app a different icon. Allow an external app to update their id, icon, type and name once they are running (and persist that info?). Loader will need to be able to find app by (external) location.
- Audio player app
- Audio recording app
- OTA updates
- T-Deck Plus: Create separate board config?
- Support for displays with different DPI. Consider the layer-based system like on Android.
- If present, use LED to show boot/wifi status
- Capacity based on voltage: estimation for various devices uses a linear voltage curve, but it should use some sort of battery discharge curve.
- Statusbar widget to show how much memory is in use?
- Wrapper for lvgl slider widget that shows "+" and "-" buttons, and also the value in a label.
- Files app: copy/paste actions
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
