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

/* 0x004f34c0: AI/notification message router. The object for `id`
   (0x005206d0) must exist and the game must be live (0x00407a60) unless
   the code is 0x20a, 0x72 or 0x74. Codes:
   0x1f9 with a == 0x25d: when `id` is ours (0x004e0980) and the current
     mode (0x00454240) is 0x6b, 0x004e04b0(0x00451ab0()).
   0x201 / 0x72 / 0x74: forwarded unchanged to the AI (+0x23c, +0x1146c,
     vtable +0x40).
   0x202 with a == 0x25d or 0x26f: if 0x004bc690(id) and 0x004bc2d0(id,
     object x +0x38, y +0x3c) yields a point (returns 1), 0x004db6a0(id,
     px, py); otherwise 0x004d7790(id).
   0x209: when object `a` exists, 0x0040aa20(its template +0x4a, 0, 10).
   0x20a: 0x004c8300(id, a, b).  0x20d: 0x004be0d0(a). */
typedef struct {
    void *context;
    int (*object_exists)(void *context, int32_t id, float *x, float *y);
    int16_t (*object_template_id)(void *context, int32_t id);
    int (*game_live)(void *context);
    int (*is_ours)(void *context, int32_t id);
    int32_t (*mode)(void *context);
    void (*mode_6b_action)(void *context);
    void (*forward_ai)(void *context, int32_t id, int32_t from, int32_t code, int32_t a,
                       int32_t b, int32_t c);
    int (*can_retarget)(void *context, int32_t id);
    int (*find_point)(void *context, int32_t id, float x, float y, int32_t *px, int32_t *py);
    void (*goto_point)(void *context, int32_t id, int32_t px, int32_t py);
    void (*fallback)(void *context, int32_t id);
    void (*flash)(void *context, int16_t template_id, int32_t a, int32_t b);
    void (*on_20a)(void *context, int32_t id, int32_t a, int32_t b);
    void (*on_20d)(void *context, int32_t a);
} AoeTribeMessageOpsRor;
void aoe_tribe_player_message_ror(const AoeTribeMessageOpsRor *ops, int32_t id, int32_t from,
                                  int32_t code, int32_t a, int32_t b, int32_t c);

/* 0x004f0030: the tribe player's per-tick update. World time (+0x84 of
   the world) accumulates at +0x238; for each whole second: the 15-step
   counter +0x230 advances and on wrapping samples the score (record
   +0x2c, 0x004b0c00 as in 0x004f1aa0); record +0x58 = 100 when
   0x0041b7c0 is set, else explored fraction (0x00517810) * 100; +0xb0 =
   +0x50 - (+0x4c - +0x2c) floored at 0; +0x24 += +0x28 capped at 100;
   the countdowns +0x22c and +0x234 drop towards 0; a player above 0
   with no state-2 unit outside classes 1, 2, 11, 20, 21, 27, 30, 37
   is told vtable +0x04(2) and +0xb4 is cleared. Then the base update
   (0x0045d040), record +0x00 -= +0x84 * +0x2c * dt / 60 floored at 0,
   the AI (+0x23c) update when 0x004253e0 is 1 (preceded by
   0x005204d0(3) when world +0xfc is -1; AI vtable +0x30(0)), and
   0x0044a850 when the global 0x00582edc is set. */
typedef struct {
    float score;      /* +0x2c */
    float explored;   /* +0x58 */
    float r50, r4c;   /* +0x50, +0x4c */
    float margin;     /* +0xb0 */
    float r24, r28;   /* +0x24, +0x28 */
    float r00, r84;   /* +0x00, +0x84 */
    int32_t rb4;      /* +0xb4 */
} AoeTribeTickRecordRor;
typedef struct {
    int16_t cls;
    uint8_t state;
} AoeTribeTickUnitRor;
typedef struct {
    float accum;        /* +0x238 */
    int32_t counter;    /* +0x230 */
    int32_t down_22c;   /* +0x22c */
    int32_t down_234;   /* +0x234 */
    int16_t player_id;  /* +0x4a */
    int has_ai;         /* +0x23c */
    AoeTribeTickRecordRor *rec; /* +0x50 */
    const AoeTribeTickUnitRor *units; /* +0x28 list */
    int32_t unit_count;
} AoeTribeTickStateRor;
typedef struct {
    void *context;
    void (*sample)(void *context, int32_t skip, int32_t value);
    int (*full_explore)(void *context);
    float (*explored_fraction)(void *context);
    void (*notify)(void *context, int32_t kind);
    void (*base_update)(void *context);
    int (*ai_ready)(void *context);
    int32_t (*world_fc)(void *context);
    void (*world_signal)(void *context, int32_t v);
    void (*ai_update)(void *context, int32_t v);
    int (*global_flag)(void *context);
    void (*global_hook)(void *context);
} AoeTribeTickOpsRor;
void aoe_tribe_player_tick_ror(AoeTribeTickStateRor *p, float dt, const AoeTribeTickOpsRor *ops);


