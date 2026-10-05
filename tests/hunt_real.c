#include "dat.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "world_combat.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hunting (0x6e, 0x004b32c0) on a real Rise of Rome scenario: a player-1
   villager ordered onto a live Gaia animal or a standing tree takes the
   hunter or woodcutter task (0x00405f20 task swap), kills the target
   through a nested attack action (state 12, 0x00401150 with its own
   sub-action list), gathers from it and deposits at a drop site. */
static int16_t head_type(const AoeLiveUnit *u) {
    const AoeActionList *l = u->derived_action_list;
    int16_t t;
    if (!l || !l->head || !l->head->action) return 0;
    memcpy(&t, l->head->action, sizeof(t));
    return t;
}
static AoeLiveUnit *nearest(AoeScenarioRuntimeRor *r, AoeLiveUnit *from,
                            int (*want)(const AoeLiveUnit *)) {
    AoeScenarioRuntimePlayerRor *g = &r->players[0];
    AoeLiveUnit *best = NULL;
    double bd = 1e30;
    for (uint32_t i = 0; i < g->unit_count; ++i) {
        AoeLiveUnit *u = &g->units[i];
        double dx, dy, d;
        if (!u->constructed || u->core.state != 2 || !want(u)) continue;
        dx = u->core.position.x38 - from->core.position.x38;
        dy = u->core.position.y3c - from->core.position.y3c;
        d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = u; }
    }
    return best;
}
static int is_prey(const AoeLiveUnit *u) {
    return u->core.unit_template->unit_class == 9 &&
           u->core.unit_template->type == 70;
}
static int is_tree(const AoeLiveUnit *u) {
    return u->core.unit_template->unit_class == 15;
}
static int run(AoeWorldCombatRor *c, AoeScenarioRuntimeRor *r,
               AoeLiveUnit *v, AoeLiveUnit *target, int16_t resource,
               const char *what) {
    AoeScenarioRuntimePlayerRor *p = &r->players[1];
    float before = p->resources.values[resource];
    int16_t code = -1;
    int ticks, saw_attack = 0;
    /* Start next to the target (the nearest one may lie across water). */
    v->core.position.x38 = target->core.position.x38 + 1.5f;
    v->core.position.y3c = target->core.position.y3c;
    assert(aoe_world_combat_order_ror(c, v, target,
                                      target->core.position.x38,
                                      target->core.position.y3c, 0.0f,
                                      &code));
    printf("%s: template %d (%s), head %d\n", what,
           v->core.unit_template->id, v->core.unit_template->name,
           head_type(v));
    assert(head_type(v) == 0x6e);
    for (ticks = 0; ticks < 20000 && !(p->resources.values[resource] > before);
         ++ticks) {
        const AoeActionList *l = v->derived_action_list;
        if (l && l->head && head_type(v) == 0x6e &&
            ((AoeGatherActionRor *)l->head->action)->base.state == 0x0c)
            saw_attack = 1;
        if (getenv("HUNT_TRACE") && ticks % 50 == 0 && ticks < 800) {
            AoeWorldCombatUnitRor *rec = aoe_world_combat_unit_ror(c, v);
            const AoeActionNode *n = rec ? rec->sub_actions.head : NULL;
            if (n) {
                AoeAttackActionRor *at = n->action;
                printf("    sub type %d state %d range %g pos %.2f,%.2f -> "
                       "%.2f,%.2f sublist %p head %p\n", at->base.type,
                       at->base.state, at->range, v->core.position.x38,
                       v->core.position.y3c, target->core.position.x38,
                       target->core.position.y3c, (void *)at->base.sub_list,
                       at->base.sub_list && at->base.sub_list->head
                           ? at->base.sub_list->head->action : NULL);
            }
        }
        if (getenv("HUNT_TRACE") && ticks % 100 == 0 && l && l->head)
            printf("  t%d head %d state %d carried %g target %d hp %g "
                   "amount %g\n", ticks, head_type(v),
                   ((AoeMoveActionRor *)l->head->action)->state,
                   v->core.storage.carried_amount, target->core.state,
                   target->core.hit_points,
                   target->core.storage.carried_amount);
        assert(aoe_world_combat_update_world_ror(c, 0.05f));
    }
    printf("%s: deposited %g after %d updates (attack seen %d, target "
           "state %d)\n", what, p->resources.values[resource] - before,
           ticks, saw_attack, target->core.state);
    assert(p->resources.values[resource] > before);
    assert(c->hunt_failures == 0);
    return saw_attack;
}

int main(int argc, char **argv) {
    static DatBlob blob;
    static DatIntro intro;
    static DatSounds sounds;
    static DatGraphics graphics;
    static DatTerrains terrains;
    static DatBorders borders;
    static DatMapGeometry geometry;
    static DatRandomMaps random_maps;
    static DatEffects effects;
    static DatTechs techs;
    static DatCivilizations civs;
    static ScnMap scenario;
    static AoeScenarioRuntimeRor runtime;
    static AoeWorldMotionRor motion;
    static AoeWorldCombatRor combat;
    AoeScenarioRuntimePlayerRor *p;
    int16_t code = -1;
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 3) return 2;
    if (!dat_load(argv[2], &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !dat_parse_techs(&blob, &civs, &techs) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario))
        return 1;
    assert(aoe_scenario_runtime_build_ror(&runtime, &scenario, &intro, &civs,
                                          &graphics, &terrains, &borders,
                                          &geometry, &techs, &effects, NULL,
                                          1));
    assert(aoe_scenario_runtime_start_game_ror(&runtime));
    assert(aoe_world_motion_init_ror(&motion, &runtime));
    assert(aoe_world_combat_init_ror(&combat, &motion));
    {
        AoeScenarioRuntimePlayerRor *p = &runtime.players[1];
        AoeLiveUnit *a = NULL, *b = NULL, *prey, *tree;
        for (uint32_t i = 0; i < p->unit_count; ++i) {
            AoeLiveUnit *u = &p->units[i];
            if (!u->constructed || u->core.state != 2 ||
                u->core.unit_template->id != 83)
                continue;
            if (!a) a = u;
            else if (!b) b = u;
        }
        assert(a && b);
        prey = nearest(&runtime, a, is_prey);
        tree = nearest(&runtime, b, is_tree);
        assert(prey && tree);
        assert(run(&combat, &runtime, a, prey, 0, "hunt"));
        (void)run(&combat, &runtime, b, tree, 1, "wood");
    }
    (void)code;
    return 0;
}
