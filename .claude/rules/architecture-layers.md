# Architecture: Layer Stack (bottom to top)

- **TactilityKernel** — C API kernel: device/driver/module lifecycle, concurrency primitives (thread, mutex, timer, dispatcher), filesystem, logging. Header convention: `<tactility/*.h>` (lowercase snake_case).
- **TactilityFreeRtos** — Thin C++ wrappers around FreeRTOS primitives.
- **Tactility** — Main OS layer: app framework, service framework, LVGL integration, networking and services (Wi-Fi, BLE, NTP, ESP-NOW), settings, i18n.
- **TactilityC** — C bindings (`tt_*.h`) for Tactility, used by side-loaded ELF apps on ESP32. Deprecated, replaced by TactilityKernel.
- **Firmware** — Entry point (`app_main`).
