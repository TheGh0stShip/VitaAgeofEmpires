#include "unit_debug_log.h"

#include <stdio.h>
#include <string.h>

size_t aoe_unit_debug_log_format_ror(char *out, size_t cap, int32_t filter_id,
                                     int32_t unit_id, int opening,
                                     int32_t world_time, const char *message) {
    char text[1024];
    size_t len = 0;
    int n;
    if (!out || !cap) return 0;
    out[0] = 0;
    if (filter_id != -1 && filter_id != unit_id) return 0;
    if (opening) {
        n = snprintf(out, cap, "WORLD TIME: %ld\n", (long)world_time);
        if (n < 0) return 0;
        len = (size_t)n < cap ? (size_t)n : cap - 1;
    }
    snprintf(text, sizeof(text), "%s", message ? message : "");
    n = text[0] ? snprintf(out + len, cap - len, "UNIT #%5d: %s\n", (int)unit_id, text)
                : snprintf(out + len, cap - len, "UNIT #%5d\n", (int)unit_id);
    if (n < 0) return len;
    return len + ((size_t)n < cap - len ? (size_t)n : cap - len - 1);
}
