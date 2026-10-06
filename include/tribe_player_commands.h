#ifndef VITA_AOE_TRIBE_PLAYER_COMMANDS_H
#define VITA_AOE_TRIBE_PLAYER_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#include "player_outcome.h" /* AoeOutcomeStream */
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
/* 0x004e1ae0 (the computer player's): the player's object with this
   ID (0x0040bab0), deleted as above; 1 when that queued. */
int aoe_tribe_player_delete_id_ror(const AoeTribePlayerOpsRor *ops,
                                   const AoeTribeCommandUnitRor *(*lookup)(void *context,
                                                                         int32_t id),
                                   int32_t id);
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

/* The RoR tribe player's own slots (vtable 0x00549a44, also
   0x00549b80) that forward to its computer player (+0x23c), the
   production queue, the command builder or the score history. */
typedef struct {
    void *context;
    int has_ai;        /* +0x23c set */
    int16_t player_id; /* +0x4a */
    void (*ai_kick)(void *context, int32_t kind);                  /* 0x004c7ff0 */
    int32_t (*ai_strategic)(void *context, int32_t i);             /* 0x004d77d0 */
    void (*ai_log)(void *context, const char *line);               /* 0x0040c240 */
    void (*ai_unit_added)(void *context, int32_t id);              /* AI +0x44 */
    void (*add_unit)(void *context, void *unit, int32_t a, int32_t b, int32_t c); /* 0x0045e8d0 */
    int (*unit_info)(void *context, void *unit, int16_t *unit_class, int32_t *id); /* +0x08 +0x14, +0x04 */
    void (*queue_train)(void *context, int16_t player, int32_t a, int32_t b, int32_t c);    /* 0x004e94e0 */
    void (*queue_research)(void *context, int16_t player, int32_t a, int32_t b, int32_t c); /* 0x004e9580 */
    void (*order_ids)(void *context, int16_t player, const int32_t *ids, int32_t count, int32_t a,
                      int32_t b, int32_t c, int32_t d);              /* 0x0042b970 */
    void (*tribute)(void *context, int16_t player, int32_t a, int32_t resource, float amount,
                    int32_t b);                                      /* world +0x58 +0x14 */
    void (*sample)(void *context, int32_t skip, int32_t value);    /* 0x004b0c00 */
} AoeTribeForwardOpsRor;

/* +0x38 0x004f29d0: the AI's kick; +0x3c 0x004f29f0: its strategic
   number (-1 without an AI); +0xe4 0x004f2a20: a formatted AI log line
   (1024 bytes). +0xe0 0x004f2b10: the base add (0x0045e8d0), then the AI
   hears of a unit with an ID that is not class 11. +0x108 0x004f2c20 /
   +0x114 0x004f2c80: train / research with the first two arguments
   swapped. +0x120 / +0x124 0x004f2d10: an order for one raw ID.
   +0xf8 0x004f13c0: a tribute when the resource is set and the amount
   nonzero (1), else 0. 0x004f1aa0: a score history sample of the
   +0x50 record's +0x2c (truncated; 0 when negative). */
void aoe_tribe_ai_kick_ror(const AoeTribeForwardOpsRor *ops, int32_t kind);
int32_t aoe_tribe_ai_strategic_ror(const AoeTribeForwardOpsRor *ops, int32_t i);
void aoe_tribe_ai_log_ror(const AoeTribeForwardOpsRor *ops, const char *format, ...);
void aoe_tribe_add_unit_ror(const AoeTribeForwardOpsRor *ops, void *unit, int32_t a, int32_t b,
                            int32_t c);
void aoe_tribe_queue_train_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t b, int32_t c);
void aoe_tribe_queue_research_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t b, int32_t c);
void aoe_tribe_order_one_ror(const AoeTribeForwardOpsRor *ops, int32_t id, int32_t a, int32_t b,
                             int32_t c, int32_t d);
int aoe_tribe_tribute_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t resource,
                          float amount, int32_t b);
void aoe_tribe_sample_score_ror(const AoeTribeForwardOpsRor *ops, float score);

