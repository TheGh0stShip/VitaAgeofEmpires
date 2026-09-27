#include "runtime_viewport.h"

#include <string.h>

int aoe_runtime_viewport_draw_tile_ror(void *opaque, uint32_t tile_x,
                                       uint32_t tile_y, int32_t anchor_x,
                                       int32_t anchor_y,
                                       uint8_t visibility_mode) {
    AoeRuntimeViewportRor *view = opaque;
    const AoeObjectUpdateList *list;
    AoeObjectUpdateNode *node;
    AoeLiveUnitViewportResult tile_result;
    size_t tile_index, count = 0;
    int alternate_pass;
    (void)visibility_mode;
    if (!view || !view->map || !view->map->objects ||
        !view->map->view_codes || !view->resolve_unit || !view->draw_ops ||
        tile_x >= view->map->width || tile_y >= view->map->height)
        return 0;
    tile_index = (size_t)tile_y * view->map->width + tile_x;
    list = &view->map->objects[tile_index];
    if (list->count > view->scratch_capacity ||
        (list->count && !view->scratch))
        return 0;
    for (node = list->head; node; node = node->next) {
        AoeTileObjectDrawEntry *entry;
        AoeLiveUnit *unit;
        if (count >= list->count) return 0;
        entry = &view->scratch[count++];
        memset(entry, 0, sizeof(*entry));
        if (!node->object) continue;
        /* AoeLiveUnitCore is intentionally the first AoeLiveUnit member;
           tile linkage retains that address just as the original does. */
        unit = node->object;
        if (!unit->constructed || !unit->core.unit_template) return 0;
        entry->object = node->object;
        entry->normal_graphic = unit->graphic.current_graphic;
        entry->draw_class = unit->core.unit_template->draw_class;
        entry->alternate_rank = unit->core.state;
        entry->marker_25 = unit->core.class_code == 0x19;
        if (view->resolve_entry &&
            !view->resolve_entry(view->resolve_entry_context, unit, entry))
            return 0;
    }
    if (count != list->count) return 0;
    ++view->tiles;
    if (!count) return 1;
    ++view->nonempty_tiles;
    alternate_pass = view->map->view_codes[tile_index] == 0x80;
    memset(&tile_result, 0, sizeof(tile_result));
    if (!aoe_live_unit_viewport_draw_ror(
            view->scratch, count, alternate_pass, view->renderer,
            anchor_x, anchor_y, view->resolve_unit,
            view->resolve_unit_context, view->draw_ops, &tile_result)) {
        view->result.visited += tile_result.visited;
        view->result.drawn += tile_result.drawn;
        view->result.failed += tile_result.failed;
        return 0;
    }
    view->result.visited += tile_result.visited;
    view->result.drawn += tile_result.drawn;
    view->result.failed += tile_result.failed;
    return 1;
}
