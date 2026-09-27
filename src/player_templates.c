#include "player_templates.h"
#include "unit_template_factory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void free_unit(DatUnit *unit) {
    if (!unit) return;
    free(unit->name);
    free(unit->tasks);
    free(unit->attacks);
    free(unit->armors);
    free(unit);
}

void aoe_player_templates_free(AoePlayerTemplates *templates) {
    if (!templates) return;
    if (templates->slots)
        for (uint16_t i = 0; i < templates->count; ++i)
            free_unit(templates->slots[i]);
    free(templates->slots);
    *templates = (AoePlayerTemplates){0};
}

static void *copy_array(const void *source, size_t count, size_t item_size) {
    void *copy;
    if (!count) return NULL;
    if (!source || count > SIZE_MAX / item_size) return NULL;
    copy = malloc(count * item_size);
    if (copy) memcpy(copy, source, count * item_size);
    return copy;
}

static DatUnit *clone_unit(const DatUnit *source) {
    DatUnit *copy = malloc(sizeof(*copy));
    size_t name_length;
    if (!copy) return NULL;
    *copy = *source;
    copy->name = NULL;
    copy->tasks = NULL;
    copy->attacks = NULL;
    copy->armors = NULL;
    if (!source->name) goto fail;
    name_length = strlen(source->name) + 1;
    copy->name = copy_array(source->name, name_length, 1);
    if (!copy->name) goto fail;
    if (source->task_count) {
        copy->tasks = copy_array(source->tasks, source->task_count,
                                 sizeof(*copy->tasks));
        if (!copy->tasks) goto fail;
    }
    if (source->attack_count) {
        copy->attacks = copy_array(source->attacks, source->attack_count,
                                   sizeof(*copy->attacks));
        if (!copy->attacks) goto fail;
    }
    if (source->armor_count) {
        copy->armors = copy_array(source->armors, source->armor_count,
                                  sizeof(*copy->armors));
        if (!copy->armors) goto fail;
    }
    return copy;
fail:
    free_unit(copy);
    return NULL;
}

int aoe_player_templates_clone_civ(const DatCiv *civ,
                                   AoePlayerTemplates *out) {
    AoePlayerTemplates fresh = {0};
    if (!civ || !out || out->slots || out->count ||
        (civ->unit_count && !civ->units)) return 0;
    fresh.count = civ->unit_count;
    if (fresh.count) {
        fresh.slots = calloc(fresh.count, sizeof(*fresh.slots));
        if (!fresh.slots) return 0;
    }
    for (uint16_t i = 0; i < fresh.count; ++i) {
        const DatUnit *source = &civ->units[i];
        AoeUnitTemplateFactorySpec spec;
        if (!source->present) continue;
        if (!aoe_unit_template_factory_spec((uint8_t)civ->player_type,
                                            source->type, &spec)) goto fail;
        fresh.slots[i] = clone_unit(source);
        if (!fresh.slots[i]) goto fail;
    }
    *out = fresh;
    return 1;
fail:
    aoe_player_templates_free(&fresh);
    return 0;
}

DatUnit *aoe_player_templates_get(const AoePlayerTemplates *templates,
                                  uint16_t unit_id) {
    if (!templates || !templates->slots || unit_id >= templates->count)
        return NULL;
    return templates->slots[unit_id];
}
