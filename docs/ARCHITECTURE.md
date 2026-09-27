# Repository architecture

This tree contains three related things: portable reconstructions of selected
game behaviors, host-side readers/auditors for original data, and a Ghidra
reference corpus. They are not yet assembled into a playable Vita game.

## Runtime code

`src/` and `include/` hold C11 modules built as static libraries by CMake.
Most modules are platform-independent and can be compiled and tested on a host.
The implementation currently spans:

- **Original data:** DRS archive access, SLP/palette graphics, DAT and CPX
  decoding, and scenario parsing.
- **World/scenario:** terrain and border interpretation, scenario/map runtime,
  visibility surfaces, scenario-editor catalog/runtime, and scene composition.
- **Units and players:** selected unit construction, placement, IDs,
  registration, state/update hooks, player queues/resources/groups, and
  source-backed lifecycle behavior.
- **Rendering support:** sprite animation/drawing, tile objects, terrain art,
  live-unit draw helpers, viewport composition, and color tables.
- **Selected simulation:** clock/timers, command drain, scenario timeline,
  action/production behavior, and parts of world/player update.

Each module is intentionally smaller than a game subsystem: many callbacks,
virtual methods, integration paths, and downstream behaviors are still
unimplemented. Tests validate the specific translated contracts, not a full
game session. Follow dependencies in `CMakeLists.txt` and evidence references
in `docs/reversing.md` before treating any module as complete.

## Tools and evidence

`tools/` contains host utilities for inspecting data and assets, rendering the
status illustration, and driving Ghidra audits/exports. Ghidra scripts use
revision-specific projects and addresses. The checked-in files in
`build/decompile/` are generated pseudocode and audit reports for the patched
executables; they are reference material, not source code and not proof that
all functions or control flow were found. See
[`DECOMPILATION_EXPORTS.md`](DECOMPILATION_EXPORTS.md) and
[`PATCH_REVISIONS.md`](PATCH_REVISIONS.md).

## Inputs and outputs

Original game executables and data are user-supplied local inputs and are not
tracked. `assets/`, `build/`, game archives, extracted audio, executables, and
Vita packages are ignored. Host CI builds the portable modules and runs the
synthetic regression suite; it does not require or redistribute game data.

## Intended direction

The long-term shape is an integrated native Vita application that loads
user-supplied original data, reconstructs a real scenario, and implements the
original simulation and UI. Until those systems are connected, do not describe
the existing libraries, audits, or status artwork as a playable port.
