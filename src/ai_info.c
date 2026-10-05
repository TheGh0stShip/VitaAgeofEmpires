#include "ai_info.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

static int get_object(const AoeAiInfoOpsRor *ops, int32_t id,
                      AoeAiObjectViewRor *out) {
    return ops && ops->object && ops->object(ops->context, id, out);
}

int aoe_ai_info_zone_open_ror(const AoeAiInfoRor *info, int32_t id,
                              const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor o;
    if (!info || !get_object(ops, id, &o) || !ops->zone) return 0;
    return info->zone_flags[ops->zone(ops->context, id)] == 0;
}

void aoe_ai_info_set_zone_flag_ror(AoeAiInfoRor *info, int32_t id,
                                   uint8_t value, const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor o;
    if (!info || !get_object(ops, id, &o) || !ops->zone) return;
    info->zone_flags[ops->zone(ops->context, id)] = value;
}

static int in_known_map(const AoeAiInfoRor *info, int32_t x, int32_t y) {
    return x >= 0 && y >= 0 && x < info->map_width && y < info->map_height;
}

int aoe_ai_info_explored_ror(const AoeAiInfoRor *info, int32_t x, int32_t y,
                             const AoeAiInfoOpsRor *ops) {
    if (!info || !in_known_map(info, x, y) || !ops || !ops->grid_get) return 0;
    return ops->grid_get(ops->context, x, y) != 0xff;
}

void aoe_ai_info_forget_tile_ror(const AoeAiInfoRor *info, int32_t x,
                                 int32_t y, const AoeAiInfoOpsRor *ops) {
    if (info && in_known_map(info, x, y) && ops && ops->grid_set)
        ops->grid_set(ops->context, x, y, 0);
}

void aoe_ai_info_mark_rect_ror(const AoeAiInfoRor *info, int32_t x0,
                               int32_t y0, int32_t x1, int32_t y1,
                               const AoeAiInfoOpsRor *ops) {
    int32_t a[2], b[2], x, y;
    if (!info || !ops || !ops->grid_get || !ops->grid_set) return;
    a[0] = x0; a[1] = y0;
    aoe_ai_info_clamp_ror(info, a);
    b[0] = x1; b[1] = y1;
    aoe_ai_info_clamp_ror(info, b);
    for (x = a[0]; x <= b[0]; ++x)
        for (y = a[1]; y <= b[1]; ++y)
            if (ops->grid_get(ops->context, x, y) != 1)
                ops->grid_set(ops->context, x, y, 2);
}

int32_t aoe_ai_info_count_owned_ror(const AoeAiInfoRor *info,
                                    int32_t seen_owner, int32_t owner,
                                    const AoeAiInfoOpsRor *ops) {
    int32_t i, n = 0;
    if (!info || !ops || !ops->world_object) return 0;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiObjectViewRor o;
        if (e->id == -1 || !ops->world_object(ops->context, e->id, &o)) continue;
        if (o.owner == owner && (seen_owner == -1 || seen_owner == e->owner))
            ++n;
    }
    return n;
}

