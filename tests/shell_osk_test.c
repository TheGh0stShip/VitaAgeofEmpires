/* The console's on-screen keyboard: layout, keys, Shift and moves. */
#include "shell_osk.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t frame[960 * 544];

int main(void) {
    AoeOskRor k;
    int32_t r[4], code = 0;
    int row = -1, col = -1;
    aoe_osk_init_ror(&k, 960, 544);
    /* Ten 88-pixel units, centred, along the bottom. */
    assert(k.unit_w == 88 && k.w == 896 && k.x == 32 && k.h == 216 && k.y == 328);
    assert(aoe_osk_row_keys_ror(0) == 10 && aoe_osk_row_keys_ror(4) == 4 &&
           aoe_osk_row_keys_ror(5) == 0);
    aoe_osk_key_rect_ror(&k, 1, 0, r);   /* q */
    assert(r[0] == 42 && r[1] == 378 && r[2] == 84 && r[3] == 36);
    assert(aoe_osk_hit_ror(&k, r[0] + 5, r[1] + 5, &row, &col) && row == 1 && col == 0);
    assert(!aoe_osk_hit_ror(&k, 10, 10, &row, &col));
    /* Shift starts on: letters are capitals, digits and signs unchanged. */
    assert(k.shift && aoe_osk_key_ror(&k, 1, 0, &code) == AOE_OSK_CHAR && code == 'Q');
    k.shift = 0;
    assert(aoe_osk_key_ror(&k, 1, 0, &code) == AOE_OSK_CHAR && code == 'q');
    assert(aoe_osk_key_ror(&k, 0, 9, &code) == AOE_OSK_CHAR && code == '0');
    assert(aoe_osk_key_ror(&k, 3, 9, &code) == AOE_OSK_CHAR && code == '_');
    /* The bottom row: Shift, Space (four units), Delete, Done. */
    assert(aoe_osk_key_ror(&k, 4, 0, &code) == AOE_OSK_SHIFT);
    assert(aoe_osk_key_ror(&k, 4, 1, &code) == AOE_OSK_CHAR && code == ' ');
    assert(aoe_osk_key_ror(&k, 4, 2, &code) == AOE_OSK_BACKSPACE && code == 8);
    assert(aoe_osk_key_ror(&k, 4, 3, &code) == AOE_OSK_ENTER);
    aoe_osk_key_rect_ror(&k, 4, 1, r);
    assert(r[0] == 32 + 8 + 2 * 88 + 2 && r[2] == 4 * 88 - 4);
    assert(aoe_osk_hit_ror(&k, r[0] + r[2] - 2, r[1] + 2, &row, &col) && row == 4 && col == 1);
    /* Moves wrap; a row change keeps the column under the key's centre:
       c (third key) down to the bottom row is Space, Done (units 8 and 9)
       up is the tenth key. */
    k.row = 3;
    k.col = 2;
    aoe_osk_move_ror(&k, 0, 1);
    assert(k.row == 4 && k.col == 1);
    aoe_osk_move_ror(&k, 1, 0);
    aoe_osk_move_ror(&k, 1, 0);
    assert(k.col == 3);
    aoe_osk_move_ror(&k, 1, 0);
    assert(k.col == 0);
    k.col = 3;
    aoe_osk_move_ror(&k, 0, -1);
    assert(k.row == 3 && k.col == 9);
    aoe_osk_move_ror(&k, 0, 2);
    assert(k.row == 0);
    aoe_osk_move_ror(&k, -1, 0);
    aoe_osk_move_ror(&k, 0, -1);
    assert(k.row == 4);
    /* Drawn only when visible, inside its rectangle. */
    memset(frame, 0x11, sizeof frame);
    aoe_osk_draw_ror(&k, frame, 960, 960, 544);
    assert(frame[960 * 500 + 480] == 0x11111111u);
    k.visible = 1;
    aoe_osk_draw_ror(&k, frame, 960, 960, 544);
    assert(frame[960 * 500 + 480] != 0x11111111u && frame[960 * 100 + 480] == 0x11111111u);
    /* A smaller frame is not overrun (the text and boxes clip to it; the
       sanitizer build checks the writes). */
    memset(frame, 0x11, sizeof frame);
    aoe_osk_init_ror(&k, 640, 480);
    k.visible = 1;
    aoe_osk_draw_ror(&k, frame, 640, 640, 480);
    assert(frame[640 * 480 + 5] == 0x11111111u);
    puts("osk ok");
    return 0;
}
