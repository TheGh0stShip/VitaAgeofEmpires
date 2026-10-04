#ifndef VITA_AOE_TECH_TREE_H
#define VITA_AOE_TECH_TREE_H

#include <stddef.h>
#include <stdint.h>

#include "dat.h"
#include "live_unit_factory.h"
#include "player_templates.h"
#include "research_action.h"

/* Patched Rise of Rome 1.0a technology: template attribute methods, the
   upgrade copy, the live-unit effect hooks, the effect interpreter and the
   research manager's availability rebuild. */

typedef enum {
    AOE_ATTRIBUTE_SET = 0, /* template +0x08, live +0x48 */
    AOE_ATTRIBUTE_ADD = 1, /* template +0x0c, live +0x4c */
    AOE_ATTRIBUTE_MUL = 2  /* template +0x10, live +0x50 */
} AoeAttributeOp;

/* Template attribute methods by template class, each deferring to its
   parent for attributes it does not own:
   - root (types 10, 90; 0x004422d0/0x00442370/0x00442420): 0 hit points,
     1 line of sight, 2 garrison capacity, 3/4 size, 14 resource capacity;
   - type 20/25 (0x0043ecd0): 5 speed;
   - type 30 (0x00440e90): 6 rotation speed;
   - type 40 (0x0043e780): 13 work rate;
   - type 50 (0x0043f920/0x0043fa60/0x0043fc40): 6 (add/multiply only:
     rotation speed), 8 armor and 9 attack (set: entries[value >> 8].amount
     = value & 0xff by index; add: the entry whose class is value >> 8
     gains value & 0xff; multiply: nothing), 10 reload, 11 accuracy, 12
     max range, 15 base armor (multiply uses the projectile word), 16
     projectile (set keeps the low byte), 18 terrain defense bonus (add and
     multiply store);
   - type 60 (0x00440810): 19 projectile lead byte (stored for every op),
     then type 50;
   - type 70 (tribe, 0x004ecf20): 100 deductible costs; 101 (set only)
     non-deductible costs and storage of resources 4/11/19;
   - type 80 (tribe, 0x004ec580): 17 building angle (set, add; multiply
     goes straight to type 70).
   Integer fields use ftol truncation. Returns 0 for an unknown class. */
int aoe_template_attribute_ror(DatUnit *unit_template, AoeAttributeOp op,
                               uint8_t attribute, float value);

/* Template vtable +0x04 used by upgrades: `slot` takes `source`'s fields
   class by class up to slot's class, but keeps its own ID (+0x10), enable
   byte (+0x52), editor-hide byte (+0x4e), task list (+0xd8, not copied by
   0x0043e6f0) and, when source's +0x4e is 1, its language name (+0x12);
   source's +0x4e is then set to 1 (0x00441f50). Name, damage graphics,
   attacks and armors are deep-copied. Returns 0 on allocation failure
   (slot unchanged). */
int aoe_template_upgrade_copy_ror(DatUnit *slot, DatUnit *source);

typedef struct {
    void *context;
    const DatGraphics *graphics;
    /* +0xdc(owner, 0, -1) then +0xd8(owner, 0, radius) for a placed unit
       in state 2. */
    int (*vision_reset)(void *context, AoeLiveUnit *unit, int32_t radius);
    /* Unit +0x38. */
    int (*select_graphic)(void *context, AoeLiveUnit *unit,
                          const DatGraphic *graphic);
    /* Unit +0x28 (state-2 damage graphics, 0x004a9240). */
    int (*damage_graphics)(void *context, AoeLiveUnit *unit);
    /* +0x184 list +0x14 (0x004045d0): every action's +0x40 with the new
       template (types 40..80). */
    void (*retask)(void *context, AoeLiveUnit *unit,
                   const DatUnit *new_template);
    /* 0x004ad550: building adjacency refresh. */
    void (*adjacent)(void *context, AoeLiveUnit *unit);
    /* Owner +0x50, for type 70's attribute 101. */
    AoePlayerResources *owner_resources;
} AoeLiveAttributeOps;

/* Live +0x48/+0x4c/+0x50, run before the template changes:
   - root (0x004a9fd0/0x004aa050/0x004aa0e0): attribute 0 rescales HP to
     the new maximum (int16-truncated), attribute 1 resets the vision of a
     placed state-2 unit with the new radius;
   - type 70 set (0x004ae820): attribute 101 moves storage contributions of
     resources 4 (+), 11 and 19 (-) from the old amount to `value`;
   - type 80 set (0x004ad350): attribute 17 writes +0x35 instead. */
int aoe_live_unit_attribute_ror(AoeLiveUnit *unit, AoeAttributeOp op,
                                uint8_t attribute, float value,
                                const AoeLiveAttributeOps *ops);