int aoe_ai_info_neighbour_terrain_ror(int32_t x, int32_t y, uint32_t value,
                                      const AoeAiInfoOpsRor *ops) {
    static const int8_t dx[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
    static const int8_t dy[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
    int32_t w, h;
    int i;
    if (!ops || !ops->map_width || !ops->map_height || !ops->tile_byte5)
        return 0;
    w = ops->map_width(ops->context);
    h = ops->map_height(ops->context);
    for (i = 0; i < 8; ++i) {
        int32_t nx = x + dx[i], ny = y + dy[i];
        if (nx >= 0 && ny >= 0 && nx <= w - 1 && ny <= h - 1 &&
            (uint32_t)(ops->tile_byte5(ops->context, nx, ny) & 0x1f) == value)
            return 1;
    }
    return 0;
}

int32_t aoe_ai_info_map_width_ror(AoeAiInfoRor *info,
                                  const AoeAiInfoOpsRor *ops) {
    if (info->map_width == -1 && ops && ops->map_width)
        info->map_width = ops->map_width(ops->context);
    return info->map_width;
}

int32_t aoe_ai_info_map_height_ror(AoeAiInfoRor *info,
                                   const AoeAiInfoOpsRor *ops) {
    if (info->map_height == -1 && ops && ops->map_height)
        info->map_height = ops->map_height(ops->context);
    return info->map_height;
}

AoeAiInfoObjectRor *aoe_ai_info_find_object_ror(const AoeAiInfoRor *info,
                                                int32_t id) {
    int32_t i;
    if (!info || id == -1) return NULL;
    for (i = 0; i < info->object_count; ++i)
        if (info->objects[i].id == id) return &info->objects[i];
    return NULL;
}

static void free_object(AoeAiInfoObjectRor *e) {
    memset(e, 0, sizeof(*e));
    e->id = -1;
    e->type_id = e->type_class = -1;
}

AoeAiInfoObjectRor *aoe_ai_info_store_object_ror(
    AoeAiInfoRor *info, const AoeAiInfoObjectRor *entry,
    const AoeAiObjectViewRor *seen, const AoeAiInfoOpsRor *ops) {
    int32_t i, free_slot = -1;
    AoeAiInfoObjectRor *grown;
    if (!info || !entry) return NULL;
    for (i = 0; i < info->object_count; ++i) {
        if (info->objects[i].id == entry->id) {
            info->objects[i] = *entry;
            return &info->objects[i];
        }
        if (info->objects[i].id == -1 && free_slot == -1) free_slot = i;
    }
    if (seen && ops) {
        uint16_t self = (uint16_t)ops->self;
        if (ops->sn && ops->sn(ops->context, 0xc9) == 1 &&
            (ops->stats_player == self || ops->stats_player == 0xff) &&
            seen->owner > 0 && seen->owner_4 == 0 && ops->settings_97d == 1) {
            int32_t bucket =
                aoe_ai_info_stat_bucket_ror(seen->type_class, seen->type_id);
            if (bucket != -1) ++info->stats[bucket];
        }
    }
    if (free_slot != -1) {
        info->objects[free_slot] = *entry;
        return &info->objects[free_slot];
    }
    if (info->object_count <= 0) return NULL;
    grown = (AoeAiInfoObjectRor *)malloc((size_t)info->object_count * 2 *
                                         sizeof(*grown));
    if (!grown) return NULL;
    memcpy(grown, info->objects, (size_t)info->object_count * sizeof(*grown));
    for (i = info->object_count; i < info->object_count * 2; ++i)
        free_object(&grown[i]);
    free(info->objects);
    info->objects = grown;
    i = info->object_count;
    grown[i] = *entry;
    info->object_count *= 2;
    return &grown[i];
}

AoeAiInfoObjectRor *aoe_ai_info_remember_ror(AoeAiInfoRor *info,
                                             const AoeAiObjectViewRor *object,
                                             float value_108,
                                             float value_100, float range,
                                             const AoeAiInfoOpsRor *ops) {
    AoeAiInfoObjectRor e;
    if (!object) return NULL;
    memset(&e, 0, sizeof e);
    e.id = object->id;
    e.type_id = object->type_id;
    e.type_class = object->type_class;
    e.x = (uint8_t)aoe_ftol32(object->x);
    e.y = (uint8_t)aoe_ftol32(object->y);
    e.z = (uint8_t)aoe_ftol32(object->z);
    e.owner = (uint8_t)object->owner;
    e.hp = (uint16_t)aoe_ftol32(object->hp);
    e.field_18 = value_108;
    e.field_1c = value_100;
    e.range = range;
    return aoe_ai_info_store_object_ror(info, &e, object, ops);
}

int aoe_ai_info_unit_class_ror(int32_t c) {
    switch (c) {
    case 0: case 2: case 3: case 4: case 6: case 0xc: case 0xd: case 0x11:
    case 0x12: case 0x14: case 0x15: case 0x16: case 0x17: case 0x18:
    case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x23: case 0x24:
    case 0x27:
        return 1;
    default:
        return 0;
    }
}

int aoe_ai_info_resource_class_ror(int32_t c) {
    switch (c) {
    case 5: case 7: case 8: case 9: case 0xf: case 0x1f: case 0x20: case 0x21:
        return 1;
    default:
        return 0;
    }
}

int aoe_ai_info_known_class_ror(int32_t c) {
    /* The third set, 1 and 0x1e, is the shared test at 0x004be200. */
    return aoe_ai_info_unit_class_ror(c) || aoe_ai_info_resource_class_ror(c) ||
           c == 1 || c == 0x1e;
}

int aoe_ai_info_key_building_ror(int16_t type_class, int16_t type_id) {
    if (type_class != 3) return 0;
    switch (type_id) {
    case 0x6d: case 0xc: case 0x57: case 0x31: case 0x2d: case 0:
    case 0x65: case 0x68:
        return 1;
    default:
        return 0;
    }
}

int32_t aoe_ai_info_list_148_count_ror(const AoeAiInfoRor *info) {
    return info->list_148.count;
}

void aoe_ai_info_set_field_204_ror(AoeAiInfoRor *info, int32_t value) {
    info->field_204 = value;
}

int32_t aoe_ai_info_current_target_ror(const AoeAiInfoRor *info, int32_t i) {
    return i >= 0 && i < 4 ? info->current_target[i] : -1;
}

int aoe_ai_info_clamp_ror(const AoeAiInfoRor *info, int32_t t[2]) {
    int moved = 0;
    if (t[0] < 0) {
        t[0] = 0;
        moved = 1;
    } else if (t[0] >= info->map_width) {
        t[0] = info->map_width - 1;
        moved = 1;
    }
    if (t[1] < 0) {
        t[1] = 0;
        return 1;
    }
    if (info->map_height <= t[1]) {
        moved = 1;
        t[1] = info->map_height - 1;
    }
    return moved;
}

int aoe_ai_info_tiles_near_ror(const int32_t a[2], const int32_t b[2],
                               int32_t mode) {
    int32_t d = (b[1] - a[1]) * (b[1] - a[1]) + (b[0] - a[0]) * (b[0] - a[0]);
    if (mode == 0) return d <= 1;
    if (mode == 1) return d <= 2;
    return 1;
}

int32_t aoe_ai_info_direction_ror(const int32_t a[2], const int32_t b[2]) {
    int32_t dx = b[0] - a[0], dy = b[1] - a[1];
    if (b[0] == a[0]) return dy >= 0 ? 5 : 1;
    if (b[1] == a[1]) return dx >= 0 ? 3 : 7;
    if (dx < 0) return dy >= 0 ? 6 : 0;
    return dy >= 0 ? 4 : 2;
}

void aoe_ai_info_set_mark_ror(AoeAiInfoRor *info, int32_t id, uint8_t x,
                              uint8_t y, uint8_t state) {
    int32_t i;
    if (!info) return;
    for (i = 0; i < info->mark_count; ++i) {
        AoeAiInfoMarkRor *m = &info->marks[i];
        if (m->id != id || m->x != x || m->y != y) continue;
        if (state != 3) {
            m->state = state;
        } else {
            m->id = -1;
            m->x = m->y = 0;
            m->state = 3;
        }
        return;
    }
}

AoeAiInfoMarkRor *aoe_ai_info_open_mark_ror(const AoeAiInfoRor *info,
                                            int32_t id) {
    int32_t i;
    if (!info) return NULL;
    for (i = 0; i < info->mark_count; ++i)
        if (info->marks[i].id == id && info->marks[i].state == 0)
            return &info->marks[i];
    return NULL;
}

int aoe_ai_info_mark_near_ror(const AoeAiInfoRor *info, int32_t id,
                              uint8_t x, uint8_t y, int32_t range) {
    int32_t i;
    if (!info) return 0;
    for (i = 0; i < info->mark_count; ++i) {
        const AoeAiInfoMarkRor *m = &info->marks[i];
        if (m->id != id || m->state != 1) continue;
        if (m->x - range <= x && x <= m->x + range && m->y - range <= y &&
            y <= m->y + range)
            return 1;
    }
    return 0;
}

int32_t aoe_ai_info_add_area_ror(AoeAiInfoRor *info, uint8_t field_4,
                                 uint8_t x0, uint8_t y0, uint8_t x1,
                                 uint8_t y1, uint8_t player_9,
                                 uint8_t player_a, uint16_t field_c,
                                 uint8_t field_e, uint32_t age,
                                 int32_t field_14,
                                 const AoeAiInfoOpsRor *ops) {
    int32_t i, slot = -1;
    if (!info) return -1;
    if (info->map_width > 0 && info->map_height > 0) {
        int32_t sum = ((int32_t)x0 + x1) * 4;
        int32_t col = sum / info->map_width / 2;
        int32_t row = sum / info->map_height / 2;
        int32_t *cell = NULL;
        if (col > 4) col = 3;
        if (row > 4) row = 3;
        if (col < 4 && row < 4 && ops) {
            if ((uint16_t)player_a == (uint16_t)ops->self)
                cell = &info->cells[col][row][1];
            else if ((uint16_t)player_9 == (uint16_t)ops->self)
                cell = &info->cells[col][row][2];
        }
        if (cell) ++*cell;
    }
    for (i = 0; i < info->area_count; ++i) {
        if (info->areas[i].id == -1) {
            slot = i;
            break;
        }
        if (slot == -1 || info->areas[slot].age < info->areas[i].age) slot = i;
    }
    if (slot != -1) {
        AoeAiInfoAreaRor *a = &info->areas[slot];
        a->id = i;
        a->field_4 = field_4;
        a->x0 = x0;
        a->y0 = y0;
        a->x1 = x1;
        a->y1 = y1;
        a->player_9 = player_9;
        a->player_a = player_a;
        a->field_c = field_c;
        a->field_e = field_e;
        a->age = age;
        a->field_14 = field_14;
    }
    return slot;
}

int32_t aoe_ai_info_add_tile_area_ror(AoeAiInfoRor *info, uint8_t field_4,
                                      uint8_t x, uint8_t y,
                                      uint8_t player_9, uint8_t player_a,
                                      uint16_t field_c, uint8_t field_e,
                                      uint32_t age, int32_t field_14,
                                      const AoeAiInfoOpsRor *ops) {
    return aoe_ai_info_add_area_ror(info, field_4, x, y, x, y, player_9,
                                    player_a, field_c, field_e, age,
                                    field_14, ops);
}

int32_t aoe_ai_info_count_near_ror(const AoeAiInfoRor *info, int32_t owner,
                                   int32_t x, int32_t y, int32_t radius,
                                   const AoeAiInfoOpsRor *ops) {
    int32_t i, n = 0, r2 = radius * radius;
    if (!info) return 0;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        int32_t dx, dy;
        if (owner == -1) {
            if (!ops || !ops->is_enemy || !ops->is_enemy(ops->context, e->owner))
                continue;
        } else if (e->owner != (uint32_t)owner) {
            continue;
        }
        dy = (int32_t)e->y - y;
        dx = (int32_t)e->x - x;
        if (dy * dy + dx * dx <= r2) ++n;
    }
    return n;
}

int aoe_ai_info_area_near_ror(const AoeAiInfoRor *info, float x, float y,
                              int32_t radius, uint32_t player_a) {
    int32_t i, ox = aoe_ftol32(x), oy = aoe_ftol32(y), r2 = radius * radius;
    if (!info) return 0;
    for (i = 0; i < info->area_count; ++i) {
        const AoeAiInfoAreaRor *a = &info->areas[i];
        int32_t dx0, dx1, dy0, dy1;
        if (a->id == -1 || !a->player_9 || a->player_a != player_a) continue;
        dx0 = (a->x0 - ox) * (a->x0 - ox);
        dx1 = (a->x1 - ox) * (a->x1 - ox);
        dy0 = (a->y0 - oy) * (a->y0 - oy);
        dy1 = (a->y1 - oy) * (a->y1 - oy);
        if (dy0 + dx0 <= r2 || dx0 + dy1 <= r2 || dy1 + dx1 <= r2 ||
            dy0 + dx1 <= r2)
            return 1;
    }
    return 0;
}

int aoe_ai_info_enemy_center_near_ror(const AoeAiInfoRor *info, float x,
                                      float y, int32_t radius,
                                      const AoeAiInfoOpsRor *ops) {
    int32_t i, ox = aoe_ftol32(x), oy = aoe_ftol32(y), r2 = radius * radius;
    if (!info || !ops || !ops->is_enemy) return 0;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        int32_t dx, dy;
        if (e->type_id != 0x6d || !ops->is_enemy(ops->context, e->owner))
            continue;
        dx = (int32_t)e->x - ox;
        dy = (int32_t)e->y - oy;
        if (dy * dy + dx * dx <= r2) return 1;
    }
    return 0;
}

