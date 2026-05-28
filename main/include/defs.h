#ifndef DEFS_H_
#define DEFS_H_

#define TFT_CS    4 // CSX
#define TFT_RESET 5
#define TFT_DC    15 // D/CX Data/command. High = data, Low = command
#define TFT_MOSI  13
#define TFT_SCK   14 // SCL Clock
#define TFT_LED   23 // Backlight
#define TFT_MISO  12

// Physical dimensions of the display, portrait
#define TFT_HEIGHT 320
#define TFT_WIDTH  240

#define C_SOFT_RESET                          0x01
#define C_READ_DISPLAY_ID                     0x04
#define C_READ_DISPLAY_STATUS                 0x09
#define C_READ_DISPLAY_POWER_MODE             0x0A
#define C_READ_DISPLAY_MADCTL                 0x0B
#define C_READ_DISPLAY_PIXEL_FORMAT           0x0C
#define C_READ_DISPLAY_IMAGE_MODE             0x0D
#define C_READ_DISPLAY_SIGNAL_MODE            0x0E
#define C_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT 0x0F
#define C_SLEEP_ENTER                         0x10
#define C_SLEEP_OUT                           0x11
#define C_PARTIAL_MODE_ON                     0x12
#define C_NORMAL_DISPLAY_MODE_ON              0x13
#define C_INVERSION_OFF                       0x20
#define C_INVERSION_ON                        0x21
#define C_GAMMA_SET                           0x26
#define C_DISPLAY_OFF                         0x28
#define C_DISPLAY_ON                          0x29
#define C_COLUMN_ADDRESS_SET                  0x2A
#define C_PAGE_ADDRESS_SET                    0x2B
#define C_MEMORY_WRITE                        0x2C
#define C_COLOR_SET                           0x2D
#define C_MEMORY_READ                         0x2E
#define C_PARTIAL_AREA                        0x30
#define C_VERT_SCROLL_DEFINITION              0x33
#define C_TEARING_EFFECT_LINE_OFF             0x34
#define C_TEARING_EFFECT_LINE_ON              0x35
#define C_MEMORY_ACCESS_CONTROL               0x36
#define C_VERT_SCROLL_START_ADDRESS           0x37
#define C_IDLE_MODE_OFF                       0x38
#define C_IDLE_MODE_ON                        0x39
#define C_PIXEL_FORMAT_SET                    0x3A
#define C_WRITE_MEMORY_CONTINUE               0x3C
#define C_READ_MEMORY_CONTINUE                0x3E
#define C_SET_TEAR_SCANLINE                   0x44
#define C_GET_SCANLINE                        0x45
#define C_WRITE_DISPLAY_BRIGHTNESS            0x51
#define C_READ_DISPLAY_BRIGHTNESS             0x52
#define C_WRITE_CTRL_DISPLAY                  0x53
#define C_READ_CTRL_DISPLAY                   0x54
#define C_WRITE_CONTENT_ADAPT_BRIGHTNESS      0x55
#define C_READ_CONTENT_ADAPT_BRIGHTNESS       0x56
#define C_WRITE_CABC                          0x5E
#define C_READ_CABC                           0x5F
#define C_READ_ID1                            0xDA
#define C_READ_ID2                            0xDB
#define C_READ_ID3                            0xDC

#define C_RGB_INTERFACE_CONTROL      0xB0
#define C_FRAME_RATE_CONTROL_NORMAL  0xB1
#define C_FRAME_RATE_CONTROL_IDLE    0xB2
#define C_FRAME_RATE_CONTROL_PARTIAL 0xB3
#define C_DISPLAY_INVERSION_CONTROL  0xB4
#define C_BLANKING_PORCH_CONTROL     0xB5
#define C_DISPLAY_FUNCTION_CONTROL   0xB6
#define C_ENTRY_MODE_SET             0xB7
#define C_BACKLIGHT_CONTROL_1        0xB8
#define C_BACKLIGHT_CONTROL_2        0xB9
#define C_BACKLIGHT_CONTROL_3        0xBA
#define C_BACKLIGHT_CONTROL_4        0xBB
#define C_BACKLIGHT_CONTROL_5        0xBC
#define C_BACKLIGHT_CONTROL_6        0xBD
#define C_BACKLIGHT_CONTROL_7        0xBE
#define C_BACKLIGHT_CONTROL_8        0xBF
#define C_POWER_CONTROL_1            0xC0
#define C_POWER_CONTROL_2            0xC1
#define C_VCOM_CONTROL_1             0xC5
#define C_VCOM_CONTROL_2             0xC7
#define C_NV_MEMORY_WRITE            0xD0
#define C_NV_MEMORY_PROTECTION_KEY   0xD1
#define C_NV_MEMORY_STATUS_READ      0xD2
#define C_READ_ID4                   0xD3
#define C_POSITIVE_GAMMA_CORRECTION  0xE0
#define C_NEGATIVE_GAMMA_CORRECTION  0xE1
#define C_DIGITAL_GAMMA_CONTROL_1    0xE2
#define C_DIGITAL_GAMMA_CONTROL_2    0xE3
#define C_INTERFACE_CONTROL          0xF6
#define C_POWER_CONTROL_A            0xCB
#define C_POWER_CONTROL_B            0xCF
#define C_DRIVER_TIMING_CONTROL_A    0xE8
#define C_DRIVER_TIMING_CONTROL_A_2  0xE9
#define C_DRIVER_TIMING_CONTROL_B    0xEA
#define C_POWER_ON_SEQUENCE_CONTROL  0xED
#define C_ENABLE_3G                  0xF2
#define C_PUMP_RATIO_CONTROL         0xF7

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// GPIO Matrix
#define GPIO_BASE             0x3FF44000
#define GPIO_OUT_REG          ((volatile uint32_t *)(GPIO_BASE + 0x0004)) // Current state ...31
#define GPIO_OUT_W1TS_REG     ((volatile uint32_t *)(GPIO_BASE + 0x0008)) // Write 1 to set output
#define GPIO_OUT_W1TC_REG     ((volatile uint32_t *)(GPIO_BASE + 0x000C)) // Write 1 to clear output
#define GPIO_OUT1_REG         ((volatile uint32_t *)(GPIO_BASE + 0x0010)) // 32...
#define GPIO_OUT1_W1TS_REG    ((volatile uint32_t *)(GPIO_BASE + 0x0014))
#define GPIO_OUT1_W1TC_REG    ((volatile uint32_t *)(GPIO_BASE + 0x0018))
#define GPIO_ENABLE_REG       ((volatile uint32_t *)(GPIO_BASE + 0x0020)) // Current state
#define GPIO_ENABLE_W1TS_REG  ((volatile uint32_t *)(GPIO_BASE + 0x0024)) // Write 1 to set output
#define GPIO_ENABLE_W1TC_REG  ((volatile uint32_t *)(GPIO_BASE + 0x0028)) // Write 1 to clear output
#define GPIO_ENABLE1_REG      ((volatile uint32_t *)(GPIO_BASE + 0x002C)) // 32...
#define GPIO_ENABLE1_W1TS_REG ((volatile uint32_t *)(GPIO_BASE + 0x0030))
#define GPIO_ENABLE1_W1TC_REG ((volatile uint32_t *)(GPIO_BASE + 0x0034))

// IO Mux
#define IO_MUX_BASE 0x3FF49000

#endif