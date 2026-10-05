#ifndef VITA_AOE_AI_BUILD_H
#define VITA_AOE_AI_BUILD_H

#include <stddef.h>
#include <stdint.h>

#include "ai_module.h"
#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a computer-player build planning: plan items,
   Build AI build-list items and list (module 0x3e9), and ConAI
   construction-plan items and plan (module 0x3ea). Lists are circular
   with an embedded sentinel, as in the original (+0x6c next, +0x70
   previous). Semantic storage, not the x86 layout. Field names follow
   the original debug line "Type=%-4d %-20s gID=%-4d Cat=%d uID=%-3d
   inProg=%d bldAtt=%-2d skip=%d #blds=%d cap=%d bldFrom=%d permSkip=%d
   skipCy=%d." (0x00408770). */

typedef struct {
    int32_t type;       /* +0x04 Type: master object type */
    int32_t game_id;    /* +0x08 gID: the placed object, -1 */
    int32_t unique_id;  /* +0x0c uID */
    char name[64];      /* +0x10, 63 characters */
    float x, y, z;      /* +0x50, +0x54, +0x58 */
    float size_x, size_y, size_z; /* +0x5c, +0x60, +0x64 */
    int32_t skip;       /* +0x68 skip */
} AoeAiPlanItemRor;

/* Plan item (vtable 0x00542a38): default 0x004076b0 (IDs -1, the rest 0,
   name ""), full 0x00407730, copy 0x004077a0; destructor 0x00407820 (deleting
   0x00407710). */
void aoe_ai_plan_item_init_ror(AoeAiPlanItemRor *item);
void aoe_ai_plan_item_set_ror(AoeAiPlanItemRor *item, int32_t type,
                              int32_t game_id, int32_t unique_id,
                              const char *name, float x, float y, float z,
                              float size_x, float size_y, float size_z);
void aoe_ai_plan_item_copy_ror(AoeAiPlanItemRor *item,
                               const AoeAiPlanItemRor *from);
void aoe_ai_plan_item_release_ror(AoeAiPlanItemRor *item);
/* 0x00407830 name; 0x00407840 x, 0x00407850 y, 0x00407860 z,
   0x00407870 size x, 0x00407880 size y, 0x00407890 size z;
   0x004078a0 skip; setters 0x004078b0 x, 0x004078c0 y, 0x004078d0 z,
   0x004078e0 skip. Shared header getters: 0x00454150 type,
   0x00451ad0 game ID, 0x00451ab0 unique ID; setters 0x00454160 game ID
   and 0x004ccc50 unique ID. */
const char *aoe_ai_plan_item_name_ror(const AoeAiPlanItemRor *item);
enum {
    AOE_AI_ITEM_X, AOE_AI_ITEM_Y, AOE_AI_ITEM_Z,
    AOE_AI_ITEM_SIZE_X, AOE_AI_ITEM_SIZE_Y, AOE_AI_ITEM_SIZE_Z
};
float aoe_ai_plan_item_float_ror(const AoeAiPlanItemRor *item, int which);
void aoe_ai_plan_item_set_position_ror(AoeAiPlanItemRor *item, int which,
                                       float value);
int32_t aoe_ai_plan_item_skip_ror(const AoeAiPlanItemRor *item);
void aoe_ai_plan_item_set_skip_ror(AoeAiPlanItemRor *item, int32_t skip);
int32_t aoe_ai_plan_item_type_ror(const AoeAiPlanItemRor *item);
int32_t aoe_ai_plan_item_game_id_ror(const AoeAiPlanItemRor *item);
int32_t aoe_ai_plan_item_unique_id_ror(const AoeAiPlanItemRor *item);
void aoe_ai_plan_item_set_game_id_ror(AoeAiPlanItemRor *item, int32_t id);
void aoe_ai_plan_item_set_unique_id_ror(AoeAiPlanItemRor *item, int32_t id);
/* 0x00409380: the same x and y. */
int aoe_ai_plan_item_same_place_ror(const AoeAiPlanItemRor *a,
                                    const AoeAiPlanItemRor *b);

/* Build-list categories (+0x74), from the file letters 0x004088b0. */
enum {
    AOE_AI_BUILD_BUILDING = 0, /* 'A' (with cap) and 'B' */
    AOE_AI_BUILD_RESEARCH = 1, /* 'R' */
    AOE_AI_BUILD_UNIT = 2,     /* 'T' (with cap) and 'U' */
    AOE_AI_BUILD_L = 3,        /* 'L' */
    AOE_AI_BUILD_C = 4,        /* 'C' */
    AOE_AI_BUILD_E = 5,        /* 'E' */
    AOE_AI_BUILD_GATHER = 6    /* 'G'; 6 also names "SetGatherPercentage" */
};

