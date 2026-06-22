#ifndef UTILS_H_
#define UTILS_H_

#include "include/tft.h"
#include "stdint.h"

uint16_t get_color565(uint8_t r, uint8_t g, uint8_t b);

float lerp_f(float start, float end, float blend_factor);

int16_t lerp_i(int16_t start, int16_t end, float blend_factor);

float get_blend_factor(int16_t value, int16_t value_start, int16_t value_end);

int16_t get_min_x(vector2_t vertices[3]);

int16_t get_min_y(vector2_t vertices[3]);

int16_t get_max_x(vector2_t vertices[3]);

int16_t get_max_y(vector2_t vertices[3]);

int32_t edge_function(vector2_t a, vector2_t b, vector2_t c);

#endif