#ifndef VITA_AOE_GAME_BASE_H
#define VITA_AOE_GAME_BASE_H

#include <stdint.h>
#include <stdio.h>

/* Patched Rise of Rome 1.0a base game object pieces (global 0x00580e38,
   0x00416e30..0x0041c290) and the per-player sync checksum
   (0x0045cec0..0x0045ce90). Semantic storage; offsets are noted. */

/* ---- per-player sync checksum ---- */

typedef struct {
    int16_t type;      /* template (+8) +0x10 */
    uint8_t state;     /* +0x48 */
    float field_44;    /* +0x44 */
    int32_t method_1d4;/* vtable +0x1d4() */
    float x, y, z;     /* +0x38, +0x3c, +0x40 */
} AoeChecksumObjectRor;

typedef struct {
    int16_t player_id;       /* +0x4a */
    int16_t civilization;    /* +0x2c -> +0x08 */
    int16_t attribute_count; /* +0x4c */
    const float *attributes; /* +0x50 */
    const AoeChecksumObjectRor *objects; /* list +0x28 in order */
    int32_t object_count;
    uint8_t valid;           /* +0x20 */
    uint32_t checksum;       /* +0x14 */
    int32_t position_sum;    /* +0x18 */
    int32_t field_1c;        /* +0x1c */
} AoePlayerChecksumRor;

/* 0x0045cec0: valid = 1, +0x1c = 0; checksum =
   ((((civilization + player) << 8) + objects) << 8 + object sum) << 8 +
   resource sum (32-bit wrap), where the resource sum adds ftol of every
   attribute and the object sum adds type + state + ftol(+0x44) +
   +0x1d4() per object; +0x18 = ftol of the float sum of x + y + z. */
uint32_t aoe_player_checksum_compute_ror(AoePlayerChecksumRor *player);
/* 0x0045ce70 (checksum) and 0x0045ce90 (all three), computing first when
   not valid. */
uint32_t aoe_player_checksum_ror(AoePlayerChecksumRor *player);
int aoe_player_checksums_ror(AoePlayerChecksumRor *player, uint32_t *checksum,
                             int32_t *position_sum, int32_t *field_1c);

/* ---- base game object ---- */

#define AOE_GAME_TIMING_SLOTS 30

typedef struct {
    uint32_t total;   /* +0x528 + 0x20 i */
    uint32_t field_4; /* +0x52c: the last second's total */
    uint32_t field_8; /* +0x530: when that second ended */
    uint32_t counted; /* +0x534: part of the busy sum */
    uint32_t held;    /* +0x538: not rolled over each second */
    uint32_t last;    /* +0x53c */
    uint32_t max;     /* +0x540 */
    uint32_t field_1c;/* +0x544: the last second's max */
} AoeGameTimingRor;

typedef struct {
    int32_t field_40;               /* +0x40 */
    int32_t mode;                   /* +0x1b0 */
    int32_t state, substate;        /* +0x1b4, +0x1b8 */
    /* World (+0x3f4): players and the viewed player (+0x7c). */
    int has_world;
    int16_t player_count;           /* world +0x3c */
    int16_t view_player;            /* world +0x7c */
    void *const *players;           /* world +0x40 */
    AoePlayerChecksumRor *const *checksums; /* the same players */
    uint8_t world_dirty;            /* world +0x1c */
    const uint8_t *records_1e4;     /* +0x1e4, 12-byte records */
    void *const *array_5c;          /* +0x5c */
    void *const *array_6c;          /* +0x6c */
    int32_t cursor_capable;         /* +0x54 */
    int32_t cursor_shown;           /* +0x1cc */
    int32_t software_cursor;        /* +0x1d0 */
    uintptr_t cursor;               /* +0x1d4 */
    int32_t field_3f8;              /* +0x3f8 */
    int32_t comm_state, comm_value; /* +0xa00, +0xa04 */
    int32_t timing_504[9];          /* +0x504..+0x524 */
    AoeGameTimingRor timing[AOE_GAME_TIMING_SLOTS];
    const char *command_line;       /* settings +0x518 */
} AoeGameBaseRor;