int32_t aoe_ai_info_nearest_enemy_ror(const AoeAiInfoRor *info, float x,
                                      float y, int32_t radius, int32_t t0,
                                      int32_t t1, int32_t t2, int32_t t3,
                                      const AoeAiInfoOpsRor *ops) {
    int32_t i, ox = aoe_ftol32(x), oy = aoe_ftol32(y), r2 = radius * radius;
    int32_t best = -1, best_d = -1;
    if (!info || !ops || !ops->is_enemy) return -1;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiObjectViewRor o;
        int32_t d, t = e->type_id;
        if (!get_object(ops, e->id, &o) || o.state >= 3 || o.hp == 0.0f)
            continue;
        if (t != t0 && t != t1 && t != t2 && t != t3) continue;
        if (!ops->is_enemy(ops->context, e->owner)) continue;
        d = ((int32_t)e->y - oy) * ((int32_t)e->y - oy) +
            ((int32_t)e->x - ox) * ((int32_t)e->x - ox);
        if (d <= r2 && (d < best_d || best == -1)) {
            best = e->id;
            best_d = d;
        }
    }
    return best;
}

int aoe_ai_info_set_target_wait_ror(AoeAiInfoRor *info, int32_t list,
                                    int32_t id, uint8_t wait) {
    int32_t i;
    if (!info || list < 0 || list >= 4) return 0;
    for (i = 0; i < info->target_count[list]; ++i) {
        if (info->targets[list][i].id == id) {
            info->targets[list][i].wait = wait;
            return 1;
        }
    }
    return 0;
}

int32_t aoe_ai_info_stat_bucket_ror(int16_t type_class, int16_t type_id) {
    if (type_class == 0x12) return 5;
    if (type_class == 0x1c) return 3;
    if (type_class == 0xd) return 4;
    if (type_class == 0xc) return 2;
    if (type_class == 0x23) return 7;
    if (type_id == 0x49) return 0;
    if (type_id == 0x4b) return 1;
    if (type_id == 4 || type_id == 5 || type_id == 0x27) return 6;
    if (type_id == 0x29) return 10;
    if (type_id == 0x2e || type_id == 0x19) return 9;
    return type_class == 0x16 ? 8 : -1;
}

int32_t aoe_ai_info_stat_share_ror(const AoeAiInfoRor *info) {
    uint32_t sum = 0;
    int i;
    for (i = 0; i < AOE_AI_INFO_HISTORY; ++i) sum += (uint32_t)info->history[i];
    /* fild / fidiv / fmul 100.0f, then __ftol. A zero sum divides to an
       infinity or NaN, which __ftol turns into 0. */
    if (sum == 0) return 100;
    return 100 - aoe_ftol32((double)info->stats[0] / (double)(int32_t)sum *
                            (double)100.0f);
}

