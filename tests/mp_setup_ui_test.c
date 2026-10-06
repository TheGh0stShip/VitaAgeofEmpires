/* Patched RoR 1.0a "MP Setup Screen" constructor 0x00487f40 as panels. */
#include "game_ui.h"
#include "mp_setup_panels.h"
#include "mp_setup_screen.h"
#include "ui_dropdown.h"
#include "ui_text.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }
static void string(void *c, int32_t id, char *out, int size) {
    (void)c;
    snprintf(out, (size_t)size, "S%d", (int)id);
}
static int fonts(void *c, int32_t index, uintptr_t *font, int32_t *w, int32_t *h) {
    (void)c; *font = (uintptr_t)index + 1; *w = 8; *h = 12; return 1;
}

/* The state's game and comm ops. */
/* A fake native edit window for the chat input (as game_ui_test.c). */
static char edit_text[128];
static int edit_window;
static void *ed_create(void *c, AoeUiAreaRor *a, int32_t x, int32_t y, int32_t w, int32_t h, int m) {
    (void)c; (void)a; (void)x; (void)y; (void)w; (void)h; (void)m; edit_text[0] = 0; return &edit_window;
}
static void ed_destroy(void *c, void *w) { (void)c; (void)w; }
static void ed_set(void *c, void *w, const char *t) { (void)c; (void)w; snprintf(edit_text, sizeof(edit_text), "%s", t); }
static void ed_get(void *c, void *w, char *o, int32_t n) { (void)c; (void)w; snprintf(o, (size_t)n, "%s", edit_text); }
static int host_ret;
static int seeds, chat_drops, readies, chats, comm_slots;
static uint32_t recipients;
static int32_t ready_args[8];
static char chat_text[16];

static int host(void *c) { (void)c; return host_ret; }
static void game_slot_48(void *c) { (void)c; }
static void seed_now(void *c) { (void)c; ++seeds; }
static int32_t rand_fake(void *c) { (void)c; return 0; }
static void chat_drop(void *c) { (void)c; ++chat_drops; }
static void chat_recipient(void *c, int32_t player) { (void)c; recipients |= 1u << player; }
static void send_ready(void *c, int32_t ready, int32_t civ, int32_t team, int32_t cd,
                       int32_t color, int32_t flag, int32_t checksum, int32_t version) {
    (void)c;
    ++readies;
    ready_args[0] = ready; ready_args[1] = civ; ready_args[2] = team; ready_args[3] = cd;
    ready_args[4] = color; ready_args[5] = flag; ready_args[6] = checksum; ready_args[7] = version;
}
static void send_chat(void *c, const char *text) {
    (void)c;
    ++chats;
    snprintf(chat_text, sizeof chat_text, "%s", text ? text : "");
}
static void comm_slot(void *c, int32_t slot, int32_t state) { (void)c; (void)slot; (void)state; ++comm_slots; }
static void count_clear(void *c) { (void)c; }
static void count_append(void *c, const char *text, int32_t id) { (void)c; (void)text; (void)id; }
static int32_t count_index(void *c, int32_t id) { (void)c; (void)id; return 0; }
static void count_select(void *c, int32_t index) { (void)c; (void)index; }
static int scenario_slot_used(void *c, int32_t slot) { (void)c; (void)slot; return 0; }
static int32_t scenario_slot_kind(void *c, int32_t slot) { (void)c; (void)slot; return 0; }
static int32_t scenario_slot_civ(void *c, int32_t slot) { (void)c; (void)slot; return 0; }
static void free_scenario(void *c) { (void)c; }
static void close_named(void *c, const char *name) { (void)c; (void)name; }
static void release_panels(void *c) { (void)c; }

