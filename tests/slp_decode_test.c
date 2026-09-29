#include "slp.h"

#include <assert.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
    put16(p, (uint16_t)value);
    put16(p + 2, (uint16_t)(value >> 16));
}

int main(void) {
    uint8_t bytes[95] = {0};
    uint32_t count = 0;
    SlpFrame frame = {0};

    memcpy(bytes, "2.0N", 4);
    put32(bytes + 4, 1);

    /* One 6x2 frame. Frame info starts at byte 32. */
    put32(bytes + 32, 72);             /* command-offset table */
    put32(bytes + 36, 64);             /* outline table */
    put32(bytes + 48, 6);
    put32(bytes + 52, 2);
    put32(bytes + 56, (uint32_t)-2);   /* hotspot x */
    put32(bytes + 60, 3);              /* hotspot y */

    put16(bytes + 64, 1);              /* row 0: left */
    put16(bytes + 66, 1);              /* row 0: right */
    put16(bytes + 68, 0);              /* row 1: left */
    put16(bytes + 70, 0);              /* row 1: right */

    put32(bytes + 72, 80);
    put32(bytes + 76, 87);

    /* Row 0 covers x=1..4: two normal colors, one player color, one shadow. */
    bytes[80] = 0x08;                  /* lesser draw, 2 */
    bytes[81] = 10;
    bytes[82] = 11;
    bytes[83] = 0x16;                  /* player-color draw, 1 */
    bytes[84] = 2;
    bytes[85] = 0x1b;                  /* shadow, 1 */
    bytes[86] = 0x0f;

    /* Row 1: skip 2, greater draw 2, then the two outline kinds. */
    bytes[87] = 0x09;                  /* lesser skip, 2 */
    bytes[88] = 0x02;                  /* greater draw */
    bytes[89] = 2;
    bytes[90] = 20;
    bytes[91] = 21;
    bytes[92] = 0x4e;                  /* outline 1 */
    bytes[93] = 0x6e;                  /* outline 2 */
    bytes[94] = 0x0f;

    assert(slp_frame_count(bytes, sizeof(bytes), &count) && count == 1);
    assert(slp_decode_frame(bytes, sizeof(bytes), 0, &frame));
    assert(frame.width == 6 && frame.height == 2);
    assert(frame.hotspot_x == -2 && frame.hotspot_y == 3);

    assert(frame.pixels[0].kind == SLP_TRANSPARENT);
    assert(frame.pixels[1].kind == SLP_COLOR && frame.pixels[1].index == 10);
    assert(frame.pixels[2].kind == SLP_COLOR && frame.pixels[2].index == 11);
    assert(frame.pixels[3].kind == SLP_PLAYER && frame.pixels[3].index == 2);
    assert(frame.pixels[4].kind == SLP_SHADOW);
    assert(frame.pixels[5].kind == SLP_TRANSPARENT);

    assert(frame.pixels[6].kind == SLP_TRANSPARENT);
    assert(frame.pixels[7].kind == SLP_TRANSPARENT);
    assert(frame.pixels[8].kind == SLP_COLOR && frame.pixels[8].index == 20);
    assert(frame.pixels[9].kind == SLP_COLOR && frame.pixels[9].index == 21);
    assert(frame.pixels[10].kind == SLP_OUTLINE1);
    assert(frame.pixels[11].kind == SLP_OUTLINE2);
    slp_free_frame(&frame);

    bytes[94] = 0x00;                  /* no end-of-row */
    assert(!slp_decode_frame(bytes, sizeof(bytes), 0, &frame));
    assert(frame.pixels == NULL && frame.width == 0 && frame.height == 0);

    memcpy(bytes, "2.0P", 4);
    assert(!slp_frame_count(bytes, sizeof(bytes), &count));
    return 0;
}