/* 0x004f0db0: the units a building can train, for a list: every template
   slot whose kind byte (+4) is above 'E', enabled (+0x52) and trained at
   this building (+0x15c) becomes a 0x28-byte entry: the template's name
   (+8), the slot, its +0x4c, the two first cost types and three amounts
   (0x004f0830 with scale 1.0), and the AI help / tree text (0x0040c1a0 /
   0x00407ae0). Building 0x76 with attribute 48 at 0.0 and template 0x6d
   disabled adds a 0x6d entry when the player owns no 0x6d object. The
   previous list is freed first; `*count` tells how many. */
typedef struct {
    const char *name;
    int16_t id;
    int16_t field_4c;
    int16_t cost_type[3];
    float cost_amount[3];
    int32_t help, tree_text;
} AoeTribeTrainEntryRor;
typedef struct {
    void *context;
    int16_t template_count;                                        /* +0x22 */
    /* Slot i: 0 when empty; else its kind byte, enabled byte, +0x15c, +0x4c and name. */
    int (*template_info)(void *context, int16_t slot, uint8_t *kind, uint8_t *enabled,
                         int16_t *trained_at, int16_t *field_4c, const char **name);
    void (*costs)(void *context, int16_t slot, int16_t types[3], int16_t amounts[3]);
    int32_t (*help)(void *context, int16_t slot);
    int32_t (*tree_text)(void *context, int16_t slot);
    float (*attribute_48)(void *context);                          /* resources +0xc0 */
    int (*owns_template)(void *context, int16_t id);               /* an object of it in +0x28 */
} AoeTribeTrainListOpsRor;
void aoe_tribe_player_train_list_ror(const AoeTribeTrainListOpsRor *o, int16_t building,
                                     AoeTribeTrainEntryRor **list, int16_t *count);

/* 0x004f6770: the player's debug dump, one line per call of `line`:
   "Player %d:", with `full` the civilisation (string 0x27f6 + civ) as
   "Human Player: %s" or, for a computer, "Computer Player: %s", "Build
   List: %s", "City Plan: %s", "VC File: %s"; then "Points:", "Combat %d",
   "Economy %d", "Religion %d", "Science %d" (category points), "Survival
   %d", "Wonder %d" (event points 0x16 / 0x17), "Total: %d" (the score),
   "Population: %d" (attribute 11), "Kills: %d", "Kill Ratio: %d",
   "Razings: %d" (samples 0, 2, 1), "Food: %d", "Gold: %d", "Stone: %d",
   "Wood: %d" (attributes 0, 3, 2, 1), "Technology: %d", "Exploration:
   %d" (samples 0x11, 7); for a computer the build AI line "Build AI
   (last %s, last2 %s, Progress %s, Phase %s)" and the civilian counts
   "Civilians (Total %d, Exp %d %d %d %d %d, Need %d %d %d %d)" and
   "Number Soldiers %d". */
typedef struct {
    void *context;
    int16_t player;
    int is_computer;
    void (*civ_name)(void *context, char *out, int size);
    const char *(*build_list)(void *context);
    const char *(*city_plan)(void *context);
    const char *(*vc_file)(void *context);
    int32_t (*points)(void *context, int category);
    int32_t (*event_points)(void *context, int index);
    int32_t (*samples)(void *context, int index);
    int32_t (*score)(void *context);
    float (*attribute)(void *context, int index);
    void (*build_ai)(void *context, char *out, int size);
    void (*civilians)(void *context, char *out, int size);
    int32_t (*soldiers)(void *context);
    void (*line)(void *context, const char *text);
} AoeTribeDumpOpsRor;
void aoe_tribe_player_dump_ror(const AoeTribeDumpOpsRor *o, int full);


/* ---- 0x004f6de0: the command and cursor for a target ---- */

