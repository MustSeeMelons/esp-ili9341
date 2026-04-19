#ifndef UTILS_H_
#define UTILS_H_

#include "stdint.h"

uint16_t get_color565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t red = (r >> 3) << 11;
    uint16_t green = (g >> 2) << 5;
    uint16_t blue = (b >> 3);

    return red | green | blue;
}

#endif