#include "include/scene.h"
#include "include/tft.h"
#include "include/utils.h"

extern uint16_t curr_display_height;
extern uint16_t curr_display_width;

scene_object_t *line = NULL;

void lines_init(void *ctx) {
    line = tft_add_line(0, 0, curr_display_width, curr_display_height, get_color565(0, 255, 0), 1);
}

void lines_update(void *ctx) {
}

scene_t line_scene = {.scene_init = lines_init, .scene_update = lines_update};