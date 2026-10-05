#include "ai_info.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"
#include "value_grid.h"
#include "le_bytes.h"
#include "aoe_stream.h"
#include "unit_class.h"

static int get_object(const AoeAiInfoOpsRor *ops, int32_t id,
                      AoeAiWorldObjectRor *out) {
    return ops && ops->object && ops->object(ops->context, id, out);
}

int aoe_ai_info_zone_open_ror(const AoeAiInfoRor *info, int32_t id,
                              const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor o;
    if (!info || !get_object(ops, id, &o) || !ops->zone) return 0;
    return info->zone_flags[ops->zone(ops->context, id)] == 0;
}

void aoe_ai_info_set_zone_flag_ror(AoeAiInfoRor *info, int32_t id,
                                   uint8_t value, const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor o;
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
        AoeAiWorldObjectRor o;
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
    const AoeAiWorldObjectRor *seen, const AoeAiInfoOpsRor *ops) {
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
                                             const AoeAiWorldObjectRor *object,
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
        AoeAiWorldObjectRor o;
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
        AoeAiWorldObjectRor o;
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
        AoeAiWorldObjectRor o;
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
                                       const AoeAiWorldObjectRor *resource,
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
        AoeAiWorldObjectRor d;
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
                                          const AoeAiWorldObjectRor *gatherer,
                                          const AoeAiInfoOpsRor *ops) {
    float best = 1000.0f;
    int32_t found = -1, i;
    uint8_t zone;
    if (!info || !gatherer || !ops || !ops->zone || !ops->tile_zone) return -1;
    zone = ops->zone(ops->context, gatherer->id);
    for (i = 0; i < info->list_190.count; ++i) {
        AoeAiWorldObjectRor d;
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
        AoeAiWorldObjectRor o;
        if (get_object(ops, info->list_190.data[i], &o) && o.type_id == type_id)
            ++n;
    }
    return n;
}

