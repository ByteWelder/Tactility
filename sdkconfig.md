# Get Started

Copy the relevant `sdkconfig.board.*` file into `sdkconfig`.
This will apply the relevant settings to build the project for your hardware.

# Useful Parameters

## Enable FPS

```
CONFIG_LV_USE_OBSERVER=y
CONFIG_LV_USE_PERF_MONITOR=y
```

## Halt on error

```
CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT=y
# CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT is not set
```
