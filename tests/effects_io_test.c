#include "effects_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

int main(void) {
    static const char text[] =
        "3 2\n"
        "1\n"
        "Bronze_Shields 2\n"
        "0 7 -1 8 1.5\n"
        "4 -1 2 9 -2\n"
        "2\n"
        "Empty 0\n";
    DatEffects e, back, *heap;
    AoeOutcomeStream s;
    uint8_t buf[256];
    char name[] = "a_b__c";

    aoe_text_underscores_to_spaces_ror(name);
    CHECK(strcmp(name, "a b  c") == 0);

    CHECK(aoe_effects_read_text_ror(&e, text));
    CHECK(e.count == 3 && e.effects[0].command_count == 0);
    CHECK(strcmp(e.effects[1].name, "Bronze Shields") == 0);
    CHECK(e.effects[1].command_count == 2);
    CHECK(e.effects[1].commands[0].template_id == 7 &&
          e.effects[1].commands[0].attribute == 8 &&
          e.effects[1].commands[0].amount == 1.5f);
    CHECK(e.effects[1].commands[1].type == 4 &&
          e.effects[1].commands[1].class_id == 2 &&
          e.effects[1].commands[1].amount == -2.0f);
    CHECK(strcmp(e.effects[2].name, "Empty") == 0);

    s.data = buf;
    s.size = sizeof buf;
    s.pos = 0;
    s.failed = 0;
    aoe_effects_write_ror(&e, &s);
    /* 4 + 3 * (31 + 2) + 2 * 11 */
    CHECK(!s.failed && s.pos == 4 + 3 * 33 + 22);
    CHECK(buf[0] == 3 && buf[4 + 33 + 31] == 2);
    s.size = s.pos;
    s.pos = 0;
    CHECK(aoe_effects_tribe_read_ror(&back, &s));
    CHECK(back.count == 3 && back.effects[1].command_count == 2);
    CHECK(back.effects[1].commands[1].type == 4 &&
          back.effects[1].commands[0].amount == 1.5f);
    CHECK(strcmp(back.effects[1].name, "Bronze Shields") == 0);
    s.size = 6; /* truncated */
    s.pos = 0;
    s.failed = 0;
    aoe_effects_tribe_release_ror(&back);
    CHECK(!aoe_effects_read_ror(&back, &s));
    aoe_effects_release_ror(&back);

    CHECK(aoe_effects_read_text_ror(&back, "x"));
    CHECK(back.count == 0 && back.effects == NULL);
    CHECK(aoe_effects_destroy_ror(&e, 0) == &e && e.effects == NULL);
    heap = malloc(sizeof *heap);
    aoe_effects_read_text_ror(heap, text);
    CHECK(aoe_effects_tribe_destroy_ror(heap, 1) == NULL);

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("effects_io_test: ok");
    return 0;
}
