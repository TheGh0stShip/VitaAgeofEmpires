# Repository architecture

This tree contains portable reconstructions of selected game behaviors, host-side readers/auditors for original data, and a Ghidra reference corpus. They are not yet assembled into a playable Vita game.

## Runtime code

src/ and include/ hold C11 modules built as static libraries by CMake. The implementation currently spans:

- **Original data:** DRS archive access, SLP/palette graphics, DAT and CPX decoding, scenario parsing, and bounded RIFF/WAVE inspection for sound resources.
- **World/scenario:** terrain and border interpretation, scenario/map runtime, visibility surfaces, scenario-editor catalog/runtime, and scene composition.
- **Units and players:** selected unit construction, placement, IDs, registration, state/update hooks, player queues/resources/groups, and source-backed lifecycle behavior.
- **Rendering support:** sprite animation/drawing, tile objects, terrain art, live-unit draw helpers, viewport composition, color tables, and a host-side populated campaign-scene path using original user-supplied data.
- **Selected simulation:** clock/timers, command drain, scenario timeline, action/production behavior, deterministic original RNG behavior, and parts of world/player update.

Each module is intentionally smaller than a complete game subsystem. Tests validate translated contracts, not a full game session.

## Evidence hierarchy

This project is reconstruction-first, not architecture-first. Evidence is weighted in roughly this order:

1. revision-qualified behavior recovered from patched AoE 1.0c / RoR 1.0a executables;
2. repeatable observations from original retail data and scenarios;
3. synthetic regression fixtures that lock down already-understood contracts;
4. mature external Genie format references used as cross-checks;
5. explicitly labeled hypotheses.

Generic RTS conventions are not evidence of Age of Empires behavior. Do not introduce invented tick rates, pathfinding rules, economy constants, entity models, or state transitions simply because they are reasonable designs.

## Tools and validation

tools/ contains host utilities for inspecting data/assets/audio, composing source-backed scenes, rendering status art, and driving Ghidra audits/exports.

The parser regression surface includes synthetic DRS, SLP, raw-DEFLATE DAT, and RIFF/WAV fixtures. A focused CI job runs those binary parsers under AddressSanitizer and UndefinedBehaviorSanitizer.

See [Decompilation exports](DECOMPILATION_EXPORTS.md), [Patch revisions](PATCH_REVISIONS.md), and [Provenance](PROVENANCE.md).

## VitaAgeofEmpires versus GenieVita

VitaAgeofEmpires is the primary evidence and integration repository for the AoE/RoR decompilation and PS Vita port.

GenieVita is a separate reusable engine/runtime repository. Code is promoted there only after this project demonstrates that the behavior is genuinely reusable Genie behavior rather than:

- AoE/RoR game rules;
- executable-revision-specific glue;
- Vita-only platform integration;
- an unvalidated parser assumption; or
- an architectural convenience with no evidence in the target.

~~~text
patched executables + original data
              |
              v
      VitaAgeofEmpires
  reconstruct / test / integrate
              |
     validated generic behavior
              v
          GenieVita
~~~

GenieVita must not become a speculative upstream design that dictates how AoE behaves.

## Replay and deterministic comparison

A replay/state-hash/first-divergence harness remains a high-value future tool, but the canonical serialization boundary is not complete yet. Hashing raw C structs now would risk including pointers, padding, or partial state.

The correct sequence is:

1. reconstruct the integrated command representation;
2. define canonical pointer-free serialization of all state that affects future simulation;
3. record/reference original-game observations where useful;
4. hash or compare the canonical representation per simulation step;
5. report the first divergence and subsystem-level state delta.

Until that boundary exists, source-backed subsystem tests remain the canonical regression mechanism.

## Intended direction

The long-term shape is an integrated native Vita application that loads user-supplied original data, reconstructs a real scenario, and implements the original simulation and UI. The current host-side scene integration is meaningful progress, but it is not a playable port.

## Target and host ABIs

The Vita target is little-endian 32-bit ARMv7-A with an ILP32 ABI and a
hard-float procedure-call standard. Vita configuration compiles an ABI probe,
checks its ELF header/ARM attributes, and checks the resolved zlib dependency
before building project libraries. A target build fails early if it is
AArch64, non-ILP32, big-endian, or mixes floating-point calling conventions.

Host builds intentionally retain their native LP64 or LLP64 ABI. Original
32-bit values at disk, wire, checksum, or emulated pointer-token boundaries
must therefore use explicit-width types and endian-safe access rather than
native `long` or pointers. Host tests and emulator runs remain supporting
evidence only; physical Vita behavior and alignment still require hardware
validation.
