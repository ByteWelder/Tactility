# Key Conventions

- `#ifdef ESP_PLATFORM` guards ESP32-specific code; the simulator uses POSIX equivalents.
- The `Drivers/` directory contains hardware drivers (display controllers, touch controllers, PMICs, etc.) — each is its own CMake component.
- `Modules/` contains cross-cutting modules. e.g.`lvgl-module` (LVGL task management).
- `Data/system/` and `Data/data/` are flashed as FAT filesystem images on ESP32.
- Translations are in `Translations/` as CSV files, generated via `generate.py`.
