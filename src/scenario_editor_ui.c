#include "scenario_editor_ui.h"

#include <stdio.h>
#include <string.h>

static void front_hidden(AoeUiPanelRor *p) {
    aoe_ui_panel_set_z_ror(p, 1, 0);
    aoe_ui_panel_set_active_ror(p, 0);
}

int aoe_scenario_editor_button_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                   AoeUiButtonRor **out, const char *text, const char *text1,
                                   const char *text2, const char *text3) {
    if (!aoe_ui_ez_create_button_ror(e->ez, parent, out, text, text1, 0, 0, 0, 0, 10, 1, 0))
        return 0;
    if (text2) aoe_ui_button_set_text_ror(*out, 2, text2);
    if (text3) aoe_ui_button_set_text_ror(*out, 3, text3);
    front_hidden(&(*out)->panel);
    if (e->bevel_on) aoe_ui_button_set_bevel_ror(*out, 3, e->bevel);
    return 1;
}

int aoe_scenario_editor_text_id_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                    AoeUiTextRor **out, int32_t text_id) {
    if (!aoe_ui_ez_create_text_id_ror(e->ez, parent, out, text_id, 0, 0, 0, 0, 10, 0, 0, 0))
        return 0;
    front_hidden(&(*out)->panel);
    return 1;
}

int aoe_scenario_editor_text_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                 AoeUiTextRor **out, const char *text) {
    if (!aoe_ui_ez_create_text_ror(e->ez, parent, out, text, 0, 0, 0, 0, 10, 0, 0, 0)) return 0;
    front_hidden(&(*out)->panel);
    return 1;
}

int aoe_scenario_editor_edit_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                 AoeUiEditRor **out, const char *text, int16_t max,
                                 int32_t format, int32_t ime_enabled, int32_t ime_open) {
    if (!aoe_ui_ez_create_edit_ror(e->ez, parent, out, text, max, format, 0, 0, 0, 0, 10,
                                   ime_enabled, ime_open))
        return 0;
    front_hidden(&(*out)->panel);
    return 1;
}

int aoe_scenario_editor_string_ror(AoeScenarioEditorUiRor *e, char *out, int32_t id, int size) {
    return aoe_ui_string_ror(e->ez->panel.system, id, out, size);
}

void aoe_scenario_editor_brush_ror(AoeScenarioEditorToolsRor *t, int32_t size,
                                   const AoeScenarioEditorToolOpsRor *ops) {
    if (size == 0) return;
    t->brush_size = size;
    ops->radio_select(ops->context, 1, size);
    t->brush = (int16_t)(size >= 1 && size <= 5 ? size * 2 - 1 : 2);
}

void aoe_scenario_editor_terrain_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                     const AoeScenarioEditorToolOpsRor *ops) {
    static const int16_t terrain[9] = {0, 6, 10, 1, 13, 20, 4, 19, 22};
    t->terrain = (line >= 0 && line <= 8) ? terrain[line] : 0;
    ops->list_select(ops->context, 0, line);
}

void aoe_scenario_editor_elevation_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                       const AoeScenarioEditorToolOpsRor *ops) {
    t->elevation = line;
    ops->list_select(ops->context, 1, line);
}

void aoe_scenario_editor_unit_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                  const AoeScenarioEditorToolOpsRor *ops) {
    t->place_unit = line == -1 ? -1 : ops->unit_entry(ops->context, line);
    ops->list_select(ops->context, 2, line);
}

void aoe_scenario_editor_cancel_ror(const AoeScenarioEditorToolsRor *t,
                                    const AoeScenarioEditorToolOpsRor *ops) {
    if (t->place_unit != -1 && ops->placeable_building(ops->context, t->place_unit))
        ops->action(ops->context, 0x15, 0);
    else
        ops->action(ops->context, 8, ops->game_6e);
}

static void switch_tool(int32_t *current, int page, int32_t tool, int32_t show,
                        const AoeScenarioEditorToolOpsRor *ops) {
    if (*current) ops->show_tool(ops->context, page, *current, 0);
    if (tool == 0) return; /* the original returns without keeping it */
    if (show) ops->show_tool(ops->context, page, tool, 1);
    *current = tool;
    ops->radio_select(ops->context, page == 0 ? 0 : 2, tool);
    ops->redraw(ops->context);
}

void aoe_scenario_editor_map_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool, int32_t show,
                                      const AoeScenarioEditorToolOpsRor *ops) {
    switch_tool(&t->map_tool, 0, tool, show, ops);
}

void aoe_scenario_editor_unit_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool, int32_t show,
                                       const AoeScenarioEditorToolOpsRor *ops) {
    switch_tool(&t->unit_tool, 1, tool, show, ops);
}

void aoe_scenario_editor_terrain_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool,
                                          int32_t show, const AoeScenarioEditorToolOpsRor *ops) {
    static const int32_t actions[4] = {0, 9, 10, 0x13};
    int pass;
    for (pass = 0; pass < 2; ++pass) {
        int32_t which, on;
        if (pass == 0) {
            which = t->terrain_tool;
            on = 0;
        } else {
            if (tool == 0) return;
            if (!show) break;
            which = tool;
            on = 1;
        }
        if (which < 1 || which > 3) continue;
        ops->show_tool(ops->context, 2, which, on);
        if (!on) {
            ops->action(ops->context, 0, 0);
            continue;
        }
        if (which == 1)
            aoe_scenario_editor_terrain_ror(t, (int16_t)ops->list_line(ops->context, 0), ops);
        else
            aoe_scenario_editor_elevation_ror(t, (int16_t)ops->list_line(ops->context, 1), ops);
        ops->action(ops->context, actions[which], 0);
    }
    t->terrain_tool = tool;
    ops->radio_select(ops->context, 3, tool);
    ops->redraw(ops->context);
}

