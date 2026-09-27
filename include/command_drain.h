#ifndef VITA_AOE_COMMAND_DRAIN_H
#define VITA_AOE_COMMAND_DRAIN_H

/* RoR 0x0042a5d0 / base 0x0041f0b0: drain the active command stream
   before player simulation. Fetch and dispatch are original-game
   dependencies that must be bound to real command implementations. */
typedef struct {
    void *user;
    int active;
    void *(*next)(void *user);
    void (*dispatch)(void *user, void *command);
    void (*release)(void *user, void *command);
} AoeCommandDrainOps;

/* Returns zero for missing dependencies. An inactive stream is left alone.
   Each fetched command is dispatched and then released before the next fetch. */
int aoe_command_drain(const AoeCommandDrainOps *ops);

#endif
