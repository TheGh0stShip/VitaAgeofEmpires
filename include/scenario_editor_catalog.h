#ifndef VITA_AOE_SCENARIO_EDITOR_CATALOG_H
#define VITA_AOE_SCENARIO_EDITOR_CATALOG_H

#include "player_templates.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t object_id;
    DatUnit *unit;
} AoeScenarioEditorUnitEntry;

/* Portable call boundary for the patched RoR editor map-click routine at
   0x0051a2e0. The original first lets template virtual +0x24 adjust the map
   coordinates, rejects the click when template virtual +0x20 returns true,
   and creates through player virtual +0x94. Submode 1 instead relocates the
   already-selected live object stored at editor view +0x174 through its
   common live-unit virtual +0x34. */
typedef struct {
    int (*adjust_position)(void *context, const DatUnit *unit,
                           float *x, float *y, int placement_variant);
    int (*placement_blocked)(void *context, const DatUnit *unit,
                             float x, float y, uint8_t placement_class);
    void *(*create)(void *context, uint16_t object_id,
                    float x, float y, float z, int editor_create);
    int (*relocate_selected)(void *context, void *selected_unit,
                             float x, float y, float z);
    void *selected_unit;
    /* Patched RoR queries the minimum graphic layer through created-unit
       +0x18, falling back to +0x10. Zero selects redraw/status 2; any
       nonzero layer selects redraw/status 1. */
    int (*created_nonzero_layer)(void *context, void *created_unit);
    void (*set_redraw_status)(void *context, int status);
    void *context;
} AoeScenarioEditorPlacementOps;

/* Common template virtual +0x24, release RoR 0x00443e90. It clamps the
   template footprint to [0,width/height-0.00001], then optionally applies
   the original nearest-edge whole-tile snap. Type 80's 0x004eed30 wrapper
   calls this same routine with snap_to_grid forced to one. The executable
   narrows truncations to signed 16-bit values, so this API rejects maps
   outside that numeric domain. */
int aoe_scenario_editor_adjust_position_ror(
    const DatUnit *unit, uint32_t map_width, uint32_t map_height,
    int snap_to_grid, float *x, float *y);

/* Patched RoR 1.0a 0x0046ab00 and 0x00492c60 build the editor unit list by
   walking the selected player's template slots and retaining every non-null
   template whose byte +0x4e is zero. The UI separately prepends its "None"
   entry; these APIs expose only actual units in the same slot order. */
size_t aoe_scenario_editor_unit_count(const AoePlayerTemplates *templates);
int aoe_scenario_editor_unit_at(const AoePlayerTemplates *templates,
                                size_t visible_index,
                                AoeScenarioEditorUnitEntry *entry);
int aoe_scenario_editor_unit_select(const AoePlayerTemplates *templates,
                                    uint16_t object_id,
                                    AoeScenarioEditorUnitEntry *entry);

/* Exact control-flow composition of patched RoR 1.0a 0x0051a2e0 after its
   screen-to-map hit test has returned terrain (code '3'). Returns 0 for a
   missing selection, rejected coordinates, blocked placement, or a
   failed factory; returns 1 after a unit is created and redraw status set.
   Mode 8/submode 1 relocates `selected_unit`; all other mode-8 placements
   use the selected player's factory. */
int aoe_scenario_editor_place_ror(
    const AoePlayerTemplates *templates, uint16_t object_id,
    int editor_mode, int editor_submode, float map_x, float map_y,
    const AoeScenarioEditorPlacementOps *ops, void **out_unit);

#endif
