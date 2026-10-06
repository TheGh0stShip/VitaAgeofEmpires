#include "net_misc.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void aoe_host_clear_ror(AoeHostInfoRor *h) {
    h->name[0] = '\0';
    h->alias_count = 0;
    h->address_count = 0;
}

void aoe_host_release_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops) {
    aoe_host_clear_ror(h);
    if (h->started) ops->cleanup(ops->context);
}

void aoe_host_clear_pair_ror(char *p) {
    p[0] = '\0';
    p[0x20] = '\0';
}

const char *aoe_host_name_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops) {
    ops->gethostname(ops->context, h->name, (int)sizeof h->name);
    return h->name;
}

int32_t aoe_host_aliases_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops, char *out) {
    const char *const *aliases = NULL;
    const uint8_t(*const *addresses)[4] = NULL;
    if (!ops->lookup(ops->context, h->name, &aliases, &addresses)) return 0;
    if (out) out[0] = '\0';
    h->alias_count = 0;
    while (aliases && aliases[h->alias_count] && h->alias_count < AOE_HOST_ENTRIES) {
        snprintf(h->entries[h->alias_count].alias, sizeof h->entries[0].alias, "%s",
                 aliases[h->alias_count]);
        ++h->alias_count;
    }
    return h->alias_count;
}

int32_t aoe_host_addresses_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops, char *out,
                               int out_size) {
    const char *const *aliases = NULL;
    const uint8_t(*const *addresses)[4] = NULL;
    if (!ops->lookup(ops->context, h->name, &aliases, &addresses)) return 0;
    if (out) out[0] = '\0';
    h->address_count = 0;
    while (addresses && addresses[h->address_count] && h->address_count < AOE_HOST_ENTRIES) {
        const uint8_t *a = *addresses[h->address_count];
        char text[0x20];
        snprintf(text, sizeof text, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
        if (out) {
            size_t used = strlen(out);
            snprintf(out + used, (size_t)out_size - used, "%s", text);
        }
        snprintf(h->entries[h->address_count].address, sizeof h->entries[0].address, "%s", text);
        ++h->address_count;
    }
    return h->address_count;
}

const char *aoe_host_alias_ror(const AoeHostInfoRor *h, uint32_t i) {
    if ((uint32_t)h->alias_count < i || i >= AOE_HOST_ENTRIES) return "";
    return h->entries[i].alias;
}

void aoe_poster_init_ror(AoeWindowPosterRor *p, uint32_t window) {
    p->window = window;
    p->field_4 = 0;
}

void aoe_poster_post_ror(const AoeWindowPosterRor *p,
                         void (*post)(void *context, uint32_t window, uint32_t msg, uint32_t w,
                                      uint32_t l),
                         void *context, uint32_t w, uint32_t l) {
    post(context, p->window, 0x400, w, l);
}

void aoe_debug_log_printf_ror(AoeDebugLogRor *l, const char *format, ...) {
    (void)l;
    (void)format;
}

void aoe_debug_log_timestamps_ror(AoeDebugLogRor *l, int on) {
    l->timestamps = on ? 1 : 0;
    aoe_debug_log_printf_ror(l, "Date & Time stamp%s",
                             l->timestamps ? " is now ACTIVE." : " has been deactivated.");
}

void aoe_debug_log_flush_ror(AoeDebugLogRor *l, int on) {
    l->flush = on ? 1 : 0;
    aoe_debug_log_printf_ror(l, "Flush queue each message %s",
                             l->flush ? " is now ACTIVE." : " has been deactivated.");
}

void aoe_debug_log_sequence_ror(AoeDebugLogRor *l, int on) {
    if (on) {
        l->sequence = 1;
        l->counter = 0;
    } else {
        l->sequence = 0;
    }
    aoe_debug_log_printf_ror(l, "Sequence numbering%s",
                             l->sequence ? " is now ACTIVE." : " has been deactivated.");
}

void aoe_debug_log_stamp_ror(AoeDebugLogRor *l, const AoeDebugLogOpsRor *ops) {
    float elapsed = (float)(ops->now_ms(ops->context) - l->start_ms) * 0.001f;
    aoe_debug_log_printf_ror(l, ">>>TIME>>>  %s   [%7.1f sec. elapsed]", "?", (double)elapsed);
}

int aoe_debug_log_open_ror(AoeDebugLogRor *l, const AoeDebugLogOpsRor *ops) {
    int k;
    if (!l->enabled || (l->open && !l->failed)) return 0;
    l->start_ms = ops->now_ms(ops->context);
    for (k = 1;; ++k) {
        if (k == 1)
            snprintf(l->path, sizeof l->path, "C:\\AOELOG.txt");
        else
            snprintf(l->path, sizeof l->path, "C:\\AOELOG%.2d.txt", k - 1);
        l->file = ops->open(ops->context, l->path);
        if (l->file) {
            l->open = 1;
            aoe_debug_log_printf_ror(l, "Log file %s is opened", l->path);
            aoe_debug_log_stamp_ror(l, ops);
            return 1;
        }
        if (k + 1 > 0x19) {
            l->failed = 1;
            l->open = 0;
            return 0;
        }
    }
}

void aoe_debug_log_close_ror(AoeDebugLogRor *l, const AoeDebugLogOpsRor *ops) {
    if (!l->enabled) return;
    aoe_debug_log_printf_ror(l, "Closing debug log file '%s'.", l->path);
    aoe_debug_log_stamp_ror(l, ops);
    aoe_debug_log_printf_ror(l, "Log file is closed");
    ops->close(ops->context, l->file);
    l->file = NULL;
    l->open = 0;
}

int aoe_host_init_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops) {
    char scratch[0xff];
    int32_t i;
    if (!h) return 0;
    aoe_host_clear_ror(h);
    for (i = 0; i < AOE_HOST_ENTRIES; ++i) aoe_host_clear_pair_ror(h->entries[i].address);
    if (!ops || !ops->startup || ops->startup(ops->context) != 0) {
        h->started = 0;
        return 0;
    }
    h->started = 1;
    if (ops->gethostname) aoe_host_name_ror(h, ops);
    scratch[0] = 0;
    if (ops->lookup) {
        aoe_host_addresses_ror(h, ops, scratch, sizeof scratch);
        aoe_host_aliases_ror(h, ops, scratch);
    }
    return 1;
}