/* What 0x004f6de0 reads from the target object (0x005206d0 by ID): a
   class 0x19 object reports through its +0x94 template and +0x98 owner,
   others through +0x08 and +0x0c -> +0x4a. */
typedef struct {
    int16_t unit_class;   /* template +0x14 */
    int16_t template_id;  /* template +0x10 */
    int16_t max_hp;       /* template +0x26 */
    int32_t owner;        /* the owning player's ID */
    uint8_t state;        /* +0x48 */
    float hp;             /* +0x30 */
} AoeTribeCursorTargetRor;

/* A row of the cursor tables (game +0xa24 / +0xa28 and +0xa2c / +0xa30,
   0x005060b0): the class, then an (action, cursor) pair per stance:
   stance 1 at [1..2], 0 at [3..4], 2 at [5..6], 3 at [7..8], 4 at
   [9..10]. */
typedef struct { int32_t v[11]; } AoeTribeCursorRowRor;

typedef struct {
    void *context;
    int (*target)(void *context, int32_t id, AoeTribeCursorTargetRor *out);
    int16_t player_count;              /* world (+0x3c) +0x3c */
    void (*refresh_flags)(void *context); /* vtable +0x98 */
    int16_t selection_count;           /* +0x1a0 */
    uint32_t *class_flags;             /* +0x19c, cleared on a retry */
    const AoeTribeCursorRowRor *flagged; int32_t flagged_count; /* +0xa24, +0xa28 */
    const AoeTribeCursorRowRor *plain;   int32_t plain_count;   /* +0xa2c, +0xa30 */
    int32_t (*stance)(void *context, int32_t owner); /* +0x8c + 4 owner */
    int32_t priests, last_priest;      /* 0x007c04f4, 0x007c04f0 */
    int convert_units, convert_buildings; /* +0x50 -> +0x6c / +0x70 non-zero */
} AoeTribeCursorOpsRor;

/* 0x004f6de0: the command (`*command`) and cursor (`*cursor`) when the
   selection points at object `id`. Both start at 0; 0 is returned for a
   missing object, an owner outside the player count, a stance above 4
   or a class without a row. The table is the flagged one while the
   selection is non-empty and has class flags, else the plain one; the
   row is the class's own index when it holds that class, else a search.
   Template 0x9e / 0xa3 adds 0x32 to the cursor. The stance picks the
   row's action: 0 -> command 0; 1 / 8 -> 4 with flag bit 0 and a target
   state below 3, 8 otherwise retried as 7 with +0x32; 2 / 3 -> 5 with
   bit 1 / bit 2; 4 -> 5 with bit 4 unless the target is the single
   selected priest, needing 0 < hp < max; 5 -> 3 with bit 5 and 0 < hp
   < max (state 0 adds 0x32); 6 / 9 -> 3 with bit 6, 9 otherwise retried
   as 1 with +0x32; 7 -> with bit 7: class 0x12 needs unit conversion,
   class 3 building conversion and a template other than 0x6d / 0x114,
   any other class gives 5; other actions give 0 / 0. A flagged lookup
   that yields 0 / 0 clears the class flags and repeats with the plain
   table. */
int aoe_tribe_player_cursor_command_ror(const AoeTribeCursorOpsRor *o, int32_t id,
                                        int32_t *command, int32_t *cursor);


/* 0x004f1ae0: the player's start. Without units (+0x28 list empty) a
   random tile (x from the map width, y from the height: rand * (size -
   30) / 0x7fff + 15, plus 0.5) is tried up to 5000 times with the +0x24
   object's +0x1b4 type's +0x20(player, x + 0.5, y + 0.5, 0, 0, 1, 1, 1,
   1, 1); the first free one gets the type's +0x18(player, x, y, 0), the
   view (0x0045d310 tile, 0x0045d330 point), 0x004ad7b0(10000.0) and
   three +0x14c-type objects at random offsets (rand * 6 / 0x7fff - 3 plus
   rand / 0x7fff, inside the map, where that type's +0x20 answers 0). With
   units the view goes to the type-0x6d object (0x00451370) or the first
   unit. Networked (0x0041b760) or with 0x0041b710, every other player
   with a stance (vtable +0x1c) is shown that object (+0xd8 / +0xdc with
   0, -1) and, for a wonder, told through its +0x94(0x70, +0x118, +0x11c,
   0, 0); when any was and we are the viewed player (world +0x7c), each
   comm slot 1..8 holding such a player (0x0041c080) gets the stance as
   its recipient flag (0x0041ca50). */
