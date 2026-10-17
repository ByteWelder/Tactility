# Mesh module

Mesh networking service (`tactility.mesh`) with pluggable protocol backends, such as `mesh-meshcore-module` and
`mesh-meshtastic-module`. The app SDK only exposes `<mesh/mesh.h>`. Backends implement `<mesh/backend.h>`.

- One backend is active at a time. It owns the radio through the kernel's advisory lease (`radio_acquire()`), and
  provides the radio profile (modulation, frequency, sync word, etc.) for the radio's regulatory region.
- A backend can offer named radio presets (`mesh_set_preset()`). The selection is persisted and resets to the
  backend's default when another backend is selected.
- The radio stays off until a region is set with `radio_set_region()`. Tactility persists the region and applies it
  at boot (see `Tactility/settings/RadioSettings.h`).
- Messages are stored durably in an append-only log of segments in the service's data directory. Records are
  buffered in RAM (PSRAM when available) and flushed every 30 seconds or when the buffer is mostly full. The oldest
  segment is deleted when the log exceeds its budget. The index is rebuilt at boot.
- Outgoing messages of a backend that is not active become "blocked", and are sent when it becomes active again.
- The service enforces the duty cycle of the region (e.g. 1% or 10% in Europe) over a sliding hour, and turns on the
  radio's listen-before-talk.
- All backend calls run on the service thread. Apps get events with the same subscribe/poll pattern as the kernel's
  radio events.

The service only starts when enabled with `mesh_set_enabled()`.
