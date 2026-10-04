# Vita ABI facts and target-model test lanes

The Vita is a little-endian ARMv7-A Cortex-A9 using ILP32. Host tests on
x86-64 (LP64) or Windows x64 (LLP64) do not exercise that data model. This
page records the ABI properties verified from the pinned VitaSDK toolchain
and the two CI lanes that check them. Neither lane establishes correctness
on Vita hardware; that still needs a hardware run.

## Verified VitaSDK properties

Checked on 2026-10-04 with `arm-vita-eabi-gcc` 15.2.0 from
`vitasdk/vitasdk@sha256:fd82d88ff456d118eef16533ebf9a3a6ae0e292684fb8a3a35f15f0707b4692d`.
Facts come from `_Static_assert` probes, predefined macros (`-dM -E`), the
compiler's configure line (`-v`) and `readelf -A` on built project objects.

| Property | VitaSDK | Linux armhf (default) | x86-64 Linux | Windows x64 |
| --- | --- | --- | --- | --- |
| `int` / `long` / pointer / `size_t` | 32 / 32 / 32 / 32 | 32 / 32 / 32 / 32 | 32 / 64 / 64 / 64 | 32 / 32 / 64 / 64 |
| Plain `char` | **signed** | unsigned | signed | signed |
| Enum size | **small** (`-fshort-enums` behavior) | `int` | `int` | `int` |
| `long double` | 64-bit (same as `double`) | 64-bit | 80-bit x87 | 64-bit (MSVC) |
| `double`/`int64_t` alignment in aggregates | 8 | 8 | 8 | 8 |
| `wchar_t` | 4 bytes, unsigned | 4 bytes, unsigned | 4 bytes, signed | 2 bytes |
| Default instruction set | Thumb-2 | Thumb-2 | n/a | n/a |
| FP / SIMD | VFPv3 + NEON, hard-float args | VFPv3-D16, hard-float args | SSE2 | SSE2 |

Consequences for portable reconstruction code:

- VitaSDK's signed `char` and small enums differ from the standard Linux ARM
  EABI. Never use plain `char` for byte values whose sign matters, and never
  use an enum type as a field in a serialized or layout-asserted structure.
  Use fixed-width integer fields there.
- `sizeof` an enum is 1, 2 or 4 on Vita depending on its enumerator range
  (for example `enum {A, B}` is 1 byte, an enum containing `0x10000` is 4).
- `long double` arithmetic is ordinary `double` arithmetic on Vita. Code that
  passes on x86-64 only because of 80-bit intermediates will diverge on the
  target. The original executables' x87 precision-control setting is a
  separate question that this does not answer.
- `Tag_CPU_unaligned_access: v6` lets GCC emit unaligned single loads and
  stores, but ARMv7 multi-word and VFP/NEON loads still require alignment.
  Read serialized data through the byte helpers or `memcpy`, not by casting
  buffer pointers to wider types.

## CI lanes

`.github/workflows/host-ci.yml` runs both lanes on every push and pull request.

### VitaSDK cross-build (`vita-cross`)

Configures with the VitaSDK toolchain, which runs the configure-time ABI gate
(`cmake/VerifyVitaAbi.cmake`: compiler probe plus zlib), builds everything,
then runs `cmake/VerifyVitaArchives.cmake`. That script extracts every object
from the build's static archives and requires ELF32, little-endian, ARM,
`Tag_CPU_arch: v7`, application profile, VFP-register arguments and small
enums. Mixing objects or libraries built with different enum sizes would
otherwise only produce linker warnings.

Reproduce locally with Docker:

```sh
docker run --rm -v "$PWD":/src -w /src \
  vitasdk/vitasdk@sha256:fd82d88ff456d118eef16533ebf9a3a6ae0e292684fb8a3a35f15f0707b4692d \
  sh -ec 'cmake -S . -B build/vita -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
          cmake --build build/vita --parallel
          cmake -DAOE_ABI_DIR=build/vita -DAOE_ABI_READELF=arm-vita-eabi-readelf -DAOE_ABI_AR=arm-vita-eabi-ar -P cmake/VerifyVitaArchives.cmake'
```

### ARMv7 hard-float host lane (`armv7-hardfloat`)

Builds the complete host test suite with `arm-linux-gnueabihf-gcc` using
`cmake/toolchains/armv7-linux-gnueabihf.cmake` and runs it under
`qemu-arm`, in Release and in Debug with UBSan. The toolchain mirrors
VitaSDK's configured defaults (`-march=armv7-a -mtune=cortex-a9 -mfpu=neon
-mfloat-abi=hard -mthumb`) and adds `-fsigned-char -fshort-enums` so that
the lane matches the table above rather than the Linux ARM defaults. The
same archive check then confirms the flags reached every object.

It exercises 32-bit `long`/pointer/`size_t`, EABI alignment, 64-bit
`long double`, Thumb-2 code generation and UBSan on a 32-bit `long`. It does
not reproduce Vita's libc (newlib/SceLibc), memory map, alignment-fault
configuration, or the hardware FPU. Alignment-fault behavior under qemu-user
is not evidence of Vita behavior.

On Ubuntu 24.04 or WSL, with armhf multiarch enabled (see the workflow for
the apt source setup):

```sh
sudo apt-get install gcc-arm-linux-gnueabihf qemu-user zlib1g-dev:armhf libubsan1:armhf
cmake -S . -B build/armhf -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/armv7-linux-gnueabihf.cmake
cmake --build build/armhf --parallel
cmake -DAOE_ABI_DIR=build/armhf -DAOE_ABI_READELF=arm-linux-gnueabihf-readelf \
  -DAOE_ABI_AR=arm-linux-gnueabihf-ar -P cmake/VerifyVitaArchives.cmake
ctest --test-dir build/armhf --output-on-failure
```

Pass extra compiler or linker flags through the `CFLAGS` and `LDFLAGS`
environment variables, for example
`CFLAGS="-fsanitize=undefined -fno-sanitize-recover=undefined"
LDFLAGS=-fsanitize=undefined`. Setting `CMAKE_C_FLAGS` on the command line
replaces the toolchain's ABI flags, and the archive check then fails.

## Findings from the first runs

- `main` at `6a27805`: all 74 host tests pass in both lanes; 73 objects in
  69 archives pass the archive check for both VitaSDK and armhf.
- The pre-existing x86-64 suite never exercised signed `char`/small enums
  together with ILP32; no current `main` code stores an enum in a serialized
  structure (the only enums are edition selectors passed as arguments).
