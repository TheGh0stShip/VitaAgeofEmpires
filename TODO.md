# Roadmap

The objective is a faithful, native PS Vita port of *Age of Empires I*, with Rise of Rome as the primary target and the base game retained. Do not mark the project complete based on Ghidra export counts, host tests, or a scene-only alpha.

## Immediate: close the executable-coverage audit

- [ ] Reconcile every decoded instruction island in both patched executables with code references, data tables, exception metadata, switch dispatch, and function boundaries.
- [ ] Triage the base stream-decoder region around 0x0051f31f and the initialization region around 0x0051f61c.
- [x] Validate the RoR target at 0x005256df as a seven-byte call/jump thunk to shared code at 0x005256b7.
- [ ] Triage residual RoR instruction region 0x005256e6..0x005259bf and internal labels in the copy-kernel area beginning at 0x00521f2f.
- [ ] Classify unresolved direct/indirect call sites and computed jumps; add Windows live-debug traces for representative gameplay paths when available.
- [ ] Repeat coverage after each recovery and keep base 1.0c and RoR 1.0a address spaces separate.

## Port milestones

- [ ] Complete source-backed reconstruction of unit templates, constructors, state dispatch, ownership/registration, update/destruction, movement, collision, attack, damage, and death behavior.
- [ ] Reconstruct player/world update, AI, pathfinding, resources, building, production, diplomacy, victory, and scenario rules from the patched targets.
- [ ] Finish scene rendering: terrain, borders, visibility/fog, buildings, animated units, effects, selection, and correct draw ordering.
- [ ] Bind in-archive WAV/effect playback to the future Vita audio backend; CD music extraction remains a separate host-side path.
- [ ] Implement Vita controls and UI, save/load, campaign and scenario flows, and music/effects playback using legally supplied local game data.
- [ ] Integrate all runtime systems into the Vita application and produce an installable VPK from the real reconstructed game.
- [ ] Validate against original-game captures and behavior, then run on Vita hardware and emulator.

## Validation/tooling milestones

- [x] Audit the supplied DeepSeek helper-code bundle rather than importing it wholesale.
- [x] Add focused synthetic format regressions for DRS, SLP, raw-DEFLATE DAT, and RIFF/WAV parsing.
- [x] Run binary-parser regressions under AddressSanitizer and UndefinedBehaviorSanitizer in CI.
- [ ] Define a canonical pointer-free command/replay representation once the command surface is reconstructed far enough.
- [ ] Define canonical deterministic simulation-state serialization after all state that can affect future ticks is represented.
- [ ] Add per-step state hashing and first-divergence reports against canonical traces.
- [ ] Capture repeatable original-game reference traces for representative movement, economy, combat, construction, and scenario flows.

## GenieVita extraction

- [ ] Keep VitaAgeofEmpires as the authoritative reconstruction/integration workspace for AoE/RoR.
- [ ] Promote only validated, reusable Genie behavior to the separate private TheGh0stShip/GenieVita repository.
- [ ] Require promoted modules to be platform-neutral, independent of target executable addresses, free of proprietary assets, and covered by synthetic tests.
- [ ] Do not move AoE/RoR-specific game rules, campaign logic, revision glue, or Vita VPK/platform integration into GenieVita.
- [ ] Establish a clear consumer/dependency boundary after the first reusable modules are promoted.

## Repository quality and release discipline

- [ ] Keep the source build and host test suite green; add regression tests for every newly translated binary behavior.
- [ ] Keep parser-focused sanitizer CI green for every binary-format change.
- [ ] Cross-build the integrated runtime with VitaSDK and verify the generated package boots and enters an original scenario.
- [ ] Maintain revision-qualified evidence and repeatable extraction/audit scripts. Never commit original executables, disc images, archives, music, or other proprietary game data.
- [ ] Publish status estimates only when evidence changes; distinguish decompilation coverage from port implementation and playable functionality.

## Translation status (2026-10-06)

- [x] Translate every Ghidra-discovered patched RoR 1.0a game-code function (4,510/4,510 cited; `docs/PORT_COVERAGE.md`). Excludes the hand-written assembly module (0x00524e90.., 32 functions) and the C runtime (0x00525c24.., 1,308 functions).
- [ ] Integrate the translations: replace the callback ("ops") seams with the reconstructed game/world/player/unit/panel objects and run them from one game loop and state machine.
- [ ] Native equivalents for the assembly module (shape rasterizers, copy kernels), audited against the existing sprite/draw code.
- [ ] Vita platform layer for the translated screens: panel/surface rendering, fonts, input mapping, audio (effects, MIDI, user-extracted CD music), settings persistence, string resources from the user's `language.dll`.
- [ ] Saved games and campaign progress on Vita storage; decide the multiplayer transport (or exclude multiplayer).
- [ ] Base AoE 1.0c translation coverage (the worklist counts RoR 1.0a only).

## Current verified baseline (2026-09-29)

- Patched AoE 1.0c: 5,870 Ghidra-discovered functions exported, zero decompilation failures; 31,055 call sites, 4,435 unresolved; 6,185 decoded instruction bytes outside function bodies in 548 islands.
- Patched Rise of Rome 1.0a: 5,850 functions exported, zero failures; 31,652 call sites, 4,537 unresolved; 7,528 decoded instruction bytes outside function bodies in 555 islands.
- Both images have zero undefined bytes in initialized executable sections; this does not prove complete function discovery or resolved dynamic control flow.
- The host reconstruction can compose populated scenes from user-supplied original campaign/DAT/DRS data, but this is not an interactive game.
- The DeepSeek tooling audit was merged: incorrect replacement DRS/SLP/DAT/simulation proposals were rejected, useful WAV/validation work was rewritten to match the existing project, and both normal host CI and parser ASan/UBSan CI pass.
- There is no playable port. A non-playable tech-demo VPK (`docs/VITA_DEMO.md`) runs the reconstructed runtime, world update and renderer on user-supplied data; it is verified in Vita3K only, not yet on hardware.
