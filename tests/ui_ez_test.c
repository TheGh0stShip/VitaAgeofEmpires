/* Patched RoR 1.0a easy panel 0x00454430..0x00457e50 and the value
   scrollbars 0x00468360..0x00468540. */
#include "ui_dialog.h"
#include "ui_ez.h"
#include "scenario_editor_ui.h"
#include "palette_fade.h"
#include "shape.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t now(void *c) { (void)c; return 0; }
/* A one-frame SHP picture: 4 x 2, hotspot (1, 1); row 0 two literal
   pixels 0xa1 0xb2 then a skip, row 1 a fill of three 0xc3. */
static uint8_t shp_data[0x40];
static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static void build_shp(void) {
    static const uint8_t rows[] = {0x05, 0xa1, 0xb2, 0x01, 1, 0x00,
                                   0x06, 0xc3, 0x00};
    uint8_t *f = shp_data + 0x10;
    memcpy(shp_data, "1.10", 4);
    put32(shp_data + 4, 1);
    put32(shp_data + 8, 0x10);
    put32(f, 4u << 16 | 2u);
    put32(f + 8, (uint32_t)-1);
    put32(f + 0xc, (uint32_t)-1);
    put32(f + 0x10, 2);
    put32(f + 0x14, 0);
    memcpy(f + 0x18, rows, sizeof rows);
}
static uint8_t *shp_res(void *c, uint32_t type, int32_t id, uint32_t *size,
                        int32_t *owned) {
    (void)c; (void)id;
    if (type != 0x73687020u) return NULL; /* only as 'shp ' */
    *size = sizeof shp_data;
    *owned = 0;
    return shp_data;
}
/* A fake device whose surfaces are plain memory. */
typedef struct { int32_t w, h; uint8_t *bits; } MemSurface;
static int32_t mem_create(void *c, void *d, int32_t w, int32_t h, int sys,
                          void **out) {
    MemSurface *m = calloc(1, sizeof *m);
    (void)c; (void)d; (void)sys;
    m->w = w; m->h = h;
    m->bits = calloc((size_t)w, (size_t)h);
    *out = m;
    return 0;
}
static int32_t mem_describe(void *c, void *s, int32_t *pitch, uint32_t *caps) {
    (void)c;
    *pitch = ((MemSurface *)s)->w;
    *caps = 0x800;
    return 0;
}
static int32_t mem_fill(void *c, void *s, const int32_t *r, uint32_t color) {
    MemSurface *m = s;
    (void)c;
    if (!r) memset(m->bits, (int)color, (size_t)m->w * (size_t)m->h);
    return 0;
}
static int32_t mem_lock(void *c, void *s, int wait, uint8_t **bits) {
    (void)c; (void)wait;
    *bits = ((MemSurface *)s)->bits;
    return 0;
}
static void mem_unlock(void *c, void *s, uint8_t *bits) {
    (void)c; (void)s; (void)bits;
}
static void mem_release(void *c, void *o) {
    (void)c;
    free(((MemSurface *)o)->bits);
    free(o);
}
static int blits;
static int32_t mem_blt(void *c, void *d, const int32_t dr[4], void *s,
                       const int32_t sr[4], int keyed) {
    (void)c; (void)d; (void)dr; (void)s; (void)sr; (void)keyed;
    ++blits;
    return 0;
}

static uint32_t ticks;
static uint32_t ticking(void *c) { (void)c; return ticks += 50; }
static int palette_sets, fills, presents;
static int32_t fake_palette_set(void *c, void *p, int32_t s, int32_t n,
                                const uint8_t *in) {
    (void)c; (void)p; (void)s; (void)n; (void)in;
    ++palette_sets;
    return 0;
}
static int32_t fake_fill(void *c, void *s, const int32_t *r, uint32_t color) {
    (void)c; (void)s; (void)r; (void)color;
    ++fills;
    return 0;
}
static int32_t fake_blt(void *c, void *d, const int32_t dr[4], void *s,
                        const int32_t sr[4], int keyed) {
    (void)c; (void)d; (void)dr; (void)s; (void)sr; (void)keyed;
    ++presents;
    return 0;
}
static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }
static void string(void *c, int32_t id, char *out, int size) {
    (void)c;
    if (id == 7) snprintf(out, (size_t)size, "Seven");
    else if (id == 120055) snprintf(out, (size_t)size, "Help text");
    else out[0] = '\0';
}
static char palette_key[64];
static uintptr_t create_palette(void *c, const char *name, int32_t id) {
    (void)c; (void)id;
    snprintf(palette_key, sizeof(palette_key), "%s", name);
    return 0x5000;
}
static void delete_palette(void *c, uintptr_t p) { (void)c; (void)p; }

