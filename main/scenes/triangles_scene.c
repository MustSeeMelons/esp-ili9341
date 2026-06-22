#include "include/scene.h"
#include "include/tft.h"
#include "include/utils.h"

scene_object_t *triangle = NULL;

void triangles_init(void *ctx) {
    triangle = tft_add_triangle((vector2_t[3]){{.x = 50, .y = 50}, {.x = 100, .y = 50}, {.x = 75, .y = 100}},
                                get_color565(255, 0, 0));
}

void triangles_update(void *ctx) {
}

scene_t triangles_scene = {.scene_init = triangles_init, .scene_update = triangles_update};