# TODOs
- Update `view_port` to use `ViewPort` as handle externally and `ViewPortData` internally
- Replace FreeRTOS semaphore from `Loader` with internal `Mutex`
- Create unit tests for `tactility-core` and `tactility` (PC-only for now)
- Have a way to deinit LVGL drivers that are created from `HardwareConfig`
- Thread is broken: `tt_thread_join()` always hangs because `tt_thread_cleanup_tcb_event()`
is not automatically called. This is normally done by a hook in `FreeRTOSConfig.h`
but that seems to not work with ESP32. I should investigate task cleanup hooks further.
- Set DPI in sdkconfig for Waveshare display
 
# Core Ideas
- Make a HAL? It would mainly be there to support PC development. It's a lot of effort for supporting what's effectively a dev-only feature.
- Support for displays with different DPI. Consider the layer-based system like on Android.

# App Ideas
- Chip 8 emulator
- Discord bot
- BadUSB
- IR transceiver app
- GPIO status viewer
- BlueTooth keyboard app
- Investigate CSI https://stevenmhernandez.github.io/ESP32-CSI-Tool/