#include "demo_app.h"   /* the 5x7 text for the data-missing screen */
#include "shell_app.h"

#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/power.h>
#include <psp2/touch.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* PS Vita platform layer for the port: the translated game's screens
   through the shell (shell_app.h) on CDRAM display buffers, with the
   front touch panel, the buttons and the left stick as its input. */

int _newlib_heap_size_user = 128 * 1024 * 1024;
unsigned int sceUserMainThreadStackSize = 1024 * 1024;

#define DATA_ROOT "ux0:data/VitaAoE"

enum {
    SCREEN_W = 960,
    SCREEN_H = 544,
    FB_STRIDE = 960,
    FB_BYTES = FB_STRIDE * SCREEN_H * 4,
    FB_BLOCK = (FB_BYTES + 0x3ffff) & ~0x3ffff,
    BUFFER_COUNT = 3
};

static struct {
    SceUID block;
    void *base;
} buffers[BUFFER_COUNT];
static int back;

static uint32_t clock_ms(void *context) {
    (void)context;
    return (uint32_t)(sceKernelGetProcessTimeWide() / 1000u);
}

static int display_init(void) {
    int i;
    for (i = 0; i < BUFFER_COUNT; ++i) {
        buffers[i].block = sceKernelAllocMemBlock("aoe_fb", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
                                                  FB_BLOCK, NULL);
        if (buffers[i].block < 0 || sceKernelGetMemBlockBase(buffers[i].block, &buffers[i].base) < 0)
            return 0;
        memset(buffers[i].base, 0, FB_BYTES);
    }
    return 1;
}

static void display_free(void) {
    int i;
    for (i = 0; i < BUFFER_COUNT; ++i)
        if (buffers[i].block >= 0) sceKernelFreeMemBlock(buffers[i].block);
}

static uint32_t *display_next(void) {
    back = (back + 1) % BUFFER_COUNT;
    return buffers[back].base;
}

static void display_show(void) {
    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof fb);
    fb.size = sizeof fb;
    fb.base = buffers[back].base;
    fb.pitch = FB_STRIDE;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = SCREEN_W;
    fb.height = SCREEN_H;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
}

static void message_screen(const char *title, const char *body) {
    uint32_t *fb = display_next();
    size_t i;
    for (i = 0; i < (size_t)FB_STRIDE * SCREEN_H; ++i) fb[i] = 0xff201810u;
    aoe_demo_draw_text(fb, FB_STRIDE, 40, 40, 3, 0xff40d0ffu, "AGE OF EMPIRES: RISE OF ROME");
    aoe_demo_draw_text(fb, FB_STRIDE, 40, 110, 2, 0xff80ffffu, title);
    aoe_demo_draw_text(fb, FB_STRIDE, 40, 150, 2, 0xffffffffu, body);
    display_show();
}

static float axis(uint8_t v) {
    float f = ((float)v - 128.0f) / 127.0f;
    if (f > -0.25f && f < 0.25f) return 0.0f;
    return f < -1.0f ? -1.0f : f > 1.0f ? 1.0f : f;
}

int main(void) {
    static char error[256], body[1200];
    AoeShell *shell;
    SceCtrlData pad;
    SceTouchData touch;

    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    if (!display_init()) {
        sceKernelExitProcess(0);
        return 0;
    }
    message_screen("LOADING...", "READING " DATA_ROOT);
    shell = aoe_shell_open(DATA_ROOT, SCREEN_W, SCREEN_H, clock_ms, NULL, error, sizeof error);
    if (!shell) {
        snprintf(body, sizeof body,
                 "%s\n\nCOPY YOUR OWN GAME FILES TO " DATA_ROOT ":\n"
                 "  DATA/      <- AGE OF EMPIRES DATA FOLDER\n"
                 "  DATA2/     <- RISE OF ROME DATA2 FOLDER\n"
                 "  CAMPAIGN/  <- RISE OF ROME CAMPAIGN FOLDER\n"
                 "  LANGUAGEX.DLL, LANGUAGE.DLL <- FROM THE GAME FOLDERS\n"
                 "  FONTS/     <- ARIAL.TTF, ARIALBD.TTF (+ COPRGTL.TTF)\n"
                 "\nPRESS START TO EXIT",
                 error);
        message_screen("GAME DATA NOT FOUND", body);
        do {
            sceCtrlPeekBufferPositive(0, &pad, 1);
            sceDisplayWaitVblankStart();
        } while (!(pad.buttons & SCE_CTRL_START));
        display_free();
        sceKernelExitProcess(0);
        return 0;
    }
    for (;;) {
        AoeShellInput in;
        memset(&in, 0, sizeof in);
        sceCtrlPeekBufferPositive(0, &pad, 1);
        if (pad.buttons & SCE_CTRL_CROSS) in.buttons |= AOE_SHELL_CROSS;
        if (pad.buttons & SCE_CTRL_SQUARE) in.buttons |= AOE_SHELL_SQUARE;
        if (pad.buttons & SCE_CTRL_CIRCLE) in.buttons |= AOE_SHELL_CIRCLE;
        if (pad.buttons & SCE_CTRL_TRIANGLE) in.buttons |= AOE_SHELL_TRIANGLE;
        if (pad.buttons & SCE_CTRL_LTRIGGER) in.buttons |= AOE_SHELL_L;
        if (pad.buttons & SCE_CTRL_RTRIGGER) in.buttons |= AOE_SHELL_R;
        if (pad.buttons & SCE_CTRL_START) in.buttons |= AOE_SHELL_START;
        in.stick_x = axis(pad.lx);
        in.stick_y = axis(pad.ly);
        if (pad.buttons & SCE_CTRL_LEFT) in.stick_x = -1.0f;
        if (pad.buttons & SCE_CTRL_RIGHT) in.stick_x = 1.0f;
        if (pad.buttons & SCE_CTRL_UP) in.stick_y = -1.0f;
        if (pad.buttons & SCE_CTRL_DOWN) in.stick_y = 1.0f;
        memset(&touch, 0, sizeof touch);
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
        if (touch.reportNum > 0) {
            /* The front panel reports 1920 x 1088 for the 960 x 544 screen. */
            in.touching = 1;
            in.touch_x = touch.report[0].x / 2;
            in.touch_y = touch.report[0].y / 2;
        }
        if (!aoe_shell_frame(shell, &in, display_next(), FB_STRIDE)) break;
        display_show();
    }
    aoe_shell_close(shell);
    display_free();
    sceKernelExitProcess(0);
    return 0;
}
