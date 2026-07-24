# gps-generic-module

Kernel driver implementing the `GPS_TYPE`/`GpsApi` interface (declared by `Drivers/gps-module`,
Apache-2.0) for generic UART-connected GPS/GNSS receivers: chipset probing, init sequences, and
NMEA parsing for MTK, Airoha/AG33xx, ATGM336H/CASIC, Unicore UC6580 and u-blox 6/7/8/9/10 modules.

## License

This module is licensed under **GPL-3.0-or-later** (see `LICENSE-GPL-3.0.md`), separately from
the rest of Tactility (Apache-2.0). The probing and initialization logic (`source/probe.cpp`,
`source/init.cpp`, `source/ublox.cpp` and their private headers) is ported from
[meshtastic/firmware](https://github.com/meshtastic/firmware) (GPL-3.0-or-later); see the
`From: <url>` comments in those files for the exact origin of each ported function.