typedef struct {
    void *context;
    int has_units;
    int32_t map_w, map_h;
    int32_t (*rand_)(void *context);                 /* 0..0x7fff */
    int (*main_blocked)(void *context, float x, float y);  /* +0x1b4 type +0x20 */
    void (*create_main)(void *context, float x, float y);  /* +0x1b4 type +0x18 */
    int (*extra_blocked)(void *context, float x, float y); /* +0x14c type +0x20 */
    void (*create_extra)(void *context, float x, float y); /* +0x14c type +0x18 */
    void (*set_view)(void *context, int32_t tx, int32_t ty, float x, float y);
    void (*add_work)(void *context, float amount);   /* 0x004ad7b0 */
    /* The type-0x6d object or the first unit: position and template id. */
    int (*anchor)(void *context, float *x, float *y, int16_t *template_id);
    int share;                                       /* 0x0041b760 || 0x0041b710 */
    int16_t player_count, own_id, viewed_player;
    int32_t (*stance)(void *context, int32_t player); /* vtable +0x1c */
    void (*reveal)(void *context, int32_t player);    /* +0xd8, +0xdc */
    void (*wonder_notice)(void *context, int32_t player); /* +0x94(0x70, ...) */
    int32_t (*slot_player)(void *context, int32_t slot);  /* 0x0041c080 */
    void (*chat_recipient)(void *context, int32_t slot, int32_t stance); /* 0x0041ca50 */
} AoeTribeStartOpsRor;
void aoe_tribe_player_place_start_ror(const AoeTribeStartOpsRor *o);


/* 0x004f2160 (+0x1c8, line): a computer player's debug status line into
   its +0x240 + 0x100 * line buffer (`out`, 256 bytes), or a constant.
   Without an AI (+0x23c): the world's current update player (+0xfc, a
   present player) gives "ConCP=%d, CurCP=%d, AveUAms=%3d, LastUAms=%3d,
   UA=%s." (world byte +0x11d, +0xfc, that AI's +0x1246c, +0x12468 and
   state name 0x004d0880), else "Invalid currentUpdateComputerPlayer...".
   1: "Build List:%s, City Plan:%s, BuildAI(C=%s, N=%s, L=%s, %d/%d)."
   (0x00408cf0, 0x0040a280, 0x00408fd0, 0x00408fe0, 0x00408fc0, 0x00408fb0,
   0x00408ce0). 2: "NeedRes: %s=%d, ... ResDiff: %s=%d, ..." for 0..3
   (resource name 0x004c8d10 of 0x004d7740(i), 0x004e2f30(i); then
   0x004d7760(i)). 3: "Gat(Act/Des): %s=(%d/%d), ...  #Civs=%d, #Sol=%d,
   #Boats=%d." (name, 0x004e2f60, 0x004e2fb0 for 0..3; 0x004d76c0,
   0x004d7710, 0x004d7730). 4: "Pers (Spec/BL): Food=(%d/%d), Wood=(%d/%d),
   Gold=(%d/%d), Stone=(%d/%d)." (strategic numbers 0x75 / 0x9c, 0x78 /
   0x9d, 0x76 / 0x9f, 0x77 / 0x9e via 0x004d77d0). 5: "Dip: P1(%d/%d/%d),
   ... P8(...)." (0x0040a9a0(p, 0..2)). 6: "Per: %s, AttEnab=%d, AttDly=%d
   Min, ConCP=%d, ..., UA=%s." (0x004cbfe0, AI +0x12284, number 0x68 / 60,
   then as line 0 for the current update player). Others: "Invalid status
   line number." */