int32_t aoe_scenario_editor_terrain_lines_ror(AoeUiTextRor *list) {
    int32_t i;
    aoe_ui_text_clear_ror(list);
    for (i = 0; i < 9; ++i) aoe_ui_text_append_id_ror(list, 0x297d + i, 0);
    return 8;
}

int aoe_scenario_editor_make_list_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                      AoeUiListRor **out, AoeUiScrollbarRor **bar, int elevation,
                                      int terrain) {
    if (!aoe_ui_ez_create_list_ror(e->ez, parent, out, 0, 0, 0, 0, 10)) return 0;
    if (!*out) return 0; /* host guard: the maker reports success either way */
    (*out)->text.panel.vt->set_active(&(*out)->text.panel, 0);
    aoe_ui_text_clear_ror(&(*out)->text);
    if (!aoe_ui_ez_create_text_scrollbar_ror(e->ez, bar, &(*out)->text, 0x14)) return 0;
    if (elevation) {
        char base[0x32], line[0x40];
        int32_t i;
        aoe_scenario_editor_string_ror(e, base, 0x2987, sizeof base);
        for (i = 1; i <= 7; ++i) {
            snprintf(line, sizeof line, "%s %d", base, (int)i);
            aoe_ui_text_append_ror(&(*out)->text, line, 0);
        }
        return 1;
    }
    if (terrain) aoe_scenario_editor_terrain_lines_ror(&(*out)->text);
    return 1;
}

int aoe_scenario_editor_button_id_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                      AoeUiButtonRor **out, int32_t text_id) {
    char text[0x78];
    aoe_scenario_editor_string_ror(e, text, text_id, sizeof text);
    return aoe_scenario_editor_button_ror(e, parent, out, text, NULL, NULL, NULL);
}

static int bevelled(AoeScenarioEditorUiRor *e, AoeUiButtonRor *b) {
    front_hidden(&b->panel);
    if (e->bevel_on) aoe_ui_button_set_bevel_ror(b, 3, e->bevel);
    return 1;
}

int aoe_scenario_editor_check_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                  AoeUiButtonRor **out) {
    if (!aoe_ui_ez_create_check_ror(e->ez, parent, out, 0, 0, 0, 0, 1, 0) || !*out) return 0;
    return bevelled(e, *out);
}

int aoe_scenario_editor_radio_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                  AoeUiButtonRor **out) {
    if (!aoe_ui_ez_create_radio_ror(e->ez, parent, out, 0, 0, 0, 0, 1, 0) || !*out) return 0;
    return bevelled(e, *out);
}

void aoe_scenario_editor_ages_ror(AoeUiDropdownRor *ages, AoeUiPanelRor *panel_5bc) {
    int32_t i;
    aoe_ui_dropdown_clear_ror(ages);
    for (i = 0; i < 5; ++i) aoe_ui_dropdown_append_id_ror(ages, 0x1069 + i, i);
    aoe_ui_dropdown_select_ror(ages, 0);
    if (panel_5bc) panel_5bc->vt->set_active(panel_5bc, 0);
}

typedef struct {
    AoeUiDropdownRor *d;
    size_t cut;
    int32_t count;
} FileChoice;

static void add_file(void *user, const char *name) {
    FileChoice *f = user;
    char base[260];
    size_t n = strlen(name);
    n = n >= f->cut ? n - f->cut : 0;
    if (n >= sizeof base) n = sizeof base - 1;
    memcpy(base, name, n);
    base[n] = '\0';
    aoe_ui_dropdown_append_ror(f->d, base, 0);
    ++f->count;
}

int32_t aoe_scenario_editor_file_choice_ror(
    AoeUiDropdownRor *d, const char *dir, const char *ext, int none, int other,
    void (*list_files)(void *context, const char *pattern,
                       void (*found)(void *user, const char *name), void *user),
    void *context) {
    FileChoice f;
    char pattern[0x104];
    aoe_ui_dropdown_clear_ror(d);
    if (none) aoe_ui_dropdown_append_id_ror(d, 0x2775, 0);
    if (other) aoe_ui_dropdown_append_id_ror(d, 0x277b, 0);
    f.d = d;
    f.cut = strlen(ext);
    f.count = 0;
    snprintf(pattern, sizeof pattern, "%s*%s", dir, ext);
    if (list_files) list_files(context, pattern, add_file, &f);
    return f.count;
}

void aoe_scenario_editor_player_page_ror(AoeUiButtonRor *buttons[12], AoeUiPanelRor *others[3],
                                         AoeUiDropdownRor *players, int16_t *current,
                                         int32_t show, int (*player_on)(void *context, int32_t i),
                                         void *context) {
    int32_t i;
    for (i = 0; i < 12; ++i) {
        buttons[i]->panel.vt->set_active(&buttons[i]->panel, show);
        if (show)
            aoe_ui_button_set_state_ror(
                buttons[i], (int16_t)((player_on(context, i) ? 2 : 0) + (*current == i)));
    }
    for (i = 0; i < 3; ++i) others[i]->vt->set_active(others[i], show);
    if (show)
        aoe_ui_dropdown_select_ror(players, (int16_t)(*current - 1));
    else
        *current = (int16_t)(aoe_ui_dropdown_selection_ror(players) + 1);
}

