#ifndef VITA_AOE_PLAYER_SELECTION_H
#define VITA_AOE_PLAYER_SELECTION_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a player selection (`this` = player, base
   vtable 0x00544d18): up to 25 selected units at player +0x138, their
   count at +0x1a0, the current unit at +0x134 and a used flag for each
   unit group number at +0x1a2. Semantic caller-owned storage; original
   offsets are noted on each field. */

#define AOE_PLAYER_SELECTION_SLOTS 25
#define AOE_PLAYER_SELECTION_GROUPS 127

typedef struct AoePlayerSelectionRor AoePlayerSelectionRor;

/* The fields of a live unit (and of its template, unit +0x08) that the
   selection routines read and write. */
typedef struct AoeSelectableUnitRor {
    const AoePlayerSelectionRor *owner; /* +0x0c, the owning player */
    int16_t owner_id;     /* owner +0x4a */
    int32_t id;           /* +0x04 */
    uint8_t selected;     /* +0x36, set while in a selection */
    uint8_t group;        /* +0x37, unit group number (0 = none) */
    uint8_t state;        /* +0x48 */
    uint8_t has_field_20; /* +0x20 is non-null */
    uint8_t type;         /* template +0x04 */
    int16_t template_id;  /* template +0x10 */
    int16_t unit_class;   /* template +0x14 */
    uint8_t field_8d;     /* template +0x8d */
    uint8_t field_8f;     /* template +0x8f */
    uint8_t has_selection_sound; /* template +0x3c is non-null */
} AoeSelectableUnitRor;

struct AoePlayerSelectionRor {
    int16_t player_id;                 /* +0x4a */
    /* The player's +0x28 unit list, in list order. */
    AoeSelectableUnitRor *const *units;
    size_t unit_count;
    int16_t field_124[7];              /* +0x124..+0x130 */
    AoeSelectableUnitRor *current;     /* +0x134 */
    AoeSelectableUnitRor *slots[AOE_PLAYER_SELECTION_SLOTS]; /* +0x138 */
    int16_t count;                     /* +0x1a0 */
    uint8_t group_used[AOE_PLAYER_SELECTION_GROUPS]; /* +0x1a2 */
};

typedef struct {
    void *context;
    /* Plays the unit template's selection sound (template +0x3c). */
    void (*play_selection_sound)(void *context,
                                 const AoeSelectableUnitRor *unit);
} AoePlayerSelectionOps;

/* 0x0045db40: +0x124..+0x130 = -1, empty selection, no current unit and
   no group number in use. */
void aoe_player_selection_init_ror(AoePlayerSelectionRor *player);

/* 0x0045dc10: put `unit` in the slot at index `count` when that slot is
   free, otherwise in the first free slot below `count` (with no free slot
   there it fails, as in the original, even if later slots are free). The
   unit becomes current and is marked selected. With `play_sound`, a unit
   whose template has a selection sound and that belongs to this player or
   to player 0 plays it. Returns 0 when the selection is full. */
int aoe_player_selection_add_ror(AoePlayerSelectionRor *player,
                                 AoeSelectableUnitRor *unit, int play_sound,
                                 const AoePlayerSelectionOps *ops);

/* 0x0045dbb0: add `unit` with its sound; then, for a group number above
   10, add the player's other units of that group silently, in list order,
   until one does not fit. The unit becomes current. Returns 0 only when
   `unit` itself did not fit. */
int aoe_player_selection_select_ror(AoePlayerSelectionRor *player,
                                    AoeSelectableUnitRor *unit,
                                    const AoePlayerSelectionOps *ops);

/* 0x0045dcb0: unmark and drop every selected unit; no current unit. */
void aoe_player_selection_clear_ror(AoePlayerSelectionRor *player);

/* 0x0045dd00: remove `unit` from the selection when present. */
void aoe_player_selection_remove_ror(AoePlayerSelectionRor *player,
                                     const AoeSelectableUnitRor *unit);

/* 0x0045dd50: remove the unit in `slot`: the slot at index count-1 is
   moved into it (when `slot` is below count-1), the count drops, and a
   removed current unit is replaced by the first occupied slot. An empty
   slot does nothing. Port: a slot outside 0..24 is ignored. */
void aoe_player_selection_remove_slot_ror(AoePlayerSelectionRor *player,
                                          int16_t slot);

/* 0x0045de00: give the selected units owned by this player a group
   number. -1 picks the first unused number 11..126, or when all are used,
   recomputes the used flags from the player's units and picks the first
   unused number 1..126 (returning 0 when none is left); the chosen number
   is marked used. Any other value (its low byte is the number) is first
   taken off the player's units that have it. Returns 0 for an empty
   selection. */
int aoe_player_selection_set_group_ror(AoePlayerSelectionRor *player,
                                       int32_t group);

/* 0x0045df40: select the player's units of group `group`, in list order;
   the first with its sound. Returns 1 when at least one unit was added. */
int aoe_player_selection_select_group_ror(AoePlayerSelectionRor *player,
                                          uint8_t group,
                                          const AoePlayerSelectionOps *ops);

/* 0x0045dfb0: whether a selected unit's template class (+0x14) equals
   `unit_class`. */
int aoe_player_selection_has_class_ror(const AoePlayerSelectionRor *player,
                                       int16_t unit_class);

/* 0x0045e000: the template +0x8f byte shared by every selected unit, or 0
   when they differ or nothing is selected. */
uint8_t aoe_player_selection_common_8f_ror(
    const AoePlayerSelectionRor *player);

/* 0x0045e060: the smallest template +0x8d byte of the selection, or 0
   when nothing is selected or the smallest is 0xff. */
uint8_t aoe_player_selection_min_8d_ror(const AoePlayerSelectionRor *player);

/* 0x0045e0c0: copy into `out` (slot order) the selected units that are in
   state 2, belong to this player, have template +0x8d >= `min_8d` (any
   level for player 0; -1 means 4), class (+0x14) and template ID (+0x10)
   equal to the low byte of `unit_class` / `template_id` unless -1,
   +0x8f equal to the low byte of `field_8f` unless -1, and a template
   type of at least 40 other than 90. Returns the number copied (the
   original returns a newly allocated array and fails when it is empty). */
int16_t aoe_player_selection_collect_ror(
    const AoePlayerSelectionRor *player,
    AoeSelectableUnitRor *out[AOE_PLAYER_SELECTION_SLOTS], int16_t min_8d,
    int16_t unit_class, int16_t template_id, int16_t field_8f);

/* 0x0045e200: drop selected units whose state is 7 or more or whose
   +0x20 is set. */
void aoe_player_selection_prune_ror(AoePlayerSelectionRor *player);

/* 0x0045e250: +0x124..+0x12a = -1. */
void aoe_player_selection_reset_124_ror(AoePlayerSelectionRor *player);

/* 0x0045e270: clear every selected unit's group number. */
void aoe_player_selection_clear_groups_ror(AoePlayerSelectionRor *player);

#endif
