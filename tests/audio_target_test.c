#include "audio_target.h"
#include "timer_actions.h"

#include <assert.h>
#include <string.h>

typedef struct {
    char events[16];
    unsigned used;
    void *voice;
    void *buffer_a;
    void *buffer_b;
} Trace;

static void mark(Trace *trace, char event) {
    assert(trace->used + 1 < sizeof(trace->events));
    trace->events[trace->used++] = event;
    trace->events[trace->used] = '\0';
}

static void stop_voice(void *user, void *voice) {
    Trace *trace = user;
    assert(voice == trace->voice);
    mark(trace, 'S');
}

static void release_voice(void *user, void *voice) {
    Trace *trace = user;
    assert(voice == trace->voice);
    mark(trace, 'V');
}

static void free_buffer(void *user, void *buffer) {
    Trace *trace = user;
    if (buffer == trace->buffer_a) mark(trace, 'A');
    else {
        assert(buffer == trace->buffer_b);
        mark(trace, 'B');
    }
}

int main(void) {
    int voice = 1, a = 2, b = 3;
    AoeAudioTarget chain[3] = {{0}};
    Trace trace = {0};
    AoeAudioTargetOps audio_ops = {&trace, stop_voice, release_voice, free_buffer};
    AoeTimedEvent record = {&chain[0], 1, 0};
    AoeTimedEventTimer timer = {1, &record, 1, 100};
    AoeTimedEventOps event_ops = {&audio_ops, aoe_audio_target_expire_callback};
    chain[0].next = &chain[1];
    chain[1].next = &chain[2];
    chain[0].active = 1;
    chain[2].name = "sound";
    chain[2].active = 1;
    chain[2].pending = 1;
    chain[2].voice = &voice;
    chain[2].buffer_a = &a;
    chain[2].buffer_b = &b;
    trace.voice = &voice;
    trace.buffer_a = &a;
    trace.buffer_b = &b;

    assert(aoe_timed_event_timer_step(&timer, 101, &event_ops));
    assert(strcmp(trace.events, "SVAB") == 0);
    assert(record.status == 2);
    assert(chain[2].active == 0 && chain[2].pending == 0);
    assert(!chain[2].voice && !chain[2].buffer_a && !chain[2].buffer_b);
    assert(chain[0].active == 1); /* Only the final chain node is expired. */

    trace.used = 0;
    trace.events[0] = '\0';
    chain[2].pending = 1;
    chain[2].buffer_a = &a;
    assert(aoe_audio_target_expire(&chain[0], &audio_ops));
    assert(trace.used == 0);
    assert(chain[2].pending == 0 && chain[2].buffer_a == &a);

    trace.used = 0;
    trace.events[0] = '\0';
    chain[2].name = "";
    chain[2].active = 1;
    chain[2].voice = &voice;
    assert(aoe_audio_target_stop(&chain[0], &audio_ops));
    assert(trace.used == 0); /* Empty name suppresses voice stop. */
    chain[2].name = "sound";
    assert(aoe_audio_target_stop(&chain[0], &audio_ops));
    assert(strcmp(trace.events, "S") == 0);

    chain[2].next = &chain[1];
    chain[2].pending = 1;
    assert(!aoe_audio_target_expire(&chain[0], &audio_ops));
    assert(chain[2].pending == 1 && strcmp(trace.events, "S") == 0);
    return 0;
}
