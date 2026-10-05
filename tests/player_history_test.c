#include "player_history.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "aoe_stream.h"

int main(void) {
    AoePlayerHistoryRor h, back;
    static uint8_t buf[1024];
    AoeOutcomeStream s = {buf, sizeof buf, 0, 0};
    uint8_t kind = 0;
    int i;

    aoe_history_init_ror(&h, NULL, 0.0f);
    aoe_history_event_ror(&h, 5, 100); /* no samples yet: ignored */
    assert(h.events == NULL);
    for (i = 0; i < 61; ++i) aoe_history_add_ror(&h, 0, (uint8_t)i);
    assert(h.count == 61 && h.capacity == 120 && aoe_history_get_ror(&h, 0, 60) == 60);
    aoe_history_add_ror(&h, 1, 99);
    assert(h.count == 62 && aoe_history_get_ror(&h, 0, 61) == 0 && aoe_history_get_ror(&h, 1, 3) == 0);
    assert(aoe_history_get_ror(&h, 0, 62) == 0 && aoe_history_get_ror(&h, 0, -1) == 0);
    aoe_history_event_ror(&h, 2, 9000);
    aoe_history_event_ror(&h, 3, 9500);
    assert(aoe_history_find_ror(&h, 62, &kind) && kind == 3);
    assert(!aoe_history_find_ror(&h, 5, &kind));

    aoe_history_save_ror(&h, &s);
    assert(s.pos == 12 + 120 + 4 + 2 * 16);
    s.size = s.pos;
    s.pos = 0;
    assert(aoe_history_init_ror(&back, &s, 7.24f) && s.pos == s.size);
    assert(back.count == 62 && back.capacity == 120 && back.bytes[10] == 10);
    assert(back.events->kind == 3 && back.events->time == 9500 && back.events->next->kind == 2);

    /* An old save: 12-byte events, the time from minutes x 15000. */
    {
        AoePlayerHistoryRor old;
        uint8_t ob[64];
        AoeOutcomeStream o = {ob, sizeof ob, 0, 0};
        aoe_stream_put_i32(&o, 1);
        aoe_stream_put_i32(&o, 0);
        aoe_stream_put_i32(&o, 1);
        aoe_stream_put_u8(&o, 7);
        aoe_stream_put_i32(&o, 1);
        aoe_stream_put_u8(&o, 4);
        aoe_stream_put(&o, "\0\0\0", 3);
        aoe_stream_put_i32(&o, 3);
        aoe_stream_put_i32(&o, 0);
        o.size = o.pos;
        o.pos = 0;
        assert(aoe_history_init_ror(&old, &o, 7.0f) && o.pos == o.size);
        assert(old.bytes[0] == 7 && old.events->kind == 4 && old.events->time == 45000);
        aoe_history_release_ror(&old);
    }
    aoe_history_release_ror(&h);
    aoe_history_release_ror(&back);
    puts("player_history_test: ok");
    return 0;
}
