#include "command_dispatch.h"

int aoe_command_opcode_routed_ror(uint8_t opcode) {
    return opcode < AOE_COMMAND_OPCODE_COUNT_ROR && opcode != 0x0d;
}

AoeCommandRouteRor aoe_command_dispatch_ror(
    const AoeCommandDispatchRor *dispatch, const uint8_t *command,
    size_t size) {
    const AoeCommandBindingRor *binding;
    if (!dispatch || !command || !size) return AOE_COMMAND_ROUTE_INVALID;
    if (!aoe_command_opcode_routed_ror(command[0]))
        return AOE_COMMAND_ROUTE_IGNORED;
    binding = &dispatch->opcodes[command[0]];
    if (!binding->handler) return AOE_COMMAND_ROUTE_UNBOUND;
    return binding->handler(binding->context, command, size) < 0
               ? AOE_COMMAND_ROUTE_INVALID
               : AOE_COMMAND_ROUTE_HANDLED;
}

static int dependency(void *context, const uint8_t *command, size_t size) {
    return aoe_command_execute_dependency_ror(command, size, context);
}

static int player_resource(void *context, const uint8_t *command,
                           size_t size) {
    return aoe_command_execute_player_resource_add_ror(command, size,
                                                       context);
}

void aoe_command_bind_dependency_ror(AoeCommandDispatchRor *dispatch,
                                     const AoeCommandDependencyOps *ops) {
    static const uint8_t opcodes[] = {
        AOE_COMMAND_DEPENDENCY_SET_ROR, AOE_COMMAND_DEPENDENCY_CLEAR_ROR,
        AOE_COMMAND_DEPENDENCY_PROPAGATE_ROR,
        AOE_COMMAND_DEPENDENCY_REMOVE_ROR};
    if (!dispatch) return;
    for (size_t i = 0; i < sizeof(opcodes); ++i) {
        dispatch->opcodes[opcodes[i]].handler = dependency;
        dispatch->opcodes[opcodes[i]].context = (void *)ops;
    }
}

void aoe_command_bind_player_resource_ror(AoeCommandDispatchRor *dispatch,
                                          const AoeCommandPlayerOps *ops) {
    if (!dispatch) return;
    dispatch->opcodes[AOE_COMMAND_PLAYER_RESOURCE_ADD_ROR].handler =
        player_resource;
    dispatch->opcodes[AOE_COMMAND_PLAYER_RESOURCE_ADD_ROR].context =
        (void *)ops;
}
