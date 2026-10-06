#ifndef VITA_AOE_UNIT_LIFECYCLE_H
#define VITA_AOE_UNIT_LIFECYCLE_H

#include <stddef.h>
#include <stdint.h>

/* The RoR 1.0a (patched EMPIRESX.EXE) unit-class constructor / destructor
   chain for the live-unit fields that are not part of the saved record
   (the record half lives in unit_record.h). Storage is semantic: pointers
   are real host pointers, never truncated 32-bit tokens, and nothing here
   mirrors the x86 object layout. Effects the original reaches through the
   owner, world, sprite, AI and action-list objects go through
   AoeUnitLifeOps; every callback is optional.

   Class vtables (for reference): root 0x547da8 (10), 0x542e14 (20),
   0x542748 (40), 0x5445e8 (60), 0x54820c (70), 0x547fa0 (80), 0x548474
   (90). */

/* The template fields the chain reads. */
typedef struct AoeUnitLifeTemplate {
    int16_t id;                /* +0x10 */
    int16_t class_code;        /* +0x14 */
    uint8_t kind;              /* +0x04 */
    int16_t hit_points;        /* +0x26 */
    int32_t standing_graphic;  /* +0x18 */
    int has_selection_sound;   /* +0x40 != 0 */
    float footprint_radius;    /* +0x38 */
    int16_t default_task;      /* +0xdc, type 80 id 0x114 only */
    /* Type 80: three build costs (+0x148, 6 bytes each: resource, amount,
       enabled byte) and the build time (+0x15a). */
    struct { int16_t resource, amount; uint8_t enabled; } build_cost[3];
    int16_t build_time;
} AoeUnitLifeTemplate;

/* The owner (player) fields the chain reads or updates. */
typedef struct AoeUnitLifeOwner {
    int16_t player_id;          /* +0x4a */
    int16_t viewed_player;      /* world (+0x3c) +0x7c */
    float *resources;           /* +0x50 float attributes */
    size_t resource_count;
    /* +0x24 shared templates, indexed by template id. */
    const AoeUnitLifeTemplate *const *templates;
    size_t template_count;
    /* Game +0xa34[player]: buildings whose status is 4. */
    int32_t *status4_counter;
} AoeUnitLifeOwner;

/* Owner resource indices touched by the type-70 counter rule. */
#define AOE_UNIT_LIFE_RES_POP_A 37u /* +0x94: classes 2, 4, 0x15 */
#define AOE_UNIT_LIFE_RES_POP_B 40u /* +0xa0: 'F' kinds and ids 0x45/0x4f/199 */

typedef struct AoeUnitLife AoeUnitLife;

