# Architecture: Service Framework

Services are a C API (`service-module`, `<service/*.h>`), not a C++ class. Each service has a `ServiceManifest` (`id`, `create_service`/`destroy_service` for its custom data, `on_start`/`on_stop` callbacks) registered via `service_manager_add()`, and is started/stopped via `service_manager_start()`/`service_manager_stop()`. Services are long-running background processes (GUI, Wi-Fi, loader, statusbar, GPS, etc.).