static void refresh_target(AoeAiInfoRor *info, AoeAiInfoTargetRor *t,
                           const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor o;
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
    AoeAiWorldObjectRor unit;
    float sum = 0.0f;
    int32_t i;
    if (!info || !get_object(ops, id, &unit)) return 0.0f;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        AoeAiWorldObjectRor o;
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
        AoeAiWorldObjectRor o;
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
    AoeAiWorldObjectRor o;
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
                           const AoeAiWorldObjectRor *unit, float sum,
                           const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor o;
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
                                      const AoeAiWorldObjectRor *unit,
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
                                    const AoeAiWorldObjectRor *unit,
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

static int sn_positive(const AoeAiInfoOpsRor *ops, int32_t index) {
    return ops->sn && ops->sn(ops->context, index) > 0;
}

int aoe_ai_info_note_special_ror(AoeAiInfoRor *info,
                                 const AoeAiWorldObjectRor *object,
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
                               const AoeAiWorldObjectRor *object,
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
                                const AoeAiWorldObjectRor *object,
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
    AoeAiWorldObjectRor o;
    if (!info) return;
    aoe_ai_int_list_remove_ror(&info->list_190, id);
    aoe_ai_int_list_remove_ror(&info->list_180, id);
    if (get_object(ops, id, &o)) {
        AoeAiInfoObjectRor *e = aoe_ai_info_find_object_ror(info, id);
        if (e) e->owner = (uint8_t)o.owner;
    }
}

int aoe_ai_info_notice_ror(AoeAiInfoRor *info,
                           const AoeAiWorldObjectRor *object, int32_t from_ally,
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
    AoeAiWorldObjectRor o;
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
        AoeAiWorldObjectRor o;
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
        AoeAiWorldObjectRor o;
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
        AoeAiWorldObjectRor o;
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
        AoeAiWorldObjectRor o;
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
    AoeAiWorldObjectRor t;
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

/* One tile of a scan; `fog` skips objects on an explored-but-unseen tile
   whose type is not shown in fog. */
static void scan_tile(AoeAiInfoRor *info, int32_t x, int32_t y, int fog,
                      const AoeAiInfoOpsRor *ops) {
    int32_t ids[256], n, i, trees = 0;
    uint8_t terrain = ops->tile_byte5(ops->context, x, y) & 0x1f;
    uint8_t seen = fog && ops->vision ? ops->vision(ops->context, x, y) : 0;
    if (info->map_width != -1) {
        int32_t col = x * 4 / info->map_width;
        int32_t row = y * 4 / info->map_height;
        if (col >= 0 && col < 4 && row >= 0 && row < 4)
            ++info->cells[col][row][0];
    }
    if (ops->grid_set) ops->grid_set(ops->context, x, y, 0);
    n = ops->tile_objects ? ops->tile_objects(ops->context, x, y, ids, 256) : 0;
    for (i = 0; i < n; ++i) {
        AoeAiWorldObjectRor o;
        if (!ops->world_object || !ops->world_object(ops->context, ids[i], &o))
            continue;
        if (seen == 0x80 && !o.fog_visible) continue;
        if (aoe_ai_info_known_class_ror(o.type_class) == 1)
            aoe_ai_info_notice_ror(info, &o, 0, ops);
        if (o.type_class == 0xf) ++trees;
    }
    if (n > 0 && trees > 0 &&
        (terrain == 10 || terrain == 0x13 || terrain == 0xd || terrain == 0x14))
        ++info->field_560;
}

void aoe_ai_info_scan_map_ror(AoeAiInfoRor *info,
                              const AoeAiInfoOpsRor *ops) {
    int32_t x, y;
    if (!info || !ops || !ops->tile_byte5) return;
    for (x = 0; x < info->map_width; ++x)
        for (y = 0; y < info->map_height; ++y) scan_tile(info, x, y, 0, ops);
}

int aoe_ai_info_scan_new_tiles_ror(AoeAiInfoRor *info,
                                   const AoeAiInfoOpsRor *ops) {
    const int32_t *xy;
    int32_t total, fresh, i;
    if (!info || !ops || !ops->tile_byte5 || !ops->new_tiles) return 1;
    fresh = ops->new_tiles(ops->context, &xy, &total);
    if (fresh <= 0) return 1;
    for (i = total - fresh; i < total; ++i)
        scan_tile(info, xy[2 * i], xy[2 * i + 1], 1, ops);
    return 1;
}

/* ---- Marks, kinds and learning ---- */

void aoe_ai_info_kind_append_ror(AoeAiInfoRor *info, int32_t kind,
                                 int32_t value) {
    int32_t n, *grown;
    if (!info || !info->kind_lists || !info->kind_counts || kind < 0) return;
    n = info->kind_counts[kind];
    grown = (int32_t *)malloc((size_t)(n + 1) * sizeof(*grown));
    if (!grown) return;
    if (n) memcpy(grown, info->kind_lists[kind], (size_t)n * sizeof(*grown));
    grown[n] = value;
    free(info->kind_lists[kind]);
    info->kind_lists[kind] = grown;
    info->kind_counts[kind] = n + 1;
}

int32_t aoe_ai_info_site_priority_ror(int32_t type_id) {
    switch (type_id) {
    case 0x2d: return 7;
    case 0x32: return 8;
    case 0x44: return 2;
    case 0x45: case 0x4f: case 199: return 3;
    case 0x46: case 0x52: case 0x54: case 0x6e: case 0x114: return 6;
    case 0x48: case 0x75: case 0x9b: return 4;
    case 0x67: return 1;
    case 0x6d: return 9;
    default: return 5;
    }
}

void aoe_ai_info_put_mark_ror(AoeAiInfoRor *info, int32_t id, uint8_t x,
                              uint8_t y, uint8_t state) {
    int32_t i, free_slot = -1, n;
    AoeAiInfoMarkRor *grown;
    if (!info) return;
    n = info->mark_count;
    for (i = 0; i < n; ++i) {
        AoeAiInfoMarkRor *m = &info->marks[i];
        if (m->id == id && m->x == x && m->y == y) {
            m->state = state;
            return;
        }
        if (free_slot == -1 && m->id == -1) free_slot = i;
    }
    if (free_slot != -1) {
        info->marks[free_slot].id = id;
        info->marks[free_slot].x = x;
        info->marks[free_slot].y = y;
        info->marks[free_slot].state = state;
        return;
    }
    if (n <= 0) return;
    grown = (AoeAiInfoMarkRor *)malloc((size_t)n * 2 * sizeof(*grown));
    if (!grown) return;
    memcpy(grown, info->marks, (size_t)n * sizeof(*grown));
    for (i = n; i < n * 2; ++i) {
        memset(&grown[i], 0, sizeof grown[i]);
        grown[i].id = -1;
        grown[i].state = state;
    }
    free(info->marks);
    info->marks = grown;
    grown[n].id = id;
    grown[n].x = x;
    grown[n].y = y;
    grown[n].state = state;
    info->mark_count = n * 2;
}

int aoe_ai_info_explore_toward_ror(AoeAiInfoRor *info, int32_t id,
                                   int32_t out[2],
                                   const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor unit;
    AoeAiInfoObjectRor *best = NULL;
    int32_t i, best_d = -1;
    uint8_t zone;
    if (!info || !ops || !get_object(ops, id, &unit) || !ops->zone ||
        !ops->tile_zone)
        return 0;
    zone = ops->zone(ops->context, id);
    for (i = 0; i < info->object_count; ++i) {
        AoeAiInfoObjectRor *e = &info->objects[i];
        int32_t d;
        if (e->id == -1 || (uint16_t)e->owner == (uint16_t)ops->self || !e->owner)
            continue;
        if (aoe_ai_info_explored_ror(info, e->x, e->y, ops) == 1) continue;
        if (ops->tile_zone(ops->context, id, e->x, e->y) != zone) continue;
        d = aoe_ftol32(ops->distance ? ops->distance(ops->context, id, (float)e->x,
                                                     (float)e->y, (float)e->z)
                                     : 0.0);
        if (!best || d < best_d) {
            best = e;
            best_d = d;
        }
    }
    if (!best) return 0;
    out[0] = best->x;
    out[1] = best->y;
    if (ops->can_reach &&
        ops->can_reach(ops->context, id, out[0], out[1], aoe_ftol32(unit.z),
                       (float)((double)unit.type_28 - (double)1.0f))) {
        if (ops->grid_set) ops->grid_set(ops->context, out[0], out[1], 0);
        return 1;
    }
    if (ops->grid_set) ops->grid_set(ops->context, out[0], out[1], 1);
    return 0;
}

int aoe_ai_info_clear_of_enemy_ror(const AoeAiInfoRor *info, int32_t x,
                                   int32_t y, const AoeAiInfoOpsRor *ops) {
    int32_t i, r;
    if (!info || !ops || !ops->is_enemy) return 1;
    r = ops->sn ? ops->sn(ops->context, 0xca) : 0;
    for (i = 0; i < info->object_count; ++i) {
        const AoeAiInfoObjectRor *e = &info->objects[i];
        int32_t dx, dy;
        if (!ops->is_enemy(ops->context, e->owner)) continue;
        switch (e->type_id) {
        case 0x6d: case 0x67: case 0x44: case 0x4f: case 199: case 0x45:
            break;
        default:
            continue;
        }
        dx = (int32_t)e->x - x;
        dy = (int32_t)e->y - y;
        if (dy * dy + dx * dx <= r * r) return 0;
    }
    return 1;
}

void aoe_ai_info_load_learning_ror(AoeAiInfoRor *info,
                                   const AoeAiInfoOpsRor *ops) {
    char path[256];
    const char *name;
    uint8_t bytes[AOE_AI_INFO_STATS * 4];
    int i;
    if (!info || !ops || !ops->read_file) return;
    name = ops->learn_name ? ops->learn_name(ops->context) : NULL;
    if (name)
        snprintf(path, sizeof path, "learn\\%s.uh", name);
    else
        snprintf(path, sizeof path, "%s", "learn\\Default.uh");
    if (ops->read_file(ops->context, path, bytes, (int)sizeof bytes))
        for (i = 0; i < AOE_AI_INFO_STATS; ++i)
            info->stats[i] = aoe_le_i32(bytes + 4 * i);
}

/* A box from two corners, each clamped to the map (0x004c2d60). */
static void site_box(const AoeAiInfoRor *info, int32_t lo[2], int32_t hi[2]) {
    aoe_ai_info_clamp_ror(info, lo);
    aoe_ai_info_clamp_ror(info, hi);
}

static int resource_class_matches(int32_t need, int16_t cls) {
    switch (need) {
    case -1: return 1;
    case 3: return cls == 0x20;
    case 2: return cls == 8;
    case 1: return cls == 0xf;
    case 0: return cls != 0x20 && cls != 8 && cls != 0xf;
    default: return 0;
    }
}

void aoe_ai_info_prepare_sites_ror(AoeAiInfoRor *info, AoeValueGridRor *g, int32_t mode,
                                   int32_t type, const int32_t lo_in[2], const int32_t hi_in[2],
                                   const AoeAiInfoOpsRor *ops) {
    int32_t w = info->map_width, h = info->map_height, x, y, i, k;
    aoe_value_grid_fill_ror(g, 0x14);
    aoe_value_grid_set_cap_ror(g, 0xff);
    if (mode == 3 || mode == 4 || mode == 0)
        for (x = 0; x < w; ++x)
            for (y = 0; y < h; ++y)
                if (x < 5 || x > w - 5 || y < 5 || y > h - 5)
                    aoe_value_grid_lower_ror(g, x, y, 10);
    if ((mode == 1 || mode == 2 || mode == 9) && ops && ops->object) {
        int32_t need = aoe_ai_info_resource_need_ror(info, mode, ops);
        for (k = 0; k < 4; ++k)
            for (i = 0; i < info->target_count[k]; ++i) {
                const AoeAiInfoTargetRor *t = &info->targets[k][i];
                AoeAiWorldObjectRor o;
                int32_t lo[2], hi[2], c[2];
                memset(&o, 0, sizeof o);
                if (t->gone == 1 || !ops->object(ops->context, t->id, &o)) continue;
                lo[0] = aoe_ftol32((double)(o.x - o.size_x) - 1.0);
                lo[1] = aoe_ftol32((double)(o.y - o.size_y) - 1.0);
                aoe_ai_info_clamp_ror(info, lo);
                hi[0] = aoe_ftol32((double)(o.size_y + o.x) + 1.0);
                hi[1] = aoe_ftol32((double)(o.size_y + o.y) + 1.0);
                aoe_ai_info_clamp_ror(info, hi);
                c[0] = aoe_ftol32(o.x);
                c[1] = aoe_ftol32(o.y);
                aoe_ai_info_clamp_ror(info, c);
                if (mode == 1 || mode == 2) aoe_value_grid_set_ror(g, c[0], c[1], 0xff);
                else aoe_value_grid_set_box_ror(g, lo[0], lo[1], hi[0], hi[1], 0xff);
                if (mode == 1) {
                    if (o.type_class == 7) {
                        lo[0] -= 5, lo[1] -= 5, hi[0] += 5, hi[1] += 5;
                        site_box(info, lo, hi);
                        aoe_value_grid_lower_box_ror(g, lo[0], lo[1], hi[0], hi[1], 0x80);
                    } else if (resource_class_matches(need, o.type_class)) {
                        static const int32_t grow[3] = {1, 3, 5};
                        static const uint8_t add[3] = {8, 4, 2};
                        for (int s = 0; s < 3; ++s) {
                            lo[0] -= grow[s], lo[1] -= grow[s], hi[0] += grow[s], hi[1] += grow[s];
                            site_box(info, lo, hi);
                            aoe_value_grid_raise_box_ror(g, lo[0], lo[1], hi[0], hi[1], add[s]);
                        }
                    }
                } else if (mode == 2 && o.type_class == 7) {
                    lo[0] -= 3, lo[1] -= 3, hi[0] += 3, hi[1] += 3;
                    site_box(info, lo, hi);
                    aoe_value_grid_raise_box_ror(g, lo[0], lo[1], hi[0], hi[1], 0x80);
                }
            }
    }
    if ((mode == 1 || mode == 2 || mode == 3 || mode == 4 || mode == 0 || mode == 5 ||
         mode == 6 || mode == 8) && ops && ops->object) {
        for (i = 0; i < info->list_190.count; ++i) {
            AoeAiWorldObjectRor o;
            int32_t lo[2], hi[2], prio, fx, fy, d;
            int center_grow = 0;
            memset(&o, 0, sizeof o);
            if (!ops->object(ops->context, info->list_190.data[i], &o)) continue;
            prio = aoe_ai_info_site_priority_ror(o.type_id);
            fx = aoe_ftol32(o.x);
            fy = aoe_ftol32(o.y);
            if ((double)o.size_x == 2.5 || ((double)o.size_x == 1.5 && prio != 6 && type == 0x46)) {
                lo[0] = fx - 3, lo[1] = fy - 3, hi[0] = fx + 3, hi[1] = fy + 3;
                center_grow = 1;
            } else if ((double)o.size_x == 1.5) {
                if (prio == 6) {
                    lo[0] = fx - 1, lo[1] = fy - 1, hi[0] = fx + 1, hi[1] = fy + 1;
                } else {
                    lo[0] = fx - 2, lo[1] = fy - 2, hi[0] = fx + 2, hi[1] = fy + 2;
                    center_grow = 1;
                }
            } else if (o.size_x == 1.0f) {
                if (prio == 6 || prio == 3) lo[0] = fx - 1, lo[1] = fy - 1, hi[0] = fx, hi[1] = fy;
                else lo[0] = fx - 2, lo[1] = fy - 2, hi[0] = fx + 1, hi[1] = fy + 1;
                center_grow = 1;
            } else if ((double)o.size_x == 0.5) {
                if (prio == 6 || prio == 4) lo[0] = fx, lo[1] = fy, hi[0] = fx, hi[1] = fy;
                else lo[0] = fx - 1, lo[1] = fy - 1, hi[0] = fx + 1, hi[1] = fy + 1;
                center_grow = 1;
            } else {
                continue; /* "Unsupported building size" logged */
            }
            if (center_grow && prio == 0x6d) lo[0]--, lo[1]--, hi[0]++, hi[1]++;
            site_box(info, lo, hi);
            aoe_value_grid_set_box_ror(g, lo[0], lo[1], hi[0], hi[1], 0xff);
            if (mode == 3 || mode == 0) {
                double r = o.type_28;
                if (mode != 0) r *= 0.5;
                if (prio == 0x6d) r *= 0.5;
                d = aoe_ftol32(r);
                lo[0] -= d, lo[1] -= d, hi[0] += d, hi[1] += d;
                site_box(info, lo, hi);
                if (mode == 0 && prio == 3)
                    aoe_value_grid_raise_box_ror(g, lo[0], lo[1], hi[0], hi[1], 10);
                else
                    aoe_value_grid_lower_box_ror(g, lo[0], lo[1], hi[0], hi[1], 10);
            } else if (mode == 1 || mode == 2) {
                double r = o.type_28;
                if (prio == 0x6d) r *= 0.5;
                else if (prio == 0x44 || prio == 0x67) r = 5.0;
                d = aoe_ftol32(r);
                lo[0] -= d, lo[1] -= d, hi[0] += d, hi[1] += d;
                site_box(info, lo, hi);
                aoe_value_grid_lower_box_ror(g, lo[0], lo[1], hi[0], hi[1], 10);
            } else if (mode == 5) {
                double r = o.type_28;
                if (prio == 0x6d) r *= 0.25;
                d = aoe_ftol32(r) * 2;
                lo[0] -= d, lo[1] -= d, hi[0] += d, hi[1] += d;
                site_box(info, lo, hi);
                aoe_value_grid_lower_box_ror(g, lo[0], lo[1], hi[0], hi[1], 0x14);
            } else if (mode == 8) {
                int32_t base_lo[2] = {lo[0], lo[1]}, base_hi[2] = {hi[0], hi[1]};
                int32_t full = aoe_ftol32(o.type_28);
                const int32_t steps[3] = {full, full / 2, full / 3};
                for (int s = 0; s < 3; ++s) {
                    int32_t a[2] = {base_lo[0] - steps[s], base_lo[1] - steps[s]};
                    int32_t b[2] = {base_hi[0] + steps[s], base_hi[1] + steps[s]};
                    site_box(info, a, b);
                    aoe_value_grid_raise_box_ror(g, a[0], a[1], b[0], b[1], 0x14);
                }
            }
        }
    }
    if ((mode == 3 || mode == 4 || mode == 0) && lo_in && hi_in) {
        for (x = lo_in[0]; x <= hi_in[0]; ++x)
            for (y = lo_in[1]; y <= hi_in[1]; ++y) {
                if (ops && ops->terrain && ops->terrain(ops->context, x, y) == 1) {
                    int32_t a[2] = {x - 2, y - 2}, b[2] = {x - 2, y - 2};
                    site_box(info, a, b);
                    aoe_value_grid_set_ror(g, x, y, 0xff);
                    aoe_value_grid_lower_box_ror(g, a[0], a[1], b[0], b[1], 10);
                }
                if (ops && ops->tile_byte5)
                    aoe_value_grid_raise_ror(g, x, y,
                                             (uint8_t)((ops->tile_byte5(ops->context, x, y) >> 5) * 5));
            }
    }
    if (mode == 3 || mode == 4 || mode == 0 || mode == 6 || mode == 5) {
        for (i = 0; i < info->area_count; ++i) {
            const AoeAiInfoAreaRor *a = &info->areas[i];
            int32_t lo[2], hi[2];
            if (a->id == -1 || !ops || a->player_a != (uint8_t)ops->self) continue;
            lo[0] = a->x0 - 4, lo[1] = a->y0 - 4;
            aoe_ai_info_clamp_ror(info, lo);
            hi[0] = a->x1 + 4, hi[1] = a->y1 + 4;
            aoe_ai_info_clamp_ror(info, hi);
            aoe_value_grid_raise_box_ror(g, lo[0], lo[1], hi[0], hi[1], 0x10);
        }
    }
    if (mode == 0 && ops && ops->tactical_sites) ops->tactical_sites(ops->context, g);
    {
        const int32_t from[2] = {0, 0}, to[2] = {w, h};
        aoe_value_grid_count_ror(g, from, to, 0xff, 1, mode == 4 ? 0 : 1, 1);
    }
    if (mode != 0)
        for (i = 0; i < info->mark_count; ++i) {
            const AoeAiInfoMarkRor *m = &info->marks[i];
            if (m->id == type && m->state == 2) aoe_value_grid_set_ror(g, m->x, m->y, 0xff);
        }
}

static void save_search(AoeAiSiteSearchRor *s, int32_t type, float size, const char *name,
                        int32_t builder, int32_t min, int32_t max, int32_t bx, int32_t by,
                        int32_t best, int32_t cx, int32_t jitter, int has_item) {
    s->type = type;
    s->size = size;
    snprintf(s->name, sizeof s->name, "%s", name ? name : "");
    s->builder = builder;
    s->min = min;
    s->max = max;
    s->best_x = bx;
    s->best_y = by;
    s->state = 1;
    s->best = best;
    s->cursor_x = cx;
    s->jitter = jitter;
    s->has_item = has_item;
}

static AoeAiConItemRor *new_site_item(float x, float y, float size, int32_t type, const char *name) {
    AoeAiConItemRor *it = malloc(sizeof *it);
    if (!it) return NULL;
    aoe_ai_con_item_init_ror(it);
    aoe_ai_con_item_set_ror(it, x, y, 1.0f, size, size, 1.0f, type, name);
    return it;
}

static int32_t iabs32(int32_t v) { return v < 0 ? -v : v; }

AoeAiConItemRor *aoe_ai_info_find_site_ror(AoeAiInfoRor *info, AoeValueGridRor *g,
                                           const AoeAiPlanItemRor *item, int32_t builder,
                                           int32_t type, float size, const char *name_in,
                                           int32_t min, int32_t max, int32_t px, int32_t py,
                                           AoeAiSiteSearchRor *s, uint32_t start, int *found,
                                           const AoeAiInfoOpsRor *ops) {
    char name[256];
    int32_t size_i, floor_score, prio, cx, cy, outer_lo[2], outer_hi[2], inner_lo[2], inner_hi[2];
    int32_t best, bx, by, x, jitter, has_item, resuming = s->state == 1;
    AoeAiWorldObjectRor tc, b;
    int have_tc, have_b, first = 1, in_box_rule;
    int32_t class4 = ops->first_of_class ? ops->first_of_class(ops->context, 4) : 0;
    if (found) *found = 0;
    if (!class4) {
        s->state = 0;
        return NULL;
    }
    if (resuming) {
        type = s->type;
        size = s->size;
        snprintf(name, sizeof name, "%s", s->name);
    } else if (item) {
        type = item->type;
        size = item->size_x;
        snprintf(name, sizeof name, "%s", item->name);
    } else {
        snprintf(name, sizeof name, "%s", name_in ? name_in : "");
    }
    size_i = aoe_ftol32(size);
    floor_score = size_i * size_i * 20;
    if (!ops->has_template || !ops->has_template(ops->context, type)) {
        s->state = 0;
        return NULL;
    }
    memset(&tc, 0, sizeof tc);
    memset(&b, 0, sizeof b);
    {
        int32_t tc_id = ops->first_of_type ? ops->first_of_type(ops->context, 0x6d) : -1;
        have_tc = tc_id != -1 && ops->object && ops->object(ops->context, tc_id, &tc);
        have_b = ops->object && ops->object(ops->context, resuming ? s->builder : builder, &b);
    }
    has_item = resuming ? s->has_item : item != NULL;
    if (!has_item && !resuming) {
        cx = px;
        cy = py;
    } else if (have_tc) {
        cx = aoe_ftol32(tc.x);
        cy = aoe_ftol32(tc.y);
    } else {
        if (!have_b) ops->object(ops->context, class4, &b);
        cx = aoe_ftol32(b.x);
        cy = aoe_ftol32(b.y);
    }
    prio = aoe_ai_info_site_priority_ror(type);
    if (prio == -1) { /* "Invalid influence placement type" */
        s->state = 0;
        return NULL;
    }
    if (has_item || resuming) {
        int32_t (*sn)(void *, int32_t) = ops->sn;
        if (prio == 1) {
            int32_t age = 1;
            if (ops->age_reached && ops->age_reached(ops->context, 2)) age = 2;
            else if (ops->age_reached && ops->age_reached(ops->context, 3)) age = 3;
            else if (ops->age_reached && ops->age_reached(ops->context, 4)) age = 4;
            min = 7;
            max = (sn ? sn(ops->context, 0x56) : 0) * age;
        } else if (prio == 2) {
            min = 4;
            max = sn ? sn(ops->context, 0x57) : 0;
        } else {
            min = (prio == 3 || prio == 4) ? (sn ? sn(ops->context, 0x49) : 0) : 4;
            max = sn ? sn(ops->context, 0x4a) : 0;
        }
    }
    if (max <= min) {
        s->state = 0;
        return NULL;
    }
    outer_lo[0] = cx - max, outer_lo[1] = cy - max;
    aoe_ai_info_clamp_ror(info, outer_lo);
    outer_hi[0] = max + cx, outer_hi[1] = max + cy;
    aoe_ai_info_clamp_ror(info, outer_hi);
    inner_lo[0] = cx - min, inner_lo[1] = cy - min;
    aoe_ai_info_clamp_ror(info, inner_lo);
    inner_hi[0] = min + cx, inner_hi[1] = min + cy;
    aoe_ai_info_clamp_ror(info, inner_hi);
    if (item && !resuming) {
        AoeAiInfoMarkRor *m = aoe_ai_info_open_mark_ror(info, type);
        if (m) {
            AoeAiConItemRor *made;
            if (prio == 4) {
                info->prev_site[0] = info->last_site[0], info->prev_site[1] = info->last_site[1];
                info->last_site[0] = m->x, info->last_site[1] = m->y;
            }
            made = size == 2.0f ? new_site_item((float)m->x, (float)m->y, size, type, name)
                                : new_site_item((float)m->x + 0.5f, (float)m->y + 0.5f, size, type,
                                                name);
            m->state = 1;
            s->state = 0;
            return made;
        }
    }
    if (!resuming) aoe_ai_info_prepare_sites_ror(info, g, prio, type, outer_lo, outer_hi, ops);
    if (!resuming) {
        int32_t n = ops->sn ? ops->sn(ops->context, 0xa8) : 0;
        jitter = n >= 1 && ops->rand ? ops->rand(ops->context) % n : 0;
        bx = by = -1;
        best = -1;
        x = outer_lo[0];
    } else {
        jitter = s->jitter;
        bx = s->best_x;
        by = s->best_y;
        best = s->best;
        x = s->cursor_x;
    }
    if (!resuming && ops->now_ms && ops->time_slice <= ops->now_ms(ops->context) - start) {
        save_search(s, type, size, name, builder, min, max, bx, by, best, x, jitter, has_item);
        return NULL;
    }
    in_box_rule = prio == 4 || prio == 3;
    for (; x <= outer_hi[0]; ++x) {
        int32_t y;
        if (!first) {
            if (ops->now_ms && ops->time_slice <= ops->now_ms(ops->context) - start) {
                save_search(s, type, size, name, builder, min, max, bx, by, best, x, jitter,
                            has_item);
                return NULL;
            }
        } else {
            first = 0;
        }
        for (y = outer_lo[1]; y <= outer_hi[1]; ++y) {
            int32_t lo[2], hi[2], score = 0, cell;
            int blocked = 0;
            if (in_box_rule && x >= inner_lo[0] && x <= inner_hi[0] && y >= inner_lo[1] &&
                y <= inner_hi[1])
                continue;
            if (size == 5.0f || (size == 3.0f && prio != 6 && prio != 3 && prio != 4 &&
                                 prio != 1 && prio != 2 && prio != 8)) {
                lo[0] = x - 2, lo[1] = y - 2, hi[0] = x + 2, hi[1] = y + 2;
            } else if (size == 3.0f) {
                lo[0] = x - 1, lo[1] = y - 1, hi[0] = x + 1, hi[1] = y + 1;
            } else if (size == 2.0f) {
                if (prio == 6 || prio == 3 || prio == 4)
                    lo[0] = x - 1, lo[1] = y - 1, hi[0] = x, hi[1] = y;
                else
                    lo[0] = x - 2, lo[1] = y - 2, hi[0] = x + 1, hi[1] = y + 1;
            } else if (size == 1.0f) {
                if (prio == 6 || prio == 3 || prio == 4)
                    lo[0] = x, lo[1] = y, hi[0] = x, hi[1] = y;
                else
                    lo[0] = x - 1, lo[1] = y - 1, hi[0] = x + 1, hi[1] = y + 1;
            } else { /* "Unsupported building size" */
                s->state = 0;
                return NULL;
            }
            if (aoe_ai_info_clamp_ror(info, lo) == 1 || aoe_ai_info_clamp_ror(info, hi) == 1)
                continue;
            if (ops->placement_refused &&
                ops->placement_refused(ops->context, type, (float)x + 0.5f, (float)y + 0.5f))
                continue;
            for (int32_t a = lo[0]; a <= hi[0] && !blocked; ++a)
                for (int32_t c = lo[1]; c <= hi[1] && !blocked; ++c) {
                    cell = aoe_value_grid_get_ror(g, a, c);
                    if (cell == 0xff) blocked = 1;
                    else score += cell;
                }
            if (blocked) continue;
            if (prio == 4 || prio == 6) {
                int32_t before = g->count;
                int32_t open = aoe_value_grid_score_ror(g, lo, hi, 0xff, 0xff, 1, 0, 1) - before;
                if (prio == 4 && open > 6) continue;
                score *= open;
            }
            if (prio == 4) {
                int32_t cand[2] = {x, y}, dir;
                if (info->last_site[0] != -1 && !aoe_ai_info_tiles_near_ror(info->last_site, cand, 0))
                    score /= 2;
                dir = aoe_ai_info_direction_ror(info->last_site, cand);
                if (dir % 2 == 1) score *= 3;
                if (aoe_ai_info_direction_ror(info->prev_site, info->last_site) == dir) score *= 3;
                {
                    int32_t dy = iabs32(info->last_site[1] - cy), dx = iabs32(info->last_site[0] - cx);
                    if (dy < dx) {
                        if (dir == 1 || dir == 5) score *= 3;
                    } else if (dx < dy && (dir == 3 || dir == 7)) {
                        score *= 3;
                    }
                }
            } else {
                if (prio == 1 || prio == 2) {
                    if (aoe_ai_info_mark_near_ror(info, type, (uint8_t)x, (uint8_t)y, 10) == 1)
                        score /= 10;
                } else if (prio != 6 &&
                           aoe_ai_info_mark_near_ror(info, type, (uint8_t)x, (uint8_t)y, size_i) == 1) {
                    score /= 10;
                }
                if (prio == 1 || prio == 2) {
                    int32_t d2 = (y - cy) * (y - cy) + (x - cx) * (x - cx);
                    int32_t d = aoe_ftol32(sqrt((double)d2)) - 10;
                    if (d > 0 && score - d / 5 > floor_score) score -= d / 5;
                }
            }
            if (best == -1 || best < score + jitter) {
                if (score <= best) jitter /= 2;
                bx = x;
                by = y;
                best = score;
            }
        }
    }
    if (best == -1 || (best <= floor_score && (prio == 2 || prio == 1))) {
        if (prio == 4) {
            info->prev_site[0] = info->last_site[0], info->prev_site[1] = info->last_site[1];
            info->last_site[0] = info->last_site[1] = -1;
        }
        s->state = 0;
        return NULL;
    }
    if (prio == 4) {
        info->prev_site[0] = info->last_site[0], info->prev_site[1] = info->last_site[1];
        info->last_site[0] = bx, info->last_site[1] = by;
    }
    if (!has_item && !resuming) {
        aoe_ai_info_put_mark_ror(info, type, (uint8_t)bx, (uint8_t)by, 0);
        s->state = 0;
        if (found) *found = 1;
        return NULL;
    }
    {
        AoeAiConItemRor *made = size == 2.0f
                                    ? new_site_item((float)bx, (float)by, size, type, name)
                                    : new_site_item((float)bx + 0.5f, (float)by + 0.5f, size, type,
                                                    name);
        aoe_ai_info_put_mark_ror(info, type, (uint8_t)bx, (uint8_t)by, 1);
        s->state = 0;
        if (found) *found = made != NULL;
        return made;
    }
}

AoeAiConItemRor *aoe_ai_info_shore_site_ror(AoeAiInfoRor *info, AoeAiConPlanRor *plan,
                                            const AoeAiPlanItemRor *item, float ox, float oy,
                                            int32_t terrain, uint32_t value,
                                            const AoeAiInfoOpsRor *ops) {
    uint8_t docks[256];
    int32_t w, h, x, y, bx = -1, by = -1, jitter;
    float best = -1.0f;
    AoeAiConItemRor *made;
    memset(docks, 0xff, sizeof docks);
    if (!ops->map_width || !ops->map_height) return NULL;
    w = ops->map_width(ops->context);
    h = ops->map_height(ops->context);
    jitter = ops->rand ? ops->rand(ops->context) % 50 : 0;
    for (x = 1; x < w - 2; ++x)
        for (y = 1; y < h - 2; ++y) {
            uint8_t zone;
            float score;
            int32_t near;
            if (!ops->vision || !ops->vision(ops->context, x, y)) continue;
            if (x + 1 > w - 1 || y + 1 > h - 1) continue;
            if (terrain != -1 &&
                (!ops->tile_byte5 || (int32_t)(ops->tile_byte5(ops->context, x, y) & 0x1f) != terrain))
                continue;
            zone = ops->map_zone ? ops->map_zone(ops->context, x, y) : 0;
            if (!ops->zone_size || ops->zone_size(ops->context, zone) < (ops->sn ? ops->sn(ops->context, 0x70) : 0))
                continue;
            if (!aoe_ai_info_neighbour_terrain_ror(x, y, value, ops)) continue;
            if (plan && aoe_ai_con_plan_placed_at_ror(plan, (float)x + 0.5f, (float)y + 0.5f)) continue;
            {
                float dx = (float)x - ox, dy = (float)y - oy;
                score = dx * dx + dy * dy;
            }
            if (docks[zone] == 0xff) docks[zone] = (uint8_t)aoe_ai_info_docks_in_zone_ror(info, zone, ops);
            if (docks[zone]) score = (float)(docks[zone] * 1000) + score;
            near = aoe_ai_info_nearest_dock_ror(info, x, y, ops);
            if (near != -1) score = (float)(100000 / near) + score;
            if (bx != -1 && (float)jitter + best <= score) continue;
            bx = x;
            by = y;
            best = score;
        }
    if (bx == -1) return NULL;
    made = malloc(sizeof *made);
    if (!made) return NULL;
    aoe_ai_con_item_init_ror(made);
    aoe_ai_con_item_set_ror(made, (float)bx + 0.5f, (float)by + 0.5f, 0.0f, item->size_x,
                            item->size_y, item->size_z, item->type, item->name);
    return made;
}

AoeAiConItemRor *aoe_ai_ror_plan_add_shore_ror(AoeAiConPlanRor *plan, AoeAiInfoRor *info,
                                               const AoeAiPlanItemRor *item,
                                               const AoeAiInfoOpsRor *ops) {
    AoeAiConItemRor *it =
        aoe_ai_info_shore_site_ror(info, plan, item, plan->origin_x, plan->origin_y, 1, 2, ops);
    if (!it) {
        plan->field_310 = 5;
        return NULL;
    }
    it->next = &plan->placed;
    it->prev = plan->placed.prev;
    plan->placed.prev->next = it;
    plan->placed.prev = it;
    plan->placed_count++;
    plan->field_310 = 0;
    return it;
}

static AoeAiConItemRor *copy_con_item(const AoeAiConItemRor *from) {
    AoeAiConItemRor *it = malloc(sizeof *it);
    if (!it) return NULL;
    aoe_ai_con_item_init_ror(it);
    aoe_ai_con_item_copy_ror(it, from);
    return it;
}

static AoeAiConItemRor *ask_finder(AoeAiInfoRor *info, AoeValueGridRor *g,
                                   const AoeAiPlanItemRor *item, int32_t builder,
                                   AoeAiSiteSearchRor *search, uint32_t start,
                                   const AoeAiInfoOpsRor *ops) {
    return aoe_ai_info_find_site_ror(info, g, item, builder, -1, 0.0f, NULL, -1, -1, -1, -1,
                                     search, start, NULL, ops);
}

AoeAiConItemRor *aoe_ai_ror_plan_choose_ror(AoeAiConPlanRor *plan, AoeAiInfoRor *info,
                                            AoeValueGridRor *g, const AoeAiPlanItemRor *item,
                                            int32_t builder, int free_placement,
                                            AoeAiSiteSearchRor *search, uint32_t start,
                                            const AoeAiInfoOpsRor *ops) {
    const AoeAiConItemRor *cands[256];
    int32_t n = 0, least = -1, class4, i;
    uint8_t zone = 0;
    int have_builder;
    AoeAiWorldObjectRor b;
    const AoeAiConItemRor *chosen;
    AoeAiConItemRor *e, *result;
    if (!(plan->origin_x > 0.0f && plan->origin_y > 0.0f && (float)plan->map_width > plan->origin_x &&
          (float)plan->map_height > plan->origin_y))
        return ask_finder(info, g, item, builder, search, start, ops);
    if (item->type == 0x2d) {
        AoeAiConItemRor *dock = aoe_ai_ror_plan_add_shore_ror(plan, info, item, ops);
        return dock ? copy_con_item(dock) : NULL;
    }
    switch (item->type) {
    case 0x67: case 0x44: case 0x4f: case 199: case 0x45: case 0x48: case 0x75: case 0x9b:
        return ask_finder(info, g, item, builder, search, start, ops);
    default:
        break;
    }
    if (free_placement || !plan->count) return ask_finder(info, g, item, builder, search, start, ops);
    class4 = ops->first_of_class ? ops->first_of_class(ops->context, 4) : 0;
    if (!class4) return NULL;
    memset(&b, 0, sizeof b);
    have_builder = ops->object && ops->object(ops->context, builder, &b);
    if (have_builder && ops->zone) zone = ops->zone(ops->context, builder);
    for (const AoeAiConItemRor *c = plan->plan.next; c && c != &plan->plan; c = c->next) {
        int32_t lx, ly, hx, hy, x, y, ok = 1;
        if (c->plan.type != item->type && c->plan.type != -1) continue;
        if (item->size_x != c->plan.size_x || item->size_y != c->plan.size_y) continue;
        if (c->field_78 == 1 || c->field_74 == 1) continue;
        lx = aoe_ftol32(c->plan.x - c->plan.size_x);
        ly = aoe_ftol32(c->plan.y - c->plan.size_y);
        hx = aoe_ftol32(c->plan.size_x) + lx;
        hy = aoe_ftol32(c->plan.size_y) + ly;
        if (lx < 0 || ly < 0 || hx >= plan->map_width || hy >= plan->map_height) continue;
        for (x = lx; x < hx && ok; ++x)
            for (y = ly; y < hy && ok; ++y) {
                if (have_builder && ops->tile_zone && ops->tile_zone(ops->context, builder, x, y) != zone)
                    ok = 0;
                else if (!ops->free_tile ||
                         !ops->free_tile(ops->context, class4, (float)x + 0.5f, (float)y + 0.5f))
                    ok = 0;
            }
        if (!ok) continue;
        if (n < (int32_t)(sizeof cands / sizeof cands[0])) cands[n++] = c;
        if (least == -1 || c->counter < least) least = c->counter;
    }
    if (least >= 2 || least == -1) {
        AoeAiConItemRor *found = ask_finder(info, g, item, builder, search, start, ops);
        if (found) return found;
    }
    if (!n) {
        plan->field_310 = 5;
        return NULL;
    }
    chosen = cands[0];
    for (i = 1; i < n; ++i) {
        if (item->type == 0x48) {
            if (chosen->counter > cands[i]->counter + 5) chosen = cands[i];
        } else if (chosen->counter > cands[i]->counter) {
            chosen = cands[i];
        }
    }
    e = aoe_ai_con_plan_at_ror(plan, chosen->plan.x, chosen->plan.y);
    if (!e) return NULL; /* host guard: the original uses it unchecked */
    e->field_74 = 1;
    aoe_ai_con_item_add_ror(e);
    e->plan.unique_id = item->unique_id;
    plan->field_310 = 0;
    result = copy_con_item(e);
    if (result) result->plan.type = item->type;
    return result;
}

/* Lifecycle, save and load. */

static const char default_learn[] = "learn\\Default.uh";

static void put_object(AoeOutcomeStream *s, const AoeAiInfoObjectRor *o) {
    uint8_t b[0x24] = {0};
    aoe_le_put_u32(b, (uint32_t)o->id);
    aoe_le_put_u16(b + 4, (uint16_t)o->type_id);
    aoe_le_put_u16(b + 6, (uint16_t)o->type_class);
    b[8] = o->x; b[9] = o->y; b[10] = o->z; b[11] = o->owner;
    aoe_le_put_u16(b + 0xc, o->hp);
    aoe_le_put_u32(b + 0x10, (uint32_t)o->field_10);
    b[0x14] = o->field_14;
    aoe_le_put_f32(b + 0x18, o->field_18);
    aoe_le_put_f32(b + 0x1c, o->field_1c);
    aoe_le_put_f32(b + 0x20, o->range);
    aoe_stream_put(s, b, sizeof b);
}

static void take_object(AoeOutcomeStream *s, AoeAiInfoObjectRor *o) {
    const uint8_t *b = aoe_stream_take(s, 0x24);
    if (!b) return;
    o->id = aoe_le_i32(b);
    o->type_id = (int16_t)aoe_le_u16(b + 4);
    o->type_class = (int16_t)aoe_le_u16(b + 6);
    o->x = b[8]; o->y = b[9]; o->z = b[10]; o->owner = b[11];
    o->hp = aoe_le_u16(b + 0xc);
    o->field_10 = aoe_le_i32(b + 0x10);
    o->field_14 = b[0x14];
    o->field_18 = aoe_le_f32(b + 0x18);
    o->field_1c = aoe_le_f32(b + 0x1c);
    o->range = aoe_le_f32(b + 0x20);
}

static void put_mark(AoeOutcomeStream *s, const AoeAiInfoMarkRor *m) {
    aoe_stream_put_i32(s, m->id);
    aoe_stream_put_u8(s, m->state);
    aoe_stream_put_u8(s, m->x);
    aoe_stream_put_u8(s, m->y);
    aoe_stream_put_u8(s, 0);
}

static void take_mark(AoeOutcomeStream *s, AoeAiInfoMarkRor *m) {
    m->id = aoe_stream_i32(s);
    m->state = aoe_stream_u8(s);
    m->x = aoe_stream_u8(s);
    m->y = aoe_stream_u8(s);
    m->pad = aoe_stream_u8(s);
}

static void put_area(AoeOutcomeStream *s, const AoeAiInfoAreaRor *a) {
    uint8_t b[0x18] = {0};
    aoe_le_put_u32(b, (uint32_t)a->id);
    b[4] = a->field_4; b[5] = a->x0; b[6] = a->y0; b[7] = a->x1; b[8] = a->y1;
    b[9] = a->player_9; b[10] = a->player_a;
    aoe_le_put_u16(b + 0xc, a->field_c);
    b[0xe] = a->field_e;
    aoe_le_put_u32(b + 0x10, a->age);
    aoe_le_put_u32(b + 0x14, (uint32_t)a->field_14);
    aoe_stream_put(s, b, sizeof b);
}

static void take_area(AoeOutcomeStream *s, AoeAiInfoAreaRor *a) {
    const uint8_t *b = aoe_stream_take(s, 0x18);
    if (!b) return;
    a->id = aoe_le_i32(b);
    a->field_4 = b[4]; a->x0 = b[5]; a->y0 = b[6]; a->x1 = b[7]; a->y1 = b[8];
    a->player_9 = b[9]; a->player_a = b[10]; a->pad_b = 0;
    a->field_c = aoe_le_u16(b + 0xc);
    a->field_e = b[0xe]; a->pad_f = 0;
    a->age = aoe_le_u32(b + 0x10);
    a->field_14 = aoe_le_i32(b + 0x14);
}

static void put_target(AoeOutcomeStream *s, const AoeAiInfoTargetRor *t) {
    uint8_t b[0x14] = {0};
    aoe_le_put_u32(b, (uint32_t)t->id);
    b[4] = t->x; b[5] = t->y; b[6] = t->picks;
    aoe_le_put_u32(b + 8, (uint32_t)t->field_8);
    b[0xc] = t->wait; b[0xd] = t->gone; b[0xe] = t->field_e;
    aoe_le_put_u32(b + 0x10, (uint32_t)t->field_10);
    aoe_stream_put(s, b, sizeof b);
}

static void take_target(AoeOutcomeStream *s, AoeAiInfoTargetRor *t) {
    const uint8_t *b = aoe_stream_take(s, 0x14);
    if (!b) return;
    memset(t, 0, sizeof *t);
    t->id = aoe_le_i32(b);
    t->x = b[4]; t->y = b[5]; t->picks = b[6];
    t->field_8 = aoe_le_i32(b + 8);
    t->wait = b[0xc]; t->gone = b[0xd]; t->field_e = b[0xe];
    t->field_10 = aoe_le_i32(b + 0x10);
}

/* Count, then each value (growing a short list as 0x004cc5c0 does). */
static void put_list(AoeOutcomeStream *s, AoeAiIntListRor *l) {
    int32_t i, *v;
    aoe_stream_put_i32(s, l->count);
    for (i = 0; i < l->count; ++i) {
        v = aoe_ai_int_list_at_ror(l, i);
        aoe_stream_put_i32(s, v ? *v : 0);
    }
}

static int take_list(AoeOutcomeStream *s, AoeAiIntListRor *l) {
    int32_t n = aoe_stream_i32(s), i;
    for (i = 0; i < n && !s->failed; ++i)
        aoe_ai_int_list_add_unique_ror(l, aoe_stream_i32(s));
    return !s->failed;
}

static void free_list(AoeAiIntListRor *l) {
    free(l->data);
    memset(l, 0, sizeof *l);
}

/* The state shared by both constructors. */
static int init_common(AoeAiInfoRor *info) {
    int i, j;
    memset(info, 0, sizeof *info);
    snprintf(info->learn_path, sizeof info->learn_path, "%s", default_learn);
    info->kind_lists = (int32_t **)calloc(4, sizeof *info->kind_lists);
    info->kind_counts = (int32_t *)calloc(4, sizeof *info->kind_counts);
    if (!info->kind_lists || !info->kind_counts) return 0;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) info->age_allowed[i][j] = i ? 1 : 0;
        info->best_distance[i] = -1;
        info->current_target[i] = -1;
    }
    return aoe_value_grid_init_ror(&info->explored, 0xfe, 0xfe, 0xff);
}

