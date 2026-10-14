# SH8601 display

A driver for the `SH8601` AMOLED display panel, over SPI/QSPI.

The SH8601 has no dedicated backlight pin: brightness is controlled through the panel's own
command interface. This module therefore exposes two devices - the display itself and a
`sh8601-backlight` device that sends brightness commands through the display's panel IO.

License: [Apache v2.0](LICENSE-Apache-2.0.md)
