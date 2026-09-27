# VitaAgeofEmpires

[![Host build and tests](https://github.com/TheGh0stShip/VitaAgeofEmpires/actions/workflows/host-ci.yml/badge.svg?branch=main)](https://github.com/TheGh0stShip/VitaAgeofEmpires/actions/workflows/host-ci.yml)

**A source-backed reconstruction of Age of Empires I for a future native
PlayStation Vita port.** Rise of Rome is the primary target; the base game is
also supported.

> Research prototype — **not playable yet**. This repository does not contain
> a game, a functional scene viewer, or an installable VPK. Vita builds
> currently produce shared reconstruction libraries, not a complete game.

| | Current state |
|---|---|
| Target | Rise of Rome 1.0a, with base AoE 1.0c retained |
| Implemented | Portable C subsystems for original data formats, terrain/scenario interpretation, selected world/unit/update/rendering behavior |
| Verified | Host build and regression suite; source-backed audits against locally supplied original data |
| Missing | Integrated game loop, complete gameplay, Vita UI/input/audio, save/load, campaign flow, playable VPK |

The status illustration below is a reproducible estimate of the **full port**, not a screenshot of running code and not a decompilation-completion metric.

![Reproducible Rise of Rome port-status illustration](status/port-progress.png)

## Start here

- [Build and test](#build-and-test) — get the host regression suite running.
- [Project map](#project-map) — find runtime code, tests, reverse-engineering evidence, and generated exports.
- [Architecture](docs/ARCHITECTURE.md) — understand how the source, tests, and binary evidence fit together.
- [Documentation guide](docs/README.md) — browse the technical notes by topic.
- [Roadmap](TODO.md) — milestones and current blockers.
- [Resume notes](docs/RESUME.md) — detailed handoff for continuing the work.
- [Contributing](CONTRIBUTING.md) — evidence, implementation, and test expectations.
- [Security policy](SECURITY.md) — reporting security issues.

## Build and test

Requirements: CMake 3.16+, a C11 compiler, zlib development files, and Python
3 (for the status-art rule test).

```sh
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Release
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
```

The host tests use synthetic fixtures unless optional original game files are
present locally. Vita cross-compilation requires
[VitaSDK](https://github.com/vitasdk/vita-toolchain); see
[Building a Vita update](docs/BUILDING_UPDATE.md) for the existing build
configuration and its limits. A successful library build does not mean the
game boots.

## Project map

| Path | Purpose |
|---|---|
| `src/`, `include/` | Portable C reconstruction components and public interfaces |
| `tests/` | Focused host regression tests for reconstructed behavior |
| `tools/` | Asset/data inspection, status-art generation, Ghidra audits and exports |
| `docs/reversing.md` | Detailed behavioral evidence, addresses, and open questions |
| `docs/PATCH_REVISIONS.md` | Exact patched executable revisions, hashes, and coverage caveats |
| `docs/DECOMPILATION_EXPORTS.md` | Contents and regeneration of checked-in Ghidra reference exports |
| `docs/RESUME.md`, `TODO.md` | Handoff notes and project roadmap |
| `status/` | Configurable Wonder construction illustration and reproducible recipe |
| `build/decompile/` | Checked-in pseudocode/reference exports; not original source code |

### Reverse-engineering status

Patched Ghidra exports currently contain 5,870 discovered functions for AoE
1.0c and 5,850 for Rise of Rome 1.0a, with no decompilation failures among
those discovered functions. This is **not complete decompilation**: executable
islands, unresolved indirect calls, computed jumps, and function-discovery gaps
remain. The exports are navigation and analysis artifacts—not compilable
source. See [the revision and coverage report](docs/PATCH_REVISIONS.md) for
the exact counts and limitations.

### Progress illustration

The Wonder scene is generated from a JSON configuration and local game art. It is deliberately separate from the runtime and cannot be used to infer a running game scene. See [status/README.md](status/README.md) for the build, review, and update procedure.

## Game data and legal boundary

Project-authored source code and documentation are licensed under
[GPL-3.0-or-later](LICENSE): you may use, modify, and redistribute them.
When distributing a work covered by the GPL, you must preserve the same
freedoms and provide corresponding source under its terms. This copyleft
choice is not legal advice. The license does not grant rights to Age of Empires
code or assets; generated Ghidra references and status-art images are
explicitly outside the project-authored license grant; see
[LICENSE-NOTICE.md](LICENSE-NOTICE.md) for the scope statement.

This repository does not include original game executables, disc images,
music, archives, or raw extracted assets. You must obtain and supply any
original files yourself; tools may read local files for analysis. Do not
commit proprietary game data or generated extracts.

## Project goal

The long-term goal is a faithful, playable native Vita port, not a diagnostic mock-up. Work should be tied to observed original behavior, carry revision-qualified evidence, and include regression tests. See [the roadmap](TODO.md) for what remains before a playable build is possible.