int aoe_ai_info_init_ror(AoeAiInfoRor *info) {
    int32_t i;
    if (!info || !init_common(info)) return 0;
    info->map_width = info->map_height = -1;
    info->object_count = 500;
    info->objects = (AoeAiInfoObjectRor *)calloc(500, sizeof *info->objects);
    info->mark_count = 200;
    info->marks = (AoeAiInfoMarkRor *)calloc(200, sizeof *info->marks);
    info->area_count = 25;
    info->areas = (AoeAiInfoAreaRor *)calloc(25, sizeof *info->areas);
    if (!info->objects || !info->marks || !info->areas) return 0;
    for (i = 0; i < 500; ++i)
        info->objects[i].id = info->objects[i].type_id =
            info->objects[i].type_class = -1;
    for (i = 0; i < 200; ++i) info->marks[i].id = -1;
    for (i = 0; i < 25; ++i) {
        info->areas[i].id = -1;
        info->areas[i].player_9 = info->areas[i].player_a = 0xff;
        info->areas[i].field_14 = -1;
    }
    info->last_site[0] = info->last_site[1] = -1;
    info->prev_site[0] = info->prev_site[1] = -1;
    info->field_1b0 = -1;
    for (i = 0; i < 4; ++i)
        info->pairs_1b4[i][0] = info->pairs_1b4[i][1] =
            info->pairs_1d4[i][0] = info->pairs_1d4[i][1] = -1;
    return 1;
}