typedef struct {
    void *context;
    int has_ai;                 /* +0x23c */
    int32_t current_player;     /* world +0xfc (-1 none) */
    int current_present;        /* its slot is non-NULL */
    uint8_t world_11d;
    int32_t cur_ave_ms, cur_last_ms; /* current AI +0x1246c, +0x12468 */
    const char *(*state_name)(void *context);  /* 0x004d0880 of the current AI */
    const char *build_list, *city_plan, *build_c, *build_n, *build_l; /* 0x00408cf0, 0x0040a280, 0x00408fd0, 0x00408fe0, 0x00408fc0 */
    int32_t build_current, build_count;        /* 0x00408fb0, 0x00408ce0 */
    const char *(*resource_name)(void *context, int32_t i);       /* 0x004c8d10(0x004d7740(i)) */
    int32_t (*need)(void *context, int32_t i);                     /* 0x004e2f30 */
    int32_t (*diff)(void *context, int32_t i);                     /* 0x004d7760 */
    int32_t (*gather_actual)(void *context, int32_t i);            /* 0x004e2f60 */
    int32_t (*gather_desired)(void *context, int32_t i);           /* 0x004e2fb0 */
    int32_t civs, soldiers, boats;                                 /* 0x004d76c0, 0x004d7710, 0x004d7730 */
    int32_t (*number)(void *context, int32_t id);                  /* 0x004d77d0 */
    int32_t (*diplomacy)(void *context, int32_t player, int32_t kind); /* 0x0040a9a0 */
    const char *sn_file;                                           /* 0x004cbfe0 */
    int32_t attack_enabled;                                        /* AI +0x12284 */
} AoeTribeStatusOpsRor;
const char *aoe_tribe_player_status_line_ror(const AoeTribeStatusOpsRor *o, int32_t line, char out[256]);


/* 0x004f2d80 (build_list, city_plan, rule_set, personality): the
   computer player's AI files. The random seed is saved (rand) and
   restored (srand) around the call; nothing else without an AI (+0x23c).
   The map type (0x00505310) is logged ("We have an all water map.",
   "... a mostly water map.", "... a land and water map.", "... a mostly
   land map.", "... an all land map.", else "We don't have a specific
   map type (i.e. this is a scenario)."). Strings 0x2775 and 0x277b (the
   random label) are read. A missing build list, or one equal to the
   random label, is chosen by 0x4f36e0 (map type, victory +0xa8c, map
   size; it also names a rule set) and loaded through the Build AI's
   +0x44 until one loads ("Loading default build list=%s." / "Loading
   random game selected build list=%s."); else "Loading build list=%s.".
   A missing or random city plan logs "Loading no city plan (influence
   placement will be used)." and uses "NONE", else "Loading city
   plan=%s."; the city plan object (AI +0x6c4) gets +0x40(name, map
   width, map height, 0, 0, 0). A missing or random rule set loads the
   chosen one (0x004cbff0) unless it is "RandomGameSpecialized" ("Rule
   set has been initialized with code-controlled options."); a given one
   "Loading rule set=%s.". "Setting difficulty level to %d." then
   0x004cc080 and 0x004c8210. Diplomacy (0x0040ab40): ourselves (0, 0)
   and (2, 100); an ally (+0x18) (0, 0x4a) and (2, 0x18); a neutral
   (+0x1c) (0, 0x18) and (2, 0x4a); then 0x0040ac80(p, neither); the
   personality preset (0x0040b2d0); a non-empty scenario +0x1010 name
   loads the learn file (0x004c3820). */
typedef struct {
    void *context;
    int has_ai;
    int32_t map_type, map_size, victory, map_w, map_h, difficulty;
    int16_t player_count, own_id;
    const char *scenario_learn_name;   /* world +0x5c -> +0x1010 */
    int32_t (*rand_)(void *context);
    void (*srand_)(void *context, int32_t seed);
    void (*log)(void *context, const char *text);                      /* 0x0040c240 */
    void (*string)(void *context, int32_t id, char *out, int size);
    void (*choose)(void *context, char *build_list, char *rule_set, int32_t type, int32_t victory, int32_t size); /* 0x4f36e0 */
    int (*load_build_list)(void *context, const char *name);          /* Build AI +0x44 */
    void (*load_city_plan)(void *context, const char *name, int32_t w, int32_t h); /* +0x6c4's +0x40 */
    void (*load_rules)(void *context, const char *name);              /* 0x004cbff0 */
    void (*set_difficulty)(void *context, int32_t level);             /* 0x004cc080 */
    void (*init_objects)(void *context);                              /* 0x004c8210 */
    int (*allied)(void *context, int32_t player);                     /* vtable +0x18 */
    int (*neutral)(void *context, int32_t player);                    /* vtable +0x1c */
    void (*diplomacy_set)(void *context, int32_t player, int32_t kind, int32_t value); /* 0x0040ab40 */
    void (*diplomacy_flag)(void *context, int32_t player, int32_t v);  /* 0x0040ac80 */
    void (*personality)(void *context, int32_t preset);               /* 0x0040b2d0 */
    void (*load_learn)(void *context, const char *name);              /* 0x004c3820 */
} AoeTribeAiLoadOpsRor;
void aoe_tribe_player_load_ai_ror(const AoeTribeAiLoadOpsRor *o, const char *build_list,
                                  const char *city_plan, const char *rule_set, int32_t personality);


