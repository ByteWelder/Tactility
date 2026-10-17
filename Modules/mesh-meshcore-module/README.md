# MeshCore backend

[MeshCore](https://github.com/meshcore-dev/MeshCore) protocol backend for the mesh service (`mesh-module`).
It registers itself with `mesh_backend_register()` as `meshcore`.

Supported:

- Identity: an Ed25519 key pair created on first start and stored in the backend's data directory.
- Adverts: sent at start and every 12 hours, received adverts are verified and remembered as contacts.
- Channel messages (`GRP_TXT`): the "Public" channel is preconfigured, more channels can be added with
  `mesh_set_channel()` (16-byte keys).
- Direct messages (`TXT_MSG`) to and from known contacts, sent as flood packets. Received messages are
  acknowledged, and acknowledgements of sent messages mark them as delivered.

This node is a chat node: it does not repeat packets for others.

Radio presets: "Default" is EU_868 (869.618 MHz, 62.5 kHz, SF8, CR 4/8) US_915 (910.525 MHz, 62.5 kHz, SF7, CR 4/5) or
AU_915 (915.8 MHz, 250 kHz, SF10, CR 4/5).
"EU NL" is the Dutch community setting for EU_868: 869.618 MHz, 62.5 kHz, SF7, CR 4/5.
Other regions report `ERROR_NOT_SUPPORTED`.
"ESP-NOW" uses MeshCore's ESP-NOW transport in every region: unencrypted broadcast frames on Wi-Fi channel 1 at 20 dBm,
through a radio with `RADIO_MODULATION_ESPNOW`. It reaches other Tactility and MeshCore ESP-NOW nodes in Wi-Fi range,
not LoRa nodes.

Not implemented: path learning (direct routing), requests to repeaters and room servers, multi-part messages.