/* Live +0x58 before an upgrade copies `new_template` into the unit's
   template slot: type 80 (0x004ad960) remaps the construction graphic and
   refreshes adjacency; 50..80 (0x00426d70) the attack graphic; 40..80
   (0x004065c0) retask their actions; 30..80 (0x0044ef40) the walking
   graphics; root (0x004aa210) resets vision, rescales HP by the old/new
   maximum, remaps standing/dying/undead graphics and reruns the damage
   graphics before and after (the template pointer itself is unchanged). */
int aoe_live_unit_upgrade_ror(AoeLiveUnit *unit, const DatUnit *new_template,
                              const AoeLiveAttributeOps *ops);

typedef struct {
    void *context;
    AoePlayerTemplates *templates;   /* player +0x24 */
    AoePlayerResources *resources;   /* player +0x50 */
    /* Player list +0x28 (awake units) in order. */
    size_t (*unit_count)(void *context);
    AoeLiveUnit *(*unit_at)(void *context, size_t index);
    /* Player +0x78(type, amount, 0). */
    void (*resource_add)(void *context, int16_t type, float amount);
    AoeResearchCompletionManagerRor *research; /* player +0x224 */
    const AoeLiveAttributeOps *live;
} AoeEffectTargetOps;

/* Tribe effect apply 0x004ebb10 (effects vtable +0x08): command type 102
   disables tech ftol(amount) (0x004f1870(id, 0)), then the base
   interpreter 0x0043b940 runs types 0 (set attribute, 0x0045d6a0), 1
   (resource: set 0x0045cfc0 when b is 0, else +0x78), 2 (enable byte b of
   template a, 0x0045d010), 3 (upgrade a to b, 0x0045d9a0), 4 (add,
   0x0045d7a0) and 5 (multiply, 0x0045d8a0). Attribute commands target
   template a when it exists, else every template of class b (when b >=
   0), applying the live hook to each awake unit of that template first.
   Out-of-range effect IDs do nothing. Returns 0 for missing
   dependencies or an allocation failure. */
int aoe_effect_apply_ror(const DatEffects *effects, int16_t effect_id,
                         const AoeEffectTargetOps *ops);

/* Effects vtable +0x0c (0x004ebb90): only the tech-disable commands (102)
   of the effect. Used for civ tech trees during scenario player load. */
int aoe_effect_apply_disables_ror(const DatEffects *effects, int16_t effect_id,
                                  AoeResearchCompletionManagerRor *research);

/* One attribute command as player 0x0045d6a0/0x0045d7a0/0x0045d8a0. */
int aoe_effect_attribute_ror(const AoeEffectTargetOps *ops, AoeAttributeOp op,
                             int16_t unit, int16_t unit_class,
                             uint8_t attribute, float amount);

/* 0x004eb7a8: completes the unfinished (state 0..2) prerequisites of a
   tech recursively, then the tech itself (0x004eaa3d). */
int aoe_research_complete_with_prerequisites_ror(
    AoeResearchCompletionManagerRor *manager, const DatTechs *techs,
    int16_t research_id);
/* 0x004eb849: completes a tech, then every tech listing it as a
   prerequisite (with their prerequisites), once per matching slot. */
int aoe_research_complete_dependents_ror(
    AoeResearchCompletionManagerRor *manager, const DatTechs *techs,
    int16_t research_id);
/* Player 0x004f18a0(step) with the age techs in owner resources 23, 24 and
   25 (Bronze, Iron, Tool): 25 completes Tool; 23 advances to Bronze; 24 to
   Iron; 1 completes Iron with its dependents. Later ages are disabled
   while earlier ones complete their dependents. Other steps do nothing. */
int aoe_research_advance_age_ror(AoeResearchCompletionManagerRor *manager,
                                 const DatTechs *techs,
                                 const AoePlayerResources *owner,
                                 int step);

/* New-game research manager 0x004ea91c: one zeroed entry per tech. */
int aoe_research_manager_init_ror(AoeResearchCompletionManagerRor *manager,
                                  const DatTechs *techs);
void aoe_research_manager_free_ror(AoeResearchCompletionManagerRor *manager);

/* 0x004eb38d: every tech in state 0/1 whose satisfied prerequisites
   (state 3) reach its required count completes when research_time <=
   progress (0x004eaa3d, recursing through the manager's rebuild) or
   becomes available (1); otherwise it drops to 0. With nothing available
   or researching, owner attribute 39 becomes 1.0 if it was 0. */
int aoe_research_rebuild_ror(AoeResearchCompletionManagerRor *manager,
                             const DatTechs *techs,
                             AoePlayerResources *owner_resources);

/* 0x004f1870(id, enable): disable (0x004eb8e9) sets state -1 unless done;
   enable (0x004eb97f) turns -1 back into 0. No rebuild. */
int aoe_research_set_enabled_ror(AoeResearchCompletionManagerRor *manager,
                                 int16_t research_id, int enable);

#endif