void aoe_ai_info_top_stats_ror(const AoeAiInfoRor *info, int32_t *first,
                               int32_t *second) {
    int32_t i, best = -1;
    *first = -1;
    for (i = 0; i < AOE_AI_INFO_STATS; ++i) {
        if (best == -1 || best < info->stats[i]) {
            *first = i;
            best = info->stats[i];
        }
    }
    best = -1;
    *second = -1;
    for (i = 0; i < AOE_AI_INFO_STATS; ++i) {
        if (i != *first && (best == -1 || best < info->stats[i])) {
            *second = i;
            best = info->stats[i];
        }
    }
}

/* ---- Dropsites, targets and threat ---- */

int32_t aoe_ai_info_docks_in_zone_ror(const AoeAiInfoRor *info,
                                      uint32_t zone,
                                      const AoeAiInfoOpsRor *ops) {
    int32_t i, n = 0;
    if (!info || !ops || !ops->map_zone) return 0;
    for (i = 0; i < info->list_190.count; ++i) {
        AoeAiObjectViewRor o;
        if (!get_object(ops, info->list_190.data[i], &o) || o.type_id != 0x2d)
            continue;
        if (ops->map_zone(ops->context, aoe_ftol32(o.x), aoe_ftol32(o.y)) == zone)
            ++n;
    }
    return n;
}

int32_t aoe_ai_info_nearest_dock_ror(const AoeAiInfoRor *info, int32_t x,
                                     int32_t y, const AoeAiInfoOpsRor *ops) {
    int32_t i, best = -1;
    if (!info) return -1;
    for (i = 0; i < info->list_190.count; ++i) {
        AoeAiObjectViewRor o;
        uint32_t dx, dy;
        int32_t d;
        if (!get_object(ops, info->list_190.data[i], &o) || o.type_id != 0x2d)
            continue;
        dx = (uint32_t)x - (uint32_t)aoe_ftol32(o.x);
        dy = (uint32_t)y - (uint32_t)aoe_ftol32(o.y);
        d = aoe_ftol32(sqrt((double)(int32_t)(dy * dy + dx * dx)));
        if (d != 0 && (best == -1 || d < best)) best = d;
    }
    return best;
}

float aoe_ai_info_nearest_dropsite_ror(const AoeAiInfoRor *info,
                                       const AoeAiObjectViewRor *resource,
                                       int32_t town_center, int32_t *out_id,
                                       const AoeAiInfoOpsRor *ops) {
    float best = 1000.0f;
    uint8_t zone;
    int32_t i;
    *out_id = -1;
    if (!info || !resource || !ops || !ops->zone || !ops->tile_zone)
        return best;
    zone = ops->zone(ops->context, resource->id);
    for (i = 0; i < info->list_190.count; ++i) {
        AoeAiObjectViewRor d;
        double dx, dy, dist;
        int take;
        if (!get_object(ops, info->list_190.data[i], &d)) continue;
        if (ops->tile_zone(ops->context, resource->id, aoe_ftol32(d.x),
                           aoe_ftol32(d.y)) != zone)
            continue;
        if (d.type_id == 0x6d)
            take = town_center != 0;
        else if (d.type_id == 0x67)
            take = resource->type_id != 0x3b && resource->type_id != 0x32;
        else if (d.type_id == 0x44)
            take = resource->type_id == 0x3b || resource->type_id == 0x32;
        else
            take = 0;
        if (!take) continue;
        dx = (double)d.x - (double)resource->x;
        dy = (double)d.y - (double)resource->y;
        dist = sqrt(dy * dy + dx * dx);
        /* fcomp 1000.0 (equal or unordered) or fcom best (below or
           unordered). */
        if (!(best != 1000.0f) || !(dist >= (double)best)) {
            *out_id = d.id;
            best = (float)dist;
        }
    }
    return best;
}

int32_t aoe_ai_info_gatherer_dropsite_ror(const AoeAiInfoRor *info,
                                          const AoeAiObjectViewRor *gatherer,
                                          const AoeAiInfoOpsRor *ops) {
    float best = 1000.0f;
    int32_t found = -1, i;
    uint8_t zone;
    if (!info || !gatherer || !ops || !ops->zone || !ops->tile_zone) return -1;
    zone = ops->zone(ops->context, gatherer->id);
    for (i = 0; i < info->list_190.count; ++i) {
        AoeAiObjectViewRor d;
        double dx, dy, dist;
        int take;
        if (!get_object(ops, info->list_190.data[i], &d)) continue;
        if (ops->tile_zone(ops->context, gatherer->id, aoe_ftol32(d.x),
                           aoe_ftol32(d.y)) != zone)
            continue;
        if (d.type_id == 0x6d)
            take = 1;
        else if (d.type_id == 0x67)
            take = gatherer->field_4c != 0x10;
        else if (d.type_id == 0x44)
            take = gatherer->field_4c == 0x10;
        else
            take = 0;
        if (!take) continue;
        dx = (double)d.x - (double)gatherer->x;
        dy = (double)d.y - (double)gatherer->y;
        dist = sqrt(dy * dy + dx * dx);
        if (found == -1 || !(dist >= (double)best)) {
            best = (float)dist;
            found = d.id;
        }
    }
    return found;
}

int32_t aoe_ai_info_count_dropsites_ror(const AoeAiInfoRor *info,
                                        int16_t type_id,
                                        const AoeAiInfoOpsRor *ops) {
    int32_t i, n = 0;
    if (!info) return 0;
    for (i = 0; i < info->list_190.count; ++i) {
        AoeAiObjectViewRor o;
        if (get_object(ops, info->list_190.data[i], &o) && o.type_id == type_id)
            ++n;
    }
    return n;
}

static void refresh_target(AoeAiInfoRor *info, AoeAiInfoTargetRor *t,
                           const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor o;
    int32_t dropsite = -1, v, slot;
    float d;
    if (!get_object(ops, t->id, &o)) return;
    d = aoe_ai_info_nearest_dropsite_ror(info, &o, 1, &dropsite, ops);
    t->field_8 = aoe_ftol32((double)d + (double)d + (double)10.0f);
    t->field_e = (uint8_t)aoe_ftol32(d);
    t->field_10 = dropsite;
    switch (o.type_class) {
    case 0xf: slot = 1; break;
    case 0x20: slot = 3; break;
    case 8: slot = 2; break;
    case 7: slot = 0; break;
    default: return;
    }
    v = aoe_ftol32(d);
    if (v < info->best_distance[slot] || info->best_distance[slot] == -1) {
        info->best_distance[slot] = v;
        info->current_target[slot] = o.id;
    }
}

