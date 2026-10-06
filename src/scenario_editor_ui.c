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
