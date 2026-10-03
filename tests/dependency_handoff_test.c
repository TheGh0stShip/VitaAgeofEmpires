#include "live_unit_factory.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static AoeLiveUnit units[5];
static int has_object[5];
static int adoptions;
static AoeLiveUnit *adopted_by, *adopted_from;

static AoeLiveUnit *resolve(void *context, int32_t unit_id) {
    (void)context;
    for (int i = 0; i < 5; ++i)
        if (units[i].constructed && units[i].core.unit_id == unit_id)
            return &units[i];
    return NULL;
}

static int group_object(void *context, const AoeLiveUnit *unit) {
    (void)context;
    return has_object[unit - units];
}

static void adopt(void *context, AoeLiveUnit *leader, AoeLiveUnit *source) {
    (void)context;
    ++adoptions;
    adopted_by = leader;
    adopted_from = source;
}

int main(void) {
    AoeLiveUnitDependencyHandoffOps ops = {
        {0, resolve}, 0, group_object, adopt};
    /* Source 10 leads [10, 11, 12, 13, 14]. 11 is dead (state 3), 12 has
       no group object, so 13 becomes leader and 14 follows it. */
    int32_t ids[5] = {10, 11, 12, 13, 14};
    for (int i = 0; i < 5; ++i) {
        units[i].constructed = 1;
        units[i].core.unit_id = 10 + i;
        units[i].core.state = 2;
        has_object[i] = 1;
        assert(aoe_live_unit_set_dependencies_ror(&units[i], ids, 5, 10, 0));
    }
    units[0].core.state = 3; /* setter already stored 3 */
    units[1].core.state = 3;
    has_object[2] = 0;
    assert(aoe_live_unit_dependency_handoff_ror(&units[0], &ops));
    assert(units[0].dependent_source_id == 10); /* source unchanged */
    assert(units[1].dependent_source_id == 10 &&
           units[2].dependent_source_id == 10);
    assert(units[3].dependent_source_id == 13 &&
           units[4].dependent_source_id == 13);
    assert(adoptions == 1 && adopted_by == &units[3] &&
           adopted_from == &units[0]);
    assert(units[0].dependent_unit_count == 5);

    /* Branch condition: not its own source, fewer than two IDs, or no
       group object on the source. */
    units[0].dependent_source_id = 13;
    assert(!aoe_live_unit_dependency_handoff_ror(&units[0], &ops));
    units[0].dependent_source_id = 10;
    has_object[0] = 0;
    assert(!aoe_live_unit_dependency_handoff_ror(&units[0], &ops));
    has_object[0] = 1;
    assert(aoe_live_unit_set_dependencies_ror(&units[0], ids, 1, 10, 0) &&
           !aoe_live_unit_dependency_handoff_ror(&units[0], &ops));
    /* No qualifying member: nobody adopts. */
    assert(aoe_live_unit_set_dependencies_ror(&units[0], ids, 3, 10, 0));
    adoptions = 0;
    assert(aoe_live_unit_dependency_handoff_ror(&units[0], &ops) &&
           !adoptions);
    ops.adopt_group = NULL;
    assert(!aoe_live_unit_dependency_handoff_ror(&units[0], &ops));
    for (int i = 0; i < 5; ++i) free(units[i].dependent_unit_ids);
    return 0;
}
