/* The panel side of the "MP Setup Screen" state's ops (patched RoR 1.0a,
   0x00487f40 onwards): the AoeMpSetupScreenOpsRor callbacks that reach
   only the screen's own panels, over the AoeMpSetupUiRor that holds them.
   The game and comm side of the ops (settings, comm, dialogs, screens,
   the chat log) stay the director's and are not set here. A panel the
   constructor failed to make is NULL; every callback tolerates that. */
#include "mp_setup_panels.h"

#include "ui_button.h"
#include "ui_dropdown.h"
#include "ui_edit.h"
#include "ui_panel.h"
#include "ui_text.h"

#define UI(c) ((AoeMpSetupUiRor *)(c))

/* ---- the player-count dropdown (+0x6e0) ---- */

/* 0x00461f10: the dropdown cleared. */
static void count_clear(void *c) {
    if (UI(c)->count) aoe_ui_dropdown_clear_ror(UI(c)->count);
}

/* 0x00461da0: a line with its ID appended. */
static void count_append(void *c, const char *text, int32_t id) {
    if (UI(c)->count) aoe_ui_dropdown_append_ror(UI(c)->count, text, id);
}

/* 0x00461e70: the index of an ID (-1 when absent). */
static int32_t count_index(void *c, int32_t id) {
    return UI(c)->count ? aoe_ui_dropdown_index_of_id_ror(UI(c)->count, id) : -1;
}

/* 0x00461d50: the line at `index` selected. */
static void count_select(void *c, int32_t index) {
    if (UI(c)->count) aoe_ui_dropdown_select_ror(UI(c)->count, (int16_t)index);
}

/* ---- the colour buttons (+0x570 + 4 i) ---- */

static AoeUiButtonRor *color_button(void *c, int32_t button) {
    return button >= 0 && button < AOE_MP_SETUP_PLAYERS ? UI(c)->color_buttons[button] : NULL;
}

/* 0x0045ef90: the text of state `state`. */
static void color_text(void *c, int32_t button, int32_t state, const char *text) {
    AoeUiButtonRor *b = color_button(c, button);
    if (b) aoe_ui_button_set_text_ror(b, (int16_t)state, text);
}

/* 0x0045ef40: the ID (and value 0) of state `state`. */
static void color_state(void *c, int32_t button, int32_t state, int32_t id) {
    AoeUiButtonRor *b = color_button(c, button);
    if (b) aoe_ui_button_set_id_ror(b, (int16_t)state, id, 0);
}

/* 0x0045f210(state, colour, 0): the fill 0x0048d250 passes no shadow colour. */
static void color_rgb(void *c, int32_t button, int32_t state, uint32_t rgb) {
    AoeUiButtonRor *b = color_button(c, button);
    if (b) aoe_ui_button_set_text_color_ror(b, state, rgb, 0);
}

/* 0x0045ef20: the number of states the button cycles through. */
static void color_count(void *c, int32_t button, int32_t count) {
    AoeUiButtonRor *b = color_button(c, button);
    if (b) aoe_ui_button_set_cycle_ror(b, (int16_t)count);
}

/* ---- the chat text (+0x618) ---- */

/* 0x0046a0b0: the line count. */
static int16_t chat_count(void *c) {
    return UI(c)->chat ? aoe_ui_text_count_ror(UI(c)->chat) : 0;
}

/* +0xfe: the last visible line (read directly; no accessor). */
static int16_t chat_bottom(void *c) {
    return UI(c)->chat ? UI(c)->chat->bottom : -1;
}

/* 0x0046a480: line 0 removed. */
static void chat_remove_first(void *c) {
    if (UI(c)->chat) aoe_ui_text_remove_ror(UI(c)->chat, 0);
}

/* 0x00468ba0: the text word-wrapped onto new lines. */
static void chat_add(void *c, const char *text) {
    if (UI(c)->chat) aoe_ui_text_wrap_ror(UI(c)->chat, text);
}

/* 0x00469500(7, 0, 1): scrolled to the bottom, the scrollbar updated. */
static void chat_scroll_bottom(void *c) {
    if (UI(c)->chat) aoe_ui_text_scroll_ror(UI(c)->chat, 7, 0, 1);
}

