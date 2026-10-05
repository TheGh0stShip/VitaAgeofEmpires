#ifndef VITA_AOE_AI_MODULE_H
#define VITA_AOE_AI_MODULE_H

#include <stdarg.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a computer-player AI module base (vtable
   0x00542c04) and its named-ID header (vtable 0x00542c00 at +4). Every AI
   module (Build AI 0x3e9, ConAI 0x3ea, Diplomacy AI 0x3eb, Emotional AI
   0x3ec, InfAI 0x3ed, Main Decision AI 1000) starts with this. Semantic
   storage, not the x86 layout. */

/* The shared history log: the original keeps one file handle (global
   0x00561454, "c:\aoeai.txt" opened "wa") and a player filter (global
   0x00554740, -1 for every player). The host supplies the file. */
typedef struct {
    void *context;
    int (*open)(void *context);                    /* nonzero on success */
    void (*close)(void *context);
    void (*write)(void *context, const char *line); /* one full line */
    const char *(*time_text)(void *context);       /* ctime-style text */
    int open_now;                                  /* the handle is set */
    int32_t player_filter;
} AoeAiLogRor;

typedef struct {
    int32_t id;          /* +0x08 (named-ID +4) */
    char name[64];       /* +0x0c (named-ID +8), 63 characters */
    int32_t player;      /* +0x4c */
    char player_name[64];/* +0x50, "Unknown" by default */
    int32_t field_90, field_94, field_98; /* +0x90..+0x98; 0x0040c1a0 */
    int32_t logging;     /* +0x9c */
    int32_t field_a0;    /* +0xa0 */
    int32_t field_e4;    /* +0xe4 = 5 */
    int32_t field_e8;    /* +0xe8 = 50 */
    int32_t field_ec;    /* +0xec = 0 */
    AoeAiLogRor *log;
} AoeAiModuleRor;

/* 0x0040bfd0: the named-ID header (a NULL name gives ""); 0x0040c050 /
   0x0040c030 are its destructor and deleting destructor (no state). */
void aoe_ai_named_init_ror(AoeAiModuleRor *module, const char *name,
                           int32_t id);
/* 0x0040c080: name and ID, the player, the defaults above.
   (A fourth argument is accepted and ignored by the original.) */
void aoe_ai_module_init_ror(AoeAiModuleRor *module, const char *name,
                            int32_t id, int32_t player, AoeAiLogRor *log);
/* 0x0040c110 (deleting form 0x0040c060): a module with logging on
   closes the shared log. */
void aoe_ai_module_release_ror(AoeAiModuleRor *module);
/* 0x0040c150: player and player name (NULL gives "Unknown"). */
void aoe_ai_module_set_player_ror(AoeAiModuleRor *module, int32_t player,
                                  const char *name);
/* 0x0040c1a0. */
int32_t aoe_ai_module_field_98_ror(const AoeAiModuleRor *module);
/* 0x0040c1b0: turning logging on opens the shared log when closed and
   writes "Common AI History File Started %s" (the time) and an empty
   line; turning it off closes an open log. */
void aoe_ai_module_set_logging_ror(AoeAiModuleRor *module, int32_t on);
/* 0x0040c240: when the filter is -1 or this player and the log is open,
   writes "P#%d %s: %s\n" (player, module name, text), or "P#%d\n" for
   empty text. The text is formatted into 1024 bytes. */
void aoe_ai_log_ror(const AoeAiModuleRor *module, const char *format, ...);
void aoe_ai_vlog_ror(const AoeAiModuleRor *module, const char *format,
                     va_list args);

#endif
