#ifndef TFT_H_
#define TFT_H_

#include "driver/spi_master.h"
#include "esp_err.h"

#define TFT_SCENE_OBJECT_MAX 32
// Must divide the screen height with no remainder!
#define TFT_SCANLINE_HEIGHT 4

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t dataCount; // Number of bytes in data; 0xFF
} lcd_init_cmd_t;

typedef enum uint8_t
{
    OBJECT_RECTANGLE,
    OBJECT_CIRCLE
} scene_object_type_t;

typedef struct {
    uint8_t id;
    scene_object_type_t type;
    uint16_t x;
    uint16_t y;
    union {
        struct {
            uint16_t width;
            uint16_t height;
            uint16_t color;
        } rectangle;
    };
} scene_object_t;

esp_err_t tft_init(void);

void tft_test(void);

void tft_square_test();

void tft_clear();

void tft_send_command(uint8_t cmd);

void tft_send_data_byte(uint8_t data);

void tft_send_data(uint8_t *data, size_t len);

void tft_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);

void tft_render_scene();

// TODO rename, way too long
void tft_add_rectangle_to_scene(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

#endif