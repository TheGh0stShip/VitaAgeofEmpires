#include "game_screen.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int active[64];
    char text[64][64];
    int help1[64];
    int msg_style[64], msg_color[64];
    char msg[64][64];
    int hidden[64];
    int redraws[64];
    int used[64], disabled[64], id[64], value[64], fields[64][3];
    int timer_mode[16], timer_index[16];
    int comm, last_action, last_arg, alt, place;
    int ncount;
    float age;
    int afford;
    int16_t sel_count;
    int32_t sel[25], current;
    AoeGameScreenObjectRor objs[8];
    int32_t list[6];
    int list_n;
    int alive[8], stance[8][8];
    float ptimer[8][2];
    int years[8];
    int16_t players;
    int world_mode;
} Fake;

static Fake f;
static int ix(void *p) { return (int)(intptr_t)p; }
static void set_active(void *c, void *p, int on) { (void)c; f.active[ix(p)] = on; }
static void redraw(void *c, void *p, int a) { (void)c; (void)a; f.redraws[ix(p)]++; }
static void set_text_id(void *c, void *p, int32_t id) { (void)c; snprintf(f.text[ix(p)], 64, "#%d", (int)id); }
static void set_text(void *c, void *p, const char *t) { (void)c; snprintf(f.text[ix(p)], 64, "%s", t); }
static int active(void *c, void *p) { (void)c; return f.active[ix(p)]; }
static const char *text(void *c, void *p) { (void)c; return f.text[ix(p)]; }
static void help(void *c, void *p, int32_t a, int32_t b) { (void)c; (void)b; f.help1[ix(p)] = a; }
static void message(void *c, void *p, int32_t st, const char *t, uint8_t col) {
    (void)c; f.msg_style[ix(p)] = st; f.msg_color[ix(p)] = col; snprintf(f.msg[ix(p)], 64, "%s", t);
}
static void message_hide(void *c, void *p) { (void)c; f.hidden[ix(p)] = 1; }
static int32_t b_dis(void *c, void *b) { (void)c; return f.disabled[ix(b)]; }
static void b_set_dis(void *c, void *b, int32_t d) { (void)c; f.disabled[ix(b)] = d; }
static int32_t b_id(void *c, void *b) { (void)c; return f.id[ix(b)]; }
static int32_t b_val(void *c, void *b) { (void)c; return f.value[ix(b)]; }
static void b_none(void *c, void *b) { (void)c; (void)b; }
static void b_pic(void *c, void *b, void *p, int32_t fr) { (void)c; (void)b; (void)p; (void)fr; }
static void b_state(void *c, void *b, int32_t id, int32_t v) { (void)c; f.id[ix(b)] = id; f.value[ix(b)] = v; }
static void b_lines(void *c, void *b, const char *l1, const char *l2) { (void)c; (void)l2; snprintf(f.text[ix(b)], 64, "%s", l1); }
static void b_help_line(void *c, void *b, const char *t) { (void)c; (void)b; (void)t; }
static int32_t b_used(void *c, void *b) { (void)c; return f.used[ix(b)]; }
static void b_set_used(void *c, void *b, int32_t u) { (void)c; f.used[ix(b)] = u; }
static void b_fields(void *c, void *b, int32_t a, int32_t d, int32_t k) {
    (void)c; f.fields[ix(b)][0] = a; f.fields[ix(b)][1] = d; f.fields[ix(b)][2] = k;
}
static void timer_set(void *c, void *t, int32_t m, int32_t i) { (void)c; f.timer_mode[ix(t) - 40] = m; f.timer_index[ix(t) - 40] = i; }
static int32_t comm(void *c) { (void)c; return f.comm; }
static void action(void *c, int32_t a, int32_t b) { (void)c; f.last_action = a; f.last_arg = b; }
static void format(void *c, int32_t k, int32_t a, int32_t b, char *o, int32_t n) { (void)c; snprintf(o, (size_t)n, "%x:%d:%d", (int)k, (int)a, (int)b); }
static const char *string(void *c, int32_t id) {
    (void)c;
    if (id == 0x1037) return "Build";
    if (id == 0x104b) return "Move";
    return "str";
}
static void soft(void *c, int on) { (void)c; f.ncount = on; }
static void place(void *c, int16_t id) { (void)c; f.place = id; }
static int32_t alt(void *c) { (void)c; return f.alt; }
static void set_alt(void *c, int32_t on) { (void)c; f.alt = on; }
static const char *chat_line(void *c, int32_t i) { (void)c; return i == 2 ? "hello" : "x"; }
static void wonder(void *c, int32_t p, int32_t *y) { (void)c; *y = f.years[p]; }
static int32_t wmode(void *c) { (void)c; return f.world_mode; }
static int16_t pcount(void *c) { (void)c; return f.players; }
static int palive(void *c, int32_t p) { (void)c; return f.alive[p]; }
static float ptimer(void *c, int32_t p, int w) { (void)c; return f.ptimer[p][w]; }
static int stance(void *c, int32_t p, int32_t o) { (void)c; return f.stance[p][o]; }
static float age(void *c) { (void)c; return f.age; }
static int afford(void *c, int16_t id, int16_t *fail) { (void)c; (void)id; *fail = 2; return f.afford; }
static int tflag(void *c, int16_t id) { (void)c; return id == 83; }
static int16_t sel_count(void *c) { (void)c; return f.sel_count; }
static int32_t selected(void *c, int32_t s) { (void)c; return f.sel[s]; }
static int32_t current(void *c) { (void)c; return f.current; }
static void set_current(void *c, int32_t id) { (void)c; f.current = id; }
static int object(void *c, int32_t id, AoeGameScreenObjectRor *o) {
    (void)c;
    if (id < 1 || id > 7) return 0;
    *o = f.objs[id];
    return 1;
}
static void g_int(void *c, int32_t g) { (void)c; f.last_arg = g; }
static void g_none(void *c) { (void)c; f.ncount++; }
static void vpos(void *c, float x, float y) { (void)c; (void)x; (void)y; }
static void vtile(void *c, int32_t x, int32_t y) { (void)c; f.last_action = x; f.last_arg = y; }
static int sel_ids(void *c, int32_t **ids, int16_t *n) { (void)c; *ids = NULL; *n = 0; return f.sel_count > 0; }
static void free_ids(void *c, int32_t *ids) { (void)c; (void)ids; }
static void pcall(void *c, int slot, int32_t a) { (void)c; f.last_action = slot; f.last_arg = a; }
static int busy(void *c, int32_t id) { (void)c; return id == 3; }
static int32_t lcount(void *c) { (void)c; return f.list_n; }
static int32_t lid(void *c, int32_t i) { (void)c; return f.list[i]; }
static int whas(void *c, int32_t id) { (void)c; return id >= 1 && id <= 7; }
static int ready(void *c, int32_t id) { (void)c; return id != 4; }
static void base_mouse(void *c, int32_t a, int32_t b, int32_t d, int32_t e, int32_t g) {
    (void)c; (void)b; (void)d; (void)e; (void)g; f.last_action = a;
}
static void focus(void *c, int32_t v) { (void)c; f.last_arg = v; }

