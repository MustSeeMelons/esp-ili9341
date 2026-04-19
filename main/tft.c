#include "include/tft.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"
#include "include/defs.h"
#include "math.h"

static const char *TAG = "TFT";

static uint16_t scene_object_id_counter = 1;

// Next object index
static uint8_t scene_index = 0;
// Refs to all active objects
static scene_object_t scene_objects[TFT_SCENE_OBJECT_MAX];
// Ids of objects currently on each scanline
static scene_object_t *scanline_residents[TFT_WIDTH / TFT_SCANLINE_HEIGHT][TFT_SCENE_OBJECT_MAX] = {NULL};

static uint8_t scanline[TFT_WIDTH * 2];

DRAM_ATTR static lcd_init_cmd_t ili_init_cmds[] = {
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
    // XXX Should make this configurable at some point
    {C_MEMORY_ACCESS_CONTROL, {0x48}, 1}, // MX + BGR
    {C_PIXEL_FORMAT_SET, {0x55}, 1},      // 16 bit for RGB and MCU interfac
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
    // Default to full screen portrait
    {C_COLUMN_ADDRESS_SET, {0, 0, (TFT_HEIGHT - 1) >> 8, (TFT_HEIGHT - 1) & 0xFF}, 4},
    {C_PAGE_ADDRESS_SET, {0, 0, (TFT_WIDTH - 1) >> 8, (TFT_WIDTH - 1) & 0xFF}, 4},
    {C_MEMORY_WRITE, {0}, 0},
    {0, {0}, 0xff}, // End marker
};

static spi_device_handle_t spi_handle;

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
    gpio_set_level(TFT_CS, 0);
    gpio_set_level(TFT_DC, 1); // Data

    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };

    spi_device_transmit(spi_handle, &t);

    gpio_set_level(TFT_CS, 1);
}

void tft_send_data(uint8_t *data, size_t len) {
    gpio_set_level(TFT_CS, 0);
    gpio_set_level(TFT_DC, 1); // Data

    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };

    spi_device_transmit(spi_handle, &t);

    gpio_set_level(TFT_CS, 1);
}

void tft_send_command(uint8_t cmd) {
    gpio_set_level(TFT_CS, 0);
    gpio_set_level(TFT_DC, 0); // Command

    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };

    spi_device_transmit(spi_handle, &t);

    gpio_set_level(TFT_CS, 1);
    gpio_set_level(TFT_DC, 1);
}

void tft_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
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

esp_err_t tft_init(void) {
    esp_err_t err;

    spi_bus_config_t buscfg = {
        .mosi_io_num = TFT_MOSI,
        .miso_io_num = TFT_MISO,
        .sclk_io_num = TFT_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        // Up to 30 Mhz is fine
        .clock_speed_hz = 100 * 1000, // 100 kHz
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
        tft_send_data(lcd_init_cmds[init_cmd_index].data, lcd_init_cmds[init_cmd_index].dataCount & 0x1F);

        if (lcd_init_cmds[init_cmd_index].dataCount & 0x80) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        init_cmd_index++;
    }

    gpio_set_level(TFT_LED, 1);

    return err;
}

void tft_add_rectangle_to_scene(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    if (scene_index >= TFT_SCENE_OBJECT_MAX) {
        return;
    }

    scene_object_t *obj = &scene_objects[scene_index++];

    obj->id = scene_object_id_counter++;
    obj->type = OBJECT_RECTANGLE;
    obj->x = x;
    obj->y = y;
    obj->rectangle.width = width;
    obj->rectangle.height = height;
    obj->rectangle.color = color;

    uint16_t start_scanline = floor(y / TFT_SCANLINE_HEIGHT);
    uint16_t end_scanline = ceil((float)(height + obj->y) / TFT_SCANLINE_HEIGHT);

    ESP_LOGI(TAG, "Adding to scanlines [%d to %d]", start_scanline, end_scanline);

    for (int i = start_scanline; i <= end_scanline; i++) {
        for (int j = 0; j < TFT_SCENE_OBJECT_MAX; j++) {
            if (scanline_residents[i][j] == 0) {
                scanline_residents[i][j] = obj;
                break;
            }
        }
    }
}

void tft_render_scene() {
    for (size_t scanline_index = 0; scanline_index < TFT_HEIGHT / TFT_SCANLINE_HEIGHT; scanline_index++) {
        for (int j = 0; j < TFT_SCENE_OBJECT_MAX; j++) {
            // Render if we have a reference
            if (scanline_residents[scanline_index][j] != NULL) {
                scene_object_t *obj = scanline_residents[scanline_index][j];

                // Get the height in pixels so we can decide if we use object height or scanline height for rendering
                uint16_t scan_y_px_start = scanline_index * TFT_SCANLINE_HEIGHT;
                uint16_t scan_y_px_end = scan_y_px_start + TFT_SCANLINE_HEIGHT;

                // Set scanline window

                uint16_t x_start = obj->x;
                uint16_t x_end = obj->x + obj->rectangle.width;

                // clang-format off
                uint16_t y_start = obj->y > scan_y_px_start ? obj->y : scan_y_px_start;
                uint16_t y_end = obj->y + obj->rectangle.height < scan_y_px_end ? obj->y + obj->rectangle.height : scan_y_px_end;
                // clang-format on

                tft_set_window(x_start, y_start, x_end - 1, y_end - 1);

                tft_set_window(0, scan_y_px_start, TFT_WIDTH - 1, scan_y_px_start + TFT_SCANLINE_HEIGHT - 1);

                for (int x = 0; x < (x_end - x_start); x++) {
                    scanline[x * 2] = obj->rectangle.color >> 8;
                    scanline[x * 2 + 1] = obj->rectangle.color & 0xFF;
                }

                for (int y = 0; y < (y_end - y_start); y++) {
                    tft_send_data(scanline, (x_end - x_start) * 2);
                }

            } else {
                // Once we find an empty slot, we can skip the rest since they will be empty too
                break;
            }
        }
    }
}

void tft_test(void) {
    uint16_t color = 0xF800; // Red

    static uint8_t line[TFT_HEIGHT * 2];

    for (int x = 0; x < TFT_HEIGHT; x++) {
        line[x * 2] = color >> 8;
        line[x * 2 + 1] = color & 0xFF;
    }

    for (int y = 0; y < TFT_WIDTH; y++) {
        tft_send_data(line, sizeof(line));
    }
}

void tft_clear() {
    tft_set_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);

    uint16_t color = 0x0000;

    static uint8_t line[TFT_HEIGHT * 2];

    for (int x = 0; x < TFT_HEIGHT; x++) {
        line[x * 2] = color >> 8;
        line[x * 2 + 1] = color & 0xFF;
    }

    for (int y = 0; y < TFT_WIDTH; y++) {
        tft_send_data(line, sizeof(line));
    }
}

void tft_square_test() {
    uint32_t r = esp_random();

    uint16_t color = (r >> 16) & 0xFFFF;
    uint8_t pos_x = r & 0xFF;
    uint8_t pos_y = (r >> 8) & 0xFF;
    uint16_t size = ((r >> 16) & 0xFF) + 1;

    tft_set_window(pos_x, pos_y, pos_x + size - 1, pos_y + size - 1);

    uint8_t line[size * 2];

    for (int x = 0; x < size; x++) {
        line[x * 2] = color >> 8;
        line[x * 2 + 1] = color & 0xFF;
    }

    for (int y = 0; y < size; y++) {
        tft_send_data(line, sizeof(line));
    }
}