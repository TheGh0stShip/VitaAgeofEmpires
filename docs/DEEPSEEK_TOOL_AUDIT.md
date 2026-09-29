# DeepSeek AoE tool audit

This document records the disposition of the DeepSeek-generated tooling supplied
for the **Age of Empires / Rise of Rome Vita decompilation and port**. It is not
an architecture proposal for `GenieVita`.

The rule used here is the repository's existing one: reconstructed behavior must
be backed by the target executables, original retail data, or a narrowly used
format reference. A plausible generic RTS implementation is not a substitute for
Age of Empires behavior.

## Disposition

| Candidate idea | Decision | Reason / replacement |
|---|---|---|
| bounds-checked C++ `Reader` | Replace | The project is C11 and its existing parsers already do explicit bounds checks. A second byte-reader abstraction would duplicate code without adding evidence. |
| DRS reader | Reject candidate; keep existing | The candidate used the wrong header layout for classic AoE/RoR. `src/drs.c` uses the executable-verified 40-byte Ensemble header, `1.00` at `0x28`, `tribe` at `0x2c`, table records at `0x40`, and the original on-disk four-byte type ordering. A new synthetic fixture now locks this down in CI. |
| SLP decoder | Reject candidate; keep existing | The candidate omitted the 24-byte `2.0N` comment/header area and simplified the draw command encoding. `src/slp.c` already implements frame-info, outline and command-offset tables and the command families needed by the retail assets. A synthetic decode test now exercises normal, transparent, player-color, shadow, greater-draw and outline paths. |
| BMP terrain loader | Reject as terrain model | AoE/RoR terrain in this project is sourced through the DAT/terrain/SLP path already validated against the executable and retail archives. DIB/BMP parsing may be appropriate for other resources, but it must not replace the proven terrain pipeline. |
| JASC palette loader | Keep existing | `src/palette.c` parses the installed 256-color JASC palette used by the target revision. Do not add alternate raw-palette formats until an owned target asset requires them. |
| WAV parser | Adopt, rewritten | `src/wav.c` adds a bounded RIFF/WAVE chunk parser and direct-PCM capability check. `wav_tool` can inspect either a loose WAV or a `" vaw"` resource directly from a DRS archive. |
| guessed fixed-size DAT records | Reject | The candidate 58-byte unit record / fixed civ layout was explicitly speculative. This repository already raw-inflates `VER 3.7` and parses the target's version-specific serialized sections, including units inside civilization data, using executable/retail evidence. A new raw-DEFLATE load regression covers the file boundary. |
| generic `AssetRegistry` / config | Reject for now | Archive precedence and lookup are already reconstructed from the target executable, including Rise of Rome expansion-before-base behavior. A generic registry would hide target-specific semantics we already know. |
| generic 20 Hz fixed-tick simulation | Reject as evidence | No fixed tick rate, ECS model, invented terrain movement costs, or generic job state machine is accepted merely because it is reasonable RTS architecture. Timing and update order stay source-backed in `world_clock`, `world_tick`, timers, command drain, player update, and later reconstructed systems. |
| A* / flow-field pathfinder | Reject as canonical implementation | The candidate itself acknowledged it was not the original pathfinder. Pathfinding remains a decompilation milestone and should be reconstructed from the target rather than chosen by convenience. |
| deterministic RNG discipline | Already present | `aoe_rng` implements the target executable's 32-bit LCG and world-tick code preserves the observed seed/checkpoint ordering. |
| replay + state hash + divergence harness | Keep concept, defer integration | This is useful once the integrated command format and complete simulation state boundary are stable. Hashing partial structs now risks canonizing padding, pointers, or incomplete state. Current source-backed subsystem tests are the safer regression surface. Build a canonical serializer/trace first, then hash that representation. |
| black-box reference traces | Keep concept | Original-game captures are useful for end-to-end validation, but they supplement rather than replace executable-qualified reconstruction. The roadmap already calls for original-game capture validation before a playable release. |

## External format references used for the audit

These are used as **references**, not vendored implementations:

- SFTtech/openage: classic DRS and SLP format documentation.
- sandsmark/genieutils: mature cross-check for DRS/SLP and the version-dependent
  Genie DAT serialization/decompression shape.
- The project's own Ghidra exports and `docs/reversing.md`: authoritative for
  target-specific behavior and archive precedence.

No external implementation was copied into the project.

## Added validation

The audit adds synthetic tests that do not require proprietary game files:

- `drs_format_test`: validates the classic AoE/RoR header, table/index
  offsets, logical lookup, extraction, bad version rejection and out-of-bounds
  entry rejection.
- `slp_decode_test`: decodes a complete synthetic `2.0N` frame using real
  command-table/outline-table structure and multiple command families.
- `dat_load_test`: creates a raw-DEFLATE `VER 3.7` fixture and checks good,
  wrong-version and truncated streams.
- `wav_test`: exercises RIFF chunk traversal, odd-chunk padding, PCM metadata,
  unsupported-but-structurally-valid format tags and malformed chunks.

The existing optional retail-data audits remain the higher-confidence gate.
The repository already records zero decode failures across the installed
base-game SLP archives and expansion graphics/interface frames.

## GenieVita boundary

Nothing in this audit is automatically promoted to `GenieVita`.
A parser/runtime component should move there only after it is shown to be
portable Genie behavior rather than an AoE/RoR-port-specific contract, and
after its API/tests no longer depend on this repository's target revisions.