typedef struct AoeAiBuildItemRor {
    AoeAiPlanItemRor plan;
    struct AoeAiBuildItemRor *next, *prev; /* +0x6c, +0x70 */
    int32_t category;       /* +0x74 Cat (0x00407a60) */
    int32_t field_78;       /* +0x78 */
    int32_t field_7c;       /* +0x7c */
    int32_t in_progress;    /* +0x80 inProg */
    int32_t built;          /* +0x84, 1 prints as "BLT" */
    int32_t build_attempts; /* +0x88 bldAtt */
    int32_t build_from;     /* +0x8c bldFrom */
    int32_t field_90[3];    /* +0x90..+0x98 */
    int32_t field_9c;       /* +0x9c */
    int32_t built_count;    /* +0xa0 #blds */
    int32_t cap;            /* +0xa4 cap */
    int32_t skip_cycles;    /* +0xa8 skipCy */
    uint8_t permanent_skip; /* +0xac permSkip */
} AoeAiBuildItemRor;

/* Build item (vtable 0x00542a3c, 0xb0 bytes): default 0x004078f0 (+0x74,
   +0x8c..+0x98 and +0xa4 = -1, the rest 0), full 0x00407980 (position
   -1, -1, -1; game ID -1); destructor 0x00407a50 (deleting 0x00407960). */
void aoe_ai_build_item_init_ror(AoeAiBuildItemRor *item);
void aoe_ai_build_item_set_ror(AoeAiBuildItemRor *item, int32_t category,
                               int32_t type, int32_t unique_id,
                               int32_t field_78, int32_t field_7c,
                               int32_t build_from, const char *name,
                               float size_x, float size_y, float size_z,
                               int32_t field_90, int32_t field_94,
                               int32_t field_98, int32_t field_9c);
void aoe_ai_build_item_release_ror(AoeAiBuildItemRor *item);
/* Getters 0x00407a70 inProg, 0x00407a80 built, 0x00407a90 bldAtt,
   0x00407aa0 bldFrom, 0x00407ab0 +0x90, 0x00407ae0 +0x9c, 0x00407b50
   #blds, 0x00407b80 cap, 0x00407ba0 skipCy, 0x00407bd0 permSkip;
   0x00407ac0(i) +0x90 + 4i for i 1..2, else -1.
   Setters 0x00407af0 inProg, 0x00407b00 built, 0x00407b10 bldAtt,
   0x00407b70 #blds, 0x00407b90 cap, 0x00407bc0 skipCy, 0x00407be0
   permSkip; 0x00407b20 / 0x00407b30 bldAtt + 1 / - 1 (not below 0),
   0x00407b60 #blds + 1, 0x00407bb0 skipCy + 1. */
enum {
    AOE_AI_BUILD_IN_PROGRESS, AOE_AI_BUILD_BUILT, AOE_AI_BUILD_ATTEMPTS,
    AOE_AI_BUILD_FROM, AOE_AI_BUILD_FIELD_90, AOE_AI_BUILD_FIELD_9C,
    AOE_AI_BUILD_COUNT, AOE_AI_BUILD_CAP, AOE_AI_BUILD_SKIP_CYCLES,
    AOE_AI_BUILD_PERMANENT_SKIP
};
int32_t aoe_ai_build_item_get_ror(const AoeAiBuildItemRor *item, int field);
void aoe_ai_build_item_put_ror(AoeAiBuildItemRor *item, int field,
                               int32_t value);
int32_t aoe_ai_build_item_field_90_ror(const AoeAiBuildItemRor *item,
                                       int32_t index);
void aoe_ai_build_item_add_attempt_ror(AoeAiBuildItemRor *item);
void aoe_ai_build_item_drop_attempt_ror(AoeAiBuildItemRor *item);
void aoe_ai_build_item_add_built_ror(AoeAiBuildItemRor *item);
void aoe_ai_build_item_add_skip_cycle_ror(AoeAiBuildItemRor *item);