/* +0x64 0x004f0280: the +0x50 record's dwords +0x00, +0x04, +0x0c,
   +0x08 (in that order) after the empty base save (0x004ed890). */
void aoe_tribe_save_record_ror(const int32_t record[4], AoeOutcomeStream *s);

/* 0x004f0ac0: after unit `after` (when given) the next unit of
   template `id` (+0x10) in the player's list that is below state 3;
   otherwise the first one in state 2; NULL when none. */
typedef struct {
    void *unit;
    int16_t template_id;
    uint8_t state;
} AoeTribeListUnitRor;
void *aoe_tribe_next_of_template_ror(const AoeTribeListUnitRor *units, int32_t count, int16_t id,
                                     const void *after);
/* Vtable 0x00549b80 +0x7c 0x004f7360: the 30-step cycle counter (+0x850)
   advances once per whole second of world time accumulated at +0x84c,
   then the base list update (0x0045d040). */
void aoe_tribe_cycle_tick_ror(float *seconds, int32_t *counter, float tick);
/* +0x70 0x004f73d0: after the base block load (0x0045c6a0), an
   attribute 50 above 0 sets +0x104 and refreshes the world's shared
   vision (0x00520c60). Returns whether it did. */
int aoe_tribe_shared_vision_check_ror(float attribute_50, int32_t *flag_104,
                                      void (*refresh)(void *context), void *context);

/* Relays of the RoR tribe player (vtable 0x00549a44, also 0x00549b80)
   that do nothing without a computer player (+0x23c); the targets are
   the computer player's message handler, its build list (+0x104) and
   its tactical module (+0x1146c). */
typedef struct {
    void *context;
    int has_ai; /* +0x23c set */
    int32_t (*ai_message)(void *context, int32_t a, int32_t b, int32_t c, int32_t d,
                          int32_t e);                                      /* 0x004c86a0 */
    void (*build_place)(void *context, int32_t a, int32_t b, int32_t c, float x, float y,
                        int32_t d, int32_t e);                             /* 0x004b8ce0 */
    void (*build_named)(void *context, int32_t a, int32_t b);              /* 0x004b8b40 */
    void (*build_done_4)(void *context, int32_t a, int32_t b, int32_t c, int32_t d); /* 0x004b8f40 */
    void (*build_done_3)(void *context, int32_t a, int32_t b, int32_t c);  /* 0x004b8fd0 */
    void (*tactical)(void *context, int32_t a, int32_t b, int32_t c);      /* 0x004e1a80 */
} AoeTribeAiRelayOpsRor;

/* +0x54 0x004f2a70: the AI's message result, 0 without one. +0x100
   0x004f2bb0, +0x104 0x004f2bf0, +0x110 0x004f2c50 and +0x11c 0x004f2ce0
   pass their arguments to the build list; +0x5c 0x004f6b50 to the
   tactical module. */
int32_t aoe_tribe_ai_message_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                                 int32_t c, int32_t d, int32_t e);
void aoe_tribe_build_place_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                               int32_t c, float x, float y, int32_t d, int32_t e);
void aoe_tribe_build_named_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b);
void aoe_tribe_build_done_4_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                                int32_t c, int32_t d);
void aoe_tribe_build_done_3_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                                int32_t c);
void aoe_tribe_tactical_relay_ror(const AoeTribeAiRelayOpsRor *ops, int32_t a, int32_t b,
                                  int32_t c);


/* The RoR tribe player's own saved fields and members (0x00549a44 /
   0x00549b80). */
typedef struct {
    uint8_t b48;                         /* +0x48 */
    int32_t v228, v22c, v230, v234, v238; /* +0x228..+0x238 */
    int32_t v844, v848;                  /* +0x844, +0x848 */
    int32_t type;                        /* +0x04 (1: computer) */
    /* 0x00549b80 only: +0x84c, +0x850, +0x854, +0x858. */
    int32_t v84c, v850, v854, v858;
} AoeTribePlayerSaveRor;

