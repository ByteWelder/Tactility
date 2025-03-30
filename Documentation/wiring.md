# 4 Pin connectors

Many devices have 4-pin connectors:
Either a [Grove](https://docs.m5stack.com/en/start/interface/grove) connector or a [JST](https://en.wikipedia.org/wiki/JST_connector) one.

Boards that implement an I2C/UART configuration for such a connector are advised to always implement both.
To make it consistent, it's important that the pins are mapped in the same manner:

## I2C Pins

Follow the Grove layout:

Black(GND) - Red(5V) - Yellow(SDA) - White(SCL)

## I2C Pins

Follow this Grove layout on the MCU board:

Black(GND) - Red(5V) - Yellow(RX) - White(TX)

And this Grove layout on the peripheral:

Black(GND) - Red(5V) - Yellow(TX) - White(RX)
