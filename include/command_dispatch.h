#ifndef VITA_AOE_COMMAND_DISPATCH_H
#define VITA_AOE_COMMAND_DISPATCH_H

#include <stddef.h>
#include <stdint.h>

#include "aoe_rng.h"
#include "command_building.h"
#include "command_dependency.h"
#include "command_game.h"
#include "command_object.h"
#include "command_order.h"
#include "command_place.h"
#include "command_player_task.h"
#include "command_production.h"
#include "command_tribe_unit.h"
#include "command_player.h"

/* Patched Rise of Rome 1.0a command-processor vtable 0x00543498 slot +0x08
   = 0x0042a180, called per queued command by 0x0042a140. Opcode byte +0x00
   is zero-extended and range-checked against 0x11 before jump table
   0x0042a230. Opcode 0x0d and opcodes above 0x11 reach a bare `ret 4`.
   Opcode 7 alone calls processor vtable slot +0x00 (0x0042ad40 in the base
   vtable) so derived processors can override it; the rest are direct calls:
     0 0042a280   1 0042a730   2 0042a780   3 0042a850   4 0042acd0
     5 0042ad10   6 0042ae20   8 0042b460   9 0042b520   a 0042b120
     b 0042b290   c 0042b3b0   e 0042b4a0   f 0042b4e0  10 0042b3f0
    11 0042b1a0 */
enum { AOE_COMMAND_OPCODE_COUNT_ROR = 0x12 };

typedef int (*AoeCommandHandlerRor)(void *context, const uint8_t *command,
                                    size_t size);

typedef struct {
    AoeCommandHandlerRor handler;
    void *context;
} AoeCommandBindingRor;

typedef struct {
    /* Index 7 is the processor's current vtable slot +0x00. */
    AoeCommandBindingRor opcodes[AOE_COMMAND_OPCODE_COUNT_ROR];
} AoeCommandDispatchRor;

typedef enum {
    AOE_COMMAND_ROUTE_IGNORED = 0,   /* original no-op: 0x0d or > 0x11 */
    AOE_COMMAND_ROUTE_HANDLED = 1,
    AOE_COMMAND_ROUTE_UNBOUND = -1,  /* original handler not yet bound */
    AOE_COMMAND_ROUTE_INVALID = -2   /* missing record or handler error */
} AoeCommandRouteRor;

/* Returns whether the original routes this opcode to a handler. */
int aoe_command_opcode_routed_ror(uint8_t opcode);

/* Routes one command. A routed opcode without a binding is reported as
   UNBOUND rather than silently dropped, so missing translations surface at
   integration time. A negative handler result is reported as INVALID; the
   original handlers return nothing, so other results count as HANDLED. */
AoeCommandRouteRor aoe_command_dispatch_ror(
    const AoeCommandDispatchRor *dispatch, const uint8_t *command,
    size_t size);

/* Derived ("tribe") processor vtable 0x00549904 slot +0x08 = 0x004e83e0,
   is the processor real RoR sessions run. Traced chain: entry 0x00528f78
   -> WinMain 0x004436f0 constructs only tribe game 0x004fdfa0 (vtable
   0x0054a264) -> game +0xbc 0x004ff3c0 builds tribe world 0x00509ae0
   (vtable 0x0054a4f8) -> world +0x54 0x0050a090 -> 0x004e8390 installs
   0x00549904 at world +0x58. The base world's +0x54 (0x0051c790, base
   processor) is reached only through base-world vtable 0x0054a954.
   0x004e83e0 first calls CRT rand() (0x0052605d) once per command and
   discards the result, so every dispatched command advances the shared
   seed. It then decodes the opcode through byte table 0x004e84fc and jump
   table 0x004e84c0:
      4 004e8570 (overrides base 0042acd0)
     64 004e85e0  65 004e8780  66 004e8920  67 004e8eb0  68 004e8e20
     69 004e8b00  6a 004e9130  6b 004e9160  6c vtable +0x10 = 004e91b0
     6d 004e9340  6e 004e9380  6f 004e93d0  77 004e9450
   Every other value, including 0x70..0x76 and values above 0x77, falls
   through to 0x0042a180. */
enum {
    AOE_COMMAND_TRIBE_FIRST_ROR = 0x64,
    AOE_COMMAND_TRIBE_LAST_ROR = 0x77,
    AOE_COMMAND_TRIBE_COUNT_ROR =
        AOE_COMMAND_TRIBE_LAST_ROR - AOE_COMMAND_TRIBE_FIRST_ROR + 1
};

typedef struct {
    AoeCommandDispatchRor base;    /* fallback 0x0042a180; its [4] unused */
    AoeCommandBindingRor opcode4;  /* 0x004e8570 */
    /* Indexed by opcode - 0x64. Only routed slots are consulted. */
    AoeCommandBindingRor tribe[AOE_COMMAND_TRIBE_COUNT_ROR];
    AoeRng *rng;                   /* shared CRT seed */
} AoeCommandTribeDispatchRor;

