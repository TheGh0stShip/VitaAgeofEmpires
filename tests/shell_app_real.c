/* The console shell on the user's data: ROOT [OUT.ppm]. Touches and
   buttons in output coordinates drive the translated menus. */
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
static void touch(int32_t x, int32_t y) {
    AoeShellInput in;
    memset(&in, 0, sizeof in);
    in.touching = 1; in.touch_x = x; in.touch_y = y;
    step(&in);
    in.touching = 0;
    step(&in);
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
    int32_t vp[4], px, py, i;
    AoeShellInput in;
    if (argc < 2) return 2;
    /* The fades between screens wait on the clock; this clock advances on
       every read, so they finish. */
    shell = aoe_shell_open(argv[1], 960, 544, clock_ms, NULL, error, sizeof error);
    if (!shell) { fprintf(stderr, "%s\n", error); return 1; }
    aoe_shell_viewport(shell, vp);
    assert(vp[0] == 117 && vp[1] == 0 && vp[2] == 725 && vp[3] == 544);
    memset(&in, 0, sizeof in);
    step(&in);
    assert(strcmp(aoe_shell_screen_name(shell), "Main Menu") == 0);
    /* Letterbox bars are black; the picture is not. */
    assert(frame[100] == 0xff000000u && frame[960 * 300 + 50] == 0xff000000u);
    assert(frame[960 * 300 + 480] != 0xff000000u);
    /* A touch on Single Player (game 320, 198 -> output 479, 224). */
    touch(479, 224);
    assert(strcmp(aoe_shell_screen_name(shell), "Single Player Menu") == 0);
    /* Circle is Escape: back to the main menu. */
    press(AOE_SHELL_CIRCLE);
    assert(strcmp(aoe_shell_screen_name(shell), "Main Menu") == 0);
    /* The stick moves the pointer (left where the touch put it, 320, 198)
       about 8 px a 16 ms frame; Cross clicks where it is: down onto
       Scenario Builder (y 328..367). */
    memset(&in, 0, sizeof in);
    in.stick_y = 1.0f;
    for (i = 0; i < 40; ++i) {
        aoe_shell_pointer(shell, &px, &py);
        if (py >= 340) break;
        step(&in);
    }
    aoe_shell_pointer(shell, &px, &py);
    assert(i > 10 && px >= 318 && px <= 321 && py >= 340 && py <= 356);
    press(AOE_SHELL_CROSS);
    assert(strcmp(aoe_shell_screen_name(shell), "Scenario Editor Menu") == 0);
    /* L holds Ctrl: Ctrl+C would need a key; Triangle is Enter. Back. */
    press(AOE_SHELL_START);
    assert(strcmp(aoe_shell_screen_name(shell), "Main Menu") == 0);
    if (argc > 2) {
        FILE *f = fopen(argv[2], "wb");
        fprintf(f, "P6\n960 544\n255\n");
        for (i = 0; i < 960 * 544; ++i) {
            unsigned char rgb[3] = {(unsigned char)frame[i], (unsigned char)(frame[i] >> 8),
                                    (unsigned char)(frame[i] >> 16)};
            fwrite(rgb, 1, 3, f);
        }
        fclose(f);
    }
    /* Exit quits: touch it (game 320, 398 -> output 479, 451). */
    {
        AoeShellInput t;
        memset(&t, 0, sizeof t);
        t.touching = 1; t.touch_x = 479; t.touch_y = 451;
        assert(aoe_shell_frame(shell, &t, frame, 960));
        t.touching = 0;
        assert(aoe_shell_frame(shell, &t, frame, 960) == 0);
    }
    aoe_shell_close(shell);
    puts("shell ok");
    return 0;
}