/* Build AI build list (module 0x3e9 state). */
typedef struct {
    AoeAiModuleRor module;
    AoeAiBuildItemRor sentinel; /* +0xf0; its next/prev are +0x15c/+0x160 */
    int32_t count;              /* +0x1a0 */
    char file[257];             /* +0x1a4 */
    char name_2a5[257];         /* +0x2a5, "None" */
    char name_3a6[257];         /* +0x3a6, "None" */
    char name_4a7[257];         /* +0x4a7, "None" */
    int32_t field_5a8;          /* +0x5a8 */
    /* +0x5ac excluded building types (+0x5b0 count, +0x5b8 capacity);
       0x00409250 frees and zeroes it. */
    int32_t *excluded;
    int32_t excluded_count, field_5b4, excluded_capacity;
} AoeAiBuildListRor;

/* 0x00407bf0: an empty Build AI for `player`. */
void aoe_ai_build_list_init_ror(AoeAiBuildListRor *list, int32_t player,
                                AoeAiLogRor *log);
/* 0x004081f0 (deleting 0x00407d30 path): frees the items and the
   excluded types. */
void aoe_ai_build_list_release_ror(AoeAiBuildListRor *list);
void aoe_ai_build_list_free_excluded_ror(AoeAiBuildListRor *list);
/* 0x004091c0: delete every item, empty list, count 0. */
void aoe_ai_build_list_clear_ror(AoeAiBuildListRor *list);
/* 0x00408ce0 count, 0x00408cf0 file, 0x00408fc0 / 0x00408fd0 /
   0x00408fe0 the other names, 0x00408fb0 +0x5a8. */
int32_t aoe_ai_build_list_count_ror(const AoeAiBuildListRor *list);
const char *aoe_ai_build_list_file_ror(const AoeAiBuildListRor *list);
const char *aoe_ai_build_list_name_ror(const AoeAiBuildListRor *list,
                                       int which); /* 0..2 */
int32_t aoe_ai_build_list_field_5a8_ror(const AoeAiBuildListRor *list);
/* 0x00408d00: the file is neither "NONE" nor "NULL". */
int aoe_ai_build_list_has_file_ror(const AoeAiBuildListRor *list);
/* 0x00409210: the item with this unique ID, or NULL. */
AoeAiBuildItemRor *aoe_ai_build_list_find_ror(const AoeAiBuildListRor *list,
                                              int32_t unique_id);
/* Counts of items with this type and category: all 0x00408ff0, built
   0x00409040, not built 0x004090a0, built or in progress 0x00409100, in
   progress 0x00409160. */
enum {
    AOE_AI_COUNT_ALL, AOE_AI_COUNT_BUILT, AOE_AI_COUNT_NOT_BUILT,
    AOE_AI_COUNT_BUILT_OR_IN_PROGRESS, AOE_AI_COUNT_IN_PROGRESS
};
int32_t aoe_ai_build_list_count_of_ror(const AoeAiBuildListRor *list,
                                       int32_t type, int32_t category,
                                       int filter);

/* The player's master object for a type (table +0x24, count +0x22): its
   name (+8) and radii (+0x30/+0x34). Nonzero when the type has one. */
typedef int (*AoeAiObjectFn)(void *context, int32_t type, const char **name,
                             float *rx, float *ry);

/* 0x00408d90: insert `copies` items after the item at `position`
   (0 = after the first; past the last, the walk stops on the sentinel and
   the copies go to the front; refused when position > count). Each copy
   goes right after that item, so copies end up in reverse. Category,
   type, build-from and cap; unique IDs continue from the count. The name
   and size come from the object (size = 2 x radius), else "SetGather
   Percentage" (category 6) or "AddedBuildListItem" with size 3 x 3.
   Items start with position -1, game ID -1, size z 1 and the counters 0.
   Returns 0 when refused or out of memory. */
int aoe_ai_build_list_insert_ror(AoeAiBuildListRor *list,
                                 AoeAiObjectFn objects, void *context,
                                 int32_t type,
                                 int32_t copies, int32_t category,
                                 int32_t build_from, int32_t cap,
                                 int32_t position);

/* 0x004088b0: load a build-list text. The file name is stored (63
   characters); "NONE" or an empty name only logs "There is no build list
   to load.". Each line not starting with "//" is
   "%c%d %s %d %d" (letter, type, name, copies, build-from); 'A' and 'T'
   also take the cap from the sixth field. 'A' and 'B' lines whose type is
   excluded are skipped; unknown letters are skipped. Buildings and units
   take their size from the object (default 1.5), doubled. Items are
   appended with consecutive unique IDs. `text` is the file content
   (lines of at most 255 characters); NULL reports a failed open (logs
   "  I cannot do it cap'n!", returns 0). An opened file first clears the
   old items. (The original then deletes a file whose name contains
   ".tmp"; that is left to the caller.) Fields a line does not supply keep
   the previous line's values, as with the original sscanf. */
