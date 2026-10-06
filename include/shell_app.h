#ifndef VITA_AOE_SHELL_APP_H
#define VITA_AOE_SHELL_APP_H

#include <stddef.h>
#include <stdint.h>

/* The console shell around the translated game: the UI host and screen
   director at the original 640 x 480, shown letterboxed in a larger
   32-bit framebuffer, with touch and buttons turned into the mouse and
   keys the screens expect, and the game's own pointer (Interfac 51001)
   drawn on top. Integration support; portable, so the host tests drive
   the same code as the Vita.

   Controls (`AoeShellInput`):
     touch                  - moves the pointer and clicks (left button)
     stick / D-pad          - moves the pointer
     Cross                  - left button (held while pressed)
     Square                 - right button
     Circle                 - Escape (back / cancel, as the screens use it)
     Triangle               - Enter
     L                      - held Ctrl, R - held Shift (for the original's
                              key combinations)
     Start                  - Escape */

enum {
    AOE_SHELL_GAME_W = 640,
    AOE_SHELL_GAME_H = 480
};

enum {
    AOE_SHELL_CROSS = 1u << 0,
    AOE_SHELL_SQUARE = 1u << 1,
    AOE_SHELL_CIRCLE = 1u << 2,
    AOE_SHELL_TRIANGLE = 1u << 3,
    AOE_SHELL_L = 1u << 4,
    AOE_SHELL_R = 1u << 5,
    AOE_SHELL_START = 1u << 6
};

typedef struct {
    uint32_t buttons;          /* AOE_SHELL_* held this frame */
    float stick_x, stick_y;    /* -1..1, dead zone already applied */
    int touching;              /* a finger on the screen */
    int32_t touch_x, touch_y;  /* in output framebuffer pixels */
} AoeShellInput;

typedef struct AoeShell AoeShell;

/* Opens the data under `root` for an `out_w` x `out_h` output (at least
   640 x 480). `clock_ms` is a monotonic millisecond clock. NULL with a
   reason in `error` on failure. */
AoeShell *aoe_shell_open(const char *root, int32_t out_w, int32_t out_h,
                         uint32_t (*clock_ms)(void *context), void *clock_context, char *error,
                         size_t error_size);
void aoe_shell_close(AoeShell *s);

/* One frame: applies the input (edges against the previous frame), runs
   the screen's idle work, paints and writes the letterboxed frame
   (A8B8G8R8, R in the low byte) into `out` with a row stride of `stride`
   pixels. Returns 0 once the game asked to quit. */
int aoe_shell_frame(AoeShell *s, const AoeShellInput *in, uint32_t *out, int32_t stride);

/* Diagnostics for tests and the overlay. */
const char *aoe_shell_screen_name(const AoeShell *s);
void aoe_shell_pointer(const AoeShell *s, int32_t *x, int32_t *y);
/* The game rectangle inside the output (x, y, w, h). */
void aoe_shell_viewport(const AoeShell *s, int32_t rect[4]);

#endif
