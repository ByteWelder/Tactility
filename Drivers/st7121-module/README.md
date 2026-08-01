# ST7121 display controller

Driver for the Sitronix `ST7121` MIPI-DSI display panel (`sitronix,st7121`), driven over ESP-IDF's
`esp_lcd_st7121` component (DPI/DBI interface, ESP32-P4 and other `SOC_MIPI_DSI_SUPPORTED` targets
only). Owns the MIPI DSI PHY LDO channel and DSI bus directly, so unlike SPI/RGB panels it has no
parent bus controller device.

The ST7121 panel's in-cell touch controller is the same chip/protocol as the ST7123's (see
`st7123-module`'s `sitronix,st7123-touch` driver) - boards using this display reuse that driver
for touch rather than duplicating it here.

License: [Apache v2.0](LICENSE-Apache-2.0.md)
