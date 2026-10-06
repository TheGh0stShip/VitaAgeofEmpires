/* Patched RoR 1.0a dialogs: the dialog base 0x00460730..0x004609f0, the
   message dialog 0x00434550..0x00434990 and the list dialog
   0x00433890..0x00433c20. */
#include "edit_object_list.h"
#include "ui_dialog.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AoePlayerTemplates obj_templates;
static const AoePlayerTemplates *obj_get(void *c) { (void)c; return &obj_templates; }
static void obj_name(void *c, int16_t id, char *out, int size) {
    (void)c;
    snprintf(out, (size_t)size, "Lang%d", (int)id);
}
static int16_t obj_place(void *c) { (void)c; return 0; }
static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }
static void string(void *c, int32_t id, char *out, int size) {
    (void)c;
    snprintf(out, (size_t)size, "S%d", (int)id);
}
static int fonts(void *c, int32_t index, uintptr_t *font, int32_t *w, int32_t *h) {
    (void)c; *font = (uintptr_t)index + 1; *w = 8; *h = 12; return 1;
}
static int32_t codes[16], n;
static AoeUiPanelRor *froms[16];
static int32_t record(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                      int32_t v, int32_t e) {
    (void)p; (void)v; (void)e;
    froms[n] = from;
    codes[n++] = code;
    return 1;
}
static int has_help(void *c) { (void)c; return 1; }
static char keyword[64];
static void win_key(void *c, const char *k) { (void)c; snprintf(keyword, sizeof(keyword), "%s", k); }
static int idles, blocked;
static void game_idle(void *c) { (void)c; ++idles; }
static int idle_blocked(void *c) { (void)c; return blocked; }

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 800, 600};
    static AoeUiPanelRor screen, other;
    static AoeUiPanelVtblRor vt;
    AoeUiEzOpsRor ops = {0};
    AoeUiMessageDialogRor *m;
    AoeUiListDialogRor *l;
    static const char *lines[] = {"alpha", "beta", "gamma"};

    pf.text_width = width;
    pf.string = string;
    sys.platform = &pf;
    ops.font = fonts;
    ops.game_idle = game_idle;
    ops.idle_blocked = idle_blocked;
    aoe_ui_panel_init_ror(&screen, &sys);
    aoe_ui_panel_setup_ror(&screen, &area, NULL, 0, 0, 800, 600, 0);
    vt = aoe_ui_panel_vtable_ror;
    vt.action = record;
    screen.vt = &vt;
    aoe_ui_panel_init_ror(&other, &sys);
    aoe_ui_panel_setup_ror(&other, &area, &screen, 0, 0, 10, 10, 0);
    aoe_ui_panel_set_current_ror(&screen, &other);

    /* A yes/no dialog centred over the screen takes its focus. */
    m = calloc(1, sizeof(*m));
    assert(aoe_ui_message_dialog_init_ror(m, &sys, &ops, "Ask"));
    assert(m->dialog.field_480 == 11 && m->dialog.field_488 == 11);
    assert(aoe_ui_message_dialog_setup_ror(m, &screen, NULL, -1, 400, 200, 2,
                                           "Sure?", 90, 30));
    assert(m->dialog.ez.panel.x == 200 && m->dialog.ez.panel.y == 200);
    assert(screen.current == &m->dialog.ez.panel && m->dialog.saved_focus == &other);
    assert(sys.modal == &m->dialog.ez.panel);
    /* Two buttons: gap (400 - 180) / 3 = 73. */
    assert(m->buttons[0]->panel.x == 200 + 73 &&
           m->buttons[1]->panel.x == 200 + 73 + 73 + 90 && !m->buttons[2]);
    assert(strcmp(m->buttons[0]->text1[0], "S4003") == 0 &&
           strcmp(m->buttons[1]->text1[0], "S4004") == 0 &&
           m->buttons[1]->hotkey == 0x1b);
    assert(m->dialog.ez.panel.current == &m->buttons[0]->panel);
    assert(m->buttons[0]->panel.tab_next == &m->buttons[1]->panel);
    /* No answers 1; the dialog's own codes do not count. */
    m->dialog.ez.panel.vt->action(&m->dialog.ez.panel, &m->buttons[1]->panel, 1, 0, 0);
    assert(n == 1 && codes[0] == 1 && froms[0] == &m->dialog.ez.panel);
    /* Idle: the game's work unless blocked. */
    m->dialog.ez.panel.vt->handle_idle(&m->dialog.ez.panel);
    blocked = 1;
    m->dialog.ez.panel.vt->handle_idle(&m->dialog.ez.panel);
    assert(idles == 1);
    /* Unknown types fail after the text. */
    {
        AoeUiMessageDialogRor *bad = calloc(1, sizeof(*bad));
        aoe_ui_message_dialog_init_ror(bad, &sys, &ops, "Bad");
        assert(!aoe_ui_message_dialog_setup_ror(bad, &screen, NULL, -1, 100, 100,
                                                6, "x", 10, 10));
        assert(aoe_ui_system_close_ror(&sys, "Bad"));
    }
    /* Destroying it gives the focus back. */
    assert(aoe_ui_system_close_ror(&sys, "Ask"));
    assert(screen.current == &other && !sys.modal);

    /* The list dialog. */
    l = calloc(1, sizeof(*l));
    assert(aoe_ui_list_dialog_init_ror(l, &sys, &ops, "Pick") && l->selected == -1);
    assert(aoe_ui_list_dialog_setup_ror(l, &screen, NULL, -1, 400, 300, 10, 10,
                                        380, 30, "Choose", 10, 50, 360, 180,
                                        lines, 3, 1, 50, 250, 250, 90, 30));
    assert(l->title && strcmp(l->title->lines->text, "Choose") == 0 &&
           l->list->text.count == 3 && l->list->text.selected == 1 &&
           l->scrollbar && l->list->text.scrollbar == &l->scrollbar->panel &&
           l->cancel->hotkey == 0x1b &&
           l->dialog.ez.panel.current == &l->list->text.panel);
    assert(strcmp(l->ok->text1[0], "S4001") == 0 &&
           l->ok->panel.x - l->dialog.ez.panel.x == 50 &&
           l->cancel->panel.x - l->dialog.ez.panel.x == 250);
    l->list->text.selected = 2;
    n = 0;
    l->dialog.ez.panel.vt->action(&l->dialog.ez.panel, &l->list->text.panel, 3, 2, 0);
    assert(n == 1 && codes[0] == 1 && aoe_ui_list_dialog_selected_ror(l) == 2 &&
           strcmp(aoe_ui_list_dialog_text_ror(l), "gamma") == 0);
    l->dialog.ez.panel.vt->action(&l->dialog.ez.panel, &l->cancel->panel, 1, 0, 0);
    assert(n == 2 && codes[1] == 2);
    assert(aoe_ui_system_close_ror(&sys, "Pick"));

    /* The help dialog beside (100, 100) on a full-screen easy panel: two
       text lines (font 12): height 13 * 2 + 55 = 81; x |0 - 5 - 100|, y
       |100 - 81 - 0 - 10|. */
    {
        AoeUiEzRor *owner = calloc(1, sizeof(*owner));
        AoeUiHelpDialogRor *h = calloc(1, sizeof(*h));
        ops.single_player = has_help;
        ops.win_help_key = win_key;
        aoe_ui_ez_init_named_ror(owner, &sys, &ops, "Game Screen");
        assert(aoe_ui_ez_setup_ror(owner, &area, NULL, NULL, -1, 1, 0, 0, 0, 0, 0));
        assert(aoe_ui_help_dialog_init_ror(h, &sys, &ops, &owner->panel,
                                           "Game Screen", "Help", 77, 100, 100));
        assert(h->page == 77 && strcmp(h->owner, "Game Screen") == 0);
        assert(h->dialog.ez.panel.x == 105 && h->dialog.ez.panel.y == 9 &&
               h->dialog.ez.panel.width == 340 && h->dialog.ez.panel.height == 81);
        /* Rows: 12 * 2 + 20; with "more help", OK moves to 195. */
        assert(h->more && h->ok && h->ok->panel.x - 105 == 195 &&
               h->more->panel.x - 105 == 25 && h->ok->panel.y - 9 == 44 &&
               h->ok->hotkey == 0x1b);
        /* More help: back to the owner screen, closed, WinHelp keyword. */
        h->dialog.ez.panel.vt->action(&h->dialog.ez.panel, &h->more->panel, 1, 0, 0);
        assert(!aoe_ui_system_find_ror(&sys, "Help Dialog") &&
               strcmp(keyword, "S77") == 0 && sys.screen == &owner->panel);
        /* A press outside a new one closes it like OK. */
        h = calloc(1, sizeof(*h));
        assert(aoe_ui_help_dialog_init_ror(h, &sys, &ops, &owner->panel,
                                           "Game Screen", "Help", -1, 700, 500));
        assert(!h->more && h->ok->panel.x - h->dialog.ez.panel.x == 110);
        h->dialog.ez.panel.vt->mouse_down(&h->dialog.ez.panel, 1, 0, 0, 0, 0);
        assert(!aoe_ui_system_find_ror(&sys, "Help Dialog"));
        assert(aoe_ui_system_close_ror(&sys, "Game Screen"));
    }
    /* The object list dialog (0x0046a890..0x0046ad40): " <None> " plus
       the shown templates (slot 1 empty, slot 2 hidden), sorted ignoring
       case; the placed object (slot 0) is the default line. */
    {
        static DatUnit zebra, hidden, named;
        static DatUnit *slots[4];
        AoeEditObjectListOpsRor lo = {0};
        AoeEditObjectListRor *d = calloc(1, sizeof(*d));
        zebra.name = "zebra";
        hidden.name = "Hidden";
        hidden.hide_in_editor = 1;
        named.name = "unused";
        named.language_name_id = 5;
        slots[0] = &zebra;
        slots[2] = &hidden;
        slots[3] = &named;
        obj_templates.count = 4;
        obj_templates.slots = slots;
        lo.templates = obj_get;
        lo.object_name = obj_name;
        lo.place_object = obj_place;
        assert(aoe_edit_object_list_init_ror(d, &sys, &ops, &lo, "Objects"));
        d->mode = 2;
        assert(aoe_edit_object_list_default_id_ror(d) == -1 &&
               !aoe_edit_object_list_build_ror(d));
        assert(aoe_edit_object_list_setup_ror(d, &screen, NULL, -1, 1, "T"));
        assert(d->count == 3 && strcmp(d->lines[0], " <None> ") == 0 &&
               strcmp(d->lines[1], "Lang5") == 0 && d->entries[1].id == 3 &&
               strcmp(d->lines[2], "zebra") == 0 && d->entries[2].id == 0 &&
               d->entries[0].id == -1 && d->default_index == 2 &&
               d->list.list->text.selected == 2);
        d->list.selected = -1;
        assert(aoe_edit_object_list_choice_ror(d) == -1);
        d->list.selected = 1;
        assert(aoe_edit_object_list_choice_ror(d) == 3);
        assert(aoe_ui_system_close_ror(&sys, "Objects"));
    }

    aoe_ui_panel_release_ror(&other);
    aoe_ui_panel_release_ror(&screen);
    free(sys.panels);
    return 0;
}