static int fonts(void *c, int32_t index, uintptr_t *font, int32_t *w, int32_t *h) {
    (void)c;
    *font = 0x100 + (uintptr_t)index;
    *w = index;
    *h = 10 + index;
    return 1;
}
static int sound_obj;
static void *sound(void *c, int32_t id) { (void)c; return id == 3 ? &sound_obj : NULL; }

/* Shapes are heap tokens recording their file and ID. */
typedef struct { char file[64]; int32_t id; } Shape;
static int shapes_live;
static void *load_shape(void *c, const char *file, int32_t id) {
    Shape *s;
    (void)c;
    if (file && strcmp(file, "missing.shp") == 0) return NULL;
    s = calloc(1, sizeof(*s));
    snprintf(s->file, sizeof(s->file), "%s", file ? file : "");
    s->id = id;
    ++shapes_live;
    return s;
}
static void free_shape(void *c, void *s) { (void)c; free(s); --shapes_live; }
static int shadows_live, last_amount;
static void *create_shadow(void *c, AoeUiAreaRor *a, int32_t amount, uintptr_t pal) {
    (void)c; (void)a; (void)pal;
    last_amount = amount;
    ++shadows_live;
    return malloc(1);
}
static void free_shadow(void *c, void *s) { (void)c; free(s); --shadows_live; }

static const char *info_text;
static char info_file[64];
static int32_t info_id;
static char *load_info(void *c, const char *file, int32_t id) {
    (void)c;
    snprintf(info_file, sizeof(info_file), "%s", file ? file : "");
    info_id = id;
    if (file && strcmp(file, "gone.sin") == 0) return NULL;
    return info_text ? strdup(info_text) : NULL;
}

static char pointer_name[64];
static int32_t pointer_id = -9;
static int has_pointer(void *c) { (void)c; return 1; }
static void set_pointer(void *c, const char *n, int32_t id) {
    (void)c;
    snprintf(pointer_name, sizeof(pointer_name), "%s", n);
    pointer_id = id;
}
static uintptr_t area_pal = 1;
static uintptr_t area_palette(void *c, AoeUiAreaRor *a) { (void)c; (void)a; return area_pal; }
static uintptr_t default_palette(void *c) { (void)c; return 0x77; }
static void set_area_palette(void *c, AoeUiAreaRor *a, uintptr_t p) { (void)c; (void)a; area_pal = p; }
static int ime_state = -1;
static void ime(void *c, int on) { (void)c; ime_state = on; }
static int cursor_help = -1, cursor_mode = -1;
static void cursor(void *c, int help, int32_t mode) { (void)c; cursor_help = help; cursor_mode = mode; }
static int32_t pstate = 42;
static int32_t pointer_state(void *c) { (void)c; return pstate; }
static void set_pointer_state(void *c, int32_t s) { (void)c; pstate = s; }

static AoeUiMessageDialogRor *message(AoeUiSystemRor *s, const char *name) {
    return (AoeUiMessageDialogRor *)aoe_ui_system_find_ror(s, name);
}

static int win_helps;
static void win_help(void *c) { (void)c; ++win_helps; }
static uint8_t help_mode;

static void free_panel(void *p) {
    AoeUiPanelRor *panel = p;
    if (!panel) return;
    panel->vt->destroy(panel);
    free(panel);
}

