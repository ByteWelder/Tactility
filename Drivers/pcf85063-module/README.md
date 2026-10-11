# PCF85063A I2C Driver

A driver for the `PCF85063A` Real-Time Clock/Calendar from NXP.

Unlike the `BM8563`/`PCF8563` (which is what this repo's `bm8563-module` targets), the
PCF85063 keeps its time registers at `0x04`–`0x0A` (seconds, minutes, hours, days,
weekdays, months, years), with the `OS` clock-integrity flag in bit 7 of the seconds
register and the century flag in bit 7 of the months register.

See https://www.nxp.com/docs/en/data-sheet/PCF85063A.pdf

License: [Apache v2.0](LICENSE-Apache-2.0.md)
