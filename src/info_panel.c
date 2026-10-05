#include "info_panel.h"

#include <stdio.h>
#include <string.h>

#include "draw_system.h"

#define IP(p) ((AoeInfoPanelRor *)(p))

static uint32_t now_ms(const AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

/* The original's __ftol: truncation toward zero. */
static int32_t ftol(float v) { return (int32_t)v; }

static int read_object(AoeInfoPanelRor *i, AoeInfoObjectRor *o) {
    memset(o, 0, sizeof *o);
    return i->object && i->ops && i->ops->read &&
           i->ops->read(i->ops->context, i->object, o);
}

static float attribute(const AoeInfoPanelRor *i, int index) {
    return i->player && i->ops && i->ops->attribute
               ? i->ops->attribute(i->ops->context, i->player, index)
               : 0.0f;
}

/* The production query of the draw, idle and snapshot: the head action,
   or with none training or researching, the first queued unit as
   training. */
static void production(const AoeInfoObjectRor *o, int16_t *type, int16_t *id,
                       int16_t *progress) {
    *type = o->action_type;
    *id = o->action_id;
    *progress = o->action_progress;
    if (*type != 0x67 && *type != 0x66 && o->queued) {
        *id = o->queued_id;
        *type = 0x66;
        *progress = o->queued_count;
    }
}

/* The resource picture of a resource type (food kinds 0, 15..17 2, wood
   0, stone 1, gold 3, 9 5), -1 for the others. */
static int32_t resource_icon(int16_t type) {
    switch (type) {
    case 0: case 0xf: case 0x10: case 0x11: return 2;
    case 1: return 0;
    case 2: return 1;
    case 3: return 3;
    case 9: return 5;
    default: return -1;
    }
}

void aoe_info_panel_snapshot_ror(AoeInfoPanelRor *i) {
    AoeInfoObjectRor o;
    int16_t id;
    if (i->mode == 2) {
        int32_t s;
        for (s = 1; s < 9; ++s) {
            int32_t p = i->ops->slot_player ? i->ops->slot_player(i->ops->context, s) : 0;
            if (p != 0 && i->ops->player_count &&
                p < i->ops->player_count(i->ops->context) && p < 9)
                i->scores[p] = i->ops->player_score
                                   ? i->ops->player_score(i->ops->context, p)
                                   : 0;
        }
        return;
    }
    if (!read_object(i, &o)) return;
    i->snap_37 = o.field_37;
    i->snap_template = o.template_id;
    i->snap_resource = o.resource_type;
    i->snap_amount = o.amount;
    i->snap_hp = o.hp;
    i->snap_contents = o.contents_first;
    i->snap_count = o.contents_count;
    i->snap_template_28 = o.template_28;
    i->snap_owner = o.owner;
    if (o.type >= 0x46 && o.type != 0x5a) {
        i->snap_has_action = o.has_action;
        /* As the original: +0x152 is filled from +0x230 again, though the
           idle routine compares it with +0x248. */
        i->snap_attack = o.attack;
        i->snap_152 = o.attack;
        i->snap_armor = o.armor;
        i->snap_range = o.range;
        i->snap_23c = o.stat_23c;
    }
    if (o.type != 0x50) return;
    i->snap_state = o.object_state;
    i->snap_percent = (float)o.build_percent;
    i->snap_action = o.action_type;
    i->snap_action_id = o.action_id;
    i->snap_progress = o.action_progress;
    memcpy(i->snap_text, o.action_text, sizeof i->snap_text);
    i->snap_text[sizeof i->snap_text - 1] = '\0';
    if (i->snap_action != 0x67 && i->snap_action != 0x66 && o.queued) {
        id = o.queued_id;
        i->snap_action_id = id;
        i->snap_action = 0x66;
        i->snap_progress = o.queued_count;
    }
    i->snap_pop = ftol(attribute(i, 11));
    i->snap_pop_cap = ftol(attribute(i, 4));
    if (o.owner == i->player && o.object_class == 0x32 && o.has_head_action)
        i->snap_head = ftol(o.head_action_2c + o.amount + 0.5f);
}

void aoe_info_panel_set_object_ror(AoeInfoPanelRor *i, void *object) {
    i->object = object;
    aoe_info_panel_snapshot_ror(i);
    i->panel.vt->set_redraw(&i->panel, 1);
}

void aoe_info_panel_set_player_ror(AoeInfoPanelRor *i, const void *player) {
    i->player = player;
    aoe_info_panel_set_object_ror(
        i, player && i->ops && i->ops->selected
               ? i->ops->selected(i->ops->context, player)
               : NULL);
}

static void free_shape(AoeInfoPanelRor *i, void **s) {
    if (*s && i->ops && i->ops->free_shape) i->ops->free_shape(i->ops->context, *s);
    *s = NULL;
}

int aoe_info_panel_init_ror(AoeInfoPanelRor *i, AoeUiSystemRor *system,
                            const AoeInfoPanelOpsRor *ops, AoeUiAreaRor *area,
                            AoeUiPanelRor *parent, uintptr_t font,
                            int32_t font_w, int32_t font_h, void *unit_icons,
                            void *const *building_icons, const void *player) {
    aoe_ui_panel_init_ror(&i->panel, system);
    i->panel.vt = aoe_info_panel_vtable_ror();
    i->ops = ops;
    i->font = font;
    i->font_w = font_w;
    i->font_h = font_h;
    i->health = NULL;
    i->item_icons = NULL;
    i->unit_icons = unit_icons;
    i->building_icons = building_icons;
    i->player = NULL;
    i->object = NULL;
    i->last_ms = 0;
    i->interval_ms = 500;
    i->mode = 1;
    i->snap_template = NULL;
    i->snap_resource = -1;
    i->snap_amount = 0;
    i->snap_state = 0;
    i->snap_percent = 0;
    i->snap_contents = NULL;
    i->snap_hp = 0;
    i->snap_attack = i->snap_152 = i->snap_armor = 0;
    i->snap_range = i->snap_23c = 0;
    i->snap_action = i->snap_action_id = i->snap_progress = 0;
    i->snap_text[0] = '\0';
    i->snap_37 = 0;
    i->snap_owner = NULL;
    i->snap_has_action = 0;
    i->snap_head = 0;
    memset(i->scores, 0, sizeof i->scores);
    if (!aoe_ui_panel_setup_ror(&i->panel, area, parent, 0, 0, 0, 0, 0)) {
        i->panel.field_d8 = 1;
        return 0;
    }
    aoe_info_panel_set_player_ror(i, player);
    if (ops && ops->load_shape) {
        i->health = ops->load_shape(ops->context, "health.shp", 0xc639);
        i->item_icons = ops->load_shape(ops->context, "itemicon.shp", 0xc62b);
    }
    return 1;
}

static void destroy(AoeUiPanelRor *p) {
    AoeInfoPanelRor *i = IP(p);
    free_shape(i, &i->health);
    free_shape(i, &i->item_icons);
    aoe_ui_panel_release_ror(p);
}

static void shape(AoeInfoPanelRor *i, void *s, int32_t frame, int32_t x,
                  int32_t y, int32_t remap) {
    if (s && i->ops && i->ops->draw_shape)
        i->ops->draw_shape(i->ops->context, i->panel.area, s, frame, x, y, remap);
}

static int32_t frames(AoeInfoPanelRor *i, void *s) {
    return i->ops && i->ops->shape_frames ? i->ops->shape_frames(i->ops->context, s) : 0;
}

static const AoeUiPlatformRor *platform(const AoeInfoPanelRor *i) {
    return i->panel.system ? i->panel.system->platform : NULL;
}

void aoe_info_panel_item_ror(AoeInfoPanelRor *i, int32_t row, int32_t icon,
                             int32_t kind, int32_t a, int32_t b) {
    AoeUiPanelRor *p = &i->panel;
    const AoeUiPlatformRor *pf = platform(i);
    char text[32];
    if (i->item_icons && icon != -1)
        shape(i, i->item_icons, icon, p->x + 0x36, p->y + 0x1e + row * 0x14, -1);
    switch (kind) {
    case 0: snprintf(text, sizeof text, "%d", a); break;
    case 1:
        if (b <= a) snprintf(text, sizeof text, "%d", a);
        else snprintf(text, sizeof text, "%d+%d", a, b - a);
        break;
    case 2: snprintf(text, sizeof text, "%d/%d", a, b); break;
    case 3: snprintf(text, sizeof text, "%d%%", a); break;
    default: return;
    }
    if (text[0] && pf && pf->text_begin &&
        pf->text_begin(pf->context, p->area, &p->clip, i->font)) {
        pf->text_out(pf->context, p->x + 0x58, p->y + 0x21 + row * 0x14, text,
                     (int32_t)strlen(text), 0xffffff);
        pf->text_end(pf->context);
    }
}

static uint32_t score_color(int16_t index) {
    switch (index) {
    case 0: return 0xff0000;
    case 1: return 0x2139c6;
    case 2: return 0xffff;
    case 3: return 0x4f73a3;
    case 4: return 0xf77f3;
    case 5: return 0xc700;
    case 6: return 0x7b90a2;
    case 7: return 0xadad00;
    default: return 0xffffff;
    }
}

static void fill_black(AoeUiPanelRor *p) {
    struct AoeDrawSurfaceRor *s = p->area ? p->area->surface : NULL;
    if (s) {
        int32_t r[4] = {p->clip.left, p->clip.top, p->clip.right, p->clip.bottom};
        aoe_surface_fill_ror(s, r, 0);
    }
}

void aoe_info_panel_draw_scores_ror(AoeInfoPanelRor *i) {
    AoeUiPanelRor *p = &i->panel;
    const AoeUiPlatformRor *pf = platform(i);
    const AoeInfoPanelOpsRor *ops = i->ops;
    int32_t slot, dy = 0;
    if (!p->area || !p->visible || !p->active) {
        p->redraw = 0;
        return;
    }
    p->vt->begin_draw(p, 0);
    fill_black(p);
    if (pf && pf->text_begin && pf->text_begin(pf->context, p->area, &p->clip, i->font)) {
        int16_t x = (int16_t)(p->x + 3);
        int32_t top = (int16_t)((int16_t)p->y - 1);
        for (slot = 1; slot < 9; ++slot) {
            int32_t pl = ops && ops->slot_player ? ops->slot_player(ops->context, slot) : 0;
            const char *name;
            char text[100];
            AoeUiRectRor r;
            uint32_t color;
            if (pl == 0 || !ops->player_count || pl >= ops->player_count(ops->context))
                continue;
            color = score_color(ops->player_color ? ops->player_color(ops->context, pl) : -1);
            name = ops->slot_name ? ops->slot_name(ops->context, slot) : NULL;
            snprintf(text, sizeof text, "%s", name ? name : "");
            pf->text_out(pf->context, x, top + dy, text, (int32_t)strlen(text), color);
            snprintf(text, sizeof text, "%d",
                     ops->player_score ? ops->player_score(ops->context, pl) : 0);
            r.left = (int16_t)(p->x + 0x5a);
            r.top = top + dy;
            r.right = p->width - 3 + p->x;
            r.bottom = top + 0xe + dy;
            if (pf->text_rect) pf->text_rect(pf->context, text, &r, 0x22, color);
            dy += 0xe;
        }
        pf->text_end(pf->context);
    }
    p->vt->end_draw(p);
}

/* The picture, health bar, owner, name and hit points. */
static void draw_header(AoeInfoPanelRor *i, const AoeInfoObjectRor *o) {
    AoeUiPanelRor *p = &i->panel;
    const AoeUiPlatformRor *pf = platform(i);
    const AoeInfoPanelOpsRor *ops = i->ops;
    char a[120], b[100];
    if (o->icon != -1) {
        int16_t icon = o->icon;
        void *pic = i->unit_icons;
        if (o->panel_kind == 2) {
            icon = (int16_t)(icon + o->building_icon_base);
            pic = i->building_icons ? i->building_icons[o->icon_set] : NULL;
        }
        shape(i, pic, icon, p->x + 3, p->y + 0x1e,
              o->owner_has_color ? (o->owner_color + 1) * 0x10 : -1);
    }
    if (o->hp > 0.0f && i->health) {
        int32_t n = frames(i, i->health);
        int16_t f = (int16_t)ftol((float)n - (float)n * o->hp / (float)o->max_hp);
        if (f < 0) f = 0;
        else if (f > frames(i, i->health) - 1) f = (int16_t)(frames(i, i->health) - 1);
        shape(i, i->health, f, p->x + 3, p->y + 0x54, -1);
    }
    if (!pf || !pf->text_begin || !pf->text_begin(pf->context, p->area, &p->clip, i->font))
        return;
    if (o->owner_number != 0) {
        a[0] = '\0';
        if (ops->multiplayer && ops->multiplayer(ops->context) && ops->comm_name) {
            const char *n = ops->comm_name(ops->context, o->owner_number - 1);
            snprintf(a, sizeof a, "%s", n ? n : "");
        }
        if (a[0] == '\0' && ops->civ_name) ops->civ_name(ops->context, o->owner_civ, a, 100);
        pf->text_out(pf->context, p->x + 3, p->y - 2, a, (int32_t)strlen(a), 0xffffff);
    }
    b[0] = '\0';
    if (o->name_id > 0 && ops->string) ops->string(ops->context, o->name_id, b, 100);
    if (b[0] == '\0') snprintf(b, sizeof b, "%s", o->name ? o->name : "");
    if (ops->show_ids && ops->show_ids(ops->context))
        snprintf(a, sizeof a, "%d-%s", o->id, b);
    else
        snprintf(a, sizeof a, "%s", b);
    pf->text_out(pf->context, p->x + 3, p->y + 0xd, a, (int32_t)strlen(a), 0xffffff);
    if (o->hp > 0.0f) {
        snprintf(b, sizeof b, "%d/%d", ftol(o->hp), o->max_hp);
        pf->text_out(pf->context, p->x + 3, p->y + 0x5c, b, (int32_t)strlen(b), 0xffffff);
    }
    pf->text_end(pf->context);
}

/* The rows of a unit (panel kinds 2 and 8). */
static void unit_rows(AoeInfoPanelRor *i, const AoeInfoObjectRor *o) {
    int16_t type = 0, id = 0, progress = 0;
    int32_t row = 0, icon;
    int own = o->owner == i->player;
    if (o->type == 0x50) {
        production(o, &type, &id, &progress);
        if (o->object_state != 0 && (!own || (type != 0x67 && type != 0x66))) {
            if (o->armor > 0) {
                aoe_info_panel_item_ror(i, 0, 7, 1, o->armor_base, o->armor);
                row = 1;
            }
            if (ftol(o->range) > 0) {
                aoe_info_panel_item_ror(i, row, 6, 1, ftol(o->range_base), ftol(o->range));
                ++row;
            }
        }
        if (own) {
            if (o->object_state == 0) {
                aoe_info_panel_item_ror(i, row, 9, 3, o->build_percent, 0);
                ++row;
            } else if (type == 0x67 || type == 0x66) {
                aoe_info_panel_item_ror(i, row, 9, 3, progress, 0);
                ++row;
            } else {
                if (o->object_class == 0x46) {
                    int32_t pop = ftol(attribute(i, 4) + attribute(i, 11));
                    int32_t cap = ftol(attribute(i, 32));
                    aoe_info_panel_item_ror(i, row, 4, 2, ftol(attribute(i, 11)),
                                            cap < pop ? cap : pop);
                    ++row;
                }
                if (o->object_class == 0x32 && o->has_head_action) {
                    aoe_info_panel_item_ror(
                        i, row, 2, 0, ftol(o->head_action_2c + o->amount + 0.5f), 0);
                    ++row;
                }
            }
        }
    }
    /* What a finished object holds: a dock (class 0x2d) its owner's
       attribute 9, the rest their amount. */
    if (o->object_state == 2) {
        int show;
        if (!own) {
            show = o->object_class == 0x2d;
        } else {
            show = type != 0x67 && type != 0x66 &&
                   (o->object_class == 0x2d ||
                    (o->object_class != 0x32 && o->resource_type != -1 &&
                     ftol(o->amount) >= 1));
        }
        if (show && (icon = resource_icon(o->resource_type)) >= 0) {
            int32_t v = o->object_class == 0x2d
                            ? ftol(o->owner_attribute_9 + 0.5f)
                            : ftol(o->amount + 0.5f);
            aoe_info_panel_item_ror(i, row, icon, 0, v, 0);
            ++row;
        }
    }
    /* Someone else's dock: the trade profit from our nearest dock. */
    if (o->object_class == 0x2d && !own && i->ops->nearest) {
        float ox, oy;
        if (i->ops->nearest(i->ops->context, i->player, 0x2d, o->x, o->y, 2,
                            i->object, &ox, &oy)) {
            float dx = ox - o->x, dy = oy - o->y;
            float v = (dx * dx + dy * dy) * 0.000251256279f * 0.7f;
            if (i->ops->civilization &&
                i->ops->civilization(i->ops->context, i->player) == 0xf)
                v += v;
            if (v < 0.35) v = 0.35f;
            v *= 20.0f;
            if (v > 75.0f) v = 75.0f;
            aoe_info_panel_item_ror(i, row, 3, 2, ftol(v + 0.5f), 0x14);
        }
    }
}

/* The rows of panel kinds 3..7 and 9. */
static void soldier_rows(AoeInfoPanelRor *i, const AoeInfoObjectRor *o) {
    uint8_t k = o->panel_kind;
    int32_t row = 0, icon;
    if (k == 3 || k == 4) {
        aoe_info_panel_item_ror(i, 0, 7, 1, o->armor_base, o->armor);
        row = 1;
    }
    if (o->attack > 0 || k == 4 || k == 6)
        aoe_info_panel_item_ror(i, row++, 8, 1, o->attack_base, o->attack);
    if (o->pierce > 0)
        aoe_info_panel_item_ror(i, row++, 10, 1, o->pierce_base, o->pierce);
    if ((k == 4 || k == 6) && o->range > 0.99f)
        aoe_info_panel_item_ror(i, row++, 6, 1, ftol(o->range_base), ftol(o->range));
    if (k == 6)
        aoe_info_panel_item_ror(i, row++, 9, 3, ftol(o->amount + 0.5f), 0);
    if (o->owner != i->player) return;
    if (k == 7) {
        aoe_info_panel_item_ror(i, row, 4, 2, o->contents_count, o->capacity);
    } else if (k == 9) {
        int32_t v = o->resource_type == 0x11 || o->resource_type == 0
                        ? (int16_t)ftol(o->amount + 0.5f)
                        : 0;
        aoe_info_panel_item_ror(i, row, 2, 0, v, 0);
    } else if (k == 5) {
        if ((icon = resource_icon(o->resource_type)) < 0) return;
        if (o->amount > 0.0f)
            aoe_info_panel_item_ror(i, row, 3, 0, ftol(o->amount + 0.5f), 0);
        else
            aoe_info_panel_item_ror(i, row, icon, 4, -1, 0);
    } else if (ftol(o->amount + 0.5f) > 0) {
        if ((icon = resource_icon(o->resource_type)) < 0) return;
        aoe_info_panel_item_ror(i, row, icon, 0, ftol(o->amount + 0.5f), 0);
    }
}

void aoe_info_panel_draw_ror(AoeUiPanelRor *p) {
    AoeInfoPanelRor *i = IP(p);
    AoeInfoObjectRor o;
    if (!p->area || !p->visible || !p->active) {
        p->redraw = 0;
        return;
    }
    if (i->mode == 2) {
        aoe_info_panel_draw_scores_ror(i);
        return;
    }
    if (!i->object || !read_object(i, &o)) {
        if (p->parent) p->parent->vt->draw_rect(p->parent, &p->clip);
        p->redraw = 0;
        return;
    }
    p->vt->begin_draw(p, 0);
    fill_black(p);
    draw_header(i, &o);
    switch (o.panel_kind) {
    case 1: {
        int32_t icon = resource_icon(o.resource_type);
        if (icon >= 0) aoe_info_panel_item_ror(i, 0, icon, 0, ftol(o.amount + 0.5f), 0);
        break;
    }
    case 2: case 8:
        unit_rows(i, &o);
        break;
    case 3: case 4: case 5: case 6: case 7: case 9:
        soldier_rows(i, &o);
        break;
    default:
        break;
    }
    p->vt->end_draw(p);
}

int32_t aoe_info_panel_idle_ror(AoeUiPanelRor *p) {
    AoeInfoPanelRor *i = IP(p);
    AoeInfoObjectRor o;
    void *selected;
    int changed = 0, notify = 0;
    uint32_t now;
    aoe_ui_panel_handle_idle_ror(p);
    if (!p->active) return 0;
    now = now_ms(p);
    if (now - i->last_ms < i->interval_ms) return 0;
    i->last_ms = now_ms(p);
    if (i->mode == 2) {
        int32_t s;
        for (s = 1; s <= 8; ++s) {
            int32_t pl = i->ops->slot_player ? i->ops->slot_player(i->ops->context, s) : 0;
            if (pl != 0 && pl < 9 && i->ops->player_score &&
                i->ops->player_score(i->ops->context, pl) != i->scores[pl])
                break;
        }
        if (s > 8) return 0;
        if (!p->redraw) p->vt->set_redraw(p, 1);
        aoe_info_panel_snapshot_ror(i);
        return 0;
    }
    selected = i->player && i->ops->selected ? i->ops->selected(i->ops->context, i->player)
                                             : NULL;
    if (selected != i->object) {
        aoe_info_panel_set_object_ror(i, selected);
        return 0;
    }
    if (!i->object || !read_object(i, &o)) return 0;
    if (o.owner != i->snap_owner || o.field_37 != i->snap_37 ||
        o.template_id != i->snap_template) {
        notify = 1;
        goto redraw;
    }
    if (o.contents_count != i->snap_count) {
        changed = 1;
        if (o.contents_count == 0 || i->snap_count == 0) {
            notify = 1;
            goto redraw;
        }
    } else if (o.resource_type == i->snap_resource && o.amount == i->snap_amount &&
               o.hp == i->snap_hp && o.contents_first == i->snap_contents &&
               o.template_28 == i->snap_template_28 && o.object_class != 0x2d) {
        if (o.type >= 0x46 && o.type != 0x5a) {
            if (o.has_action != i->snap_has_action) {
                notify = 1;
                goto redraw;
            }
            if (o.attack != i->snap_attack || o.pierce != i->snap_152 ||
                o.armor != i->snap_armor || o.range != i->snap_range ||
                o.stat_23c != i->snap_23c)
                changed = 1;
        }
    } else {
        changed = 1;
    }
    if (o.type == 0x50) {
        int16_t type, id, progress;
        if (o.object_state != i->snap_state) {
            notify = 1;
            goto redraw;
        }
        if ((float)o.build_percent != i->snap_percent) {
            changed = 1;
            if ((float)o.build_percent > 99.0f) {
                notify = 1;
                goto redraw;
            }
        }
        production(&o, &type, &id, &progress);
        if (type != i->snap_action || id != i->snap_action_id ||
            progress != i->snap_progress ||
            strcmp(o.action_text, i->snap_text) != 0) {
            changed = 1;
            if (type != i->snap_action || id != i->snap_action_id ||
                strcmp(o.action_text, i->snap_text) != 0) {
                notify = 1;
                goto redraw;
            }
        }
        if (o.object_class == 0x46 &&
            (ftol(attribute(i, 11)) != i->snap_pop ||
             ftol(attribute(i, 4)) != i->snap_pop_cap))
            changed = 1;
        if (o.owner == i->player && o.object_class == 0x32 && o.has_head_action &&
            ftol(o.head_action_2c + o.amount + 0.5f) != i->snap_head)
            goto redraw;
    }
    if (!changed) return 0;
redraw:
    if (!p->redraw) p->vt->set_redraw(p, 1);
    aoe_info_panel_snapshot_ror(i);
    if (notify && p->parent) p->parent->vt->action(p->parent, p, 1, 0, 0);
    return 0;
}

static AoeUiPanelVtblRor vtable;
static int vtable_ready;

const AoeUiPanelVtblRor *aoe_info_panel_vtable_ror(void) {
    if (!vtable_ready) {
        vtable = aoe_ui_panel_vtable_ror;
        vtable.destroy = destroy;
        vtable.draw = aoe_info_panel_draw_ror;
        vtable.handle_idle = aoe_info_panel_idle_ror;
        vtable_ready = 1;
    }
    return &vtable;
}
