#include "game_startup_init.h"

#include <stddef.h>

/* GUID bytes as stored in EMPIRESX.EXE's data section. */
const uint8_t aoe_dxcheck_dplay_class_ror[16] = {      /* 0x0054b640 */
    0x20, 0x6d, 0xeb, 0xd1, 0x23, 0x89, 0xd0, 0x11,
    0x9d, 0x97, 0x00, 0xa0, 0xc9, 0x0a, 0x43, 0xcb};
const uint8_t aoe_dxcheck_dplay_iface_ror[16] = {      /* 0x0054b600 */
    0x40, 0xfe, 0x3e, 0x13, 0xdc, 0x32, 0xd0, 0x11,
    0x9c, 0xfb, 0x00, 0xa0, 0xc9, 0x0a, 0x43, 0xcb};
const uint8_t aoe_dxcheck_ddraw2_iface_ror[16] = {     /* 0x0054aaa0 */
    0xe0, 0xf3, 0xa6, 0xb3, 0x43, 0x2b, 0xcf, 0x11,
    0xa2, 0xde, 0x00, 0xaa, 0x00, 0xb9, 0x33, 0x56};
const uint8_t aoe_dxcheck_surface3_iface_ror[16] = {   /* 0x0054aae0 */
    0x00, 0x4e, 0x04, 0xda, 0xb2, 0x69, 0xd0, 0x11,
    0xa1, 0xd5, 0x00, 0xaa, 0x00, 0xb8, 0xdf, 0xbb};

/* The strings it passes (data section addresses). */
static const char k_dinput_dll[] = "DINPUT.DLL";                                  /* 0x00557590 */
static const char k_no_dinput[] = "Couldn't LoadLibrary DInput\r\n";              /* 0x00557570 */
static const char k_dinput_create[] = "DirectInputCreateA";                       /* 0x0055755c */
static const char k_no_dinput_create[] = "Couldn't GetProcAddress DInputCreate\r\n"; /* 0x00557534 */
static const char k_ddraw_dll[] = "DDRAW.DLL";                                    /* 0x00557528 */
static const char k_ddraw_create[] = "DirectDrawCreate";                          /* 0x00557514 */
static const char k_no_ddraw[] = "Couldn't LoadLibrary DDraw\r\n";                /* 0x005574f4 */
static const char k_no_ddraw_create[] = "Couldn't create DDraw\r\n";              /* 0x005574dc */
static const char k_no_ddraw2[] = "Couldn't QI DDraw2\r\n";                       /* 0x005574c4 */
static const char k_no_coop[] = "Couldn't Set coop level\r\n";                    /* 0x005574a8 */
static const char k_no_surface[] = "Couldn't CreateSurface\r\n";                  /* 0x0055748c */

/* 0x0043d0ad / 0x0043d354: neg; sbb; neg; add 0x500. */
static uint32_t dx5_version(const AoeDirectXCheckOpsRor *ops) {
    return 0x500u + (ops->dplay_version_ok(ops->context) != 0);
}

/* 0x0043cfc4..0x0043d0e2: Windows NT. */
static void nt_version(uint32_t *version, int32_t *platform, uint32_t major,
                       const AoeDirectXCheckOpsRor *ops) {
    void *dinput, *dplay = NULL;
    int32_t co;
    *platform = 2;
    if (major < 4) {        /* cmp ecx,4; jae (version untouched) */
        *platform = 0;
        return;
    }
    if (major != 4) {
        *version = 0x501;
        return;
    }
    *version = 0x200;
    dinput = ops->load_library(ops->context, k_dinput_dll);
    if (!dinput) {
        ops->debug_string(ops->context, k_no_dinput);
        return;
    }
    {
        void *create = ops->get_proc(ops->context, dinput, k_dinput_create);
        ops->free_library(ops->context, dinput);
        if (!create) {
            ops->debug_string(ops->context, k_no_dinput_create);
            return;
        }
    }
    *version = 0x300;
    co = ops->co_initialize(ops->context);
    /* CoCreateInstance(0x0054b640, NULL, 1, 0x0054b600, &frame+0x1c). */
    if (ops->co_create_instance(ops->context, aoe_dxcheck_dplay_class_ror, NULL, 1,
                                aoe_dxcheck_dplay_iface_ror, &dplay) < 0) {
        /* 0x0043d080: uninitialized only when CoInitialize gave nonzero. */
        if (co != 0) ops->co_uninitialize(ops->context);
        return;
    }
    ops->release(ops->context, dplay);
    if (co != 0) ops->co_uninitialize(ops->context);
    *version = dx5_version(ops);
}