static int pc_which = -1, toggles, chats, state5, is_paused, is_net;
static uint32_t pc_value, clock_ms;
static char chat_text[64];
static void pcommand(void *c, int w, uint32_t v) { (void)c; pc_which = w; pc_value = v; }
static int32_t gstate(void *c) { (void)c; return state5 ? 5 : 1; }
static int gpaused(void *c) { (void)c; return is_paused; }
static int gnet(void *c) { (void)c; return is_net; }
static void gtoggle(void *c) { (void)c; ++toggles; }
static void gchat(void *c, const char *t) { (void)c; ++chats; snprintf(chat_text, sizeof chat_text, "%s", t); }
static uint32_t gnow(void *c) { (void)c; return clock_ms; }

static const AoeGameScreenOpsRor ops = {
    NULL, set_active, redraw, set_text_id, set_text, active, text, help, message, message_hide,
    b_dis, b_set_dis, b_id, b_val, b_none, b_none, b_pic, b_state, b_lines, b_help_line,
    b_used, b_set_used, b_fields, timer_set, comm, action, format, string, soft, place, alt,
    set_alt, chat_line, wonder, wmode, pcount, palive, ptimer, stance, age, afford, tflag,
    sel_count, selected, current, set_current, object, g_int, g_int, g_none, g_none, g_none,
    vpos, vtile, sel_ids, free_ids, pcall, busy, lcount, lid, whas, ready, base_mouse,
    pcommand, gstate, gpaused, gnet, gtoggle, gchat, gnow};

