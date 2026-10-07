#include "game_save.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char lg[1024]; static int lg_n;
static void add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); lg_n += vsnprintf(lg + lg_n, sizeof lg - (size_t)lg_n, fmt, ap); va_end(ap); }
static void reset(void) { lg_n = 0; lg[0] = '\0'; }

static int32_t g558094;
static uint8_t save_result = 1, single = 1;
static int32_t global_at_save = -1;
static char saved_name[0x200];
static size_t world_name_len;

static void f_cursor(void *c, int w) { (void)c; add("cur%d ", w); }
static void f_status(void *c, int32_t id) { (void)c; add("st%x ", (unsigned)id); }
static void f_close(void *c) { (void)c; add("stclose "); }
static uint8_t f_save(void *c, const char *n) {
    (void)c;
    global_at_save = g558094;
    world_name_len = strlen(n);
    if (world_name_len < 64) add("save(%s) ", n);
    else add("save(long) ");
    return save_result;
}
static void f_name(void *c, const char *n) {
    (void)c;
    snprintf(saved_name, sizeof saved_name, "%s", n);
    if (strlen(n) < 64) add("name(%s) ", n);
    else add("name(long) ");
}
static uint8_t f_single(void *c) { (void)c; return single; }
static void f_unpause(void *c) { (void)c; add("unpause "); }
static void f_show(void *c, const char *n) { (void)c; add("show(%s) ", n); }
static void f_remove(void *c, const char *n) { (void)c; add("rm(%s) ", n); }

static AoeGameSaveOpsRor ops(int32_t mode) {
    AoeGameSaveOpsRor o = {NULL, mode, &g558094, f_cursor, f_status, f_close, f_save,
                           f_name, f_single, f_unpause, f_show, f_remove};
    return o;
}

/* Outside game states 4..7 nothing is touched. */
static void gate_tests(void) {
    static const int32_t refused[] = {-1, 0, 1, 2, 3, 8, 9, 0x100};
    size_t i;
    for (i = 0; i < sizeof refused / sizeof refused[0]; ++i) {
        AoeGameSaveOpsRor o = ops(refused[i]);
        reset(); g558094 = 0;
        assert(aoe_game_save_ror(&o, "hill.gmx") == 0);
        assert(lg_n == 0 && g558094 == 0);
    }
}

/* A saved game: the full name to the world, the name without its
   extension kept, a single player game unpaused, back to the game. */
static void success_tests(void) {
    int32_t mode;
    for (mode = 4; mode <= 7; ++mode) {
        AoeGameSaveOpsRor o = ops(mode);
        reset(); g558094 = 0; global_at_save = -1; save_result = 1; single = 1;
        assert(aoe_game_save_ror(&o, "hill.gmx") == 1);
        assert(strcmp(lg, "cur1 st453 save(hill.gmx) name(hill) unpause show(Game Screen) "
                          "rm(Save Game Screen) rm(Status Screen) cur0 ") == 0);
        assert(g558094 == 1 && global_at_save == 0);
    }
    {
        AoeGameSaveOpsRor o = ops(6);
        /* Only exactly 1 is a single player game. */
        reset(); single = 0;
        assert(aoe_game_save_ror(&o, "a.b.gmx") == 1);
        assert(strcmp(lg, "cur1 st453 save(a.b.gmx) name(a) show(Game Screen) "
                          "rm(Save Game Screen) rm(Status Screen) cur0 ") == 0);
        reset(); single = 2;
        assert(aoe_game_save_ror(&o, "noext") == 1);
        assert(strcmp(lg, "cur1 st453 save(noext) name(noext) show(Game Screen) "
                          "rm(Save Game Screen) rm(Status Screen) cur0 ") == 0);
        reset(); single = 1;
        assert(aoe_game_save_ror(&o, ".gmx") == 1 && saved_name[0] == '\0');
        assert(strstr(lg, "save(.gmx) name() unpause ") != NULL);
    }
}

/* A failed world save: the status closed, the cursor released, 0; the
   global is still set. */
static void failure_tests(void) {
    AoeGameSaveOpsRor o = ops(5);
    reset(); g558094 = 0; save_result = 0; single = 1;
    assert(aoe_game_save_ror(&o, "hill.gmx") == 0);
    assert(strcmp(lg, "cur1 st453 save(hill.gmx) stclose cur0 ") == 0);
    assert(g558094 == 1);
    save_result = 1;
}

/* The 264-byte copy: the world sees the whole name, the kept name is at
   most 263 characters. */
static void long_name_tests(void) {
    char name[400];
    AoeGameSaveOpsRor o = ops(4);
    memset(name, 'x', sizeof name - 1);
    name[sizeof name - 1] = '\0';
    reset(); single = 0;
    assert(aoe_game_save_ror(&o, name) == 1);
    assert(world_name_len == sizeof name - 1 && strlen(saved_name) == 0x107);
    name[300] = '.';
    reset();
    assert(aoe_game_save_ror(&o, name) == 1 && strlen(saved_name) == 0x107);
    name[300] = 'x';
    name[10] = '.';
    reset();
    assert(aoe_game_save_ror(&o, name) == 1 && strlen(saved_name) == 10);
}

int main(void) {
    gate_tests();
    success_tests();
    failure_tests();
    long_name_tests();
    return 0;
}
