#ifndef VITA_AOE_AUDIO_TARGET_H
#define VITA_AOE_AUDIO_TARGET_H

#include <stdint.h>

/* Semantic fields of the original sound target: name +0, next +4, flags +8/+9,
   buffers +0x2c/+0x30, voice +0x34. Not the PC's packed 32-bit ABI. */
typedef struct AoeAudioTarget {
    const char *name;
    struct AoeAudioTarget *next;
    uint8_t active;
    uint8_t pending;
    void *buffer_a;
    void *buffer_b;
    void *voice;
} AoeAudioTarget;

typedef struct {
    void *user;
    /* Voice vtable +0x48 (stop) and +0x08 (release). */
    void (*stop_voice)(void *user, void *voice);
    /* Voice vtable +0x08, followed by the original allocator's free. */
    void (*release_voice)(void *user, void *voice);
    void (*free_buffer)(void *user, void *buffer);
} AoeAudioTargetOps;

/* RoR 0x004a4b70 / base 0x0044d510: stop terminal voice only if name is
   nonempty, active flag is set, and voice exists. */
int aoe_audio_target_stop(AoeAudioTarget *head,
                          const AoeAudioTargetOps *ops);

/* RoR 0x004a49e0 / base 0x0044d370. Returns zero without mutation for
   missing hooks or a cyclic chain; original binaries had no cycle guard. */
int aoe_audio_target_expire(AoeAudioTarget *head,
                            const AoeAudioTargetOps *ops);

/* AoeTimedEventOps.expire_target-compatible binding. */
int aoe_audio_target_expire_callback(void *user, void *target);

#endif