typedef struct AoeUnitLifeOps {
    void *ctx;
    void (*set_graphic)(void *ctx, AoeUnitLife *u, int32_t graphic); /* vt+0x38 */
    void (*place)(void *ctx, AoeUnitLife *u, float x, float y, float z); /* vt+0x34 */
    void (*storage_reset)(void *ctx, AoeUnitLife *u); /* 0x004a7150 + 0x004a71a0 */
    /* Owner vt+0xdc: returns the owner list node. */
    void *(*owner_register)(void *ctx, AoeUnitLife *u, uint8_t alive, uint8_t doppel);
    /* Owner vt+0xe0. */
    void (*owner_unregister)(void *ctx, AoeUnitLife *u, uint8_t alive, uint8_t doppel,
                             void *node);
    int32_t (*rand_draw)(void *ctx);           /* C runtime rand() */
    void (*srand_seed)(void *ctx, int32_t seed);
    void (*play_selection_sound)(void *ctx, AoeUnitLife *u); /* 0x004a2370(1) */
    void (*sprites_stop)(void *ctx, AoeUnitLife *u);         /* 0x004aa350 */
    void (*tile_drop)(void *ctx, AoeUnitLife *u);            /* 0x004a7200 */
    void (*outcome_drop)(void *ctx, AoeUnitLife *u);         /* 0x0050da90 */
    void (*sprite_manager_free)(void *ctx, void *mgr);
    void (*selection_remove)(void *ctx, AoeUnitLife *u);     /* 0x0045dd00 */
    void (*tile_unlink)(void *ctx, AoeUnitLife *u, void *node); /* 0x00450fe0 */
    void (*footprint_lift)(void *ctx, AoeUnitLife *u);       /* 0x004ab7f0(2) */
    void (*content_release)(void *ctx, void *member);        /* member vt+0xd4 */
    void *(*ai_make)(void *ctx, AoeUnitLife *u);             /* vt+0x254 */
    void (*ai_free)(void *ctx, void *ai);
    void *(*action_list_make)(void *ctx, AoeUnitLife *u);    /* vt+0x21c */
    void (*action_list_free)(void *ctx, void *list);         /* 0x00403d70 + free */
    void (*action_list_clear)(void *ctx, void *list);        /* 0x00403e50 */
    void (*action_list_relink)(void *ctx, void *list);       /* 0x00403df0 */
    void (*post_load_fixup)(void *ctx, AoeUnitLife *u);      /* root post-load fixup */
    void (*wake)(void *ctx, AoeUnitLife *u);                 /* 0x004ac380(0) */
    void (*template_free)(void *ctx, const AoeUnitLifeTemplate *t);
    void *(*watch_add)(void *ctx, AoeUnitLife *u, int32_t tx, int32_t ty); /* 0x004368f0 */
    void (*watch_remove)(void *ctx, AoeUnitLife *u);          /* 0x00436980 */
    void (*default_task)(void *ctx, AoeUnitLife *u, int16_t task); /* 0x004063d0 */
    void (*building_init)(void *ctx, AoeUnitLife *u);         /* 0x004ac8a0 */
    void (*pairs_free)(void *ctx, void *pairs);
    void (*queue_free)(void *ctx, void *queue);
    void (*combat_release)(void *ctx, AoeUnitLife *u);       /* 0x004496a0 */
    void (*root_cleanup)(void *ctx, AoeUnitLife *u);         /* 0x004a6920 */
    /* Owner vt+0x78(resource, amount, 0): add to an owner attribute. */
    void (*owner_add_attribute)(void *ctx, AoeUnitLife *u, int16_t resource, float amount);
    void (*remove_finished)(void *ctx, AoeUnitLife *u);      /* 0x004ae960 */
    /* The type-30 set-graphic slot: 0x004a9e10 select a graphic and
       0x0044bbd0 refresh the facet. */
    void (*graphic_select)(void *ctx, AoeUnitLife *u, int32_t graphic);
    void (*facet_refresh)(void *ctx, AoeUnitLife *u);
} AoeUnitLifeOps;

struct AoeUnitLife {
    uint8_t class_code;        /* +0x4e */
    const AoeUnitLifeTemplate *tmpl; /* +0x08 */
    uint8_t private_template;  /* +0x1bc (type 70+) */
    AoeUnitLifeOwner *owner;   /* +0x0c */
    float hit_points;          /* +0x30 */
    uint8_t alive;             /* +0x49 */
    uint8_t doppelganger;      /* +0x4a: template kind 0x19 */
    uint8_t state;             /* +0x48 */
    uint8_t flag_34, flag_35, selected_bits, flag_37, flag_4b, flag_4f, flag_86;
    float x, y, z;             /* +0x38 / +0x3c / +0x40 */
    int16_t draw_rect[4];      /* +0x28..+0x2e */
    int32_t graphic_10, graphic_14; /* +0x10, +0x14 */
    void *sprite_manager;      /* +0x18 */
    void *tile_node, *tile_node2; /* +0x1c, +0x20 */
    void **contents;           /* +0x24 members */
    size_t content_count;
    void *owner_node;          /* +0x50 */
    void *buf_a; uint32_t buf_a_n[3]; /* +0x54..+0x60 */
    void *buf_b; uint32_t buf_b_n[3]; /* +0x64..+0x70 */
    void *ai;                  /* +0x74 */
    int32_t source_id_78, source_id_7c; /* -1 */
    float range_80;            /* 5.0 */
    uint8_t flag_84, flag_85;
    int32_t type20_88;
    /* type 40 */
    uint8_t type40_180; void *action_list; int32_t type40_188;
    /* type 60 */
    int32_t type60_1c0;
    /* type 70 */
    uint8_t gaia_owned;       /* +0x1b8 */
    float carried_amount;      /* +0x44 */
    int16_t carried_type;      /* +0x4c */
    /* type 80 */
    void *pairs;               /* +0x1c0 */
    int16_t pair_count, pair_cap; /* +0x1c4, +0x1c6 */
    uint8_t flag_1c8;
    void *queue;               /* +0x1cc */
    uint8_t flag_1d0, flag_1d1;
    int16_t status;            /* +0x1d2 */
    float build_progress;      /* +0x1d4 */
    int32_t completion_value;  /* +0x1d8 */
    void *watch;               /* +0x1e0 */
    int32_t tail_1e4[6];       /* +0x1e4..+0x1f8 */
};

