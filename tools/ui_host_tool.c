/* Runs the translated screens on user-supplied data through the UI host,
   starting at the Main Menu, with scripted input:
     ui_host_tool ROOT OUTDIR STEP...
   Steps: c:X,Y click; r:X,Y right click; m:X,Y move the pointer; k:VK key press (decimal or 0x);
   d:VK / u:VK key down / up (to hold Ctrl 0x11 or Shift 0x10);
   t:TEXT typed characters; i:N idle N times; p:NAME paint and write
   OUTDIR/NAME.ppm. After each step the current screen's name is printed.
   AOE_USER_DIR names the folder for registry.txt and game<N>.nfo (default:
   the data root). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game_ui.h"
#include "ui_director.h"
#include "ui_host.h"

/* A real clock: the screen transitions are the original's blocking
   palette fades, which wait on it. */
static uint32_t clock_ms(void *c) {
    struct timespec ts;
    (void)c;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
}

static int write_ppm(const char *path, const uint32_t *px, int w, int h) {
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) return 0;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (i = 0; i < w * h; ++i) {
        unsigned char rgb[3] = {(unsigned char)(px[i] & 0xff), (unsigned char)((px[i] >> 8) & 0xff),
                                (unsigned char)((px[i] >> 16) & 0xff)};
        fwrite(rgb, 1, 3, f);
    }
    return fclose(f) == 0;
}

static int colours_used(const AoeUiHostRor *h) {
    static unsigned char seen[256];
    int32_t pitch, i, n = 0;
    const uint8_t *p = aoe_soft_draw_primary_ror(&h->display, &pitch);
    memset(seen, 0, sizeof seen);
    if (!p) return 0;
    for (i = 0; i < h->display.height * pitch; ++i) seen[p[i]] = 1;
    for (i = 0; i < 256; ++i) n += seen[i];
    return n;
}

int main(int argc, char **argv) {
    static AoeUiHostRor host;
    static AoeUiDirectorRor dir;
    static uint32_t frame[640 * 480];
    char error[256];
    int i;
    if (argc < 3) {
        fprintf(stderr, "usage: %s ROOT OUTDIR STEP...\n", argv[0]);
        return 2;
    }
    if (!aoe_ui_host_open_ror(&host, argv[1], 640, 480, clock_ms, NULL, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    aoe_ui_director_init_ror(&dir, &host, getenv("AOE_USER_DIR"));
    if (!aoe_ui_director_start_ror(&dir)) {
        fprintf(stderr, "the main menu did not build\n");
        return 1;
    }
    printf("start: %s\n", aoe_ui_director_screen_name_ror(&dir));
    for (i = 3; i < argc && !dir.quit; ++i) {
        const char *a = argv[i];
        int x = 0, y = 0;
        if (a[0] == 'm' && sscanf(a, "m:%d,%d", &x, &y) == 2) {
            aoe_ui_host_mouse_move_ror(&host, x, y);
        } else if (a[0] == 'c' && sscanf(a, "c:%d,%d", &x, &y) == 2) {
            aoe_ui_host_click_ror(&host, x, y);
        } else if (a[0] == 'r' && sscanf(a, "r:%d,%d", &x, &y) == 2) {
            aoe_ui_host_mouse_move_ror(&host, x, y);
            aoe_ui_host_mouse_button_ror(&host, 2, 1, x, y);
            aoe_ui_host_mouse_button_ror(&host, 2, 0, x, y);
        } else if (a[0] == 'k' && a[1] == ':') {
            int vk = (int)strtol(a + 2, NULL, 0);
            aoe_ui_host_key_ror(&host, vk, 1);
            aoe_ui_host_key_ror(&host, vk, 0);
        } else if ((a[0] == 'd' || a[0] == 'u') && a[1] == ':') {
            aoe_ui_host_key_ror(&host, (int)strtol(a + 2, NULL, 0), a[0] == 'd');
        } else if (a[0] == 't' && a[1] == ':') {
            const char *t;
            for (t = a + 2; *t; ++t) aoe_ui_host_char_ror(&host, (unsigned char)*t);
        } else if (a[0] == 'i' && a[1] == ':') {
            int n = atoi(a + 2), k;
            for (k = 0; k < n; ++k) aoe_ui_host_idle_ror(&host);
        } else if (a[0] == 'p' && a[1] == ':') {
            char path[600];
            aoe_ui_host_paint_ror(&host);
            snprintf(path, sizeof path, "%s/%s.ppm", argv[2], a + 2);
            if (!aoe_soft_draw_present_abgr_ror(&host.display, frame, 640) ||
                !write_ppm(path, frame, 640, 480)) {
                fprintf(stderr, "could not write %s\n", path);
                return 1;
            }
            printf("frame %s: %d colours\n", a + 2, colours_used(&host));
        } else {
            fprintf(stderr, "unknown step %s\n", a);
            return 2;
        }
        /* The game repaints continuously; so does the tool after every
           step (an edit field shows its control from its draw). */
        if (!dir.quit) aoe_ui_host_paint_ror(&host);
        printf("%s -> %s%s%s\n", a, dir.quit ? "(quit)" : aoe_ui_director_screen_name_ror(&dir),
               host.system.modal ? " modal " : "",
               host.system.modal && host.system.modal->name ? host.system.modal->name : "");
    }
    printf("screens built: %u\n", (unsigned)dir.opened);
    if (getenv("AOE_LIST_PANELS")) {
        int32_t k;
        for (k = 0; k < host.system.panel_count; ++k)
            printf("registered: %s\n", host.system.panels[k] && host.system.panels[k]->name
                                           ? host.system.panels[k]->name : "(none)");
    }
    aoe_ui_host_close_ror(&host);
    aoe_ui_director_release_ror(&dir);
    return 0;
}
