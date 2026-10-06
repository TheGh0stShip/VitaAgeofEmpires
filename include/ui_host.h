#ifndef VITA_AOE_UI_HOST_H
#define VITA_AOE_UI_HOST_H

#include <stddef.h>
#include <stdint.h>

#include "drs.h"
#include "draw_system.h"
#include "pe_strings.h"
#include "shape.h"
#include "soft_draw.h"
#include "ui_ez.h"
#include "ui_font.h"
#include "ui_panel.h"

/* The platform beneath the translated panel system (ui_panel.h, ui_ez.h):
   the user's original data (DRS archives, language libraries, TrueType
   fonts) and the software draw system, without Windows. It implements
   the GDI / DirectDraw / resource calls the screens make; the screens
   themselves are the translations. Integration support, not an original
   function. Platform layers (Vita, host tools) own input and the
   display, presenting aoe_soft_draw_present_abgr_ror of `display`.

   Data root layout (names matched case-insensitively):
     data/   base game: Interfac.drs, graphics.drs, sounds.drs, ...
     data2/  Rise of Rome: INTERFAC.DRS, GRAPHICS.DRS, SOUNDS.DRS
     languagex.dll / language.dll (or LANGUA_1.DLL) in the root, data2/
       or data/
     fonts/  arial.ttf, arialbd.ttf and optionally COPRGTL.TTF */

enum { AOE_UI_HOST_ARCHIVES = 8, AOE_UI_HOST_PALETTES = 8 };

typedef struct {
    char name[64];
    int32_t id;
    uint8_t entries[1024];      /* RGBX */
    int used;
} AoeUiHostPaletteRor;

typedef struct AoeUiHostRor {
    DrsArchive archives[AOE_UI_HOST_ARCHIVES];
    DrsArchive *order[AOE_UI_HOST_ARCHIVES];
    size_t archive_count;
    AoePeStringsRor lang_x, lang_b;
    AoeFontFileRor arial, arial_bold, copperplate;
    AoeUiFontTableRor fonts;
    AoeSoftDisplayRor display;
    AoeDrawOpsRor draw_ops;
    AoeDrawSystemRor draw;
    AoeUiAreaRor area;
    AoeUiPlatformRor platform;
    AoeUiEzOpsRor ez_ops;
    AoeShapeOpsRor shape_ops;
    AoeShapeDrawOpsRor shape_draw;
    int32_t shape_batching, shapes_drawn, shape_loads;
    AoeUiSystemRor system;
    AoeUiHostPaletteRor palettes[AOE_UI_HOST_PALETTES];
    uintptr_t area_palette, default_palette;
    /* The text context between text_begin and text_end. */
    const AoeUiFontRor *text_font;
    int32_t text_clip[4];
    uint8_t *text_bits;
    /* The clip of the last platform clip call (inclusive; whole area when
       none). */
    int32_t clip[4], clipped;
    uint32_t (*now_ms)(void *context);
    void *now_context;
    char root[512];
} AoeUiHostRor;

/* Opens the data under `root` and a `width` x `height` 8-bit display.
   0 with a reason in `error` when the archives or language libraries
   cannot be read (fonts are optional: without them text is skipped). */
int aoe_ui_host_open_ror(AoeUiHostRor *h, const char *root, int32_t width, int32_t height,
                         uint32_t (*now_ms)(void *context), void *now_context, char *error,
                         size_t error_size);
void aoe_ui_host_close_ror(AoeUiHostRor *h);

/* A resource's bytes (malloc'd) from the archives in search order, or
   NULL. `type` is the executable's constant ('slp ' 0x736c7020, 'bina'
   0x62696e61, 'wav ' 0x77617620). */
uint8_t *aoe_ui_host_resource_ror(AoeUiHostRor *h, uint32_t type, int32_t id, uint32_t *size);

/* Paints the current screen (its handle_paint) and presents the back
   buffer to the primary. */
void aoe_ui_host_paint_ror(AoeUiHostRor *h);

#endif
