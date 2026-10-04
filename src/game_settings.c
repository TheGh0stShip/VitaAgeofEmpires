#include "game_settings.h"

#include <string.h>

#include "le_bytes.h"

/* Index arguments are trusted by the original; out-of-range indices here
   read 0 and write nothing. */
#define IN(i, n) ((i) >= 0 && (i) < (int)(n))

float aoe_settings_speed_ror(const AoeGameSettingsRor *g) { return g->speed; }
void aoe_settings_set_speed_ror(AoeGameSettingsRor *g, float v) { g->speed = v; }
uint8_t aoe_settings_flag_8fc_ror(const AoeGameSettingsRor *g) { return g->flag_8fc; }
int aoe_settings_flag_8fc_clear_ror(const AoeGameSettingsRor *g) { return g->flag_8fc == 0; }
void aoe_settings_set_flag_8fc_ror(AoeGameSettingsRor *g, uint8_t v) { g->flag_8fc = v; }
const char *aoe_settings_scenario_name_ror(const AoeGameSettingsRor *g) {
    return g->scenario_name;
}
void aoe_settings_set_scenario_name_ror(AoeGameSettingsRor *g,
                                        const char *name) {
    strncpy(g->scenario_name, name ? name : "", 128);
    g->scenario_name[128] = 0;
}
uint8_t aoe_settings_flag_97d_ror(const AoeGameSettingsRor *g) { return g->flag_97d; }
uint8_t aoe_settings_flag_97e_ror(const AoeGameSettingsRor *g) { return g->flag_97e; }
void aoe_settings_set_flag_97d_ror(AoeGameSettingsRor *g, int v) {
    g->flag_97d = (uint8_t)v;
    g->flag_97e = v == 0;
}
void aoe_settings_set_flag_97e_ror(AoeGameSettingsRor *g, int v) {
    g->flag_97e = (uint8_t)v;
    g->flag_97d = v == 0;
}
uint8_t aoe_settings_map_width_ror(const AoeGameSettingsRor *g) { return g->map_width; }
uint8_t aoe_settings_map_height_ror(const AoeGameSettingsRor *g) { return g->map_height; }
uint8_t aoe_settings_max_players_ror(const AoeGameSettingsRor *g) { return g->max_players; }
void aoe_settings_set_map_ror(AoeGameSettingsRor *g, uint8_t w, uint8_t h,
                              uint8_t players) {
    g->map_width = w;
    g->map_height = h;
    g->max_players = players;
}
uint8_t aoe_settings_byte_982_ror(const AoeGameSettingsRor *g) { return g->byte_982; }
uint8_t aoe_settings_byte_983_ror(const AoeGameSettingsRor *g) { return g->byte_983; }
uint8_t aoe_settings_byte_984_ror(const AoeGameSettingsRor *g) { return g->byte_984; }
uint8_t aoe_settings_byte_985_ror(const AoeGameSettingsRor *g) { return g->byte_985; }
uint8_t aoe_settings_byte_986_ror(const AoeGameSettingsRor *g) { return g->byte_986; }
uint8_t aoe_settings_byte_987_ror(const AoeGameSettingsRor *g) { return g->byte_987; }
uint8_t aoe_settings_byte_988_ror(const AoeGameSettingsRor *g) { return g->byte_988; }
uint8_t aoe_settings_byte_989_ror(const AoeGameSettingsRor *g) { return g->byte_989; }
void aoe_settings_set_byte_982_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_982 = v; }
void aoe_settings_set_byte_983_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_983 = v; }
void aoe_settings_set_byte_984_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_984 = v; }
void aoe_settings_set_byte_985_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_985 = v; }
void aoe_settings_set_byte_986_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_986 = v; }
void aoe_settings_set_byte_987_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_987 = v; }
void aoe_settings_set_byte_988_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_988 = v; }
void aoe_settings_set_byte_989_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_989 = v; }
uint8_t aoe_settings_player_98a_ror(const AoeGameSettingsRor *g, int i) {
    return IN(i, 9) ? g->player_98a[i] : 0;
}
void aoe_settings_set_player_98a_ror(AoeGameSettingsRor *g, int i, uint8_t v) {
    if (IN(i, 9)) g->player_98a[i] = v;
}
uint8_t aoe_settings_player_flag_ror(const AoeGameSettingsRor *g, int i) {
    return aoe_settings_player_98a_ror(g, i) & 1;
}
uint8_t aoe_settings_player_version_ror(const AoeGameSettingsRor *g, int i) {
    return aoe_settings_player_98a_ror(g, i) >> 1;
}
const char *aoe_settings_player_version_text_ror(const AoeGameSettingsRor *g,
                                                 int i) {
    static const char *const names[23] = {
        "",     "1.0",  "1.0a", "1.0b", "1.0c", "1.0d", "1.0e", "1.1",
        "1.1a", "1.1b", "1.1c", "1.2",  "1.2a", "1.2b", "1.2c", "1.3",
        "1.3a", "1.3b", "1.3c", "1.4",  "1.4a", "1.4b", "1.4c"};
    uint8_t v = aoe_settings_player_version_ror(g, i);
    return v < 23 ? names[v] : "?";
}
void aoe_settings_set_player_flag_ror(AoeGameSettingsRor *g, int i,
                                      uint8_t bit) {
    if (IN(i, 9))
        g->player_98a[i] = (uint8_t)((g->player_98a[i] & 0xfe) | bit);
}
void aoe_settings_set_player_version_ror(AoeGameSettingsRor *g, int i,
                                         uint8_t version) {
    if (IN(i, 9))
        g->player_98a[i] = (uint8_t)((g->player_98a[i] & 1) | (version << 1));
}
uint8_t aoe_settings_byte_993_ror(const AoeGameSettingsRor *g) { return g->byte_993; }
void aoe_settings_set_byte_993_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_993 = v; }
uint8_t aoe_settings_player_994_ror(const AoeGameSettingsRor *g, int i) {
    return IN(i, 9) ? g->player_994[i] : 0;
}
void aoe_settings_set_player_994_ror(AoeGameSettingsRor *g, int i, uint8_t v) {
    if (IN(i, 9)) g->player_994[i] = v;
}
int32_t aoe_settings_int_9a0_ror(const AoeGameSettingsRor *g) { return g->int_9a0; }
int32_t aoe_settings_int_9a4_ror(const AoeGameSettingsRor *g) { return g->int_9a4; }
void aoe_settings_set_int_9a0_ror(AoeGameSettingsRor *g, int32_t v) { g->int_9a0 = v; }
void aoe_settings_set_int_9a4_ror(AoeGameSettingsRor *g, int32_t v) { g->int_9a4 = v; }
void aoe_settings_set_player_9b0_ror(AoeGameSettingsRor *g, int i, int32_t v) {
    if (IN(i, 10)) g->player_9b0[i] = v;
}
void aoe_settings_clear_wonder_ror(AoeGameSettingsRor *g, int p) {
    if (IN(p, 10)) g->wonder_years[p] = -1;
}
void aoe_settings_set_wonder_ror(AoeGameSettingsRor *g, int p, int32_t y) {
    if (IN(p, 10) && (y < g->wonder_years[p] || g->wonder_years[p] < 0))
        g->wonder_years[p] = y;
}
void aoe_settings_wonder_ror(const AoeGameSettingsRor *g, int p, int32_t *y) {
    if (y) *y = IN(p, 10) ? g->wonder_years[p] : -1;
}
float aoe_settings_float_a0c_ror(const AoeGameSettingsRor *g) { return g->float_a0c; }
void aoe_settings_set_float_a0c_ror(AoeGameSettingsRor *g, float v) { g->float_a0c = v; }
int32_t aoe_settings_int_a10_ror(const AoeGameSettingsRor *g) { return g->int_a10; }
void aoe_settings_set_int_a10_ror(AoeGameSettingsRor *g, int32_t v) { g->int_a10 = v; }
uint8_t aoe_settings_byte_a14_ror(const AoeGameSettingsRor *g) { return g->byte_a14; }
void aoe_settings_set_byte_a14_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_a14 = v; }
const char *aoe_settings_empty_text_ror(void) {
    static char empty[1];
    empty[0] = 0;
    return empty;
}

