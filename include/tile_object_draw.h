#ifndef VITA_AOE_TILE_OBJECT_DRAW_H
#define VITA_AOE_TILE_OBJECT_DRAW_H

#include <stddef.h>
#include <stdint.h>

/* Fields read by Rise of Rome FUN_00451fd0 while visiting a tile's existing
   linked object list. The caller flattens that list in its original order;
   this routine never invents a depth sort or object. */
typedef struct {
    void *object;                  /* Original node's object, or NULL. */
    const void *normal_graphic;   /* Actor +0x10c RoR, +0x108 base. */
    const void *alternate_graphic;/* Object +0x90, for class 3. */
    uint8_t draw_class;           /* Unit definition +0x65. */
    uint8_t alternate_rank;       /* Object +0x48, for class 2. */
    uint8_t marker_25;            /* Object +0x4e equals 0x19. */
    uint8_t alternate_eligible;   /* Class-3 nested graphic pointer exists. */
} AoeTileObjectDrawEntry;

typedef void (*AoeTileObjectDrawVisit)(void *object, const void *graphic,
                                        void *context);

typedef void (*AoeTileObjectDrawVisitAt)(void *object, const void *graphic,
                                         void *renderer,
                                         int32_t screen_x, int32_t screen_y,
                                         void *context);

/* Direct filtering/order translation of RoR 0x00451fd0. alternate_pass is
   its final nonzero flag, passed by the viewport when tile state is 0x80.
   The original dispatches vtable +0x0c for each retained object. Returns
   the number of visits; NULL entries are skipped as in the linked list. */
size_t aoe_tile_object_visit_draw(const AoeTileObjectDrawEntry *entries,
                                  size_t count, int alternate_pass,
                                  AoeTileObjectDrawVisit visit,
                                  void *context);

/* Coordinate-preserving form of the same RoR 0x00451fd0 traversal. The
   original forwards its renderer argument and already-projected tile anchor
   unchanged to object vtable +0x0c. This avoids reconstructing unit screen
   placement later. */
size_t aoe_tile_object_visit_draw_at(
    const AoeTileObjectDrawEntry *entries, size_t count, int alternate_pass,
    void *renderer, int32_t screen_x, int32_t screen_y,
    AoeTileObjectDrawVisitAt visit, void *context);

/* Base Empires.exe 0x0049ccd0 has the same linked-list phase but only the
   class-0 and class-3 alternate-pass filters. Its actor graphic is +0x108
   rather than Rise of Rome's +0x10c; caller supplies the edition's pointer. */
size_t aoe_tile_object_visit_draw_base(
    const AoeTileObjectDrawEntry *entries, size_t count,
    int alternate_pass, AoeTileObjectDrawVisit visit, void *context);

#endif
