#include "demo_app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Host harness for the Vita tech demo: runs the same portable demo loop
   headless with scripted input at a fixed 30 Hz frame step and writes the
   final frame as a PPM. It exercises exactly the code the Vita build runs
   apart from the display/input/clock layer. */

static uint64_t clock_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

static int write_ppm(const char *path, const uint32_t *fb, size_t stride) {
    FILE *f = fopen(path, "wb");
    int ok = f && fprintf(f, "P6\n%d %d\n255\n", AOE_DEMO_WIDTH,
                          AOE_DEMO_HEIGHT) > 0;
    for (int y = 0; ok && y < AOE_DEMO_HEIGHT; ++y)
        for (int x = 0; ok && x < AOE_DEMO_WIDTH; ++x) {
            uint32_t p = fb[(size_t)y * stride + (size_t)x];
            uint8_t rgb[3] = {(uint8_t)p, (uint8_t)(p >> 8),
                              (uint8_t)(p >> 16)};
            ok = fwrite(rgb, 1, 3, f) == 3;
        }
    if (f && fclose(f) != 0) ok = 0;
    return ok;
}

int main(int argc, char **argv) {
    char error[256];
    AoeDemo *demo;
    AoeDemoStats stats;
    uint32_t *fb;
    int frames, ok = 1;
    size_t start_scenario = 0;
    double draw_total = 0.0, update_total = 0.0;
    size_t terrain_mismatches = 0;
    int verify = getenv("DEMO_VERIFY_TERRAIN") != NULL;
    if (argc < 4) {
        fprintf(stderr, "usage: demo_host ROOT FRAMES OUT.ppm [SCENARIO "
                "[CAMERA_X CAMERA_Y]]\n");
        return 2;
    }
    frames = atoi(argv[2]);
    if (argc > 4) start_scenario = (size_t)strtoul(argv[4], NULL, 10);
    demo = aoe_demo_open(argv[1], clock_us, error, sizeof(error));
    if (!demo) {
        fprintf(stderr, "demo open failed: %s\n", error);
        return 1;
    }
    fb = calloc((size_t)AOE_DEMO_WIDTH * AOE_DEMO_HEIGHT, sizeof(*fb));
    if (!fb || !aoe_demo_load_scenario(demo, start_scenario)) {
        fprintf(stderr, "scenario load failed: %s\n", aoe_demo_status(demo));
        return 1;
    }
    /* A fixed camera disables the scripted input (a still view). */
    int still = argc > 6;
    if (still)
        aoe_demo_set_camera(demo, strtof(argv[5], NULL),
                            strtof(argv[6], NULL));
    for (int i = 0; ok && i < frames; ++i) {
        AoeDemoInput in;
        memset(&in, 0, sizeof(in));
        if (still) goto step;
        /* Script: train, order the army, cycle views, scroll. */
        if (i == 2) in.pressed = AOE_DEMO_BUTTON_TRAIN;
        if (i == 4) in.pressed = AOE_DEMO_BUTTON_CAMERA;
        if (i == 6) in.pressed = AOE_DEMO_BUTTON_ATTACK;
        if (i > 10 && i < 30) in.scroll_x = 1.0f;
        if (i > 30 && i < 50) in.scroll_y = 1.0f;
        if (i > 50 && i < 70) {
            in.scroll_x = -0.7f;
            in.scroll_y = -0.4f;
        }
    step:
        if (!aoe_demo_update(demo, &in, 1.0f / 30.0f) ||
            !aoe_demo_render(demo, fb, AOE_DEMO_WIDTH))
            ok = 0;
        if (ok && verify) terrain_mismatches +=
            aoe_demo_verify_terrain_cache(demo);
        aoe_demo_stats(demo, &stats);
        draw_total += stats.draw_ms;
        update_total += stats.update_ms;
    }
    aoe_demo_stats(demo, &stats);
    printf("%s: %llu frames, %llu world updates, %u live units, %u drawn, "
           "trained %u, attack orders %u, avg update %.2f ms, draw %.2f ms, "
           "camera %.1f,%.1f\n",
           aoe_demo_status(demo), (unsigned long long)stats.frames,
           (unsigned long long)stats.world_updates, stats.live_units,
           stats.units_drawn, stats.trained, stats.attack_orders,
           frames ? update_total / frames : 0.0,
           frames ? draw_total / frames : 0.0, stats.camera_x,
           stats.camera_y);
    printf("terrain cache: %u full, %u shifted redraws",
           stats.terrain_full, stats.terrain_shifted);
    if (verify) {
        printf(", %zu mismatched pixels against full redraws",
               terrain_mismatches);
        if (terrain_mismatches) ok = 0;
    }
    printf("\n");
    if (ok && !write_ppm(argv[3], fb, AOE_DEMO_WIDTH)) ok = 0;
    free(fb);
    aoe_demo_close(demo);
    return ok ? 0 : 1;
}