int aoe_ai_build_list_load_text_ror(AoeAiBuildListRor *list,
                                    const char *file, const char *text,
                                    AoeAiObjectFn objects, void *context);

/* 0x00408770: log "Build list '%s':" and one debug line per item
   ("BLT " first when built). */
void aoe_ai_build_list_describe_ror(const AoeAiBuildListRor *list);

/* Save 0x00408290 and load constructor 0x00407d50. Strings are a u32
   length then the bytes; `version` is the save version (global
   0x0055f804): the cap is read from 2.2 and the fourth name from 3.1
   ("None" before). Items are i32/f32 fields (see the source). */
int aoe_ai_build_list_save_ror(const AoeAiBuildListRor *list,
                               AoeOutcomeStream *out);
int aoe_ai_build_list_load_ror(AoeAiBuildListRor *list, int32_t player,
                               AoeAiLogRor *log, AoeOutcomeStream *in,
                               float version);

/* ConAI construction item (vtable 0x00542a94, 0x80 bytes). */
typedef struct AoeAiConItemRor {
    AoeAiPlanItemRor plan;
    struct AoeAiConItemRor *next, *prev; /* +0x6c, +0x70 */
    int32_t field_74;  /* +0x74 0x004093c0 */
    int32_t field_78;  /* +0x78 0x004093d0 / 0x00453f30 */
    int32_t counter;   /* +0x7c 0x004093b0 / 0x00409410 */
} AoeAiConItemRor;

/* Default 0x00409280, full 0x004092d0 (position, size, type, name; IDs
   -1), copy 0x00409330 (links cleared); destructor 0x00409370 (deleting
   0x004092b0). Counter + 1 0x004093e0, - 1 not below 0 0x004093f0. */
void aoe_ai_con_item_init_ror(AoeAiConItemRor *item);
void aoe_ai_con_item_set_ror(AoeAiConItemRor *item, float x, float y,
                             float z, float size_x, float size_y,
                             float size_z, int32_t type, const char *name);
void aoe_ai_con_item_copy_ror(AoeAiConItemRor *item,
                              const AoeAiConItemRor *from);
void aoe_ai_con_item_release_ror(AoeAiConItemRor *item);
void aoe_ai_con_item_set_74_ror(AoeAiConItemRor *item, int32_t value);
void aoe_ai_con_item_set_78_ror(AoeAiConItemRor *item, int32_t value);
int32_t aoe_ai_con_item_78_ror(const AoeAiConItemRor *item);
int32_t aoe_ai_con_item_counter_ror(const AoeAiConItemRor *item);
void aoe_ai_con_item_set_counter_ror(AoeAiConItemRor *item, int32_t value);
void aoe_ai_con_item_add_ror(AoeAiConItemRor *item);
void aoe_ai_con_item_drop_ror(AoeAiConItemRor *item);

typedef struct {
    AoeAiModuleRor module;
    int32_t count;            /* +0xf0 */
    AoeAiConItemRor plan;     /* +0xf4 sentinel; next/prev +0x160/+0x164 */
    int32_t placed_count;     /* +0x174 */
    AoeAiConItemRor placed;   /* +0x178 sentinel; next/prev +0x1e4/+0x1e8 */
    char file[260];           /* +0x1f8 */
    float origin_x, origin_y, origin_z; /* +0x2fc..+0x304 */
    int32_t map_width, map_height;      /* +0x308, +0x30c */
    int32_t field_310;        /* +0x310 */
} AoeAiConPlanRor;

/* 0x00409420: empty, origin -1, map -1 x -1. Destructor 0x00409a50
   (deleting 0x00409520) clears the plan list; the derived plan of vtable
   0x00548b18 destructs the same (0x004b96e0, deleting 0x004b9690). */
void aoe_ai_con_plan_init_ror(AoeAiConPlanRor *plan, int32_t player,
                              AoeAiLogRor *log);
void aoe_ai_con_plan_release_ror(AoeAiConPlanRor *plan);
/* 0x0040a730: delete the plan items. */
void aoe_ai_con_plan_clear_ror(AoeAiConPlanRor *plan);
/* 0x0040a280. */
const char *aoe_ai_con_plan_file_ror(const AoeAiConPlanRor *plan);
/* 0x0040a290: store origin and map size; an origin at or below 0 or at
   or past the map edge clears the plan; otherwise every item moves by
   the origin and items whose half-size box leaves the map (below 0 or
   past size - 1) are deleted. */
