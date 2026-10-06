/* The console shell around the translated game's screens. */
#include "shell_app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slp.h"
#include "ui_director.h"
#include "ui_host.h"

#define TYPE_SLP 0x736c7020u

struct AoeShell {
    AoeUiHostRor host;
    AoeUiDirectorRor director;
    int32_t out_w, out_h;
    int32_t vx, vy, vw, vh;         /* the game's rectangle in the output */
    float px, py;                   /* the pointer, game pixels */
    uint32_t held;                  /* buttons last frame */
    int touching;
    uint32_t last_ms;
    /* The pointer shape, decoded once per ID. */
    int32_t pointer_id;
    SlpFrame pointer;
    int pointer_ok;
    uint32_t *scaled_x;             /* output column -> game column */
};

AoeShell *aoe_shell_open(const char *root, int32_t out_w, int32_t out_h,
                         uint32_t (*clock_ms)(void *context), void *clock_context, char *error,
                         size_t error_size) {
    AoeShell *s;
    int32_t x;
    if (out_w < AOE_SHELL_GAME_W || out_h < AOE_SHELL_GAME_H) {
        if (error_size) error[0] = '\0';
        return NULL;
    }
    s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->out_w = out_w;
    s->out_h = out_h;
    /* The largest 4:3 rectangle, centred. */
    s->vh = out_h;
    s->vw = out_h * AOE_SHELL_GAME_W / AOE_SHELL_GAME_H;
    if (s->vw > out_w) {
        s->vw = out_w;
        s->vh = out_w * AOE_SHELL_GAME_H / AOE_SHELL_GAME_W;
    }
    s->vx = (out_w - s->vw) / 2;
    s->vy = (out_h - s->vh) / 2;
    s->scaled_x = malloc(sizeof *s->scaled_x * (size_t)s->vw);
    if (!s->scaled_x) {
        free(s);
        return NULL;
    }
    for (x = 0; x < s->vw; ++x) s->scaled_x[x] = (uint32_t)(x * AOE_SHELL_GAME_W / s->vw);
    if (!aoe_ui_host_open_ror(&s->host, root, AOE_SHELL_GAME_W, AOE_SHELL_GAME_H, clock_ms,
                              clock_context, error, error_size)) {
        free(s->scaled_x);
        free(s);
        return NULL;
    }
    aoe_ui_director_init_ror(&s->director, &s->host);
    if (!aoe_ui_director_start_ror(&s->director)) {
        snprintf(error, error_size, "the main menu could not be built from the data");
        aoe_shell_close(s);
        return NULL;
    }
    s->px = AOE_SHELL_GAME_W / 2.0f;
    s->py = AOE_SHELL_GAME_H / 2.0f;
    s->pointer_id = -1;
    s->last_ms = clock_ms ? clock_ms(clock_context) : 0;
    return s;
}

void aoe_shell_close(AoeShell *s) {
    if (!s) return;
    if (s->pointer_ok) slp_free_frame(&s->pointer);
    aoe_ui_host_close_ror(&s->host);
    free(s->scaled_x);
    free(s);
}

static void key(AoeShell *s, int32_t vk, int down) { aoe_ui_host_key_ror(&s->host, vk, down); }

static void apply_input(AoeShell *s, const AoeShellInput *in, float dt) {
    uint32_t now = in->buttons, pressed = now & ~s->held, released = s->held & ~now;
    int32_t x, y;
    /* Modifiers first, so clicks and keys this frame see them. */
    if (pressed & AOE_SHELL_L) key(s, 0x11, 1);
    if (released & AOE_SHELL_L) key(s, 0x11, 0);
    if (pressed & AOE_SHELL_R) key(s, 0x10, 1);
    if (released & AOE_SHELL_R) key(s, 0x10, 0);
    /* Pointer: touch places it, the stick moves it (a screen width in
       about 1.2 seconds at full tilt). */
    if (in->touching) {
        s->px = (float)(in->touch_x - s->vx) * AOE_SHELL_GAME_W / (float)s->vw;
        s->py = (float)(in->touch_y - s->vy) * AOE_SHELL_GAME_H / (float)s->vh;
    } else {
        s->px += in->stick_x * 520.0f * dt;
        s->py += in->stick_y * 520.0f * dt;
    }
    if (s->px < 0) s->px = 0;
    if (s->py < 0) s->py = 0;
    if (s->px > AOE_SHELL_GAME_W - 1) s->px = AOE_SHELL_GAME_W - 1;
    if (s->py > AOE_SHELL_GAME_H - 1) s->py = AOE_SHELL_GAME_H - 1;
    x = (int32_t)s->px;
    y = (int32_t)s->py;
    if (x != s->host.mouse_x || y != s->host.mouse_y || in->touching != s->touching)
        aoe_ui_host_mouse_move_ror(&s->host, x, y);
    /* A touch is a left button for as long as the finger is down. */
    if (in->touching && !s->touching) aoe_ui_host_mouse_button_ror(&s->host, 1, 1, x, y);
    if (!in->touching && s->touching) aoe_ui_host_mouse_button_ror(&s->host, 1, 0, x, y);
    s->touching = in->touching;
    if (pressed & AOE_SHELL_CROSS) aoe_ui_host_mouse_button_ror(&s->host, 1, 1, x, y);
    if (released & AOE_SHELL_CROSS) aoe_ui_host_mouse_button_ror(&s->host, 1, 0, x, y);
    if (pressed & AOE_SHELL_SQUARE) aoe_ui_host_mouse_button_ror(&s->host, 2, 1, x, y);
    if (released & AOE_SHELL_SQUARE) aoe_ui_host_mouse_button_ror(&s->host, 2, 0, x, y);
    if (pressed & (AOE_SHELL_CIRCLE | AOE_SHELL_START)) {
        key(s, 0x1b, 1);
        key(s, 0x1b, 0);
    }
    if (pressed & AOE_SHELL_TRIANGLE) {
        key(s, 0x0d, 1);
        key(s, 0x0d, 0);
    }
    s->held = now;
}