void aoe_ai_info_set_owner_ror(AoeAiInfoRor *info, void *owner) {
    if (info) info->owner = owner;
}

int aoe_ai_info_setup_ror(AoeAiInfoRor *info, const AoeAiInfoOpsRor *ops) {
    const char *name;
    if (!info || !ops) return 0;
    if (ops->map_width) info->map_width = ops->map_width(ops->context);
    if (ops->map_height) info->map_height = ops->map_height(ops->context);
    name = ops->learn_name ? ops->learn_name(ops->context) : NULL;
    if (name)
        snprintf(info->learn_path, sizeof info->learn_path, "learn\\%s.uh",
                 name);
    else
        snprintf(info->learn_path, sizeof info->learn_path, "%s",
                 default_learn);
    return 1;
}

static void write_area_file(AoeAiInfoRor *info, const AoeAiInfoOpsRor *ops) {
    AoeOutcomeStream s;
    size_t size = 16 + (size_t)info->area_count * 0x18 +
                  (size_t)info->object_count * 0x24 + 8 * 2 +
                  ((size_t)info->list_138.count + info->list_148.count) * 4;
    int32_t i, k, n;
    for (k = 0; k < 4; ++k) size += 8 + (size_t)info->target_capacity[k] * 0x14;
    memset(&s, 0, sizeof s);
    s.size = size;
    s.data = (uint8_t *)malloc(size);
    if (!s.data) return;
    aoe_stream_put_i32(&s, info->map_width);
    aoe_stream_put_i32(&s, info->map_height);
    for (n = 0, i = 0; i < info->area_count; ++i) n += info->areas[i].id != -1;
    aoe_stream_put_i32(&s, n);
    for (i = 0; i < info->area_count; ++i)
        if (info->areas[i].id != -1) put_area(&s, &info->areas[i]);
    for (n = 0, i = 0; i < info->object_count; ++i)
        n += info->objects[i].id != -1 && info->objects[i].type_id == 0x6d;
    aoe_stream_put_i32(&s, n);
    for (i = 0; i < info->object_count; ++i)
        if (info->objects[i].id != -1 && info->objects[i].type_id == 0x6d)
            put_object(&s, &info->objects[i]);
    for (k = 0; k < 4; ++k) {
        aoe_stream_put_i32(&s, info->target_capacity[k]);
        aoe_stream_put_i32(&s, info->target_count[k]);
        for (i = 0; i < info->target_capacity[k] && info->targets[k]; ++i)
            put_target(&s, &info->targets[k][i]);
    }
    put_list(&s, &info->list_138);
    put_list(&s, &info->list_148);
    if (!s.failed)
        ops->write_file(ops->context, info->area_path, s.data, (int)s.pos);
    free(s.data);
}

