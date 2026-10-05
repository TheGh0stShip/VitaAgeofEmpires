#include <assert.h>
#include <stddef.h>

#include "player_pair_list.h"

int main(void) {
    AoePlayerPairListRor list;
    const int32_t *pairs;
    int32_t count, i;

    assert(aoe_pair_list_init_ror(&list, 2));
    assert(list.capacity == 2 && list.count == 0 && list.added == 0 &&
           list.shrink == 0 && list.pairs[3] == 0);
    for (i = 0; i < 5; ++i) assert(aoe_pair_list_append_ror(&list, i, -i));
    assert(list.capacity == 8 && list.count == 5 && list.added == 5);
    aoe_pair_list_get_ror(&list, &pairs, &count);
    assert(count == 5 && pairs[8] == 4 && pairs[9] == -4 && pairs[2] == 1);
    assert(aoe_pair_list_take_added_ror(&list) == 5 && list.added == 0);
    assert(aoe_pair_list_take_added_ror(&list) == 0);

    /* Clearing keeps the storage unless the shrink flag is set. */
    for (i = 0; i < 4; ++i) aoe_pair_list_append_ror(&list, 9, 9);
    assert(aoe_pair_list_clear_ror(&list));
    assert(list.count == 0 && list.added == 0 && list.capacity == 16);
    list.shrink = 1;
    assert(aoe_pair_list_clear_ror(&list));
    assert(list.capacity == 8 && list.shrink == 0 && list.pairs[0] == 0);
    aoe_pair_list_destroy_ror(&list);
    assert(list.pairs == NULL);

    /* An empty list grows to one pair. */
    assert(aoe_pair_list_init_ror(&list, 0));
    assert(aoe_pair_list_append_ror(&list, 3, 4) && list.capacity == 1);
    aoe_pair_list_destroy_ror(&list);
    return 0;
}
