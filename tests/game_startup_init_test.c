/* Patched RoR 1.0a DirectX version check (0x0043cf60). Evidence from
   EMPIRESX.EXE (objdump -d --start-address=0x43cf60 --stop-address=0x43d390):
   - GetVersionExA failing stores 0 to both outputs (0x43cf99..0x43cfa9).
   - dwPlatformId (frame +0xa0) 2: platform 2; major (+0x94) below 4
     stores platform 0 only (0x43cfdd), above 4 version 0x501 (0x43d0d5),
     4: 0x200, DINPUT.DLL and DirectInputCreateA (FreeLibrary right after
     GetProcAddress), 0x300, CoInitialize, CoCreateInstance(0x54b640, 0,
     1, 0x54b600, &p), Release (+8), CoUninitialize only for a nonzero
     CoInitialize result (cmp esi,edi; je), then 0x500 + (0x43d390 != 0).
   - Otherwise platform 1; build (+0x9c) & 0xffff above 0x549: 0x501.
     DDRAW.DLL missing: 0/0 and FreeLibrary(0) (0x43d135). DirectDrawCreate
     missing or failing: 0/0, FreeLibrary, the message. 0x100 and the QI
     for 0x54aaa0 (Release the DirectDraw object, FreeLibrary, message on
     failure); Release the result, 0x200, DINPUT as above with the
     clean-up orders of 0x43d234 and 0x43d272; 0x300; SetCooperativeLevel
     (+0x50) (0, 8); CreateSurface (+0x18) of a 0x6c descriptor with
     flags 1 and caps 0x200 (0x43d295..0x43d2bc); both failures store
     version 0. The surface's QI for 0x54aae0: success sets the version
     and releases frame +0x14 (the surface, 0x43d366); then Release the
     DirectDraw object and FreeLibrary.
   The strings are read from the executable's data section (0x55748c..
   0x557590). */
#include "game_startup_init.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int ddraw_dll, dinput_dll, ddraw_proc, dinput_proc;
static int ddraw, ddraw2, surface, surface3, dplay;

typedef struct {
    int version_ok;
    AoeOsVersionRor os;
    int no_ddraw_dll, no_ddraw_proc, no_dinput_dll, no_dinput_proc;
    int32_t co_init, co_create, dd_create, qi_ddraw2, coop, create, qi_surface3;
    int dplay_ok;
    /* What the calls received. */
    char log[1024];
    const char *debug;
    uint32_t cls_context, coop_flags;
    void *coop_window;
    AoeSurfaceRequestRor desc;
    int guids_ok;
} Fake;

static void say(Fake *f, const char *s) {
    strcat(f->log, s);
    strcat(f->log, " ");
}

static const char *name_of(void *p) {
    if (!p) return "null";
    if (p == &ddraw_dll) return "ddraw_dll";
    if (p == &dinput_dll) return "dinput_dll";
    if (p == &ddraw) return "ddraw";
    if (p == &ddraw2) return "ddraw2";
    if (p == &surface) return "surface";
    if (p == &surface3) return "surface3";
    if (p == &dplay) return "dplay";
    return "?";
}

