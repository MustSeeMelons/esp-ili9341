#include "include/tft.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"
#include "include/defs.h"
#include "include/font.h"
#include "include/utils.h"
#include "math.h"
#include <string.h>

static const char *TAG = "TFT";

static uint16_t scene_object_id_counter = 1;

extern const uint8_t font[96][32];

uint16_t curr_display_height;
uint16_t curr_display_width;

// Next object index
static uint8_t scene_index = 0;
// Refs to all active objects
static scene_object_t scene_objects[TFT_SCENE_OBJECT_MAX];
// Ids of objects currently on each scanline, using larges axis
static scene_object_t *scanline_residents[TFT_HEIGHT / TFT_SCANLINE_HEIGHT][TFT_SCENE_OBJECT_MAX] = {NULL};

// Each pixel is a uint_16, 565 bit colors
// heap_caps_alloc made it slower for some reason
static uint8_t scanline[TFT_HEIGHT * TFT_SCANLINE_HEIGHT * 2];

static lcd_init_cmd_t ili_init_cmds[] = {
    {C_POWER_CONTROL_B, {0x00, 0x83, 0X30}, 3},
    {C_POWER_ON_SEQUENCE_CONTROL, {0x64, 0x03, 0X12, 0X81}, 4},
    {C_DRIVER_TIMING_CONTROL_A, {0x85, 0x01, 0x79}, 3},
    {C_POWER_CONTROL_A, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    {C_PUMP_RATIO_CONTROL, {0x30}, 1},
    {C_DRIVER_TIMING_CONTROL_B, {0x00, 0x00}, 2},
    {C_POWER_CONTROL_1, {0x26}, 1},
    {C_POWER_CONTROL_2, {0x11}, 1},
    {C_VCOM_CONTROL_1, {0x35, 0x3E}, 2},
    {C_VCOM_CONTROL_2, {0xBE}, 1},
    {C_MEMORY_ACCESS_CONTROL, {TFT_Portrait}, 1},
    {C_PIXEL_FORMAT_SET, {0x55}, 1}, // 16 bit for RGB and MCU interfac
    {C_FRAME_RATE_CONTROL_NORMAL, {0x00, 0x1B}, 2},
    {C_ENABLE_3G, {0x02}, 1}, // Disable extra gamma correction
    {C_GAMMA_SET, {0x02}, 1},
    // Gamma corrections are some defaults from samples
    {C_POSITIVE_GAMMA_CORRECTION,
     {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00},
     15},
    {C_NEGATIVE_GAMMA_CORRECTION,
     {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F},
     15},
    {C_ENTRY_MODE_SET, {0x07}, 1},
    {C_DISPLAY_FUNCTION_CONTROL, {0x0A, 0x82, 0x27, 0x00}, 4},
    // Default sequnce as per datasheet
    {C_SLEEP_OUT, {0}, 0x80},
    {C_DISPLAY_ON, {0}, 0x80},
    {C_COLUMN_ADDRESS_SET, {0, 0, (TFT_WIDTH - 1) >> 8, (TFT_WIDTH - 1) & 0xFF}, 4},
    {C_PAGE_ADDRESS_SET, {0, 0, (TFT_HEIGHT - 1) >> 8, (TFT_HEIGHT - 1) & 0xFF}, 4},
    {C_MEMORY_WRITE, {0}, 0},
    {0, {0}, 0xff}, // End marker
};

static spi_device_handle_t spi_handle;

static inline uint16_t tft_scanline_count(void) {
    // Integer ceiling division
    return (curr_display_height + TFT_SCANLINE_HEIGHT - 1) / TFT_SCANLINE_HEIGHT;
}

static void tft_set_command_data(uint8_t command_id, uint8_t *data, uint8_t data_len) {
    for (size_t i = 0; i < sizeof(ili_init_cmds) / sizeof(ili_init_cmds[0]); i++) {
        if (ili_init_cmds[i].cmd == command_id) {
            ili_init_cmds[i].dataCount = data_len;
            memcpy(ili_init_cmds[i].data, data, data_len);
            return;
        }
    }
}

void tft_patch_init_commands(tft_rotation_t rotation) {
    // clang-format off
    switch (rotation) {
    case TFT_Portrait:
        curr_display_width = TFT_WIDTH;
        curr_display_height = TFT_HEIGHT;

        tft_set_command_data(C_MEMORY_ACCESS_CONTROL, (uint8_t[]){TFT_Portrait}, 1);
        break;
    case TFT_Landscape:
        curr_display_width = TFT_HEIGHT;
        curr_display_height = TFT_WIDTH;

        tft_set_command_data(C_MEMORY_ACCESS_CONTROL, (uint8_t[]){TFT_Landscape}, 1);
        break;
    case TFT_Portrait_Flipped:
        curr_display_width = TFT_WIDTH;
        curr_display_height = TFT_HEIGHT;

        tft_set_command_data(C_MEMORY_ACCESS_CONTROL, (uint8_t[]){TFT_Portrait_Flipped}, 1); 
        break;
    case TFT_Landscape_Flipped:
        curr_display_width = TFT_HEIGHT;
        curr_display_height = TFT_WIDTH;

        tft_set_command_data(C_MEMORY_ACCESS_CONTROL, (uint8_t[]){TFT_Landscape_Flipped}, 1);
        break;
    default:
        break;
    }
    // clang-format on
}

/** Initialize non SPI TFT pins */
static esp_err_t tft_init_pins(void) {
    esp_err_t err;

    gpio_config_t io_led = {
        .pin_bit_mask = 1ULL << TFT_LED,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    err = gpio_config(&io_led);

    if (err != ESP_OK) {
        return err;
    }

    gpio_config_t io_cs = {
        .pin_bit_mask = 1ULL << TFT_CS,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    err = gpio_config(&io_cs);

    if (err != ESP_OK) {
        return err;
    }

    gpio_config_t io_dc = {
        .pin_bit_mask = 1ULL << TFT_DC,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    err = gpio_config(&io_dc);

    if (err != ESP_OK) {
        return err;
    }

    gpio_config_t io_reset = {
        .pin_bit_mask = 1ULL << TFT_RESET,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    err = gpio_config(&io_reset);

    gpio_set_level(TFT_CS, 1);
    gpio_set_level(TFT_DC, 1);

    return err;
}

void tft_send_data_byte(uint8_t data) {
    *GPIO_OUT_W1TC_REG = (0x1 << TFT_CS);
    *GPIO_OUT_W1TS_REG = (0x1 << TFT_DC);

    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };

    spi_device_transmit(spi_handle, &t);

    *GPIO_OUT_W1TS_REG = (0x1 << TFT_CS);
}

void tft_send_data(uint8_t *data, size_t len) {
    if (len == 0) {
        return;
    }

    // XXX Raw registers made no FPS difference
    *GPIO_OUT_W1TC_REG = (0x1 << TFT_CS);
    *GPIO_OUT_W1TS_REG = (0x1 << TFT_DC);

    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };

    spi_device_transmit(spi_handle, &t);

    *GPIO_OUT_W1TS_REG = (0x1 << TFT_CS);
}

void tft_send_command(uint8_t cmd) {
    *GPIO_OUT_W1TC_REG = (0x1 << TFT_CS);
    *GPIO_OUT_W1TC_REG = (0x1 << TFT_DC);

    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };

    spi_device_transmit(spi_handle, &t);

    *GPIO_OUT_W1TS_REG = (0x1 << TFT_CS);
    *GPIO_OUT_W1TS_REG = (0x1 << TFT_DC);
}

void tft_set_window(int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end) {
    tft_send_command(C_COLUMN_ADDRESS_SET);
    tft_send_data_byte(x_start >> 8);
    tft_send_data_byte(x_start & 0xFF);
    tft_send_data_byte(x_end >> 8);
    tft_send_data_byte(x_end & 0xFF);

    tft_send_command(C_PAGE_ADDRESS_SET);
    tft_send_data_byte(y_start >> 8);
    tft_send_data_byte(y_start & 0xFF);
    tft_send_data_byte(y_end >> 8);
    tft_send_data_byte(y_end & 0xFF);

    tft_send_command(C_MEMORY_WRITE);
}

esp_err_t tft_init(tft_init_config_t config) {
    esp_err_t err;

    // Apply rotation to init commands
    tft_patch_init_commands(config.rotation);

    spi_bus_config_t buscfg = {
        .mosi_io_num = TFT_MOSI,
        .miso_io_num = TFT_MISO,
        .sclk_io_num = TFT_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TFT_HEIGHT * TFT_SCANLINE_HEIGHT * 2,
    };

    err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    if (err != ESP_OK) {
        return err;
    }

    spi_device_interface_config_t devcfg = {
        // Up to 34 Mhz is fine
        .clock_speed_hz = 34 * 1000 * 1000, // 34 MHz
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
        .flags = 0,
    };

    err = tft_init_pins();

    if (err != ESP_OK) {
        return err;
    }

    // Hardware reset the driver as per the datasheet
    gpio_set_level(TFT_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TFT_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TFT_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    err = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);

    if (err != ESP_OK) {
        return err;
    }

    gpio_set_level(TFT_LED, 0);

    uint8_t init_cmd_index = 0;
    lcd_init_cmd_t *lcd_init_cmds = ili_init_cmds;

    // Power ON sequence
    while (lcd_init_cmds[init_cmd_index].dataCount != 0xff) {
        tft_send_command(lcd_init_cmds[init_cmd_index].cmd);

        uint8_t data_len = lcd_init_cmds[init_cmd_index].dataCount & 0x1F; // 5 bits have length
        if (data_len > 0) {
            tft_send_data(lcd_init_cmds[init_cmd_index].data, data_len);
        }

        if (lcd_init_cmds[init_cmd_index].dataCount & 0x80) { // High bit signals delay
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        init_cmd_index++;
    }

    gpio_set_level(TFT_LED, 1);

    return err;
}

void tft_reset() {
    scene_index = 0;
    memset(scene_objects, 0, sizeof(scene_objects));
    memset(scanline_residents, 0, sizeof(scanline_residents));
    tft_clear();
}

void update_scanline_residents() {
    memset(scanline_residents, 0, sizeof(scanline_residents));
    uint16_t total_scanlines = tft_scanline_count();

    for (uint8_t i = 0; i < scene_index; i++) {
        scene_object_t *obj = &scene_objects[i];

        int16_t start_scanline = obj->y / TFT_SCANLINE_HEIGHT;
        int16_t end_scanline = 0;

        switch (obj->type) {
        case OBJECT_RECTANGLE:
            end_scanline = (obj->y + obj->rectangle.height) / TFT_SCANLINE_HEIGHT;
            break;
        case OBJECT_TEXT:
            end_scanline = (obj->y + FONT_HEIGHT * obj->text.font_size) / TFT_SCANLINE_HEIGHT;
            break;
        case OBJECT_CIRCLE:
            start_scanline = (obj->y - obj->circle.radius) / TFT_SCANLINE_HEIGHT;
            end_scanline = (obj->y + obj->circle.radius) / TFT_SCANLINE_HEIGHT;
            break;
        default:
            break;
        }

        for (int j = start_scanline; j <= end_scanline; j++) {
            if (j < 0 || j >= total_scanlines) {
                continue;
            }

            for (int k = 0; k < TFT_SCENE_OBJECT_MAX; k++) {
                if (scanline_residents[j][k] == 0) {
                    scanline_residents[j][k] = obj;
                    break;
                }
            }
        }
    }
}

scene_object_t *tft_add_rectangle(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color) {
    if (scene_index >= TFT_SCENE_OBJECT_MAX) {
        return NULL;
    }

    scene_object_t *obj = &scene_objects[scene_index++];

    obj->id = scene_object_id_counter++;
    obj->type = OBJECT_RECTANGLE;
    obj->x = x;
    obj->y = y;
    obj->rectangle.width = width;
    obj->rectangle.height = height;
    obj->rectangle.color = color;

    int16_t start_scanline = obj->y / TFT_SCANLINE_HEIGHT;
    int16_t end_scanline = (obj->y + obj->rectangle.height) / TFT_SCANLINE_HEIGHT;

    uint16_t total_scanlines = tft_scanline_count();

    // Place it in the first empty slot
    for (int i = start_scanline; i <= end_scanline; i++) {
        if (i < 0 || i >= total_scanlines) {
            continue;
        }

        for (int j = 0; j < TFT_SCENE_OBJECT_MAX; j++) {
            if (scanline_residents[i][j] == 0) {
                scanline_residents[i][j] = obj;
                break;
            }
        }
    }

    return obj;
}

scene_object_t *tft_add_circle(int16_t x, int16_t y, uint16_t radius, uint16_t color) {
    if (scene_index >= TFT_SCENE_OBJECT_MAX) {
        return NULL;
    }

    scene_object_t *obj = &scene_objects[scene_index++];

    obj->id = scene_object_id_counter++;
    obj->type = OBJECT_CIRCLE;
    obj->x = x;
    obj->y = y;
    obj->circle.radius = radius;
    obj->circle.color = color;

    int16_t start_scanline = (y - radius) / TFT_SCANLINE_HEIGHT;
    int16_t end_scanline = (y + radius) / TFT_SCANLINE_HEIGHT;

    uint16_t total_scanlines = tft_scanline_count();

    // Place it in the first empty slot
    for (int16_t i = start_scanline; i <= end_scanline; i++) {
        if (i < 0 || i >= total_scanlines) {
            continue;
        }

        for (int j = 0; j < TFT_SCENE_OBJECT_MAX; j++) {
            if (scanline_residents[i][j] == 0) {
                scanline_residents[i][j] = obj;
                break;
            }
        }
    }

    return obj;
}

scene_object_t *tft_add_text(int16_t x, int16_t y, char *text, uint16_t font_size, uint16_t color) {
    if (scene_index >= TFT_SCENE_OBJECT_MAX) {
        return NULL;
    }

    scene_object_t *obj = &scene_objects[scene_index++];

    obj->id = scene_object_id_counter++;
    obj->type = OBJECT_TEXT;
    obj->x = x;
    obj->y = y;
    obj->text.text = text;
    obj->text.font_size = font_size;
    obj->text.color = color;

    int16_t start_scanline = obj->y / TFT_SCANLINE_HEIGHT;
    int16_t end_scanline = (obj->y + FONT_HEIGHT * font_size) / TFT_SCANLINE_HEIGHT;

    uint16_t total_scanlines = tft_scanline_count();

    for (int i = start_scanline; i <= end_scanline; i++) {
        if (i < 0 || i >= total_scanlines) {
            continue;
        }

        for (int j = 0; j < TFT_SCENE_OBJECT_MAX; j++) {
            if (scanline_residents[i][j] == 0) {
                scanline_residents[i][j] = obj;
                break;
            }
        }
    }

    return obj;
}

scene_object_t *tft_add_line(
    int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end, uint16_t color, uint8_t stroke) {
    if (scene_index >= TFT_SCENE_OBJECT_MAX) {
        return NULL;
    }

    scene_object_t *obj = &scene_objects[scene_index++];

    obj->id = scene_object_id_counter++;
    obj->type = OBJECT_LINE;
    obj->x = x_start;
    obj->y = y_start;
    obj->line.x_end = x_end;
    obj->line.y_end = y_end;
    obj->line.color = color;
    obj->line.stroke = stroke;

    int16_t start_scanline = obj->y / TFT_SCANLINE_HEIGHT;
    int16_t end_scanline = obj->line.y_end / TFT_SCANLINE_HEIGHT;

    uint16_t total_scanlines = tft_scanline_count();

    for (int i = start_scanline; i <= end_scanline; i++) {
        if (i < 0 || i >= total_scanlines) {
            continue;
        }

        for (int j = 0; j < TFT_SCENE_OBJECT_MAX; j++) {
            if (scanline_residents[i][j] == 0) {
                scanline_residents[i][j] = obj;
                break;
            }
        }
    }

    return obj;
}

scene_object_t *tft_add_triangle(vector2_t vertices[3], uint16_t color) {
    if (scene_index >= TFT_SCENE_OBJECT_MAX) {
        return NULL;
    }

    scene_object_t *obj = &scene_objects[scene_index++];
    obj->id = scene_object_id_counter++;
    obj->type = OBJECT_TRIANGLE;
    memcpy(obj->triangle.vertices, vertices, sizeof(obj->triangle.vertices));
    obj->triangle.color = color;
    obj->triangle.min_x = get_min_x(vertices);
    obj->triangle.max_x = get_max_x(vertices);
    obj->triangle.min_y = get_min_y(vertices);
    obj->triangle.max_y = get_max_y(vertices);

    uint16_t total_scanlines = tft_scanline_count();

    int16_t start_scanline = obj->triangle.min_y / TFT_SCANLINE_HEIGHT;
    int16_t end_scanline = obj->triangle.max_y / TFT_SCANLINE_HEIGHT;

    for (int i = start_scanline; i <= end_scanline; i++) {
        if (i < 0 || i >= total_scanlines) {
            continue;
        }

        for (int j = 0; j < TFT_SCENE_OBJECT_MAX; j++) {
            if (scanline_residents[i][j] == 0) {
                scanline_residents[i][j] = obj;
                break;
            }
        }
    }

    return obj;
}

static void tft_render_line(
    int16_t scan_y_px_start, int16_t x_start, int16_t y_start, int16_t x_end, int16_t y_end, uint16_t color) {
    // Bresenham line
    int16_t dx = abs(x_end - x_start);
    int16_t dy = abs(y_end - y_start);

    int16_t p = 2 * dy - dx;

    int16_t two_dy = 2 * dy;
    int16_t two_dy_dx = 2 * (dy - dx);

    int16_t scanline_row = y_start - scan_y_px_start;

    if (x_start >= 0 && x_start < curr_display_width && scanline_row >= 0 && scanline_row < TFT_SCANLINE_HEIGHT) {
        scanline[(scanline_row * curr_display_width + x_start) * 2] = color >> 8;
        scanline[(scanline_row * curr_display_width + x_start) * 2 + 1] = color & 0xFF;
    }

    while (x_start < x_end) {
        x_start++;

        if (p < 0) {
            p += two_dy;
        } else {
            p += two_dy_dx;
            y_start++;
        }

        scanline_row = y_start - scan_y_px_start;

        if (x_start >= 0 && x_start < curr_display_width && scanline_row >= 0 && scanline_row < TFT_SCANLINE_HEIGHT) {
            scanline[(scanline_row * curr_display_width + x_start) * 2] = color >> 8;
            scanline[(scanline_row * curr_display_width + x_start) * 2 + 1] = color & 0xFF;
        }
    }
}

void tft_render_scene() {
    uint16_t total_scanlines = tft_scanline_count();

    // Go through each scanline
    for (size_t scanline_index = 0; scanline_index < total_scanlines; scanline_index++) {
        // Clear scanline
        int16_t scan_y_px_start = scanline_index * TFT_SCANLINE_HEIGHT;
        int16_t scanline_height = MIN(TFT_SCANLINE_HEIGHT, curr_display_height - scan_y_px_start);

        memset(scanline, 0x00, curr_display_width * scanline_height * 2);

        // Get the height in pixels so we can decide if we use object height or scanline height for rendering
        int16_t scan_y_px_end = scan_y_px_start + scanline_height;

        for (int j = 0; j < TFT_SCENE_OBJECT_MAX; j++) {
            // Render if we have a reference
            if (scanline_residents[scanline_index][j] != NULL) {
                scene_object_t *obj = scanline_residents[scanline_index][j];

                switch (obj->type) {
                case OBJECT_RECTANGLE: {
                    int16_t x_start = MAX(0, obj->x);
                    int16_t x_end = MIN(MAX(obj->x + obj->rectangle.width, 0), curr_display_width);

                    // clang-format off
                    int16_t y_start = obj->y > scan_y_px_start ? obj->y : scan_y_px_start;
                    int16_t y_end = MIN(obj->y + obj->rectangle.height < scan_y_px_end ? obj->y + obj->rectangle.height : scan_y_px_end, curr_display_height);
                    // clang-format on

                    // Fill object colors
                    for (int16_t y = y_start; y < y_end; y++) {
                        int16_t scanline_row = y - scan_y_px_start; // Which row of the scanline we are on

                        for (int16_t x = x_start; x < x_end; x++) {
                            scanline[(scanline_row * curr_display_width + x) * 2] = obj->rectangle.color >> 8;
                            scanline[(scanline_row * curr_display_width + x) * 2 + 1] = obj->rectangle.color & 0xFF;
                        }
                    }
                    break;
                }
                case OBJECT_TEXT: {
                    // clang-format off
                    int16_t y_start = MAX(0, obj->y > scan_y_px_start ? obj->y : scan_y_px_start);
                    int16_t y_end = MIN(obj->y + FONT_HEIGHT * obj->text.font_size < scan_y_px_end ? obj->y + FONT_HEIGHT * obj->text.font_size : scan_y_px_end, curr_display_height);
                    // clang-format on

                    // Render strip of the text
                    for (int16_t y = y_start; y < y_end; y++) {
                        // How many px of the text have been rendered so far
                        int16_t rendered_px = y - obj->y;
                        // Which row of the glyph we are on
                        int16_t glyph_row = rendered_px / obj->text.font_size;
                        // Which row of the scanline we are on
                        int16_t scanline_row = y - scan_y_px_start;

                        // Iterate over each character
                        char *text = obj->text.text;

                        int16_t char_counter = 0;

                        while (*text) {
                            char c = *text;

                            // Guard against unsupported characters in the font
                            if (c < FONT_CHAR_START || c > FONT_CHAR_END) {
                                text++;
                                continue;
                            }

                            // 10x16 font stores each glyph row as 2 bytes (little-endian)
                            uint16_t char_row_data = (uint16_t)font[c - FONT_CHAR_START][glyph_row * 2] |
                                                     ((uint16_t)font[c - FONT_CHAR_START][glyph_row * 2 + 1] << 8);

                            // Iterate over each bit
                            for (uint8_t bit = 0; bit < FONT_WIDTH; bit++) {
                                int16_t character_x_base =
                                    obj->x + char_counter + ((FONT_WIDTH - bit - 1) * obj->text.font_size);

                                // Use top FONT_WIDTH bits from the 16-bit row container.
                                if ((char_row_data >> (15 - bit)) & 0x1) {

                                    for (int16_t s = 0; s < obj->text.font_size; s++) {
                                        int16_t x = character_x_base + s;

                                        if (x < 0 || x >= curr_display_width) {
                                            continue;
                                        }

                                        int16_t pos = (scanline_row * curr_display_width + x) * 2;

                                        scanline[pos] = obj->text.color >> 8;
                                        scanline[pos + 1] = obj->text.color & 0xFF;
                                    }
                                }
                            }

                            char_counter += FONT_WIDTH * obj->text.font_size + obj->text.font_size;
                            text++;
                        }
                    }

                    break;
                }
                case OBJECT_CIRCLE: {
                    int16_t y_start = MAX(obj->y - obj->circle.radius, scan_y_px_start);
                    int16_t y_end = MIN(obj->y + obj->circle.radius, scan_y_px_end);

                    for (int16_t y = y_start; y < y_end; y++) {
                        int16_t scanline_row = y - scan_y_px_start;

                        // Get distance from center of circle
                        int16_t dy = y - obj->y;

                        // Get x distance using circle equation
                        int16_t dx = sqrt(obj->circle.radius * obj->circle.radius - dy * dy);

                        int16_t x_start = MAX(0, obj->x - dx);
                        int16_t x_end = MIN(MAX(obj->x + dx, 0), curr_display_width);

                        for (int16_t x = x_start; x < x_end; x++) {
                            int16_t pos = (scanline_row * curr_display_width + x) * 2;

                            scanline[pos] = obj->circle.color >> 8;
                            scanline[pos + 1] = obj->circle.color & 0xFF;
                        }
                    }

                    break;
                }
                case OBJECT_LINE: {
                    // clang-format off
                    int16_t y_start = MIN(MAX(obj->y > scan_y_px_start ? obj->y : scan_y_px_start, 0), curr_display_height);
                    int16_t y_end = MIN(MAX(obj->line.y_end < scan_y_px_end ? obj->line.y_end : scan_y_px_end, 0), curr_display_height);
                    
                    float t_start = get_blend_factor(y_start, obj->y, obj->line.y_end);
                    float t_end = get_blend_factor(y_end, obj->y, obj->line.y_end);
                    
                    int16_t x_start = MAX(lerp_i(obj->x, obj->line.x_end, t_start), 0);
                    int16_t x_end = MIN(lerp_i(obj->x, obj->line.x_end, t_end), curr_display_width);
                    // clang-format on

                    if (obj->line.stroke > 1) {
                        // Calculate perpendicular vector (-dy, dx)
                        int16_t dx = abs(x_end - x_start);
                        int16_t dy = abs(y_end - y_start);

                        // Normalize
                        int len = MAX(abs(dx), abs(dy)); // Approximate length is good enough here
                        int px = (-dy * 256) / len;      // Fixed point scaling to avoid floats
                        int py = (dx * 256) / len;

                        int half = obj->line.stroke / 2;

                        for (int i = -half; i <= half; i++) {
                            // Add half (128) to round
                            // Scale back by dividing by 256
                            int ox = (px * i + 128) >> 8;
                            int oy = (py * i + 128) >> 8;

                            tft_render_line(
                                scan_y_px_start, x_start + ox, y_start + oy, x_end + ox, y_end + oy, obj->line.color);
                        }
                    } else {
                        tft_render_line(scan_y_px_start, x_start, y_start, x_end, y_end, obj->line.color);
                    }

                    break;
                }
                case OBJECT_TRIANGLE: {
                    // clang-format off
                    int16_t y_start = MIN(MAX(obj->triangle.min_y > scan_y_px_start ? obj->triangle.min_y : scan_y_px_start, 0), curr_display_height);
                    int16_t y_end = MIN(MAX(obj->triangle.max_y < scan_y_px_end ? obj->triangle.max_y : scan_y_px_end, 0), curr_display_height);

                    // TODO Calculate min/max x bounds for the triangle in this scanline
                    int16_t x_start = MAX(obj->triangle.min_x, 0);
                    int16_t x_end = MIN(obj->triangle.max_x, curr_display_width);
                    // clang-format on

                    int32_t area =
                        edge_function(obj->triangle.vertices[0], obj->triangle.vertices[1], obj->triangle.vertices[2]);

                    if (area == 0) {
                        break;
                    }

                    for (int16_t y = y_start; y < y_end; y++) {
                        int16_t scanline_row = y - scan_y_px_start;

                        for (int16_t x = x_start; x < x_end; x++) {
                            vector2_t p = {x, y};

                            int32_t abp = edge_function(obj->triangle.vertices[0], obj->triangle.vertices[1], p);

                            int32_t bcp = edge_function(obj->triangle.vertices[1], obj->triangle.vertices[2], p);

                            int32_t cap = edge_function(obj->triangle.vertices[2], obj->triangle.vertices[0], p);

                            if ((area > 0 && abp >= 0 && bcp >= 0 && cap >= 0) ||
                                (area < 0 && abp <= 0 && bcp <= 0 && cap <= 0)) {
                                scanline[(scanline_row * curr_display_width + x) * 2] = obj->triangle.color >> 8;
                                scanline[(scanline_row * curr_display_width + x) * 2 + 1] = obj->triangle.color & 0xFF;
                            }
                        }
                    }

                    break;
                }
                default:
                    ESP_LOGI(TAG, "Can't render %d", obj->type);
                    break;
                }

            } else {
                // Once we find an empty slot, we can skip the rest since they will be empty too
                break;
            }
        }

        tft_set_window(0, scan_y_px_start, curr_display_width - 1, scan_y_px_end - 1);
        tft_send_data(scanline, curr_display_width * scanline_height * 2);
    }
}

void tft_clear() {
    tft_set_window(0, 0, curr_display_width - 1, curr_display_height - 1);

    uint16_t color = 0x0000;

    // Use the largest axis
    uint8_t line[curr_display_width * 2];

    // XXX Line by line is inneficent
    for (int x = 0; x < curr_display_width; x++) {
        line[x * 2] = color >> 8;
        line[x * 2 + 1] = color & 0xFF;
    }

    for (int y = 0; y < curr_display_height; y++) {
        tft_send_data(line, sizeof(line));
    }
}