/* 0x0043d0e3..0x0043d387: not NT. */
static void ddraw_version(uint32_t *version, int32_t *platform, uint32_t build,
                          const AoeDirectXCheckOpsRor *ops) {
    void *ddraw_dll, *create, *dinput, *dinput_create;
    void *ddraw = NULL, *ddraw2 = NULL, *surface = NULL, *surface3 = NULL;
    AoeSurfaceRequestRor desc;
    *platform = 1;
    if ((build & 0xffffu) > 0x549u) {
        *version = 0x501;
        return;
    }
    ddraw_dll = ops->load_library(ops->context, k_ddraw_dll);
    if (!ddraw_dll) {
        *version = 0;
        *platform = 0;
        ops->free_library(ops->context, NULL);   /* 0x0043d135: push edi (0) */
        return;
    }
    create = ops->get_proc(ops->context, ddraw_dll, k_ddraw_create);
    if (!create) {
        *version = 0;
        *platform = 0;
        ops->free_library(ops->context, ddraw_dll);
        ops->debug_string(ops->context, k_no_ddraw);
        return;
    }
    if (ops->direct_draw_create(ops->context, create, NULL, &ddraw, NULL) < 0) {
        *version = 0;
        *platform = 0;
        ops->free_library(ops->context, ddraw_dll);
        ops->debug_string(ops->context, k_no_ddraw_create);
        return;
    }
    *version = 0x100;
    if (ops->query_interface(ops->context, ddraw, aoe_dxcheck_ddraw2_iface_ror, &ddraw2) < 0) {
        ops->release(ops->context, ddraw);
        ops->free_library(ops->context, ddraw_dll);
        ops->debug_string(ops->context, k_no_ddraw2);
        return;
    }
    ops->release(ops->context, ddraw2);
    *version = 0x200;
    dinput = ops->load_library(ops->context, k_dinput_dll);
    if (!dinput) {
        ops->debug_string(ops->context, k_no_dinput);
        ops->release(ops->context, ddraw);
        ops->free_library(ops->context, ddraw_dll);
        return;
    }
    dinput_create = ops->get_proc(ops->context, dinput, k_dinput_create);
    ops->free_library(ops->context, dinput);
    if (!dinput_create) {
        ops->free_library(ops->context, ddraw_dll);
        ops->release(ops->context, ddraw);
        ops->debug_string(ops->context, k_no_dinput_create);
        return;
    }
    *version = 0x300;
    desc.size = 0x6c;
    desc.flags = 1;
    desc.caps = 0x200;
    /* SetCooperativeLevel(NULL, 8). */
    if (ops->set_cooperative_level(ops->context, ddraw, NULL, 8) < 0) {
        ops->release(ops->context, ddraw);
        ops->free_library(ops->context, ddraw_dll);
        *version = 0;
        ops->debug_string(ops->context, k_no_coop);
        return;
    }
    if (ops->create_surface(ops->context, ddraw, &desc, &surface, NULL) < 0) {
        ops->release(ops->context, ddraw);
        ops->free_library(ops->context, ddraw_dll);
        *version = 0;
        ops->debug_string(ops->context, k_no_surface);
        return;
    }
    /* 0x0043d33d: the surface's QueryInterface into frame +0x20; on
       success the version is set and frame +0x14 (the surface itself) is
       released. The +0x20 interface is never released, nor the surface
       when the query fails. */
    if (ops->query_interface(ops->context, surface, aoe_dxcheck_surface3_iface_ror, &surface3) >= 0) {
        *version = dx5_version(ops);
        ops->release(ops->context, surface);
    }
    ops->release(ops->context, ddraw);
    ops->free_library(ops->context, ddraw_dll);
}

/* 0x0043cf60 */
void aoe_directx_version_ror(uint32_t *version, int32_t *platform,
                             const AoeDirectXCheckOpsRor *ops) {
    AoeOsVersionRor os = {0, 0, 0};
    if (!ops->get_version(ops->context, &os)) {
        *version = 0;
        *platform = 0;
        return;
    }
    if (os.platform == 2) {
        nt_version(version, platform, os.major, ops);
    } else {
        ddraw_version(version, platform, os.build, ops);
    }
}
