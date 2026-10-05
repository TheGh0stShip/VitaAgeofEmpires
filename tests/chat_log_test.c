#include "chat_log.h"

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

static int buffers, plays, busy;
static char last_path[64];
static int notified = -1;

static int32_t create_buffer(void *c, void *d, const AoeWaveFormatRor *f,
                             uint32_t size, void **b) {
    (void)c;
    (void)d;
    (void)f;
    (void)size;
    *b = &buffers;
    ++buffers;
    return 0;
}
static int32_t fill(void *c, void *b, const uint8_t *data, uint32_t size) {
    (void)c;
    (void)b;
    (void)data;
    (void)size;
    return 0;
}
static int32_t status(void *c, void *b, uint32_t *s) {
    (void)c;
    (void)b;
    *s = busy ? 1 : 0;
    return 0;
}
static void set_int(void *c, void *b, int32_t v) {
    (void)c;
    (void)b;
    (void)v;
}
static void buf_op(void *c, void *b) {
    (void)c;
    (void)b;
}
static void release(void *c, void *o) {
    (void)c;
    (void)o;
}
static int32_t play(void *c, void *b, int looping) {
    (void)c;
    (void)b;
    (void)looping;
    ++plays;
    return 0;
}
static uint8_t *read_file(void *c, const char *path, uint32_t *size) {
    uint8_t *d;
    (void)c;
    snprintf(last_path, sizeof last_path, "%s", path);
    if (strcmp(path, "snd/Taunt007.wav") != 0) return NULL;
    d = calloc(1, 0x30);
    memcpy(d, "RIFF", 4);
    d[0x14] = 1;
    d[0x28] = 4;
    *size = 0x30;
    return d;
}

static const AoeAudioOpsRor ops = {
    .context = NULL,
    .prefer_files = 1,
    .release = release,
    .create_buffer = create_buffer,
    .fill = fill,
    .status = status,
    .set_volume = set_int,
    .set_frequency = set_int,
    .set_pan = set_int,
    .rewind = buf_op,
    .play = play,
    .stop = buf_op,
    .read_file = read_file,
};

static void notify(void *window, int32_t index) {
    (void)window;
    notified = index;
}

static void test_taunts(void) {
    AoeSoundSystemRor s;
    AoeTauntPlayerRor t;

    aoe_sound_clear_ror(&s, &ops);
    aoe_sound_set_dir_ror(&s, "snd/");
    s.ready = 1;
    aoe_taunt_init_ror(&t, &s);
    CHECK(strcmp(t.waves[0]->name, "Silence") == 0 && t.waves[3] != NULL);

    CHECK(aoe_taunt_number_ror("7") == 7);
    CHECK(aoe_taunt_number_ror("!12") == 12);
    CHECK(aoe_taunt_number_ror("1000") == 0);
    CHECK(aoe_taunt_number_ror("hello") == 0);

    CHECK(aoe_taunt_line_ror(&t, "7", 0) == 7);
    CHECK(plays == 1 && t.waves[0]->loaded);
    CHECK(strcmp(t.waves[0]->name, "Taunt007.wav") == 0);
    /* A missing taunt fails and leaves the slot unloaded. */
    CHECK(aoe_taunt_line_ror(&t, "8", 0) == 8 && plays == 1);
    CHECK(strcmp(last_path, "snd/Taunt008.wav") == 0);

    /* Loaded slots that are playing are skipped; with all four busy
       nothing plays. */
    busy = 1;
    CHECK(aoe_taunt_play_ror(&t, 7) && aoe_taunt_play_ror(&t, 7));
    CHECK(aoe_taunt_play_ror(&t, 7) && aoe_taunt_play_ror(&t, 7));
    CHECK(plays == 5 && t.waves[3]->loaded);
    CHECK(!aoe_taunt_play_ror(&t, 7) && plays == 5);
    busy = 0;

    CHECK(aoe_taunt_line_ror(&t, "!MUTE", 0) == 0 && !t.muted);
    CHECK(aoe_taunt_line_ror(&t, "!MUTE", 1) == 0 && t.muted);
    CHECK(aoe_taunt_line_ror(&t, "7", 1) == 0 && plays == 5);
    CHECK(aoe_taunt_line_ror(&t, "!NOMUTE", 1) == 0 && !t.muted);
    aoe_taunt_free_waves_thunk_ror(&t);
    CHECK(t.waves[0] == NULL);
}

static void test_log(void) {
    AoeSoundSystemRor s;
    AoeChatLogRor c;
    int i;

    aoe_sound_clear_ror(&s, &ops);
    aoe_chat_log_init_ror(&c, NULL, &s);
    c.notify = notify;
    CHECK(!aoe_chat_log_add_ror(&c, NULL, "x", 0));
    CHECK(aoe_chat_log_add_ror(&c, "Ann", "hello", 0));
    CHECK(notified == 1 && strcmp(aoe_chat_log_line_ror(&c, 1), "Ann: hello") == 0);
    for (i = 0; i < 50; ++i) aoe_chat_log_add_ror(&c, "Bob", "x", 0);
    CHECK(c.last == 0 && notified == 0); /* the ring wrapped */
    CHECK(aoe_chat_log_line_ror(&c, 51) == NULL);
    CHECK(aoe_chat_log_line_ror(&c, -1) == NULL);

    aoe_chat_log_set_recipient_ror(&c, 0, 1); /* ignored */
    aoe_chat_log_set_recipient_ror(&c, 3, 1);
    CHECK(aoe_chat_log_recipient_ror(&c, 0) == 0);
    CHECK(aoe_chat_log_recipient_ror(&c, 3) == 1);
    CHECK(aoe_chat_log_recipient_ror(&c, 10) == 0);

    aoe_chat_log_free_waves_ror(&c);
    CHECK(c.taunts->waves[0] == NULL);
    CHECK(aoe_chat_log_make_waves_ror(&c, &s) && c.taunts->waves[0]);
    aoe_chat_log_clear_ror(&c);
    CHECK(c.last == 0 && aoe_chat_log_line_ror(&c, 1) == NULL);
    aoe_chat_log_free_ror(&c);
    CHECK(c.taunts == NULL);
}

int main(void) {
    test_taunts();
    test_log();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("chat_log_test: ok");
    return 0;
}
