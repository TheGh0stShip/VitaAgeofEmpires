#include "live_unit_factory.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

/* Patched RoR 1.0a per-unit update pieces around the root state switch:
   dormancy 0x004ac380 with the type-20 constructor wake 0x00415130, target
   attach/release (+0xc0/+0xc4), the 0x00415200 tail and the 0x00406500
   action list. */

static void record_cell(void *context, int32_t x, int32_t y) {
    (void)context; (void)x; (void)y;
}

typedef struct {
    char events[16];
    unsigned count;
    uint8_t last_state;
} Trace;
static void event(Trace *t, char e) {
    assert(t->count < sizeof(t->events));
    t->events[t->count++] = e;
}
static int trace_set_state(void *context, AoeLiveUnit *unit, uint8_t state) {
    event(context, 'S');
    ((Trace *)context)->last_state = state;
    unit->core.state = state;
    return 1;
}

/* Action list fixtures: actions are small integers cast to pointers. */
static unsigned destroyed, activated;
static uint8_t next_result;
static void action_destroy(void *context, void *action) {
    (void)context; (void)action;
    ++destroyed;
}
static void action_activate(void *context, void *action, uint8_t state) {
    (void)context; (void)action; (void)state;
    ++activated;
}
static uint8_t head_update(void *context, void *action) {
    (void)context; (void)action;
    return next_result;
}
static void waiting_update(void *context, void *action) {
    (void)context; (void)action;
}
static int16_t default_seen;
static int default_task(void *context, AoeLiveUnit *unit, int16_t task) {
    (void)context; (void)unit;
    default_seen = task;
    return 1;
}
static unsigned attached_calls;
static int attached_update(void *context, AoeLiveUnit *unit,
                           AoeActionList *list) {
    (void)context; (void)unit; (void)list;
    ++attached_calls;
    return 1;
}

static int list_count(const AoeActionList *l) {
    int n = 0;
    for (const AoeActionNode *x = l->head; x; x = x->next) ++n;
    return n;
}