void aoe_ai_info_refresh_targets_ror(AoeAiInfoRor *info, int32_t list,
                                     const AoeAiInfoOpsRor *ops) {
    int32_t i;
    if (!info || list < 0 || list >= 4) return;
    info->best_distance[list] = -1;
    info->current_target[list] = -1;
    for (i = 0; i < info->target_count[list]; ++i)
        if (info->targets[list][i].gone != 1)
            refresh_target(info, &info->targets[list][i], ops);
}

void aoe_ai_info_refresh_all_targets_ror(AoeAiInfoRor *info,
                                         const AoeAiInfoOpsRor *ops) {
    int32_t k, i;
    if (!info) return;
    for (k = 0; k < 4; ++k) {
        info->best_distance[k] = -1;
        info->current_target[k] = -1;
        for (i = 0; i < info->target_count[k]; ++i)
            refresh_target(info, &info->targets[k][i], ops);
    }
}

float aoe_ai_info_threat_ror(const AoeAiInfoRor *info, int32_t id,
                             int32_t x, int32_t y, const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor unit;
    float sum = 0.0f;
    int32_t i;
    if (!info || !get_object(ops, id, &unit)) return 0.0f;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiObjectViewRor o;
        double dist;
        float a, b;
        if (!ops->is_enemy || !ops->is_enemy(ops->context, e->owner)) continue;
        if (!get_object(ops, e->id, &o) || o.state > 2) continue;
        dist = ops->distance ? ops->distance(ops->context, o.id, (float)x,
                                             (float)y, o.z)
                             : 0.0;
        if (dist > (double)o.type_28) continue;
        a = (float)(ops->value_104 ? ops->value_104(ops->context, id, o.id) : 0.0);
        b = (float)(ops->value_100 ? ops->value_100(ops->context, o.id) : 0.0);
        if (b == 0.0f || a == 0.0f) continue;
        sum = (float)((double)a / (double)b + (double)sum);
    }
    return sum;
}

int aoe_ai_info_in_reach_ror(const AoeAiInfoRor *info, int32_t owner,
                             int32_t x, int32_t y, int32_t range_only,
                             const AoeAiInfoOpsRor *ops) {
    int32_t i;
    if (!info || !ops) return 0;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiObjectViewRor o;
        float d;
        double r;
        if (owner == -1) {
            if (!ops->is_enemy || !ops->is_enemy(ops->context, e->owner))
                continue;
        } else if (e->owner != (uint32_t)owner) {
            continue;
        }
        if (!get_object(ops, e->id, &o) || o.state >= 3) continue;
        d = (float)(ops->distance ? ops->distance(ops->context, o.id, (float)x,
                                                  (float)y, o.z)
                                  : 0.0);
        r = ops->range ? ops->range(ops->context, o.id) : 0.0;
        if (range_only == 0 && d < o.type_28) r = o.type_28;
        if ((double)d <= r) return 1;
    }
    return 0;
}

static float damage_one(int32_t id, int32_t attacker, float sum,
                        const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor o;
    float a;
    double b;
    if (!get_object(ops, id, &o)) return sum;
    a = (float)(ops->value_104 ? ops->value_104(ops->context, attacker, id) : 0.0);
    b = ops->value_100 ? ops->value_100(ops->context, id) : 0.0;
    /* fdivr: a / b in extended precision; no zero test. */
    return (float)((double)a / b + (double)sum);
}

float aoe_ai_info_group_damage_ror(const AoeAiInfoRor *info,
                                   const AoeAiUnitGroupRor *group,
                                   int32_t attacker,
                                   const AoeAiInfoOpsRor *ops) {
    float sum = 0.0f;
    int32_t i;
    (void)info;
    if (!group || !ops) return 0.0f;
    for (i = 0; i < aoe_ai_unit_group_count_ror(group); ++i)
        sum = damage_one(aoe_ai_unit_group_member_ror(group, i), attacker, sum,
                         ops);
    return sum;
}

float aoe_ai_info_ids_damage_ror(const AoeAiInfoRor *info,
                                 const int32_t *ids, int32_t count,
                                 int32_t attacker,
                                 const AoeAiInfoOpsRor *ops) {
    float sum = 0.0f;
    int32_t i;
    (void)info;
    if (!ids || !ops) return 0.0f;
    for (i = 0; i < count; ++i) sum = damage_one(ids[i], attacker, sum, ops);
    return sum;
}

static float endurance_one(const AoeAiInfoRor *info, int32_t id,
                           const AoeAiObjectViewRor *unit, float sum,
                           const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor o;
    float threat;
    double part;
    if (!get_object(ops, id, &o)) return sum;
    threat = aoe_ai_info_threat_ror(info, id, aoe_ftol32(unit->x),
                                    aoe_ftol32(unit->y), ops);
    part = threat == 0.0f ? (double)600.0f : (double)o.hp / (double)threat;
    return (float)(part + (double)sum);
}

float aoe_ai_info_group_endurance_ror(const AoeAiInfoRor *info,
                                      const AoeAiUnitGroupRor *group,
                                      const AoeAiObjectViewRor *unit,
                                      const AoeAiInfoOpsRor *ops) {
    float sum = 0.0f;
    int32_t i;
    if (!group || !unit) return 0.0f;
    for (i = 0; i < aoe_ai_unit_group_count_ror(group); ++i)
        sum = endurance_one(info, aoe_ai_unit_group_member_ror(group, i), unit,
                            sum, ops);
    return sum;
}

float aoe_ai_info_ids_endurance_ror(const AoeAiInfoRor *info,
                                    const int32_t *ids, int32_t count,
                                    const AoeAiObjectViewRor *unit,
                                    const AoeAiInfoOpsRor *ops) {
    float sum = 0.0f;
    int32_t i;
    if (!ids || !unit) return 0.0f;
    for (i = 0; i < count; ++i) sum = endurance_one(info, ids[i], unit, sum, ops);
    return sum;
}

/* ---- Sightings, lists and penalty maps ---- */

/* Append unless present, searching up to the capacity and growing to
   count + 1 (the inline form shared by these members). */
static void list_add_cap(AoeAiIntListRor *list, int32_t value) {
    int32_t i;
    for (i = 0; i < list->count && i < list->capacity; ++i)
        if (list->data[i] == value) return;
    aoe_ai_int_list_add_unique_ror(list, value);
}

/* Remove the first match within the capacity, shifting the rest down;
   the count drops (not below 0). */
static void list_remove_cap(AoeAiIntListRor *list, int32_t value) {
    int32_t i, n = list->capacity;
    for (i = 0; i < n; ++i)
        if (list->data[i] == value) break;
    if (i >= n) return;
    for (; i < n - 1; ++i) list->data[i] = list->data[i + 1];
    if (--list->count < 0) list->count = 0;
}