int aoe_editor_condition_plan_ror(int32_t kind, int32_t show,
                                  AoeEditorConditionPlanRor *plan) {
    const uint32_t amount = AOE_EDITOR_COND_AMOUNT_LABEL |
                            AOE_EDITOR_COND_AMOUNT;
    const uint32_t player = AOE_EDITOR_COND_PLAYER_LABEL |
                            AOE_EDITOR_COND_PLAYER;
    AoeEditorConditionPlanRor p = {0};
    int ok = 1;
    if (!show) {
        p.shown = 0;
        ok = 0;
    } else {
        switch (kind) {
        case 1:
            p.shown = AOE_EDITOR_COND_UNIT | AOE_EDITOR_COND_TARGET;
            break;
        case 2:
            p.shown = AOE_EDITOR_COND_UNIT | AOE_EDITOR_COND_TARGET |
                      AOE_EDITOR_COND_AREA;
            break;
        case 3:
        case 4:
            p.shown = AOE_EDITOR_COND_CLASS | amount;
            if (kind == 4)
                p.shown |= AOE_EDITOR_COND_TARGET | AOE_EDITOR_COND_AREA;
            p.help_id = kind == 3 ? 0x7766 : 0x7767;
            p.focus = AOE_EDITOR_COND_AMOUNT;
            break;
        case 5:
            p.shown = AOE_EDITOR_COND_CLASS | amount | player;
            p.help_id = 0x7768;
            p.tab_count = 2;
            p.focus = AOE_EDITOR_COND_PLAYER;
            break;
        case 6:
        case 9:
            p.shown = AOE_EDITOR_COND_UNIT;
            break;
        case 7:
        case 8:
            p.shown = player | (kind == 7 ? AOE_EDITOR_COND_CLASS : 0);
            p.focus = AOE_EDITOR_COND_PLAYER;
            break;
        case 10: case 11: case 12: case 13: case 14: case 16:
            p.shown = amount;
            p.help_id = kind == 16 ? 0x776e : 0x7769 + (kind - 10);
            p.focus = AOE_EDITOR_COND_AMOUNT;
            break;
        case 15:
            p.shown = AOE_EDITOR_COND_TECH;
            break;
        case 17:
            p.shown = amount | AOE_EDITOR_COND_SUBTYPE;
            p.help_id = 0x776f;
            p.focus = AOE_EDITOR_COND_AMOUNT;
            break;
        case 18:
            p.shown = AOE_EDITOR_COND_RELIC;
            break;
        default:
            ok = 0;
        }
    }
    if (plan) *plan = p;
    return ok;
}

/* ---- 0x00490e30: the editor screen's release order ---- */

static const int16_t release_a[] = {0x520,0x508,0x514,0x50c,0x518,0x510,0x51c,0x524,0x528,0x52c,0x530,0x538,0x534,0x550,0x53c,0x540,0x544,0x548,0x54c,0x554,0x558,0x56c,0x55c,0x570,0x560,0x574,0x564,0x578,0x568,0x57c,0x580,0x584,0x590,0x588,0x594,0x58c,0x598,0x59c,0x5a0,0x5a4,0x5a8,0x5ac,0x5bc,0x5b0,0x5f4,0x60c,0x610,0x614,0x5b4,0x5b8,0x5c4,0x5d8,0x5c8,0x5dc,0x5cc,0x5e0,0x5d0,0x5e4,0x5d4,0x5e8,0x5ec,0x5f8,0x5f0,0x5fc,0x600,0x604,0x608,0x618,0x61c,0x62c,0x620,0x630,0x624,0x634,0x628,0x638,0x63c,0x640};
static const int16_t release_b[] = {0x668,0x66c,0x670,0x674,0x678,0x67c,0x680,0x694,0x684,0x698,0x68c,0x690,0x688,0x69c,0x6a0,0x6a4,0x6a8,0x6ac,0x6b0,0x6b4,0x6b8,0x6bc,0x6c0,0x6c4,0x6c8,0x6cc,0x6e0,0x6d0,0x6e4,0x6d4,0x6e8,0x6d8,0x6ec,0x6dc,0x6f0,0x6f4,0x6f8,0x6fc,0x700,0x704,0x708,0x70c,0x710,0x714,0x718,0x71c,0x720,0x724,0x728,0x72c,0x730,0x734,0x738,0x73c,0x748,0x74c,0x750,0x754,0x758,0x75c,0x760,0x764,0x76c,0x740,0x744,0x768,0x770,0x778,0x78c,0x77c,0x790,0x780,0x794,0x784,0x798,0x788,0x79c,0x774,0x7a4,0x7b4,0x7a8,0x7b8,0x7ac,0x7bc,0x7b0,0x7c0,0x7c4,0x7c8,0x7cc,0x7d0,0x7d4,0x814,0x7d8,0x818,0x7dc,0x81c,0x7e0,0x820,0x7e4,0x824,0x7e8,0x828,0x7ec,0x82c,0x7f0,0x830,0x7f4,0x834,0x7f8,0x838,0x7fc,0x83c,0x800,0x840,0x804,0x844,0x808,0x848,0x80c,0x84c,0x810,0x850,0x894,0x874,0x8a8,0x8ac,0x8b0,0x908,0x894,0x878,0x8b4,0x8b8,0x8bc,0x90c,0x894,0x87c,0x8c0,0x8c4,0x8c8,0x910,0x894,0x880,0x8cc,0x8d0,0x8d4,0x914,0x894,0x884,0x8d8,0x8dc,0x8e0,0x918,0x894,0x888,0x8e4,0x8e8,0x8ec,0x91c,0x894,0x88c,0x8f0,0x8f4,0x8f8,0x920,0x894,0x890,0x8fc,0x900,0x904,0x924,0x898,0x89c,0x8a0,0x8a4,0x500,0x504,0x4d8,0x4dc,0x4e0,0x4e4,0x4e8,0x4ec,0x4f0,0x4f4,0x4f8,0x4fc,0x4d4,0x4d0,0x4c8,0x4cc};

