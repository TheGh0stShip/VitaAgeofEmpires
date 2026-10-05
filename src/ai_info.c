#include "ai_info.h"

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

int32_t aoe_ai_info_field_14c_ror(const AoeAiInfoRor *info) {
    return info->field_14c;
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