typedef struct {
    void *context;
    void (*refresh_view)(void *context);        /* 0x00444a30 */
    int32_t (*comm_value)(void *context);       /* 0x004239b0 */
    void (*comm_mark)(void *context);           /* 0x00423a80 */
    void (*cursor_draw_on)(void *context);      /* 0x0044a780 */
    void (*cursor_draw_off)(void *context);     /* 0x0044a720 */
    /* SetCursor(c) + SetClassLongA(GCL_HCURSOR, c). */
    void (*set_system_cursor)(void *context, uintptr_t cursor);
    uintptr_t *last_cursor;                     /* global 0x00580e10 */
    void (*world_reset)(void *context);         /* 0x004523c0 */
    /* String table: vtable +0x20(id, buffer, 0x200) / LoadStringA. */
    int (*load_string)(void *context, uint32_t id, char *out, int size);
} AoeGameBaseOps;

/* Base game virtual defaults 0x004172a0 (nothing) and 0x004172b0 /
   0x004172c0 / 0x004172d0 / 0x004172e0 / 0x004172f0 / 0x00417300 (1). */
void aoe_game_default_nothing_ror(AoeGameBaseRor *game);
int32_t aoe_game_default_true_ror(AoeGameBaseRor *game, int which);

/* Timing: 0x00417310 reset (+0x504..+0x524 and each slot's six fields),
   0x00417370 sample (total += v, last = v, max raised), 0x004173b0 /
   0x004173c0 count +0x508 / +0x50c, 0x004173d0 set last, getters
   0x004173f0 (+4), 0x00417410 (last), 0x00417430 (total), 0x00417450
   (+0x514), 0x00417460 (+0x40). Slots are host-bounded. */
void aoe_game_timing_reset_ror(AoeGameBaseRor *game);
void aoe_game_timing_sample_ror(AoeGameBaseRor *game, int32_t slot,
                                uint32_t value);
void aoe_game_timing_count_508_ror(AoeGameBaseRor *game);
void aoe_game_timing_count_50c_ror(AoeGameBaseRor *game);
void aoe_game_timing_set_last_ror(AoeGameBaseRor *game, int32_t slot,
                                  uint32_t value);
uint32_t aoe_game_timing_field4_ror(const AoeGameBaseRor *game, int32_t slot);
uint32_t aoe_game_timing_last_ror(const AoeGameBaseRor *game, int32_t slot);
uint32_t aoe_game_timing_total_ror(const AoeGameBaseRor *game, int32_t slot);
int32_t aoe_game_timing_514_ror(const AoeGameBaseRor *game);
int32_t aoe_game_field_40_ror(const AoeGameBaseRor *game);

/* 0x004190b0: slots 1, 2, 3, 10 and 13 counted, slots 0 and 4 held. */
void aoe_game_timing_setup_ror(AoeGameBaseRor *game);

/* +0x130 0x0041af70: the once-a-second rollover: every slot not held
   keeps its total and max as the last second's and restarts; slot 0 gets
   the time since the last rollover (the counted sum the first time),
   slot 4 the idle rest (slot 0 less the counted sum, not below 0) and
   slot 9 slot 2 less slots 6 and 7 (not below 0); +0x51c / +0x520 /
   +0x524 become per-second rates of +0x504 / +0x508 / +0x50c (1000 / (ms
   per item); 1000, 0, 0 when undefined); then +0x134 (`hook`) and, with
   the performance log (global 0x00580df0) in mode 4, one log line; the
   three counts move to +0x510..+0x518 and restart. */
typedef struct {
    void *context;
    void (*hook)(void *context);            /* +0x134 */
    FILE *log;                              /* 0x00580df4 when 0x00580df0 */
    uint32_t world_id;                      /* world +0x04 */
    int32_t globals[4];                     /* 0x00580dfc..0x00580e08 */
    int16_t gaia_counts[2];                 /* player 0's +0x28 / +0x2c counts */
    int32_t other_counts[2];                /* the rest of the players' */
} AoeGameRolloverOpsRor;
void aoe_game_timing_rollover_ror(AoeGameBaseRor *game, uint32_t now,
                                  const AoeGameRolloverOpsRor *ops);

/* 0x00417470: the string into a 0x200 static buffer (global 0x00580b78);
   0x00417490: LoadStringA, "" on failure, last byte cleared. */
