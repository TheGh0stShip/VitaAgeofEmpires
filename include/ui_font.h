#ifndef VITA_AOE_UI_FONT_H
#define VITA_AOE_UI_FONT_H

#include <stddef.h>
#include <stdint.h>

#include "game_frame.h"   /* AoeLogFontRor */

/* GDI-style text for the translated UI without Windows: the game's font
   table (0x00418e70 IDs, 0x00419390 descriptions) realised from
   user-supplied TrueType files (the Windows fonts the original asked GDI
   for: Arial, and Copperplate Gothic Light when the user has it), drawn
   on 8-bit palette surfaces. Integration support, not an original
   function. Deviations from GDI, all host-checkable: no TrueType hinting
   (stb_truetype), unhinted advances rounded to whole pixels, and
   monochrome glyphs (an 8-bit palette DC does not smooth text). */

typedef struct {
    uint8_t *data;              /* the whole .ttf (owned) */
    size_t size;
    void *info;                 /* stbtt_fontinfo (owned) */
    int bold;                   /* a bold face (e.g. arialbd.ttf) */
    /* OS/2 values in font units. */
    int32_t units_per_em, win_ascent, win_descent, avg_width, line_gap, hhea_span;
} AoeFontFileRor;

/* Loads a TrueType file; `bold` marks a bold face. 0 when unreadable. */
int aoe_font_file_load_ror(AoeFontFileRor *f, const char *path, int bold);
/* From memory (takes ownership of malloc'd `data`). */
int aoe_font_file_open_ror(AoeFontFileRor *f, uint8_t *data, size_t size, int bold);
void aoe_font_file_free_ror(AoeFontFileRor *f);

typedef struct {
    const AoeFontFileRor *file;
    float scale;                /* pixels per font unit */
    int synth_bold;             /* weight 700 on a regular face: +1 px overstrike */
    int strikeout;
    /* TEXTMETRIC: tmAscent, tmDescent, tmHeight, tmAveCharWidth,
       tmExternalLeading. */
    int32_t ascent, descent, height, ave_width, external_leading;
} AoeUiFontRor;

/* CreateFontIndirectA + GetTextMetricsA: a negative lfHeight is the em
   size in pixels, a positive one the cell height (ascent + descent).
   0 without a file. */
int aoe_ui_font_create_ror(AoeUiFontRor *font, const AoeFontFileRor *file,
                           const AoeLogFontRor *lf);
/* GetTextExtentPoint32A width of `count` Windows-1252 bytes. */
int32_t aoe_ui_font_text_width_ror(const AoeUiFontRor *font, const char *text, int32_t count);
/* TextOutA (transparent background) with the cell's top-left at (x, y),
   in palette index `index`, clipped to [left, right] x [top, bottom]
   (inclusive) of a top-down 8-bit buffer. */
void aoe_ui_font_draw_ror(const AoeUiFontRor *font, uint8_t *bits, int32_t pitch, int32_t width,
                          int32_t height, const int32_t clip[4], int32_t x, int32_t y,
                          const char *text, int32_t count, uint8_t index);

/* Windows-1252 byte to Unicode (unassigned bytes map to themselves). */
uint16_t aoe_cp1252_to_unicode_ror(uint8_t c);
/* GetNearestPaletteIndex for a COLORREF (0x00bbggrr) against 256 RGBX
   palette entries: the least squared distance, the lowest index on ties. */
uint8_t aoe_ui_nearest_index_ror(const uint8_t palette[1024], uint32_t colorref);

/* The game's twelve-entry font table (game +0x1e4): for each index the
   0x00418e70 string ID, the face / size / style strings (0x00419390),
   the face file from `resolve` (face name, bold wanted; NULL when the
   user has no such font; the caller then falls back, as Windows font
   mapping would, by asking for "Arial"). */
enum { AOE_UI_FONT_SLOTS = 12 };
typedef struct {
    AoeUiFontRor fonts[AOE_UI_FONT_SLOTS];
    int present[AOE_UI_FONT_SLOTS];
    int32_t width[AOE_UI_FONT_SLOTS], height[AOE_UI_FONT_SLOTS]; /* +4, +8 of each record */
} AoeUiFontTableRor;
typedef const AoeFontFileRor *(*AoeFontResolveRor)(void *context, const char *face, int bold);
int aoe_ui_font_table_ror(AoeUiFontTableRor *t,
                          void (*load_string)(void *context, int32_t id, char *buf, int32_t size),
                          AoeFontResolveRor resolve, void *context);

#endif
