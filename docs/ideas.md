# TODOs
- Make `desktop` app listen to changes in `app_manifest_registry`
- Update `view_port` to use `ViewPort` as handle externally and `ViewPortData` internally
- Replace FreeRTOS semaphore from `Loader` with internal `Mutex`
- Create unit tests for `tactility-core` and `tactility` (PC-only for now)
- Have a way to deinit LVGL drivers that are created from `HardwareConfig`
 
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