static int get_version(void *c, AoeOsVersionRor *out) {
    Fake *f = c;
    say(f, "ver");
    if (!f->version_ok) return 0;
    *out = f->os;
    return 1;
}
static void *load_library(void *c, const char *name) {
    Fake *f = c;
    if (strcmp(name, "DDRAW.DLL") == 0) {
        say(f, "load:ddraw");
        return f->no_ddraw_dll ? NULL : &ddraw_dll;
    }
    assert(strcmp(name, "DINPUT.DLL") == 0);
    say(f, "load:dinput");
    return f->no_dinput_dll ? NULL : &dinput_dll;
}
static void *get_proc(void *c, void *module, const char *name) {
    Fake *f = c;
    if (module == &ddraw_dll) {
        assert(strcmp(name, "DirectDrawCreate") == 0);
        say(f, "proc:ddraw");
        return f->no_ddraw_proc ? NULL : &ddraw_proc;
    }
    assert(module == &dinput_dll && strcmp(name, "DirectInputCreateA") == 0);
    say(f, "proc:dinput");
    return f->no_dinput_proc ? NULL : &dinput_proc;
}
static void free_library(void *c, void *module) {
    Fake *f = c;
    char s[32];
    snprintf(s, sizeof s, "free:%s", name_of(module));
    say(f, s);
}
static void debug_string(void *c, const char *text) {
    Fake *f = c;
    say(f, "dbg");
    f->debug = text;
}
static int32_t co_initialize(void *c) {
    Fake *f = c;
    say(f, "coinit");
    return f->co_init;
}
static int32_t co_create_instance(void *c, const uint8_t *clsid, void *outer, uint32_t ctx,
                                  const uint8_t *iid, void **out) {
    Fake *f = c;
    say(f, "cocreate");
    f->cls_context = ctx;
    f->guids_ok = clsid[0] == 0x20 && clsid[15] == 0xcb && iid[0] == 0x40 && iid[3] == 0x13 &&
                  outer == NULL;
    if (f->co_create < 0) return f->co_create;
    *out = &dplay;
    return 0;
}
static void co_uninitialize(void *c) { say(c, "couninit"); }
static int32_t direct_draw_create(void *c, void *proc, void *guid, void **out, void *outer) {
    Fake *f = c;
    assert(proc == &ddraw_proc && guid == NULL && outer == NULL);
    say(f, "ddcreate");
    if (f->dd_create < 0) return f->dd_create;
    *out = &ddraw;
    return 0;
}
static int32_t query_interface(void *c, void *object, const uint8_t *iid, void **out) {
    Fake *f = c;
    if (object == &ddraw) {
        assert(iid == aoe_dxcheck_ddraw2_iface_ror && iid[0] == 0xe0 && iid[15] == 0x56);
        say(f, "qi:ddraw2");
        if (f->qi_ddraw2 < 0) return f->qi_ddraw2;
        *out = &ddraw2;
        return 0;
    }
    assert(object == &surface && iid == aoe_dxcheck_surface3_iface_ror && iid[3] == 0xda);
    say(f, "qi:surface3");
    if (f->qi_surface3 < 0) return f->qi_surface3;
    *out = &surface3;
    return 0;
}
static void release(void *c, void *object) {
    Fake *f = c;
    char s[32];
    snprintf(s, sizeof s, "rel:%s", name_of(object));
    say(f, s);
}
static int32_t set_coop(void *c, void *dd, void *window, uint32_t flags) {
    Fake *f = c;
    assert(dd == &ddraw);
    say(f, "coop");
    f->coop_window = window;
    f->coop_flags = flags;
    return f->coop;
}
static int32_t create_surface(void *c, void *dd, const AoeSurfaceRequestRor *desc, void **out,
                              void *outer) {
    Fake *f = c;
    assert(dd == &ddraw && outer == NULL);
    say(f, "surface");
    f->desc = *desc;
    if (f->create < 0) return f->create;
    *out = &surface;
    return 0;
}
static int dplay_version_ok(void *c) {
    Fake *f = c;
    say(f, "dplayver");
    return f->dplay_ok;
}

static Fake fake;
static AoeDirectXCheckOpsRor ops = {
    &fake, get_version, load_library, get_proc, free_library, debug_string,
    co_initialize, co_create_instance, co_uninitialize, direct_draw_create,
    query_interface, release, set_coop, create_surface, dplay_version_ok};

#define E_FAIL_ ((int32_t)0x80004005u)

static void reset(uint32_t platform_id, uint32_t major, uint32_t build) {
    memset(&fake, 0, sizeof fake);
    fake.version_ok = 1;
    fake.os.platform = platform_id;
    fake.os.major = major;
    fake.os.build = build;
}

static void run(uint32_t *v, int32_t *p) {
    *v = 0xdead;
    *p = -7;
    aoe_directx_version_ror(v, p, &ops);
}

