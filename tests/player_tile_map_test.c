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
    {
        /* 0x00517490 */
        uint8_t in[] = {3, 0, 0, 0, 2, 0, 0, 0, 4, 0, 0, 0, 2, 0, 1, 2, 3, 4, 5, 6};
        AoeOutcomeStream r = {in, sizeof in, 0, 0};
        AoePlayerTileMapRor l;
        int pa, pb, pc;
        void *players[3] = {&pa, &pb, &pc};
        assert(aoe_player_tile_map_load_ror(&l, 77, players, 3, 7.0f, &r) == 2);
        assert(r.pos == sizeof in && l.width == 3 && l.height == 2 && l.explored == 4 && l.total == 6);
        assert(l.player == &pc && l.field_c == 77 && l.rows[1][2] == 6 && l.rows[0][0] == 1);
        assert(l.mask == 0x40000u);
        aoe_player_tile_map_free_ror(&l);
        /* Before 6.7 no explored count. */
        {
            uint8_t old[] = {0, 0, 0, 0, 5, 0, 0, 0, 1, 0};
            AoeOutcomeStream o = {old, sizeof old, 0, 0};
            assert(aoe_player_tile_map_load_ror(&l, 0, players, 3, 6.5f, &o) == 1);
            assert(o.pos == sizeof old && !l.data && !l.rows && l.explored == 0 && l.player == &pb && l.total == 0);
        }
        r.pos = 0; r.size = 18;
        assert(aoe_player_tile_map_load_ror(&l, 0, players, 3, 7.0f, &r) == -1 && !l.data);
    }
    return 0;
}
