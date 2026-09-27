#include "world_tick.h"

static int valid_ops(AoeWorldEdition edition, int pump,
                     const AoeWorldTickOps *ops) {
    if (!ops || !ops->timers.action_a || !ops->timers.action_b ||
        !ops->commands.next || !ops->commands.dispatch ||
        !ops->commands.release || !ops->timeline.object_class ||
        !ops->timeline.action_pair || !ops->timeline.spawn ||
        !ops->timeline.action_position || !ops->timeline.action_simple ||
        !ops->player.emit_state_event || !ops->player.map_update ||
        !ops->player.object_ops.predicate ||
        !ops->player.object_ops.action ||
        !ops->player.object_ops.cooperative_pending ||
        !ops->player.object_ops.cooperative_pump ||
        !ops->visibility.notify_lost ||
        !ops->world_finalize || !ops->seed_sync) return 0;
    if (edition == AOE_WORLD_BASE) return 1;
    if (edition == AOE_WORLD_RISE_OF_ROME)
        return !pump || ops->cooperative_pump;
    return 0;
}

static int valid_players(const AoeWorldTickState *state,
                         AoeWorldEdition edition) {
    int limit = edition == AOE_WORLD_BASE ? 40 : 25;
    for (int i = 0; i < state->player_count; ++i) {
        const AoePlayerUpdateState *player = state->players[i];
        if (!player) continue;
        if (!player->outcome_manager || !player->outcome_manager->owner ||
            player->group.count < 0 || player->group.count > limit)
            return 0;
    }
    return 1;
}

int aoe_world_tick_core(AoeWorldTickState *state, AoeWorldEdition edition,
                        int32_t gate_result, uint32_t now_ms,
                        int cooperative_pending, const AoeWorldTickOps *ops,
                        AoeWorldClockResult *result) {
    AoeWorldClock next_clock;
    AoeWorldClockResult next_result;
    uint16_t checkpoint;
    if (!state || !result || !valid_ops(edition, cooperative_pending, ops) ||
        !aoe_world_timers_valid(&state->timers) ||
        !aoe_visibility_registry_valid(state->registry) ||
        state->player_count < 0 ||
        state->player_count > AOE_WORLD_MAX_PLAYERS ||
        state->next_elapsed_reset < 0 ||
        (state->player_count > 0 &&
         state->next_elapsed_reset >= state->player_count) ||
        !valid_players(state, edition)) return 0;
    next_clock = state->clock;
    if (!aoe_world_clock_step(&next_clock, gate_result, now_ms, &next_result))
        return 0;
    if (!next_result.gate_accepted) {
        state->clock = next_clock;
        *result = next_result;
        return 1;
    }

    aoe_rng_seed(&state->rng, state->saved_seed);
    if (!aoe_world_timers_step(&state->timers, now_ms, &state->rng,
                               &ops->timers)) return 0;
    /* The original timer calls precede the world clock write. */
    state->clock = next_clock;
    *result = next_result;

    if (next_result.run_simulation) {
        if (!aoe_command_drain(&ops->commands)) return 0;
        if (!aoe_scenario_timeline_step(&state->timeline,
                                        state->clock.game_time_ms,
                                        &ops->timeline)) return 0;
        state->update_serial++;
        for (int i = 0; i < state->player_count; ++i) {
            AoePlayerUpdateState *player = state->players[i];
            if (!player) continue;
            state->player_elapsed_ms[i] += state->clock.delta_game_ms;
            if (!aoe_player_update_step(
                    player, edition == AOE_WORLD_BASE ? AOE_GROUP_BASE :
                    AOE_GROUP_RISE_OF_ROME, &ops->player)) return 0;
            if (!aoe_player_queue_reset(&player->queue58)) return 0;
        }
        if (edition == AOE_WORLD_RISE_OF_ROME && cooperative_pending)
            ops->cooperative_pump(ops->user, &state->rng);
        checkpoint = aoe_rng_next(&state->rng);
        if (edition == AOE_WORLD_BASE) {
            for (int i = 0; i < state->player_count; ++i) {
                AoePlayerUpdateState *player = state->players[i];
                if (!player) continue;
                if (ops->player.object_ops.cooperative_pending(
                        ops->player.object_ops.user))
                    ops->player.object_ops.cooperative_pump(
                        ops->player.object_ops.user);
                if (!aoe_object_update_list(player->object_list30.head,
                                            &ops->player.object_ops)) return 0;
                if (!aoe_visibility_watch_step(
                        &player->visibility_watch40, AOE_VISIBILITY_BASE,
                        player->outcome_manager->owner->index4a,
                        state->registry,
                        &ops->visibility)) return 0;
            }
        } else {
            for (int i = 0; i < state->player_count; ++i)
                if (state->players[i] &&
                    !aoe_object_update_list(
                        state->players[i]->object_list30.head,
                        &ops->player.object_ops)) return 0;
            for (int i = 0; i < state->player_count; ++i)
                if (state->players[i])
                    if (!aoe_visibility_watch_step(
                            &state->players[i]->visibility_watch40,
                            AOE_VISIBILITY_RISE_OF_ROME,
                            state->players[i]->outcome_manager->owner->index4a,
                            state->registry,
                            &ops->visibility)) return 0;
        }
        aoe_rng_seed(&state->rng, checkpoint);
        if (state->player_count > 0) {
            state->player_elapsed_ms[state->next_elapsed_reset] = 0;
            state->next_elapsed_reset++;
            if (state->next_elapsed_reset >= state->player_count)
                state->next_elapsed_reset = 0;
        }
        ops->world_finalize(ops->user, &state->rng);
    }
    state->saved_seed = aoe_rng_next(&state->rng);
    ops->seed_sync(ops->user, state->saved_seed);
    return 1;
}
