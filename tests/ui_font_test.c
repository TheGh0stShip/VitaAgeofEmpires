/* GDI-style text: synthetic checks, and with arguments (ARIAL.TTF
   ARIALBD.TTF LANGUAGEX.DLL LANGUAGE.DLL [COPRGTL.TTF]) the user's fonts
   and strings. */
#include "ui_font.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pe_strings.h"

static AoePeStringsRor lang_x, lang_b;
static AoeFontFileRor regular, bold, copperplate;
static int resolved_copperplate;

static void load_string(void *c, int32_t id, char *buf, int32_t size) {
    (void)c;
    aoe_game_string_ror(&lang_x, &lang_b, id, buf, size);
}
static const AoeFontFileRor *resolve(void *c, const char *face, int want_bold) {
    (void)c;
    if (strcmp(face, "Arial") != 0) {
        if (strcmp(face, "Copperplate Gothic Light") == 0) {
            resolved_copperplate++;
            if (copperplate.info) return &copperplate;
        }
        return NULL;   /* the user has no such font */
    }
    return want_bold && bold.info ? &bold : &regular;
}
static void no_strings(void *c, int32_t id, char *buf, int32_t size) { (void)c; (void)id; if (size > 0) buf[0] = '\0'; }
static const AoeFontFileRor *no_fonts(void *c, const char *face, int b) { (void)c; (void)face; (void)b; return NULL; }

int main(int argc, char **argv) {
    uint8_t pal[1024];
    AoeUiFontTableRor table;
    int i;
    /* Windows-1252 and the nearest palette entry. */
    assert(aoe_cp1252_to_unicode_ror(0x80) == 0x20ac && aoe_cp1252_to_unicode_ror(0xae) == 0xae &&
           aoe_cp1252_to_unicode_ror('A') == 'A' && aoe_cp1252_to_unicode_ror(0x81) == 0x81);
    for (i = 0; i < 256; ++i) { pal[4 * i] = (uint8_t)i; pal[4 * i + 1] = 0; pal[4 * i + 2] = 0; pal[4 * i + 3] = 0; }
    pal[4 * 7 + 1] = 255;   /* entry 7: (7, 255, 0) */
    assert(aoe_ui_nearest_index_ror(pal, 0x00000064) == 100);   /* red 100 */
    assert(aoe_ui_nearest_index_ror(pal, 0x0000ff00) == 7);     /* green */
    /* No strings or no fonts: an empty table. */
    assert(aoe_ui_font_table_ror(&table, no_strings, no_fonts, NULL) == 0);
    /* A non-font file fails to open. */
    {
        static uint8_t junk[64];
        AoeFontFileRor f;
        uint8_t *copy = malloc(sizeof junk);
        memcpy(copy, junk, sizeof junk);
        assert(aoe_font_file_open_ror(&f, copy, sizeof junk, 0) == 0);
        aoe_font_file_free_ror(&f);
    }
    if (argc < 5) {
        puts("ui font ok (synthetic only)");
        return 0;
    }
    assert(aoe_font_file_load_ror(&regular, argv[1], 0) && aoe_font_file_load_ror(&bold, argv[2], 1));
    assert(aoe_pe_strings_load(&lang_x, argv[3]) && aoe_pe_strings_load(&lang_b, argv[4]));
    if (argc > 5) assert(aoe_font_file_load_ror(&copperplate, argv[5], 0));
    /* The game's table: slots 0, 1, 4 (Copperplate, falling back to Arial
       here), 7..11 Arial; 2, 3, 5 and 6 (an empty face) none. */
    assert(aoe_ui_font_table_ror(&table, load_string, resolve, NULL) == 8);
    assert(resolved_copperplate == 3);
    assert(!table.present[2] && !table.present[3] && !table.present[5] && !table.present[6]);
    assert(table.present[0] && table.present[1] && table.present[7] && table.present[8]);
    /* With the user's Copperplate Gothic Light the title slots use it,
       emboldened by one pixel (there is no bold face, as on Windows);
       without it they fall back to Arial bold. */
    if (copperplate.info)
        assert(table.fonts[0].file == &copperplate && table.fonts[1].file == &copperplate &&
               table.fonts[4].file == &copperplate && table.fonts[1].synth_bold);
    else
        assert(table.fonts[0].file == &bold && !table.fonts[0].synth_bold);
    printf("slot 1 (titles): %s, height %d\n", copperplate.info ? "Copperplate" : "Arial fallback",
           (int)table.fonts[1].height);
    /* Sizes: -MulDiv(points, 96, 72): 12 -> 16, 21 -> 28, 10 -> 13, 9 -> 12 pixels em. */
    assert(table.fonts[1].height > table.fonts[0].height && table.fonts[0].height > table.fonts[7].height);
    assert(table.fonts[8].strikeout && !table.fonts[11].strikeout && table.fonts[8].file == &bold &&
           table.fonts[7].file == &regular && !table.fonts[8].synth_bold);
    printf("arial 10: ascent %d descent %d height %d ave %d lead %d\n", (int)table.fonts[7].ascent,
           (int)table.fonts[7].descent, (int)table.fonts[7].height, (int)table.fonts[7].ave_width,
           (int)table.fonts[7].external_leading);
    /* Drawing "Single Player" (string 9202) into a buffer: pixels only in
       the given index and inside the clip; the width matches the measure. */
    {
        static uint8_t buf[64 * 200];
        char text[64];
        int32_t clip[4] = {0, 0, 199, 63}, w, minx = 999, maxx = -1, y, x, set = 0;
        load_string(NULL, 9202, text, sizeof text);
        assert(strcmp(text, "Single Player") == 0);
        w = aoe_ui_font_text_width_ror(&table.fonts[7], text, (int32_t)strlen(text));
        assert(w > 40 && w < 120);
        aoe_ui_font_draw_ror(&table.fonts[7], buf, 200, 200, 64, clip, 10, 5, text,
                             (int32_t)strlen(text), 42);
        for (y = 0; y < 64; ++y)
            for (x = 0; x < 200; ++x)
                if (buf[y * 200 + x]) {
                    assert(buf[y * 200 + x] == 42 && y >= 5 && y < 5 + table.fonts[7].height);
                    if (x < minx) minx = x;
                    if (x > maxx) maxx = x;
                    ++set;
                }
        assert(set > 50 && minx >= 10 && maxx < 10 + w);
        /* A narrower clip keeps everything inside it. */
        memset(buf, 0, sizeof buf);
        clip[2] = 30;
        aoe_ui_font_draw_ror(&table.fonts[7], buf, 200, 200, 64, clip, 10, 5, text,
                             (int32_t)strlen(text), 42);
        for (y = 0; y < 64; ++y)
            for (x = 31; x < 200; ++x) assert(buf[y * 200 + x] == 0);
        printf("\"%s\" width %d, %d pixels\n", text, (int)w, set);
    }
    aoe_font_file_free_ror(&regular);
    aoe_font_file_free_ror(&bold);
    aoe_font_file_free_ror(&copperplate);
    aoe_pe_strings_free(&lang_x);
    aoe_pe_strings_free(&lang_b);
    puts("ui font ok");
    return 0;
}
