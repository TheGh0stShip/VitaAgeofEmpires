# Host regression lane for the Vita data model.
#
# Builds the host test suite as 32-bit little-endian ARMv7-A code with the
# hard-float procedure-call standard and runs it under qemu-user. Compared with
# an x86-64 LP64 host this shares Vita's ILP32 sizes, ARM EABI aggregate
# alignment (8-byte double/int64), Thumb-2 code generation and, through the
# flags below, VitaSDK's signed plain char and short (variable-size) enums.
# It is still a Linux host lane: passing here never establishes correctness on
# Vita hardware. See docs/VITA_ABI.md.
#
# Extra flags (for example sanitizers) must be passed through the CFLAGS and
# LDFLAGS environment variables. Setting CMAKE_C_FLAGS or
# CMAKE_EXE_LINKER_FLAGS on the command line replaces the values below; the
# archive check in cmake/VerifyVitaArchives.cmake then fails on enum size.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

set(AOE_ARMHF_TRIPLE arm-linux-gnueabihf)
set(CMAKE_C_COMPILER ${AOE_ARMHF_TRIPLE}-gcc)
set(CMAKE_LIBRARY_ARCHITECTURE ${AOE_ARMHF_TRIPLE})

# These mirror arm-vita-eabi-gcc's configured defaults (--with-arch=armv7-a
# --with-tune=cortex-a9 --with-fpu=neon --with-float=hard --with-mode=thumb),
# plus its bare-metal EABI choices of signed char and small enums, which the
# Linux gnueabihf compiler does not default to.
set(CMAKE_C_FLAGS_INIT
  "-march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard -mthumb -fsigned-char -fshort-enums")

# glibc's startup objects use 32-bit enums. Project code passes no enum values
# to glibc, so the per-executable mismatch warning is not actionable.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--no-enum-size-warning")

find_program(AOE_QEMU_ARM NAMES qemu-arm qemu-arm-static REQUIRED)
set(CMAKE_CROSSCOMPILING_EMULATOR "${AOE_QEMU_ARM}")