void aoe_ai_con_plan_place_ror(AoeAiConPlanRor *plan, int32_t width,
                               int32_t height, float x, float y, float z);
/* 0x0040a640: the plan item at exactly (x, y), else the placed item;
   0x0040a6e0: the placed item only. */
AoeAiConItemRor *aoe_ai_con_plan_at_ror(AoeAiConPlanRor *plan, float x,
                                        float y);
AoeAiConItemRor *aoe_ai_con_plan_placed_at_ror(AoeAiConPlanRor *plan,
                                               float x, float y);
/* 0x0040a480: the first plan item at the same place as `like` gets
   +0x78 = value. */
void aoe_ai_con_plan_mark_ror(AoeAiConPlanRor *plan,
                              const AoeAiPlanItemRor *like, int32_t value);
/* 0x0040a4d0 (at the place of `like`) and 0x0040a550 (at x, y): reset
   the plan item (+0x74, +0x78 = 0, unique ID -1, counter - 1 when
   `drop` is 1) and return its previous unique ID, or -1. */
int32_t aoe_ai_con_plan_release_item_ror(AoeAiConPlanRor *plan,
                                         const AoeAiPlanItemRor *like,
                                         int drop);
int32_t aoe_ai_con_plan_release_at_ror(AoeAiConPlanRor *plan, float x,
                                       float y, int drop);
/* 0x0040a5e0 / 0x0040a610: counter - 1 / + 1, `times` times, for the item
   at (x, y). */
void aoe_ai_con_plan_drop_ror(AoeAiConPlanRor *plan, float x, float y,
                              int32_t times);
void aoe_ai_con_plan_add_ror(AoeAiConPlanRor *plan, float x, float y,
                             int32_t times);
/* 0x00409fb0: place (0x0040a290), clear a non-empty plan, store the file
   name; "NONE" logs "There is no construction plan to load.". Each line
   not starting with "//" is "%f %f %f %f %f %f %d %s" (position, size,
   type, name); the position moves by the origin and a line at an
   occupied place is skipped. NULL text is a failed open (returns 0). */
int aoe_ai_con_plan_load_text_ror(AoeAiConPlanRor *plan, const char *file,
                                  const char *text, int32_t width,
                                  int32_t height, float x, float y,
                                  float z);
/* Save 0x00409ad0 and load constructor 0x00409540. The derived plan of
   vtable 0x00548b18 builds on 0x00409420 (0x004b9660) or 0x00409540
   (0x004b96b0), clearing its own +0x314, which 0x004b96f0 sets.
   0x004ba0a0: of two plans the one with the lower counter (0x004093b0;
   the first on a tie). */
int aoe_ai_con_plan_save_ror(const AoeAiConPlanRor *plan,
                             AoeOutcomeStream *out);
int aoe_ai_con_plan_load_ror(AoeAiConPlanRor *plan, int32_t player,
                             AoeAiLogRor *log, AoeOutcomeStream *in);

/* The RoR build list (vtable 0x00548acc): the base list and the
   computer player it belongs to (+0x5bc). 0x004b7650 / 0x004b76a0
   construct it as the base (0x00407bf0 / load 0x00407d50) with no
   owner; 0x004b76d0 releases it (0x004081f0; deleting 0x004b7680); 0x004b76e0 sets the owner.
   0x004b8fb0 (a player's 0x004f2cb0 when it has the list, +0x23c): the
   item with this unique ID (if any) is no longer in
   progress; always 1. */
typedef struct {
    AoeAiBuildListRor base;
    void *owner; /* +0x5bc */
} AoeAiRorBuildListRor;

void aoe_ai_ror_build_list_init_ror(AoeAiRorBuildListRor *list, int32_t player, AoeAiLogRor *log);
int aoe_ai_ror_build_list_load_ror(AoeAiRorBuildListRor *list, int32_t player, AoeAiLogRor *log,
                                   AoeOutcomeStream *in, float version);
void aoe_ai_ror_build_list_release_ror(AoeAiRorBuildListRor *list);
void aoe_ai_ror_build_list_set_owner_ror(AoeAiRorBuildListRor *list, void *owner);
int aoe_ai_build_list_stop_ror(AoeAiBuildListRor *list, int32_t unique_id);