/* chat_line (0x0041c9f0) is the game's chat log line i (chat_log.h), not
   a panel's: it is the director's and is left unset here. */

/* ---- tab order and focus ---- */

/* vtable +0xcc (0x004540e0): a cyclic tab order over `n` panels. */
static void tab_order(void *c, void *const *panels, int32_t n) {
    (void)c;
    aoe_ui_panels_tab_cycle_ror((AoeUiPanelRor **)panels, (int16_t)n);
}

/* 0x00453eb0: the screen's focused child. */
static void focus(void *c, void *panel) {
    aoe_ui_panel_set_current_ror(&UI(c)->ez.panel, (AoeUiPanelRor *)panel);
}

/* ---- the action handler's panel accessors (0x004895b0) ---- */

/* +0x6c: the panel is active. */
static int panel_active(void *c, void *panel) {
    (void)c;
    return panel && ((AoeUiPanelRor *)panel)->active != 0;
}

/* 0x0045f400: a button's state. */
static int32_t button_state(void *c, void *panel) {
    (void)c;
    return panel ? aoe_ui_button_state_ror((AoeUiButtonRor *)panel) : 0;
}

/* 0x0045f410: a button's value. */
static int32_t button_value(void *c, void *panel) {
    (void)c;
    return panel ? aoe_ui_button_value_ror((AoeUiButtonRor *)panel) : 0;
}

/* 0x0045f3a0: a button's state set. */
static void button_set_state(void *c, void *panel, int32_t state) {
    (void)c;
    if (panel) aoe_ui_button_set_state_ror((AoeUiButtonRor *)panel, (int16_t)state);
}

/* 0x00461ed0: a list's selected item ID. */
static int32_t list_id(void *c, void *panel) {
    (void)c;
    return panel ? aoe_ui_dropdown_selected_id_ror((AoeUiDropdownRor *)panel) : -1;
}

/* 0x00461e50: a list's selected index. */
static int32_t list_selected(void *c, void *panel) {
    (void)c;
    return panel ? aoe_ui_dropdown_selected_ror((AoeUiDropdownRor *)panel) : -1;
}

/* 0x00461e70: the index of an ID in a list (-1 when absent). */
static int32_t list_index(void *c, void *panel, int32_t id) {
    (void)c;
    return panel ? aoe_ui_dropdown_index_of_id_ror((AoeUiDropdownRor *)panel, id) : -1;
}

/* 0x00461d50: a list's line at `index` selected. */
static void list_select(void *c, void *panel, int32_t index) {
    (void)c;
    if (panel) aoe_ui_dropdown_select_ror((AoeUiDropdownRor *)panel, (int16_t)index);
}

/* 0x00462560: an input's text (NULL when none). */
static const char *input_text(void *c, void *panel) {
    (void)c;
    return panel ? aoe_ui_edit_text_ror((AoeUiEditRor *)panel) : NULL;
}

/* 0x004623e0: an input's text set. */
static void input_set(void *c, void *panel, const char *text) {
    (void)c;
    if (panel) aoe_ui_edit_set_text_ror((AoeUiEditRor *)panel, text);
}

void aoe_mp_setup_panels_bind_ror(AoeMpSetupScreenOpsRor *ops, AoeMpSetupUiRor *u) {
    ops->context = u;
    ops->count_clear = count_clear;
    ops->count_append = count_append;
    ops->count_index = count_index;
    ops->count_select = count_select;
    ops->color_text = color_text;
    ops->color_state = color_state;
    ops->color_rgb = color_rgb;
    ops->color_count = color_count;
    ops->chat_count = chat_count;
    ops->chat_bottom = chat_bottom;
    ops->chat_remove_first = chat_remove_first;
    ops->chat_add = chat_add;
    ops->chat_scroll_bottom = chat_scroll_bottom;
    ops->tab_order = tab_order;
    ops->focus = focus;
    ops->panel_active = panel_active;
    ops->button_state = button_state;
    ops->button_value = button_value;
    ops->button_set_state = button_set_state;
    ops->list_id = list_id;
    ops->list_selected = list_selected;
    ops->list_index = list_index;
    ops->list_select = list_select;
    ops->input_text = input_text;
    ops->input_set = input_set;
}
