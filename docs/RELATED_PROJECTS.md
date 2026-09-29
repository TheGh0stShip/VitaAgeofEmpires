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

Research refreshed 2026-09-29.