static void nt_tests(void) {
    uint32_t v;
    int32_t p;

    reset(2, 4, 1381);
    fake.version_ok = 0;
    run(&v, &p);
    assert(v == 0 && p == 0 && strcmp(fake.log, "ver ") == 0);

    /* NT 3.51: only the platform is written (0x43cfdd). */
    reset(2, 3, 1057);
    run(&v, &p);
    assert(v == 0xdead && p == 0 && strcmp(fake.log, "ver ") == 0);

    reset(2, 5, 2195);
    run(&v, &p);
    assert(v == 0x501 && p == 2 && strcmp(fake.log, "ver ") == 0);

    reset(2, 4, 1381);
    fake.no_dinput_dll = 1;
    run(&v, &p);
    assert(v == 0x200 && p == 2);
    assert(strcmp(fake.log, "ver load:dinput dbg ") == 0);
    assert(strcmp(fake.debug, "Couldn't LoadLibrary DInput\r\n") == 0);

    reset(2, 4, 1381);
    fake.no_dinput_proc = 1;
    run(&v, &p);
    assert(v == 0x200 && strcmp(fake.log, "ver load:dinput proc:dinput free:dinput_dll dbg ") == 0);
    assert(strcmp(fake.debug, "Couldn't GetProcAddress DInputCreate\r\n") == 0);

    /* CoCreateInstance failing with CoInitialize S_OK: no CoUninitialize. */
    reset(2, 4, 1381);
    fake.co_create = E_FAIL_;
    run(&v, &p);
    assert(v == 0x300 && p == 2);
    assert(strcmp(fake.log, "ver load:dinput proc:dinput free:dinput_dll coinit cocreate ") == 0);
    assert(fake.cls_context == 1 && fake.guids_ok);

    /* ... and with a nonzero CoInitialize (S_FALSE 1): CoUninitialize. */
    reset(2, 4, 1381);
    fake.co_create = E_FAIL_;
    fake.co_init = 1;
    run(&v, &p);
    assert(v == 0x300);
    assert(strcmp(fake.log, "ver load:dinput proc:dinput free:dinput_dll coinit cocreate couninit ") == 0);

    reset(2, 4, 1381);
    fake.dplay_ok = 1;
    run(&v, &p);
    assert(v == 0x501 && p == 2);
    assert(strcmp(fake.log, "ver load:dinput proc:dinput free:dinput_dll coinit cocreate rel:dplay "
                            "dplayver ") == 0);

    reset(2, 4, 1381);
    fake.co_init = 1;
    run(&v, &p);
    assert(v == 0x500);
    assert(strcmp(fake.log, "ver load:dinput proc:dinput free:dinput_dll coinit cocreate rel:dplay "
                            "couninit dplayver ") == 0);
}

