# Documentation guide

| Document | Use it for |
|---|---|
| [Architecture](ARCHITECTURE.md) | What is implemented, where code lives, and how the repository fits together |
| [Related projects](RELATED_PROJECTS.md) | Clean-room survey of Genie/AoE reconstructions and lessons used here |
| [Resume notes](RESUME.md) | Current handoff, verified recent work, and the most useful next actions |
| [Roadmap](../TODO.md) | Project-level milestones and known blockers |
| [Reversing notebook](reversing.md) | Detailed behavioral evidence, addresses, hypotheses, and outstanding dependencies |
| [Patch revisions](PATCH_REVISIONS.md) | Exact target versions and hashes, Ghidra coverage measurements, and their limits |
| [Decompilation exports](DECOMPILATION_EXPORTS.md) | What the checked-in generated exports contain and how to regenerate them |
| [Vita build update](BUILDING_UPDATE.md) | Current Vita build configuration and one detailed reconstructed subsystem |
| [Training action](TRAIN_ACTION.md) | Evidence and implementation notes for the training action lifecycle |
| [DeepSeek tool audit](DEEPSEEK_TOOL_AUDIT.md) | Audit disposition, corrected format assumptions, and regression coverage for the supplied helper code |
| [Vita ABI and test lanes](VITA_ABI.md) | Verified VitaSDK data model (signed char, small enums, 64-bit long double) and the ARMv7/VitaSDK CI lanes |
| [Wonder status art](../status/README.md) | Reproducible configuration and rendering of the progress illustration |
| [License](../LICENSE) and [scope notice](../LICENSE-NOTICE.md) | GPL-3.0-or-later terms for project-authored code and documentation, with explicit third-party exclusions |

## Evidence conventions

Addresses are always tied to a named executable revision. A translated body
can still have missing callers, callbacks, or integration paths; consult its
local notes before interpreting a passing unit test as end-to-end behavior.
Ghidra exports are generated navigation artifacts and do not establish that
all code, functions, or indirect control-flow edges have been discovered.