int32_t aoe_scenario_editor_screen_panel_count_ror(void) {
    return (int32_t)(sizeof release_a / sizeof release_a[0] +
                     sizeof release_b / sizeof release_b[0]);
}

void aoe_scenario_editor_screen_release_ror(const AoeScenarioEditorReleaseOpsRor *o) {
    static const char *const dialogs[5] = {"QuitSaveDialog", "OpenSaveDialog", "NewSaveDialog",
                                           "Save Game Screen", "Scenario Menu Dialog"};
    size_t i;
    if (!o) return;
    for (i = 0; i < 5; ++i)
        if (o->close_dialog) o->close_dialog(o->context, dialogs[i]);
    for (i = 0; i < sizeof release_a / sizeof release_a[0]; ++i)
        if (o->delete_panel) o->delete_panel(o->context, release_a[i]);
    if (o->free_buffer) o->free_buffer(o->context, 0x644);
    if (o->delete_object) o->delete_object(o->context, 0x660);
    for (i = 0; i < 6; ++i)
        if (o->free_shape) o->free_shape(o->context, 0x648 + (int32_t)i * 4);
    for (i = 0; i < sizeof release_b / sizeof release_b[0]; ++i)
        if (o->delete_panel) o->delete_panel(o->context, release_b[i]);
    if (o->free_shape) o->free_shape(o->context, 0x4c4);
    if (o->view_notify) o->view_notify(o->context);
    if (o->base_release) o->base_release(o->context);
}

void aoe_scenario_editor_attach_world_ror(void *world, const AoeScenarioEditorAttachOpsRor *o) {
    void *player;
    if (!o) return;
    if (o->view_set_map) o->view_set_map(o->context, world);
    player = o->viewed_player ? o->viewed_player(o->context) : NULL;
    if (o->view_set_player) o->view_set_player(o->context, player);
    if (o->minimap_set_world) o->minimap_set_world(o->context, world);
    if (o->minimap_set_player) o->minimap_set_player(o->context, player);
}

int aoe_scenario_editor_key_ror(AoeEditorKeyStateRor *st, int32_t key, int32_t alt,
                                int32_t modal, AoeEditorKeyResultRor *out) {
    static const struct { int32_t key, page; } pages[] = {
        {'C',10},{'D',5},{'I',6},{'M',1},{'O',8},{'P',3},{'T',2},{'U',4},{'V',7},{'W',9}};
    AoeEditorKeyResultRor r = {AOE_EDKEY_NONE, 0, 0, 0, 0, 0};
    size_t i;
    int32_t n;
    if (out) *out = r;
    if (!st || modal) return 0;
    if (alt) {
        if (key == 'A') {
            st->grid = st->grid == 3 ? 0 : 3;
            r.action = AOE_EDKEY_GRID;
            if (out) *out = r;
            return 1;
        }
        if (key == 'B') {
            st->cycle++;
            if (st->cycle >= (st->cycle_wide ? 5 : 2)) st->cycle = 0;
            r.action = AOE_EDKEY_CYCLE;
            if (out) *out = r;
            return 0;
        }
        for (i = 0; i < sizeof pages / sizeof pages[0]; ++i)
            if (pages[i].key == key) {
                r.action = AOE_EDKEY_PAGE;
                r.page = pages[i].page;
                if (out) *out = r;
                return 0;
            }
        if (key >= 0x41 && key <= 0x57) return 0;
    }
    if (key == 9) {
        if (st->page == 3 && !st->edit_focused) r.action = AOE_EDKEY_FOCUS_PLAYERS;
        else if (st->page == 6 && !st->edit_focused && st->victory_items)
            r.action = AOE_EDKEY_FOCUS_VICTORY;
    } else if (key == 0x1b) {
        r.action = AOE_EDKEY_CLEAR;
    } else if (key >= '0' && key <= '8') {
        n = key - '0';
        r.action = AOE_EDKEY_DROPDOWN;
        r.dropdown_index = n - 1;
        if (st->page == 8 && n > 0) {
            r.panel_offset = 0x7cc; r.notify_arg = 3; r.player = n;
        } else if (st->page == 3) {
            if (n > 0 && !st->edit_focused) r.panel_offset = 0x5b4;
            else r.action = AOE_EDKEY_NONE;
        } else if (st->page == 4) {
            r.dropdown_index = n; r.panel_offset = 0x618;
        } else if (st->page == 6) {
            if (n > 0 && !st->edit_focused) { r.panel_offset = 0x73c; r.notify_arg = 1; }
            else r.action = AOE_EDKEY_NONE;
        } else if (st->page == 5) {
            if (n > 0) { r.panel_offset = 0x894; r.notify_arg = 1; }
            else r.action = AOE_EDKEY_NONE;
        } else if (n > 0) {
            r.action = AOE_EDKEY_PLAYER; r.dropdown_index = 0; r.player = n;
        } else r.action = AOE_EDKEY_NONE;
    }
    if (out) *out = r;
    return 0;
}

int aoe_scenario_editor_resize_ror(int32_t w, int32_t h, const AoeScenarioEditorResizeOpsRor *o) {
    if (!o) return 0;
    if (o->base_set_rect) o->base_set_rect(o->context, w, h);
    if (o->layout) o->layout(o->context);
    if (o->base_set_rect) o->base_set_rect(o->context, w, h);
    return 0;
}