static void setup(AoeGameScreenRor *s) {
    int i;
    memset(&f, 0, sizeof f);
    memset(s, 0, sizeof *s);
    s->ops = &ops;
    s->view = (void *)1;
    s->resources = (void *)2;
    s->age_text = (void *)3;
    s->panel_584 = (void *)4;
    s->title = (void *)5;
    for (i = 0; i < 12; ++i) s->buttons[i] = (void *)(intptr_t)(10 + i);
    for (i = 0; i < 6; ++i) s->lines[i] = (void *)(intptr_t)(24 + i);
    for (i = 0; i < 8; ++i) s->chat[i] = (void *)(intptr_t)(30 + i);
    for (i = 0; i < 10; ++i) s->timers[i] = (void *)(intptr_t)(40 + i);
}

int main(void) {
    AoeGameScreenRor s;
    AoeGameScreenButtonRor b;
    int i;
    int32_t ra[5] = {0, 1, 5, 5, 9}, rb[5] = {0, 2, 5, 5, 9};

    setup(&s);
    s.chat_next = 7;
    aoe_game_screen_chat_line_ror(&s, 2);
    assert(strcmp(f.msg[37], "hello") == 0 && f.msg_style[37] == 3 && f.msg_color[37] == 0xff);
    assert(s.chat_next == 0);

    aoe_game_screen_show_584_ror(&s);
    assert(f.active[4] == 1 && strcmp(f.text[4], "#9001") == 0 && f.redraws[1] == 1);
    aoe_game_screen_hide_584_ror(&s);
    assert(f.active[4] == 0 && f.redraws[1] == 2);
    aoe_game_screen_hide_584_ror(&s);
    assert(f.redraws[1] == 2);

    f.age = 3.0f;
    aoe_game_screen_age_ror(&s);
    assert(s.age == 3 && f.help1[3] == 0x106b && strcmp(f.text[3], "65:3:0") == 0);

    aoe_game_screen_title_ror(&s, "abc");
    assert(strcmp(f.text[5], "abc") == 0);

    aoe_game_screen_mouse_ror(&s, 7, 0, 0, 0, 0);
    assert(f.hidden[24] && f.hidden[29] && f.last_action == 7);

    assert(aoe_game_screen_button_slot_ror(1) == 0 && aoe_game_screen_button_slot_ror(6) == 6);
    assert(aoe_game_screen_button_slot_ror(11) == 0 && aoe_game_screen_button_slot_ror(20) == 10);
    assert(aoe_game_screen_button_slot_ror(0) == 0 && aoe_game_screen_button_slot_ror(21) == 0);

    memset(&b, 0, sizeof b);
    b.index = 2;
    b.command = 3;
    b.value = 9;
    aoe_game_screen_set_button_ror(&s, &b);
    assert(f.used[12] == 1 && f.id[12] == 3 && f.fields[12][2] == 'B' && f.active[12] == 1);
    f.active[12] = 0;
    aoe_game_screen_set_button_ror(&s, &b);
    assert(f.active[12] == 0); /* unchanged: no re-show */
    b.command = 6;
    aoe_game_screen_set_button_ror(&s, &b);
    assert(f.fields[12][2] == 0x1b);
    b.command = 0x11;
    b.hotkey_id = 0;
    aoe_game_screen_set_button_ror(&s, &b);
    assert(f.fields[12][1] == 2 && f.fields[12][2] == 0);
    aoe_game_screen_hide_unused_ror(&s);
    assert(f.active[12] == 1 && f.active[10] == 0 && f.value[10] == -1 && f.help1[10] == -1);

    /* Timers: player 1 wonder; players 1 and 2 allied, so 2's timers are
       not shown; player 3 timer 1. */
    f.world_mode = 0;
    f.players = 4;
    f.years[1] = 100;
    for (i = 0; i < 4; ++i) { f.ptimer[i][0] = -1.0f; f.ptimer[i][1] = -1.0f; f.alive[i] = 1; }
    for (i = 0; i < 4; ++i) { int j; for (j = 0; j < 4; ++j) f.stance[i][j] = 3; }
    f.stance[1][2] = f.stance[2][1] = 0;
    f.ptimer[2][0] = 5.0f;
    f.ptimer[3][1] = 2.0f;
    aoe_game_screen_timers_ror(&s);
    assert(f.timer_mode[0] == 4 && f.timer_index[0] == 1);
    assert(f.timer_mode[1] == 6 && f.timer_index[1] == 3);
    assert(f.timer_mode[2] == 0 && f.timer_mode[9] == 0);
    f.world_mode = 2;
    aoe_game_screen_timers_ror(&s);
    assert(f.timer_mode[0] == 3);

    assert(aoe_game_screen_score_cmp_ror(ra, rb) == 1);
    assert(aoe_game_screen_score_cmp_ror(rb, ra) == -1);
    assert(aoe_game_screen_score_cmp_ror(ra, ra) == 0);

    setup(&s);
    aoe_game_screen_announce_ror(&s, 0xbe2, 0x67);
    assert(f.last_action == 0x67 && f.msg_color[24] == 0x32);
    f.comm = 1;
    f.last_action = 0;
    aoe_game_screen_announce_ror(&s, 0xbe2, 0x67);
    assert(f.last_action == 0);
    f.comm = 0;

    f.afford = 1;
    aoe_game_screen_train_ror(&s, 83);
    assert(f.place == 83 && f.last_action == 0x15);
    aoe_game_screen_train_ror(&s, 84);
    assert(f.last_action == 7);
    f.afford = 0;
    aoe_game_screen_train_ror(&s, 84);
    assert(strcmp(f.msg[24], "67:1:2") == 0 && f.msg_color[24] == 0x56);

    aoe_game_screen_alt_bar_ror(&s);
    assert(f.alt == 1 && f.active[5] == 1 && f.active[2] == 0);
    aoe_game_screen_alt_bar_ror(&s);
    assert(f.alt == 1);
    aoe_game_screen_alt_bar_ror(&s);
    assert(f.alt == 0 && f.active[2] == 1);

    aoe_game_screen_group_assign_ror(&s, 0);
    assert(f.last_arg == 10);

    /* Cycling the selection: 2, 5, 6 in slots 0, 3, 7. */
    f.sel_count = 3;
    f.sel[0] = 2; f.sel[3] = 5; f.sel[7] = 6;
    f.current = 5;
    aoe_game_screen_cycle_ror(&s, 1);
    assert(f.current == 6);
    aoe_game_screen_cycle_ror(&s, 1);
    assert(f.current == 2);
    aoe_game_screen_cycle_ror(&s, 0);
    assert(f.current == 6);
    aoe_game_screen_cycle_ror(&s, 0);
    assert(f.current == 5);

    f.objs[3].type = 0x30;
    f.objs[5].type = 0x30;
    assert(!aoe_game_screen_selection_busy_ror(&s));
    f.sel[3] = 3;
    assert(aoe_game_screen_selection_busy_ror(&s));

    f.objs[2].x = 3.7f; f.objs[2].y = 9.2f;
    f.current = 2;
    aoe_game_screen_center_ror(&s);
    assert(f.last_action == 3 && f.last_arg == 9);

    /* Next idle: list 1..5; 4 not ready, 5 has category 4. */
    f.list_n = 5;
    for (i = 0; i < 5; ++i) { f.list[i] = i + 1; f.objs[i + 1].category = 0; }
    f.objs[4].category = 4;
    f.objs[5].category = 4;
    f.objs[1].category = 2;
    assert(aoe_game_screen_next_idle_ror(&s, 3) == 5 && s.idle_id == 5);
    assert(aoe_game_screen_next_idle_ror(&s, 5) == 1);
    assert(aoe_game_screen_next_idle_ror(&s, 99) == 5);

    s.field_78 = 1;
    aoe_game_screen_focus_ror(&s, focus, 4);
    assert(f.last_arg == 4 && f.ncount == 0);
    puts("game_screen_test: ok");
    /* Delete, opcode 0x6d and the pause key. */
    f.comm = 1;
    aoe_game_screen_delete_ror(&s);
    assert(pc_which == -1);
    f.comm = 0;
    aoe_game_screen_delete_ror(&s);
    assert(pc_which == 0);
    aoe_game_screen_command_6d_ror(&s, 9);
    assert(pc_which == 1 && pc_value == 9);
    s.pause_time = 0;
    clock_ms = 5000;
    is_net = 1;
    aoe_game_screen_pause_ror(&s);
    assert(toggles == 1 && chats == 1 && !strcmp(chat_text, "str") && s.pause_time == 5000);
    clock_ms = 5500;
    aoe_game_screen_pause_ror(&s);
    assert(toggles == 1);
    is_paused = 1;
    aoe_game_screen_pause_ror(&s);
    assert(toggles == 2 && chats == 2);
    state5 = 1;
    clock_ms = 9000;
    aoe_game_screen_pause_ror(&s);
    assert(toggles == 2 && s.pause_time == 9000);
    return 0;
}
