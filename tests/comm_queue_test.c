#include "comm_queue.h"
#include "combat_stats.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    AoeCommQueueRor q;
    uint8_t *d;
    uint32_t late = 0;
    int i;

    assert(aoe_comm_queue_init_ror(&q, 3) && q.capacity == 20 && q.seq == 1);
    assert(aoe_comm_queue_add_ror(&q, 10, "aaa", 3, 2, 5, 0));
    assert(aoe_comm_queue_add_ror(&q, 10, "bb", 2, 1, 9, 0));
    assert(aoe_comm_queue_add_ror(&q, 11, "c", 1, 1, 1, 0));
    assert(q.high == 2 && q.current == 2 && q.entries[0].key == 2005 && q.entries[1].key == 1009);
    /* Lowest key for turn 10 first: player 1's (1009). */
    d = aoe_comm_queue_take_turn_ror(&q, 10, &late);
    assert(d && memcmp(d, "bb", 2) == 0 && late == 0);
    free(d);
    d = aoe_comm_queue_take_turn_ror(&q, 11, &late);
    assert(d && d[0] == 'c' && late == 1); /* turn 10's command is late */
    free(d);
    assert(aoe_comm_queue_take_turn_ror(&q, 12, NULL) == NULL);
    /* Fill past the capacity: it grows by 20. */
    for (i = 0; i < 25; ++i) assert(aoe_comm_queue_add_ror(&q, 20, "x", 1, 3, (uint8_t)i, i));
    assert(q.capacity == 40 && q.entries[0].size == 3);
    aoe_comm_queue_flush_player_ror(&q, 2);
    assert(q.entries[0].size == 0);
    d = aoe_comm_queue_take_first_ror(&q);
    assert(d && d[0] == 'x');
    free(d);
    assert(aoe_comm_queue_flush_ror(&q, 20, 20) >= 24);
    assert(aoe_comm_queue_take_first_ror(&q) == NULL);
    /* Sequence bytes per turn. */
    assert(aoe_comm_queue_next_seq_ror(&q, 7) == 1 && aoe_comm_queue_next_seq_ror(&q, 7) == 2);
    q.seq = 0xfd;
    assert(aoe_comm_queue_next_seq_ror(&q, 7) == 0xfe && aoe_comm_queue_next_seq_ror(&q, 7) == 0);
    assert(aoe_comm_queue_next_seq_ror(&q, 8) == 1);
    aoe_comm_queue_release_ror(&q);

    {
        AoeTemplateRor t;
        AoeTemplatePairRor atk[2] = {{3, 7}, {4, 12}}, arm[2] = {{3, 2}, {4, -1}};
        int16_t v, b;
        memset(&t, 0, sizeof t);
        t.field_100 = 5;
        t.attacks = atk; t.attack_count = 2;
        t.armors = arm; t.armor_count = 2;
        t.max_attack = 6; t.max_armor = 1;
        t.field_114 = 7.0f; t.field_120 = 1.5f; t.size[0] = 0.3f; t.size[1] = 0.4f;
        aoe_combat_attack_ror(&t, &v, &b);
        assert(v == 12 && b == 6);
        aoe_combat_armor_ror(&t, &v, &b);
        assert(v == 2 && b == 1);
        assert(aoe_combat_armor_sum_ror(&t) == 2.0f && aoe_combat_reload_ror(&t) == 1.5f);
        assert(aoe_combat_in_range_ror(&t, 7.0f, 7.79f) && !aoe_combat_in_range_ror(&t, 7.0f, 7.81f));
    }
    puts("comm_queue_test: ok");
    return 0;
}
