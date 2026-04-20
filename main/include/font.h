#ifndef FONT_H_
#define FONT_H_

#include "stdint.h"

struct Glyph {
    uint8_t width;     // width in pixels
    uint8_t height;    // height in pixels (usually same for all glyphs)
    int8_t x_offset;   // horizontal offset from cursor
    int8_t y_offset;   // vertical offset from baseline
    uint8_t x_advance; // how much to move cursor after drawing

    const uint8_t *bitmap; // row-major bitmap
};

struct Font {
    uint8_t height;          // full height for now, no fancy baselines
    uint8_t first_char_code; // usually 32 for space
    uint8_t last_char_code;  // usually 126 for tilde

    const Glyph *glyphs; // array of glyphs
};

#endif