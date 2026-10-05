#ifndef VITA_AOE_PROJECTILE_H
#define VITA_AOE_PROJECTILE_H

#include <stddef.h>
#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a projectile flight action (vtable 0x0054267c,
   action type 8, constructor 0x00404990), pushed onto a projectile unit by
   0x00449070 (at a target) / 0x00449120 (at a point), which also store the
   launch radius at projectile +0x1bc. `base.x/y/z` hold the aim point,
   `base.target` the target. */
typedef struct {
    AoeMoveActionRor base;
    AoeLiveUnit *launcher;  /* +0x14, ID in base.secondary_id (+0x1c) */
    float traveled;         /* +0x2c */
    float dir_x, dir_y, dir_z; /* +0x40/+0x44/+0x48 */
    float climb;            /* +0x4c: vertical rate */
    float gravity;          /* +0x50: vertical acceleration */
    float launch_radius;    /* projectile unit +0x1bc */
} AoeProjectileRor;

typedef struct {
    AoeMoveActionOps move; /* resolve, target_released/attached (also used
                              by the launcher setter 0x004073f0) */
    float (*tick_seconds)(void *context);                 /* world +0x84 */
    float (*target_speed)(void *context, AoeLiveUnit *t);   /* +0xf4 */
    float (*target_heading)(void *context, AoeLiveUnit *t); /* +0xf8 */
    /* 0x004495b0: the projectile keeps a private copy of the launcher's
       attacks, blast and terrain fields once the launcher dies. */
    void (*capture_launcher)(void *context, AoeLiveUnit *projectile,
                             AoeLiveUnit *launcher);
    void (*destroy)(void *context, AoeLiveUnit *projectile); /* +0x6c */
    void (*set_facing)(void *context, AoeLiveUnit *projectile,
                       float angle);                       /* +0xb4 */
    /* 0x0044be10 then 0x004491d0: store the velocity, move by it (leaving
       the map destroys the projectile through +0x6c), and report 1 when
       the placed height equals the ground height (landed). */
    int (*fly)(void *context, AoeLiveUnit *projectile, float vx, float vy,
                float vz);
    /* 0x00449280(step): units overlapping the projectile, in the original
       tile-walk order (see aoe_projectile_overlaps_ror). Returns the
       count written (at most `capacity`). */
    size_t (*hit_query)(void *context, AoeLiveUnit *projectile, float step,
                        AoeLiveUnit **out, size_t capacity);
    /* Striker vtable +0x22c(victim, striker, x, y, z) and +0x220(x, y, z,
       striker, 0). */
    void (*hit)(void *context, AoeLiveUnit *striker, AoeLiveUnit *victim,
                float x, float y, float z);
    void (*blast)(void *context, AoeLiveUnit *striker, float x, float y,
                  float z);
    int16_t (*owner_id)(void *context, AoeLiveUnit *unit); /* player +0x4a */
    /* Unit owner players compared by identity in the original. */
    int (*same_owner)(void *context, AoeLiveUnit *a, AoeLiveUnit *b);
} AoeProjectileOps;

/* 0x00404990: type 8, aim point, target (0x00407380) and launcher
   (0x004073f0). */
int aoe_projectile_init_ror(AoeProjectileRor *p, AoeLiveUnit *projectile,
                            AoeLiveUnit *launcher, AoeLiveUnit *target,
                            float x, float y, float z, float launch_radius,
                            const AoeProjectileOps *ops);

/* +0x5c = 0x00404ab0. State 1 destroys the projectile. State 3 aims at the
   target (centre height: z + height / 2) or the point, leading a moving
   target when template +0x149 == 1 (launcher IDs 35, 36 and 280 cap the
   target speed at 1.0); flight mode 0 stores the unit direction, 3 the
   whole offset, and both face it; 1/2 do not move. With a positive arc
   (+0x150) the flight time d / speed sets gravity -2 d arc / (t/2)^2 and
   climb 2 d arc / (t/2). Then state 4. */
int aoe_projectile_set_state_ror(AoeProjectileRor *p, uint8_t state,
                                 const AoeProjectileOps *ops);

/* +0x1c = 0x00404a10: with `aim`, a target or a valid point (neither
   coordinate the -1.0 marker) aims (3), otherwise stops (1); without, it
   flies as is (4). */
int aoe_projectile_activate_ror(AoeProjectileRor *p, int aim,
                                const AoeProjectileOps *ops);

/* Per-unit test of 0x00449280 (which finds nothing unless the projectile
   footprint is positive): `other` (with a positive footprint)
   overlaps when -(other height + 0.1) <= dz <= projectile height + 0.1
   and both axis gaps |d| - (other size + projectile size + step) are
   <= 0. The tile scan covers ftol(x -/+ 3.0) by ftol(y -/+ 3.0), clamped
   to the map, and skips the projectile itself. */
int aoe_projectile_overlaps_ror(const AoeLiveUnit *projectile,
                                const AoeLiveUnit *other, float step);

/* +0x28 = 0x00404db0. Returns 1 once the projectile is done (state 1),
   otherwise 0; -1 for missing dependencies (host-only). Drops a vanished
   or dying target, and a dying launcher after capturing its attack data.
   In state 4: mode 0 moves step = tick * speed along the direction and
   expires (state 1) once `traveled` reaches the launch radius; mode 3
   moves the whole offset; the arc adds climb. Mode 3 then hits the target;
   modes 0-2 hit what overlaps per hit mode (+0x14a):
   - 0: only the target;
   - 1: units of other, non-Gaia owners;
   - 2: anything but the launcher, blasting at own or Gaia units.
   The launcher strikes while alive, otherwise the projectile itself. A
   landed projectile still in flight blasts and stops. */
int aoe_projectile_update_ror(AoeProjectileRor *p,
                              const AoeProjectileOps *ops);

/* The hit list 0x00449280 collects (0x14-byte nodes, newest first):
   0x004ab640 empties the head, 0x004ab680 pushes (unit, two values, a
   flag), 0x004ab650 frees every node. */
typedef struct AoeHitNodeRor {
    void *unit;                 /* +0x00 */
    float a, b;                 /* +0x04, +0x08 */
    uint8_t flag;               /* +0x0c */
    struct AoeHitNodeRor *next; /* +0x10 */
} AoeHitNodeRor;

void aoe_hit_list_init_ror(AoeHitNodeRor **head);
int aoe_hit_list_push_ror(AoeHitNodeRor **head, void *unit, float a, float b, uint8_t flag);
void aoe_hit_list_free_ror(AoeHitNodeRor **head);

#endif