static const char *INFO =
    "s640 bg640 a640 1 2   s800 bg800 a800 3 4   s1024 bg1024 a1024 5 6 "
    "palette pal 50500   pointer arrow 51000   shadow x 3   pictures btn 50721 "
    "popup popinfo 50400   mode 2   fill 12   bevel 1 2 3 4 5 6 "
    "text 255 0 0   shadow 0 1 2   focus 0 255 0   fshadow 3 4 5 "
    "hot 0 0 255   hshadow 6 7 8";

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 800, 600};
    static AoeUiEzRor ez;
    AoeUiEzOpsRor ops = {0};

    pf.now_ms = now;
    pf.text_width = width;
    pf.string = string;
    pf.create_palette = create_palette;
    pf.delete_palette = delete_palette;
    sys.platform = &pf;
    ops.font = fonts;
    ops.sound = sound;
    ops.load_shape = load_shape;
    ops.free_shape = free_shape;
    ops.create_shadow = create_shadow;
    ops.free_shadow = free_shadow;
    ops.load_info = load_info;
    ops.has_pointer = has_pointer;
    ops.set_pointer = set_pointer;
    ops.area_palette = area_palette;
    ops.default_palette = default_palette;
    ops.set_area_palette = set_area_palette;
    ops.ime = ime;
    ops.cursor = cursor;
    ops.pointer_state = pointer_state;
    ops.set_pointer_state = set_pointer_state;
    ops.win_help = win_help;
    ops.help_mode = &help_mode;
    ops.caret_blink_ms = 530;

    /* Defaults. */
    assert(aoe_ui_ez_init_named_ror(&ez, &sys, &ops, "Main"));
    assert(ez.ref_w == 640 && ez.ref_h == 480 && ez.info_id == -1 &&
           ez.pointer_id == -1 && ez.popup_id == -1 &&
           ez.text_color == 0xffffff && ez.focus_color == 0xffff &&
           ez.hot_color == 0xffffff);

    /* Setup from a resource: the full area; the 800 set for an 800 area;
       the shadow amount comes before the palette, which then builds the
       table. */
    info_text = INFO;
    assert(aoe_ui_ez_setup_ror(&ez, &area, NULL, "scr", 50000, 1, 9, 9, 9, 9, 0));
    assert(info_id == 50000 && info_file[0] == '\0');
    assert(ez.panel.width == 800 && ez.ref_w == 800 && ez.ref_h == 600);
    assert(strcmp(((Shape *)ez.background)->file, "bg800.shp") == 0 &&
           ((Shape *)ez.background)->id == 3);
    assert(strcmp(((Shape *)ez.background2)->file, "a800.shp") == 0 &&
           ((Shape *)ez.background2)->id == 4);
    assert(ez.palette == 0x5000 && strcmp(palette_key, "pal.pal") == 0);
    assert(ez.shadow_amount == 3 && ez.shadow_table && shadows_live == 1 &&
           last_amount == 3);
    assert(strcmp(ez.pointer_name, "arrow") == 0 && ez.pointer_id == 51000);
    assert(((Shape *)ez.button_pictures)->id == 50721);
    assert(strcmp(ez.popup_name, "popinfo") == 0 && ez.popup_id == 50400);
    assert(ez.background_mode == 2 && ez.fill_color == 12 &&
           ez.fill_color2 == 12 && ez.panel.color == 12 && ez.bevel_on == 1);
    assert(ez.bevel[0] == 1 && ez.bevel[5] == 6);
    assert(ez.text_color == 0x0000ff && ez.text_shadow == 0x020100 &&
           ez.focus_color == 0x00ff00 && ez.focus_shadow == 0x050403 &&
           ez.hot_color == 0xff0000 && ez.hot_shadow == 0x080706);
    aoe_ui_ez_release_ror(&ez);
    assert(shapes_live == 0 && shadows_live == 0);

    /* A 640 area, and loose files: "<name>.sin", falling back to the
       resource when the file is missing. */
    {
        AoeUiAreaRor small = {(void *)1, 640, 480};
        ops.loose_files = 1;
        aoe_ui_ez_init_ror(&ez, &sys, &ops);
        assert(aoe_ui_ez_setup_ror(&ez, &small, NULL, "scr", 50000, 1, 0, 0, 0, 0, 0));
        assert(strcmp(info_file, "scr.sin") == 0);
        assert(((Shape *)ez.background)->id == 1);
        aoe_ui_ez_release_ror(&ez);
        aoe_ui_ez_init_ror(&ez, &sys, &ops);
        assert(aoe_ui_ez_setup_ror(&ez, &small, NULL, "gone", 50000, 1, 0, 0, 0, 0, 0));
        assert(info_id == 50000 && ez.background);
        aoe_ui_ez_release_ror(&ez);
        ops.loose_files = 0;
    }
    /* "none" for the 800 set falls back to 640 even on a wide area. */
    {
        AoeUiAreaRor wide = {(void *)1, 1024, 768};
        info_text = "a bg640 x 1 2 b none none 3 4 c bg1024 y 5 6";
        aoe_ui_ez_init_ror(&ez, &sys, &ops);
        assert(aoe_ui_ez_setup_ror(&ez, &wide, NULL, "s", 1, 1, 0, 0, 0, 0, 0));
        assert(((Shape *)ez.background)->id == 1);
        aoe_ui_ez_release_ror(&ez);
        info_text = "a bg640 x 1 2 b bg800 y 3 4 c bg1024 z 5 6";
        aoe_ui_ez_init_ror(&ez, &sys, &ops);
        assert(aoe_ui_ez_setup_ror(&ez, &wide, NULL, "s", 1, 1, 0, 0, 0, 0, 0));
        assert(((Shape *)ez.background)->id == 5);
        aoe_ui_ez_release_ror(&ez);
    }
    /* Centred placement without an info file. */
    aoe_ui_ez_init_ror(&ez, &sys, &ops);
    assert(aoe_ui_ez_setup_ror(&ez, &area, NULL, NULL, -1, 0, -1, -1, 200, 100, 0));
    assert(ez.panel.x == 300 && ez.panel.y == 250 && ez.ref_w == 200);
    aoe_ui_ez_release_ror(&ez);

    /* Pictures: "none" with an ID loads the resource; a dotted name is
       kept; a failed load leaves none. */
    aoe_ui_ez_init_ror(&ez, &sys, &ops);
    assert(aoe_ui_ez_set_background_ror(&ez, "none", 77) &&
           ((Shape *)ez.background)->id == 77 &&
           ((Shape *)ez.background)->file[0] == '\0');
    assert(aoe_ui_ez_set_background_ror(&ez, "pic.slp", 2) &&
           strcmp(((Shape *)ez.background)->file, "pic.slp") == 0);
    assert(!aoe_ui_ez_set_background_ror(&ez, "missing", 2) && !ez.background);
    assert(!aoe_ui_ez_set_background2_ror(&ez, NULL, -1));
    /* Palette without a shadow amount reports 0 but keeps the palette. */
    assert(aoe_ui_ez_set_palette_ror(&ez, NULL, -1) == 1 && !ez.palette);
    assert(aoe_ui_ez_set_palette_ror(&ez, "p2", 9) == 0 && ez.palette == 0x5000 &&
           !ez.shadow_table);
    aoe_ui_ez_set_shadow_amount_ror(&ez, -1);
    assert(ez.shadow_amount == 0 && !ez.shadow_table);
    aoe_ui_ez_set_shadow_amount_ror(&ez, 5);
    assert(ez.shadow_table && last_amount == 5);
    assert(strcmp(aoe_ui_ez_info_name_ror(&ez), "") == 0 &&
           aoe_ui_ez_palette_ror(&ez) == 0x5000);
    aoe_ui_ez_release_ror(&ez);
    assert(shapes_live == 0 && shadows_live == 0);

    /* Creators on a 1280x960 panel over the 640x480 grid. */
    {
        AoeUiAreaRor big = {(void *)1, 1280, 960};
        AoeUiButtonRor *b = NULL, *chk = NULL, *radio = NULL;
        AoeUiTextRor *t = NULL, *lines = NULL;
        AoeUiInputRor *in = NULL;
        AoeUiDropdownRor *dd = NULL;
        AoeUiListRor *l = NULL;
        AoeUiScrollbarRor *sb = NULL;
        AoeUiValueScrollbarRor *hv = NULL;
        static const char *rows[] = {"one", "two", "three"};
        const uint8_t bevel[6] = {9, 8, 7, 6, 5, 4};

        aoe_ui_ez_init_ror(&ez, &sys, &ops);
        assert(aoe_ui_ez_setup_ror(&ez, &big, NULL, NULL, -1, 1, 0, 0, 0, 0, 0));
        ((const AoeUiEzVtblRor *)ez.panel.vt)->set_reference(&ez, 640, 480);
        aoe_ui_ez_set_bevel_on_ror(&ez, 1);
        aoe_ui_ez_set_bevel_colors_ror(&ez, bevel);
        aoe_ui_ez_set_hot_colors_ror(&ez, 0x123456, 0x654321);

        assert(aoe_ui_ez_create_button_id_ror(&ez, &ez.panel, &b, 7, 8, 10, 20,
                                              100, 30, -1, 3, 99));
        assert(b->panel.x == 20 && b->panel.y == 40 && b->panel.width == 200 &&
               b->panel.height == 60 && b->draw_type == 3 && b->ids[0] == 99);
        assert(b->sound == &sound_obj && b->sound_id == 3 && b->font == 0x10a &&
               b->font_height == 20);
        /* The empty second line clears state 1's text. */
        assert(strcmp(b->text1[0], "Seven") == 0 && !b->text1[1]);
        assert(b->bevel_type == 4 && b->bevel[0] == 9);
        assert(b->text_color[0] == 0xffffff && b->text_color[8] == 0xffffff &&
               b->text_color[1] == 0x123456 && b->text_shadow[1] == 0x654321 &&
               b->focus_color[1] == 0xffff);

        /* Check box without pictures: an "X" in a text button. */
        assert(aoe_ui_ez_create_check_ror(&ez, &ez.panel, &chk, 0, 0, 10, 10, -1, 5));
        assert(chk->type == 3 && chk->state_count == 2 && chk->draw_type == 3 &&
               strcmp(chk->text1[1], "X") == 0 && chk->font == 0x100 &&
               !chk->sound);
        /* Radio with pictures: frames 4 and 6. */
        ez.button_pictures = load_shape(NULL, "btn.shp", 1);
        assert(aoe_ui_ez_create_radio_ror(&ez, &ez.panel, &radio, 0, 0, 10, 10, -1, 6));
        assert(radio->draw_type == 6 && radio->pictures[0] == ez.button_pictures &&
               radio->frames[0] == 4 && radio->frames[1] == 6 && radio->type != 3);

        /* Text: left-aligned and top unless centred / middle. */
        assert(aoe_ui_ez_create_text_ror(&ez, &ez.panel, &t, "Hi", 0, 0, 100, 20,
                                         2, 0, 0, 1));
        assert(t->align == 1 && t->vertical == 3 && t->wrap == 1 &&
               t->font == 0x102 && t->color == 0xffffff);
        free_panel(t);
        assert(aoe_ui_ez_create_text_id_ror(&ez, &ez.panel, &t, 7, 0, 0, 100, 20,
                                            -1, 1, 1, 0));
        assert(t->align == 0 && t->vertical == 0 &&
               strcmp(t->lines->text, "Seven") == 0);
        assert(aoe_ui_ez_create_lines_ror(&ez, &ez.panel, &lines, rows, 3, 0, 0,
                                          100, 100, 1, 0, 0));
        assert(lines->count == 3 && lines->top == 0);

        assert(aoe_ui_ez_create_input_ror(&ez, &ez.panel, &in, "abc", 10, 2, 0, 0,
                                          100, 20, -1));
        assert(in->max == 10 && in->format == 2 && in->text.border == 1 &&
               in->caret_color == 0xff && in->text.bevel_type == 3 &&
               in->text.focus_color == 0xffff);

        /* Drop-down: the button and scrollbar 18 units wide (36 here). */
        ops.dropdown_ops = NULL;
        assert(aoe_ui_ez_create_dropdown_ror(&ez, &ez.panel, &dd, 100, 50, 10, 10,
                                             100, 15, -1));
        assert(dd->button_w == 36 && dd->scroll_w == 36 && dd->text_h == 30 &&
               dd->list_w == 200 && dd->list->text.count == 1 &&
               dd->scrollbar->picture == ez.button_pictures &&
               dd->scrollbar->frames[1] == 8 && dd->button->button.frames[0] == 14);
        /* The drop-down maps the text bevel to 6, then the text gets 3. */
        assert(dd->list->text.color == 0xffffff && dd->text->bevel_type == 3);

        assert(aoe_ui_ez_create_list_ror(&ez, &ez.panel, &l, 0, 0, 100, 100, -1));
        assert(l->text.count == 1 && l->text.selected == 0 &&
               l->text.border == 1 && l->text.bevel_type == 3);

        /* A scrollbar for the lines panel: unscaled width; attached. */
        assert(aoe_ui_ez_create_text_scrollbar_ror(&ez, &sb, lines, 16));
        assert(sb->panel.width == 16 && sb->target == &lines->panel &&
               lines->scrollbar == &sb->panel && lines->has_scrollbar == 1 &&
               sb->frames[3] == 0xc);

        /* Horizontal value scrollbar: no arrows; thumb from x + 1. */
        assert(aoe_ui_ez_create_value_scrollbar_ror(&ez, &ez.panel, &hv, 1, 100,
                                                    1, 10, 10, 10, 100, 10));
        assert(hv->field_1c4 == 1 && hv->field_1c8 == 0 &&
               hv->bar.range == 100 && hv->bar.line_step == 1 &&
               hv->bar.page_step == 10 && hv->bar.horizontal == 1);
        assert(hv->bar.back.left == 20 && hv->bar.back.right == 20 &&
               hv->bar.forward.left == 219 && hv->bar.thumb_min_x == 21 &&
               hv->bar.thumb_max_x == 219 - 20 + 2);

        aoe_ui_ez_place_ror(&ez, &b->panel, 5, 5, 50, 50);
        assert(b->panel.x == 10 && b->panel.width == 100);

        free_panel(b);
        free_panel(chk);
        free_panel(radio);
        free_panel(t);
        free_panel(sb);
        free_panel(lines);
        free_panel(in);
        free_panel(dd);
        free_panel(l);
        free_panel(hv);
        aoe_ui_ez_release_ror(&ez);
        assert(shapes_live == 0);
    }

    /* Dialogs: registered message dialogs of w x h over the panel. */
    aoe_ui_ez_init_ror(&ez, &sys, &ops);
    assert(aoe_ui_ez_setup_ror(&ez, &area, NULL, NULL, -1, 1, 0, 0, 0, 0, 0));
    aoe_ui_ez_set_popup_ror(&ez, "pop", 5);
    help_mode = 0;
    aoe_ui_ez_help_dialog_id_ror(&ez, 7, NULL, 300, 200);
    {
        AoeUiMessageDialogRor *m = message(&sys, "OKDialog");
        assert(m && m->type == 5 && help_mode == 2 &&
               m->dialog.ez.panel.width == 300 &&
               m->dialog.ez.panel.x == 400 - 150 &&
               strcmp(m->text->lines->text, "Seven") == 0);
        /* One button 90 x 30 centred: gap (300 - 90) / 2. */
        assert(m->buttons[0] && !m->buttons[1] &&
               m->buttons[0]->panel.width == 90 &&
               m->buttons[0]->panel.y - m->dialog.ez.panel.y == 200 - 30 - 7 &&
               sys.modal == &m->dialog.ez.panel);
    }
    /* Once shown, help mode 2 blocks another; mode 1 is WinHelp. */
    aoe_ui_ez_help_dialog_ror(&ez, "again", NULL, 300, 200);
    assert(strcmp(message(&sys, "OKDialog")->text->lines->text, "Seven") == 0);
    help_mode = 1;
    aoe_ui_ez_help_dialog_ror(&ez, "again", NULL, 300, 200);
    assert(win_helps == 1);
    /* An OK dialog of the same name replaces the old one. */
    aoe_ui_ez_ok_dialog_ror(&ez, "ok", NULL, 300, 200);
    assert(message(&sys, "OKDialog")->type == 0 &&
           strcmp(message(&sys, "OKDialog")->text->lines->text, "ok") == 0);
    aoe_ui_ez_yes_no_cancel_dialog_id_ror(&ez, 7, "Mine", 400, 200);
    {
        AoeUiMessageDialogRor *m = message(&sys, "Mine");
        assert(m->type == 4 && m->buttons[2] &&
               m->buttons[0]->panel.width == 0x78 &&
               m->buttons[0]->panel.height == 0x19 &&
               m->buttons[2]->hotkey == 0x1b && m->buttons[0]->hotkey == 0);
        /* Its third button answers the parent with action 2... */
    }
    /* A dialog's buttons close it; other codes are swallowed. */
    {
        AoeUiPanelRor *box = &message(&sys, "Mine")->dialog.ez.panel;
        aoe_ui_panel_set_name_ror(box, "YesNoCancelDialog");
        assert(ez.panel.vt->action(&ez.panel, box, 2, 0, 0) == 1 &&
               !aoe_ui_system_find_ror(&sys, "YesNoCancelDialog"));
    }
    aoe_ui_ez_yes_no_dialog_ror(&ez, "q", NULL, 300, 200);
    {
        AoeUiPanelRor *box = &message(&sys, "YesNoDialog")->dialog.ez.panel;
        assert(ez.panel.vt->action(&ez.panel, box, 2, 0, 0) == 1 &&
               aoe_ui_system_find_ror(&sys, "YesNoDialog"));
        assert(ez.panel.vt->action(&ez.panel, box, 1, 0, 0) == 1 &&
               !aoe_ui_system_find_ror(&sys, "YesNoDialog"));
    }
    /* Clicking No (button 1) answers the parent with 1, which closes it;
       the parent's focus comes back. */
    {
        AoeUiPanelRor *before = ez.panel.current;
        AoeUiMessageDialogRor *m;
        aoe_ui_ez_yes_no_dialog_ror(&ez, "q", NULL, 300, 200);
        m = message(&sys, "YesNoDialog");
        assert(ez.panel.current == &m->dialog.ez.panel);
        m->dialog.ez.panel.vt->action(&m->dialog.ez.panel,
                                      &m->buttons[1]->panel, 1, 0, 0);
        assert(!aoe_ui_system_find_ror(&sys, "YesNoDialog"));
        (void)before;
    }
    aoe_ui_system_close_ror(&sys, "OKDialog");
    assert(!sys.modal || sys.modal == &ez.panel);

    /* Help mode: armed, then a left press shows the panel's help. */
    aoe_ui_ez_help_mode_on_ror(&ez);
    assert(ez.help_armed && cursor_help == 1 && cursor_mode == 6 &&
           ez.saved_pointer == 42 && pstate == 0);
    ez.panel.active = 1;
    ez.panel.help_id = 7;
    ez.panel.clip.right = ez.panel.clip.bottom = 100;
    assert(ez.panel.vt->mouse_down(&ez.panel, 1, 5, 5, 0, 0) == 1);
    assert(!ez.help_armed && cursor_help == 0 && pstate == 42);
    /* 0x00457c50: a registered help dialog with the panel's help text. */
    {
        AoeUiHelpDialogRor *hd =
            (AoeUiHelpDialogRor *)aoe_ui_system_find_ror(&sys, "Help Dialog");
        assert(hd && hd->text && strcmp(hd->text->lines->text, "Seven") == 0);
        aoe_ui_system_close_ror(&sys, "Help Dialog");
    }

    /* Focus: the pointer, the default palette, the IME. */
    ez.ime_toggle = 1;
    aoe_ui_ez_set_pointer_ror(&ez, "busy", 3);
    ez.panel.area = &area;
    ez.panel.vt->set_focus(&ez.panel, 1);
    assert(strcmp(pointer_name, "busy") == 0 && pointer_id == 3 &&
           area_pal == 0x77 && ime_state == 1);
    ez.panel.vt->set_focus(&ez.panel, 0);
    assert(ime_state == 0);
    aoe_ui_ez_release_ror(&ez);

    /* The screen subclass: help popup string 120055 at (450, 100). */
    help_mode = 0;
    assert(aoe_ui_screen_init_ror(&ez, &sys, &ops, "Screen"));
    assert(aoe_ui_screen_setup_ror(&ez, &area, NULL, -1, 0));
    assert(ez.panel.width == 800);
    ez.panel.vt->slot_b0(&ez.panel);
    assert(message(&sys, "OKDialog")->type == 5 &&
           message(&sys, "OKDialog")->dialog.ez.panel.width == 450 &&
           strcmp(message(&sys, "OKDialog")->text->lines->text, "Help text") == 0);
    aoe_ui_system_close_ror(&sys, "OKDialog");

    /* The fading focus on a full-screen display: out to black, clear and
       show, then back in; only once until another screen fades in. */
    {
        static const AoeDrawOpsRor dops = {.palette_set = fake_palette_set,
                                           .fill = fake_fill,
                                           .blt = fake_blt};
        static AoeDrawSystemRor d;
        static AoeDrawSurfaceRor back;
        aoe_draw_clear_ror(&d, &dops);
        d.type = 2;
        d.screen = 2;
        d.palette_object = &d;
        d.primary_surface = &d;
        d.back = &back;
        back.system = &d;
        back.surface = &back;
        area.surface = &back;
        pf.now_ms = ticking;
        ez.panel.vt->set_focus(&ez.panel, 1);
        assert(sys.faded_screen == &ez.panel);
        assert(palette_sets >= 6 && fills >= 1 && presents == 2);
        ez.panel.vt->set_focus(&ez.panel, 0);
        palette_sets = presents = 0;
        ez.panel.vt->set_focus(&ez.panel, 1);
        assert(palette_sets == 0 && presents == 0);
        area.surface = NULL;
        pf.now_ms = now;
    }

    /* Backgrounds on a memory surface: drawn at the panel's origin; with
       the parent's rect flag darkened through the shadow table; a full
       redraw clears the panel first. */
    {
        static uint8_t bits[16 * 8];
        static uint8_t *rows[8];
        static uint8_t shade[256];
        static AoeDrawSystemRor d;
        static AoeDrawSurfaceRor surf;
        static AoeShapeRor shape;
        static const AoeShapeOpsRor sops = {NULL, 0, NULL, NULL, shp_res, NULL};
        static const AoeDrawOpsRor mops = {.create_surface = mem_create,
                                           .describe = mem_describe,
                                           .fill = mem_fill,
                                           .lock = mem_lock,
                                           .unlock = mem_unlock,
                                           .release = mem_release,
                                           .blt = mem_blt};
        AoeUiRectRor clip;
        int i;
        static const AoeShapeDrawOpsRor draw_ops = {0};
        build_shp();
        aoe_shape_load_ror(&shape, NULL, 1, &sops);
        ops.shape_ops = &draw_ops;
        for (i = 0; i < 256; ++i) shade[i] = (uint8_t)(i ^ 0xff);
        d.type = 1;
        surf.system = &d;
        surf.view.bits = bits;
        surf.view.width = 16;
        surf.view.height = 8;
        surf.view.pitch = 16;
        surf.view.direction = 1;
        surf.view.clip_right = 15;
        surf.view.clip_bottom = 7;
        for (i = 0; i < 8; ++i) rows[i] = bits + 16 * i;
        surf.rows = rows;
        area.surface = &surf;
        ez.panel.x = 2;
        ez.panel.y = 3;
        ez.panel.width = 10;
        ez.panel.height = 4;
        clip = ez.panel.clip;
        ez.panel.clip.left = 2;
        ez.panel.clip.top = 3;
        ez.panel.clip.right = 11;
        ez.panel.clip.bottom = 6;
        ez.background = &shape;
        ez.background_mode = 0;
        ez.bevel_on = 0;
        ez.panel.redraw = 0;
        memset(bits, 0x11, sizeof bits);
        aoe_ui_ez_draw_background_ror(&ez, 0);
        /* Hotspot (1, 1) at (2, 3): the frame's top-left lands at (1, 2). */
        assert(bits[2 * 16 + 1] == 0xa1 && bits[2 * 16 + 2] == 0xb2 &&
               bits[3 * 16 + 1] == 0xc3 && bits[3 * 16 + 3] == 0xc3);
        ez.shadow_table = shade;
        memset(bits, 0x11, sizeof bits);
        aoe_ui_ez_draw_background_ror(&ez, 1);
        /* Darkened over the picture's box from the origin (2, 3). */
        assert(bits[3 * 16 + 2] == (uint8_t)(0xc3 ^ 0xff) &&
               bits[3 * 16 + 1] == 0xc3 && bits[0] == 0x11);
        ez.panel.redraw = 2;
        ez.fill_color = 0x55;
        ez.shadow_table = NULL;
        memset(bits, 0x11, sizeof bits);
        aoe_ui_ez_draw_background_ror(&ez, 0);
        assert(bits[6 * 16 + 11] == 0x55 && bits[2 * 16 + 1] == 0xa1);
        ez.panel.redraw = 0;

        /* The shadow area on a DirectDraw display: a surface the picture's
           size, drawn and darkened, then copied onto the panel. */
        aoe_draw_clear_ror(&d, &mops);
        d.type = 2;
        d.device = &d;
        surf.system = &d;
        ez.shadow_table = shade;
        ez.fill_color = 0x55;
        aoe_ui_ez_shadow_area_ror(&ez, 0);
        assert(ez.shadow_panel && ez.shadow_panel->view.width == 4 &&
               ez.shadow_panel->view.height == 2);
        /* As in the original, the picture's hotspot is put at (0, 0), so
           its first row falls outside and its second lands on row 0. */
        assert(((MemSurface *)ez.shadow_panel->surface)->bits[0] ==
               (uint8_t)(0xc3 ^ 0xff));
        assert(((MemSurface *)ez.shadow_panel->surface)->bits[4] ==
               (uint8_t)(0x55 ^ 0xff));
        surf.surface = &surf; /* the area has a device surface now */
        blits = 0;
        aoe_ui_ez_draw_background_ror(&ez, 1);
        assert(blits == 1);
        surf.surface = NULL;
        ez.background = NULL;
        ez.shadow_table = NULL;
        aoe_shape_free_ror(&shape, &sops);
        ez.panel.clip = clip;
        area.surface = NULL;
    }
    {
        AoeScenarioEditorUiRor ed = {&ez, 1, {1, 2, 3, 4, 5, 6}};
        AoeUiButtonRor *b = NULL;
        AoeUiTextRor *t = NULL;
        AoeUiEditRor *e = NULL;
        char buf[32];
        assert(aoe_scenario_editor_button_ror(&ed, &ez.panel, &b, "A", NULL, "Two", NULL));
        assert(b && !b->panel.active && b->bevel_type == 3 && b->bevel[5] == 6);
        assert(aoe_scenario_editor_text_ror(&ed, &ez.panel, &t, "Hi") && t && !t->panel.active);
        assert(aoe_scenario_editor_text_id_ror(&ed, &ez.panel, &t, 7) && !t->panel.active);
        /* Without edit ops here the edit cannot be made: 0, nothing hidden. */
        if (aoe_scenario_editor_edit_ror(&ed, &ez.panel, &e, "x", 10, 0, 0, 0))
            assert(!e->panel.active);
        (void)aoe_scenario_editor_string_ror(&ed, buf, 7, sizeof buf);
    }
    aoe_ui_ez_release_ror(&ez);
    return 0;
}
