#ifndef TFT_H_
#define TFT_H_

#include "driver/spi_master.h"
#include "esp_err.h"

#define TFT_SCENE_OBJECT_MAX 32
// Save on memory at the cost of max FPS. Must divide the screen height with no remainder.
#define TFT_SCANLINE_HEIGHT 32

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t dataCount; // Low stores number, high stores flag for delay. 0xFF as end marker
} lcd_init_cmd_t;

typedef enum
{
    OBJECT_RECTANGLE,
    OBJECT_TEXT,
    OBJECT_CIRCLE,
    OBJECT_TRIANGLE,
    OBJECT_LINE
} scene_object_type_t;

typedef struct {
    int16_t x;
    int16_t y;
} vector2_t;

typedef struct {
    int32_t step_x;
    int32_t step_y;
} edge_step_t;

typedef struct {
    uint8_t id;
    scene_object_type_t type;
    int16_t x;
    int16_t y;
    union {
        struct {
            uint16_t width;
            uint16_t height;
            uint16_t color;
        } rectangle;
        struct {
            char *text;
            uint16_t font_size;
            uint16_t width;
            uint16_t color;
        } text;
        struct {
            uint16_t radius;
            uint16_t color;
        } circle;
        struct {
            vector2_t vertices[3];
            int16_t min_x;
            int16_t min_y;
            int16_t max_x;
            int16_t max_y;
            edge_step_t ab;
            edge_step_t bc;
            edge_step_t ca;
            uint16_t color;
        } triangle;
        struct {
            int16_t x_end;
            int16_t y_end;
            uint16_t color;
            uint8_t stroke;
        } line;
    };
} scene_object_t;

typedef enum
{
    TFT_Portrait = 0x48,         // 0 Degrees
    TFT_Landscape = 0x28,        // 90 Degrees
    TFT_Portrait_Flipped = 0x88, // 180 Degrees
    TFT_Landscape_Flipped = 0xE8 // 270 Degrees
} tft_rotation_t;

typedef struct {
    tft_rotation_t rotation;
} tft_init_config_t;

esp_err_t tft_init(tft_init_config_t config);

void tft_reset();

void tft_patch_init_commands(tft_rotation_t rotation);

void tft_clear();

void tft_send_command(uint8_t cmd);

void tft_send_data_byte(uint8_t data);

void tft_send_data(uint8_t *data, size_t len);

void tft_set_window(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end);

void tft_render_scene();

void update_scanline_residents();

scene_object_t *tft_add_rectangle(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color);

scene_object_t *tft_add_text(int16_t x, int16_t y, char *text, uint16_t font_size, uint16_t color);

scene_object_t *tft_add_circle(int16_t x, int16_t y, uint16_t radius, uint16_t color);

scene_object_t *tft_add_triangle(vector2_t vertices[3], uint16_t color);

scene_object_t *tft_add_line(
    int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end, uint16_t color, uint8_t stroke);

void tft_update_scene_object(scene_object_t *obj);

#endif