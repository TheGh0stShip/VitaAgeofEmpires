#include <assert.h>
#include <stddef.h>

#include "player_watch.h"

static int objs[4];
static int lost_count, lost_player[8];
static const void *lookup(void *c, int32_t id) {
    (void)c;
    return id >= 0 && id < 3 ? &objs[id] : NULL; /* object 3 is gone */
}
static void lost(void *c, int32_t p, const void *o) {
    (void)c; (void)o;
    lost_player[lost_count++] = p;
}

int main(void) {
    AoePlayerWatchRor w;
    AoePlayerWatchOpsRor ops = {NULL, lookup, lost};
    uint32_t m0 = 0x0e, m1 = 0, m3 = 2;
    assert(aoe_player_watch_init_ror(&w, 1, 2) && w.capacity == 2 && w.count == 0);
    /* Another player's object is refused; a duplicate too. */
    assert(!aoe_player_watch_add_ror(&w, &objs[0], 2, 0, &m0));
    assert(aoe_player_watch_add_ror(&w, &objs[0], 1, 0, &m0));
    assert(!aoe_player_watch_add_ror(&w, &objs[0], 1, 0, &m0));
    assert(aoe_player_watch_add_ror(&w, &objs[1], 1, 1, &m1));
    /* Full: grows by 0x20. */
    assert(aoe_player_watch_add_ror(&w, &objs[3], 1, 3, &m3) && w.capacity == 0x22 && w.count == 3);
    /* Object 0 loses players 1 (itself, skipped), 2 and 3; object 3 is gone. */
    m0 = 0;
    m3 = 0;
    aoe_player_watch_update_ror(&w, &ops);
    assert(lost_count == 2 && lost_player[0] == 2 && lost_player[1] == 3);
    assert(w.count == 2 && w.last[0] == 0);
    assert(aoe_player_watch_remove_ror(&w, &objs[0]) && w.count == 1 && w.objects[0] == &objs[1]);
    assert(!aoe_player_watch_remove_ror(&w, &objs[0]));
    aoe_player_watch_release_ror(&w);
    return 0;
}
