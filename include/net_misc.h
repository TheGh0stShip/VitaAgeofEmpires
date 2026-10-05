#ifndef VITA_AOE_NET_MISC_H
#define VITA_AOE_NET_MISC_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a networking odds and ends: the local host's
   name and addresses (0x0041d390..0x0041d6a0, Winsock through ops), the
   window poster (0x0041d6a0 / 0x0041d6c0) and the debug log object
   (global 0x00580db8, 0x0042c000..0x0042c330; its printf 0x0042c330 is
   empty in this build). Semantic storage; offsets noted. */

#define AOE_HOST_ENTRIES 16

typedef struct {
    char name[0x80];                         /* +0x194 */
    int32_t started;                         /* +0x214: Winsock started */
    int32_t address_count;                   /* +0x218 */
    int32_t alias_count;                     /* +0x21c */
    struct {
        char address[0x20];                  /* +0x00 */
        char alias[0x80];                    /* +0x20 */
    } entries[AOE_HOST_ENTRIES];             /* +0x220, 0xa0 each */
} AoeHostInfoRor;

typedef struct {
    void *context;
    int (*gethostname)(void *context, char *out, int size);       /* Ordinal 57 */
    /* gethostbyname: its aliases and IPv4 addresses (NULL ends). */
    int (*lookup)(void *context, const char *name, const char *const **aliases,
                  const uint8_t (*const **addresses)[4]);
    void (*cleanup)(void *context);                                /* Ordinal 116 */
} AoeHostOpsRor;

/* 0x0041d600: the name and counts emptied; 0x0041d390 also ends Winsock
   when started; 0x0041d640 two strings (+0x00 and +0x20) emptied.
   0x0041d3e0: the host name (gethostname, 0x80 bytes). 0x0041d400: the
   aliases (each into an entry's +0x20); 0x0041d500: the addresses as
   "%d.%d.%d.%d", each also appended to `out`; both return their counts
   (0 when the lookup fails). 0x0041d4d0: alias i ("" beyond the
   count). */
void aoe_host_clear_ror(AoeHostInfoRor *h);
void aoe_host_release_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops);
void aoe_host_clear_pair_ror(char *p);
const char *aoe_host_name_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops);
int32_t aoe_host_aliases_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops, char *out);
int32_t aoe_host_addresses_ror(AoeHostInfoRor *h, const AoeHostOpsRor *ops, char *out,
                               int out_size);
const char *aoe_host_alias_ror(const AoeHostInfoRor *h, uint32_t i);

/* 0x0041d6a0: a poster for a window; 0x0041d6c0: WM_USER (w, l) posted to
   it. */
typedef struct {
    uint32_t window;
    int32_t field_4;
} AoeWindowPosterRor;
void aoe_poster_init_ror(AoeWindowPosterRor *p, uint32_t window);
void aoe_poster_post_ror(const AoeWindowPosterRor *p,
                         void (*post)(void *context, uint32_t window, uint32_t msg, uint32_t w,
                                      uint32_t l),
                         void *context, uint32_t w, uint32_t l);

/* The debug log. */
typedef struct {
    int32_t timestamps;     /* +0x04 */
    int32_t enabled;        /* +0x08 */
    int32_t sequence;       /* +0x10 */
    int32_t flush;          /* +0x14 */
    int32_t open;           /* +0x18 */
    int32_t failed;         /* +0x1c */
    uint32_t start_ms;      /* +0x20 */
    int32_t counter;        /* +0x24 */
    char path[0x104];       /* +0x730 */
    void *file;             /* 0x00581ab4 */
} AoeDebugLogRor;

typedef struct {
    void *context;
    void *(*open)(void *context, const char *path);  /* fopen "wt" */
    void (*close)(void *context, void *file);
    uint32_t (*now_ms)(void *context);
} AoeDebugLogOpsRor;

/* 0x0042c330: the log line (empty in this build). 0x0042c0c0 time
   stamps, 0x0042c180 flush each message, 0x0042c1c0 sequence numbers
   (the counter restarts), each announced. 0x0042c100: a time line
   (announced only). 0x0042c200: open "C:\AOELOG.txt", else
   "C:\AOELOG%.2d.txt" 1..24 (enabled, not open, not failed; giving up
   marks it failed), the start time kept. 0x0042c2e0 (0x0042c000) closes
   it. */
void aoe_debug_log_printf_ror(AoeDebugLogRor *l, const char *format, ...);
void aoe_debug_log_timestamps_ror(AoeDebugLogRor *l, int on);
void aoe_debug_log_flush_ror(AoeDebugLogRor *l, int on);
void aoe_debug_log_sequence_ror(AoeDebugLogRor *l, int on);
void aoe_debug_log_stamp_ror(AoeDebugLogRor *l, const AoeDebugLogOpsRor *ops);
int aoe_debug_log_open_ror(AoeDebugLogRor *l, const AoeDebugLogOpsRor *ops);
void aoe_debug_log_close_ror(AoeDebugLogRor *l, const AoeDebugLogOpsRor *ops);

#endif
