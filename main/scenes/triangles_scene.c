#include "include/scene.h"
#include "include/tft.h"
#include "include/utils.h"

extern uint16_t curr_display_height;
extern uint16_t curr_display_width;

scene_object_t *triangle = NULL;

void triangles_init(void *ctx) {
    // clang-format off
    triangle = tft_add_triangle((vector2_t[3]){
        {.x = 0, .y = 0},
        {.x = curr_display_width, .y = curr_display_height /2 },
        {.x = 0, .y = curr_display_height}
    }, get_color565(255, 0, 0));
    // clang-format on
}

void triangles_update(void *ctx) {
}

scene_t triangles_scene = {.scene_init = triangles_init, .scene_update = triangles_update};