#ifndef VITA_AOE_COMMAND_DISPATCH_H
#define VITA_AOE_COMMAND_DISPATCH_H

#include <stddef.h>
#include <stdint.h>

#include "command_dependency.h"
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

/* Bindings for opcodes that are already translated. Each stores `ops` by
   reference; the caller keeps it alive for the dispatcher's lifetime. */
void aoe_command_bind_dependency_ror(AoeCommandDispatchRor *dispatch,
                                     const AoeCommandDependencyOps *ops);
void aoe_command_bind_player_resource_ror(AoeCommandDispatchRor *dispatch,
                                          const AoeCommandPlayerOps *ops);

#endif
