
#include "esp_log.h"
#include "esp_timer.h"
#include "include/defs.h"
#include "include/scene.h"
#include "include/tft.h"
#include "include/utils.h"
#include <stdio.h>

static const char *TAG = "Main";

static void app_do_fps(scene_object_t *text) {
    static uint32_t last_frame = 0;
    static float fps = 0.0f;

    int64_t now = esp_timer_get_time() / 1000;

    if (last_frame != 0) {
        uint32_t frame_time = now - last_frame;

        if (frame_time > 0) {
            float current_fps = 1000.0f / frame_time;

            // Smooth out sudden changes
            fps = fps * 0.8f + current_fps * 0.2f;
        }

        static char fps_text[32];
        snprintf(fps_text, sizeof(fps_text), "%.1f FPS (%lu ms)", fps, frame_time);
        text->text.text = fps_text;
    }

    last_frame = now;
}

extern scene_t egg_fall_scene;

void app_main(void) {

    tft_init_config_t config = {.rotation = TFT_Landscape};

    esp_err_t err = tft_init(config);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TFT: %s", esp_err_to_name(err));
        return;
    }

    egg_fall_scene.scene_init(NULL);

    // FPS text should be added last
    scene_object_t *fps_text = tft_add_text(0, 0, "FPS: 0", 1, get_color565(255, 255, 255));

    tft_render_scene();

    while (1) {
        egg_fall_scene.scene_update(NULL);

        app_do_fps(fps_text);
        tft_render_scene();
    }
}
