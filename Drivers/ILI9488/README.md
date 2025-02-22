# ILI9488

A basic Tactility display driver for ILI9488 panels.

**Warning:** This driver uses 3 or 18 bits per pixel in SPI mode. This requires a software pixel conversion at runtime
and comes with a big performance penalty. It lowers the rate of rendering and it also requires an extra display buffer.