static int sn_positive(const AoeAiInfoOpsRor *ops, int32_t index) {
    return ops->sn && ops->sn(ops->context, index) > 0;
}

int aoe_ai_info_note_special_ror(AoeAiInfoRor *info,
                                 const AoeAiObjectViewRor *object,
                                 const AoeAiInfoOpsRor *ops) {
    int32_t number;
    if (!info || !object || !ops) return 0;
    switch (object->type_id) {
    case 0x2d:
        if (object->owner != ops->self || !sn_positive(ops, 0x46)) return 0;
        break;
    case 0x6d:
        if (object->owner != ops->self || !sn_positive(ops, 0x38)) return 0;
        break;
    case 0x9e:
    case 0xa3:
        if (!sn_positive(ops, 0x36)) return 0;
        break;
    case 0x9f:
        if (!sn_positive(ops, 0x37)) return 0;
        break;
    default:
        if (object->type_class == 7)
            number = 0x34;
        else if (object->type_class == 8)
            number = 0x33;
        else if (object->type_class == 0x20)
            number = 0x32;
        else
            return 0;
        if (!sn_positive(ops, number)) return 0;
        break;
    }
    list_add_cap(&info->list_148, object->id);
    return 1;
}

static void keep_best(AoeAiInfoRor *info, int16_t type_class, float d,
                      int32_t id) {
    int32_t slot, v;
    switch (type_class) {
    case 0xf: slot = 1; break;
    case 0x20: slot = 3; break;
    case 8: slot = 2; break;
    case 7: slot = 0; break;
    default: return;
    }
    v = aoe_ftol32(d);
    if (v < info->best_distance[slot] || info->best_distance[slot] == -1) {
        info->best_distance[slot] = v;
        info->current_target[slot] = id;
    }
}

int aoe_ai_info_add_target_ror(AoeAiInfoRor *info,
                               const AoeAiObjectViewRor *object,
                               const AoeAiInfoOpsRor *ops) {
    int32_t list, i, cap, dropsite = -1;
    AoeAiInfoTargetRor *t;
    float d;
    if (!info || !object) return 0;
    if (object->type_class == 0xf)
        list = 1;
    else if (object->type_class == 8)
        list = 2;
    else if (object->type_class == 0x20)
        list = 3;
    else
        list = 0;
    if (info->target_count[list] == info->target_capacity[list] ||
        !info->targets[list]) {
        AoeAiInfoTargetRor *grown;
        cap = info->target_capacity[list] ? info->target_capacity[list] * 2 : 100;
        grown = (AoeAiInfoTargetRor *)malloc((size_t)cap * sizeof(*grown));
        if (!grown) return 0;
        i = 0;
        if (info->targets[list])
            for (; i < info->target_capacity[list]; ++i)
                grown[i] = info->targets[list][i];
        for (; i < cap; ++i) {
            memset(&grown[i], 0, sizeof grown[i]);
            grown[i].id = -1;
            grown[i].field_8 = 0xff;
            grown[i].field_e = grown[i].pad_f = 0xff;
            grown[i].field_10 = -1;
        }
        free(info->targets[list]);
        info->targets[list] = grown;
        info->target_capacity[list] = cap;
    }
    for (i = 0; i < info->target_capacity[list]; ++i) {
        t = &info->targets[list][i];
        if (t->id != object->id) continue;
        d = aoe_ai_info_nearest_dropsite_ror(info, object, 1, &dropsite, ops);
        t->field_8 = aoe_ftol32((double)d + (double)d + (double)10.0f);
        t->field_10 = dropsite;
        keep_best(info, object->type_class, d, t->id);
        return 1;
    }
    t = &info->targets[list][info->target_count[list]];
    t->id = object->id;
    t->x = (uint8_t)aoe_ftol32(object->x);
    t->y = (uint8_t)aoe_ftol32(object->y);
    t->picks = 0;
    t->wait = 0;
    t->gone = 0;
    t->pad_f = (uint8_t)list;
    d = aoe_ai_info_nearest_dropsite_ror(info, object, 1, &dropsite, ops);
    t->field_e = (uint8_t)aoe_ftol32(d);
    t->field_10 = dropsite;
    t->field_8 = aoe_ftol32((double)d + (double)d + (double)10.0f);
    ++info->target_count[list];
    keep_best(info, object->type_class, d, object->id);
    return 1;
}

void aoe_ai_info_own_object_ror(AoeAiInfoRor *info,
                                const AoeAiObjectViewRor *object,
                                const AoeAiInfoOpsRor *ops) {
    AoeAiInfoObjectRor *e;
    if (!info || !object || object->id < 0) return;
    e = aoe_ai_info_find_object_ror(info, object->id);
    if (e) e->owner = (uint8_t)object->owner;
    list_add_cap(&info->list_180, object->id);
    if (object->type_class != 3 && object->type_class != 0x1b) return;
    list_add_cap(&info->list_190, object->id);
    if (object->type_id == 0x6d || object->type_id == 0x67 ||
        object->type_id == 0x44)
        aoe_ai_info_refresh_all_targets_ror(info, ops);
    if (object->type_id == 0x32) aoe_ai_info_add_target_ror(info, object, ops);
}

void aoe_ai_info_lose_object_ror(AoeAiInfoRor *info, int32_t id,
                                 const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor o;
    if (!info) return;
    list_remove_cap(&info->list_190, id);
    list_remove_cap(&info->list_180, id);
    if (get_object(ops, id, &o)) {
        AoeAiInfoObjectRor *e = aoe_ai_info_find_object_ror(info, id);
        if (e) e->owner = (uint8_t)o.owner;
    }
}

int aoe_ai_info_notice_ror(AoeAiInfoRor *info,
                           const AoeAiObjectViewRor *object, int32_t from_ally,
                           const AoeAiInfoOpsRor *ops) {
    int32_t c, special;
    if (!info || !object || object->id < 0 || !ops) return 0;
    c = object->type_class;
    if (!aoe_ai_info_known_class_ror(c)) return 0;
    if (from_ally == 0 && object->owner > 0 && ops->is_enemy &&
        ops->is_enemy(ops->context, object->owner) && ops->player_count > 3 &&
        ops->sn && ops->sn(ops->context, 0xc2) == 1) {
        int32_t p;
        for (p = 1; p < ops->player_count; ++p) {
            if (p == ops->self) continue;
            if (!ops->player_field_4 || !ops->player_field_4(ops->context, p))
                continue;
            if (!ops->is_ally || !ops->is_ally(ops->context, p)) continue;
            if (!ops->names_us_ally || !ops->names_us_ally(ops->context, p))
                continue;
            if (ops->share) ops->share(ops->context, p, object->id);
        }
    }
    special = aoe_ai_info_note_special_ror(info, object, ops);
    list_add_cap(&info->list_118, object->id);
    if (aoe_ai_info_unit_class_ror(c) == 1)
        list_add_cap(&info->list_128, object->id);
    else if (aoe_ai_info_resource_class_ror(c) == 1)
        aoe_ai_info_add_target_ror(info, object, ops);
    else if (c == 1 || c == 0x1e)
        list_add_cap(&info->list_138, object->id);
    if (aoe_ai_info_unit_class_ror(c) == 1 || special == 1) {
        float v108 = (float)(ops->value_108 ? ops->value_108(ops->context, object->id) : 0.0);
        float v100 = (float)(ops->value_100 ? ops->value_100(ops->context, object->id) : 0.0);
        float range = (float)(ops->range ? ops->range(ops->context, object->id) : 0.0);
        aoe_ai_info_remember_ror(info, object, v108, v100, range, ops);
    }
    return 1;
}

