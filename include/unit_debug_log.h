#ifndef VITA_AOE_UNIT_DEBUG_LOG_H
#define VITA_AOE_UNIT_DEBUG_LOG_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a 0x004ab900: the unit trace log. The original
   appends to "c:\aoeunit.txt" (opened "w" on first use, global 0x007c046c)
   for units matching the filter (global 0x00554744, -1 = every unit; the
   shipped value is -1). On opening it first writes "WORLD TIME: %ld\n"
   (the world's +0x04 time). Each message is vsprintf'd into a 1024-byte
   buffer and written as "UNIT #%5d: %s\n", or "UNIT #%5d\n" when empty.

   Portable form: the caller owns the file; this formats the text that
   would be appended into `out` (always terminated when cap > 0) and
   returns its length, or 0 when the filter skips the unit. `opening`
   asks for the world-time header. The message is limited to 1023 bytes
   like the original buffer. */
size_t aoe_unit_debug_log_format_ror(char *out, size_t cap, int32_t filter_id,
                                     int32_t unit_id, int opening,
                                     int32_t world_time, const char *message);

#endif
