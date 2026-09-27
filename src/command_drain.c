#include "command_drain.h"

int aoe_command_drain(const AoeCommandDrainOps *ops) {
    void *command;
    if (!ops || !ops->next || !ops->dispatch || !ops->release) return 0;
    if (!ops->active) return 1;
    while ((command = ops->next(ops->user)) != 0) {
        ops->dispatch(ops->user, command);
        ops->release(ops->user, command);
    }
    return 1;
}
