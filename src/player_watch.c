#include "player_watch.h"

#include <stdlib.h>
#include <string.h>

int aoe_player_watch_init_ror(AoePlayerWatchRor *w, int16_t player, int32_t capacity) {
    if (!w) return 0;
    memset(w, 0, sizeof *w);
    w->player = player;
    w->capacity = capacity;
    w->objects = calloc((size_t)capacity, sizeof *w->objects);
    w->masks = calloc((size_t)capacity, sizeof *w->masks);
    w->last = calloc((size_t)capacity, sizeof *w->last);
    w->ids = calloc((size_t)capacity, sizeof *w->ids);
    return capacity <= 0 || (w->objects && w->masks && w->last && w->ids);
}

void aoe_player_watch_release_ror(AoePlayerWatchRor *w) {
    if (!w) return;
    free(w->objects);
    free(w->masks);
    free(w->last);
    free(w->ids);
    w->objects = NULL;
    w->masks = NULL;
    w->last = NULL;
    w->ids = NULL;
}

int aoe_player_watch_grow_ror(AoePlayerWatchRor *w, int32_t extra) {
    const void **o;
    const uint32_t **m;
    uint32_t *l;
    int32_t *i;
    size_t n;
    if (!w) return 0;
    w->capacity += extra;
    n = (size_t)w->capacity;
    o = calloc(n, sizeof *o);
    m = calloc(n, sizeof *m);
    l = calloc(n, sizeof *l);
    i = calloc(n, sizeof *i);
    if (!o || !m || !l || !i) {
        free(o);
        free(m);
        free(l);
        free(i);
        w->capacity -= extra;
        return 0;
    }
    if (w->count > 0) {
        memcpy(o, w->objects, (size_t)w->count * sizeof *o);
        memcpy(m, w->masks, (size_t)w->count * sizeof *m);
        memcpy(l, w->last, (size_t)w->count * sizeof *l);
        memcpy(i, w->ids, (size_t)w->count * sizeof *i);
    }
    aoe_player_watch_release_ror(w);
    w->objects = o;
    w->masks = m;
    w->last = l;
    w->ids = i;
    return 1;
}

int aoe_player_watch_add_ror(AoePlayerWatchRor *w, const void *object, int16_t owner,
                             int32_t id, const uint32_t *mask) {
    int32_t k;
    if (!w || !mask) return 0;
    if (w->capacity <= w->count && !aoe_player_watch_grow_ror(w, 0x20)) return 0;
    if (owner != w->player) return 0;
    for (k = 0; k < w->count; ++k)
        if (w->objects[k] == object) return 0;
    w->objects[w->count] = object;
    w->masks[w->count] = mask;
    w->last[w->count] = *mask;
    w->ids[w->count] = id;
    ++w->count;
    return 1;
}

int aoe_player_watch_remove_ror(AoePlayerWatchRor *w, const void *object) {
    int32_t k;
    if (!w) return 0;
    for (k = 0; k < w->count; ++k) {
        if (w->objects[k] != object) continue;
        --w->count;
        if (k < w->count) {
            w->objects[k] = w->objects[w->count];
            w->masks[k] = w->masks[w->count];
            w->last[k] = w->last[w->count];
            w->ids[k] = w->ids[w->count];
        }
        w->objects[w->count] = NULL;
        w->masks[w->count] = NULL;
        w->last[w->count] = 0;
        w->ids[w->count] = 0;
        return 1;
    }
    return 0;
}

void aoe_player_watch_update_ror(AoePlayerWatchRor *w, const AoePlayerWatchOpsRor *ops) {
    int32_t k = 0;
    if (!w || !ops) return;
    while (k < w->count) {
        uint32_t cur = *w->masks[k], bits;
        int32_t p;
        const void *o = w->objects[k];
        if (cur == w->last[k]) {
            ++k;
            continue;
        }
        if (!o || !ops->lookup || ops->lookup(ops->context, w->ids[k]) != o) {
            aoe_player_watch_remove_ror(w, o);
            continue;
        }
        bits = (((w->last[k] ^ cur) & w->last[k]) >> 1) & 0x7fff;
        for (p = 1; bits; bits >>= 1, ++p)
            if ((bits & 1) && p != w->player && ops->lost) ops->lost(ops->context, p, o);
        w->last[k] = cur;
        ++k;
    }
}