/* Whether 0x004e83e0 handles the opcode itself (not via 0x0042a180). */
int aoe_command_opcode_tribe_routed_ror(uint8_t opcode);

/* Advances `rng` once, exactly as the original does for every command,
   then routes. Returns INVALID without advancing when the dispatcher, RNG
   or record is missing. */
AoeCommandRouteRor aoe_command_dispatch_tribe_ror(
    const AoeCommandTribeDispatchRor *dispatch, const uint8_t *command,
    size_t size);

/* Bindings for opcodes that are already translated. Each stores `ops` by
   reference; the caller keeps it alive for the dispatcher's lifetime. */
void aoe_command_bind_dependency_ror(AoeCommandDispatchRor *dispatch,
                                     const AoeCommandDependencyOps *ops);
void aoe_command_bind_player_resource_ror(AoeCommandDispatchRor *dispatch,
                                          const AoeCommandPlayerOps *ops);
/* Opcodes 0-3 and 6: 0x0042a280 unit order, 0x0042a730 stop, 0x0042a780
   alternate order, 0x0042a850 +0x9c order and 0x0042ae20 arrangement
   (shared by both processors). */
void aoe_command_bind_order_ror(AoeCommandDispatchRor *dispatch,
                                const AoeCommandOrderOps *ops);
/* Opcodes 0x0a, 0x0b, 0x0c, 0x10 and 0x11 (player-virtual commands). */
void aoe_command_bind_player_task_ror(AoeCommandDispatchRor *dispatch,
                                      const AoeCommandPlayerTaskOps *ops);
/* Opcode 4: base 0x0042acd0 into the base table, and tribe 0x004e8570 into
   the derived override slot. */
void aoe_command_bind_object_create_ror(AoeCommandDispatchRor *dispatch,
                                        const AoeCommandObjectOps *ops);
void aoe_command_bind_object_create_tribe_ror(
    AoeCommandTribeDispatchRor *dispatch, const AoeCommandObjectOps *ops);
/* Tribe opcode 0x6c: fee tribute through derived processor +0x10. */
void aoe_command_bind_player_tribute_fee_ror(
    AoeCommandTribeDispatchRor *dispatch, const AoeCommandPlayerOps *ops);
/* Tribe opcodes 0x66 (building placement) and 0x69 (walls). */
void aoe_command_bind_place_ror(AoeCommandTribeDispatchRor *dispatch,
                                const AoeCommandPlaceOps *ops);
/* Tribe opcode 0x67: game control (diplomacy, cheats, sync error). */
void aoe_command_bind_game_ror(AoeCommandTribeDispatchRor *dispatch,
                               const AoeCommandGameOps *ops);
/* Tribe opcodes 0x64 (train) and 0x65 (research). */
void aoe_command_bind_production_ror(AoeCommandTribeDispatchRor *dispatch,
                                     const AoeCommandProductionOps *ops);
/* Tribe opcodes 0x68, 0x6a, 0x6b, 0x6d, 0x6e and 0x6f. */
void aoe_command_bind_tribe_unit_ror(AoeCommandTribeDispatchRor *dispatch,
                                     const AoeCommandTribeUnitOps *ops);
/* Tribe opcode 0x77: 0x004e9450 production queue add/cancel. */
void aoe_command_bind_building_queue_ror(AoeCommandTribeDispatchRor *dispatch,
                                         const AoeCommandBuildingOps *ops);
/* Opcode 7: both processor vtables keep 0x0042ad40 in slot +0x00. */
void aoe_command_bind_player_tribute_ror(AoeCommandDispatchRor *dispatch,
                                         const AoeCommandPlayerOps *ops);

/* The command processor (0x18 bytes; base vtable 0x00543498, tribe
   0x00549904). 0x0042a0b0 constructs it (world +0x04, comm +0x14, the
   buffer +0x08 and +0x0c cleared; the base world's +0x54 0x0051c790 and
   the tribe constructor 0x004e8390 build one at world +0x58). 0x0042a100
   destroys it (the buffer freed, world and comm cleared; thunk 0x004e83d0,
   deleting 0x0042a0e0 / 0x004e83b0). 0x0042a140: with a comm object,
   each next command (0x0041ead0) goes to +0x08 and is freed. */
typedef struct {
    void *world;          /* +0x04 */
    uint8_t *buffer;      /* +0x08 */
    int32_t field_0c;     /* +0x0c */
    void *comm;           /* +0x14 */
} AoeCommandProcessorRor;

void aoe_command_processor_init_ror(AoeCommandProcessorRor *p, void *world, void *comm);
void aoe_command_processor_release_ror(AoeCommandProcessorRor *p);
/* `next` returns a heap command (freed here) or NULL; `run` is +0x08. */
void aoe_command_processor_run_ror(AoeCommandProcessorRor *p,
                                   uint8_t *(*next)(void *context, void *comm),
                                   void (*run)(void *context, const uint8_t *command),
                                   void *context);

#endif