/* ---- 0x004f36e0: the computer player's random personality ---- */
#define AOE_TRIBE_WEIGHTS 131
typedef struct {
    void *context;
    int has_module;             /* +0x23c */
    uint8_t civ;                /* +0x109 */
    int32_t difficulty;         /* the settings' byte +0x993 (0x0041b990) */
    uint8_t death_match;        /* 0x005054e0 */
    int32_t flag_554734;        /* the data flag 0x00554734 (1 in the patched build) */
    int32_t *counters;          /* 0x007d2464[13] (world_game.h path_flags) */
    /* The civilisation's weights (0x004f5430..0x004f6640 for civs 1..16, see tribe_civ_setup.h). */
    void (*weights)(void *context, uint8_t civ, int32_t w[AOE_TRIBE_WEIGHTS], int32_t map_type,
                    int32_t victory, int32_t size);
    /* 0x004c6670 then 0x004c6710: the largest statistics bucket (-1 none). */
    int32_t (*top_bucket)(void *context);
    int32_t (*rand_)(void *context);                                /* rand() */
    void (*set_sn)(void *context, int32_t index, int32_t value);    /* 0x004cc030 */
    int32_t (*get_sn)(void *context, int32_t index);                /* 0x004cc060 */
    void (*string)(void *context, int32_t id, char *out, int size); /* game vtable +0x20 */
    void (*log)(void *context, const char *text);                   /* 0x0040c240 */
} AoeTribePersonalityOpsRor;
/* 0x004f36e0(build_list, rule_set, map_type, victory, size): nothing
   without the computer-player object. The civilisation's 131 weights;
   outside a death match civs 1, 2, 4, 5, 7, 8, 9 and 11 adjust weights
   13 / 14 by the map type (0: +300 / +100, 1: +250 / +75, 2 / 6 / 8:
   +200 / +50 when counter 12 is 0 / not; cleared while counter 11 is
   below 1). SN 0xc9 = 1. Outside a death match the top learn bucket
   doubles, quarters or triples groups of weights (0x004c6710); seven
   groups are then divided by 3 (positive entries only), negatives
   cleared, and a weighted draw (rand() % total) picks the build list:
   its name is string 0x26930 + pick (into `build_list`), and the pick's
   group bumps the 0x007d2464 counters (groups of three set SN 0x3b = 4
   and 0x3c = 10). Logs "I am the %s civ (culture #%d):", "Selected a
   best build list of '%s'." and "Setting AI Personality:"; `rule_set`
   becomes "RandomGameSpecialized". Then the strategic numbers: by the
   victory (0 / 1: 0x36 = 0x37 = 5; 2..5: 0 = 50, 1 = 2 = 25 unless SN 1
   is below 1 (then 2 = 50), 3 = 5 for map types above 1, 0x2a = 0x3d =
   2, and for 3..5 0x26 = size + 1, 0x24 = 1, 0x36 / 0x37 = 1 / 5, 5 / 1,
   5 / 5), 4 = 10 in a death match, 0x68 by the pick's group ((rand % 12
   + 28), (% 6 + 20), (% 2 + 5) or (% 4 + 11) minutes, less 300 with
   probability 1/3 at difficulty 0), 0x10 = 2, 0x1a = 10, 0x5d = 1, 99 = 4,
   0xbc, 0xc3, 0xbd random, 0xc6 / 199 random times the difficulty, 0xa1 = 4,
   0x49 / 0x4a by the size less 2 / 1 for map types 0 / 1, 0xd0 / 0xd1 /
   0xd2 / 0xdf = 1, 0xd4 / 0xd5 / 0xd6 / 0xe0 by the map type, 0x8a..0x8d
   = 2000 at difficulty 0 (else 0), 0xa8 = 0xa2 = 0, 0xc2 / 0x2f by
   difficulty < 3, 0xba = 180 then 200 / 150 / 100 / 50 by difficulty
   0..3, 0xb5 and 0xb6
   random. */
void aoe_tribe_player_personality_ror(const AoeTribePersonalityOpsRor *o, char *build_list,
                                      char *rule_set, int32_t map_type, int32_t victory,
                                      int32_t size);

#endif
