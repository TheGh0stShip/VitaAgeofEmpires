/* Builds a translated screen on user-supplied data through the UI host and
   writes the presented frame: ui_host_tool ROOT OUT.ppm [menu]. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game_ui.h"
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

int main(int argc, char **argv) {
    static AoeUiHostRor host;
    static AoeGameSettingsRor settings;
    static AoeGameUiOpsRor ops;
    static AoeGameUiRor ui;
    static uint32_t frame[640 * 480];
    AoeMainMenuRor *menu;
    char error[256];
    int i, distinct = 0;
    static unsigned char seen[256];
    if (argc < 3) {
        fprintf(stderr, "usage: %s ROOT OUT.ppm\n", argv[0]);
        return 2;
    }
    if (!aoe_ui_host_open_ror(&host, argv[1], 640, 480, clock_ms, NULL, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    ui.system = &host.system;
    ui.ez_ops = &host.ez_ops;
    ui.settings = &settings;
    ui.ops = &ops;
    menu = calloc(1, sizeof *menu);
    if (!menu || !aoe_main_menu_init_ror(menu, &ui, &host.area, NULL)) {
        fprintf(stderr, "the main menu did not build\n");
        return 1;
    }
    aoe_ui_system_show_ror(&host.system, &menu->ez.panel, 0);
    aoe_ui_host_paint_ror(&host);
    if (!aoe_soft_draw_present_abgr_ror(&host.display, frame, 640) ||
        !write_ppm(argv[2], frame, 640, 480)) {
        fprintf(stderr, "could not write %s\n", argv[2]);
        return 1;
    }
    {
        int32_t pitch;
        const uint8_t *p = aoe_soft_draw_primary_ror(&host.display, &pitch);
        for (i = 0; i < 480 * pitch; ++i) seen[p[i]] = 1;
        for (i = 0; i < 256; ++i) distinct += seen[i];
    }
    printf("archives %zu, fonts %d/%d/%d, background %s, palette %s, %d colours used\n",
           host.archive_count, host.arial.info != NULL, host.arial_bold.info != NULL,
           host.copperplate.info != NULL, menu->ez.background ? "yes" : "no",
           host.area_palette ? "yes" : "no", distinct);
    aoe_ui_system_close_ror(&host.system, "Main Menu");
    aoe_ui_host_close_ror(&host);
    return 0;
}
