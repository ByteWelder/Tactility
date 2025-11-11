# Get Started

See [https://docs.tactility.one]

# Useful Parameters

You can copy these into `sdkconfig` manually or set them via `idf.py menuconfig`

## Enable FPS

```properties
CONFIG_LV_USE_OBSERVER=y
CONFIG_LV_USE_PERF_MONITOR=y
```

## Halt on error

```properties
CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT=y
# CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT is not set
```

## LVGL

```properties
CONFIG_STACK_CHECK_STRONG=y

CONFIG_LV_USE_SYSMON=y
CONFIG_LV_USE_OBSERVER=y
CONFIG_LV_USE_PERF_MONITOR=y

CONFIG_LV_USE_REFR_DEBUG=y
CONFIG_LV_USE_LAYER_DEBUG=y
```