static void ddraw_tests(void) {
    uint32_t v;
    int32_t p;
    static const char head[] = "ver load:ddraw proc:ddraw ddcreate qi:ddraw2 rel:ddraw2 load:dinput "
                               "proc:dinput free:dinput_dll ";
    char want[512];

    /* Build 0x54a (above 0x549): 0x501 at once; only the low word counts. */
    reset(1, 4, 0x054a);
    run(&v, &p);
    assert(v == 0x501 && p == 1 && strcmp(fake.log, "ver ") == 0);
    reset(0, 3, 0x04000549u);
    fake.no_ddraw_dll = 1;
    run(&v, &p);
    assert(v == 0 && p == 0 && strcmp(fake.log, "ver load:ddraw free:null ") == 0);
    assert(fake.debug == NULL);

    reset(1, 4, 950);
    fake.no_ddraw_proc = 1;
    run(&v, &p);
    assert(v == 0 && p == 0 && strcmp(fake.log, "ver load:ddraw proc:ddraw free:ddraw_dll dbg ") == 0);
    assert(strcmp(fake.debug, "Couldn't LoadLibrary DDraw\r\n") == 0);

    reset(1, 4, 950);
    fake.dd_create = E_FAIL_;
    run(&v, &p);
    assert(v == 0 && p == 0);
    assert(strcmp(fake.log, "ver load:ddraw proc:ddraw ddcreate free:ddraw_dll dbg ") == 0);
    assert(strcmp(fake.debug, "Couldn't create DDraw\r\n") == 0);

    reset(1, 4, 950);
    fake.qi_ddraw2 = E_FAIL_;
    run(&v, &p);
    assert(v == 0x100 && p == 1);
    assert(strcmp(fake.log, "ver load:ddraw proc:ddraw ddcreate qi:ddraw2 rel:ddraw free:ddraw_dll dbg ") == 0);
    assert(strcmp(fake.debug, "Couldn't QI DDraw2\r\n") == 0);

    /* 0x43d234: the message, then Release, then FreeLibrary. */
    reset(1, 4, 950);
    fake.no_dinput_dll = 1;
    run(&v, &p);
    assert(v == 0x200 && p == 1);
    assert(strcmp(fake.log, "ver load:ddraw proc:ddraw ddcreate qi:ddraw2 rel:ddraw2 load:dinput dbg "
                            "rel:ddraw free:ddraw_dll ") == 0);
    assert(strcmp(fake.debug, "Couldn't LoadLibrary DInput\r\n") == 0);

    /* 0x43d272: FreeLibrary, then Release, then the message. */
    reset(1, 4, 950);
    fake.no_dinput_proc = 1;
    run(&v, &p);
    assert(v == 0x200);
    assert(strcmp(fake.log, "ver load:ddraw proc:ddraw ddcreate qi:ddraw2 rel:ddraw2 load:dinput "
                            "proc:dinput free:dinput_dll free:ddraw_dll rel:ddraw dbg ") == 0);
    assert(strcmp(fake.debug, "Couldn't GetProcAddress DInputCreate\r\n") == 0);

    /* SetCooperativeLevel(NULL, 8) failing: version 0, platform stays 1. */
    reset(1, 4, 950);
    fake.coop = E_FAIL_;
    run(&v, &p);
    assert(v == 0 && p == 1);
    assert(fake.coop_window == NULL && fake.coop_flags == 8);
    snprintf(want, sizeof want, "%scoop rel:ddraw free:ddraw_dll dbg ", head);
    assert(strcmp(fake.log, want) == 0);
    assert(strcmp(fake.debug, "Couldn't Set coop level\r\n") == 0);

    reset(1, 4, 950);
    fake.create = E_FAIL_;
    run(&v, &p);
    assert(v == 0 && p == 1);
    assert(fake.desc.size == 0x6c && fake.desc.flags == 1 && fake.desc.caps == 0x200);
    snprintf(want, sizeof want, "%scoop surface rel:ddraw free:ddraw_dll dbg ", head);
    assert(strcmp(fake.log, want) == 0);
    assert(strcmp(fake.debug, "Couldn't CreateSurface\r\n") == 0);

    /* The surface's QI failing: 0x300 stays, the surface is not released. */
    reset(1, 4, 950);
    fake.qi_surface3 = E_FAIL_;
    run(&v, &p);
    assert(v == 0x300 && p == 1 && fake.debug == NULL);
    snprintf(want, sizeof want, "%scoop surface qi:surface3 rel:ddraw free:ddraw_dll ", head);
    assert(strcmp(fake.log, want) == 0);

    /* Success: the surface (not the queried interface) is released. */
    reset(1, 4, 950);
    fake.dplay_ok = 7;
    run(&v, &p);
    assert(v == 0x501 && p == 1 && fake.debug == NULL);
    snprintf(want, sizeof want,
             "%scoop surface qi:surface3 dplayver rel:surface rel:ddraw free:ddraw_dll ", head);
    assert(strcmp(fake.log, want) == 0);

    reset(1, 4, 950);
    run(&v, &p);
    assert(v == 0x500 && p == 1);
}

static void guid_tests(void) {
    /* 16 bytes each at 0x54b640, 0x54b600, 0x54aaa0, 0x54aae0. */
    static const uint8_t dplay_class[16] = {0x20, 0x6d, 0xeb, 0xd1, 0x23, 0x89, 0xd0, 0x11,
                                            0x9d, 0x97, 0x00, 0xa0, 0xc9, 0x0a, 0x43, 0xcb};
    static const uint8_t surface3_iface[16] = {0x00, 0x4e, 0x04, 0xda, 0xb2, 0x69, 0xd0, 0x11,
                                               0xa1, 0xd5, 0x00, 0xaa, 0x00, 0xb8, 0xdf, 0xbb};
    assert(memcmp(aoe_dxcheck_dplay_class_ror, dplay_class, 16) == 0);
    assert(memcmp(aoe_dxcheck_surface3_iface_ror, surface3_iface, 16) == 0);
    assert(aoe_dxcheck_dplay_iface_ror[0] == 0x40 && aoe_dxcheck_dplay_iface_ror[15] == 0xcb);
    assert(aoe_dxcheck_ddraw2_iface_ror[0] == 0xe0 && aoe_dxcheck_ddraw2_iface_ror[15] == 0x56);
}

int main(void) {
    nt_tests();
    ddraw_tests();
    guid_tests();
    puts("game_startup_init: ok");
    return 0;
}
