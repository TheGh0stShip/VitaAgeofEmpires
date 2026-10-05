# Related Genie-engine reconstruction research

This project is an independent clean-room reconstruction. Related projects are
used to identify architecture questions, file-format test cases, and likely
subsystem boundaries. Their code is not copied, translated line-for-line, or
treated as evidence of the original executable's behavior. Behavioral claims
here must still be supported by the named AoE 1.0c or Rise of Rome 1.0a binary,
original data, or observed original-game behavior.

## Directly relevant projects

- [openage](https://github.com/SFTtech/openage) is a broad GPLv3+ Genie-engine
  reimplementation. It converts original game assets into an engine-owned data
  model and supports AoE1-family inputs, but deliberately does not promise
  binary or network compatibility. Useful lesson: keep asset conversion,
  simulation state, rendering, and presentation as explicit layers. Its
  event-driven simulation is a design reference, not evidence that AoE1 used
  the same internal model.
- [Age of Empires Free Software Remake](https://github.com/folkertvanverseveld/aoe)
  explicitly targets the original Age of Empires and Rise of Rome and reports
  basic multiplayer gameplay. It is the closest known peer for coverage
  comparison. Because it also reconstructs behavior from disassembly, its
  implementation is excluded as a source for this project's clean-room code.
  Public status and subsystem names can instead help reveal missing research
  areas such as AI, networking, UI, and platform abstraction.
- [genieutils](https://github.com/sandsmark/genieutils) and
  [genie-rs](https://github.com/SiegeEngineers/genie-rs) read and write Genie
  family formats. They are useful independent format cross-checks, especially
  for malformed-input and round-trip test ideas. This project retains its own
  parsers and verifies AoE1/RoR revision-specific layouts locally.

## Primary historical references

- Ensemble's [1500 Archers on a 28.8: Network Programming in Age of Empires
  and Beyond](https://www.gamedevs.org/uploads/1500-archers-age-of-empires-network-programming.pdf)
  describes the original games' deterministic command simulation and
  single-threaded game-loop constraints. It supports prioritizing fixed-width,
  deterministic state transitions before attempting multiplayer.
- The contemporary [Age of Empires architecture postmortem](https://media.gdcvault.com/GD_Mag_Archives/GDM_March_1998.pdf)
  describes the separation of the reusable Genie engine from the Tribe game
  layer and the performance pressure on the 2D renderer. It motivates keeping
  low-level art/viewport work separate from game rules and profiling on the
  actual Vita rather than extrapolating from a desktop host.
- [VitaSDK's CMake toolchain](https://github.com/vitasdk/vita-toolchain/blob/master/cmake_toolchain/vita.toolchain.cmake)
  declares the target processor as ARMv7-A and selects `arm-vita-eabi` tools.
  Local compiler ELF attributes are authoritative for floating-point PCS;
  CPU VFP/NEON support alone does not establish that calling convention.

## Pocket PC editions

Locally supplied Pocket PC releases (Gold Edition 1.07 and 1.25, published
by ZIO and credited as based on Ensemble's Genie code) are ARM builds of the
same engine lineage. They are a cross-revision research reference, never
evidence for PC behaviour. See [`POCKETPC.md`](POCKETPC.md).

## Upstream survey (refreshed 2026-10-03)

License and last-push data come from the GitHub API on 2026-10-03. "Use"
states the only way each project may affect this repository.

| Project | License | Last push | Use here |
|---|---|---|---|
| [SFTtech/openage](https://github.com/SFTtech/openage) | GPLv3+ (GitHub reports NOASSERTION) | 2026-10-03, active | Architecture and layering questions only |
| [folkertvanverseveld/aoe](https://github.com/folkertvanverseveld/aoe) | Apache-2.0 | 2026-09-22, active | Subsystem checklist only; disassembly-derived, so excluded as a code or behavior source |
| [sandsmark/freeaoe](https://github.com/sandsmark/freeaoe) | GPL-3.0 | 2022-12-08 | Architecture reference; AoE2-focused |
| [ChariotEngine/Chariot](https://github.com/ChariotEngine/Chariot) | not detected | 2018-04-17, inactive | Historical AoE1 reference only |
| [Tapsa/genieutils](https://github.com/Tapsa/genieutils) (active fork of [sandsmark/genieutils](https://github.com/sandsmark/genieutils)) | LGPL-3.0 | 2026-07-08 | Host-only, out-of-process format oracle |
| [SiegeEngineers/genie-rs](https://github.com/SiegeEngineers/genie-rs) | GPL-3.0 | 2023-04-04 | Host-only format oracle; mainly AoC layouts |
| [SDL](https://github.com/libsdl-org/SDL) with [Vita port](https://github.com/libsdl-org/SDL/blob/main/docs/README-vita.md) | zlib | active | Candidate platform layer; SDL2 and SDL3 already ship with the local VitaSDK |
| [Ghidra BSim](https://ghidra.re/ghidra_docs/GhidraClass/BSim/README.html) | Apache-2.0 (Ghidra) | bundled with Ghidra 12 | Cross-revision function-candidate search |

## Work-reduction plan

Each item removes repeated effort without weakening the evidence rule.

1. **One platform backend for host and Vita.** VitaSDK already provides
   SDL2/SDL3 (zlib license, GPL-compatible), with Vita controller, touch and
   audio support. A single SDL-backed video/audio/input layer runs the same
   reconstructed runtime on a desktop host and on Vita, replacing two
   hand-written platform layers. Choose SDL2 or SDL3 only after measuring
   on real Vita hardware.
2. **Cross-revision candidates through BSim.** Mapping disc RoR, patched
   RoR 1.0a and patched AoE 1.0c functions has so far relied on manual or
   trigram matching (`docs/ror_revision_map.tsv`). BSim's decompiler feature
   vectors can rank candidates in bulk. A BSim hit is only a candidate:
   acceptance still needs the existing semantic or disassembly comparison.
   The opcode-5 work showed the cost of skipping this step: older player
   resource notes had silently carried disc addresses.
3. **Differential format oracles.** genieutils (LGPL-3.0) or genie-rs
   (GPL-3.0) may run as separate host processes against locally supplied
   data, so their DAT/DRS/SLP readings can be compared with ours. They are
   never linked into the port or copied. A disagreement is a research lead;
   the original executable decides.
4. **Peer subsystem checklists.** The folkertvanverseveld/aoe and openage
   subsystem lists (AI, networking, UI, scenario flow) go into `TODO.md` as
   coverage questions, never as behavior.
5. **Shared in-tree primitives.** `include/le_bytes.h` gives one
   endian- and alignment-safe codec for original 32-bit records, and
   `command_dispatch` translates the RoR 1.0a router `0x0042a180`, so each
   remaining opcode only needs a handler and a binding. Private `le32`
   copies in the older parsers (`cpx.c`, `dat_tech.c`, `drs.c`) can move to
   it when those files are next touched.

## Consequences for this port

1. Preserve original 32-bit identifiers, serialization, command ordering, and
   RNG semantics at host/target boundaries.
2. Keep format parsing, persistent simulation, rendering, UI, and platform
   services separable enough to test without inventing gameplay substitutes.
3. Use peer projects to build a missing-subsystem checklist, never to claim
   behavior parity. Revision-qualified local evidence remains the acceptance
   criterion.
4. Treat deterministic simulation and real-hardware performance as independent
   gates. Host and emulator success cannot prove physical Vita correctness.

Research refreshed 2026-10-03.
