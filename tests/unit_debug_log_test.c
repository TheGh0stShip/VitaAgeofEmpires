#include "unit_debug_log.h"

#include <assert.h>
#include <string.h>

/* Patched RoR 1.0a 0x004ab900 aoeunit.txt lines. */
int main(void) {
    char out[64];
    assert(aoe_unit_debug_log_format_ror(out, sizeof out, -1, 42, 1, 1234,
                                         "idle") == strlen(out));
    assert(!strcmp(out, "WORLD TIME: 1234\nUNIT #   42: idle\n"));
    assert(aoe_unit_debug_log_format_ror(out, sizeof out, -1, 7, 0, 0, "") &&
           !strcmp(out, "UNIT #    7\n"));
    assert(aoe_unit_debug_log_format_ror(out, sizeof out, 9, 7, 0, 0, "x") == 0 &&
           out[0] == 0);
    assert(aoe_unit_debug_log_format_ror(out, sizeof out, 7, 7, 0, 0, "x") &&
           !strcmp(out, "UNIT #    7: x\n"));
    /* Truncation keeps the buffer terminated. */
    assert(aoe_unit_debug_log_format_ror(out, 8, -1, 7, 0, 0, "x") == 7 &&
           !strcmp(out, "UNIT # "));
    return 0;
}
