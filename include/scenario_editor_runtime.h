#ifndef VITA_AOE_SCENARIO_EDITOR_RUNTIME_H
#define VITA_AOE_SCENARIO_EDITOR_RUNTIME_H

#include "scenario_editor_catalog.h"
#include "scenario_runtime.h"
#include "unit_placement_check.h"

typedef struct {
    AoeScenarioRuntimeRor *runtime;
    const DatIntro *intro;
    uint8_t player_index;
    int last_placement_result;
    int32_t blocking_unit_id;
    AoeLiveUnit *moving_unit;
} AoeScenarioEditorRuntimeContext;

/* Concrete callbacks for AoeScenarioEditorPlacementOps. They bind patched
   RoR 0x0051a2e0 to the translated common template +0x24/+0x20 methods and
   the owned player factory. The placement check supplies the original
   scenario-editor mode-7 visibility bypass. The caller still supplies the
   original special submode factory and redraw-status method. */
int aoe_scenario_editor_runtime_adjust(
    void *context, const DatUnit *unit, float *x, float *y,
    int placement_variant);
int aoe_scenario_editor_runtime_blocked(
    void *context, const DatUnit *unit, float x, float y,
    uint8_t placement_class);
void *aoe_scenario_editor_runtime_create(
    void *context, uint16_t object_id, float x, float y, float z,
    int editor_create);
int aoe_scenario_editor_runtime_relocate(
    void *context, void *selected_unit, float x, float y, float z);

/* Direct live-unit state portion of patched editor selection/mouse-up around
   0x00518ea0 and 0x00519470. Begin stores an owned selected unit and writes
   original live byte +0x48 state 7. Finish restores state 2 and clears the
   selection, whether the move was placed or cancelled. */
int aoe_scenario_editor_runtime_begin_move(
    AoeScenarioEditorRuntimeContext *context, AoeLiveUnit *selected_unit);
int aoe_scenario_editor_runtime_finish_move(
    AoeScenarioEditorRuntimeContext *context);

/* Patched RoR 0x004a6030, 0x00415c70, and 0x004159d0 as reached by editor
   click 0x0051a2e0. A graphic without deltas returns its own layer. A
   graphic with deltas returns the minimum direct child-graphic layer from
   sentinel 20. The live-unit query returns the minimum over every sprite;
   an empty sprite list therefore also returns 20. */
uint8_t aoe_scenario_editor_graphic_layer_ror(const DatGraphic *graphic);
uint8_t aoe_scenario_editor_live_layer_ror(const AoeLiveUnit *unit);
int aoe_scenario_editor_runtime_nonzero_layer(
    void *context, void *created_unit);

#endif
