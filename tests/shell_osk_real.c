/* The on-screen keyboard on the user's data: ROOT [FRAME.ppm]. With no players yet,
   Single Player -> Campaign asks for a name; the keyboard comes up, a
   name is typed with touches, Cross and Circle, and Done enters it. */
#include "shell_app.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t fake_ms;
static uint32_t clock_ms(void *c) { (void)c; return fake_ms += 16; }

static AoeShell *shell;
static uint32_t frame[960 * 544];

static void step(const AoeShellInput *in) { assert(aoe_shell_frame(shell, in, frame, 960)); }
static void idle(void) {
    AoeShellInput in;
    memset(&in, 0, sizeof in);
    step(&in);
}
static void touch(int32_t x, int32_t y) {
    AoeShellInput in;
    memset(&in, 0, sizeof in);
    in.touching = 1; in.touch_x = x; in.touch_y = y;
    step(&in);
    in.touching = 0;
    step(&in);
}
static void touch_key(int row, int col) {
    int32_t r[4];
    aoe_osk_key_rect_ror(aoe_shell_osk(shell), row, col, r);
    touch(r[0] + r[2] / 2, r[1] + r[3] / 2);
}
static void press(uint32_t b) {
    AoeShellInput in;
    memset(&in, 0, sizeof in);
    in.buttons = b;
    step(&in);
    in.buttons = 0;
    step(&in);
}

int main(int argc, char **argv) {
    char error[256];
    const char *text;
    if (argc < 2) return 2;
    shell = aoe_shell_open(argv[1], 960, 544, clock_ms, NULL, error, sizeof error);
    if (!shell) { fprintf(stderr, "%s\n", error); return 1; }
    idle();
    assert(!aoe_shell_osk(shell)->visible);
    touch(479, 224);                       /* Single Player (game 320, 198) */
    assert(strcmp(aoe_shell_screen_name(shell), "Single Player Menu") == 0);
    touch(479, 213);                       /* Campaign (game 320, 188) */
    idle();
    assert(aoe_shell_osk(shell)->visible && aoe_shell_osk(shell)->shift);
    text = aoe_shell_text_field(shell);
    assert(text && text[0] == '\0');
    touch_key(2, 0);                       /* A (Shift was on) */
    touch_key(3, 4);                       /* b */
    text = aoe_shell_text_field(shell);
    assert(text && strcmp(text, "Ab") == 0);
    press(AOE_SHELL_CROSS);                /* the highlighted key again: b */
    assert(strcmp(aoe_shell_text_field(shell), "Abb") == 0);
    press(AOE_SHELL_CIRCLE);               /* delete */
    assert(strcmp(aoe_shell_text_field(shell), "Ab") == 0);
    if (argc > 2) {                        /* the frame with the keyboard up */
        FILE *f = fopen(argv[2], "wb");
        int i;
        assert(f);
        fprintf(f, "P6\n960 544\n255\n");
        for (i = 0; i < 960 * 544; ++i) {
            unsigned char rgb[3] = {(unsigned char)frame[i], (unsigned char)(frame[i] >> 8),
                                    (unsigned char)(frame[i] >> 16)};
            fwrite(rgb, 1, 3, f);
        }
        fclose(f);
    }
    touch_key(4, 3);                       /* Done: Enter */
    idle();
    assert(strcmp(aoe_shell_screen_name(shell), "Campaign Selection Screen") == 0);
    assert(!aoe_shell_osk(shell)->visible);
    aoe_shell_close(shell);
    puts("osk on the data ok");
    return 0;
}