int aoe_scenario_editor_new_scenario_ror(const AoeScenarioEditorNewOpsRor *o, int32_t kind,
                                         int32_t arg, int16_t size_a, int16_t size_b,
                                         int with_status) {
    char names[8][0x41];
    uint8_t civs[8], teams[8];
    int16_t slots[8];
    int32_t order[9];
    int32_t i, team = 0, max;
    int ok;
    if (!o) return 0;
    if (!o->has_world(o->context)) {
        if (with_status) o->status(o->context, 0x44d);
        if (!o->ensure_world(o->context)) {
            if (with_status) o->status_close(o->context);
            return 0;
        }
    }
    if (with_status) o->status(o->context, kind == 0 ? 0x44e : 0x44f);
    max = o->max_team(o->context);
    for (i = 0; i < 8; ++i) {
        if (++team >= max) team = 1;
        o->player_name(o->context, 0x2865 + i, names[i], 0x41);
        civs[i] = 0;
        teams[i] = (uint8_t)team;
        slots[i] = -1;
        order[i] = i;
    }
    order[8] = 8;
    o->set_busy(o->context, 1);
    ok = o->generate(o->context, kind, names, civs, teams, slots, order, size_a, size_b, arg);
    if (ok && o->world_player_count(o->context) < 8) {
        ok = 0;
        o->clear_world(o->context);
    }
    o->set_busy(o->context, 0);
    if (ok && o->has_player_list) {
        for (i = 0; i < 8; ++i) {
            if (!o->slot_active(o->context, i)) {
                if (i > 0 && i < 8) o->select_count(o->context, i - 1);
                break;
            }
        }
        o->player_count_changed(o->context);
    }
    return ok;
}

/* 0x00496a40 */
void aoe_scenario_editor_clear_controls_ror(AoeScenarioEditorControlsRor *c) {
    c->ptr_47c = NULL;
    memset(c->group_4c4, 0, sizeof c->group_4c4);
    memset(c->group_5c4, 0, sizeof c->group_5c4);
    memset(c->group_618, 0, sizeof c->group_618);
    c->field_664 = 0;
    memset(c->group_668, 0, sizeof c->group_668);
    c->ptr_894 = NULL;
}

/* 0x00490450 */
int aoe_scenario_editor_open_dropdown_ror(const AoeScenarioEditorDropdownOpsRor *o, int32_t offset) {
    int32_t width, select = 0, i;
    int known = 1;
    switch (offset) {
    case 0x738: case 0x744: case 0x748: case 0x74c: case 0x600: case 0x604: case 0x608: case 0x700:
        width = 0xdc; break;
    case 0x734: width = 0xb4; break;
    case 0x7b4: case 0x7b8: case 0x7bc: case 0x7c0: case 0x528: case 0x530: case 0x538:
        width = 0xaa; break;
    case 0x5bc: case 0x5b4: case 0x5b8: case 0x618: case 0x73c: case 0x7cc: case 0x894: case 0x740:
        width = 0x96; break;
    default: width = 0x82; break;
    }
    if (!o->create(o->context, width)) return 0;
    o->to_front(o->context);
    o->hide(o->context);
    o->clear(o->context);
    switch (offset) {
    case 0x530: for (i = 0; i < 6; ++i) o->add_id(o->context, 0x2973 + i, 0); select = 3; break;
    case 0x538: for (i = 0; i < 9; ++i) o->add_id(o->context, 0x296a + i, 0); select = 3; break;
    case 0x5b8: case 0x740: for (i = 0; i < 8; ++i) o->add_id(o->context, 0x2877 + i, 0); break;
    case 0x744: {
        static const int32_t pairs[7][2] = {{0x2882, 0x2b}, {0x2883, 0x29}, {0x2884, 0x2c}, {0x2886, 0x28},
                                            {0x2887, 0x15}, {0x2888, 0x14}, {0x2889, 0x25}};
        for (i = 0; i < 7; ++i) o->add_id(o->context, pairs[i][0], pairs[i][1]);
        break;
    }
    case 0x528: o->add_id(o->context, 0x297d, 0); o->add_id(o->context, 0x297e, 0); o->add_id(o->context, 0x2980, 0); o->add_id(o->context, 0x2985, 0); break;
    case 0x5f8: o->add_id(o->context, 0x27e3, 0); o->add_id(o->context, 0x27e4, 0); break;
    case 0x734: for (i = 0x2851; i <= 0x2862; ++i) o->add_id(o->context, i, 0); o->add_id(o->context, 0x284f, 0); break;
    case 0x5fc:
        for (i = 0x27f7; i <= 0x2802; ++i) o->add_id(o->context, i, 0);
        o->add_id(o->context, 0x2806, 0); o->add_id(o->context, 0x2807, 0); o->add_id(o->context, 0x2809, 0); o->add_id(o->context, 0x2808, 0);
        break;
    case 0x748: for (i = 0; i < 4; ++i) o->add_id(o->context, 0x1069 + i, 0); break;
    case 0x74c:
        o->sorted(o->context, 1);
        for (i = 0; i < o->scenario_count; ++i) {
            const char *n = o->scenario_name(o->context, i);
            if (n && *n) o->add_text(o->context, n, i);
        }
        break;
    case 0x608: o->files(o->context, 0, ".per", 0, 1); break;
    case 0x600: o->files(o->context, 0, ".ai", 1, 1); break;
    case 0x604: o->files(o->context, 0, ".cty", 1, 0); break;
    case 0x7b4: case 0x7b8: case 0x7bc: o->files(o->context, 1, ".avi", 1, 0); break;
    case 0x7c0: o->files(o->context, 2, ".bmp", 1, 0); break;
    case 0x738: {
        char buf[0x100];
        o->sorted(o->context, 1);
        o->clear(o->context);
        for (i = 0; i < o->template_count; ++i) {
            const char *n = o->template_name(o->context, i, buf, (int)sizeof buf);
            if (n) o->add_text(o->context, n, i);
        }
        break;
    }
    default: known = 0; break;
    }
    if (known) o->select(o->context, select);
    return !o->failed;
}