void aoe_settings_pack_ror(const AoeGameSettingsRor *g,
                           uint8_t out[AOE_SETTINGS_RECORD_SIZE]) {
    uint32_t bits;
    size_t n;
    float speed = aoe_settings_speed_ror(g);
    memset(out, 0, AOE_SETTINGS_RECORD_SIZE);
    memcpy(&bits, &speed, sizeof(bits));
    aoe_le_put_u32(out, bits);
    out[4] = aoe_settings_flag_8fc_ror(g);
    n = strlen(aoe_settings_scenario_name_ror(g));
    memcpy(out + 5, aoe_settings_scenario_name_ror(g), n < 128 ? n + 1 : 128);
    out[0x85] = aoe_settings_flag_97d_ror(g);
    out[0x86] = aoe_settings_flag_97e_ror(g);
    out[0x87] = aoe_settings_map_width_ror(g);
    out[0x88] = aoe_settings_map_height_ror(g);
    out[0x89] = aoe_settings_max_players_ror(g);
    out[0x8a] = aoe_settings_byte_982_ror(g);
    out[0x8c] = aoe_settings_byte_984_ror(g);
    out[0x8d] = aoe_settings_byte_985_ror(g);
    out[0x8e] = aoe_settings_byte_986_ror(g);
    out[0x8f] = aoe_settings_byte_987_ror(g);
    out[0x90] = aoe_settings_byte_988_ror(g);
    out[0x91] = aoe_settings_byte_989_ror(g);
    out[0x9b] = aoe_settings_byte_993_ror(g);
    out[0x8b] = aoe_settings_byte_983_ror(g);
    for (int i = 0; i < 9; ++i) {
        out[0x92 + i] = aoe_settings_player_98a_ror(g, i);
        out[0x9c + i] = aoe_settings_player_994_ror(g, i);
    }
}

