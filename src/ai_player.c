#include "ai_player.h"

#include <string.h>

#include "aoe_stream.h"

static void own_fields_zero(AoeAiPlayerAiRor *ai) {
    ai->diplomacy_time = ai->field_12580 = 0;
    ai->field_12584 = ai->tribute_delay = 0;
    ai->field_1258c = ai->field_12590 = ai->field_12594 = 0;
    ai->field_12598 = ai->field_1259c = ai->field_125a0 = 0;
    memset(ai->field_125a4, 0, sizeof ai->field_125a4);
}

/* Owner hookups, then the player name on every module, as both
   constructors do after the base's name and logging. */
static void wire_modules(AoeAiPlayerAiRor *ai, int32_t player,
                         const char *name, const AoeAiPlayerOpsRor *ops) {
    int m;
    aoe_ai_module_set_player_ror(&ai->decision.module, player, name);
    aoe_ai_module_set_logging_ror(&ai->decision.module,
                                  ops && ops->logging_on != 0);
    if (!ops) return;
    for (m = 0; m < AOE_AI_MODULES; ++m)
        if (ops->module_owner) ops->module_owner(ops->context, m);
    for (m = 0; m < AOE_AI_MODULES; ++m)
        if (ops->module_name) ops->module_name(ops->context, m, player, name);
}

void aoe_ai_player_init_ror(AoeAiPlayerAiRor *ai, int32_t player,
                            int32_t stagger, const char *name,
                            AoeAiLogRor *log, const AoeAiPlayerOpsRor *ops) {
    static const int32_t kind0[] = {0x30, 0x7e, 0x3b, 0x41, 0x34, 0x35,
                                    0x104, 0x107, 0x32};
    int32_t i;
    int m;
    if (!ai) return;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_decision_init_ror(&ai->decision, player, NULL, log);
    for (m = 0; m < AOE_AI_MODULES; ++m)
        if (ops && ops->module_init) ops->module_init(ops->context, m, NULL);
    ai->setup_pending = 1;
    ai->field_12574 = 1;
    ai->update_count = stagger;
    own_fields_zero(ai);
    wire_modules(ai, player, name, ops);
    if (ops && ops->info_kind) {
        for (i = 0; i < (int32_t)(sizeof kind0 / sizeof kind0[0]); ++i)
            ops->info_kind(ops->context, 0, kind0[i]);
        for (i = 0x6f; i < 0x73; ++i) ops->info_kind(ops->context, 1, i);
        for (i = 0x86; i < 0x9a; ++i) ops->info_kind(ops->context, 1, i);
        for (i = 0xc0; i < 0xc6; ++i) ops->info_kind(ops->context, 1, i);
        ops->info_kind(ops->context, 2, 0x66);
        ops->info_kind(ops->context, 3, 0x42);
        for (i = 0x150; i < 0x154; ++i) ops->info_kind(ops->context, 3, i);
    }
    if (ops && ops->personality) ops->personality(ops->context, 0);
    memset(ai->field_125a4, 0, sizeof ai->field_125a4);
}

int aoe_ai_player_load_ror(AoeAiPlayerAiRor *ai, int32_t player,
                           const char *name, float version,
                           AoeOutcomeStream *in, AoeAiLogRor *log,
                           const AoeAiPlayerOpsRor *ops) {
    int m, i;
    if (!ai || !in) return 0;
    memset(ai, 0, sizeof(*ai));
    if (!aoe_ai_decision_load_ror(&ai->decision, player, NULL, log, in))
        return 0;
    for (m = 0; m < AOE_AI_MODULES; ++m)
        if (ops && ops->module_init) ops->module_init(ops->context, m, in);
    ai->setup_pending = 0;
    ai->field_12574 = 0;
    ai->update_count = player;
    own_fields_zero(ai);
    wire_modules(ai, player, name, ops);
    ai->update_count = aoe_stream_i32(in);
    ai->diplomacy_time = aoe_stream_i32(in);
    ai->field_12580 = aoe_stream_i32(in);
    if (version >= 1.2f) {
        ai->field_12584 = aoe_stream_i32(in);
        ai->tribute_delay = aoe_stream_i32(in);
        ai->field_1258c = aoe_stream_i32(in);
        ai->field_12590 = aoe_stream_i32(in);
        ai->field_12594 = aoe_stream_i32(in);
        ai->field_12598 = aoe_stream_i32(in);
    }
    if (version >= 7.08f) {
        ai->field_1259c = aoe_stream_i32(in);
        ai->field_125a0 = aoe_stream_i32(in);
        for (i = 0; i < 9; ++i) ai->field_125a4[i] = aoe_stream_i32(in);
    }
    return !in->failed;
}