int main(void) {
    ScnTile terrain[16] = {0};
    AoeObjectUpdateList tiles[16] = {0};
    int8_t counts[16];
    uint32_t masks[16] = {0};
    uint8_t view[16] = {0};
    int32_t relations[1] = {1};
    float resources[1] = {0};
    AoePlayerResources resource_state = {1, resources};
    AoeOwnedVisibilityRegistry world;
    AoePlayerUpdateState owner = {0};
    AoeRng rng = {.state = 1};
    DatUnit tree = {.present = 1, .type = 90, .hit_points = 25,
                    .standing_graphic = 0, .walking_graphic = -1};
    DatUnit man = {.present = 1, .type = 70, .hit_points = 25,
                   .standing_graphic = 0, .walking_graphic = -1,
                   .default_task_id = -1};
    DatGraphic graphic = {.present = 1, .frame_count = 4, .angle_count = 1,
                          .frame_duration = 1.0f, .sequence_type = 1};
    DatGraphics graphics = {.count = 1, .present_count = 1,
                            .graphics = &graphic};
    DatMapGeometry geometry = {.tile_half_width = 32,
                               .tile_half_height = 16};
    AoeVisibilityFootprintGrid grid = {
        .width = 4, .height = 4, .counts = counts, .tile_masks = masks,
        .view_codes = view, .visible_mask = 1u, .clear_visible_mask = ~1u,
        .explored_mask = 0x10000u
    };
    AoeLiveUnitVisionBinding vision = {
        .owner_grid = &grid, .owner_index = 0, .active_index = 0,
        .active_relations = relations, .relation_count = 1,
        .explored_ops = {.record_owner = record_cell,
                         .record_active = record_cell}
    };
    AoeLiveUnit dormant = {0}, awake = {0};
    int handled = 0, notice = 0;
    for (unsigned i = 0; i < 16; ++i) counts[i] = -1;
    assert(aoe_owned_visibility_registry_init(&world));

    /* Root/type-90 objects with hit points stay dormant in +0x2c; type
       20..80 units are woken into +0x28 by the type-20 constructor tail,
       which also clears +0x88. */
    assert(aoe_live_unit_construct_ror(&dormant, &tree, &graphics,
                                       &resource_state, &world, &owner,
                                       terrain, tiles, &geometry, 4, 4,
                                       &vision, &rng, 1.5f, 1.5f, 0.0f));
    assert(dormant.select_2c == 1 && dormant.core.positive_hit_points &&
           owner.object_list2c.count == 1 && owner.object_list28.count == 0);
    awake.animation_rate = 5.0f;
    assert(aoe_live_unit_construct_ror(&awake, &man, &graphics,
                                       &resource_state, &world, &owner,
                                       terrain, tiles, &geometry, 4, 4,
                                       &vision, &rng, 2.5f, 2.5f, 0.0f));
    assert(awake.select_2c == 0 && !awake.core.positive_hit_points &&
           awake.animation_rate == 0.0f && awake.owner_node &&
           owner.object_list28.count == 1 && owner.object_list2c.count == 1);
    assert(aoe_visibility_registry_resolve(&world.view, awake.core.unit_id) ==
           &awake.core.position);

    /* 0x004ac380(1) is deferred to the prelude; (0) cancels a deferral. */
    assert(aoe_live_unit_set_dormant_ror(&awake, &owner, &world, 1));
    assert(awake.core.positive_hit_points && awake.core.registration_pending &&
           awake.select_2c == 0 && owner.object_list28.count == 1);
    assert(aoe_live_unit_set_dormant_ror(&awake, &owner, &world, 0));
    assert(!awake.core.positive_hit_points &&
           !awake.core.registration_pending && owner.object_list28.count == 1);

    /* Root attach (+0xc0 0x004aad90) counts and wakes; root release
       (+0xc4 0x004aadb0) at zero in state 2 defers dormancy, which the
       next prelude carries out. */
    assert(aoe_live_unit_target_attach_ror(&dormant, &owner, &world));
    assert(dormant.core.target_refs == 1 && dormant.select_2c == 0 &&
           owner.object_list28.count == 2 && owner.object_list2c.count == 0);
    assert(aoe_live_unit_target_attach_ror(&dormant, &owner, &world));
    assert(aoe_live_unit_target_release_ror(&dormant, &owner, &world,
                                            &notice));
    assert(dormant.core.target_refs == 1 && !dormant.core.positive_hit_points
           && !notice);
    assert(aoe_live_unit_target_release_ror(&dormant, &owner, &world,
                                            &notice));
    assert(!dormant.core.target_refs && dormant.core.registration_pending &&
           dormant.select_2c == 0 && !notice);
    assert(aoe_live_unit_update_prelude_ror(&dormant, &owner, &world,
                                            &handled) && handled);
    assert(dormant.select_2c == 1 && !dormant.core.registration_pending &&
           owner.object_list2c.count == 1 && owner.object_list28.count == 1);
    dormant.core.target_refs = 0xfa;
    assert(aoe_live_unit_target_attach_ror(&dormant, &owner, &world) &&
           dormant.core.target_refs == 0xfa);

    /* Types 50..80 release through 0x00426b40: re-woken at once and a
       0x2bb notice. */
    assert(aoe_live_unit_target_attach_ror(&awake, &owner, &world));
    assert(aoe_live_unit_target_release_ror(&awake, &owner, &world,
                                            &notice));
    assert(notice && !awake.core.positive_hit_points &&
           !awake.core.registration_pending && awake.select_2c == 0);

    /* 0x00415200 tail. */
    {
        Trace t = {0};
        AoeLiveUnitUpdateStateOps ops = {0};
        AoeSpriteInstance *si =
            (AoeSpriteInstance *)aoe_sprite_runtime_at(&awake.sprites, 0);
        ops.context = &t;
        ops.set_state = trace_set_state;
        ops.world_delta = 0.0f;
        assert(si && si->kind == 2 && si->graphic ==
               awake.graphic.current_graphic);
        awake.core.state = 3;
        assert(aoe_live_unit_update_type20_tail_ror(&awake, &ops, 1));
        assert(t.count == 1 && t.last_state == 4 && awake.core.state == 4);
        awake.animation_rate = 2.0f;
        si->animation.frame = 2;
        assert(aoe_live_unit_update_type20_tail_ror(&awake, &ops, 1));
        assert(awake.core.state == 4 && awake.animation_rate == 2.0f);
        /* Only AL of the frame is tested: frame 0x100 counts as 0. */
        si->animation.frame = 0x100;
        assert(aoe_live_unit_update_type20_tail_ror(&awake, &ops, 1));
        assert(awake.core.state == 5 && awake.animation_rate == 0.0f &&
               t.count == 1); /* a raw byte write, not the setter */
        awake.core.state = 3;
        assert(aoe_live_unit_update_type20_tail_ror(&awake, &ops, 0));
        assert(t.last_state == 5 && awake.core.state == 5);
        /* A static current sprite answers frame 0. */
        si->kind = 1;
        si->animation.frame = 3;
        awake.core.state = 4;
        assert(aoe_live_unit_update_type20_tail_ror(&awake, &ops, 1));
        assert(awake.core.state == 5);
        si->kind = 2;
        awake.core.state = 2;
    }

    /* 0x00406500 action list. */
    {
        AoeActionList list = {0};
        AoeActionTickOps tick = {NULL, head_update, waiting_update};
        AoeLiveUnitActionListOps ops = {NULL, default_task, attached_update,
                                        &tick};
        list.owner_state = &awake.core.state;
        list.ops = (AoeActionListOps){NULL, action_destroy, action_activate};

        /* Empty in state 2: the default task when +0xdc is not -1. */
        default_seen = -2;
        assert(aoe_live_unit_update_action_list_ror(&awake, &list, &ops));
        assert(default_seen == -2);
        man.default_task_id = 1;
        assert(aoe_live_unit_update_action_list_ror(&awake, &list, &ops));
        assert(default_seen == 1);
        man.default_task_id = -1;

        /* Result 0 keeps the head; result 3 pops it once. */
        assert(aoe_action_list_push(&list, (void *)1));
        assert(aoe_action_list_push(&list, (void *)2));
        next_result = 0;
        assert(aoe_live_unit_update_action_list_ror(&awake, &list, &ops));
        assert(list_count(&list) == 2 && destroyed == 0);
        next_result = 3;
        assert(aoe_live_unit_update_action_list_ror(&awake, &list, &ops));
        assert(list_count(&list) == 1 && destroyed == 1);
        /* Result 1 is popped by the list update and again here. */
        assert(aoe_action_list_push(&list, (void *)3));
        next_result = 1;
        assert(aoe_live_unit_update_action_list_ror(&awake, &list, &ops));
        assert(list_count(&list) == 0 && destroyed == 3);

        /* An attached unit diverts to the list's +0x08. */
        assert(aoe_action_list_push(&list, (void *)4));
        awake.placement.attached_unit = &dormant.core;
        assert(aoe_live_unit_update_action_list_ror(&awake, &list, &ops));
        assert(attached_calls == 1 && list_count(&list) == 1);
        awake.placement.attached_unit = NULL;

        /* Above state 2 the list is cleared. */
        awake.core.state = 3;
        assert(aoe_live_unit_update_action_list_ror(&awake, &list, &ops));
        assert(list_count(&list) == 0 && destroyed == 4);
        assert(aoe_live_unit_update_action_list_ror(&awake, NULL, &ops));
        awake.core.state = 2;
    }

    aoe_live_unit_destroy_ror(&awake, &owner, &world, &vision);
    aoe_live_unit_destroy_ror(&dormant, &owner, &world, &vision);
    assert(owner.object_list28.count == 0 && owner.object_list2c.count == 0);
    aoe_owned_visibility_registry_destroy(&world);
    return 0;
}