void aoe_settings_apply_ror(AoeGameSettingsRor *g,
                            const uint8_t in[AOE_SETTINGS_RECORD_SIZE]) {
    uint32_t bits = aoe_le_u32(in);
    float speed;
    char name[129];
    memcpy(&speed, &bits, sizeof(speed));
    aoe_settings_set_speed_ror(g, speed);
    aoe_settings_set_flag_8fc_ror(g, in[4]);
    memcpy(name, in + 5, 128);
    name[128] = 0;
    aoe_settings_set_scenario_name_ror(g, name);
    aoe_settings_set_flag_97d_ror(g, in[0x85]);
    aoe_settings_set_flag_97e_ror(g, in[0x86]);
    aoe_settings_set_map_ror(g, in[0x87], in[0x88], in[0x89]);
    aoe_settings_set_byte_982_ror(g, in[0x8a]);
    aoe_settings_set_byte_984_ror(g, in[0x8c]);
    aoe_settings_set_byte_985_ror(g, in[0x8d]);
    aoe_settings_set_byte_986_ror(g, in[0x8e]);
    aoe_settings_set_byte_987_ror(g, in[0x8f]);
    aoe_settings_set_byte_988_ror(g, in[0x90]);
    aoe_settings_set_byte_989_ror(g, in[0x91]);
    aoe_settings_set_byte_993_ror(g, in[0x9b]);
    aoe_settings_set_byte_983_ror(g, in[0x8b]);
    for (int i = 0; i < 9; ++i) {
        aoe_settings_set_player_98a_ror(g, i, in[0x92 + i]);
        aoe_settings_set_player_994_ror(g, i, in[0x9c + i]);
    }
}

