#include "action_io.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

int main(void) {
    static uint8_t buf[2048];
    AoeOutcomeStream s = {buf, sizeof buf, 0, 0};
    AoeActionRecordRor list[4], sub, *back = NULL;
    int32_t n = 0;
    uint8_t bytes[3] = {7, 8, 9};

    memset(list, 0, sizeof list);
    memset(&sub, 0, sizeof sub);
    sub.type = 1;
    sub.state = 2;
    sub.target_id = -1;
    sub.task = -1;
    sub.graphic = 4;
    sub.u32[0] = 0x11223344;
    list[0].type = 9;
    list[0].state = 6;
    list[0].target_id = 1234;
    list[0].target2_id = -1;
    list[0].field_18 = 5;
    list[0].task = 2;
    list[0].graphic = -1;
    list[0].subs = &sub;
    list[0].sub_count = 1;
    list[0].u8[2] = 3;
    list[0].graphics[1] = 77;
    list[1].type = 0x66;
    list[1].i16[0] = 33;
    list[1].u8[0] = 1;
    list[2].type = 0x6c;
    list[2].bytes = bytes;
    list[2].byte_count = 3;
    list[3].type = 0x6a;
    list[3].u8[0] = 9;
    aoe_action_list_save_ror(list, 4, &s, 1);
    assert(!s.failed);
    /* type 9: 2 + base (1 + 4*8 + 2 + 1) + sub (2 + 36 + 2 + 2 + 4 + 2) + 2 + 4+4+2+2+3+6 */
    s.size = s.pos;
    s.pos = 0;
    assert(aoe_action_list_load_ror(&back, &n, &s, 1, 3, 7.24f) == 1);
    assert(s.pos == s.size && n == 4);
    assert(back[0].type == 9 && back[0].state == 6 && back[0].target_id == 1234);
    assert(back[0].target2_id == -1 && back[0].field_18 == 5 && back[0].task == 2);
    assert(back[0].sub_count == 1 && back[0].subs[0].u32[0] == 0x11223344);
    assert(back[0].subs[0].graphic == 4 && back[0].u8[2] == 3 && back[0].graphics[1] == 77);
    assert(back[1].i16[0] == 33 && back[1].u8[0] == 1);
    assert(back[2].byte_count == 3 && back[2].bytes[2] == 9);
    assert(back[3].u8[0] == 9);

    /* Older versions: research has no +0x4c byte before 7.23; the 0x6a
       byte arrives with 6.5. */
    {
        AoeActionRecordRor one[1];
        AoeOutcomeStream o = {buf, sizeof buf, 0, 0};
        memset(one, 0, sizeof one);
        one[0].type = 0x66;
        one[0].i16[0] = 5;
        aoe_action_list_save_ror(one, 1, &o, 1);
        o.size = o.pos - 3; /* drop the +0x4c byte and the end marker */
        buf[o.size] = 0;
        buf[o.size + 1] = 0;
        o.size += 2;
        o.pos = 0;
        assert(aoe_action_list_load_ror(&back, &n, &o, 1, 3, 7.0f) == 1 && n == 1);
        assert(back[0].i16[0] == 5 && back[0].u8[0] == 0 && o.pos == o.size);
    }
    /* The base factory does not make RoR types. */
    {
        AoeActionRecordRor one;
        AoeOutcomeStream o = {buf, sizeof buf, 0, 0};
        assert(aoe_action_load_ror(&one, 0x66, &o, 0, 0, 7.24f) == 0);
        assert(aoe_action_load_ror(&one, 0x6d, &o, 1, 0, 7.24f) == 0);
    }
    {
        int32_t i;
        for (i = 0; i < n; ++i) aoe_action_release_ror(&back[i]);
        free(back);
    }
    puts("action_io_test: ok");
    return 0;
}
