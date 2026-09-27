# Roadmap

The objective is a faithful, native PS Vita port of *Age of Empires I*, with
Rise of Rome as the primary target and the base game retained. This roadmap is
deliberately broader than the current implementation. Do not mark the project
complete based on Ghidra export counts, host tests, or a scene-only alpha.

## Immediate: close the executable-coverage audit

- [ ] Reconcile every decoded instruction island in both patched executables
  with code references, data tables, exception metadata, switch dispatch, and
  function boundaries.
- [ ] Triage the base stream-decoder region around `0x0051f31f` and the
  initialization region around `0x0051f61c`; recover only independently
  evidenced entries and shared-tail functions.
- [ ] Validate the RoR entry at `0x005256df` and inspect residual labels in the
  copy-kernel area beginning at `0x00521f2f`.
- [ ] Classify unresolved direct/indirect call sites and computed jumps using
  static evidence; add Windows live-debug traces for representative gameplay
  paths when available.
- [ ] Repeat coverage after each recovery and keep base 1.0c and RoR 1.0a
  address spaces separate.

## Port milestones

- [ ] Complete source-backed reconstruction of unit templates, constructors,
  state dispatch, ownership/registration, update/destruction, movement,
  collision, attack, damage, and death behavior.
- [ ] Reconstruct player/world update, AI, pathfinding, resources, building,
  production, diplomacy, victory, and scenario rules from the patched targets.
- [ ] Finish scene rendering: terrain, borders, visibility/fog, buildings,
  animated units, effects, selection, and correct draw ordering.
- [ ] Implement Vita controls and UI, save/load, campaign and scenario flows,
  and music/effects playback using legally supplied local game data.
- [ ] Integrate all runtime systems into the Vita application and produce an
  installable VPK from the real game implementation (no diagnostic game or
  scripted imitation behavior).
- [ ] Validate against original-game captures and behavior, then run on Vita
  hardware and emulator.

## Repository quality and release discipline

- [ ] Keep the source build and host test suite green; add regression tests for
  every newly translated binary behavior.
- [ ] Cross-build the integrated runtime with VitaSDK and verify the generated
  package boots and enters an original scenario.
- [ ] Maintain revision-qualified evidence and repeatable extraction/audit
  scripts. Never commit original executables, disc images, archives, music, or
  other proprietary game data.
- [ ] Publish status estimates only when evidence changes; distinguish
  decompilation coverage from port implementation and playable functionality.

## Current verified baseline (2026-09-26)

- Patched AoE 1.0c: 5,870 Ghidra-discovered functions exported, zero
  decompilation failures; 31,055 call sites, 4,435 unresolved; 6,185 decoded
  instruction bytes outside function bodies in 548 islands.
- Patched Rise of Rome 1.0a: 5,849 functions exported, zero failures; 31,651
  call sites, 4,537 unresolved; 7,535 decoded instruction bytes outside
  function bodies in 555 islands.
- Both images have zero undefined bytes in initialized executable sections.
  This does not prove complete function discovery or resolved dynamic control
  flow; see [`docs/PATCH_REVISIONS.md`](docs/PATCH_REVISIONS.md).
- There is no playable port or installable VPK. Current Vita builds are shared
  reconstruction libraries; see [`README.md`](README.md) and
  [`docs/RESUME.md`](docs/RESUME.md).
