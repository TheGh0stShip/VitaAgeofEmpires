#include "dat.h"
#include "unit_storage.h"

#include <assert.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
    put16(p, (uint16_t)value);
    put16(p + 2, (uint16_t)(value >> 16));
}

int main(void) {
    uint8_t bytes[380] = {0};
    DatBlob blob = {bytes, sizeof(bytes)};
    DatMapGeometry geometry = {0};
    DatRandomMaps maps;
    DatRandomMaps bad_maps;
    DatEffects effects;
    DatCivilizations civilizations;

    put32(bytes + 48, 1); /* one random map */
    put32(bytes + 56, 7); /* map ID, first pass */
    for (unsigned i = 0; i < 4; ++i)
        put32(bytes + 56 + 40 + i * 8, 1);
    put32(bytes + 164, 1); /* second-pass land/terrain/unit/elevation */
    put32(bytes + 216, 1);
    put32(bytes + 248, 1);
    put32(bytes + 300, 1);
    put32(bytes + 332, 1); /* one effect */
    memcpy(bytes + 336, "Test Effect", 11);
    put16(bytes + 367, 1);
    bytes[369] = 4; /* relative attribute modifier */
    put16(bytes + 370, 12); /* unit */
    put16(bytes + 372, 3); /* class */
    put16(bytes + 374, 9); /* attack */
    put32(bytes + 376, 0x3fc00000); /* 1.5f */

    assert(dat_parse_random_maps(&blob, &geometry, &maps));
    assert(maps.count == 1 && maps.maps[0].map_id == 7);
    assert(maps.maps[0].land_count == 1 && maps.maps[0].terrain_count == 1);
    assert(maps.maps[0].unit_count == 1 && maps.maps[0].elevation_count == 1);
    assert(maps.next_offset == 332);
    assert(dat_parse_effects(&blob, &maps, &effects));
    assert(effects.count == 1 && effects.next_offset == sizeof(bytes));
    assert(strcmp(effects.effects[0].name, "Test Effect") == 0);
    assert(effects.effects[0].command_count == 1);
    assert(effects.effects[0].commands[0].type == 4);
    assert(effects.effects[0].commands[0].target_unit == 12);
    assert(effects.effects[0].commands[0].unit_class == 3);
    assert(effects.effects[0].commands[0].attribute == 9);
    assert(effects.effects[0].commands[0].amount == 1.5f);
    dat_free_effects(&effects);
    blob.size--;
    assert(!dat_parse_effects(&blob, &maps, &effects));
    blob.size++;
    put32(bytes + 216, 2);
    assert(!dat_parse_random_maps(&blob, &geometry, &bad_maps));
    dat_free_effects(&effects);
    dat_free_random_maps(&maps);
    {
        uint8_t civ_bytes[180] = {0};
        DatBlob civ_blob = {civ_bytes, sizeof(civ_bytes)};
        DatEffects before_civs = {0};
        put16(civ_bytes, 1); /* civilization count */
        civ_bytes[2] = 1;
        memcpy(civ_bytes + 3, "Gaia", 5);
        put16(civ_bytes + 28, 1); /* one unit slot */
        put32(civ_bytes + 30, 1); /* present-unit pointer placeholder */
        civ_bytes[34] = 10; /* eye-candy unit, no moving/combat subtype */
        put16(civ_bytes + 34 + 9, 3); /* class */
        put16(civ_bytes + 34 + 11, 413); /* standing graphic */
        put16(civ_bytes + 34 + 13, 414); /* dying graphic */
        put16(civ_bytes + 34 + 15, 415); /* undead graphic */
        civ_bytes[34 + 17] = 1; /* template +0x24: undead mode */
        civ_bytes[103] = 1; /* template +0x68: raise to terrain height */
        put32(civ_bytes + 106, 0x3f000000); /* template +0x80: 0.5f */
        civ_bytes[132] = 1; /* template +0xa4: negative-ID allocation */
        civ_bytes[134] = 2; /* template +0xa6: state-effect mode */
        put16(civ_bytes + 150, 1); /* first storage: carried wood */
        put32(civ_bytes + 152, 0x40200000); /* 2.5f */
        civ_bytes[156] = 0;
        put16(civ_bytes + 157, 0); /* second storage: grant food */
        put32(civ_bytes + 159, 0x40400000); /* 3.0f */
        civ_bytes[163] = 1;
        put16(civ_bytes + 164, 2); /* third storage: grant stone */
        put32(civ_bytes + 166, 0x40800000); /* 4.0f */
        civ_bytes[170] = 2;
        put16(civ_bytes + 172, 50); /* selection sound */
        put16(civ_bytes + 174, 51); /* dying sound */
        assert(dat_parse_civilizations(&civ_blob, &before_civs,
                                       &civilizations));
        assert(civilizations.count == 1 && civilizations.next_offset == 180);
        assert(strcmp(civilizations.civs[0].name, "Gaia") == 0);
        assert(civilizations.civs[0].unit_count == 1);
        assert(civilizations.civs[0].units[0].present);
        assert(civilizations.civs[0].units[0].type == 10);
        assert(civilizations.civs[0].units[0].unit_class == 3);
        assert(civilizations.civs[0].units[0].standing_graphic == 413);
        assert(civilizations.civs[0].units[0].dying_graphic == 414);
        assert(civilizations.civs[0].units[0].undead_graphic == 415);
        assert(civilizations.civs[0].units[0].undead_mode == 1);
        assert(civilizations.civs[0].units[0].elevation_mode == 1);
        assert(civilizations.civs[0].units[0].decay_rate == 0.5f);
        assert(civilizations.civs[0].units[0].selection_sound == 50);
        assert(civilizations.civs[0].units[0].dying_sound == 51);
        assert(civilizations.civs[0].units[0].negative_id_mode == 1);
        assert(civilizations.civs[0].units[0].state_effect_mode == 2);
        assert(civilizations.civs[0].units[0].storage[0].resource_type == 1);
        assert(civilizations.civs[0].units[0].storage[0].amount == 2.5f);
        assert(civilizations.civs[0].units[0].storage[1].paid == 1);
        assert(civilizations.civs[0].units[0].storage[2].paid == 2);
        {
            float values[3] = {10.0f, 20.0f, 30.0f};
            AoePlayerResources resources = {.count = 3, .values = values};
            AoeUnitStorageState state = {0};
            assert(aoe_unit_storage_initialize(&civilizations.civs[0].units[0],
                                                &resources, AOE_PLAYER_BASE,
                                                &state));
            assert(state.carried_resource_type == 1 &&
                   state.carried_amount == 2.5f);
            assert(values[0] == 13.0f && values[1] == 20.0f &&
                   values[2] == 34.0f);
        }
        assert(dat_find_unit(&civilizations, 0, 0) ==
               &civilizations.civs[0].units[0]);
        assert(dat_find_unit(&civilizations, 1, 0) == NULL);
        assert(dat_find_unit(&civilizations, 0, 1) == NULL);
        dat_free_civilizations(&civilizations);
        civ_blob.size--;
        assert(!dat_parse_civilizations(&civ_blob, &before_civs,
                                        &civilizations));
    }
    {
        uint8_t civ_bytes[388] = {0};
        DatBlob civ_blob = {civ_bytes, sizeof(civ_bytes)};
        DatEffects before_civs = {0};
        const DatUnit *unit;
        put16(civ_bytes, 1);
        civ_bytes[2] = 1;
        memcpy(civ_bytes + 3, "Gaia", 5);
        put16(civ_bytes + 28, 1);
        put32(civ_bytes + 30, 1);
        civ_bytes[34] = 80; /* building unit */
        put16(civ_bytes + 34 + 9, 4); /* civilian class */
        put16(civ_bytes + 221, 1); /* one task */
        put16(civ_bytes + 223 + 5, 101); /* build action */
        put32(civ_bytes + 223 + 29, 0x40200000); /* 2.5 work range */
        civ_bytes[282] = 1; /* base armor */
        put16(civ_bytes + 283, 1); /* one attack */
        put16(civ_bytes + 285, 4); /* attack class */
        put16(civ_bytes + 287, 7); /* attack amount */
        put16(civ_bytes + 289, 1); /* one armor */
        put16(civ_bytes + 291, 3); /* armor class */
        put16(civ_bytes + 293, 2); /* armor amount */
        put32(civ_bytes + 297, 0x40200000); /* max range */
        put16(civ_bytes + 347, 0); /* food cost */
        put16(civ_bytes + 349, 50);
        put16(civ_bytes + 351, 1); /* paid */
        put16(civ_bytes + 365, 20); /* train time */
        put16(civ_bytes + 372, 600); /* construction graphic */
        civ_bytes[374] = 3; /* adjacent mode */
        put16(civ_bytes + 375, 7); /* completed angle */
        civ_bytes[377] = 1; /* disappears when built */
        put16(civ_bytes + 378, 42); /* stack unit */
        put16(civ_bytes + 380, 9); /* foundation terrain */
        put16(civ_bytes + 382, 10); /* old terrain */
        put16(civ_bytes + 384, 77); /* automatic research */
        put16(civ_bytes + 386, 88); /* completion sound */
        assert(dat_parse_civilizations(&civ_blob, &before_civs,
                                       &civilizations));
        assert(civilizations.next_offset == sizeof(civ_bytes));
        unit = dat_find_unit(&civilizations, 0, 0);
        assert(unit && unit->type == 80 && unit->unit_class == 4);
        assert(unit->task_count == 1 && unit->tasks[0].action_type == 101);
        assert(unit->tasks[0].work_range == 2.5f);
        assert(unit->attack_count == 1 && unit->attacks[0].class_id == 4);
        assert(unit->attacks[0].amount == 7);
        assert(unit->armor_count == 1 && unit->armors[0].amount == 2);
        assert(unit->base_armor == 1 && unit->max_range == 2.5f);
        assert(unit->costs[0].amount == 50 && unit->costs[0].paid == 1);
        assert(unit->train_time == 20);
        assert(unit->construction_graphic == 600);
        assert(unit->building_adjacent_mode == 3 && unit->building_angle == 7);
        assert(unit->disappears_when_built == 1 && unit->stack_unit_id == 42);
        assert(unit->foundation_terrain == 9 && unit->old_terrain == 10);
        assert(unit->building_research_id == 77 && unit->completion_sound == 88);
        dat_free_civilizations(&civilizations);
        civ_blob.size--;
        assert(!dat_parse_civilizations(&civ_blob, &before_civs,
                                        &civilizations));
    }
    {
        uint8_t tech_bytes[60] = {0};
        DatBlob tech_blob = {tech_bytes, sizeof(tech_bytes)};
        DatCivilizations before_techs = {0};
        DatTechs techs;
        put16(tech_bytes, 1);
        put16(tech_bytes + 2, 0); /* first prerequisite */
        for (unsigned i = 1; i < 4; ++i)
            put16(tech_bytes + 2 + i * 2, UINT16_MAX);
        put16(tech_bytes + 2 + 8, 0); /* food */
        put16(tech_bytes + 2 + 10, 50);
        tech_bytes[2 + 12] = 1; /* paid */
        put16(tech_bytes + 2 + 23, 1); /* required count */
        put16(tech_bytes + 2 + 25, 45); /* research location */
        put16(tech_bytes + 2 + 31, 30); /* research time */
        put16(tech_bytes + 2 + 33, 7); /* effect ID */
        put16(tech_bytes + 2 + 37, 12); /* icon */
        tech_bytes[2 + 39] = 3; /* button */
        put16(tech_bytes + 2 + 52, 4); /* name length */
        memcpy(tech_bytes + 56, "Test", 4);
        assert(dat_parse_techs(&tech_blob, &before_techs, &techs));
        assert(techs.count == 1 && techs.next_offset == sizeof(tech_bytes));
        assert(strcmp(techs.techs[0].name, "Test") == 0);
        assert(techs.techs[0].required_techs[0] == 0);
        assert(techs.techs[0].required_techs[1] == -1);
        assert(techs.techs[0].costs[0].amount == 50);
        assert(techs.techs[0].research_time == 30);
        assert(techs.techs[0].effect_id == 7);
        assert(techs.techs[0].icon_id == 12);
        dat_free_techs(&techs);
        tech_blob.size--;
        assert(!dat_parse_techs(&tech_blob, &before_techs, &techs));
    }
    return 0;
}
