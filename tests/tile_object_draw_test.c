#include "tile_object_draw.h"

#include <assert.h>

typedef struct { int called; } AtTrace;

static void visit_at(void *object, const void *graphic, void *renderer,
                     int32_t x, int32_t y, void *context) {
    AtTrace *trace = context;
    assert(object == (void *)1 && graphic == (void *)101);
    assert(renderer == (void *)99 && x == -17 && y == 42);
    trace->called++;
}
#include <stdint.h>

typedef struct {
    uintptr_t objects[16];
    uintptr_t graphics[16];
    size_t count;
} Visits;

static void record(void *object, const void *graphic, void *context) {
    Visits *visits = context;
    assert(visits->count < 16);
    visits->objects[visits->count] = (uintptr_t)object;
    visits->graphics[visits->count] = (uintptr_t)graphic;
    ++visits->count;
}

int main(void) {
    AoeTileObjectDrawEntry entries[8] = {
        {(void *)1, (void *)101, 0, 0, 0, 0, 0},
        {(void *)2, (void *)102, 0, 2, 2, 0, 0},
        {(void *)3, (void *)103, 0, 2, 3, 0, 0},
        {(void *)4, (void *)104, (void *)204, 3, 0, 0, 1},
        {(void *)5, (void *)105, (void *)205, 3, 0, 0, 0},
        {(void *)6, (void *)106, 0, 4, 0, 0, 0},
        {(void *)7, (void *)107, 0, 5, 0, 0, 0},
        {0, (void *)108, 0, 5, 0, 0, 0}
    };
    Visits visits = {0};
    assert(aoe_tile_object_visit_draw(entries, 8, 0, record, &visits) == 5);
    assert(visits.count == 5);
    assert(visits.objects[0] == 1 && visits.graphics[0] == 101);
    assert(visits.objects[1] == 2 && visits.graphics[1] == 102);
    assert(visits.objects[2] == 3 && visits.graphics[2] == 103);
    assert(visits.objects[3] == 6 && visits.graphics[3] == 106);
    assert(visits.objects[4] == 7 && visits.graphics[4] == 107);

    visits.count = 0;
    assert(aoe_tile_object_visit_draw(entries, 8, 1, record, &visits) == 4);
    assert(visits.objects[0] == 2 && visits.objects[1] == 4 &&
           visits.objects[2] == 6 && visits.objects[3] == 7);
    assert(visits.graphics[1] == 204);

    visits.count = 0;
    assert(aoe_tile_object_visit_draw_base(entries, 8, 1,
                                           record, &visits) == 5);
    assert(visits.objects[0] == 2 && visits.objects[1] == 3 &&
           visits.objects[2] == 4 && visits.objects[3] == 6 &&
           visits.objects[4] == 7);
    assert(visits.graphics[2] == 204);

    entries[6].marker_25 = 1;
    visits.count = 0;
    assert(aoe_tile_object_visit_draw(entries, 8, 1, record, &visits) == 3);
    assert(visits.objects[0] == 2 && visits.objects[1] == 4 &&
           visits.objects[2] == 7);
    visits.count = 0;
    assert(aoe_tile_object_visit_draw_base(entries, 8, 1,
                                           record, &visits) == 5);
    entries[6].marker_25 = 0;

    visits.count = 0;
    assert(aoe_tile_object_visit_draw(&entries[5], 1, 1, record, &visits) == 1);
    assert(visits.objects[0] == 6);
    visits.count = 0;
    assert(aoe_tile_object_visit_draw(entries, 8, -128, record, &visits) == 4);
    assert(aoe_tile_object_visit_draw(NULL, 1, 0, record, &visits) == 0);
    assert(aoe_tile_object_visit_draw(entries, 8, 0, NULL, &visits) == 0);
    AtTrace at = {0};
    assert(aoe_tile_object_visit_draw_at(entries, 1, 0, (void *)99,
                                         -17, 42, visit_at, &at) == 1);
    assert(at.called == 1);
    return 0;
}
