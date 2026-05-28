#ifndef UTILS_H_
#define UTILS_H_

#include "stdint.h"

static inline uint16_t get_color565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t red = (r & 0xf8) << 8;
    uint16_t green = (g & 0xfc) << 3;
    uint16_t blue = (b & 0xf8) >> 3;

    return red | green | blue;
}

static inline uint16_t lerp(uint16_t v0, uint16_t v1, float t) {
    return (1 - t) * v0 + t * v1;
}

#endif