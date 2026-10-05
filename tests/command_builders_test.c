/* Patched RoR 1.0a command builders 0x0042b730..0x0042bf00 and the
   repeat filter 0x0042b550..0x0042b690. */
#include "command_builders.h"
#include "command_order.h"
#include "le_bytes.h"

#include <assert.h>
#include <string.h>

static uint32_t cell(void *c, int32_t row, int32_t column) {
    (void)c;
    return (uint32_t)(row * 1000 + column);
}

int main(void) {
    AoeCommandObjectRor a = {11, 0}, b = {12, 0}, target = {40, 7};
    const AoeCommandObjectRor *units[3] = {&a, NULL, &b};
    AoeCommandSessionRor session = {1, 3, 1, 0x30005, NULL, cell};
    uint8_t buf[128], other[128];
    AoeCommandRepeatRor repeat;
    size_t n;

    n = aoe_command_build_order_ror(buf, sizeof(buf), units, 3, &target,
                                    5.75f, 9.25f, &session);
    assert(n == 0x1c + 12 && buf[0] == 0 && buf[1] == 3);
    assert(aoe_le_i32(buf + 4) == 40 && buf[0x11] == 7 && buf[0x10] == 3 &&
           buf[0x12] == 1 && aoe_le_u32(buf + 0x14) == 0x30005 &&
           aoe_le_u32(buf + 0x18) == 9005);
    assert(aoe_le_i32(buf + 0x1c) == 11 && aoe_le_i32(buf + 0x20) == -1 &&
           aoe_le_i32(buf + 0x24) == 12 && aoe_le_f32(buf + 8) == 5.75f);
    session.session = 0;
    n = aoe_command_build_order_ror(other, sizeof(other), units, 3, NULL,
                                    5.0f, 9.9f, &session);
    assert(aoe_le_i32(other + 4) == -1 && other[0x11] == 0 &&
           other[0x10] == 0 && aoe_le_u32(other + 0x18) == 0);
    assert(!aoe_command_build_order_ror(buf, sizeof(buf), units, 0, NULL, 0,
                                        0, NULL));
    assert(!aoe_command_build_order_ror(buf, 0x20, units, 3, NULL, 0, 0,
                                        NULL));

    /* Repeats: same target/IDs and the same truncated position. */
    aoe_le_put_u32(other + 4, 40);
    repeat = (AoeCommandRepeatRor){buf, n, 1000};
    assert(aoe_command_is_repeat_ror(&repeat, other, n, 1499));
    assert(!aoe_command_is_repeat_ror(&repeat, other, n, 1500));
    aoe_le_put_f32(other + 0xc, 10.0f);
    assert(!aoe_command_is_repeat_ror(&repeat, other, n, 1100));

    n = aoe_command_build_order_alternate_ror(buf, sizeof(buf), units, 2,
                                              &target, 1.0f, 2.0f);
    assert(n == 0x24 && buf[0] == 2 && aoe_le_i32(buf + 0x20) == -1);
    {
        int32_t ids[2] = {5, 6};
        n = aoe_command_build_order_alternate_ids_ror(other, sizeof(other),
                                                      ids, 2, 9, 1, 2);
        assert(n == 0x24 && aoe_le_i32(other + 4) == 9 &&
               aoe_le_i32(other + 0x20) == 6);
    }
    n = aoe_command_build_arrange_ror(buf, sizeof(buf), units, 3, 4);
    assert(n == 0x18 && buf[0] == 6 && aoe_le_u32(buf + 4) == 3 &&
           aoe_le_i32(buf + 8) == 4 && aoe_le_i32(buf + 0x14) == 12);
    {
        AoeCommandTaskFieldsRor f = {2, 9, -2, 4, 77, 1, 0x3f800000u,
                                     0xbf800000u, 0, 5, 6, 7};
        n = aoe_command_build_task_list_ror(buf, sizeof(buf), units, 1, &f);
        assert(n == 0x28 && buf[0] == 0x11 && buf[3] == 1 && buf[2] == 9 &&
               buf[4] == 0xfe && buf[5] == 0xff && buf[6] == 4 &&
               aoe_le_i32(buf + 8) == 77 && buf[0x21] == 7 &&
               aoe_le_u32(buf + 0x14) == 0xbf800000u &&
               aoe_le_i32(buf + 0x24) == 11);
    }
    assert(aoe_command_build_resign_ror(buf, 4, 1, 2, 3) == 4 &&
           buf[0] == 0xb && buf[2] == 2 && buf[3] == 3);
    {
        uint8_t rec[6] = {1, 2, 3, 4, 5, 6};
        assert(aoe_command_build_unit_records_ror(buf, sizeof(buf), 2, 88,
                                                  rec, 2) == 0x18);
        assert(buf[0] == 0xc && buf[8] == 2 && buf[0xc] == 4 &&
               aoe_le_i32(buf + 4) == 88);
        assert(!aoe_command_build_unit_records_ror(buf, sizeof(buf), 2, 88,
                                                   rec, 6));
    }
    n = aoe_command_build_record_list_ror(buf, sizeof(buf), units, 2, 8, 9);
    assert(n == 12 && buf[0] == 0x10 && buf[2] == 8 && buf[3] == 9 &&
           aoe_le_i32(buf + 8) == -1);

    /* Stop records compare IDs from +2. */
    {
        int32_t ids[2] = {3, 4};
        uint8_t s1[16], s2[16];
        assert(aoe_command_encode_stop_ror(s1, sizeof(s1), ids, 2) ==
               aoe_command_encode_stop_ror(s2, sizeof(s2), ids, 2));
        assert(aoe_command_same_ror(s1, s2));
        s2[6] = 9;
        assert(!aoe_command_same_stop_ror(s1, s2));
        assert(!aoe_command_same_ror(buf, buf)); /* opcode 0x10 */
    }
    return 0;
}
