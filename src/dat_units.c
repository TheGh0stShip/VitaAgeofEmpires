#include "dat.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    const uint8_t *bytes;
    size_t size;
    size_t offset;
    int valid;
} Cursor;

static const uint8_t *take(Cursor *cursor, size_t count) {
    const uint8_t *p;
    if (!cursor->valid || cursor->offset > cursor->size ||
        count > cursor->size - cursor->offset) {
        cursor->valid = 0;
        return NULL;
    }
    p = cursor->bytes + cursor->offset;
    cursor->offset += count;
    return p;
}

static uint8_t u8(Cursor *cursor) {
    const uint8_t *p = take(cursor, 1);
    return p ? p[0] : 0;
}

static uint16_t u16(Cursor *cursor) {
    const uint8_t *p = take(cursor, 2);
    return p ? (uint16_t)p[0] | ((uint16_t)p[1] << 8) : 0;
}

static uint32_t u32(Cursor *cursor) {
    const uint8_t *p = take(cursor, 4);
    return p ? (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
               ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24) : 0;
}

static float f32(Cursor *cursor) {
    uint32_t bits = u32(cursor);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint16_t at16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static float atfloat(const uint8_t *p) {
    uint32_t bits = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                    ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int parse_unit(Cursor *cursor, DatUnit *unit) {
    uint16_t name_len, task_count, attack_count, armor_count;
    uint8_t damage_count;
    const uint8_t *name, *p;
    unit->present = 1;
    unit->type = u8(cursor);
    name_len = u16(cursor);
    unit->id = (int16_t)u16(cursor);
    unit->language_name_id = u16(cursor); /* template +0x12 */
    take(cursor, 2); /* creation language ID */
    unit->unit_class = (int16_t)u16(cursor);
    unit->standing_graphic = (int16_t)u16(cursor);
    unit->dying_graphic = (int16_t)u16(cursor);
    unit->undead_graphic = (int16_t)u16(cursor);
    unit->undead_mode = u8(cursor);
    unit->hit_points = (int16_t)u16(cursor);
    unit->line_of_sight = f32(cursor);
    unit->garrison_capacity = u8(cursor); /* template +0x2c */
    for (unsigned i = 0; i < 3; ++i) unit->size[i] = f32(cursor);
    take(cursor, 2); /* train sound */
    unit->dead_unit_id = (int16_t)u16(cursor); /* template +0x48 */
    p = take(cursor, 8); /* sort, build-on, icon, editor, portrait, enabled */
    if (!p) return 0;
    unit->placement_mode = p[0];
    unit->placement_overlap_mode = p[1];
    unit->hide_in_editor = p[4];
    unit->enabled = p[7];
    p = take(cursor, 8); /* placement-side and placement-terrain pairs */
    if (!p) return 0;
    unit->placement_side_terrain[0] = (int16_t)at16(p);
    unit->placement_side_terrain[1] = (int16_t)at16(p + 2);
    unit->placement_terrain[0] = (int16_t)at16(p + 4);
    unit->placement_terrain[1] = (int16_t)at16(p + 6);
    p = take(cursor, 8); /* clearance X/Y */
    if (!p) return 0;
    unit->placement_clearance[0] = atfloat(p);
    unit->placement_clearance[1] = atfloat(p + 4);
    p = take(cursor, 21); /* template +0x64..+0x90 fields */
    if (!p) return 0;
    unit->placement_class = p[0]; /* scenario-editor placement validator */
    unit->draw_class = p[1]; /* viewport object-list filtering */
    unit->terrain_restriction = (int16_t)at16(p + 2);
    unit->elevation_mode = p[4]; /* live placement vtable +0x34 */
    unit->resource_capacity = (int16_t)at16(p + 5); /* template +0x7c */
    unit->decay_rate = atfloat(p + 7); /* runtime template +0x80 */
    unit->blast_defense_level = p[11]; /* +0x8b, blast 0x00426280 */
    unit->field_8c = p[12]; /* loader 0x004413b0 order: +0x8b,+0x8c,... */
    unit->field_8f = p[15];
    p = take(cursor, 16); /* help/hotkey IDs and flags at +0xa4..+0xa7 */
    if (!p) return 0;
    unit->negative_id_mode = p[12]; /* live constructor 0x004a8200 */
    unit->state_notify_mode = p[13]; /* pre-terminal 0x004abbf0 */
    unit->state_effect_mode = p[14]; /* state setter 0x004abdf0 */
    unit->state_notify_value = p[15]; /* pre-terminal event argument */
    take(cursor, 14); /* selection/editor style, outline and HP-bar height */
    p = take(cursor, 21); /* three seven-byte resource-storage entries */
    if (!p) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        unit->storage[i].resource_type = (int16_t)at16(p + i * 7);
        unit->storage[i].amount = atfloat(p + i * 7 + 2);
        unit->storage[i].paid = (int8_t)p[i * 7 + 6];
    }
    damage_count = u8(cursor);
    if (damage_count > 64) return 0;
    p = take(cursor, (size_t)damage_count * 5);
    if (damage_count && !p) return 0;
    unit->damage_graphic_count = damage_count;
    if (damage_count) {
        unit->damage_graphics =
            calloc(damage_count, sizeof(*unit->damage_graphics));
        if (!unit->damage_graphics) return 0;
        for (unsigned i = 0; i < damage_count; ++i) {
            unit->damage_graphics[i].graphic_id = (int16_t)at16(p + i * 5);
            unit->damage_graphics[i].damage_percent = p[i * 5 + 2];
            unit->damage_graphics[i].apply_mode = p[i * 5 + 4];
        }
    }
    p = take(cursor, 6); /* selection/dying sounds and two reaction bytes */
    if (!p) return 0;
    unit->selection_sound = (int16_t)at16(p);
    unit->dying_sound = (int16_t)at16(p + 2);
    if (!cursor->valid || name_len > 4096) return 0;
    name = take(cursor, name_len);
    if (!cursor->valid) return 0;
    unit->name = malloc((size_t)name_len + 1);
    if (!unit->name) return 0;
    memcpy(unit->name, name, name_len);
    unit->name[name_len] = '\0';
    take(cursor, 2); /* copy ID in AoE and Rise of Rome */
    if (!cursor->valid ||
        (unit->type != 90 && unit->type != 10 && unit->type != 15 &&
         unit->type != 20 && unit->type != 25 && unit->type != 30 && unit->type != 40 &&
         unit->type != 50 && unit->type != 60 && unit->type != 70 &&
         unit->type != 80)) return 0;
    if (unit->type == 90 || unit->type < 20) return 1;
    unit->speed = f32(cursor);
    if (unit->type >= 30) {
        unit->walking_graphic = (int16_t)u16(cursor);
        unit->walking_graphic2 = (int16_t)u16(cursor); /* +0xc0 */
        unit->rotation_speed = f32(cursor);
        take(cursor, 9); /* remaining AoE moving fields */
    }
    if (unit->type >= 40) {
        unit->default_task_id = (int16_t)u16(cursor);
        unit->search_radius = f32(cursor);
        unit->work_rate = f32(cursor);
        unit->drop_sites[0] = (int16_t)u16(cursor); /* template +0xe8 */
        unit->drop_sites[1] = (int16_t)u16(cursor); /* template +0xea */
        p = take(cursor, 5); /* task-swap group, attack and move sounds */
        if (!p) return 0;
        unit->task_swap_group = p[0]; /* template +0xec, 0x0043e810 order */
        take(cursor, 1); /* run pattern */
        task_count = u16(cursor);
        if (!cursor->valid || task_count > 1024) return 0;
        unit->task_count = task_count;
        if (task_count) {
            unit->tasks = calloc(task_count, sizeof(*unit->tasks));
            if (!unit->tasks) return 0;
        }
        for (uint16_t i = 0; i < task_count; ++i) {
            DatUnitTask *task = &unit->tasks[i];
            p = take(cursor, 59);
            if (!p) return 0;
            task->task_type = (int16_t)at16(p);
            task->id = (int16_t)at16(p + 2);
            task->is_default = p[4];
            task->action_type = (int16_t)at16(p + 5);
            task->target_class = (int16_t)at16(p + 7);
            task->target_unit = (int16_t)at16(p + 9);
            task->target_terrain = (int16_t)at16(p + 11);
            task->resource_in = (int16_t)at16(p + 13);
            task->resource_multiplier = (int16_t)at16(p + 15);
            task->resource_out = (int16_t)at16(p + 17);
            task->work_value1 = atfloat(p + 21);
            task->work_value2 = atfloat(p + 25);
            task->work_range = atfloat(p + 29);
            task->auto_search = p[33];
            task->search_wait = atfloat(p + 34);
            task->enable_targeting = p[38];
            task->field_30 = (int16_t)at16(p + 40);
            task->field_32 = (int16_t)at16(p + 42);
            task->owner_mode = p[44];
            task->requires_carried_resource = p[45];
            task->unbuilt_target = p[46];
            task->moving_graphic = (int16_t)at16(p + 47);
            task->proceeding_graphic = (int16_t)at16(p + 49);
            task->working_graphic = (int16_t)at16(p + 51);
            task->carrying_graphic = (int16_t)at16(p + 53);
            task->gathering_sound = (int16_t)at16(p + 55);
            task->deposit_sound = (int16_t)at16(p + 57);
        }
    }
    if (unit->type >= 50) {
        unit->base_armor = u8(cursor);
        attack_count = u16(cursor);
        if (!cursor->valid || attack_count > 256) return 0;
        unit->attack_count = attack_count;
        if (attack_count) {
            unit->attacks = calloc(attack_count, sizeof(*unit->attacks));
            if (!unit->attacks) return 0;
        }
        for (uint16_t i = 0; i < attack_count; ++i) {
            unit->attacks[i].class_id = (int16_t)u16(cursor);
            unit->attacks[i].amount = (int16_t)u16(cursor);
        }
        armor_count = u16(cursor);
        if (!cursor->valid || armor_count > 256) return 0;
        unit->armor_count = armor_count;
        if (armor_count) {
            unit->armors = calloc(armor_count, sizeof(*unit->armors));
            if (!unit->armors) return 0;
        }
        for (uint16_t i = 0; i < armor_count; ++i) {
            unit->armors[i].class_id = (int16_t)u16(cursor);
            unit->armors[i].amount = (int16_t)u16(cursor);
        }
        unit->terrain_defense_bonus = (int16_t)u16(cursor);
        unit->max_range = f32(cursor);
        unit->blast_width = f32(cursor);
        unit->reload_time = f32(cursor);
        unit->projectile_unit_id = (int16_t)u16(cursor);
        unit->accuracy_percent = (int16_t)u16(cursor);
        take(cursor, 1); /* break-off combat */
        unit->frame_delay = (int16_t)u16(cursor);
        /* Graphic displacement XYZ: projectile spawn offsets, original
           template +0x12c/+0x130/+0x134 (0x00426520). */
        for (unsigned i = 0; i < 3; ++i)
            unit->projectile_offset[i] = f32(cursor);
        unit->blast_level = u8(cursor); /* template +0x11c */
        unit->min_range = f32(cursor);
        unit->attack_graphic = (int16_t)u16(cursor);
        unit->displayed_melee_armor = (int16_t)u16(cursor);
        unit->displayed_attack = (int16_t)u16(cursor);
        unit->displayed_range = f32(cursor);
        unit->displayed_reload_time = f32(cursor);
    }
    if (unit->type == 60) {
        /* Type-60 loader 0x00440530: bytes +0x148..+0x14c, float +0x150. */
        p = take(cursor, 5);
        if (!p) return 0;
        unit->projectile_flight_mode = p[0];
        unit->projectile_lead = p[1];
        unit->projectile_hit_mode = p[2];
        unit->projectile_arc = f32(cursor);
    }
    if (unit->type >= 70) {
        for (unsigned i = 0; i < 3; ++i) {
            unit->costs[i].resource_type = (int16_t)u16(cursor);
            unit->costs[i].amount = (int16_t)u16(cursor);
            unit->costs[i].paid = (int16_t)u16(cursor);
        }
        unit->train_time = (int16_t)u16(cursor);
        unit->train_location_id = (int16_t)u16(cursor);
        unit->button_id = (int8_t)u8(cursor);
        unit->displayed_pierce_armor = (int16_t)u16(cursor);
    }
    if (unit->type == 80) {
        unit->construction_graphic = (int16_t)u16(cursor);
        unit->building_adjacent_mode = u8(cursor);
        unit->building_angle = u16(cursor);
        unit->disappears_when_built = u8(cursor);
        unit->stack_unit_id = (int16_t)u16(cursor);
        unit->foundation_terrain = (int16_t)u16(cursor);
        unit->old_terrain = (int16_t)u16(cursor);
        unit->building_research_id = (int16_t)u16(cursor);
        unit->completion_sound = (int16_t)u16(cursor);
    }
    return cursor->valid;
}

void dat_free_civilizations(DatCivilizations *civilizations) {
    if (!civilizations) return;
    if (civilizations->civs)
        for (uint16_t i = 0; i < civilizations->count; ++i) {
            DatCiv *civ = &civilizations->civs[i];
            if (civ->units)
                for (uint16_t j = 0; j < civ->unit_count; ++j) {
                    free(civ->units[j].name);
                    free(civ->units[j].tasks);
                    free(civ->units[j].damage_graphics);
                    free(civ->units[j].attacks);
                    free(civ->units[j].armors);
                }
            free(civ->units);
            free(civ->resources);
        }
    free(civilizations->civs);
    memset(civilizations, 0, sizeof(*civilizations));
}

const DatUnit *dat_find_unit(const DatCivilizations *civilizations,
                             uint16_t civilization_id, uint16_t unit_id) {
    const DatCiv *civ;
    if (!civilizations || !civilizations->civs ||
        civilization_id >= civilizations->count) return NULL;
    civ = &civilizations->civs[civilization_id];
    if (!civ->units || unit_id >= civ->unit_count ||
        !civ->units[unit_id].present) return NULL;
    return &civ->units[unit_id];
}

int dat_parse_civilizations(const DatBlob *blob, const DatEffects *effects,
                            DatCivilizations *civilizations) {
    Cursor cursor;
    uint16_t count;
    if (!civilizations) return 0;
    memset(civilizations, 0, sizeof(*civilizations));
    if (!blob || !blob->bytes || !effects || effects->next_offset > blob->size)
        return 0;
    cursor = (Cursor){blob->bytes, blob->size, effects->next_offset, 1};
    count = u16(&cursor);
    if (!cursor.valid || count > 64) return 0;
    civilizations->civs = calloc(count ? count : 1,
                                  sizeof(*civilizations->civs));
    if (!civilizations->civs) return 0;
    civilizations->count = count;
    for (uint16_t i = 0; i < count; ++i) {
        DatCiv *civ = &civilizations->civs[i];
        const uint8_t *name, *pointers;
        civ->player_type = (int8_t)u8(&cursor);
        name = take(&cursor, 20);
        if (!cursor.valid || !memchr(name, 0, 20)) goto fail;
        memcpy(civ->name, name, 20);
        civ->resource_count = u16(&cursor);
        civ->tech_tree_id = (int16_t)u16(&cursor);
        if (!cursor.valid || civ->resource_count > 1024) goto fail;
        civ->resources = calloc(civ->resource_count ? civ->resource_count : 1,
                                sizeof(*civ->resources));
        if (!civ->resources) goto fail;
        for (uint16_t j = 0; j < civ->resource_count; ++j)
            civ->resources[j] = f32(&cursor);
        civ->icon_set = u8(&cursor);
        civ->unit_count = u16(&cursor);
        if (!cursor.valid || civ->unit_count > 2048) goto fail;
        pointers = take(&cursor, (size_t)civ->unit_count * 4);
        if (!cursor.valid) goto fail;
        civ->units = calloc(civ->unit_count ? civ->unit_count : 1,
                            sizeof(*civ->units));
        if (!civ->units) goto fail;
        for (uint16_t j = 0; j < civ->unit_count; ++j)
            if (pointers[(size_t)j * 4] || pointers[(size_t)j * 4 + 1] ||
                pointers[(size_t)j * 4 + 2] || pointers[(size_t)j * 4 + 3])
                if (!parse_unit(&cursor, &civ->units[j])) goto fail;
    }
    if (!cursor.valid) goto fail;
    civilizations->next_offset = cursor.offset;
    return 1;
fail:
    dat_free_civilizations(civilizations);
    return 0;
}
