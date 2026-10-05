#include "research_manager.h"

#include <stdlib.h>
#include <string.h>

static const DatTech *tech_at(const DatTechs *techs, int16_t id) {
    /* The original checks `count < id`; the port also refuses id == count. */
    if (!techs || id < 0 || id >= (int16_t)techs->count) return NULL;
    return &techs->techs[id];
}

int32_t aoe_tech_help_ror(const DatTechs *techs, int16_t id) {
    const DatTech *t = tech_at(techs, id);
    return t ? t->language_help : -1;
}

int32_t aoe_tech_tree_text_ror(const DatTechs *techs, int16_t id) {
    const DatTech *t = tech_at(techs, id);
    return t ? t->language_tree : -1;
}

int32_t aoe_tech_hotkey_ror(const DatTechs *techs, int16_t id) {
    const DatTech *t = tech_at(techs, id);
    return t ? t->hotkey : -1;
}

int aoe_research_manager_load_ror(AoeResearchCompletionManagerRor *manager,
                                  AoeOutcomeStream *in, float save_version,
                                  int reapply) {
    int16_t i;
    const uint8_t *p = aoe_stream_take(in, 2);
    manager->count = p ? (int16_t)(p[0] | p[1] << 8) : 0;
    manager->entries = NULL;
    if (manager->count <= 0) return !in->failed;
    manager->entries = calloc((size_t)manager->count, sizeof *manager->entries);
    if (!manager->entries) {
        manager->count = 0;
        return 0;
    }
    for (i = 0; i < manager->count; ++i) {
        AoeResearchProgressEntry *e = &manager->entries[i];
        if (save_version > 7.03f) {
            e->progress = aoe_stream_f32(in);
        } else {
            /* Two bytes into the float's low half, as the original. */
            uint8_t raw[4] = {0, 0, 0, 0};
            p = aoe_stream_take(in, 2);
            if (p) memcpy(raw, p, 2);
            e->progress = aoe_le_f32(raw);
        }
        p = aoe_stream_take(in, 2);
        e->state = p ? (int16_t)(p[0] | p[1] << 8) : 0;
    }
    if (reapply) {
        for (i = 0; i < manager->count; ++i) {
            if (manager->entries[i].state != 3) continue;
            manager->entries[i].state = 1;
            aoe_research_complete_ror(manager, i);
        }
        if (manager->rebuild) manager->rebuild(manager->context);
    }
    return !in->failed;
}

void aoe_research_manager_release_ror(AoeResearchCompletionManagerRor *manager) {
    if (manager->entries) {
        free(manager->entries);
        manager->entries = NULL;
    }
}

void aoe_research_manager_save_ror(const AoeResearchCompletionManagerRor *manager,
                                   AoeOutcomeStream *out) {
    uint8_t b[2];
    int16_t i;
    b[0] = (uint8_t)manager->count;
    b[1] = (uint8_t)((uint16_t)manager->count >> 8);
    aoe_stream_put(out, b, 2);
    for (i = 0; i < manager->count; ++i) {
        aoe_stream_put_f32(out, manager->entries[i].progress);
        b[0] = (uint8_t)manager->entries[i].state;
        b[1] = (uint8_t)((uint16_t)manager->entries[i].state >> 8);
        aoe_stream_put(out, b, 2);
    }
}

static int in_range(const AoeResearchCompletionManagerRor *m, int16_t id) {
    return id < m->count && id >= 0;
}

int16_t aoe_research_state_ror(const AoeResearchCompletionManagerRor *manager,
                               int16_t id) {
    return in_range(manager, id) ? manager->entries[id].state : -1;
}

int16_t aoe_research_percent_ror(const AoeResearchCompletionManagerRor *manager,
                                 const DatTechs *techs, int16_t id) {
    const DatTech *t;
    if (!in_range(manager, id)) return 0;
    if (manager->entries[id].state == 3) return 100;
    if (manager->entries[id].state != 2) return 0;
    t = tech_at(techs, id);
    if (!t || t->research_time == 0) return 0; /* port guard */
    return (int16_t)(int32_t)((double)manager->entries[id].progress * 100.0 /
                              (double)t->research_time);
}

static const DatTech *managed(const AoeResearchCompletionManagerRor *m,
                              const DatTechs *techs, int16_t id) {
    return in_range(m, id) ? tech_at(techs, id) : NULL;
}

