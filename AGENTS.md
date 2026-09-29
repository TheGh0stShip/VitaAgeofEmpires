# Project agent instructions

## Target architecture

Treat the PS Vita ABI as a prerequisite for every target-side change. Vita is
little-endian ARMv7-A (Cortex-A9), 32-bit ARM/Thumb, using ILP32: `int`,
`long`, pointers, and `size_t` are all 32 bits. It is not AArch64.

Configure Vita builds through the VitaSDK toolchain and keep the
`vita_abi_check` configure gate passing. Inspect ELF attributes for the
compiler and linked dependencies; the expected VitaSDK ABI uses ARM EABI,
ARMv7 application profile and VFP-register argument passing. Never infer the
floating-point calling convention merely from VFP/NEON availability.

## Host/target boundaries

Linux and WSL x86-64 normally use LP64; Windows x64 uses LLP64. Host tools and
tests must preserve original 32-bit disk, wire, checksum, and pointer-token
semantics with fixed-width integer types and endian-safe access. Add layout
assertions at serialized boundaries. Do not globally pack structures and do
not truncate real host pointers into emulated 32-bit tokens.

Check ARM alignment and calling-convention assumptions before trusting a host
test. Label defects that only affect host tooling as host-only. A passing host
or emulator test never establishes correctness on physical Vita hardware.

## Reconstruction discipline

Keep base Age of Empires 1.0c and Rise of Rome 1.0a evidence
revision-qualified. External reimplementations may inform architecture and
test questions, but this project is a clean-room reconstruction: do not copy
their source or translate it line-for-line. Original behavior claims require
local executable/data evidence and focused regression tests.
