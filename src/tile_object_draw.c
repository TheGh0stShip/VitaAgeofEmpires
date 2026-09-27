#include "tile_object_draw.h"

typedef struct {
    void *renderer;
    int32_t x, y;
    AoeTileObjectDrawVisitAt visit;
    void *context;
} VisitAtContext;

static void visit_at_adapter(void *object, const void *graphic, void *opaque) {
    VisitAtContext *at = opaque;
    at->visit(object, graphic, at->renderer, at->x, at->y, at->context);
}

static size_t visit_draw(const AoeTileObjectDrawEntry *entries,
                         size_t count, int alternate_pass,
                         AoeTileObjectDrawVisit visit,
                         void *context, int expansion) {
    size_t drawn = 0;
    int has_marker_25 = 0;
    if ((!entries && count) || !visit) return 0;
    if (expansion && alternate_pass && count > 1) {
        for (size_t i = 0; i < count; ++i)
            if (entries[i].object && entries[i].marker_25) {
                has_marker_25 = 1;
                break;
            }
    }
    for (size_t i = 0; i < count; ++i) {
        const AoeTileObjectDrawEntry *entry = &entries[i];
        const void *graphic = entry->normal_graphic;
        int draw = 1;
        if (!entry->object) continue;
        switch (entry->draw_class) {
        case 0:
            draw = !alternate_pass;
            break;
        case 2:
            if (expansion)
                draw = !alternate_pass || entry->alternate_rank <= 2;
            break;
        case 3:
            draw = alternate_pass && entry->alternate_eligible;
            graphic = entry->alternate_graphic;
            break;
        case 4:
            if (expansion)
                draw = !alternate_pass || count <= 1 || !has_marker_25;
            break;
        default:
            break;
        }
        if (draw) {
            visit(entry->object, graphic, context);
            ++drawn;
        }
    }
    return drawn;
}

size_t aoe_tile_object_visit_draw(const AoeTileObjectDrawEntry *entries,
                                  size_t count, int alternate_pass,
                                  AoeTileObjectDrawVisit visit,
                                  void *context) {
    return visit_draw(entries, count, alternate_pass, visit, context, 1);
}

size_t aoe_tile_object_visit_draw_at(
    const AoeTileObjectDrawEntry *entries, size_t count, int alternate_pass,
    void *renderer, int32_t screen_x, int32_t screen_y,
    AoeTileObjectDrawVisitAt visit, void *context) {
    VisitAtContext at = {renderer, screen_x, screen_y, visit, context};
    if (!visit) return 0;
    return visit_draw(entries, count, alternate_pass, visit_at_adapter,
                      &at, 1);
}

size_t aoe_tile_object_visit_draw_base(
    const AoeTileObjectDrawEntry *entries, size_t count,
    int alternate_pass, AoeTileObjectDrawVisit visit, void *context) {
    return visit_draw(entries, count, alternate_pass, visit, context, 0);
}