int aoe_ai_info_notice_id_ror(AoeAiInfoRor *info, int32_t id,
                              const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor o;
    if (!get_object(ops, id, &o)) return 0;
    return aoe_ai_info_notice_ror(info, &o, 0, ops);
}

static void raise_penalty(const AoeAiInfoRor *info, int32_t x0, int32_t y0,
                          int32_t x1, int32_t y1, const AoeAiInfoOpsRor *ops) {
    int32_t a[2], b[2], x, y;
    a[0] = x0; a[1] = y0;
    aoe_ai_info_clamp_ror(info, a);
    b[0] = x1; b[1] = y1;
    aoe_ai_info_clamp_ror(info, b);
    for (x = a[0]; x <= b[0]; ++x)
        for (y = a[1]; y <= b[1]; ++y) {
            uint32_t v = (uint32_t)ops->penalty_get(ops->context, x, y) + 10;
            ops->penalty_set(ops->context, x, y, (uint8_t)(v > 0xff ? 0xff : v));
        }
}

void aoe_ai_info_penalize_objects_ror(const AoeAiInfoRor *info,
                                      uint32_t owner, int32_t skip,
                                      const AoeAiInfoOpsRor *ops) {
    int32_t i;
    if (!info || !ops || !ops->penalty_get || !ops->penalty_set) return;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiObjectViewRor o;
        double r;
        if (e->owner != owner || e->id == skip || !get_object(ops, e->id, &o))
            continue;
        r = o.type_28;
        raise_penalty(info, aoe_ftol32((double)o.x - r), aoe_ftol32((double)o.y - r),
                      aoe_ftol32(r + (double)o.x), aoe_ftol32(r + (double)o.y), ops);
        if (!(e->range > 1.0f)) continue;
        r = e->range;
        raise_penalty(info, aoe_ftol32((double)o.x - r), aoe_ftol32((double)o.y - r),
                      aoe_ftol32((double)o.x + r), aoe_ftol32(r + (double)o.y), ops);
    }
}

static void fill_penalty(const AoeAiInfoRor *info, int32_t a[2], int32_t b[2],
                         uint8_t value, const AoeAiInfoOpsRor *ops) {
    int32_t x, y;
    aoe_ai_info_clamp_ror(info, a);
    aoe_ai_info_clamp_ror(info, b);
    for (x = a[0]; x <= b[0]; ++x)
        for (y = a[1]; y <= b[1]; ++y) ops->penalty_set(ops->context, x, y, value);
}

void aoe_ai_info_penalize_areas_ror(const AoeAiInfoRor *info,
                                    uint32_t player_a,
                                    const AoeAiInfoOpsRor *ops) {
    int32_t i;
    if (!info || !ops || !ops->penalty_set) return;
    for (i = 0; i < info->area_count; ++i) {
        const AoeAiInfoAreaRor *r = &info->areas[i];
        int32_t a[2], b[2];
        if (r->id == -1 || r->player_a != player_a ||
            (uint16_t)r->player_9 != (uint16_t)ops->self)
            continue;
        a[0] = r->x0; a[1] = r->y0;
        b[0] = r->x1; b[1] = r->y1;
        fill_penalty(info, a, b, 0x4b, ops);
        a[0] -= 5; a[1] -= 5;
        b[0] += 5; b[1] += 5;
        fill_penalty(info, a, b, 0xf, ops);
    }
}

void aoe_ai_info_unit_influence_ror(const AoeAiInfoRor *info, uint32_t owner,
                                    int32_t x, int32_t y,
                                    const AoeAiInfoOpsRor *ops) {
    int32_t i;
    if (!info || !ops || !ops->influence_clear || !ops->influence_origin ||
        !ops->influence_add || !ops->influence_finish)
        return;
    ops->influence_clear(ops->context, 0);
    ops->influence_origin(ops->context, 0, x, y);
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiObjectViewRor o;
        int32_t r, ox, oy;
        double v;
        if (e->owner != owner || !get_object(ops, e->id, &o)) continue;
        r = aoe_ftol32(o.type_28);
        ox = aoe_ftol32(o.x);
        oy = aoe_ftol32(o.y);
        v = ops->value_108 ? ops->value_108(ops->context, o.id) : 0.0;
        ops->influence_add(ops->context, 0, ox - r, oy - r, ox + r, r + oy,
                           v > 0.0 ? 5 : 1);
    }
    ops->influence_finish(ops->context, 0);
}

void aoe_ai_info_area_influence_ror(const AoeAiInfoRor *info,
                                    uint32_t player_a, int32_t x, int32_t y,
                                    const AoeAiInfoOpsRor *ops) {
    int32_t i;
    if (!info || !ops || !ops->influence_clear || !ops->influence_origin ||
        !ops->influence_add || !ops->influence_finish)
        return;
    ops->influence_clear(ops->context, 1);
    ops->influence_origin(ops->context, 1, x, y);
    for (i = 0; i < info->area_count; ++i) {
        const AoeAiInfoAreaRor *r = &info->areas[i];
        if (r->player_a != player_a ||
            (uint16_t)r->player_9 != (uint16_t)ops->self)
            continue;
        ops->influence_add(ops->context, 1, r->x0, r->y0, r->x1, r->y1, 1);
    }
    ops->influence_finish(ops->context, 1);
}

/* ---- Choosing targets ---- */

static int seen_as(const AoeAiInfoObjectRor *e, uint32_t owner,
                   const AoeAiInfoOpsRor *ops) {
    if (owner == 0xffffffffu)
        return ops->is_enemy && ops->is_enemy(ops->context, e->owner);
    return e->owner == owner;
}

