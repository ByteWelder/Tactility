# app-hub-module

App Hub service (`tactility.apphub`): finds, installs and removes apps from the online app catalog.
The C API is in `<app_hub/app_hub.h>`.

- `app_hub_search()` lists the catalog apps that are compatible with the device, merged with all installed apps.
- `app_hub_refresh()`, `app_hub_install()`, `app_hub_update_all()` and `app_hub_remove()` queue an operation. Operations run one at a time
  on a background task, which stops after 5 seconds without operations and starts again for the next one.
- Subscribe with `app_hub_event_subscribe()`, then wait with `app_hub_event_await()` and read with `app_hub_event_poll()`.
  The events are `REFRESHING`/`REFRESHED`, `INSTALLING`/`INSTALLED`, `REMOVING`/`REMOVED` and `UPDATING`/`UPDATED`, with the app id and the result.

The `apphub` shell command uses the same API: `refresh`, `search [query]`, `updates`, `info <id>`, `install <id>`, `update` and `remove <id>`.

The catalog is stored in the service's user data directory, so it's available offline after the first refresh.

License: [Apache v2.0](LICENSE-Apache-2.0.md)
