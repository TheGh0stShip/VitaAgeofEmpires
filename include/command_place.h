#ifndef VITA_AOE_COMMAND_PLACE_H
#define VITA_AOE_COMMAND_PLACE_H

#include <stddef.h>
#include <stdint.h>

#include "live_unit_factory.h"
#include "player_costs.h"

/* Patched Rise of Rome 1.0a tribe command 0x66, building placement
   (0x004e8920): builder count +0x01, player +0x02, floats x/y +0x04/+0x08,
   int16 template index +0x0c, dword +0x10 passed to placement and creation,
   and builder IDs from +0x14. */
enum {
    AOE_COMMAND_PLACE_ROR = 0x66,
    AOE_COMMAND_PLACE_HEADER_ROR = 0x14,
    AOE_TEMPLATE_TOWN_CENTER_ROR = 109
};

typedef struct {
    AoeLiveUnitDependencyOps world;
    void *context;
    /* World +0x40 player slot and its +0x24 template table. */
    int (*player_present)(void *context, uint8_t player);
    const DatUnit *(*player_template)(void *context, uint8_t player,
                                      int16_t index);
    AoePlayerResources *(*resources)(void *context, uint8_t player);
    /* Template vtable +0x20(player, x, y, &out, 1, 1, 1, 1, 1, 1): nonzero
       is a placement error. The original's `out` is the handler's own
       argument slot, so a check that never writes it leaves the record
       pointer there; this translation starts it at 0 instead of truncating
       a host pointer, and implementations should always write it. */
    uint8_t (*placement_check)(void *context, uint8_t player,
                               const DatUnit *unit_template, float x, float y,
                               uint32_t *out);
    /* Player +0x28 list 0x00451370(id, -1.0f, -1.0f, 0, 0, 0). */
    AoeLiveUnit *(*find_object)(void *context, uint8_t player, int16_t id);
    /* Player vtable +0x100(code, out, index, x, y, +0x10, builder ID). */
    void (*placement_failed)(void *context, uint8_t player, uint8_t code,
                             uint32_t out, int16_t index, uint32_t x_bits,
                             uint32_t y_bits, uint32_t value_10,
                             int32_t builder_id);
    /* Template vtable +0x3c(player, x, y, 0, +0x10); may return NULL. */
    AoeLiveUnit *(*create)(void *context, uint8_t player,
                           const DatUnit *unit_template, float x, float y,
                           uint32_t value_10);
    int32_t (*owner_kind)(void *context, const AoeLiveUnit *unit);
    /* Unit vtable +0x98(target, x, y, 0). */
    void (*order)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target,
                  float x, float y);
} AoeCommandPlaceOps;

size_t aoe_command_encode_place_ror(uint8_t *out, size_t out_size,
                                    uint8_t player, float x, float y,
                                    int16_t template_index, uint32_t value_10,
                                    const int32_t *builder_ids, uint8_t count);

/* Nothing happens without the player, with index -1, or without the
   template. A placement error reports through placement_failed when the
   first builder resolves. Otherwise a town-center template (109) whose
   own `enabled` byte (+0x52) is 0 is refused silently while another live
   (state <= 2) town center exists. Payment is 0x004f1750 (multiplier 1.0,
   pop flag 0): failure reports code 0 with the placement out-value.
   Success creates the foundation and sends every resolved builder, after
   0x004ac320, through +0x98(foundation, x, y). Returns -1 for a malformed
   record or missing dependencies, 0 when nothing is placed, 1 when placed. */
int aoe_command_execute_place_ror(const uint8_t *command, size_t size,
                                  const AoeCommandPlaceOps *ops);

#endif
