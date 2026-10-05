#include "civ_table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

static int16_t sid(void *c, const void *p) { (void)c; (void)p; return -1; }

int main(void) {
    AoeTemplateOpsRor ops;
    AoeCivRor c, d;
    AoeTemplateRor *t;
    static uint8_t buf[4096];
    AoeOutcomeStream s = {buf, sizeof buf, 0, 0};
    FILE *f;
    memset(&ops, 0, sizeof ops);
    ops.sprite_id = sid;
    ops.sound_id = sid;

    f = tmpfile();
    fputs("Greek 3 4\n2\n1 1.5\n3 -2\n7\n", f);
    rewind(f);
    assert(aoe_civ_from_text_ror(&c, f));
    fclose(f);
    assert(!strcmp(c.name, "Greek") && c.field_2a == 3 && c.value_count == 4);
    assert(c.values[1] == 1.5f && c.values[3] == -2.0f && c.values[0] == 0.0f && c.field_28 == 7);
    assert(c.template_count == 0 && c.templates == NULL);

    assert(aoe_civ_set_template_count_ror(&c, 3));
    t = calloc(1, sizeof *t);
    t->type = 10;
    t->id = 83;
    t->hit_points = 25;
    c.templates[1] = t;
    c.field_29 = 1;

    aoe_civ_save_ror(&c, &s, &ops);
    assert(!s.failed && buf[0] == 1 && !memcmp(buf + 1, "Greek", 5));
    s.size = s.pos;
    s.pos = 1; /* the world reads the leading byte */
    assert(aoe_civ_read_ror(&d, &s));
    assert(!strcmp(d.name, "Greek") && d.value_count == 4 && d.values[3] == -2.0f && d.field_28 == 7);
    assert(aoe_civ_read_templates_ror(&d, &s, NULL, NULL, 7.2f, &ops));
    assert(s.pos == s.size && d.template_count == 3 && !d.templates[0] && !d.templates[2]);
    assert(d.templates[1] && d.templates[1]->id == 83 && d.templates[1]->hit_points == 25);

    /* Unknown types leave the slot empty. */
    assert(aoe_civ_read_template_ror(&d, &s, 70, NULL, NULL, 0, 7.2f, &ops) && !d.templates[0]);

    aoe_civ_release_ror(&c, &ops);
    aoe_civ_release_ror(&d, &ops);
    assert(!c.values && !c.templates && d.template_count == 0);
    return 0;
}
