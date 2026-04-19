# ILI9341

## Datasheet

Can be found here: https://cdn-shop.adafruit.com/datasheets/ILI9341.pdf
ILI9341 init sample: https://github.com/espressif/esp-idf/blob/2060ee9a548a0975bd7c5d690230f6d332f5054f/examples/peripherals/spi_master/main/spi_master_example_main.c

# Notes

- Moving picture area can be specieid in GRAM by window address function
- Bit format per pixel color order is selected by DBI[2:0] of 3Ah register.
- CSX = CS = Chip Enable

# Logic

Channel 0 = Clock 
Channel 1 = MISO
Channel 2 = DCX
Channel 3 = CX