const char *aoe_research_name_ror(const AoeResearchCompletionManagerRor *manager,
                                  const DatTechs *techs, int16_t id) {
    const DatTech *t = managed(manager, techs, id);
    return t ? t->name : NULL;
}

uint16_t aoe_research_language_name_ror(
    const AoeResearchCompletionManagerRor *manager, const DatTechs *techs,
    int16_t id) {
    const DatTech *t = managed(manager, techs, id);
    return t ? t->language_name : 0;
}

uint16_t aoe_research_language_description_ror(
    const AoeResearchCompletionManagerRor *manager, const DatTechs *techs,
    int16_t id) {
    const DatTech *t = managed(manager, techs, id);
    return t ? t->language_description : 0;
}

uint8_t aoe_research_button_ror(const AoeResearchCompletionManagerRor *manager,
                                const DatTechs *techs, int16_t id) {
    const DatTech *t = managed(manager, techs, id);
    return t ? (uint8_t)t->button_id : 0;
}

int32_t aoe_research_help_ror(const AoeResearchCompletionManagerRor *manager,
                              const DatTechs *techs, int16_t id) {
    return in_range(manager, id) ? aoe_tech_help_ror(techs, id) : -1;
}

int32_t aoe_research_tree_text_ror(
    const AoeResearchCompletionManagerRor *manager, const DatTechs *techs,
    int16_t id) {
    return in_range(manager, id) ? aoe_tech_tree_text_ror(techs, id) : -1;
}

int32_t aoe_research_hotkey_ror(const AoeResearchCompletionManagerRor *manager,
                                const DatTechs *techs, int16_t id) {
    return in_range(manager, id) ? aoe_tech_hotkey_ror(techs, id) : -1;
}

void aoe_research_costs_ror(const AoeResearchOwnerRor *owner, int16_t id,
                            int16_t types[3], int16_t amounts[3]) {
    const AoeResearchCompletionManagerRor *m = owner->manager;
    const AoePlayerResources *r = owner->resources;
    const DatTech *t;
    int16_t shortfall = 0, reason;
    int can, i;
    for (i = 0; i < 3; ++i) {
        types[i] = -1;
        amounts[i] = 0;
    }
    if (!m || !in_range(m, id) || !(t = tech_at(owner->techs, id))) return;
    can = aoe_research_can_start_ror(owner, id, &reason) == 1;
    if (!r || r->count <= 29 || r->values[29] < 1.0f) can = 0;
    for (i = 0; i < 3; ++i) {
        int16_t type, have;
        if (!t->costs[i].paid) continue;
        type = t->costs[i].resource_type;
        types[i] = type;
        if (type < 0) continue;
        amounts[i] = t->costs[i].amount;
        if (!r || type >= r->count) continue; /* port guard */
        if (!(r->values[type] < (float)amounts[i]) || !can) continue;
        have = (int16_t)(int32_t)r->values[type];
        if (i == 0)
            shortfall = (int16_t)(amounts[i] - have);
        else
            shortfall = (int16_t)(shortfall + (amounts[i] - have));
        amounts[i] = have;
    }
    if (shortfall > 0) {
        if (types[0] == 3 || types[0] == -1) {
            types[0] = 3;
            amounts[0] = (int16_t)(amounts[0] + shortfall);
        } else if (types[1] == 3 || types[1] == -1) {
            types[1] = 3;
            amounts[1] = (int16_t)(amounts[1] + shortfall);
        } else if (types[2] == 3 || types[2] == -1) {
            types[2] = 3;
            amounts[2] = shortfall;
        }
    }
    for (i = 0; i < 3; ++i)
        if (amounts[i] < 1) types[i] = -1;
}

void aoe_research_action_readout_ror(const AoeResearchCompletionManagerRor *manager,
                                     const DatTechs *techs, uint8_t action_state, int16_t tech,
                                     int16_t *id, int16_t *percent, char *name, int16_t size) {
    *id = tech;
    *percent = action_state == 1 ? 100 : aoe_research_percent_ror(manager, techs, tech);
    if (name && size > 0) {
        const char *src = aoe_research_name_ror(manager, techs, tech);
        strncpy(name, src ? src : "", (size_t)size);
        name[size - 1] = '\0';
    }
}
