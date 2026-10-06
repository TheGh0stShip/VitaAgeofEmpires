/* The port's Game Screen helpers: the 8-bit BMP reader and the view
   rectangle. */
#include "game_screen_host.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

/* A 3 x 2 image; rows padded to 4 bytes. */
static uint32_t make(uint8_t *b, int32_t height) {
    memset(b, 0, 1200);
    b[0] = 'B';
    b[1] = 'M';
    le32(b + 10, 54 + 1024);
    le32(b + 14, 40);
    le32(b + 18, 3);
    le32(b + 22, (uint32_t)height);
    b[26] = 1;
    b[28] = 8;
    /* stored rows: first {1,2,3}, second {4,5,6} */
    memcpy(b + 1078, "\x01\x02\x03\x00\x04\x05\x06\x00", 8);
    return 1078 + 8;
}

int main(void) {
    uint8_t b[1200], *px;
    int32_t w = 0, h = 0, r[4];
    uint32_t n = make(b, 2);
    px = aoe_game_screen_host_bmp_ror(b, n, &w, &h);
    /* Bottom-up: the last stored row is the top. */
    assert(px && w == 3 && h == 2 && memcmp(px, "\x04\x05\x06\x01\x02\x03", 6) == 0);
    free(px);
    n = make(b, -2);
    px = aoe_game_screen_host_bmp_ror(b, n, &w, &h);
    assert(px && memcmp(px, "\x01\x02\x03\x04\x05\x06", 6) == 0);
    free(px);
    assert(!aoe_game_screen_host_bmp_ror(b, n - 1, &w, &h));   /* truncated */
    b[28] = 24;
    assert(!aoe_game_screen_host_bmp_ror(b, n, &w, &h));       /* not 8-bit */
    aoe_game_screen_host_view_rect_ror(640, 480, r);
    assert(r[0] == 0 && r[1] == 0x14 && r[2] == 640 && r[3] == 0x14e);
    aoe_game_screen_host_view_rect_ror(800, 600, r);
    assert(r[2] == 800 && r[3] == 0x1c6);
    aoe_game_screen_host_view_rect_ror(1024, 768, r);
    assert(r[2] == 0x400 && r[3] == 0x26e);
    puts("game screen host ok");
    return 0;
}
