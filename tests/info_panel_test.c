/* Patched RoR 1.0a game-screen object panel (0x004f83d0..0x004fa150). */
#include "info_panel.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { void *shape; int32_t frame, x, y, remap; } Draw;
static Draw draws[16];
static int ndraws;
typedef struct { int32_t x, y; uint32_t color; char text[64]; } Out;
static Out outs[32];
static int nouts;
static char rect_text[32];
static AoeUiRectRor rect_rect;

static char health, items, units, bld0, bld1, loaded;
static void *bld[2] = {&bld0, &bld1};
static int freed;

static uint32_t now;
static uint32_t clock_ms(void *c) { (void)c; return now; }
static int tb(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r, uintptr_t f) {
    (void)c; (void)a; (void)r; (void)f;
    return 1;
}
static void to(void *c, int32_t x, int32_t y, const char *t, int32_t n, uint32_t col) {
    (void)c;
    if (nouts < 32) {
        outs[nouts].x = x, outs[nouts].y = y, outs[nouts].color = col;
        snprintf(outs[nouts].text, sizeof outs[nouts].text, "%.*s", (int)n, t);
    }
    ++nouts;
}
static void te(void *c) { (void)c; }
static void trect(void *c, const char *t, const AoeUiRectRor *r, uint32_t f, uint32_t col) {
    (void)c; (void)col;
    assert(f == 0x22);
    snprintf(rect_text, sizeof rect_text, "%s", t);
    rect_rect = *r;
}

static AoeInfoObjectRor obj;
static char object_token, player_token, other_owner;
static float attrs[40];
static void *selected(void *c, const void *p) { (void)c; return p == &player_token ? &object_token : NULL; }
static int read(void *c, void *o, AoeInfoObjectRor *out) {
    (void)c;
    assert(o == &object_token);
    *out = obj;
    return 1;
}
static float attribute(void *c, const void *p, int i) { (void)c; assert(p == &player_token); return attrs[i]; }
static uint8_t civ(void *c, const void *p) { (void)c; (void)p; return 0xf; }
static int nearest(void *c, const void *p, int16_t cls, float x, float y, uint8_t st,
                   void *u, float *ox, float *oy) {
    (void)c; (void)p; (void)x; (void)y; (void)u;
    assert(cls == 0x2d && st == 2);
    *ox = obj.x + 30.0f;
    *oy = obj.y + 40.0f;
    return 1;
}
static void civ_name(void *c, uint8_t civ_id, char *out, int n) { (void)c; snprintf(out, (size_t)n, "Civ%d", civ_id); }
static void string(void *c, int32_t id, char *out, int n) { (void)c; snprintf(out, (size_t)n, id == 300 ? "Granary" : ""); }
static int32_t slot_player(void *c, int32_t s) { (void)c; return s <= 2 ? s : 0; }
static int32_t player_count(void *c) { (void)c; return 3; }
static int16_t player_color(void *c, int32_t p) { (void)c; return (int16_t)(p - 1); }
static int32_t scores[9] = {0, 120, 75};
static int32_t player_score(void *c, int32_t p) { (void)c; return scores[p]; }
static const char *slot_name(void *c, int32_t s) { (void)c; return s == 1 ? "Ann" : "Bo"; }
static void draw_shape(void *c, AoeUiAreaRor *a, void *s, int32_t f, int32_t x, int32_t y, int32_t remap) {
    (void)c; (void)a;
    if (ndraws < 16) draws[ndraws] = (Draw){s, f, x, y, remap};
    ++ndraws;
}
static int32_t shape_frames(void *c, void *s) { (void)c; assert(s == &health); return 20; }
static void *load_shape(void *c, const char *f, int32_t id) {
    (void)c;
    ++loaded;
    if (strcmp(f, "health.shp") == 0 && id == 0xc639) return &health;
    if (strcmp(f, "itemicon.shp") == 0 && id == 0xc62b) return &items;
    return NULL;
}
static void free_shape(void *c, void *s) { (void)c; (void)s; ++freed; }

static AoeInfoTemplateRor tmpl;
static char tmpl_token, sprite_token;
static int sprite_x, sprite_y, sprite_draws;
static int read_tmpl(void *c, const void *t, AoeInfoTemplateRor *out) {
    (void)c;
    assert(t == &tmpl_token);
    *out = tmpl;
    return 1;
}
static int16_t pnum(void *c, const void *pl) { (void)c; (void)pl; return 3; }
static int pcolour(void *c, const void *pl, int16_t *ix) { (void)c; (void)pl; *ix = 2; return 1; }
static int sbounds(void *c, void *s, int32_t *mx, int32_t *my) {
    (void)c;
    assert(s == &sprite_token);
    *mx = -20, *my = -40;
    return 1;
}
static void dsprite(void *c, AoeUiAreaRor *a, void *s, int32_t x, int32_t y, const void *pl) {
    (void)c; (void)a; (void)s; (void)pl;
    sprite_x = x, sprite_y = y;
    ++sprite_draws;
}
static void unknown_name(void *c, uint8_t civ_id, char *out, int n) {
    (void)c;
    snprintf(out, (size_t)n, civ_id == 3 ? "Unknown" : "P%d", civ_id);
}