AoeAiInfoObjectRor *aoe_ai_info_best_9e_ror(AoeAiInfoRor *info,
                                            uint32_t owner,
                                            const AoeAiInfoOpsRor *ops) {
    AoeAiInfoObjectRor *best = NULL;
    int32_t i, best_score = -1, center;
    if (!info || !ops) return NULL;
    center = ops->first_of_type ? ops->first_of_type(ops->context, 0x6d) : -1;
    for (i = 0; i < info->object_count; ++i) {
        AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiObjectViewRor o;
        int32_t score;
        if (e->type_id != 0x9e && e->type_id != 0xa3) continue;
        if (!seen_as(e, owner, ops)) continue;
        if (!get_object(ops, e->id, &o) || o.state >= 3) continue;
        score = aoe_ai_info_count_near_ror(info, (int32_t)owner, e->x, e->y, 5, ops);
        if (center != -1 && ops->distance)
            score *= aoe_ftol32(ops->distance(ops->context, center, (float)e->x,
                                              (float)e->y, (float)e->z));
        if (!best || score < best_score) {
            best = e;
            best_score = score;
        }
    }
    return best;
}

AoeAiInfoObjectRor *aoe_ai_info_best_9f_ror(AoeAiInfoRor *info,
                                            uint32_t owner,
                                            const AoeAiInfoOpsRor *ops) {
    AoeAiInfoObjectRor *best = NULL;
    int32_t i, best_score = -1, center;
    if (!info || !ops || !ops->vision) return NULL;
    center = ops->first_of_type ? ops->first_of_type(ops->context, 0x6d) : -1;
    for (i = 0; i < info->object_count; ++i) {
        AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiObjectViewRor o;
        int32_t score, x, y;
        if (e->type_id != 0x9f || !seen_as(e, owner, ops)) continue;
        if (!get_object(ops, e->id, &o) || o.state >= 3) continue;
        x = aoe_ftol32(o.x);
        y = aoe_ftol32(o.y);
        if (!ops->vision(ops->context, x, y)) continue;
        e->x = (uint8_t)aoe_ftol32(o.x);
        e->y = (uint8_t)aoe_ftol32(o.y);
        e->z = (uint8_t)aoe_ftol32(o.z);
        score = aoe_ai_info_count_near_ror(info, (int32_t)owner, x, y, 5, ops);
        if (center != -1 && ops->object_distance)
            score *= aoe_ftol32(ops->object_distance(ops->context, center, o.id));
        if (!best || score < best_score) {
            best = e;
            best_score = score;
        }
    }
    return best;
}

int32_t aoe_ai_info_resource_need_ror(const AoeAiInfoRor *info, int32_t mode,
                                      const AoeAiInfoOpsRor *ops) {
    if (mode == 2) return 0;
    if (ops && ops->build_has && !ops->build_has(ops->context, 0x67, 0) &&
        info->best_distance[1] != -1)
        return 1;
    if (mode == 1 && ops && ops->sn) {
        if (info->best_distance[1] != -1 &&
            ops->sn(ops->context, 0xa4) < info->best_distance[1])
            return 1;
        if (info->best_distance[3] != -1 &&
            ops->sn(ops->context, 0xa6) < info->best_distance[3])
            return 3;
        if (info->best_distance[0] != -1 && 15 < info->best_distance[0])
            return 2;
        if (info->best_distance[2] != -1 &&
            ops->sn(ops->context, 0xa5) < info->best_distance[2])
            return 2;
    }
    return -1;
}

/* The target is reachable from the town centre's zone (or there is no
   centre or target). */
static int same_zone_as_center(const AoeAiInfoRor *info, int32_t k,
                               const AoeAiInfoOpsRor *ops) {
    AoeAiObjectViewRor t;
    int32_t center;
    if (!ops->first_of_type || !ops->zone || !ops->tile_zone) return 1;
    center = ops->first_of_type(ops->context, 0x6d);
    if (center == -1 || !get_object(ops, info->current_target[k], &t)) return 1;
    return ops->zone(ops->context, center) ==
           ops->tile_zone(ops->context, center, aoe_ftol32(t.x), aoe_ftol32(t.y));
}

int aoe_ai_info_target_far_ror(const AoeAiInfoRor *info, int32_t k,
                               const AoeAiInfoOpsRor *ops) {
    int32_t age;
    if (!info || !ops || k < 0 || k >= 4) return 0;
    age = aoe_ftol32(ops->resource ? ops->resource(ops->context, 6) : 0.0f) - 1;
    if (age < 0)
        age = 0;
    else if (age > 3)
        age = 3;
    if (!info->age_allowed[k][age]) return 0;
    if (!same_zone_as_center(info, k, ops)) return 0;
    return info->best_distance[k] > (ops->sn ? ops->sn(ops->context, 0xa3 + k) : -1);
}

int aoe_ai_info_food_far_ror(const AoeAiInfoRor *info,
                             const AoeAiInfoOpsRor *ops) {
    if (!info || !ops || info->best_distance[0] == -1) return 0;
    if (!same_zone_as_center(info, 0, ops)) return 0;
    return (ops->sn ? ops->sn(ops->context, 0xa3) : -1) < info->best_distance[0];
}

void aoe_ai_info_scan_map_ror(AoeAiInfoRor *info,
                              const AoeAiInfoOpsRor *ops) {
    int32_t x, y, ids[256];
    if (!info || !ops || !ops->tile_byte5) return;
    for (x = 0; x < info->map_width; ++x) {
        for (y = 0; y < info->map_height; ++y) {
            uint8_t terrain = ops->tile_byte5(ops->context, x, y) & 0x1f;
            int32_t n, i, trees = 0;
            if (info->map_width != -1) {
                int32_t col = x * 4 / info->map_width;
                int32_t row = y * 4 / info->map_height;
                if (col >= 0 && col < 4 && row >= 0 && row < 4)
                    ++info->cells[col][row][0];
            }
            if (ops->grid_set) ops->grid_set(ops->context, x, y, 0);
            n = ops->tile_objects ? ops->tile_objects(ops->context, x, y, ids, 256)
                                  : 0;
            for (i = 0; i < n; ++i) {
                AoeAiObjectViewRor o;
                if (!ops->world_object || !ops->world_object(ops->context, ids[i], &o))
                    continue;
                if (aoe_ai_info_known_class_ror(o.type_class) == 1)
                    aoe_ai_info_notice_ror(info, &o, 0, ops);
                if (o.type_class == 0xf) ++trees;
            }
            if (n > 0 && trees > 0 &&
                (terrain == 10 || terrain == 0x13 || terrain == 0xd ||
                 terrain == 0x14))
                ++info->field_560;
        }
    }
}
