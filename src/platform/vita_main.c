#include "demo_app.h"

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/power.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* PS Vita platform layer for the tech demo: CDRAM display buffers, the
   controller, the process clock and CPU clocks. The demo writes its 32-bit
   output straight into the CDRAM back buffer (the overlay never reads it
   back), which is then flipped on the next vblank. Loading and error
   screens go through a main-memory frame. */

/* The original game data, runtime and sprite cache live on the newlib
   heap; the default heap is too small for them. */
int _newlib_heap_size_user = 128 * 1024 * 1024;
/* Deep translated call chains (research, effects) run on the main thread. */
unsigned int sceUserMainThreadStackSize = 1024 * 1024;

#define DEMO_ROOT "ux0:data/VitaAoE"

enum {
    FB_STRIDE = 960, /* pixels; a multiple of 64 as the display requires */
    FB_BYTES = FB_STRIDE * AOE_DEMO_HEIGHT * 4,
    FB_BLOCK = (FB_BYTES + 0x3ffff) & ~0x3ffff /* 256 KiB CDRAM granule */
};

typedef struct {
    SceUID block;
    void *base;
} DisplayBuffer;

static DisplayBuffer buffers[2];
static int back;

static uint64_t clock_us(void) {
    return (uint64_t)sceKernelGetProcessTimeWide();
}

static int display_init(void) {
    for (int i = 0; i < 2; ++i) {
        buffers[i].block = sceKernelAllocMemBlock(
            "aoe_fb", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, FB_BLOCK, NULL);
        if (buffers[i].block < 0 ||
            sceKernelGetMemBlockBase(buffers[i].block, &buffers[i].base) < 0)
            return 0;
        memset(buffers[i].base, 0, FB_BYTES);
    }
    back = 0;
    return 1;
}

/* Shows the back buffer from the next vblank and waits for the switch, so
   the following frame never draws into the buffer being scanned out. */
static void display_flip(void) {
    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof(fb));
    fb.size = sizeof(fb);
    fb.base = buffers[back].base;
    fb.pitch = FB_STRIDE;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = AOE_DEMO_WIDTH;
    fb.height = AOE_DEMO_HEIGHT;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitSetFrameBuf();
    back ^= 1;
}

static void display_present(const uint32_t *frame) {
    SceDisplayFrameBuf fb;
    memcpy(buffers[back].base, frame, FB_BYTES);
    memset(&fb, 0, sizeof(fb));
    fb.size = sizeof(fb);
    fb.base = buffers[back].base;
    fb.pitch = FB_STRIDE;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = AOE_DEMO_WIDTH;
    fb.height = AOE_DEMO_HEIGHT;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    back ^= 1;
}

static void display_free(void) {
    for (int i = 0; i < 2; ++i)
        if (buffers[i].block >= 0) sceKernelFreeMemBlock(buffers[i].block);
}

static void text_screen(uint32_t *frame, const char *title,
                        const char *body) {
    for (size_t i = 0; i < (size_t)FB_STRIDE * AOE_DEMO_HEIGHT; ++i)
        frame[i] = 0xff201810u;
    aoe_demo_draw_text(frame, FB_STRIDE, 40, 40, 3, 0xff40d0ffu,
                       "AGE OF EMPIRES: RISE OF ROME");
    aoe_demo_draw_text(frame, FB_STRIDE, 40, 80, 2, 0xffffffffu,
                       "PS VITA RECONSTRUCTION - TECH DEMO");
    aoe_demo_draw_text(frame, FB_STRIDE, 40, 150, 2, 0xff80ffffu, title);
    aoe_demo_draw_text(frame, FB_STRIDE, 40, 190, 2, 0xffffffffu, body);
    display_present(frame);
}

static float axis(uint8_t value) {
    float v = ((float)value - 128.0f) / 127.0f;
    if (v > -0.25f && v < 0.25f) return 0.0f; /* dead zone */
    return v < -1.0f ? -1.0f : v > 1.0f ? 1.0f : v;
}

