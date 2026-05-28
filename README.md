# ILI9341

This is a playful learning experience driving an ILI9341 TFT display module resulting in a simple library.
It exposes almost no options because it is not supposed to be a full library.

It implements a scanline rasterizer which can:

- Render rectangles
- Circles
- Bitmap fonts
- Lines

Upon adding something you get a handle to edit the objects properties. 
Animations can be done using scenes.

## Datasheet

Can be found here: https://cdn-shop.adafruit.com/datasheets/ILI9341.pdf
ILI9341 init sample: https://github.com/espressif/esp-idf/blob/2060ee9a548a0975bd7c5d690230f6d332f5054f/examples/peripherals/spi_master/main/spi_master_example_main.c

ESP32: https://documentation.espressif.com/esp32_technical_reference_manual_en.pdf

# Notes

- Moving picture area can be specieid in GRAM by window address function
- Bit format per pixel color order is selected by DBI[2:0] of 3Ah register.
- CSX = CS = Chip Enable

# Logic Analyzer

Channel 0 = Clock 
Channel 1 = MISO
Channel 2 = DCX
Channel 3 = CX

# TODO

- Triangle rendering
- Rotating cube
- Polygons