int32_t aoe_settings_map_size_ror(const AoeGameSettingsRor *g) { return g->map_size; }
void aoe_settings_set_map_size_ror(AoeGameSettingsRor *g, int32_t size) {
    static const uint8_t tiles[6] = {0x48, 0x60, 0x78, 0x90, 200, 0xfa};
    g->map_size = size;
    if (size >= 0 && size < 6)
        aoe_settings_set_map_ror(g, tiles[size], tiles[size], 8);
}
int32_t aoe_settings_int_a80_ror(const AoeGameSettingsRor *g) { return g->int_a80; }
void aoe_settings_set_int_a80_ror(AoeGameSettingsRor *g, int32_t v) { g->int_a80 = v; }
int32_t aoe_settings_int_a84_ror(const AoeGameSettingsRor *g) { return g->int_a84; }
void aoe_settings_set_int_a84_ror(AoeGameSettingsRor *g, int32_t v) { g->int_a84 = v; }
int32_t aoe_settings_int_a88_ror(const AoeGameSettingsRor *g) { return g->int_a88; }
void aoe_settings_set_int_a88_ror(AoeGameSettingsRor *g, int32_t v) { g->int_a88 = v; }
int32_t aoe_settings_int_a8c_ror(const AoeGameSettingsRor *g) { return g->int_a8c; }
int32_t aoe_settings_int_a90_ror(const AoeGameSettingsRor *g) { return g->int_a90; }
void aoe_settings_set_a8c_a90_ror(AoeGameSettingsRor *g, int32_t a, int32_t b) {
    g->int_a8c = a;
    g->int_a90 = b;
}
uint8_t aoe_settings_byte_a94_ror(const AoeGameSettingsRor *g, int i) {
    return IN(i, 12) ? g->byte_a94[i] : 0;
}
void aoe_settings_set_byte_a94_ror(AoeGameSettingsRor *g, int i, uint8_t v) {
    if (IN(i, 12)) g->byte_a94[i] = v;
}
int32_t aoe_settings_int_aa0_ror(const AoeGameSettingsRor *g, int i) {
    return IN(i, 9) ? g->int_aa0[i] : 0;
}
void aoe_settings_set_int_aa0_ror(AoeGameSettingsRor *g, int i, int32_t v) {
    if (IN(i, 9)) g->int_aa0[i] = v;
}
uint8_t aoe_settings_byte_ac4_ror(const AoeGameSettingsRor *g, int i) {
    return IN(i, 9) ? g->byte_ac4[i] : 0;
}
void aoe_settings_set_byte_ac4_ror(AoeGameSettingsRor *g, int i, uint8_t v) {
    if (IN(i, 9)) g->byte_ac4[i] = v;
}
uint8_t aoe_settings_byte_acd_ror(const AoeGameSettingsRor *g, int i) {
    return IN(i, 9) ? g->byte_acd[i] : 0;
}
void aoe_settings_set_byte_acd_ror(AoeGameSettingsRor *g, int i, uint8_t v) {
    if (IN(i, 9)) g->byte_acd[i] = v;
}
uint8_t aoe_settings_option_ad6_ror(const AoeGameSettingsRor *g) { return g->option_ad6; }
uint8_t aoe_settings_option_ad7_ror(const AoeGameSettingsRor *g) { return g->option_ad7; }
uint8_t aoe_settings_option_ad8_ror(const AoeGameSettingsRor *g) { return g->option_ad8; }
uint8_t aoe_settings_option_ad9_ror(const AoeGameSettingsRor *g) { return g->option_ad9; }
int32_t aoe_settings_resource_level_ror(const AoeGameSettingsRor *g) { return g->resource_level; }
int32_t aoe_settings_starting_age_ror(const AoeGameSettingsRor *g) { return g->starting_age; }
uint8_t aoe_settings_option_ae4_ror(const AoeGameSettingsRor *g) { return g->option_ae4; }
uint8_t aoe_settings_option_ae5_ror(const AoeGameSettingsRor *g) { return g->option_ae5; }
uint8_t aoe_settings_population_limit_ror(const AoeGameSettingsRor *g) {
    return aoe_settings_flag_97e_ror(g) ? g->population_ae6 : 0x32;
}
uint8_t aoe_settings_byte_1190_ror(const AoeGameSettingsRor *g) { return g->byte_1190; }
void aoe_settings_set_option_ad6_ror(AoeGameSettingsRor *g, uint8_t v) { g->option_ad6 = v; }
void aoe_settings_set_option_ad7_ror(AoeGameSettingsRor *g, uint8_t v) { g->option_ad7 = v; }
void aoe_settings_set_option_ad8_ror(AoeGameSettingsRor *g, uint8_t v) { g->option_ad8 = v; }
void aoe_settings_set_option_ad9_ror(AoeGameSettingsRor *g, uint8_t v) { g->option_ad9 = v; }
void aoe_settings_set_resource_level_ror(AoeGameSettingsRor *g, int32_t v) {
    g->resource_level = v;
}
void aoe_settings_set_starting_age_ror(AoeGameSettingsRor *g, int32_t v) {
    g->starting_age = v;
}
void aoe_settings_set_option_ae4_ror(AoeGameSettingsRor *g, uint8_t v) { g->option_ae4 = v; }
void aoe_settings_set_option_ae5_ror(AoeGameSettingsRor *g, uint8_t v) { g->option_ae5 = v; }
void aoe_settings_set_population_ror(AoeGameSettingsRor *g, uint8_t v) { g->population_ae6 = v; }
void aoe_settings_set_byte_1190_ror(AoeGameSettingsRor *g, uint8_t v) { g->byte_1190 = v; }
void aoe_settings_set_int_1194_ror(AoeGameSettingsRor *g, int32_t v) { g->int_1194 = v; }
