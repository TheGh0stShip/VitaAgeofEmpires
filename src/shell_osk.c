/* The console's on-screen keyboard (see shell_osk.h). */
#include "shell_osk.h"

#include <string.h>

#include "demo_app.h"

/* Rows of ten single-unit keys, then the bottom row of wide keys. */
static const char *const rows[4] = {"1234567890", "qwertyuiop", "asdfghjkl-", "zxcvbnm,._"};

typedef struct {
    AoeOskKeyKind kind;
    int32_t code;
    int units;
    const char *label;
} BottomKey;

static const BottomKey bottom[4] = {
    {AOE_OSK_SHIFT, 0, 2, "SHIFT"},
    {AOE_OSK_CHAR, ' ', 4, "SPACE"},
    {AOE_OSK_BACKSPACE, 8, 2, "DEL"},
    {AOE_OSK_ENTER, 13, 2, "DONE"}};

enum { MARGIN = 8, GAP = 4 };

void aoe_osk_init_ror(AoeOskRor *k, int32_t out_w, int32_t out_h) {
    memset(k, 0, sizeof *k);
    k->unit_w = (out_w - 2 * MARGIN) / AOE_OSK_UNITS;
    if (k->unit_w > 88) k->unit_w = 88;
    k->row_h = 40;
    k->w = k->unit_w * AOE_OSK_UNITS + 2 * MARGIN;
    k->h = k->row_h * AOE_OSK_ROWS + 2 * MARGIN;
    k->x = (out_w - k->w) / 2;
    k->y = out_h - k->h;
    k->row = 1;
    k->col = 0;
    k->shift = 1;
}

int aoe_osk_row_keys_ror(int row) {
    if (row < 0 || row >= AOE_OSK_ROWS) return 0;
    return row < 4 ? AOE_OSK_UNITS : 4;
}

/* The first unit and width (in units) of key (row, col). */
static void key_units(int row, int col, int *first, int *units) {
    int i;
    if (row < 4) {
        *first = col;
        *units = 1;
        return;
    }
    *first = 0;
    for (i = 0; i < col; ++i) *first += bottom[i].units;
    *units = bottom[col].units;
}

void aoe_osk_key_rect_ror(const AoeOskRor *k, int row, int col, int32_t r[4]) {
    int first, units;
    key_units(row, col, &first, &units);
    r[0] = k->x + MARGIN + first * k->unit_w + GAP / 2;
    r[1] = k->y + MARGIN + row * k->row_h + GAP / 2;
    r[2] = units * k->unit_w - GAP;
    r[3] = k->row_h - GAP;
}

int aoe_osk_hit_ror(const AoeOskRor *k, int32_t x, int32_t y, int *row, int *col) {
    int r, c;
    for (r = 0; r < AOE_OSK_ROWS; ++r)
        for (c = 0; c < aoe_osk_row_keys_ror(r); ++c) {
            int32_t b[4];
            aoe_osk_key_rect_ror(k, r, c, b);
            if (x >= b[0] && y >= b[1] && x < b[0] + b[2] && y < b[1] + b[3]) {
                *row = r;
                *col = c;
                return 1;
            }
        }
    return 0;
}

AoeOskKeyKind aoe_osk_key_ror(const AoeOskRor *k, int row, int col, int32_t *code) {
    if (row < 0 || row >= AOE_OSK_ROWS || col < 0 || col >= aoe_osk_row_keys_ror(row))
        return AOE_OSK_NONE;
    if (row < 4) {
        char c = rows[row][col];
        if (k->shift && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        *code = (unsigned char)c;
        return AOE_OSK_CHAR;
    }
    *code = bottom[col].code;
    return bottom[col].kind;
}

void aoe_osk_move_ror(AoeOskRor *k, int dx, int dy) {
    int n = aoe_osk_row_keys_ror(k->row);
    if (dx) k->col = ((k->col + dx) % n + n) % n;
    if (dy) {
        /* Keep the column nearest the key's centre. */
        int first, units, centre, r = ((k->row + dy) % AOE_OSK_ROWS + AOE_OSK_ROWS) % AOE_OSK_ROWS,
                                     c;
        key_units(k->row, k->col, &first, &units);
        centre = first * 2 + units;   /* in half units */
        k->row = r;
        for (c = 0; c < aoe_osk_row_keys_ror(r); ++c) {
            key_units(r, c, &first, &units);
            if (centre < (first + units) * 2) break;
        }
        k->col = c < aoe_osk_row_keys_ror(r) ? c : aoe_osk_row_keys_ror(r) - 1;
    }
}

void aoe_osk_draw_ror(const AoeOskRor *k, uint32_t *out, int32_t stride, int32_t out_w,
                      int32_t out_h) {
    int r, c;
    if (!k->visible) return;
    aoe_demo_fill_rect_in(out, (size_t)stride, out_w, out_h, k->x, k->y, k->w, k->h, 0xff000000u,
                          1);
    for (r = 0; r < AOE_OSK_ROWS; ++r)
        for (c = 0; c < aoe_osk_row_keys_ror(r); ++c) {
            int32_t b[4], code = 0;
            char label[8];
            const char *text = label;
            int highlighted = r == k->row && c == k->col;
            int lit = r == 4 && c == 0 && k->shift;
            uint32_t face = highlighted ? 0xff1c8cd8u : lit ? 0xff3c6e3cu : 0xff4a4a4au;
            aoe_osk_key_rect_ror(k, r, c, b);
            aoe_demo_fill_rect_in(out, (size_t)stride, out_w, out_h, b[0], b[1], b[2], b[3],
                                  0xffc8c8c8u, 0);
            aoe_demo_fill_rect_in(out, (size_t)stride, out_w, out_h, b[0] + 1, b[1] + 1, b[2] - 2,
                                  b[3] - 2, face, 0);
            if (r < 4) {
                aoe_osk_key_ror(k, r, c, &code);
                label[0] = (char)code;
                label[1] = '\0';
            } else {
                text = bottom[c].label;
            }
            aoe_demo_draw_text_in(out, (size_t)stride, out_w, out_h,
                                  b[0] + (b[2] - (int32_t)strlen(text) * 12 + 2) / 2,
                                  b[1] + (b[3] - 14) / 2, 2, 0xffffffffu, text);
        }
}