void aoe_ai_info_release_ror(AoeAiInfoRor *info, const AoeAiInfoOpsRor *ops) {
    uint8_t bytes[AOE_AI_INFO_STATS * 4];
    int k;
    if (!info) return;
    if (ops && ops->write_file) {
        if (info->field_204 == 1) write_area_file(info, ops);
        if (ops->stats_player == (uint8_t)ops->self || ops->stats_player == 0xff) {
            for (k = 0; k < AOE_AI_INFO_STATS; ++k)
                aoe_le_put_u32(bytes + 4 * k, (uint32_t)info->stats[k]);
            ops->write_file(ops->context, info->learn_path, bytes,
                            (int)sizeof bytes);
        }
    }
    if (info->kind_lists)
        for (k = 0; k < 4; ++k) free(info->kind_lists[k]);
    free(info->kind_lists);
    free(info->kind_counts);
    free(info->objects);
    free(info->marks);
    free(info->areas);
    for (k = 0; k < 4; ++k) free(info->targets[k]);
    free_list(&info->list_190);
    free_list(&info->list_180);
    aoe_value_grid_free_ror(&info->explored);
    free_list(&info->list_148);
    free_list(&info->list_138);
    free_list(&info->list_128);
    free_list(&info->list_118);
    free_list(&info->list_f0);
    memset(info, 0, sizeof *info);
}