/* 0x00499160: the fixed rectangles, in the original order. */
static const AoeEditorRectRor rects_a[45] = {
    {0x520, 4, 4, 0x78, 0x16}, {0x508, 4, 0x16, 0x16, 0x16}, {0x50c, 4, 0x2c, 0x16, 0x16},
    {0x510, 4, 0x42, 0x16, 0x16}, {0x514, 0x24, 0x16, 0x73, 0x16}, {0x518, 0x24, 0x2c, 0x73, 0x16},
    {0x51c, 0x24, 0x42, 0x73, 0x16}, {0x52c, 0xaa, 4, 0x82, 0x16}, {0x530, 0xaa, 0x16, 0xaa, 0x16},
    {0x534, 0xaa, 0x30, 0x82, 0x16}, {0x538, 0xaa, 0x42, 0xaa, 0x16}, {0x524, 0xaa, 0x30, 0x82, 0x16},
    {0x528, 0xaa, 0x42, 0xaa, 0x16}, {0x53c, 0x24, 0x5a, 0x78, 0x16}, {0x540, 0x24, 0x70, 0x37, 0x16},
    {0x544, 0x24, 0x5a, 0x78, 0x16}, {0x548, 0x24, 0x70, 0x37, 0x16}, {0x54c, 0xb4, 0x60, 0x82, 0x26},
    {0x5a0, 0x73, 0x17, 0xa5, 0x6e}, {0x59c, 0x73, 1, 0x3c, 0x16}, {0x5a8, 0x73, 0x17, 0xa5, 0x6e},
    {0x580, 2, 1, 0x6e, 0x14}, {0x584, 2, 0x17, 0x1e, 0x16}, {0x588, 2, 0x2d, 0x1e, 0x16},
    {0x58c, 2, 0x43, 0x1e, 0x16}, {0x590, 0x21, 0x17, 0x51, 0x16}, {0x594, 0x21, 0x2d, 0x51, 0x16},
    {0x598, 0x21, 0x43, 0x51, 0x16}, {0x554, 0x118, 4, 0x6e, 0x14}, {0x558, 0x11a, 0x17, 0x1e, 0x16},
    {0x55c, 0x11a, 0x2d, 0x1e, 0x16}, {0x560, 0x11a, 0x43, 0x1e, 0x16}, {0x564, 0x11a, 0x59, 0x1e, 0x16},
    {0x568, 0x11a, 0x6f, 0x1e, 0x16}, {0x56c, 0x139, 0x17, 0x5a, 0x16}, {0x570, 0x139, 0x2d, 0x5a, 0x16},
    {0x574, 0x139, 0x43, 0x5a, 0x16}, {0x578, 0x139, 0x59, 0x5a, 0x16}, {0x57c, 0x139, 0x6f, 0x5a, 0x16},
    {0x5b0, 4, 4, 0x50, 0x12}, {0x5f4, 4, 0x5c, 0x5f, 0x12}, {0x5bc, 4, 0x6e, 0x96, 0x16},
    {0x5b0, 4, 0x2a, 0x96, 0x12}, {0x5b4, 4, 7, 0x96, 0x16}, {0x5b8, 4, 0x3e, 0x96, 0x16},
};
static const AoeEditorRectRor rects_b[52] = {
    {0x5d4, 0xa6, 0x5c, 100, 0x12}, {0x5e8, 0xa6, 0x6e, 0x7d, 0x16}, {0x600, 300, 0x45, 0xbe, 0x16},
    {0x604, 300, 0x19, 0xbe, 0x16}, {0x60c, 300, 0x33, 0x82, 0x12}, {0x610, 300, 7, 0x82, 0x12},
    {0x614, 300, 0x5c, 0x82, 0x12}, {0x608, 300, 0x6e, 0xbe, 0x16}, {0x618, 4, 4, 0x96, 0x16},
    {0x61c, 0xa0, 0x16, 0x1e, 0x16}, {0x620, 0xa0, 0x30, 0x1e, 0x16}, {0x624, 0xa0, 0x4a, 0x1e, 0x16},
    {0x628, 0xa0, 0x62, 0x1e, 0x16}, {0x62c, 0xc0, 0x16, 0x6e, 0x16}, {0x630, 0xc0, 0x30, 0x6e, 0x16},
    {0x634, 0xc0, 0x4a, 0x6e, 0x16}, {0x638, 0xc0, 0x62, 0x6e, 0x16}, {0x688, 0x22, 0, 0x78, 0x12},
    {0x6e0, 0xe, 0x12, 0x14, 0x12}, {0x6e4, 0xe, 0x26, 0x14, 0x12}, {0x6e8, 0xe, 0x3a, 0x14, 0x12},
    {0x6ec, 0xe, 0x4e, 0x14, 0x12}, {0x6f0, 0xe, 0x62, 0x14, 0x12}, {0x6cc, 0x22, 0x12, 0x78, 0x12},
    {0x6d0, 0x22, 0x26, 0x78, 0x12}, {0x6d4, 0x22, 0x3a, 0x78, 0x12}, {0x6d8, 0x22, 0x4e, 0x78, 0x12},
    {0x6dc, 0x22, 0x62, 0x78, 0x12}, {0x6f4, 0xa4, 0x12, 100, 0x12}, {0x6f8, 0xa4, 0x28, 100, 0x16},
    {0x6fc, 0xa4, 0x12, 0xdc, 0x12}, {0x700, 0xa4, 0x28, 0xdc, 0x16}, {0x668, 0xa4, 0x12, 0x14, 0x12},
    {0x66c, 0xa4, 0x26, 0x14, 0x12}, {0x670, 0xa4, 0x3a, 0x14, 0x12}, {0x674, 0xa4, 0x4e, 0x14, 0x12},
    {0x678, 0xa4, 0x62, 0x14, 0x12}, {0x67c, 0xa4, 0x76, 0x14, 0x12}, {0x69c, 0xb8, 0x12, 0x82, 0x12},
    {0x6a0, 0xb8, 0x26, 0x82, 0x12}, {0x6a8, 0xb8, 0x3a, 0x82, 0x12}, {0x6ac, 0xb8, 0x4e, 0x82, 0x12},
    {0x6b0, 0xb8, 0x62, 0x82, 0x12}, {0x68c, 0x126, 0, 0x5a, 0x12}, {0x6b8, 0x126, 0x26, 0x3c, 0x12},
    {0x6bc, 0x126, 0x3a, 0x3c, 0x12}, {0x6c0, 0x126, 0x4e, 0x3c, 0x12}, {0x6c4, 0x126, 0x62, 0x3c, 0x12},
    {0x680, 0x180, 0x12, 0x14, 0x12}, {0x684, 0x180, 0x26, 0x14, 0x12}, {0x694, 0x194, 0x12, 0x5a, 0x12},
    {0x698, 0x194, 0x26, 0x5a, 0x12},
};
static const AoeEditorRectRor rects_c[14] = {
    {0x73c, 4, 4, 0x96, 0x16}, {0x734, 4, 0x1c, 0xb4, 0x16}, {0x75c, 0xdc, 4, 0x8c, 0x16},
    {0x750, 4, 0x38, 0x96, 0x16}, {0x754, 4, 0x52, 0x96, 0x16}, {0x758, 4, 0x6c, 0x96, 0x16},
    {0x764, 0xa0, 0x50, 0x3c, 0x16}, {0x76c, 0xa0, 0x66, 0x32, 0x16}, {0x740, 4, 0x66, 0x96, 0x16},
    {0x744, 4, 0x38, 0xb4, 0x16}, {0x768, 4, 0x50, 0x96, 0x16}, {0x738, 4, 0x38, 0xb4, 0x16},
    {0x748, 4, 0x38, 0xb4, 0x16}, {0x74c, 4, 0x38, 0xb4, 0x16},
};
static const AoeEditorRectRor rects_d[6] = {
    {0x7b0, 0xb2, 4, 0xaa, 0x12}, {0x7c0, 0xb2, 0x16, 0xaa, 0x16}, {0x7c8, 3, 0x2c, 0x14, 0x14},
    {0x7c4, 0x16, 0x2c, 0x87, 0x14}, {0x7d0, 0xa0, 3, 0xa0, 0x16}, {0x7cc, 3, 3, 0x96, 0x16},
};
static const AoeEditorRectRor rects_e[1] = {
    {0x894, 4, 4, 0x96, 0x16},
};
static const AoeEditorRectRor rects_f[4] = {
    {0x898, 2, 0x34, 0x91, 0x16}, {0x89c, 2, 0x48, 0x91, 0x16}, {0x8a0, 2, 0x5c, 0x91, 0x16},
    {0x8a4, 2, 0x70, 0x91, 0x16},
};

