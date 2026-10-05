#include "game_settings.h"

#include <assert.h>
#include <string.h>

/* Patched RoR 1.0a game settings (0x0041b490..0x0041c1a0,
   0x00505260..0x005055c0). */
int main(void) {
    static AoeGameSettingsRor a, b;
    uint8_t rec[AOE_SETTINGS_RECORD_SIZE];
    int32_t y;
    /* The +0x97d/+0x97e pair excludes each other. */
    aoe_settings_set_flag_97d_ror(&a, 1);
    assert(a.flag_97d == 1 && a.flag_97e == 0);
    aoe_settings_set_flag_97e_ror(&a, 1);
    assert(a.flag_97e == 1 && a.flag_97d == 0);
    /* Map sizes 0..5 set the square map with 8 players; others only the
       size. */
    aoe_settings_set_map_size_ror(&a, 4);
    assert(a.map_size == 4 && a.map_width == 200 && a.map_height == 200 &&
           a.max_players == 8);
    aoe_settings_set_map_size_ror(&a, 9);
    assert(a.map_size == 9 && a.map_width == 200);
    aoe_settings_set_map_size_ror(&a, 0);
    assert(a.map_width == 72);
    /* Player byte: flag bit and version index. */
    aoe_settings_set_player_version_ror(&a, 3, 22);
    aoe_settings_set_player_flag_ror(&a, 3, 1);
    assert(a.player_98a[3] == 45 && aoe_settings_player_flag_ror(&a, 3) == 1 &&
           !strcmp(aoe_settings_player_version_text_ror(&a, 3), "1.4c"));
    aoe_settings_set_player_flag_ror(&a, 3, 0);
    assert(aoe_settings_player_version_ror(&a, 3) == 22);
    assert(!strcmp(aoe_settings_player_version_text_ror(&a, 0), ""));
    /* Population limit: +0xae6 only with +0x97e. */
    aoe_settings_set_population_ror(&a, 75);
    assert(aoe_settings_population_limit_ror(&a) == 75);
    aoe_settings_set_flag_97d_ror(&a, 1);
    assert(aoe_settings_population_limit_ror(&a) == 50);
    /* Wonder countdown keeps the lower value; cleared is -1. */
    aoe_settings_clear_wonder_ror(&a, 2);
    aoe_settings_set_wonder_ror(&a, 2, 2000);
    aoe_settings_set_wonder_ror(&a, 2, 1500);
    aoe_settings_set_wonder_ror(&a, 2, 1800);
    aoe_settings_wonder_ror(&a, 2, &y);
    assert(y == 1500);
    /* The name keeps 128 characters. */
    {
        char longname[200];
        memset(longname, 'x', sizeof(longname) - 1);
        longname[199] = 0;
        aoe_settings_set_scenario_name_ror(&a, longname);
        assert(strlen(aoe_settings_scenario_name_ror(&a)) == 128);
    }
    /* Packet round trip. */
    aoe_settings_set_speed_ror(&a, 1.5f);
    aoe_settings_set_scenario_name_ror(&a, "EXPANS_1_1");
    aoe_settings_set_byte_983_ror(&a, 7);
    aoe_settings_set_byte_989_ror(&a, 9);
    aoe_settings_set_player_994_ror(&a, 8, 33);
    aoe_settings_pack_ror(&a, rec);
    assert(rec[0] == 0 && rec[3] == 0x3f && rec[2] == 0xc0 &&
           !memcmp(rec + 5, "EXPANS_1_1", 11) && rec[0x85] == 1 &&
           rec[0x86] == 0 && rec[0x87] == 72 && rec[0x8b] == 7 &&
           rec[0x91] == 9 && rec[0x92 + 3] == 44 && rec[0x9c + 8] == 33);
    aoe_settings_apply_ror(&b, rec);
    assert(b.speed == 1.5f && !strcmp(b.scenario_name, "EXPANS_1_1") &&
           b.flag_97d == 1 && b.flag_97e == 0 && b.map_width == 72 &&
           b.byte_983 == 7 && b.player_98a[3] == 44 &&
           b.player_994[8] == 33);
    /* Applying sets +0x97d then +0x97e, so +0x97e wins a 1/1 record. */
    rec[0x85] = 1;
    rec[0x86] = 1;
    aoe_settings_apply_ror(&b, rec);
    assert(b.flag_97e == 1 && b.flag_97d == 0);
    assert(aoe_settings_empty_text_ror()[0] == 0);
    return 0;
}