const char *aoe_game_string_ror(const AoeGameBaseOps *ops, uint32_t id);
char *aoe_game_load_string_ror(const AoeGameBaseOps *ops, uint32_t id,
                               char *out, int size);

/* 0x00416e30 mode; 0x00419970 state/substate; 0x00419990 view a player
   below the count and refresh; 0x004199c0 the comm value (0 in mode 7);
   0x00419a60 store it, and on a change store the value (or 0) and mark;
   0x00419a40 mark and set world +0x1c. */
void aoe_game_set_mode_ror(AoeGameBaseRor *game, int32_t mode);
void aoe_game_set_state_ror(AoeGameBaseRor *game, int32_t state,
                            int32_t substate);
void aoe_game_view_player_ror(AoeGameBaseRor *game, int16_t player,
                              const AoeGameBaseOps *ops);
int32_t aoe_game_comm_value_ror(const AoeGameBaseRor *game,
                                const AoeGameBaseOps *ops);
void aoe_game_comm_mark_ror(AoeGameBaseRor *game, const AoeGameBaseOps *ops);
void aoe_game_comm_update_ror(AoeGameBaseRor *game, int32_t value,
                              int32_t extra, const AoeGameBaseOps *ops);
/* 0x004199e0 record i of +0x1e4, 0x00419a00 / 0x00419a20 entries of
   +0x5c / +0x6c (NULL without the array). 0x00419b70: the viewed player
   (NULL without a world, players, or a valid index). */
const uint8_t *aoe_game_record_1e4_ror(const AoeGameBaseRor *game, int32_t i);
void *aoe_game_entry_5c_ror(const AoeGameBaseRor *game, int32_t i);
void *aoe_game_entry_6c_ror(const AoeGameBaseRor *game, int32_t i);
void *aoe_game_view_player_object_ror(const AoeGameBaseRor *game);
/* 0x0041bc70: checksum of player `p` (the viewed one when negative; 0
   without a world or past the count). 0x0041bcb0: the three checksum
   values summed over every player (0 without a world). */
uint32_t aoe_game_player_checksum_ror(AoeGameBaseRor *game, int32_t p);
int aoe_game_checksums_ror(AoeGameBaseRor *game, uint32_t *checksum,
                           int32_t *position_sum, int32_t *field_1c);
/* 0x0041bfe0: the upper-cased command line (255 bytes) contains `option`. */
int aoe_game_has_option_ror(const AoeGameBaseRor *game, const char *option);
/* 0x0041c290: +0x3f8 = 1, then the world reset. */
void aoe_game_mark_3f8_ror(AoeGameBaseRor *game, const AoeGameBaseOps *ops);

/* 0x00419ab0: mouse message decode: x/y from the low/high 16 bits (above
   60000 wraps by 0x10000) and the button bits 1, 2, 8, 4. */
void aoe_game_decode_mouse_ror(uint32_t wparam, uint32_t lparam, int32_t xy[2],
                               uint32_t *left, uint32_t *right,
                               uint32_t *shift, uint32_t *control);

/* Cursor: show 0x00419bb0, hide 0x00419c20, software mode 0x00419c80,
   set cursor 0x00419d40. */
void aoe_game_cursor_show_ror(AoeGameBaseRor *game, const AoeGameBaseOps *ops);
void aoe_game_cursor_hide_ror(AoeGameBaseRor *game, const AoeGameBaseOps *ops);
void aoe_game_cursor_software_ror(AoeGameBaseRor *game, int32_t on,
                                  const AoeGameBaseOps *ops);
void aoe_game_cursor_set_ror(AoeGameBaseRor *game, uintptr_t cursor,
                             const AoeGameBaseOps *ops);


/* 0x0041656c is not a function: Ghidra splits the game constructor's
   field-reset loop (the 0x1e-iteration clear of the 32-byte records from
   +0x528 onwards, then +0x8e8..+0x8f0, +0x9ac..+0xa20 and the 0x00418620
   / 0x004186d0 / 0x004166b0 initialisers that follow) at a loop body
   entered from 0x0041657f. It belongs to the constructor that owns the
   fields above. */

#endif