static void mode(const AoeEditorLayoutOpsRor *o, uint16_t off, int32_t a, int32_t b, int32_t c,
                 int32_t d, int32_t e, int32_t f, int32_t g, int32_t h, int32_t i, uint16_t r0,
                 uint16_t r1, uint16_t r2, uint16_t r3) {
    AoeEditorModeRor m;
    m.v[0] = a; m.v[1] = b; m.v[2] = c; m.v[3] = d; m.v[4] = e; m.v[5] = f; m.v[6] = g; m.v[7] = h;
    m.v[8] = i;
    m.refs[0] = r0; m.refs[1] = r1; m.refs[2] = r2; m.refs[3] = r3;
    o->position_mode(o->context, off, &m);
}
static void rows(const AoeEditorLayoutOpsRor *o, const AoeEditorRectRor *t, int n) {
    int i;
    for (i = 0; i < n; ++i) o->set_rect(o->context, t[i].offset, t[i].x, t[i].y, t[i].w, t[i].h);
}
void aoe_scenario_editor_layout_ror(const AoeEditorLayoutOpsRor *o) {
    int i, k;
    uint16_t ref;
    if (!o->has_controls) return;
    for (i = 0; i < 5; ++i)
        mode(o, (uint16_t)(0x4d8 + 4 * i), 7, 2, 2, 2, 2, 0x32, 0x6d, 0x16, 0x16,
             (uint16_t)(i == 0 ? 0 : 0x4d4 + 4 * i), 0, 0, 0);
    for (i = 5; i < 10; ++i)
        mode(o, (uint16_t)(0x4d8 + 4 * i), 7, 2, 2, 2, 2, 0x32, 0x6d, 0x16, 0x16,
             (uint16_t)(i == 5 ? 0 : 0x4d4 + 4 * i), (uint16_t)(0x4c4 + 4 * i), 0, 0);
    mode(o, 0x500, 9, 2, 5, 3, 3, 0x3a, 0x3a, 0x14, 0x28, 0, 0, 0, 0);
    mode(o, 0x504, 10, 5, 5, 5, 5, 0x14, 0x1e, 0x14, 0x1e, 0, 0, 0, 0);
    mode(o, 0x4cc, 10, 0, 0, 0, 0, 0xe6, 0xe6, 0x87, 0x87, 0, 0, 0, 0);
    mode(o, 0x4d4, 8, 2, 2, 2, 2, 0, 0, 0x89, 0x89, 0, 0, 0x4cc, 0);
    mode(o, 0x63c, 7, 0, 6, 0, 4, 0x7c, 200, 0, 0, 0, 0x500, 0, 0x4d4);
    mode(o, 0x660, 8, 4, 4, 4, 4, 0x96, 0x96, 0x6c, 0x6c, 0, 0, 0, 0);
    ref = 0x63c;
    if (o->lines_overflow) {
        mode(o, 0x63c, 7, 0, 6, 0, 4, 0x68, 0xdc, 0, 0, 0, 0x500, 0, 0x4d4);
        mode(o, 0x640, 7, 1, 1, 0, 2, 0x12, 0x12, 0, 0, 0x63c, 0x500, 0, 0x4d4);
        ref = 0x640;
    }
    mode(o, 0x4c8, 7, 0, 6, 0, 4, 0, 0, 0, 0, ref, 0x500, 0, 0x4d4);
    mode(o, 0x4d0, 8, 4, 4, 4, 4, 0, 0, 0x18, 0x18, 0, 0, 0, 0);
    rows(o, rects_a, (int)(sizeof rects_a / sizeof rects_a[0]));
    for (k = 0; k < 4; ++k) {
        int32_t x = 0xa6 + 0x44 * (k / 2), y = 0x19 + 0x2c * (k % 2);
        o->set_rect(o->context, (uint16_t)(0x5c4 + 4 * k), x, y - 0x12, 0x37, 0x12);
        o->set_rect(o->context, (uint16_t)(0x5d8 + 4 * k), x, y, 0x37, 0x16);
    }
    for (k = 0; k < 2; ++k) {
        int32_t y = 0x19 + 0x2c * k;
        o->set_rect(o->context, (uint16_t)(0x5ec + 4 * k), 500, y - 0x12, 0x82, 0x12);
        o->set_rect(o->context, (uint16_t)(0x5f8 + 4 * k), 500, y, 0x82, 0x16);
    }
    rows(o, rects_b, (int)(sizeof rects_b / sizeof rects_b[0]));
    for (i = 0; i < 12; ++i)
        o->set_rect(o->context, (uint16_t)(0x704 + 4 * i), (i / 4) * 0x30 + 0xdc, (i % 4) * 0x1a + 0x1e,
                    0x2c, 0x16);
    rows(o, rects_c, (int)(sizeof rects_c / sizeof rects_c[0]));
    for (k = 0; k < 5; ++k) {
        o->set_rect(o->context, (uint16_t)(0x778 + 4 * k), 4, 8 + 0x19 * k, 0x1e, 0x12);
        o->set_rect(o->context, (uint16_t)(0x78c + 4 * k), 0x24, 8 + 0x19 * k, 0x82, 0x12);
    }
    mode(o, 0x774, 7, 0xaf, 4, 0x28, 4, 0, 0, 0, 0, 0, 0, 0, 0);
    for (k = 0; k < 3; ++k) {
        o->set_rect(o->context, (uint16_t)(0x7a4 + 4 * k), 4, 0x16 + 0x2c * k - 0x12, 0xaa, 0x12);
        o->set_rect(o->context, (uint16_t)(0x7b4 + 4 * k), 4, 0x16 + 0x2c * k, 0xaa, 0x16);
    }
    rows(o, rects_d, (int)(sizeof rects_d / sizeof rects_d[0]));
    for (k = 0; k < 5; ++k) {
        o->set_rect(o->context, (uint16_t)(0x7d4 + 4 * k), 0xa0, 0x1a + 0x14 * k, 0x14, 0x14);
        o->set_rect(o->context, (uint16_t)(0x814 + 4 * k), 0xb4, 0x1a + 0x14 * k, 0x87, 0x14);
    }
    for (k = 0; k < 5; ++k) {
        o->set_rect(o->context, (uint16_t)(0x7e8 + 4 * k), 0x13b, 0x1a + 0x14 * k, 0x14, 0x14);
        o->set_rect(o->context, (uint16_t)(0x828 + 4 * k), 0x14f, 0x1a + 0x14 * k, 0x87, 0x14);
    }
    for (k = 0; k < 6; ++k) {
        o->set_rect(o->context, (uint16_t)(0x7fc + 4 * k), 0x1d6, 6 + 0x14 * k, 0x14, 0x14);
        o->set_rect(o->context, (uint16_t)(0x83c + 4 * k), 0x1ea, 6 + 0x14 * k, 0x87, 0x14);
    }
    rows(o, rects_e, (int)(sizeof rects_e / sizeof rects_e[0]));
    for (k = 0; k < 8; ++k) {
        int32_t x = 0x96 + 0x1f * k;
        o->set_rect(o->context, (uint16_t)(0x874 + 4 * k), x, 0x1c, 0x1e, 0x16);
        o->set_rect(o->context, (uint16_t)(0x8a8 + 12 * k), x, 0x34, 0x1a, 0x12);
        o->set_rect(o->context, (uint16_t)(0x8ac + 12 * k), x, 0x48, 0x1a, 0x12);
        o->set_rect(o->context, (uint16_t)(0x8b0 + 12 * k), x, 0x5c, 0x1a, 0x12);
        o->set_rect(o->context, (uint16_t)(0x908 + 4 * k), x, 0x70, 0x1a, 0x12);
    }
    rows(o, rects_f, (int)(sizeof rects_f / sizeof rects_f[0]));
}