int aoe_ai_player_save_ror(AoeAiPlayerAiRor *ai, AoeOutcomeStream *out,
                           const AoeAiPlayerOpsRor *ops) {
    int m, i;
    if (!ai || !out) return 0;
    aoe_ai_decision_save_ror(&ai->decision, out);
    for (m = 0; m < AOE_AI_MODULES; ++m)
        if (ops && ops->module_save) ops->module_save(ops->context, m, out);
    aoe_stream_put_i32(out, ai->update_count);
    aoe_stream_put_i32(out, ai->diplomacy_time);
    aoe_stream_put_i32(out, ai->field_12580);
    aoe_stream_put_i32(out, ai->field_12584);
    aoe_stream_put_i32(out, ai->tribute_delay);
    aoe_stream_put_i32(out, ai->field_1258c);
    aoe_stream_put_i32(out, ai->field_12590);
    aoe_stream_put_i32(out, ai->field_12594);
    aoe_stream_put_i32(out, ai->field_12598);
    aoe_stream_put_i32(out, ai->field_1259c);
    aoe_stream_put_i32(out, ai->field_125a0);
    for (i = 0; i < 9; ++i) aoe_stream_put_i32(out, ai->field_125a4[i]);
    return !out->failed;
}

void aoe_ai_player_release_ror(AoeAiPlayerAiRor *ai,
                               const AoeAiPlayerOpsRor *ops) {
    int m;
    if (!ai) return;
    for (m = AOE_AI_MODULES - 1; m >= 0; --m)
        if (ops && ops->module_release) ops->module_release(ops->context, m);
    aoe_ai_decision_release_ror(&ai->decision);
}

void aoe_ai_player_set_ror(AoeAiPlayerAiRor *ai, int32_t value,
                           const AoeAiPlayerOpsRor *ops) {
    int m;
    (void)ai;
    if (!ops) return;
    for (m = 0; m < AOE_AI_MODULES; ++m)
        if (ops->module_set) ops->module_set(ops->context, m, value);
    if (ops->base_set) ops->base_set(ops->context, value);
}

int aoe_ai_player_object_added_ror(AoeAiPlayerAiRor *ai, int32_t id,
                                   int has_object,
                                   const AoeAiPlayerOpsRor *ops) {
    if (!ai || !has_object) return 0;
    if (ops && ops->tactical_added) ops->tactical_added(ops->context, id);
    if (ops && ops->info_added) ops->info_added(ops->context, id);
    return aoe_ai_decision_add_ror(&ai->decision, 1, id);
}

int aoe_ai_player_object_removed_ror(AoeAiPlayerAiRor *ai, int32_t id,
                                     const AoeAiPlayerOpsRor *ops) {
    if (!ai) return 0;
    if (ops && ops->tactical_removed) ops->tactical_removed(ops->context, id);
    if (ops && ops->build_removed) ops->build_removed(ops->context, id);
    if (ops && ops->info_removed) ops->info_removed(ops->context, id);
    return aoe_ai_decision_remove_ror(&ai->decision, id);
}

int32_t aoe_ai_player_action_class_ror(const AoeAiPlayerAiRor *ai,
                                       int32_t action) {
    switch (action) {
    case 0x25a: case 0x25d: case 0x261: case 0x265:
        return 4;
    case 0x26b: case 0x26c:
        return 3;
    default:
        return aoe_ai_unknown_action_ror(&ai->decision.module, action);
    }
}

int aoe_ai_player_object_fits_ror(const AoeAiPlayerAiRor *ai, int32_t id,
                                  int32_t action,
                                  const AoeAiPlayerOpsRor *ops) {
    int16_t c;
    if (!ops || !ops->object_class || !ops->object_class(ops->context, id, &c))
        return 0;
    return c == aoe_ai_player_action_class_ror(ai, action);
}

void aoe_ai_player_detask_ror(const AoeAiPlayerOpsRor *ops, int32_t id) {
    if (ops && ops->detask) ops->detask(ops->context, id);
}

int aoe_ai_player_active_unit_ror(const AoeAiPlayerOpsRor *ops, int32_t id) {
    int16_t c;
    if (!ops || !ops->object_class || !ops->object_class(ops->context, id, &c))
        return 0;
    switch (c) {
    case 3: case 0x1b: case 0x1e: case 0x1f: case 5: case 0x21: case 7:
    case 8: case 0x20: case 0x10: case 0xe: case 0xf:
        return 0;
    default:
        return 1;
    }
}

void aoe_ai_player_roll_delay_ror(AoeAiPlayerAiRor *ai,
                                  const AoeAiPlayerOpsRor *ops) {
    int32_t n, r;
    if (!ai || !ops || !ops->sn) return;
    n = ops->sn(ops->context, 0x7e);
    if (n > 0) {
        n = ops->sn(ops->context, 0x7e);
        r = ops->rand ? ops->rand(ops->context) : 0;
        ai->tribute_delay = r % n - ops->sn(ops->context, 0x7e) / 2 +
                            ops->sn(ops->context, 0x7d);
        return;
    }
    ai->tribute_delay = ops->sn(ops->context, 0x7d);
}

void aoe_ai_player_kick_ror(AoeAiPlayerAiRor *ai, int32_t kind,
                            const AoeAiPlayerOpsRor *ops) {
    if (!ai) return;
    aoe_ai_log_ror(&ai->decision.module, "Got a kick=%d.", kind);
    if (kind == 3) aoe_ai_log_ror(&ai->decision.module, "Current Build list is:");
    if ((kind == 1 || kind == 2 || kind == 3) && ops && ops->kick)
        ops->kick(ops->context, kind);
}