/* Root load-constructor defaults (0x004a6590) */
void aoe_unit_life_load_defaults(AoeUnitLife *u);
/* Root init (0x004a67d0) */
void aoe_unit_life_init_root(AoeUnitLife *u, const AoeUnitLifeOps *ops,
                             const AoeUnitLifeTemplate *t, AoeUnitLifeOwner *owner,
                             float x, float y, float z);
/* Root destructor (0x004a6640). The type-25 doppelganger has no destructor
   of its own: its deleting form 0x004361d0 runs this root destructor and frees
   the object when bit 0 of the flag is set. */
void aoe_unit_life_destroy_root(AoeUnitLife *u, const AoeUnitLifeOps *ops);
/* Type 20 (0x00415020 / 0x00415170) and type 90 (0x004b0d20) inits */
void aoe_unit_life_init_type20(AoeUnitLife *u, const AoeUnitLifeOps *ops,
                               const AoeUnitLifeTemplate *t, AoeUnitLifeOwner *owner,
                               float x, float y, float z);
void aoe_unit_life_init_type90(AoeUnitLife *u, const AoeUnitLifeOps *ops,
                               const AoeUnitLifeTemplate *t, AoeUnitLifeOwner *owner,
                               float x, float y, float z);
/* Type 40 (0x00405b60 reinit tail, 0x00405cc0, 0x00405d20) */
void aoe_unit_life_init_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops);
void aoe_unit_life_reinit_type40(AoeUnitLife *u);
void aoe_unit_life_post_load_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops);
void aoe_unit_life_reset_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops);
void aoe_unit_life_destroy_type40(AoeUnitLife *u, const AoeUnitLifeOps *ops);
/* Type 60 (0x00448ec0 init tail, 0x00448e50 dtor, 0x00448ea0 cleanup) */
void aoe_unit_life_init_type60(AoeUnitLife *u);
void aoe_unit_life_destroy_type60(AoeUnitLife *u, const AoeUnitLifeOps *ops);
void aoe_unit_life_cleanup_type60(AoeUnitLife *u, const AoeUnitLifeOps *ops);
/* Type 70 (0x004ae4e0 init, 0x004ae3f0 dtor) */
void aoe_unit_life_init_type70(AoeUnitLife *u, const AoeUnitLifeOps *ops);
void aoe_unit_life_destroy_type70(AoeUnitLife *u, const AoeUnitLifeOps *ops);
/* Type 80 (0x004ac3f0 / 0x004ac550 / 0x004ac670, 0x004ac740, 0x004ac810) */
void aoe_unit_life_construct_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops,
                                    int32_t completion_value, int run_init);
void aoe_unit_life_load_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops);
void aoe_unit_life_destroy_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops);
void aoe_unit_life_cleanup_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops);
/* 0x004adf50: drop a type-80 unit's queue. A set status (+0x1d2) becomes
   -1, releasing the owner's status-4 count when it was 4; +0x1c6 is
   zeroed, the +0x1cc queue cleared (0x00403e50; skipped here when absent)
   and the +0x1d0 counter bumped. The pairs themselves are kept. */
void aoe_unit_life_reset_queue_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops);

/* 0x0044b8e0 (vtable +0x38 of types 30..80): `graphic` 0 means the
   template's standing graphic (+0x18); when it differs from the current
   one (+0x10) the current becomes the previous (+0x14), the graphic is
   selected (0x004a9e10) and the facet refreshed (0x0044bbd0). */
void aoe_unit_life_set_graphic_type30(AoeUnitLife *u, const AoeUnitLifeOps *ops, int32_t graphic);
/* 0x004acf60 (vtable 0x00547fa0 +0x2c): type-80 post-load, the type-40
   one (0x00405cc0) then the +0x1cc queue relinked (0x00403df0). */
void aoe_unit_life_post_load_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops);
/* 0x004ad6d0 (vtable 0x00547fa0 +0x244, overriding 0x004ae960): removing
   a type-80 unit. From state 2 on it is 0x004ae960. Below that, a
   selected unit (+0x36 bit 0) leaves the selection (0x0045dd00); the
   unbuilt share of the cost is refunded: fraction = (build_time -
   progress) / build_time * 0.5, clamped to 1, and 0 when hit points are
   not positive. Each enabled cost with a resource >= 0 adds amount *
   fraction to the owner (vt+0x78, even when the fraction is 0); then the
   hit points become 0. */
void aoe_unit_life_remove_type80(AoeUnitLife *u, const AoeUnitLifeOps *ops);

#endif