/* The game frame scaled (nearest) into the viewport, black around it. */
static void blit_scaled(AoeShell *s, uint32_t *out, int32_t stride) {
    const uint8_t *src, *pal = s->host.display.palette;
    uint32_t lut[256];
    int32_t pitch, x, y, i;
    src = aoe_soft_draw_primary_ror(&s->host.display, &pitch);
    for (i = 0; i < 256; ++i)
        lut[i] = 0xff000000u | ((uint32_t)pal[4 * i + 2] << 16) | ((uint32_t)pal[4 * i + 1] << 8) |
                 pal[4 * i];
    for (y = 0; y < s->out_h; ++y) {
        uint32_t *row = out + (size_t)y * (size_t)stride;
        if (y < s->vy || y >= s->vy + s->vh || !src) {
            for (x = 0; x < s->out_w; ++x) row[x] = 0xff000000u;
            continue;
        }
        {
            const uint8_t *line = src + (size_t)((y - s->vy) * AOE_SHELL_GAME_H / s->vh) * (size_t)pitch;
            for (x = 0; x < s->vx; ++x) row[x] = 0xff000000u;
            for (x = 0; x < s->vw; ++x) row[s->vx + x] = lut[line[s->scaled_x[x]]];
            for (x = s->vx + s->vw; x < s->out_w; ++x) row[x] = 0xff000000u;
        }
    }
}

static void draw_pointer(AoeShell *s, uint32_t *out, int32_t stride) {
    const uint8_t *pal = s->host.display.palette;
    int32_t gx, gy, ox, oy, x, y;
    if (s->host.pointer_id != s->pointer_id) {
        uint32_t size = 0;
        uint8_t *data;
        if (s->pointer_ok) slp_free_frame(&s->pointer);
        s->pointer_ok = 0;
        s->pointer_id = s->host.pointer_id;
        data = aoe_ui_host_resource_ror(&s->host, TYPE_SLP, s->pointer_id, &size);
        if (data) {
            memset(&s->pointer, 0, sizeof s->pointer);
            s->pointer_ok = slp_decode_frame(data, size, 0, &s->pointer);
            free(data);
        }
    }
    if (!s->pointer_ok) return;
    gx = (int32_t)s->px - s->pointer.hotspot_x;
    gy = (int32_t)s->py - s->pointer.hotspot_y;
    /* Scaled like the frame: each pointer pixel covers its output pixels. */
    for (oy = 0; oy < (int32_t)s->pointer.height * s->vh / AOE_SHELL_GAME_H + 1; ++oy) {
        int32_t py = oy * AOE_SHELL_GAME_H / s->vh;
        if (py >= (int32_t)s->pointer.height) break;
        y = s->vy + (gy * s->vh) / AOE_SHELL_GAME_H + oy;
        if (y < s->vy || y >= s->vy + s->vh) continue;
        for (ox = 0; ox < (int32_t)s->pointer.width * s->vw / AOE_SHELL_GAME_W + 1; ++ox) {
            int32_t pxi = ox * AOE_SHELL_GAME_W / s->vw;
            const SlpPixel *p;
            if (pxi >= (int32_t)s->pointer.width) break;
            x = s->vx + (gx * s->vw) / AOE_SHELL_GAME_W + ox;
            if (x < s->vx || x >= s->vx + s->vw) continue;
            p = &s->pointer.pixels[(size_t)py * s->pointer.width + (size_t)pxi];
            if (p->kind == SLP_COLOR || p->kind == SLP_PLAYER) {
                const uint8_t *e = pal + 4 * p->index;
                out[(size_t)y * (size_t)stride + (size_t)x] =
                    0xff000000u | ((uint32_t)e[2] << 16) | ((uint32_t)e[1] << 8) | e[0];
            }
        }
    }
}

int aoe_shell_frame(AoeShell *s, const AoeShellInput *in, uint32_t *out, int32_t stride) {
    uint32_t now = s->host.now_ms ? s->host.now_ms(s->host.now_context) : s->last_ms + 16;
    float dt = (float)(now - s->last_ms) / 1000.0f;
    s->last_ms = now;
    if (dt > 0.1f) dt = 0.1f;
    if (in) apply_input(s, in, dt);
    if (s->director.quit) return 0;
    aoe_ui_host_idle_ror(&s->host);
    aoe_ui_host_paint_ror(&s->host);
    if (out) {
        blit_scaled(s, out, stride);
        draw_pointer(s, out, stride);
    }
    return !s->director.quit;
}

const char *aoe_shell_screen_name(const AoeShell *s) {
    return aoe_ui_director_screen_name_ror(&s->director);
}

void aoe_shell_pointer(const AoeShell *s, int32_t *x, int32_t *y) {
    *x = (int32_t)s->px;
    *y = (int32_t)s->py;
}

void aoe_shell_viewport(const AoeShell *s, int32_t rect[4]) {
    rect[0] = s->vx;
    rect[1] = s->vy;
    rect[2] = s->vw;
    rect[3] = s->vh;
}
