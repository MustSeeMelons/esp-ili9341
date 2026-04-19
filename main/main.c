
#include "esp_log.h"
#include "include/tft.h"
#include <stdio.h>

static const char *TAG = "Main";

void app_main(void) {

    esp_err_t err = tft_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TFT: %s", esp_err_to_name(err));
        return;
    }

    tft_add_rectangle_to_scene(10, 10, 50, 50, 0xF80A);
    tft_render_scene();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
