#include "command_dispatch.h"

static AoeCommandRouteRor call(const AoeCommandBindingRor *binding,
                               const uint8_t *command, size_t size) {
    if (!binding->handler) return AOE_COMMAND_ROUTE_UNBOUND;
    return binding->handler(binding->context, command, size) < 0
               ? AOE_COMMAND_ROUTE_INVALID
               : AOE_COMMAND_ROUTE_HANDLED;
}

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
    return call(binding, command, size);
}

int aoe_command_opcode_tribe_routed_ror(uint8_t opcode) {
    return opcode == 4 ||
           (opcode >= AOE_COMMAND_TRIBE_FIRST_ROR && opcode <= 0x6f) ||
           opcode == AOE_COMMAND_TRIBE_LAST_ROR;
}

AoeCommandRouteRor aoe_command_dispatch_tribe_ror(
    const AoeCommandTribeDispatchRor *dispatch, const uint8_t *command,
    size_t size) {
    if (!dispatch || !dispatch->rng || !command || !size)
        return AOE_COMMAND_ROUTE_INVALID;
    (void)aoe_rng_next(dispatch->rng);
    if (!aoe_command_opcode_tribe_routed_ror(command[0]))
        return aoe_command_dispatch_ror(&dispatch->base, command, size);
    if (command[0] == 4) return call(&dispatch->opcode4, command, size);
    return call(&dispatch->tribe[command[0] - AOE_COMMAND_TRIBE_FIRST_ROR],
                command, size);
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

static int player_tribute(void *context, const uint8_t *command,
                          size_t size) {
    return aoe_command_execute_player_tribute_ror(command, size, context);
}

void aoe_command_bind_player_tribute_ror(AoeCommandDispatchRor *dispatch,
                                         const AoeCommandPlayerOps *ops) {
    if (!dispatch) return;
    dispatch->opcodes[AOE_COMMAND_PLAYER_TRIBUTE_ROR].handler = player_tribute;
    dispatch->opcodes[AOE_COMMAND_PLAYER_TRIBUTE_ROR].context = (void *)ops;
}

static int object_create(void *context, const uint8_t *command, size_t size) {
    return aoe_command_execute_object_create_ror(command, size, context);
}

static int object_create_tribe(void *context, const uint8_t *command,
                               size_t size) {
    return aoe_command_execute_object_create_tribe_ror(command, size,
                                                       context);
}

void aoe_command_bind_object_create_ror(AoeCommandDispatchRor *dispatch,
                                        const AoeCommandObjectOps *ops) {
    if (!dispatch) return;
    dispatch->opcodes[AOE_COMMAND_OBJECT_CREATE_ROR].handler = object_create;
    dispatch->opcodes[AOE_COMMAND_OBJECT_CREATE_ROR].context = (void *)ops;
}

void aoe_command_bind_object_create_tribe_ror(
    AoeCommandTribeDispatchRor *dispatch, const AoeCommandObjectOps *ops) {
    if (!dispatch) return;
    dispatch->opcode4.handler = object_create_tribe;
    dispatch->opcode4.context = (void *)ops;
}

static int building_queue(void *context, const uint8_t *command,
                          size_t size) {
    return aoe_command_execute_building_queue_ror(command, size, context);
}

void aoe_command_bind_building_queue_ror(AoeCommandTribeDispatchRor *dispatch,
                                         const AoeCommandBuildingOps *ops) {
    if (!dispatch) return;
    dispatch->tribe[AOE_COMMAND_BUILDING_QUEUE_ROR -
                    AOE_COMMAND_TRIBE_FIRST_ROR].handler = building_queue;
    dispatch->tribe[AOE_COMMAND_BUILDING_QUEUE_ROR -
                    AOE_COMMAND_TRIBE_FIRST_ROR].context = (void *)ops;
}

static int order(void *context, const uint8_t *command, size_t size) {
    return aoe_command_execute_order_ror(command, size, context);
}

static int stop(void *context, const uint8_t *command, size_t size) {
    return aoe_command_execute_stop_ror(command, size, context);
}

static int order_formation(void *context, const uint8_t *command,
                           size_t size) {
    return aoe_command_execute_order_formation_ror(command, size, context);
}

static int order_alternate(void *context, const uint8_t *command,
                           size_t size) {
    return aoe_command_execute_order_alternate_ror(command, size, context);
}

void aoe_command_bind_order_ror(AoeCommandDispatchRor *dispatch,
                                const AoeCommandOrderOps *ops) {
    if (!dispatch) return;
    dispatch->opcodes[AOE_COMMAND_ORDER_ROR].handler = order;
    dispatch->opcodes[AOE_COMMAND_ORDER_ROR].context = (void *)ops;
    dispatch->opcodes[AOE_COMMAND_STOP_ROR].handler = stop;
    dispatch->opcodes[AOE_COMMAND_STOP_ROR].context = (void *)ops;
    dispatch->opcodes[AOE_COMMAND_ORDER_ALTERNATE_ROR].handler =
        order_alternate;
    dispatch->opcodes[AOE_COMMAND_ORDER_ALTERNATE_ROR].context = (void *)ops;
    dispatch->opcodes[AOE_COMMAND_ORDER_FORMATION_ROR].handler =
        order_formation;
    dispatch->opcodes[AOE_COMMAND_ORDER_FORMATION_ROR].context = (void *)ops;
}
