#include "ai_module.h"

#include <stdio.h>
#include <string.h>

static void copy63(char *out, const char *text) {
    size_t n = 0;
    while (text && n < 63 && text[n]) ++n;
    if (n) memcpy(out, text, n);
    out[n] = '\0';
}

void aoe_ai_named_init_ror(AoeAiModuleRor *module, const char *name,
                           int32_t id) {
    if (!module) return;
    module->id = id;
    if (name) copy63(module->name, name);
    else module->name[0] = '\0';
}

void aoe_ai_module_init_ror(AoeAiModuleRor *module, const char *name,
                            int32_t id, int32_t player, AoeAiLogRor *log) {
    if (!module) return;
    memset(module, 0, sizeof(*module));
    aoe_ai_named_init_ror(module, name, id);
    module->player = player;
    module->field_e4 = 5;
    module->field_e8 = 50;
    module->field_ec = 0;
    copy63(module->player_name, "Unknown");
    module->log = log;
}

static void close_log(AoeAiLogRor *log) {
    if (!log || !log->open_now) return;
    if (log->close) log->close(log->context);
    log->open_now = 0;
}

void aoe_ai_module_release_ror(AoeAiModuleRor *module) {
    if (module && module->logging) close_log(module->log);
}

void aoe_ai_module_set_player_ror(AoeAiModuleRor *module, int32_t player,
                                  const char *name) {
    if (!module) return;
    module->player = player;
    copy63(module->player_name, name ? name : "Unknown");
}

int32_t aoe_ai_module_field_98_ror(const AoeAiModuleRor *module) {
    return module ? module->field_98 : 0;
}

void aoe_ai_module_set_logging_ror(AoeAiModuleRor *module, int32_t on) {
    AoeAiLogRor *log;
    if (!module) return;
    module->logging = on;
    log = module->log;
    if (!log) return;
    if (!on) {
        close_log(log);
        return;
    }
    if (log->open_now) return;
    log->open_now = log->open && log->open(log->context);
    aoe_ai_log_ror(module, "Common AI History File Started %s",
                   log->time_text ? log->time_text(log->context) : "");
    aoe_ai_log_ror(module, "");
}

void aoe_ai_vlog_ror(const AoeAiModuleRor *module, const char *format,
                     va_list args) {
    const AoeAiLogRor *log = module ? module->log : NULL;
    char text[1024], line[1200];
    if (!log || !log->open_now || !log->write || !format) return;
    if (log->player_filter != -1 && log->player_filter != module->player)
        return;
    vsnprintf(text, sizeof(text), format, args);
    if (!text[0])
        snprintf(line, sizeof(line), "P#%d\n", (int)module->player);
    else
        snprintf(line, sizeof(line), "P#%d %s: %s\n", (int)module->player,
                 module->name, text);
    log->write(log->context, line);
}

void aoe_ai_log_ror(const AoeAiModuleRor *module, const char *format, ...) {
    va_list args;
    va_start(args, format);
    aoe_ai_vlog_ror(module, format, args);
    va_end(args);
}
