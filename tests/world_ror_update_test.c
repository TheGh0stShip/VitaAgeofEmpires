/* Patched RoR 1.0a 0x004e9fa4: the tech list from its text file. The
   formats are the executable's strings 0x0055a1bc "r", 0x0055a1c0
   "%d %d", 0x0055a1c8 "%d", 0x0055a1cc "%s %hd %hd %hd %hd %hd %hd %hd",
   0x0055a1ec "%hd", 0x0055a1f0 "%hd %hd %hd", 0x0055a1fc
   "%hd %hd %d %d %d"; the record offsets are those of the fscanf
   arguments (0x004ea05f..0x004ea323). */
#include "world_ror_update.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int read_text(DatTechs *t, const char *text) {
    return aoe_techs_read_text_ror(t, text, strlen(text));
}

static void well_formed(void) {
    /* A list of three with two listed in reverse order; record 1 stays as
       calloc 0x52607b(0x3c, 3) left it. */
    static const char text[] =
        "3 2\n"
        "2\n"
        "Bronze_Age 1 140 5 2 33 300 109\n" /* button 300: low byte 44 */
        "101\n-1\n-1\n-1\n"
        "0 120 1\n"
        "1 -1 0\n"
        "2 70000 257\n"                     /* %hd low 16; paid byte 1 */
        "40000 5201 106201 107201 32\n"     /* %hd 40000 = 0x9c40 */
        "0\n"
        "Tool_Age 0 100 3 -1 -1 1 109\n"
        "-1 -1 -1 -1\n"
        "0 100 1\n-1 0 0\n-1 0 0\n"
        "4200 5200 106200 107200 84\n";
    DatTechs t;
    const DatTech *a, *b, *none;
    assert(read_text(&t, text) == 1);
    assert(t.count == 3 && t.techs);
    a = &t.techs[2];
    b = &t.techs[0];
    none = &t.techs[1];
    /* Underscores become spaces (0x004ea144..0x004ea16a) before the copy
       0x00450ea0 into +0x00. */
    assert(strcmp(a->name, "Bronze Age") == 0);
    assert(a->required_count == 1 && a->research_time == 140);   /* +0x0c +0x1e */
    assert(a->effect_id == 5 && a->field_22 == 2 && a->field_24 == 33);
    assert((uint8_t)a->button_id == 44);       /* mov dl,[ebp-0x14] at 0x004ea125 */
    assert(a->research_location == 109);       /* +0x28 */
    assert(a->required_techs[0] == 101 && a->required_techs[1] == -1 &&
           a->required_techs[3] == -1);        /* +0x04..+0x0a */
    assert(a->costs[0].resource_type == 0 && a->costs[0].amount == 120 &&
           a->costs[0].paid == 1);
    assert(a->costs[1].resource_type == 1 && a->costs[1].amount == -1 &&
           a->costs[1].paid == 0);
    assert(a->costs[2].amount == (int16_t)(70000 - 65536) && a->costs[2].paid == 1);
    assert(a->language_name == 40000 && a->language_description == 5201);
    assert(a->language_help == 106201 && a->language_tree == 107201 &&
           a->hotkey == 32);                   /* +0x30 +0x34 +0x38 */
    assert(strcmp(b->name, "Tool Age") == 0 && b->field_22 == -1 &&
           b->button_id == 1 && b->required_techs[0] == -1 &&
           b->costs[0].amount == 100 && b->costs[1].resource_type == -1 &&
           b->hotkey == 84);
    assert(none->name == NULL && none->research_time == 0 &&
           none->required_techs[0] == 0 && none->hotkey == 0);
    dat_free_techs(&t);
}

static void no_list(void) {
    DatTechs t;
    /* fopen 0x5263f1 failed: *this = 0 and nothing else. */
    assert(aoe_techs_read_text_ror(&t, NULL, 0) == 0);
    assert(t.techs == NULL && t.count == 0);
    /* cmp eax,-1 (0x004e9ffc) then the low int16 of the size jle
       (0x004ea011): no list. */
    assert(read_text(&t, "") == 1 && t.techs == NULL && t.count == 0);
    assert(read_text(&t, "  \n") == 1 && t.techs == NULL);
    assert(read_text(&t, "0 4\n") == 1 && t.techs == NULL && t.count == 0);
    assert(read_text(&t, "-3 1\n") == 1 && t.techs == NULL && t.count == 0);
    assert(read_text(&t, "65536 1\n") == 1 && t.techs == NULL); /* low int16 0 */
    assert(read_text(&t, "abc\n") == 1 && t.techs == NULL);
    /* Only the size: the list exists, the port lists nothing (the
       original's listed count is uninitialised stack). */
    assert(read_text(&t, "2") == 1 && t.count == 2 && t.techs &&
           t.techs[0].name == NULL && t.techs[1].name == NULL);
    dat_free_techs(&t);
}

