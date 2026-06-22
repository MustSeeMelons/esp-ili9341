#include "include/scene.h"
#include "include/tft.h"
#include "include/utils.h"
#include <math.h>

extern uint16_t curr_display_height;
extern uint16_t curr_display_width;

scene_object_t *background = NULL;
scene_object_t *large_circle = NULL;
scene_object_t *small_circle = NULL;

uint16_t min_radius = 30;
uint16_t min_radius_other = 25;
uint16_t max_one = 90;
uint16_t max_other = 30;

float t = 0.0f;

void egg_fall_init(void *ctx) {
    background = tft_add_rectangle(0, 0, curr_display_width, curr_display_height, get_color565(0, 0, 0));
    large_circle =
        tft_add_circle(curr_display_width / 2, curr_display_height / 2, min_radius, get_color565(255, 255, 255));
    small_circle =
        tft_add_circle(curr_display_width / 2, curr_display_height / 2, min_radius_other, get_color565(255, 255, 0));
}

void egg_fall_update(void *ctx) {
    t += 0.1f;

    float scale_one = (sinf(t) + 1.0f) / 2.0f; // Scale between 0 and 1
    float scale_other = (cosf(t) + 1.0f) / 2.0f;
    uint16_t radius = (uint16_t)(min_radius + max_one * scale_one);
    uint16_t radius_other = (uint16_t)(min_radius_other + max_other * scale_other);

    large_circle->circle.radius = radius;
    small_circle->circle.radius = radius_other;

    update_scanline_residents();
}

scene_t egg_fall_scene = {.scene_init = egg_fall_init, .scene_update = egg_fall_update};