static void reset_counts(void) {
    seeds = chat_drops = readies = chats = comm_slots = 0;
    recipients = 0;
    memset(ready_args, -1, sizeof ready_args);
    memset(chat_text, 0, sizeof chat_text);
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 800, 600};
    AoeUiEzOpsRor ez_ops = {0};
    static AoeUiEditOpsRor eops;
    AoeGameUiOpsRor ops = {0};
    static AoeGameSettingsRor settings;
    static AoeGameUiRor ui;
    AoeMpSetupScreenOpsRor mp = {0};
    AoeMpSetupUiRor *u;
    int i;

    pf.text_width = width;
    pf.string = string;
    sys.platform = &pf;
    ez_ops.font = fonts;
    eops.create = ed_create; eops.destroy = ed_destroy;
    eops.set_text = ed_set; eops.get_text = ed_get;
    ez_ops.edit_ops = &eops;
    ui.system = &sys;
    ui.ez_ops = &ez_ops;
    ui.settings = &settings;
    ui.ops = &ops;

    mp.host = host;
    mp.game_slot_48 = game_slot_48;
    mp.seed_now = seed_now;
    mp.rand = rand_fake;
    mp.chat_drop = chat_drop;
    mp.chat_recipient = chat_recipient;
    mp.send_ready = send_ready;
    mp.send_chat = send_chat;
    mp.comm_slot = comm_slot;
    mp.count_clear = count_clear;
    mp.count_append = count_append;
    mp.count_index = count_index;
    mp.count_select = count_select;
    mp.scenario_slot_used = scenario_slot_used;
    mp.scenario_slot_kind = scenario_slot_kind;
    mp.scenario_slot_civ = scenario_slot_civ;
    mp.free_scenario = free_scenario;
    mp.close_named = close_named;
    mp.release_panels = release_panels;

    /* 1. Single player: +0x97d set, not multiplayer, not hosting. */
    reset_counts();
    settings.flag_97d = 1;
    settings.flag_97e = 0;
    settings.flag_8fc = 0;
    host_ret = 0;
    u = calloc(1, sizeof(*u));
    aoe_mp_setup_panels_bind_ror(&mp, u); /* the count and colour ops on the panels */
    assert(aoe_mp_setup_ui_init_ror(u, &ui, &area, &mp) == 1);
    assert(strcmp(aoe_ui_text_line_ror(u->title, 0), "S9679") == 0);
    assert(u->settings_label == NULL);
    assert(u->settings && u->start && u->cancel && u->help && u->hidden);
    assert(u->count && u->count_label);
    assert(u->chat_input == NULL && u->chat == NULL);
    assert(u->ready == NULL && u->addresses == NULL);
    /* The kind list: 9683 -> 4, then 9685 / 9686 -> 0; no 9684 -> 1. */
    assert(aoe_ui_dropdown_id_ror(u->kind_lists[0], 0) == 4);
    assert(aoe_ui_dropdown_index_of_id_ror(u->kind_lists[0], 1) == -1);
    /* The civilisation list: 10231..10242 -> 1..12, 10246 -> 13, ...,
       10250 -> 17. */
    assert(aoe_ui_dropdown_id_ror(u->civ_lists[7], 12) == 13);
    assert(aoe_ui_dropdown_index_of_id_ror(u->civ_lists[7], 17) == 16);
    assert(u->cancel->hotkey == 0x1b);
    assert(u->ez.panel.current == &u->start->panel);
    assert(u->state.panel_700 == (void *)u->start);
    assert(u->state.panel_70c == (void *)u->cancel);
    assert(u->state.panel_620 == (void *)u->settings);
    assert(u->state.kind_lists[3] == (void *)u->kind_lists[3]);
    assert(chat_drops == 1);
    assert(recipients == 0x1feu); /* players 1..8 */
    assert(chats == 1);
    assert(memcmp(chat_text, "\x03\x02\x01", 4) == 0);
    assert(readies == 0);
    assert(comm_slots > 0);
    /* The players refresh filled the real count list (0x0048d1d0): "2".. with
       the settings' +0x988 selected. */
    assert(aoe_ui_dropdown_id_ror(u->count, 0) == 2 &&
           aoe_ui_dropdown_selected_id_ror(u->count) == (int32_t)aoe_settings_byte_988_ror(&settings)); /* the players refresh 0x0048cfb0 */
    assert(seeds == 0);
    u->ez.panel.vt->destroy(&u->ez.panel);
    free(u);

    /* 2. Multiplayer host. */
    reset_counts();
    settings.flag_97d = 0;
    settings.flag_97e = 1;
    host_ret = 1;
    u = calloc(1, sizeof(*u));
    assert(aoe_mp_setup_ui_init_ror(u, &ui, &area, &mp) == 1);
    assert(strcmp(aoe_ui_text_line_ror(u->title, 0), "S9678") == 0);
    assert(seeds == 1); /* the multiplayer settings reset */
    assert(u->chat_input && u->chat && u->chat_bar && u->chat_label);
    assert(u->settings && u->ready && u->ready_label && u->start && u->addresses);
    assert(u->settings_label == NULL);
    assert(u->count == NULL);
    assert(u->start->disabled == 1);
    /* The kind list gains 9684 -> 1 at index 1; 9685 -> 0 follows. */
    assert(aoe_ui_dropdown_index_of_id_ror(u->kind_lists[0], 1) == 1);
    assert(aoe_ui_dropdown_id_ror(u->kind_lists[0], 2) == 0);
    assert(readies == 1);
    for (i = 0; i < 8; ++i)
        assert(ready_args[i] == 0);
    assert(u->ez.panel.current == &u->chat_input->panel);
    assert(u->hidden->panel.active == 1);
    assert(u->cancel->hotkey == 0x1b);
    u->ez.panel.vt->destroy(&u->ez.panel);
    free(u);

    /* 3. Multiplayer guest. */
    reset_counts();
    settings.flag_97d = 0;
    settings.flag_97e = 1;
    host_ret = 0;
    u = calloc(1, sizeof(*u));
    assert(aoe_mp_setup_ui_init_ror(u, &ui, &area, &mp) == 1);
    assert(u->settings == NULL);
    assert(u->settings_label != NULL);
    assert(u->start == NULL);
    assert(u->addresses == NULL);
    assert(u->ready && u->cancel);
    assert(u->chat_input != NULL);
    assert(u->ez.panel.current == &u->chat_input->panel);
    assert(readies == 0);
    u->ez.panel.vt->destroy(&u->ez.panel);
    free(u);

    puts("mp setup constructor ok");
    return 0;
}