static void int16_index_and_size(void) {
    /* movsx of the size (0x004ea01d) and of the index (0x004ea070). */
    DatTechs t;
    assert(read_text(&t, "65538 1\n65536\nZero 7 0 0 0 0 0 0\n"
                         "0 0 0 0\n0 0 0\n0 0 0\n0 0 0\n0 0 0 0 0\n") == 1);
    assert(t.count == 2);
    assert(strcmp(t.techs[0].name, "Zero") == 0 && t.techs[0].required_count == 7);
    assert(t.techs[1].name == NULL);
    dat_free_techs(&t);
}

static void stale_locals_past_the_end(void) {
    /* Two listed, one present: the second pass keeps the index, name,
       button (local_18) and paid (local_10) of the first, so every paid
       byte of record 0 becomes the last one read (0) and the name is
       copied again. */
    DatTechs t;
    assert(read_text(&t, "1 2\n0\nA_b 0 0 0 0 0 7 0\n0 0 0 0\n"
                         "5 6 1\n7 8 1\n9 10 0\n1 2 3 4 5\n") == 1);
    assert(t.count == 1 && strcmp(t.techs[0].name, "A b") == 0);
    assert(t.techs[0].button_id == 7);
    assert(t.techs[0].costs[0].resource_type == 5 && t.techs[0].costs[0].paid == 0);
    assert(t.techs[0].costs[1].amount == 8 && t.techs[0].costs[1].paid == 0);
    assert(t.techs[0].costs[2].paid == 0);
    assert(t.techs[0].language_help == 3 && t.techs[0].hotkey == 5);
    dat_free_techs(&t);
}

static void partial_record(void) {
    /* A scan that stops early has stored what it matched. */
    DatTechs t;
    assert(read_text(&t, "1 1\n0\nP 4 5") == 1);
    assert(strcmp(t.techs[0].name, "P") == 0);
    assert(t.techs[0].required_count == 4 && t.techs[0].research_time == 5);
    assert(t.techs[0].effect_id == 0 && t.techs[0].research_location == 0);
    dat_free_techs(&t);
}

static void port_guards(void) {
    DatTechs t;
    char text[512];
    char longname[151];
    /* Indexes outside the list (the original writes there) are read but
       dropped; a re-listed index keeps the last name. */
    assert(read_text(&t, "2 4\n"
                         "5\nFar 1 1 1 1 1 1 1\n1 1 1 1\n1 1 1\n1 1 1\n1 1 1\n1 1 1 1 1\n"
                         "-1\nNeg 1 1 1 1 1 1 1\n1 1 1 1\n1 1 1\n1 1 1\n1 1 1\n1 1 1 1 1\n"
                         "1\nOld 0 9 0 0 0 0 0\n0 0 0 0\n0 0 0\n0 0 0\n0 0 0\n0 0 0 0 0\n"
                         "1\nNew 0 8 0 0 0 0 0\n0 0 0 0\n0 0 0\n0 0 0\n0 0 0\n0 0 0 0 0\n") == 1);
    assert(t.count == 2 && t.techs[0].name == NULL && t.techs[0].research_time == 0);
    assert(strcmp(t.techs[1].name, "New") == 0 && t.techs[1].research_time == 8);
    dat_free_techs(&t);

    /* The 100-byte local_84: the port keeps 99 characters and reads on. */
    memset(longname, 'x', 150);
    longname[150] = '\0';
    snprintf(text, sizeof text, "1 1\n0\n%s 3 0 0 0 0 0 0\n0 0 0 0\n"
                                "0 0 0\n0 0 0\n0 0 0\n0 0 0 0 9\n", longname);
    assert(read_text(&t, text) == 1);
    assert(strlen(t.techs[0].name) == 99 && t.techs[0].required_count == 3 &&
           t.techs[0].hotkey == 9);
    dat_free_techs(&t);
}

static void from_file(void) {
    static const char path[] = "world_ror_update_test.tmp";
    DatTechs t;
    FILE *f;
    assert(aoe_techs_load_text_file_ror(&t, "no/such/techs.txt") == 0);
    assert(t.techs == NULL && t.count == 0);
    f = fopen(path, "w");
    assert(f);
    fputs("1 1\n0\nFile_Tech 0 30 0 0 0 0 0\n0 0 0 0\n0 0 0\n0 0 0\n0 0 0\n"
          "0 0 0 0 0\n", f);
    fclose(f);
    assert(aoe_techs_load_text_file_ror(&t, path) == 1);
    assert(t.count == 1 && strcmp(t.techs[0].name, "File Tech") == 0 &&
           t.techs[0].research_time == 30);
    dat_free_techs(&t);
    remove(path);
}

int main(void) {
    well_formed();
    no_list();
    int16_index_and_size();
    stale_locals_past_the_end();
    partial_record();
    port_guards();
    from_file();
    return 0;
}
