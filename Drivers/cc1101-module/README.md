# CC1101 driver module

Kernel driver for the Texas Instruments CC1101 sub-GHz transceiver, built on
[RadioLib](https://github.com/jgromes/RadioLib). It registers a `radio` device
(see `<tactility/drivers/radio.h>`) that supports FSK, 4-FSK and OOK packets for TX/RX.

The device is a child of an SPI controller. The chip-select line comes from the parent's
`cs-gpios` entry matching the node's unit address. The `reg` property must match that
unit address (checked at device start). GDO0 must be an SoC GPIO. GDO2 is not used.

Boards with a frequency-dependent antenna matching network can wire up to two band switch
lines (any GPIO controller). Before the modem starts, the driver drives them from the
`band-switch-low` (300-348 MHz), `band-switch-mid` (387-464 MHz) or `band-switch-high`
(779-928 MHz) value: bit 0 drives `pin-band-switch-0` and bit 1 drives `pin-band-switch-1`.

Example (LilyGO T-Embed CC1101):

```dts
spi0 {
    compatible = "espressif,esp32-spi";
    cs-gpios = <&gpio0 41 GPIO_FLAG_NONE>, // 0: display
               <&gpio0 13 GPIO_FLAG_NONE>, // 1: SD card
               <&gpio0 12 GPIO_FLAG_NONE>; // 2: CC1101
    /* ... */

    radio@2 {
        compatible = "ti,cc1101";
        reg = <2>;
        pin-gdo0 = <&gpio0 3 GPIO_FLAG_NONE>;
        pin-band-switch-0 = <&gpio0 48 GPIO_FLAG_NONE>;
        pin-band-switch-1 = <&gpio0 47 GPIO_FLAG_NONE>;
        band-switch-low = <2>;
        band-switch-mid = <3>;
        band-switch-high = <1>;
    };
};
```

Notes:

- Starting the device reads the chip's VERSION register, so an absent or unpowered module
  fails at device start. The modem itself is initialized when the radio is enabled via
  `radio_set_enabled()`, which requires a modulation to be set first.
- Packets are limited to 61 bytes so a whole packet (with length byte and status bytes)
  always fits the 64-byte FIFO. Longer received packets are discarded by the chip.
- Packets use variable length mode with CRC enabled. Packets with a CRC mismatch are dropped.
- RX events report RSSI. SNR is `NAN`, as the CC1101 reports LQI instead.
- TX-done is detected by polling MARCSTATE until the chip returns to IDLE, with a timeout
  derived from the packet's time-on-air.
- Supported parameters: power (-30, -20, -15, -10, 0, 5, 7 or 10 dBm), frequency, RX
  bandwidth (58-812 kHz steps), data rate, frequency deviation (FSK and 4-FSK),
  preamble length (16-192 bits) and the 16-bit sync word.
- Listen-before-talk (`RADIO_PARAMETER_LBT`): before each transmission, the channel counts as busy when a sync word
  is being received or the RSSI is at or above `RADIO_PARAMETER_LBT_RSSI_THRESHOLD`. A busy channel backs off
  20-200 ms, up to 5 times, before the transmission reports `RADIO_TRANSMISSION_STATE_CHANNEL_BUSY`.
- `radio_get_rssi()` and `radio_is_receiving()` are served by the radio thread without leaving RX.
- Bus sharing works the same as in the sx126x module: each RadioLib exchange runs inside
  the kernel SPI controller lock and ESP-IDF's per-host bus lock.
- Hotplug: a node with status "okay" is only started when the chip is found. Before the device starts, the probe
  reads the VERSION register. While the radio is on, the radio thread reads that register. While the radio is off,
  a started device counts as present. The probe skips the bus when another started device on the same SPI
  controller uses the same chip-select line.
