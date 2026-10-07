#ifndef VITA_AOE_GAME_STARTUP_INIT_H
#define VITA_AOE_GAME_STARTUP_INIT_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a DirectX version check, 0x0043cf60(version,
   platform) (cdecl, two int out-pointers). The start-up chain (0x4166b0)
   calls it at 0x41696f unless 0x41bfe0("NODXCHECK") (string 0x5547b4) is
   set, and refuses a version below 0x501 (unsigned) with error 0x14 (see
   game_startup.h, AoeStartupChainOpsRor.directx_version).

   It touches no game object; its only state is its own frame:
   - an OSVERSIONINFOA at frame +0x90 (size 0x94 written at +0x00); it
     reads +0x04 the major version, +0x0c the build number (low word) and
     +0x10 the platform (2 Windows NT);
   - frame +0x10 the DirectDraw object, +0x18 its DirectDraw2 interface,
     +0x14 the primary surface, +0x20 the surface's QueryInterface result,
     +0x1c the DirectPlay object (NT path);
   - frame +0x24 a 0x6c-byte DDSURFACEDESC (0x1b dwords zeroed, then +0x00
     size 0x6c, +0x04 flags 1, +0x68 caps 0x200).

   Results. `platform`: 0 unknown (GetVersionExA failed, NT below 4, or
   DDRAW.DLL / DirectDrawCreate missing or failing), 1 not NT, 2 NT.
   `version`: 0 none, 0x100 DirectDraw, 0x200 DirectDraw2 (NT 4 starts
   here), 0x300 DirectInput, 0x500 / 0x501 the final test passed and
   0x0043d390 (the dplayx.dll file version, aoe_dplay_version_ok_ror)
   answered no / yes; NT above 4 and
   a non-NT build above 0x549 are 0x501 at once. NT below 4 writes only
   the platform (0) and leaves `version` as the caller had it.

   The Windows and COM calls are ops. Handles and interfaces stay opaque
   host pointers (never 32-bit tokens). A COM call's HRESULT is an
   int32_t and fails when negative. */

/* The OSVERSIONINFOA fields 0x0043cf60 reads (semantic storage; the
   structure's offsets noted). */
typedef struct {
    uint32_t major;     /* +0x04 dwMajorVersion */
    uint32_t build;     /* +0x0c dwBuildNumber (its low word is compared) */
    uint32_t platform;  /* +0x10 dwPlatformId: 2 NT */
} AoeOsVersionRor;

/* The DDSURFACEDESC passed to CreateSurface: every other byte of its
   0x6c is zero. */
typedef struct {
    uint32_t size;  /* +0x00: 0x6c */
    uint32_t flags; /* +0x04: 1 (the caps field is valid) */
    uint32_t caps;  /* +0x68: 0x200 (the primary surface) */
} AoeSurfaceRequestRor;

/* The GUIDs it passes, as the executable stores them (16 bytes, the
   first three fields little-endian). */
extern const uint8_t aoe_dxcheck_dplay_class_ror[16];    /* 0x0054b640 */
extern const uint8_t aoe_dxcheck_dplay_iface_ror[16];    /* 0x0054b600 */
extern const uint8_t aoe_dxcheck_ddraw2_iface_ror[16];   /* 0x0054aaa0 */
extern const uint8_t aoe_dxcheck_surface3_iface_ror[16]; /* 0x0054aae0 */

typedef struct {
    void *context;
    /* GetVersionExA (import 0x005420f0): 0 on failure. */
    int (*get_version)(void *context, AoeOsVersionRor *out);
    /* LoadLibraryA (0x00542224): NULL when missing. */
    void *(*load_library)(void *context, const char *name);
    /* GetProcAddress (0x005420f8): NULL when missing. */
    void *(*get_proc)(void *context, void *module, const char *name);
    /* FreeLibrary (0x00542228); called with NULL once (see the .c). */
    void (*free_library)(void *context, void *module);
    /* OutputDebugStringA (0x005420c8). */
    void (*debug_string)(void *context, const char *text);
    /* CoInitialize(NULL) (0x005423e8), CoCreateInstance (0x005423ec),
       CoUninitialize (0x005423f0). */
    int32_t (*co_initialize)(void *context);
    int32_t (*co_create_instance)(void *context, const uint8_t *clsid, void *outer,
                                  uint32_t cls_context, const uint8_t *iid, void **out);
    void (*co_uninitialize)(void *context);
    /* The DirectDrawCreate entry `proc`, called (NULL, &ddraw, NULL). */
    int32_t (*direct_draw_create)(void *context, void *proc, void *guid, void **ddraw,
                                  void *outer);
    /* Vtable calls: +0x00 QueryInterface and +0x08 Release (result
       unused) on any interface; IDirectDraw +0x50 SetCooperativeLevel
       (window, flags) and +0x18 CreateSurface(desc, &surface, outer). */
    int32_t (*query_interface)(void *context, void *object, const uint8_t *iid, void **out);
    void (*release)(void *context, void *object);
    int32_t (*set_cooperative_level)(void *context, void *ddraw, void *window, uint32_t flags);
    int32_t (*create_surface)(void *context, void *ddraw, const AoeSurfaceRequestRor *desc,
                              void **surface, void *outer);
    /* 0x0043d390: dplayx.dll is new enough (nonzero). */
    int (*dplay_version_ok)(void *context);
} AoeDirectXCheckOpsRor;

/* 0x0043cf60: see above. */
void aoe_directx_version_ror(uint32_t *version, int32_t *platform,
                             const AoeDirectXCheckOpsRor *ops);

#endif
