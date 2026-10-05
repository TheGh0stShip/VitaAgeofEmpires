/* Patched RoR 1.0a easy panel 0x00454430..0x00457fe0 and the value
   scrollbars 0x00468360..0x00468540. */
#include "ui_ez.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t now(void *c) { (void)c; return 0; }
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

static char closed[64], dialog_name[64], dialog_text[64];
static int32_t dialog_type = -1, dialog_w, dialog_h, dialogs;
static int exists(void *c, const char *n) { (void)c; return strcmp(n, "OKDialog") == 0; }
static void close_panel(void *c, const char *n) { (void)c; snprintf(closed, sizeof(closed), "%s", n); }
static void dialog(void *c, AoeUiEzRor *parent, const char *name, const char *info,
                   int32_t iid, int32_t x, int32_t y, int32_t type, const char *text,
                   int32_t w, int32_t h) {
    (void)c; (void)parent; (void)info; (void)iid; (void)x; (void)y;
    snprintf(dialog_name, sizeof(dialog_name), "%s", name);
    snprintf(dialog_text, sizeof(dialog_text), "%s", text);
    dialog_type = type;
    dialog_w = w;
    dialog_h = h;
    ++dialogs;
}
static char help_text[64];
static void help_popup(void *c, const char *panel, const char *text, int32_t page,
                       int32_t x, int32_t y) {
    (void)c; (void)panel; (void)page; (void)x; (void)y;
    snprintf(help_text, sizeof(help_text), "%s", text);
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
    ops.panel_exists = exists;
    ops.close_panel = close_panel;
    ops.dialog = dialog;
    ops.help_popup = help_popup;
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

    /* Dialogs. */
    aoe_ui_ez_init_ror(&ez, &sys, &ops);
    aoe_ui_ez_set_popup_ror(&ez, "pop", 5);
    help_mode = 0;
    aoe_ui_ez_help_dialog_id_ror(&ez, 7, NULL, 1, 2);
    assert(dialogs == 1 && dialog_type == 5 && help_mode == 2 &&
           strcmp(dialog_name, "OKDialog") == 0 &&
           strcmp(closed, "OKDialog") == 0 && strcmp(dialog_text, "Seven") == 0);
    aoe_ui_ez_help_dialog_ror(&ez, "again", NULL, 1, 2);
    assert(dialogs == 1);
    help_mode = 1;
    aoe_ui_ez_help_dialog_ror(&ez, "again", NULL, 1, 2);
    assert(dialogs == 1 && win_helps == 1);
    aoe_ui_ez_yes_no_dialog_ror(&ez, "q", NULL, 0, 0);
    assert(dialog_type == 2 && strcmp(dialog_name, "YesNoDialog") == 0);
    aoe_ui_ez_yes_no_cancel_dialog_id_ror(&ez, 7, "Mine", 0, 0);
    assert(dialog_type == 4 && strcmp(dialog_name, "Mine") == 0 &&
           dialog_w == 0x78 && dialog_h == 0x19);
    aoe_ui_ez_ok_dialog_ror(&ez, "ok", NULL, 0, 0);
    assert(dialog_type == 0 && dialog_w == 0x5a);

    /* A dialog's buttons close it; other codes are swallowed. */
    {
        static AoeUiPanelRor box;
        aoe_ui_panel_init_ror(&box, &sys);
        aoe_ui_panel_set_name_ror(&box, "YesNoCancelDialog");
        closed[0] = '\0';
        assert(ez.panel.vt->action(&ez.panel, &box, 2, 0, 0) == 1 &&
               strcmp(closed, "YesNoCancelDialog") == 0);
        aoe_ui_panel_set_name_ror(&box, "YesNoDialog");
        closed[0] = '\0';
        assert(ez.panel.vt->action(&ez.panel, &box, 2, 0, 0) == 1 && !closed[0]);
        aoe_ui_panel_release_ror(&box);
    }

    /* Help mode: armed, then a left press shows the panel's help. */
    aoe_ui_ez_help_mode_on_ror(&ez);
    assert(ez.help_armed && cursor_help == 1 && cursor_mode == 6 &&
           ez.saved_pointer == 42 && pstate == 0);
    ez.panel.active = 1;
    ez.panel.help_id = 7;
    ez.panel.clip.right = ez.panel.clip.bottom = 100;
    assert(ez.panel.vt->mouse_down(&ez.panel, 1, 5, 5, 0, 0) == 1);
    assert(!ez.help_armed && cursor_help == 0 && pstate == 42 &&
           strcmp(help_text, "Seven") == 0);

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
    assert(dialog_type == 5 && strcmp(dialog_text, "Help text") == 0);
    aoe_ui_ez_release_ror(&ez);
    return 0;
}