int aoe_ai_info_save_ror(AoeAiInfoRor *info, AoeOutcomeStream *out) {
    int32_t i, k, len;
    for (k = 0; k < 4; ++k) {
        aoe_stream_put_i32(out, info->kind_counts[k]);
        for (i = 0; i < info->kind_counts[k]; ++i)
            aoe_stream_put_i32(out, info->kind_lists[k][i]);
    }
    aoe_stream_put_i32(out, info->map_width);
    aoe_stream_put_i32(out, info->map_height);
    aoe_stream_put_i32(out, info->field_10c);
    aoe_stream_put_i32(out, info->object_count);
    for (i = 0; i < info->object_count; ++i) put_object(out, &info->objects[i]);
    aoe_stream_put_i32(out, info->mark_count);
    for (i = 0; i < info->mark_count; ++i) put_mark(out, &info->marks[i]);
    aoe_stream_put_i32(out, info->area_count);
    for (i = 0; i < info->area_count; ++i) put_area(out, &info->areas[i]);
    put_list(out, &info->list_118);
    put_list(out, &info->list_128);
    put_list(out, &info->list_138);
    put_list(out, &info->list_148);
    put_list(out, &info->list_190);
    put_list(out, &info->list_180);
    aoe_value_grid_save_ror(&info->explored, out);
    for (i = 0; i < 2; ++i) aoe_stream_put_i32(out, info->last_site[i]);
    for (i = 0; i < 2; ++i) aoe_stream_put_i32(out, info->prev_site[i]);
    aoe_stream_put_i32(out, info->field_1b0);
    for (k = 0; k < 4; ++k)
        for (i = 0; i < 4; ++i)
            aoe_stream_put_i32(out, i < 2 ? info->pairs_1b4[k][i]
                                          : info->pairs_1d4[k][i - 2]);
    aoe_stream_put_i32(out, info->field_204);
    len = (int32_t)strlen(info->area_path);
    aoe_stream_put_i32(out, len);
    aoe_stream_put(out, info->area_path, (size_t)len);
    aoe_stream_put(out, info->zone_flags, 0xff);
    for (k = 0; k < 4; ++k)
        for (i = 0; i < 4; ++i)
            for (len = 0; len < 3; ++len)
                aoe_stream_put_i32(out, info->cells[k][i][len]);
    for (k = 0; k < 4; ++k)
        if (info->target_capacity[k] && !info->targets[k])
            info->target_capacity[k] = 0;
    for (k = 0; k < 4; ++k) aoe_stream_put_i32(out, info->target_capacity[k]);
    for (k = 0; k < 4; ++k) aoe_stream_put_i32(out, info->target_count[k]);
    for (k = 0; k < 4; ++k)
        for (i = 0; i < info->target_capacity[k]; ++i)
            put_target(out, &info->targets[k][i]);
    for (k = 0; k < 4; ++k)
        for (i = 0; i < 4; ++i) aoe_stream_put_i32(out, info->age_allowed[k][i]);
    for (k = 0; k < 4; ++k) aoe_stream_put_i32(out, info->best_distance[k]);
    for (k = 0; k < 4; ++k) aoe_stream_put_i32(out, info->current_target[k]);
    aoe_stream_put_i32(out, info->field_560);
    for (i = 0; i < AOE_AI_INFO_HISTORY; ++i)
        aoe_stream_put_i32(out, info->history[i]);
    return !out->failed;
}

