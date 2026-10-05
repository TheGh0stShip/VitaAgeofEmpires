#ifndef VITA_AOE_TRIBE_PLAYER_COMMANDS_H
#define VITA_AOE_TRIBE_PLAYER_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#include "tribe_command_builders.h"

/* Patched Rise of Rome 1.0a tribe player command methods
   (0x004f10e0..0x004f2d60): the player turns an interface request into a
   command for its selection (0x0045e0c0 with the filters noted) and
   queues it (the builders' calloc + queue); the AI module relays. The
   selection, the player's templates and the queue are ops. */

typedef struct {
    void *context;
    int16_t player;                     /* +0x4a */
    /* 0x0045e0c0(min +0x8d, class, template, +0x8f): up to 25 units. */
    int16_t (*collect)(void *context, int16_t min_8d, int16_t cls, int16_t tmpl,
                       int16_t f8f, const AoeTribeCommandUnitRor **out);
    /* A template (+0x24[id]): 0 absent, 1 present, 2 available (+0x52). */
    int (*template_state)(void *context, int16_t id);
    /* The acknowledgements: template 118's +0x2c, a unit's +0xac. */
    void (*ack_builder)(void *context);
    void (*ack_unit)(void *context, const AoeTribeCommandUnitRor *unit);
    /* The selected object (+0x134). */
    const AoeTribeCommandUnitRor *(*selected)(void *context);
    uint8_t (*object_type)(void *context, const AoeTribeCommandUnitRor *o);  /* template +4 */
    int16_t (*object_class)(void *context, const AoeTribeCommandUnitRor *o); /* template +0x14 */
    int (*queue)(void *context, const uint8_t *command, size_t size);
} AoeTribePlayerOpsRor;

/* 0x004f10e0 (+?): build `id` at (x, y) with the selected villagers
   (+0x8d >= 4, class 4), when the template is available or is 0x6d:
   template 118 acknowledges, then opcode 0x66. 0x004f1170: the same for
   opcode 0x69 (a..d), the template must be available. */
int aoe_tribe_player_build_ror(const AoeTribePlayerOpsRor *ops, int16_t id, float x, float y);
int aoe_tribe_player_build_69_ror(const AoeTribePlayerOpsRor *ops, int16_t id, int32_t a,
                                  int32_t b, int32_t c, int32_t d);
/* 0x004f1410 / 0x004f1200: train / research `item` at the selected
   building (type 'P' = 80). */
int aoe_tribe_player_train_ror(const AoeTribePlayerOpsRor *ops, int16_t item);
int aoe_tribe_player_research_ror(const AoeTribePlayerOpsRor *ops, int16_t item);
/* 0x004f1240: delete the selected object (ours, type 80 or 70, class not
   1); 0x004f1280 the given one (ours, type 80 or 70): opcode 0x6a. */
int aoe_tribe_player_delete_selected_ror(const AoeTribePlayerOpsRor *ops);
int aoe_tribe_player_delete_ror(const AoeTribePlayerOpsRor *ops,
                                const AoeTribeCommandUnitRor *object, int ours);
/* 0x004f12c0 (+0x8f 5) opcode 0x6d value; 0x004f1350 (+0x8f 4) opcode
   0x6b (a, b); 0x004f1580 (+0x8f 7) opcode 0x6f (x, y): the first unit
   acknowledges. 0x004f1330: opcode 0x6d for one raw ID. */
int aoe_tribe_player_6d_ror(const AoeTribePlayerOpsRor *ops, uint32_t value);
int aoe_tribe_player_6d_one_ror(const AoeTribePlayerOpsRor *ops, int32_t id, uint32_t value);
int aoe_tribe_player_6b_ror(const AoeTribePlayerOpsRor *ops, uint32_t a, uint32_t b);
int aoe_tribe_player_6f_ror(const AoeTribePlayerOpsRor *ops, float x, float y);
/* 0x004f2b60: opcode 0x66 for one raw ID (value, x, y, +0x10);
   0x004f2d60: opcode 0x67 sub 0 (the player, b, value). */
int aoe_tribe_player_66_one_ror(const AoeTribePlayerOpsRor *ops, int32_t id, int16_t value,
                                float x, float y, int32_t field_10);
int aoe_tribe_player_67_ror(const AoeTribePlayerOpsRor *ops, int16_t b, int16_t value);
/* 0x004f20c0: a template is usable (available, or ID 0x6d present... as
   the original: absent or unavailable fail unless the ID is 0x6d). */
int aoe_tribe_player_template_usable_ror(const AoeTribePlayerOpsRor *ops, int16_t id);
/* 0x004f20f0: tech `i`'s state (tech tree +0x224, 8-byte entries, +4) is
   1. 0x004f2110: a research state (0x004ea9ac) other than 3 and -1. */
int aoe_tribe_tech_state_is_one_ror(const uint8_t *entries, int32_t i);
int aoe_tribe_research_open_ror(int16_t state);

#endif
