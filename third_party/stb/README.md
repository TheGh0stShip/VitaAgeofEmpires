# stb_truetype

`stb_truetype.h` v1.19 by Sean Barrett, copied from the VitaSDK
distribution so the host and Vita builds rasterize fonts identically. The
VitaSDK copy maps `STBTT_memcpy` to `sceClibMemcpy`; that one line is
restored to the upstream `memcpy` so the host can build it too.
Dual licensed MIT / public domain (see the end of the header). It renders
the user-supplied Windows fonts (Arial, Copperplate Gothic Light) that
the original game asked GDI for; no font file is distributed here.
