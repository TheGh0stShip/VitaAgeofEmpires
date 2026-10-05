#ifndef VITA_AOE_CHAT_LOG_H
#define VITA_AOE_CHAT_LOG_H

#include <stdint.h>

#include "wave_sound.h"

/* Patched Rise of Rome 1.0a taunt player (0x004e80f0..0x004e8310, 0x198
   bytes): four wave slots playing "<prefix>Taunt<nnn>.wav" for chat
   lines that are taunt numbers. Semantic storage; offsets are noted. */
typedef struct {
    AoeWaveRor *waves[4];  /* +0x000 */
    char prefix[0x81];     /* +0x113 */
    int32_t muted;         /* +0x194 */
} AoeTauntPlayerRor;

/* 0x004e80f0: an empty prefix, unmuted, four "Silence" waves on `system`
   (0x004e8310, which also remakes them). 0x004e82e0 (and its thunk
   0x004e8120) frees the waves. */
void aoe_taunt_init_ror(AoeTauntPlayerRor *t, AoeSoundSystemRor *system);
int aoe_taunt_make_waves_ror(AoeTauntPlayerRor *t, AoeSoundSystemRor *system);
void aoe_taunt_free_waves_ror(AoeTauntPlayerRor *t);
void aoe_taunt_free_waves_thunk_ror(AoeTauntPlayerRor *t);
/* 0x004e82b0: the taunt number of a line ("!" allowed first; 0 for none
   or above 999). 0x004e81d0: play taunt `n` (not when muted): idle slots
   are unloaded and the first idle one plays it (0 when all are busy or
   it fails to load). 0x004e8130: a chat line; "!MUTE" / "!NOMUTE" switch
   muting when `commands` is set; returns the number played (0 when
   muted). */
int32_t aoe_taunt_number_ror(const char *text);
int aoe_taunt_play_ror(AoeTauntPlayerRor *t, int32_t n);
int32_t aoe_taunt_line_ror(AoeTauntPlayerRor *t, const char *text,
                           int commands);

/* The chat log (0x0041c830..0x0041caa0): the last 51 lines as "sender:
   text" in a ring, ten per-player recipient flags and the taunt player.
   `notify` stands for the window message the original posts for each
   new line (its index). */
typedef struct {
    void *window;              /* +0x000 */
    int32_t recipients[10];    /* +0x004 */
    char *lines[51];           /* +0x02c */
    int32_t last;              /* +0x0f8 */
    AoeTauntPlayerRor *taunts; /* +0x0fc */
    void (*notify)(void *window, int32_t index);
} AoeChatLogRor;

/* 0x0041c830: empty, with a taunt player on `system`; 0x0041c8d0 frees
   it. 0x0041c900: add a line (both parts needed; 0 otherwise), notify,
   and pass the text to the taunt player. 0x0041c9f0 line `i` (NULL
   outside 0..50); 0x0041ca10 drop every line. 0x0041ca50 set recipient
   flag `i` (1..9); 0x0041ca70 read one (0..9). 0x0041ca90 frees the
   taunt waves and 0x0041caa0 remakes them on another system. */
void aoe_chat_log_init_ror(AoeChatLogRor *c, void *window,
                           AoeSoundSystemRor *system);
void aoe_chat_log_free_ror(AoeChatLogRor *c);
int aoe_chat_log_add_ror(AoeChatLogRor *c, const char *sender,
                         const char *text, int commands);
const char *aoe_chat_log_line_ror(const AoeChatLogRor *c, int32_t i);
void aoe_chat_log_clear_ror(AoeChatLogRor *c);
void aoe_chat_log_set_recipient_ror(AoeChatLogRor *c, int32_t i,
                                    int32_t value);
int32_t aoe_chat_log_recipient_ror(const AoeChatLogRor *c, int32_t i);
void aoe_chat_log_free_waves_ror(AoeChatLogRor *c);
int aoe_chat_log_make_waves_ror(AoeChatLogRor *c, AoeSoundSystemRor *system);

#endif
