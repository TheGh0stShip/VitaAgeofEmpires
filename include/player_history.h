#ifndef VITA_AOE_PLAYER_HISTORY_H
#define VITA_AOE_PLAYER_HISTORY_H

#include <stdint.h>

#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a player history (0x004b09f0..0x004b0cf0; the
   RoR player's timeline for the achievements graph): a growing log of
   one byte per sample (grown by 60) and a list of events, each a kind
   byte, the sample count when it happened and the game time. */

typedef struct AoeHistoryEventRor {
    uint8_t kind;                    /* +0x00 */
    int32_t sample;                  /* +0x04 */
    int32_t time;                    /* +0x08 */
    struct AoeHistoryEventRor *next; /* +0x0c */
} AoeHistoryEventRor;

typedef struct {
    uint8_t *bytes;                  /* +0x00 */
    AoeHistoryEventRor *events;      /* +0x04: newest first */
    int32_t field_8;                 /* +0x08 */
    int32_t count;                   /* +0x0c */
    int32_t capacity;                /* +0x10 */
} AoePlayerHistoryRor;

/* 0x004b09f0: empty, then loaded (0x004b0af0) when `stream` is given;
   0x004b0a20: freed. */
int aoe_history_init_ror(AoePlayerHistoryRor *h, AoeOutcomeStream *stream, float version);
void aoe_history_release_ror(AoePlayerHistoryRor *h);
/* 0x004b0af0 load / 0x004b0a60 save: count, +0x8, capacity, the
   capacity's bytes, the event count and the events (16 bytes each; up to
   7.2 they were 12 bytes, kind and a minute count whose time was
   minutes x 15000). The load keeps the saved order. */
int aoe_history_load_ror(AoePlayerHistoryRor *h, AoeOutcomeStream *s, float version);
void aoe_history_save_ror(const AoePlayerHistoryRor *h, AoeOutcomeStream *s);
/* 0x004b0c00: a sample (a skipped one, `skip`, only counts); 0x004b0c70
   sample i (0 when skipped or out of range). */
void aoe_history_add_ror(AoePlayerHistoryRor *h, int skip, uint8_t v);
uint8_t aoe_history_get_ror(const AoePlayerHistoryRor *h, int skip, int32_t i);
/* 0x004b0ca0: an event of `kind` at the current sample and `time` (only
   once there are samples); 0x004b0cf0: the kind of the event at sample
   `sample` (the newest first; 0 when none). */
void aoe_history_event_ror(AoePlayerHistoryRor *h, uint8_t kind, int32_t time);
int aoe_history_find_ror(const AoePlayerHistoryRor *h, int32_t sample, uint8_t *kind);

#endif