/* The RoR build list (vtable 0x00548acc) over the same list:
   0x004b9300 adds research `type` (5, 7 or 8 only; else 0) built at
   0x2d, cap 1, at `position` (-1: after the leading built items);
   0x004b93a0 adds a gather setting (category 6) for `type` from
   `build_from` at `position`. Both go through 0x00408d90 with the
   computer player's objects. */
int aoe_ai_build_list_add_research_ror(AoeAiBuildListRor *list, AoeAiObjectFn objects,
                                       void *context, int32_t type, int32_t position);
int aoe_ai_build_list_add_gather_ror(AoeAiBuildListRor *list, AoeAiObjectFn objects,
                                     void *context, int32_t type, int32_t build_from,
                                     int32_t position);
/* 0x004b93d0 (+0x48): built or in progress, except for buildings 0x67
   and 0x44 (category 0), which count those in progress plus `extra`
   (0x004c55b0 / 0x004c5680, the player's own counts). */
int32_t aoe_ai_build_list_count_ror2(const AoeAiBuildListRor *list, int32_t type,
                                     int32_t category,
                                     int32_t (*extra)(void *context, int32_t type),
                                     void *context);
/* 0x004b9440: 0 when an item of either category is neither built nor in
   progress, else 1. 0x004b95e0: the first such item of plan type `type`
   (and either category), or NULL. */
int aoe_ai_build_list_all_started_ror(const AoeAiBuildListRor *list, int32_t category_a,
                                      int32_t category_b);
AoeAiBuildItemRor *aoe_ai_build_list_pending_ror(const AoeAiBuildListRor *list, int32_t type,
                                                 int32_t category_a, int32_t category_b);
/* 0x004b9540: every item neither built nor in progress (except 0x67 /
   0x44 items while the list's +0x48 count of that building is positive)
   has its skip, attempts, skip cycles and permanent skip reset to 0. */
void aoe_ai_build_list_retry_ror(AoeAiBuildListRor *list,
                                 int32_t (*count)(void *context, int32_t type, int32_t category),
                                 void *context);

/* RoR build list events (vtable 0x00548acc). 0x004b76f0: types 0x46,
   0x67 and 0x44 are added to the excluded list when absent (the array
   grows by one when full), then the text load 0x004088b0. Each event
   below ends with the hooks given ("then"): 0x004b8f40(object, player
   arg, unique ID, game ID): the item with the unique ID is no longer in
   progress, built, with that game ID and one more built; then the
   player's hook (0x004c8180) and the update. 0x004b8fd0:
   the same without the game ID. 0x004b8c40(game ID): the first built
   item with that game ID becomes unbuilt (not in progress, game ID -1,
   no attempts; a 0x114 type clears the owner's +0x122b4); then the
   update. 0x004b8b40(object, unique ID): the first unbuilt item of the
   object's type with that unique ID (none for -1) is built with the
   object's ID, one more built (a 0x114 type sets +0x122b4), its name
   copied to +0x2a5 and its unique ID to +0x5a8; then the update; always
   1. 0x004b8a70(object, quiet): a key building (0x004be220) is reported;
   unless quiet the first unbuilt item of its type is built
   the same way (1), else 0. 0x004b8030: the first unbuilt item not in
   progress and not skipped is skipped when it is not the strategy's
   value 0xb1 type or not a building: its attempts become value 0x72,
   skip 1, one more skip cycle, and at value 0x92 cycles a permanent
   skip. */
typedef struct {
    void *context;
    void (*player_hook)(void *context, int32_t arg);      /* 0x004c8180 */
    void (*update)(void *context);
    int32_t (*value)(void *context, int32_t id);          /* 0x004d77d0 */
    int (*key_building)(void *context, int16_t type);     /* 0x004be220 */
    void (*report_key)(void *context, int32_t object_id);
    int32_t *owner_flag_122b4;
} AoeAiRorBuildOpsRor;
int aoe_ai_ror_build_list_load_ror2(AoeAiRorBuildListRor *list, const char *file,
                                    const char *text, AoeAiObjectFn objects, void *context);
int aoe_ai_ror_build_done_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops,
                              int32_t arg, int32_t unique_id, int set_game_id, int32_t game_id);
int aoe_ai_ror_build_lost_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops,
                              int32_t game_id);
int aoe_ai_ror_build_placed_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops,
                                int32_t object_type, int32_t object_id, int32_t unique_id);
int aoe_ai_ror_build_object_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops,
                                int32_t object_type, int32_t object_id, int quiet);
void aoe_ai_ror_build_skip_ror(AoeAiRorBuildListRor *list, const AoeAiRorBuildOpsRor *ops);

#endif