typedef struct {
    void *context;
    int has_ai;                                                /* +0x23c set */
    void (*base_save)(void *context, AoeOutcomeStream *s);     /* 0x0045d0d0 */
    void (*research_save)(void *context, AoeOutcomeStream *s); /* 0x004eb59f (+0x224) */
    void (*history_save)(void *context, AoeOutcomeStream *s);  /* 0x004b0a60 (+0x840) */
    int32_t (*is_host)(void *context);                         /* 0x00423c20 */
    void (*ai_save)(void *context, AoeOutcomeStream *s);       /* AI vtable +0x3c */
} AoeTribePlayerSaveOpsRor;

/* 0x004eff20 (vtable +0x84): the base player save, +0x48 (1 byte),
   +0x22c, +0x234, +0x228, +0x238, the research states, +0x230, the score
   history, +0x844, +0x848; for a computer player (type 1) a 4-byte 1 and
   the AI when it has one and the comm handler is the host, else 0.
   0x004f7300 (0x00549b80's +0x84): that, then +0x84c, +0x850, +0x858,
   +0x854. */
void aoe_tribe_player_save_ror(const AoeTribePlayerSaveRor *p, AoeOutcomeStream *s,
                               const AoeTribePlayerSaveOpsRor *ops);
void aoe_tribe_player_save_timed_ror(const AoeTribePlayerSaveRor *p, AoeOutcomeStream *s,
                                     const AoeTribePlayerSaveOpsRor *ops);

typedef struct {
    void *context;
    void *research;  /* +0x224 */
    void *ai;        /* +0x23c */
    void *history;   /* +0x840 */
    void (*research_free)(void *context, void *research); /* 0x004ea982, delete */
    void (*ai_delete)(void *context, void *ai);           /* vtable +0x00 (1) */
    void (*history_free)(void *context, void *history);   /* 0x004b0a20, delete */
    void (*base_destroy)(void *context);                  /* 0x0045ba50 */
} AoeTribePlayerMembersRor;

/* 0x004efdc0 (thunk 0x004f7280; deleting 0x004efae0 / 0x004f7260):
   research states, the AI, the history (each cleared), then the base
   player. */
void aoe_tribe_player_destroy_ror(AoeTribePlayerMembersRor *m);


typedef struct {
    void *context;
    int has_ai; /* +0x23c set */
    int32_t (*base_add)(void *context, void *unit, int32_t a, int32_t b); /* 0x0045e880 */
    int (*unit_info)(void *context, void *unit, int16_t *unit_class, int32_t *id); /* +0x08 +0x14, +0x04 */
    void (*ai_object_added)(void *context, void *unit);     /* AI vtable +0x40 */
    void (*build_event)(void *context, void *unit, int32_t arg); /* 0x004b8a70 */
    uint8_t (*game_state)(void *context);                   /* player +0x3c's +0x1d */
} AoeTribeAddObjectOpsRor;

/* 0x004f2aa0 (vtable +0xdc): the base add (0x0045e880); then, for a unit
   not of class 11 with an AI and a nonnegative ID, the AI hears of the
   object and the build list gets its event with 1, or 0 while the game
   state byte is 2 or 3. Returns the base add's result. */
int32_t aoe_tribe_add_object_ror(const AoeTribeAddObjectOpsRor *ops, void *unit, int32_t a,
                                 int32_t b);

/* Vtable 0x00549a44 +0x12c 0x004ef7f0(kind, stream, player): a saved
   object rebuilt by its type byte: 'F' (70, a 0x1c0-byte combat unit,
   load constructor 0x004ae380) and 'P' (80, a 0x1fc-byte building,
   0x004ac670) here, every other kind through the base player's loader
   (0x004516e0). NULL when the allocation fails. */
typedef struct {
    void *context;
    void *(*load_combat)(void *context, void *stream, void *player);   /* 0x004ae380 */
    void *(*load_building)(void *context, void *stream, void *player); /* 0x004ac670 */
    void *(*load_base)(void *context, uint8_t kind, void *stream, void *player); /* 0x004516e0 */
} AoeTribeLoadObjectOpsRor;
void *aoe_tribe_load_object_ror(const AoeTribeLoadObjectOpsRor *ops, uint8_t kind, void *stream,
                                void *player);

