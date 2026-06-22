#include "./include/utils.h"

uint16_t get_color565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t red = (r & 0xf8) << 8;
    uint16_t green = (g & 0xfc) << 3;
    uint16_t blue = (b & 0xf8) >> 3;

    return red | green | blue;
}

float lerp_f(float start, float end, float blend_factor) {
    return start + blend_factor * (end - start);
}

int16_t lerp_i(int16_t start, int16_t end, float blend_factor) {
    return (int16_t)(start + blend_factor * (end - start));
}
float get_blend_factor(int16_t value, int16_t value_start, int16_t value_end) {
    if (value <= value_start) {
        return 0.0f;
    } else if (value >= value_end) {
        return 1.0f;
    } else {
        return (float)(value - value_start) / (value_end - value_start);
    }
}

int16_t get_min_x(vector2_t vertices[3]) {
    int16_t min_x = vertices[0].x;

    for (int i = 1; i < 3; i++) {
        if (vertices[i].x < min_x) {
            min_x = vertices[i].x;
        }
    }

    return min_x;
}

int16_t get_max_x(vector2_t vertices[3]) {
    int16_t max_x = vertices[0].x;

    for (int i = 1; i < 3; i++) {
        if (vertices[i].x > max_x) {
            max_x = vertices[i].x;
        }
    }

    return max_x;
}

int16_t get_min_y(vector2_t vertices[3]) {
    int16_t min_y = vertices[0].y;

    for (int i = 1; i < 3; i++) {
        if (vertices[i].y < min_y) {
            min_y = vertices[i].y;
        }
    }

    return min_y;
}

int16_t get_max_y(vector2_t vertices[3]) {
    int16_t max_y = vertices[0].y;

    for (int i = 1; i < 3; i++) {
        if (vertices[i].y > max_y) {
            max_y = vertices[i].y;
        }
    }

    return max_y;
}

int32_t edge_function(vector2_t a, vector2_t b, vector2_t c) {
    return (int32_t)(b.x - a.x) * (c.y - a.y) - (int32_t)(b.y - a.y) * (c.x - a.x);
}