int aoe_ai_info_load_ror(AoeAiInfoRor *info, AoeOutcomeStream *in,
                         float version) {
    int32_t i, k, n;
    if (!info || !init_common(info)) return 0;
    for (k = 0; k < 4; ++k) {
        n = aoe_stream_i32(in);
        for (i = 0; i < n && !in->failed; ++i)
            aoe_ai_info_kind_append_ror(info, k, aoe_stream_i32(in));
    }
    info->map_width = aoe_stream_i32(in);
    info->map_height = aoe_stream_i32(in);
    info->field_10c = aoe_stream_i32(in);
    n = aoe_stream_i32(in);
    if (in->failed || n < 0) return 0;
    info->object_count = n;
    info->objects = (AoeAiInfoObjectRor *)calloc((size_t)n + 1, sizeof *info->objects);
    if (!info->objects) return 0;
    for (i = 0; i < n; ++i) take_object(in, &info->objects[i]);
    n = aoe_stream_i32(in);
    if (in->failed || n < 0) return 0;
    info->mark_count = n;
    info->marks = (AoeAiInfoMarkRor *)calloc((size_t)n + 1, sizeof *info->marks);
    if (!info->marks) return 0;
    for (i = 0; i < n; ++i) take_mark(in, &info->marks[i]);
    n = aoe_stream_i32(in);
    if (in->failed || n < 0) return 0;
    info->area_count = n;
    info->areas = (AoeAiInfoAreaRor *)calloc((size_t)n + 1, sizeof *info->areas);
    if (!info->areas) return 0;
    for (i = 0; i < n; ++i) take_area(in, &info->areas[i]);
    if (!take_list(in, &info->list_118) || !take_list(in, &info->list_128) ||
        !take_list(in, &info->list_138) || !take_list(in, &info->list_148) ||
        !take_list(in, &info->list_190) || !take_list(in, &info->list_180))
        return 0;
    if (!aoe_value_grid_load_ror(&info->explored, in)) return 0;
    for (i = 0; i < 2; ++i) info->last_site[i] = aoe_stream_i32(in);
    for (i = 0; i < 2; ++i) info->prev_site[i] = aoe_stream_i32(in);
    info->field_1b0 = aoe_stream_i32(in);
    if (version < 1.6f) (void)aoe_stream_i32(in);
    for (k = 0; k < 4; ++k)
        for (i = 0; i < 4; ++i) {
            if (i < 2) info->pairs_1b4[k][i] = aoe_stream_i32(in);
            else info->pairs_1d4[k][i - 2] = aoe_stream_i32(in);
        }
    info->field_204 = aoe_stream_i32(in);
    if (version >= 1.6f) {
        const uint8_t *p;
        n = aoe_stream_i32(in);
        if (in->failed || n < 0 || n >= (int32_t)sizeof info->area_path) return 0;
        p = aoe_stream_take(in, (size_t)n);
        if (!p) return 0;
        memcpy(info->area_path, p, (size_t)n);
        info->area_path[n] = 0;
    }
    {
        const uint8_t *p = aoe_stream_take(in, 0xff);
        if (!p) return 0;
        memcpy(info->zone_flags, p, 0xff);
    }
    if (version >= 2.5f)
        for (k = 0; k < 4; ++k)
            for (i = 0; i < 4; ++i)
                for (n = 0; n < 3; ++n) info->cells[k][i][n] = aoe_stream_i32(in);
    for (k = 0; k < 4; ++k) info->target_capacity[k] = aoe_stream_i32(in);
    for (k = 0; k < 4; ++k) info->target_count[k] = aoe_stream_i32(in);
    if (in->failed) return 0;
    for (k = 0; k < 4; ++k) {
        if (info->target_capacity[k] <= 0) continue;
        info->targets[k] = (AoeAiInfoTargetRor *)calloc(
            (size_t)info->target_capacity[k], sizeof **info->targets);
        if (!info->targets[k]) return 0;
        for (i = 0; i < info->target_capacity[k]; ++i)
            take_target(in, &info->targets[k][i]);
    }
    if (version >= 6.8f) {
        for (k = 0; k < 4; ++k)
            for (i = 0; i < 4; ++i) info->age_allowed[k][i] = aoe_stream_i32(in);
        for (k = 0; k < 4; ++k) info->best_distance[k] = aoe_stream_i32(in);
        for (k = 0; k < 4; ++k) info->current_target[k] = aoe_stream_i32(in);
    }
    if (version >= 6.91f) info->field_560 = aoe_stream_i32(in);
    if (version >= 6.99f)
        for (i = 0; i < AOE_AI_INFO_HISTORY; ++i)
            info->history[i] = aoe_stream_i32(in);
    return !in->failed;
}

static int32_t step_rand(const AoeAiInfoOpsRor *ops) {
    return ops->rand ? ops->rand(ops->context) : 0;
}

/* rand() % 4 with the sign of the dividend, as the original's masking. */
static int32_t rand4(const AoeAiInfoOpsRor *ops) {
    return step_rand(ops) % 4;
}

static int explore_candidate(const AoeAiInfoRor *info, int32_t id,
                             const AoeAiWorldObjectRor *unit, int32_t x,
                             int32_t y, uint8_t own_zone,
                             const AoeAiInfoOpsRor *ops) {
    int g = ops->grid_get ? ops->grid_get(ops->context, x, y) : 0xff;
    uint8_t seen = ops->vision ? ops->vision(ops->context, x, y) : 0;
    (void)info;
    if (seen == 0xf) return 0;
    if (unit->type_class == 4 ? g != 0xff : g != 0xff && g != 2) return 0;
    if (!ops->free_tile || !ops->free_tile(ops->context, id, (float)x, (float)y))
        return 0;
    return ops->tile_zone && ops->tile_zone(ops->context, id, x, y) == own_zone;
}