static int actions;
static int32_t parent_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t a, int32_t b) {
    (void)p; (void)from; (void)a; (void)b;
    if (code == 1) ++actions;
    return 1;
}

static const Out *find(const char *text) {
    for (int k = 0; k < nouts && k < 32; ++k)
        if (strcmp(outs[k].text, text) == 0) return &outs[k];
    return NULL;
}

int main(void) {
    static AoeUiSystemRor sys;
    AoeUiPlatformRor pf = {0};
    AoeInfoPanelOpsRor ops = {0};
    AoeUiAreaRor area = {(void *)1, 640, 480};
    static AoeUiPanelRor parent;
    static AoeUiPanelVtblRor parent_vt;
    static AoeInfoPanelRor ip;
    AoeUiPanelRor *p = &ip.panel;
    const Out *o;
    pf.now_ms = clock_ms;
    pf.text_begin = tb;
    pf.text_out = to;
    pf.text_end = te;
    pf.text_rect = trect;
    sys.platform = &pf;
    ops.selected = selected;
    ops.read = read;
    ops.attribute = attribute;
    ops.civilization = civ;
    ops.nearest = nearest;
    ops.civ_name = civ_name;
    ops.string = string;
    ops.slot_player = slot_player;
    ops.player_count = player_count;
    ops.player_color = player_color;
    ops.player_score = player_score;
    ops.slot_name = slot_name;
    ops.draw_shape = draw_shape;
    ops.shape_frames = shape_frames;
    ops.load_shape = load_shape;
    ops.free_shape = free_shape;
    aoe_ui_panel_init_ror(&parent, &sys);
    aoe_ui_panel_setup_ror(&parent, &area, NULL, 0, 0, 640, 480, 0);
    parent_vt = *parent.vt;
    parent_vt.action = parent_action;
    parent.vt = &parent_vt;

    /* An own granary under construction. */
    obj.owner = &player_token;
    obj.id = 77;
    obj.template_id = &units;
    obj.type = 0x50;
    obj.object_class = 0x32;
    obj.name_id = 300;
    obj.name = "granary";
    obj.max_hp = 200;
    obj.hp = 50.0f;
    obj.icon = 4;
    obj.panel_kind = 2;
    obj.building_icon_base = 10;
    obj.icon_set = 1;
    obj.object_state = 0;
    obj.resource_type = -1;
    obj.build_percent = 25;
    obj.owner_has_color = 1;
    obj.owner_color = 2;
    obj.owner_number = 1;
    obj.owner_civ = 5;
    assert(aoe_info_panel_init_ror(&ip, &sys, &ops, &area, &parent, 3, 8, 12, &units,
                                   (void *const *)bld, &player_token));
    assert(ip.object == &object_token && ip.health == &health && ip.item_icons == &items);
    assert(ip.mode == 1 && ip.interval_ms == 500 && ip.snap_percent == 25.0f &&
           ip.snap_state == 0 && ip.snap_owner == &player_token);
    p->vt->set_fixed_position(p, 100, 200, 180, 120);
    p->vt->draw(p);
    /* The building picture (set 1, icon + base) in the owner's colours; the
       health bar at 20 - 20 * 50 / 200 = 15. */
    assert(draws[0].shape == &bld1 && draws[0].frame == 14 && draws[0].x == 103 &&
           draws[0].y == 230 && draws[0].remap == 0x30);
    assert(draws[1].shape == &health && draws[1].frame == 15 && draws[1].y == 200 + 0x54);
    assert((o = find("Civ5")) && o->x == 103 && o->y == 198);
    assert((o = find("Granary")) && o->y == 200 + 0xd);
    assert(find("50/200"));
    /* Row 0: the build percent with item icon 9. */
    assert(draws[2].shape == &items && draws[2].frame == 9 && draws[2].x == 100 + 0x36 &&
           draws[2].y == 200 + 0x1e);
    assert((o = find("25%")) && o->x == 100 + 0x58 && o->y == 200 + 0x21);

    /* Idle: nothing changed, nothing happens; the percent moving redraws;
       over 99 percent also tells the parent. */
    now = 1000;
    p->redraw = 0;
    p->vt->handle_idle(p);
    assert(!p->redraw && actions == 0 && ip.last_ms == 1000);
    obj.build_percent = 60;
    now = 1200;
    p->vt->handle_idle(p);
    assert(!p->redraw);
    now = 1500;
    p->vt->handle_idle(p);
    assert(p->redraw && actions == 0 && ip.snap_percent == 60.0f);
    p->redraw = 0;
    obj.build_percent = 100;
    now = 2000;
    p->vt->handle_idle(p);
    assert(p->redraw && actions == 1);

    /* Finished and training: the progress row; a new template notifies. */
    obj.object_state = 2;
    obj.build_percent = 100;
    obj.action_type = 0x66;
    obj.action_progress = 40;
    now = 2500;
    p->redraw = 0;
    p->vt->handle_idle(p);
    assert(p->redraw && actions == 2);
    nouts = ndraws = 0;
    p->vt->draw(p);
    assert(find("40%"));
    obj.template_id = &items;
    now = 3000;
    p->vt->handle_idle(p);
    assert(actions == 3 && ip.snap_template == &items);

    /* A soldier (kind 4): armour 2+1, attack 5, range 7; the second
       snapshot attack slot is compared with pierce armour, as the
       original, so pierce 3 redraws every half second. */
    memset(&obj, 0, sizeof obj);
    obj.owner = &other_owner;
    obj.template_id = &units;
    obj.type = 0x46;
    obj.icon = 2;
    obj.panel_kind = 4;
    obj.hp = 30.0f;
    obj.max_hp = 30;
    obj.armor = 3;
    obj.armor_base = 2;
    obj.attack = 5;
    obj.attack_base = 5;
    obj.pierce = 0;
    obj.range = 7.0f;
    obj.range_base = 7.0f;
    obj.resource_type = -1;
    obj.name = "Bowman";
    aoe_info_panel_set_object_ror(&ip, &object_token);
    assert(ip.snap_attack == 5 && ip.snap_152 == 5 && ip.snap_armor == 3);
    nouts = ndraws = 0;
    p->vt->draw(p);
    assert(draws[0].shape == &units && draws[0].frame == 2 && draws[0].remap == -1);
    assert((o = find("2+1")) && o->y == 200 + 0x21);
    assert((o = find("5")) && o->y == 200 + 0x21 + 0x14);
    assert((o = find("7")) && o->y == 200 + 0x21 + 0x28);
    assert(find("Bowman") && !find("Civ0"));
    actions = 0;
    p->redraw = 0;
    now = 4000;
    p->vt->handle_idle(p);
    assert(p->redraw && actions == 0); /* 5 (attack) != 0 (pierce) */
    obj.pierce = 5;
    p->redraw = 0;
    now = 4500;
    p->vt->handle_idle(p);
    assert(!p->redraw);

    /* Item formats. */
    nouts = ndraws = 0;
    aoe_info_panel_item_ror(&ip, 1, -1, 1, 4, 4);
    aoe_info_panel_item_ror(&ip, 1, -1, 2, 3, 9);
    aoe_info_panel_item_ror(&ip, 1, 5, 4, 1, 1);
    assert(nouts == 2 && strcmp(outs[0].text, "4") == 0 && strcmp(outs[1].text, "3/9") == 0);
    assert(ndraws == 1 && draws[0].frame == 5);

    /* Another player's dock: the trade profit from our nearest dock.
       (900 + 1600) * 0.000251256 * 0.7 = 0.44, doubled for civ 15 = 0.88,
       times 20 = 17.6 -> 18 of 20. Its stock is its owner's attribute 9. */
    memset(&obj, 0, sizeof obj);
    obj.owner = &other_owner;
    obj.template_id = &units;
    obj.type = 0x50;
    obj.object_class = 0x2d;
    obj.panel_kind = 2;
    obj.icon = -1;
    obj.object_state = 2;
    obj.resource_type = 3;
    obj.owner_attribute_9 = 41.6f;
    obj.x = 10.0f;
    obj.y = 10.0f;
    aoe_info_panel_set_object_ror(&ip, &object_token);
    nouts = ndraws = 0;
    p->vt->draw(p);
    assert((o = find("42")) && o->y == 200 + 0x21);
    assert((o = find("18/20")) && o->y == 200 + 0x21 + 0x14);

    /* Score mode. */
    ip.mode = 2;
    nouts = 0;
    p->vt->draw(p);
    assert(nouts == 2 && strcmp(outs[0].text, "Ann") == 0 && outs[0].color == 0xff0000 &&
           outs[0].x == 103 && outs[0].y == 199);
    assert(strcmp(outs[1].text, "Bo") == 0 && outs[1].color == 0x2139c6 && outs[1].y == 213);
    assert(strcmp(rect_text, "75") == 0 && rect_rect.left == 100 + 0x5a &&
           rect_rect.right == 100 + 180 - 3 && rect_rect.top == 213 && rect_rect.bottom == 227);
    aoe_info_panel_snapshot_ror(&ip);
    assert(ip.scores[1] == 120 && ip.scores[2] == 75);
    p->redraw = 0;
    now = 6000;
    p->vt->handle_idle(p);
    assert(!p->redraw);
    scores[2] = 80;
    now = 7000;
    p->vt->handle_idle(p);
    assert(p->redraw && ip.scores[2] == 80);

    /* The selection going away clears the panel. */
    ip.mode = 1;
    aoe_info_panel_set_player_ror(&ip, NULL);
    assert(ip.object == NULL);

    /* The scenario editor's template panel (0x004a4350 / 0x004a49a0). */
    {
        static AoeInfoEditorPanelRor ed;
        ops.read_template = read_tmpl;
        ops.player_number = pnum;
        ops.player_colour = pcolour;
        ops.sprite_bounds = sbounds;
        ops.draw_sprite = dsprite;
        ops.civ_name = unknown_name;
        ed.base = ip;
        ed.base.mode = 1;
        ed.base.player = &player_token;
        ed.base.panel.visible = ed.base.panel.active = 1;
        ed.tmpl = &tmpl_token;
        ed.icon_set = 1;

        /* Rows sit 18 pixels apart with the editor's offsets. */
        ndraws = nouts = 0;
        aoe_info_editor_item_ror(&ed, 2, 5, 1, 4, 6);
        assert(draws[0].shape == &items && draws[0].frame == 5 && draws[0].x == 100 + 0x3c &&
               draws[0].y == 200 + 0x1e + 36);
        assert((o = find("4+2")) && o->x == 200 && o->y == 200 + 0x20 + 36);

        /* Kind 2: building icon in the player's colours, "Unknown" shown
           as Gaia, hit points max/max, then three rows. */
        memset(&tmpl, 0, sizeof tmpl);
        tmpl.icon = 4;
        tmpl.panel_kind = 2;
        tmpl.building_icon_base = 10;
        tmpl.name = "Wall";
        tmpl.max_hp = 300;
        tmpl.displayed_13e = 7;
        tmpl.displayed_13c = 2;
        tmpl.range_114 = 5.9f;
        ndraws = nouts = 0;
        aoe_info_editor_draw_ror(&ed);
        assert(draws[0].shape == &bld1 && draws[0].frame == 14 && draws[0].x == 102 &&
               draws[0].y == 200 + 0x21 && draws[0].remap == 0x30);
        assert((o = find("Gaia")) && o->x == 102 && o->y == 200);
        assert((o = find("Wall")) && o->y == 200 + 0xf);
        assert((o = find("300/300")) && o->x == 105 && o->y == 200 + 0x55);
        assert(find("7") && find("2") && find("5") && ndraws == 4);
        assert(draws[3].frame == 6 && draws[3].y == 200 + 0x1e + 36);

        /* Kind 4: the range row carries the pierce armor as its maximum. */
        tmpl.panel_kind = 4;
        tmpl.pierce = 9;
        tmpl.range_140 = 7.0f;
        ndraws = nouts = 0;
        aoe_info_editor_draw_ror(&ed);
        assert(ndraws == 5 && draws[1].frame == 7 && draws[2].frame == 8 &&
               draws[3].frame == 10 && draws[4].frame == 6);
        assert(find("7+2") && find("9"));

        /* Kind 1: the first resource with a positive amount. */
        tmpl.panel_kind = 1;
        tmpl.resource_type[0] = 1, tmpl.resource_amount[0] = 0.0f;
        tmpl.resource_type[1] = 0x10, tmpl.resource_amount[1] = 150.7f;
        ndraws = nouts = 0;
        aoe_info_editor_draw_ror(&ed);
        assert(ndraws == 2 && draws[1].frame == 2 && find("150"));

        /* No icon: the sprite placed by its bounds, no hit points or rows. */
        tmpl.icon = -1;
        tmpl.sprite = &sprite_token;
        ndraws = nouts = 0;
        aoe_info_editor_draw_ror(&ed);
        assert(sprite_draws == 1 && sprite_x == 100 + 20 + 10 && sprite_y == 200 + 40 + 0x28);
        assert(ndraws == 0 && !find("300/300") && find("Wall"));
    }

    p->vt->destroy(p);
    assert(freed == 2);
    puts("info panel ok");
    return 0;
}