/* Tribe player template loader 0x004f06d0(stream, slot, kind, a, b):
   'F' (0x164 bytes, 0x004eca90), 'P' (0x17c, 0x004ec180) and 'Z' (0xb8,
   0x004ed2c0) are built here and stored in the template table (+0x24)
   at `slot` (NULL when the allocation fails); every other kind goes to
   the base loader 0x0045cb70, which stores its own. */
typedef struct {
    void *context;
    void **templates;           /* +0x24 */
    void *(*load_f)(void *context, void *stream, int32_t a, int32_t b); /* 0x004eca90 */
    void *(*load_p)(void *context, void *stream, int32_t a, int32_t b); /* 0x004ec180 */
    void *(*load_z)(void *context, void *stream, int32_t a, int32_t b); /* 0x004ed2c0 */
    void (*load_base)(void *context, void *stream, int16_t slot, uint8_t kind, int32_t a,
                      int32_t b);                                       /* 0x0045cb70 */
} AoeTribeTemplateLoadOpsRor;
void aoe_tribe_template_load_ror(const AoeTribeTemplateLoadOpsRor *ops, void *stream,
                                 int16_t slot, uint8_t kind, int32_t a, int32_t b);

/* 0x004f1450(target): repair. A building ('P') of ours or of a player we
   are allied to (vtable +0x1c), below its maximum hit points (template
   +0x26 vs unit +0x30) and in state (+0x48) 0..2 qualifies directly;
   otherwise another player's object needs the alliance both ways and any
   object must be of class 2, 20, 21 or 22, damaged and in state 0..2.
   Then the selected villagers (min +0x8d 4, class 4): none gives 0; the
   first one's task for the target (+0xbc(target, 0, 0, 0)) acknowledges
   (+0x2c, `ack`) and opcode 0x6e (0x004e9b20) is queued. */
typedef struct {
    const AoeTribeCommandUnitRor *unit;
    uint8_t type;      /* template +4 */
    int16_t cls;       /* template +0x14 */
    float hp;          /* +0x30 */
    int16_t max_hp;    /* template +0x26 */
    uint8_t state;     /* +0x48 */
} AoeTribeRepairTargetRor;
int aoe_tribe_player_repair_ror(const AoeTribePlayerOpsRor *ops,
                                const AoeTribeRepairTargetRor *target,
                                int (*allied)(void *context, int16_t from, int16_t to),
                                void (*ack)(void *context, const AoeTribeCommandUnitRor *unit,
                                            const AoeTribeCommandUnitRor *target));


/* 0x004f04e0 (part of the tribe player load): the template count (+0x22,
   int16) and the +0x24 table of flags (4 bytes each); every nonzero slot
   reads a kind byte and makes the template through 0x004f06d0 (its
   kind: 'F' 0x004eca90, 'P' 0x004ec180, 'Z' 0x004ed2c0, others the base
   0x0045cb70); then the tile map (+0x38, 0x00517490), the object memory
   (+0x114, 0x00516e50), the three object lists +0x28 / +0x2c / +0x30
   (0x00450ef0) and two list loads (0x00451680; a third from version
   7.09). */
typedef enum {
    AOE_TRIBE_TEMPLATE_BASE = 0, AOE_TRIBE_TEMPLATE_F, AOE_TRIBE_TEMPLATE_P, AOE_TRIBE_TEMPLATE_Z
} AoeTribeTemplateKindRor;
AoeTribeTemplateKindRor aoe_tribe_template_kind_ror(uint8_t kind);

typedef struct {
    void *context;
    void (*load_template)(void *context, int16_t slot, uint8_t kind, AoeTribeTemplateKindRor which,
                          AoeOutcomeStream *s);
    void (*load_tile_map)(void *context, AoeOutcomeStream *s);
    void (*load_object_memory)(void *context, AoeOutcomeStream *s);
    void (*new_list)(void *context, int which);
    void (*load_list)(void *context, int which, AoeOutcomeStream *s);
} AoeTribePlayerLoadOpsRor;
/* Returns the template count read (-1 on a short stream). */
int32_t aoe_tribe_player_load_templates_ror(AoeOutcomeStream *s, float version,
                                            const AoeTribePlayerLoadOpsRor *ops);

#endif