int aoe_ai_info_explore_step_ror(AoeAiInfoRor *info, int32_t id, float x,
                                 float y, float out[2],
                                 const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor unit;
    int32_t dir, step = 0, leg, i, tx, ty;
    int off_lo_x = 0, off_hi_x = 0, off_lo_y = 0, off_hi_y = 0;
    uint8_t own_zone;
    if (!info || !ops || !ops->world_object ||
        !ops->world_object(ops->context, id, &unit))
        return 0;
    own_zone = ops->zone ? ops->zone(ops->context, id) : 0;
    dir = rand4(ops);
    tx = aoe_ftol32(x);
    ty = aoe_ftol32(y);
    for (;;) {
        if (off_lo_x && off_hi_x && off_lo_y && off_hi_y) {
            aoe_ai_info_set_zone_flag_ror(info, id, 1, ops);
            return 0;
        }
        ++step;
        for (leg = 0; leg < 2; ++leg) {
            dir = (dir + 1) % 4;
            if (dir < 0) dir += 4;
            for (i = 0; i < step; ++i) {
                switch (dir) {
                case 0: ++ty; break;
                case 1: ++tx; break;
                case 2: --ty; break;
                default: --tx; break;
                }
                if (tx < 0) off_lo_x = 1;
                else if (tx >= info->map_width) off_hi_x = 1;
                else if (ty < 0) off_lo_y = 1;
                else if (ty >= info->map_height) off_hi_y = 1;
                else if (explore_candidate(info, id, &unit, tx, ty, own_zone, ops))
                    goto found;
            }
        }
    }
found:
    if (ops->can_reach &&
        ops->can_reach(ops->context, id, tx, ty, aoe_ftol32(unit.z),
                       unit.type_28 - 1.0f)) {
        if (ops->grid_set) ops->grid_set(ops->context, tx, ty, 0);
    } else {
        int32_t best = -1, lx = 0, ly = 0, hx, hy, c, r, v;
        if (ops->grid_set) ops->grid_set(ops->context, tx, ty, 1);
        for (c = 0; c < 4; ++c)
            for (r = 0; r < 4; ++r) {
                v = info->cells[c][r][0] + step_rand(ops) % 100 +
                    info->cells[c][r][1] * 25;
                if (best == -1 || v < best) {
                    best = v;
                    lx = info->map_width * c / 4;
                    ly = info->map_height * r / 4;
                }
            }
        hx = info->map_width / 4 + lx - 1;
        hy = info->map_height / 4 + ly - 1;
        switch (rand4(ops)) {
        case 0: tx = lx; ty = ly; break;
        case 1: tx = lx; ty = hy; break;
        case 2: tx = hx; ty = hy; break;
        default: tx = hx; ty = ly; break;
        }
    }
    out[0] = (float)tx;
    out[1] = (float)ty;
    return 1;
}

static int gatherers(const AoeAiInfoOpsRor *ops, int32_t id) {
    return ops->gatherers ? ops->gatherers(ops->context, id) : 0;
}

static int pick_class_ok(int boat, int16_t cls) {
    int water = cls == 5 || cls == 0x1f || cls == 0x21;
    return boat == 1 ? water : !water;
}

int32_t aoe_ai_info_pick_target_ror(AoeAiInfoRor *info, int32_t k,
                                    int32_t id, float pos[2], int32_t *drop,
                                    const int32_t *exclude,
                                    int32_t exclude_count,
                                    const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor unit, obj;
    AoeAiInfoTargetRor *best = NULL;
    int32_t best_score = -1, ux, uy, i, j, score;
    int refresh = 0, boat;
    uint8_t own_zone;
    *drop = -1;
    if (!info || !ops || k < 0 || k > 3 || !ops->object ||
        !ops->object(ops->context, id, &unit))
        return -1;
    own_zone = ops->zone ? ops->zone(ops->context, id) : 0;
    ux = aoe_ftol32(unit.x);
    uy = aoe_ftol32(unit.y);
    boat = aoe_class_is_boat_ror(unit.type_class);
    for (i = 0; i < info->target_count[k]; ++i) {
        AoeAiInfoTargetRor *t = &info->targets[k][i];
        int32_t dx, dy;
        if (t->gone == 1) continue;
        if (t->wait) {
            if (t->gone == 0) --t->wait;
            continue;
        }
        for (j = 0; j < exclude_count; ++j)
            if (exclude[j] == t->id) break;
        if (j < exclude_count) continue;
        if (!ops->object(ops->context, t->id, &obj)) {
            t->gone = 1;
            if (info->current_target[k] == t->id) refresh = 1;
            continue;
        }
        if (!pick_class_ok(boat, obj.type_class)) continue;
        dx = ux - t->x;
        dy = uy - t->y;
        score = dx * dx + t->field_8 * 10 + dy * dy;
        if (obj.type_id == 0x3b) {
            if (t->field_e > 20) continue;
            score += t->picks / 150;
            if (gatherers(ops, obj.id) > 2) score += 200;
        } else {
            score += t->picks / 50;
        }
        if (best && score >= best_score) continue;
        if ((obj.type_class == 9 || obj.type_class == 10) && obj.state > 2 &&
            obj.field_44 > 25.0f)
            score += 25;
        if (aoe_ai_info_enemy_center_near_ror(info, obj.x, obj.y, 0x19, ops) == 1)
            continue;
        if (aoe_ai_info_area_near_ror(info, obj.x, obj.y, 0xf,
                                      (uint32_t)ops->self) == 1)
            score += 500;
        if (obj.type_id == 0x32) {
            if (obj.owner != ops->self || obj.state != 2 ||
                gatherers(ops, obj.id) > 0)
                continue;
            for (j = 0; j < info->list_f0.count && j < info->list_f0.capacity; ++j)
                if (info->list_f0.data[j] == obj.id) break;
            if (j < info->list_f0.count && j < info->list_f0.capacity) continue;
        }
        if (k == 1 ? gatherers(ops, obj.id) >= 1
                   : (k == 2 || k == 3) && gatherers(ops, obj.id) >= 4)
            continue;
        if (obj.type_id == 0x30 && obj.state == 2 && ops->sn &&
            (ops->tactical_d68 ? ops->tactical_d68(ops->context, 0) : 0) <
                ops->sn(ops->context, 0xcc))
            continue;
        if (!ops->tile_zone ||
            ops->tile_zone(ops->context, id, t->x, t->y) != own_zone)
            continue;
        if (!best || score < best_score) {
            best = t;
            best_score = score;
            pos[0] = obj.x;
            pos[1] = obj.y;
        }
    }
    if (refresh) aoe_ai_info_refresh_targets_ror(info, k, ops);
    if (!best) return -1;
    ++best->picks;
    *drop = best->field_10;
    return best->id;
}

int32_t aoe_ai_info_resource_priority_ror(int32_t type_id, int32_t type_class,
                                          const AoeAiInfoOpsRor *ops) {
    int32_t n;
    switch (type_id) {
    case 0x2d: n = 0x46; break;
    case 0x6d: n = 0x38; break;
    case 0x9e: case 0xa3: n = 0x36; break;
    case 0x9f: n = 0x37; break;
    default:
        if (type_class == 7) n = 0x34;
        else if (type_class == 8) n = 0x33;
        else if (type_class == 0x20) n = 0x32;
        else return -1;
    }
    return ops && ops->sn ? ops->sn(ops->context, n) : -1;
}

/* The ID is among the first entries of +0x148 (count, bounded by the
   capacity). */
static int in_list_148(const AoeAiInfoRor *info, int32_t id) {
    int32_t i;
    for (i = 0; i < info->list_148.count && i < info->list_148.capacity; ++i)
        if (info->list_148.data[i] == id) return 1;
    return 0;
}

AoeAiInfoObjectRor *aoe_ai_info_free_resource_ror(const AoeAiInfoRor *info,
                                                  int32_t id,
                                                  const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor unit;
    AoeAiInfoObjectRor *best = NULL;
    int32_t best_p = -1, i, p, pt[2];
    if (!info || !ops || !ops->object || !ops->object(ops->context, id, &unit))
        return NULL;
    for (i = 0; i < info->object_count; ++i) {
        AoeAiInfoObjectRor *e = &info->objects[i];
        if (e->id == -1 || !in_list_148(info, e->id) || e->field_10 >= 1) continue;
        pt[0] = e->x;
        pt[1] = e->y;
        if (ops->group_near &&
            ops->group_near(ops->context, 4, ops->sn ? ops->sn(ops->context, 0x5c) : -1, pt))
            continue;
        p = aoe_ai_info_resource_priority_ror(e->type_id, e->type_class, ops);
        if (!best || p < best_p) {
            best = e;
            best_p = p;
        }
    }
    return best;
}

AoeAiInfoObjectRor *aoe_ai_info_better_resource_ror(
    const AoeAiInfoRor *info, int32_t id, int32_t type_id, int32_t type_class,
    const AoeAiInfoOpsRor *ops) {
    AoeAiWorldObjectRor unit, obj;
    AoeAiInfoObjectRor *best = NULL;
    int32_t best_p, i, p, pt[2];
    uint8_t zone;
    if (!info || !ops || !ops->object || !ops->object(ops->context, id, &unit))
        return NULL;
    zone = ops->zone ? ops->zone(ops->context, id) : 0;
    best_p = aoe_ai_info_resource_priority_ror(type_id, type_class, ops);
    for (i = 0; i < info->object_count; ++i) {
        AoeAiInfoObjectRor *e = &info->objects[i];
        if (e->id == -1 || !in_list_148(info, e->id) || e->field_10 > 0) continue;
        if (!ops->object(ops->context, e->id, &obj)) continue;
        if ((ops->zone ? ops->zone(ops->context, e->id) : 0) != zone) {
            pt[0] = aoe_ftol32(obj.x);
            pt[1] = aoe_ftol32(obj.y);
            if (!ops->zone_reaches ||
                ops->zone_reaches(ops->context, id, zone,
                                  ops->range ? (float)ops->range(ops->context, id) : 0.0f,
                                  pt) != 1)
                continue;
        }
        p = aoe_ai_info_resource_priority_ror(e->type_id, e->type_class, ops);
        if (p < best_p) {
            best = e;
            best_p = p;
        }
    }
    return best;
}
