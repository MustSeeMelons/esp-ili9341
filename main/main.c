
#include "esp_log.h"
#include "include/defs.h"
#include "include/tft.h"
#include "include/utils.h"
#include <stdio.h>

static const char *TAG = "Main";

void app_main(void) {

    esp_err_t err = tft_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TFT: %s", esp_err_to_name(err));
        return;
    }

    scene_object_t *obj = tft_add_rectangle(0, 0, 50, 50, get_color565(255, 255, 255));
    tft_render_scene();

    while (1) {
        // obj->x = (obj->x + 1) % TFT_WIDTH;
        // obj->y = (obj->y + 1) % TFT_HEIGHT;

        // update_scanline_residents();

        tft_render_scene();
        // vTaskDelay(1);
    }
}
