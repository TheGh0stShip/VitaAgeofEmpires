#include "drs.h"

#include <assert.h>

int main(int argc, char **argv) {
    DrsEntry base_entries[] = {{0x736c7020, 149, 10, 100},
                               {0x736c7020, 150, 20, 200}};
    DrsEntry expansion_entries[] = {{0x736c7020, 149, 30, 300}};
    DrsArchive base = {0}, expansion = {0};
    DrsArchive *stack[] = {&expansion, &base};
    DrsMatch match = {0};
    expansion.entries = expansion_entries;
    expansion.entry_count = 1;
    base.entries = base_entries;
    base.entry_count = 2;
    assert(drs_find_first(stack, 2, 0x736c7020, 149, &match));
    assert(match.archive == &expansion && match.entry == expansion_entries);
    assert(drs_find_first(stack, 2, 0x736c7020, 150, &match));
    assert(match.archive == &base && match.entry == base_entries + 1);
    assert(!drs_find_first(stack, 2, 0x736c7020, 151, &match));
    assert(!match.archive && !match.entry);
    if (argc == 3) {
        DrsArchive real_expansion, real_base;
        DrsArchive *real_stack[] = {&real_expansion, &real_base};
        assert(drs_open(&real_expansion, argv[1]));
        assert(drs_open(&real_base, argv[2]));
        assert(drs_find_first(real_stack, 2, drs_type(" pls"), 149, &match));
        assert(match.archive == &real_expansion);
        assert(drs_find_first(real_stack, 2, drs_type(" pls"), 0, &match));
        assert(match.archive == &real_base);
        drs_close(&real_base);
        drs_close(&real_expansion);
    }
    return 0;
}
