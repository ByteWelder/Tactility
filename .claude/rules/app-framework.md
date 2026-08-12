# Architecture: App Framework

Apps are event-driven, C API (`app-module`, `<app/*.h>`), not a C++ class. Each app has an `AppManifest` (`id`, `name`, `category`, `location`, `flags`) and a `main(app_instance_id, argc, argv)` entry point (`AppMainFn`), modelled on a C program's `main()`. Every app instance gets its own dedicated task for its whole lifetime, and blocks in that task until it returns.

Lifecycle and inter-app communication go through `app_manager_*()` (`app/manager.h`) and `app_event_*()` (`app/event.h`):
- `app_manager_start()`/`app_manager_start_with_parameters()` launch a plain instance; `app_manager_start_for_result()` launches a modal child that reports back to a parent instance.
- An app subscribes with `app_event_subscribe()`/`app_event_await()` and reacts to `APP_EVENT_CLOSE` (terminate now) and `APP_EVENT_RESULT` (a child it started reported back).
- An app closes itself by calling `app_manager_finish()` right before returning from `main()`; another instance is closed via `app_manager_stop()`.

Apps are registered at startup via `app_manager_add()`. External apps can be loaded from SD card via `manifest.properties` files, or side-loaded as ELF binaries on ESP32 (see `app/loader.h`'s `AppLoaderApi`).

Apps can be loaded from:

- memory (`APP_LOCATION_MEMORY`)
- a path pointing to an install folder where an `.app` file was installed (`APP_LOCATION_PATH`)
- a path pointing to an `.elf` file (`APP_LOCATION_PATH`)

An app can build an optional UI via the LVGL window-manager module (see `lvgl.md`).