int main(void) {
    static char error[256], body[1024];
    uint32_t *frame;
    AoeDemo *demo;
    SceCtrlData pad;
    uint32_t previous = 0;
    uint64_t last;

    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    frame = malloc(FB_BYTES);
    if (!frame || !display_init()) {
        sceKernelExitProcess(0);
        return 0;
    }
    text_screen(frame, "LOADING ORIGINAL DATA...",
                "READING " DEMO_ROOT "\n\nDAT, DRS ARCHIVES AND SCENARIOS");
    demo = aoe_demo_open(DEMO_ROOT, clock_us, error, sizeof(error));
    if (!demo) {
        snprintf(body, sizeof(body),
                 "%s\n\nCOPY YOUR OWN GAME FILES:\n"
                 "  " DEMO_ROOT "/DATA   <- AOE DATA FOLDER\n"
                 "  " DEMO_ROOT "/DATA2  <- ROR DATA2 FOLDER\n"
                 "  " DEMO_ROOT "/CAMPAIGN <- ROR CAMPAIGN\n"
                 "\nPRESS START TO EXIT",
                 error);
        text_screen(frame, "GAME DATA NOT FOUND", body);
        do {
            sceCtrlPeekBufferPositive(0, &pad, 1);
            sceDisplayWaitVblankStart();
        } while (!(pad.buttons & SCE_CTRL_START));
        display_free();
        free(frame);
        sceKernelExitProcess(0);
        return 0;
    }
    text_screen(frame, "BUILDING SCENARIO...", aoe_demo_status(demo));
    aoe_demo_load_scenario(demo, 0);
    last = clock_us();
    for (;;) {
        AoeDemoInput in;
        uint32_t buttons, pressed;
        uint64_t now;
        float dt;
        sceCtrlPeekBufferPositive(0, &pad, 1);
        buttons = pad.buttons;
        pressed = buttons & ~previous;
        previous = buttons;
        memset(&in, 0, sizeof(in));
        in.scroll_x = axis(pad.lx);
        in.scroll_y = axis(pad.ly);
        if (buttons & SCE_CTRL_LEFT) in.scroll_x = -1.0f;
        if (buttons & SCE_CTRL_RIGHT) in.scroll_x = 1.0f;
        if (buttons & SCE_CTRL_UP) in.scroll_y = -1.0f;
        if (buttons & SCE_CTRL_DOWN) in.scroll_y = 1.0f;
        if (pressed & SCE_CTRL_LTRIGGER) in.pressed |= AOE_DEMO_BUTTON_PREV;
        if (pressed & SCE_CTRL_RTRIGGER) in.pressed |= AOE_DEMO_BUTTON_NEXT;
        if (pressed & SCE_CTRL_CROSS) in.pressed |= AOE_DEMO_BUTTON_TRAIN;
        if (pressed & SCE_CTRL_SQUARE) in.pressed |= AOE_DEMO_BUTTON_ATTACK;
        if (pressed & SCE_CTRL_CIRCLE) in.pressed |= AOE_DEMO_BUTTON_CAMERA;
        if (pressed & SCE_CTRL_TRIANGLE) in.pressed |= AOE_DEMO_BUTTON_HUD;
        if (pressed & SCE_CTRL_SELECT) in.pressed |= AOE_DEMO_BUTTON_PAUSE;
        if (pressed & SCE_CTRL_START) in.pressed |= AOE_DEMO_BUTTON_QUIT;
        if (in.pressed & (AOE_DEMO_BUTTON_PREV | AOE_DEMO_BUTTON_NEXT))
            text_screen(frame, "BUILDING SCENARIO...", "");
        now = clock_us();
        dt = (float)(now - last) / 1000000.0f;
        last = now;
        if (dt > 0.25f) dt = 0.25f;
        if (!aoe_demo_update(demo, &in, dt)) break;
        aoe_demo_render(demo, buffers[back].base, FB_STRIDE);
        display_flip();
    }
    aoe_demo_close(demo);
    display_free();
    free(frame);
    sceKernelExitProcess(0);
    return 0;
}
