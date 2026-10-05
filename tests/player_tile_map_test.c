#include "player_tile_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int reported, finished;
static void on_explored(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; ++reported; }
static void on_done(void *c) { (void)c; ++finished; }

int main(void) {
    uint8_t data[6] = {0xff, 0, 0xff, 1, 0xff, 0xff};
    uint8_t *rows[2] = {data, data + 3};
    uint32_t masks[6] = {0};
    AoeVisibilityMaskMap mm = {3, 2, masks};
    AoePlayerTileMapRor m;
    uint8_t buf[256];
    AoeOutcomeStream s = {buf, sizeof buf, 0, 0};
    AoePlayerMemoryEntry entries[2] = {{7, 1, 2, 3, 4}, {9, 0, 0, 5, 6}};
    AoePlayerMemoryCategory cats[2] = {{entries, 2, 32}, {NULL, 0, 32}};
    AoePlayerObjectMemory mem = {cats, 2};
    uint32_t *global[256];

    memset(&m, 0, sizeof m);
    m.rows = rows;
    m.data = data;
    m.width = 3;
    m.height = 2;
    m.explored = 2;
    m.total = 6;
    m.mask = 4;
    assert(aoe_player_tile_map_explored_ror(&m) == 2.0f / 6.0f);

    assert(aoe_player_tile_map_save_ror(&m, 3, 7.0f, &s));
    assert(s.pos == 4 + 4 + 4 + 2 + 6 && buf[8] == 2 && buf[12] == 3 && buf[14] == 0xff);
    s.pos = 0;
    assert(aoe_player_tile_map_save_ror(&m, 3, 6.5f, &s) && s.pos == 4 + 4 + 2 + 6);

    aoe_player_tile_map_reveal_ror(&m, &mm, on_explored, on_done, NULL);
    assert(m.explored == 6 && reported == 4 && finished == 1);
    assert(data[0] == 0 && data[3] == 1 && masks[0] == 4 && masks[3] == 0 && masks[5] == 4);

    s.pos = 0;
    assert(aoe_player_object_memory_save_ror(&mem, &s));
    assert(s.pos == 4 + 2 * 8 + 2 * 8 && buf[0] == 2 && buf[4] == 32 && buf[8] == 2);
    assert(buf[20] == 7 && buf[24] == 1 && buf[26] == 3 && buf[27] == 4);

    s.pos = 0;
    masks[1] = 0x01020304;
    assert(aoe_visibility_masks_save_ror(&mm, &s) && s.pos == 8 + 24 && buf[12] == 4 && buf[15] == 1);
    aoe_visibility_masks_bind_rows_ror(&mm, global);
    assert(global[0] == masks && global[1] == masks + 3 && global[2] == NULL && global[255] == NULL);

    m.data = malloc(4);
    m.rows = malloc(sizeof(uint8_t *));
    m.player = &m;
    aoe_player_tile_map_free_ror(&m);
    assert(!m.data && !m.rows && !m.player);
    return 0;
}
