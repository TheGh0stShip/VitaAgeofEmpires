#ifndef VITA_AOE_RANDOM_MAP_H
#define VITA_AOE_RANDOM_MAP_H

#include <stdint.h>

#include "aoe_rng.h"

/* Patched Rise of Rome 1.0a random-map generator (0x0046bd00..0x00473d70).
   See docs/RANDOM_MAP.md for the class map. The generator works on a tile
   byte per map cell: terrain in the low five bits, elevation in the high
   three (tile +0x05 in the original). Arithmetic follows the original's
   x87 code with 53-bit precision control: intermediates are doubles,
   rounded to float wherever the original stores a float. rand() is the
   CRT generator (AoeRng); every call the original makes, including the
   ones that only feed log lines, is kept so the sequence matches. */

typedef struct AoeRmNodeRor {
    int32_t x, y;                    /* +0x00, +0x04 */
    uint32_t value;                  /* +0x08: an int, or a float's bits */
    float priority;                  /* +0x0c */
    struct AoeRmNodeRor *next, *prev;/* +0x10, +0x14 */
} AoeRmNodeRor;

/* Map hooks for the original RGE_Map calls the modules make. */
typedef struct AoeRmMapRor AoeRmMapRor;
struct AoeRmMapRor {
    int32_t width, height;           /* map +0x08, +0x0c */
    uint8_t *tiles;                  /* row-major tile +0x05 bytes */
    void *context;
    /* Map vtable +0x10: terrain along a line (x0, y0)-(x1, y1). */
    void (*paint_line)(void *context, int32_t x0, int32_t y0, int32_t x1,
                       int32_t y1, int32_t a, int32_t b);
    /* Map vtable +0x20. */
    void (*vt20)(void *context, int32_t x0, int32_t y0, int32_t x1,
                 int32_t y1, int32_t a, int32_t b);
};

struct AoeRmModuleRor;
typedef struct AoeRmChildRor {
    struct AoeRmModuleRor *module;
    struct AoeRmChildRor *next;
} AoeRmChildRor;

typedef struct {
    int (*run)(struct AoeRmModuleRor *);       /* slot +0x00 */
    void (*destroy)(struct AoeRmModuleRor *);
} AoeRmModuleVtblRor;

/* Module base (vtable 0x00545cc8): a node in the module tree with the map,
   a priority, and work grids shared with its parent (a byte per tile and
   a search node per tile). */
typedef struct AoeRmModuleRor {
    const AoeRmModuleVtblRor *vt;
    struct AoeRmModuleRor *parent;   /* +0x04 */
    AoeRmChildRor *children;         /* +0x08 */
    AoeRmMapRor *map;                /* +0x0c (+0x10 tile rows) */
    int32_t width, height;           /* +0x14, +0x18 */
    float priority;                  /* +0x1c */
    uint8_t *grid;                   /* +0x20 */
    uint8_t **grid_rows;             /* +0x24 */
    AoeRmNodeRor *nodes;             /* +0x28 */
    AoeRmNodeRor **node_rows;        /* +0x2c */
    AoeRng *rng;
    int owns_grids;
} AoeRmModuleRor;

/* 0x0046bd00: under a parent the module joins its child list and shares
   its grids; a root allocates them (0x0046bfc0) when asked. 0x0046bdb0
   destroys the children, then frees the grids (root) or leaves the
   parent (0x0046bf60). */
void aoe_rm_module_init_ror(AoeRmModuleRor *m, const AoeRmModuleVtblRor *vt,
                            AoeRmMapRor *map, AoeRmModuleRor *parent,
                            int allocate, AoeRng *rng);
void aoe_rm_module_release_ror(AoeRmModuleRor *m);
/* 0x0046bf20 add / 0x0046bf60 remove a child. */
int aoe_rm_module_add_child_ror(AoeRmModuleRor *m, AoeRmModuleRor *child);
int aoe_rm_module_remove_child_ror(AoeRmModuleRor *m, AoeRmModuleRor *child);
/* 0x0046be20 priority; 0x0046be30: sort the children by priority
   (ascending, stable), draw rand() for the start log line, run each, and
   draw rand() again after each that succeeds. 0 if one fails. */
float aoe_rm_module_priority_ror(const AoeRmModuleRor *m);
int aoe_rm_module_run_children_ror(AoeRmModuleRor *m);
/* 0x0046bfc0 allocate, 0x0046c0a0 free, 0x0046c170 share the grids;
   0x0046c120 / 0x0046c100 set the map here and below. */
int aoe_rm_module_alloc_grids_ror(AoeRmModuleRor *m);
void aoe_rm_module_free_grids_ror(AoeRmModuleRor *m);
void aoe_rm_module_set_map_ror(AoeRmModuleRor *m, AoeRmMapRor *map);
/* Tile byte access (tile +0x05). */
uint8_t *aoe_rm_tile_ror(const AoeRmModuleRor *m, int32_t x, int32_t y);

/* Node lists. 0x0046c1b0 init a head, 0x0046c1e0 detach it from its
   nodes, 0x0046c2d0 link a node after another, 0x0046c310 unlink,
   0x0046c200 insert the node of (x, y) ordered by priority (before equal
   ones), 0x0046c290 pop the first, 0x0046c340 unlink
   every grid node, 0x0046c390 shuffle (two passes through 100 random
   buckets, gathered back in pairs). */
void aoe_rm_list_init_ror(AoeRmNodeRor *head);
void aoe_rm_list_detach_ror(AoeRmNodeRor *head);
void aoe_rm_node_link_ror(AoeRmNodeRor *after, AoeRmNodeRor *node);
void aoe_rm_node_unlink_ror(AoeRmNodeRor *node);
void aoe_rm_list_push_ror(AoeRmModuleRor *m, AoeRmNodeRor *head, int32_t x,
                          int32_t y, uint32_t value, float priority);
AoeRmNodeRor *aoe_rm_list_pop_ror(AoeRmNodeRor *head, int32_t *x, int32_t *y,
                                  uint32_t *value);
void aoe_rm_nodes_unlink_all_ror(AoeRmModuleRor *m);
void aoe_rm_list_shuffle_ror(AoeRmModuleRor *m, AoeRmNodeRor *head);
uint32_t aoe_rm_float_bits(float f);
float aoe_rm_bits_float(uint32_t bits);
/* rand() * n / 0x7fff, the original's random scaling. */
int32_t aoe_rm_rand_scaled_ror(AoeRmModuleRor *m, int32_t n);

/* 0x0046c4c0 (0x0046c470 with a local open list): best-first search from
   (x, y) to (*goal_x, *goal_y) over tiles whose terrain cost
   (costs[terrain]) is positive. The byte grid records how each tile was
   reached (4..11: from +x, -x, +y, -y and the four diagonals); 1 marks
   blocked tiles, 2 the goal, 3 the start. Diagonals cost 1.36x and need
   both adjacent straight steps open. 1 when the goal is reached. */
int aoe_rm_search_ror(AoeRmModuleRor *m, AoeRmNodeRor *open, int32_t x,
                      int32_t y, int32_t *goal_x, int32_t *goal_y,
                      const float *costs);
int aoe_rm_find_path_ror(AoeRmModuleRor *m, int32_t x, int32_t y,
                         int32_t *goal_x, int32_t *goal_y,
                         const float *costs);

#endif
