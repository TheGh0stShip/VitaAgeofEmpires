# Microsoft patch revisions

`AOEPATCH.exe` is Microsoft's combined December 1999 update installer, SHA-256
`b0d15f0ef1b1505054f2f1e343c0d1261ba7fb74033f1483b8b3f56631251f1a`.
Its embedded CAB contains complete replacement executables, not binary-delta
instructions:

| Target | Product version | Size | SHA-256 |
|---|---:|---:|---|
| `EMPIRES.EXE` | AoE 1.0c | 1,470,464 | `f1d71a53389fe866ca5379a4d03046721fa091a741812f491b7d99be72abb8c8` |
| `EMPIRESX.EXE` | Rise of Rome 1.0a | 1,503,232 | `948cd653c05f5a9557bae1584b77360d43864cb8559e8dc08a7861a1f1d8e3fb` |

The expansion update is officially named **1.0a**, despite sharing the
installer with base-game 1.0c. Keep revision-qualified addresses in reversing
notes because both patched images differ in layout from the currently indexed
release binaries.

Run `tools/import_patch_revisions.sh` to verify the installer and payload
hashes, reproducibly extract them under `build/patch_1_0c`, import/analyze them
as the `AoE10c` and `AoERoR10a` Ghidra projects, audit pointer runs and every
aligned executable pointer in initialized data, recover missing or merged
function starts only when table/instruction evidence supports them, repeat the
audits, and regenerate revision-specific call graphs plus whole-program
decompiler reference exports under `build/decompile`.

The authoritative patched dumps and selected final audit TSVs are checked in
at the `base_10c_` and `ror_10a_` paths; Ghidra project databases and original
executables are not. See [`DECOMPILATION_EXPORTS.md`](DECOMPILATION_EXPORTS.md)
for the precise artifact list and regeneration instructions.

This recovery step is mandatory. The initial patched RoR analysis omitted,
among other methods, the scenario-editor vtable's large input handler at
`0x00493580`; a zero-failure export only proved that every function Ghidra had
already discovered decompiled successfully.

The current patched reference exports (full Ghidra auto-analysis, refreshed
2026-09-26) contain:

| Revision | Discovered/decompiled functions | Call sites | Unresolved call sites |
|---|---:|---:|---:|
| AoE 1.0c | 5,870 / 5,870 | 31,055 | 4,435 |
| Rise of Rome 1.0a | 5,849 / 5,849 | 31,651 | 4,537 |

The earlier pointer-run recovery added 950 base and 736 expansion starts.
After full automatic analysis, the broader pointer scan recovered three more
expansion starts from nearby table evidence. Its post-recovery results are:

| Revision | Unique executable-pointer targets | Known function entries | Interior references | Unresolved missing targets |
|---|---:|---:|---:|---:|
| AoE 1.0c | 2,462 | 2,419 | 42 | 1 |
| Rise of Rome 1.0a | 2,478 | 2,437 | 39 | 2 |

This scan deliberately excludes `.rsrc`: its UTF-16 `ICON` data previously
produced a false code-looking target. Raw inspection classified the remaining
three targets (one base, two expansion) as non-entry data or a mid-instruction
reference, rather than functions. There remain 42/39 interior pointers and
4,435/4,537 unresolved indirect call sites. More importantly, code with no
data-pointer evidence and computed jumps are not proven complete by these
counts. They describe a verified subset of function discovery, not a complete
decompilation of either game.

The executable-byte coverage audit is available as
`build/decompile/base_10c_exec_coverage.tsv` / `base_10c_exec_gaps.tsv` and
the corresponding `ror_10a_` files. Both patched images have zero undefined
bytes in initialized executable blocks. Base: 1,286,144 executable bytes
(1,215,443 instruction, 70,701 defined data), with 6,185 decoded instruction
bytes outside Ghidra function bodies. RoR: 1,314,816 executable bytes
(1,244,843 instruction, 69,973 defined data), with 7,535 instruction bytes
outside function bodies. Merging adjacent
unowned instructions yields 548 base and 555 RoR islands after recovering
fifteen and seventeen manually verified function starts, respectively. The TSV
preserves each island's incoming-reference and control-flow-exit evidence.
These islands require semantic/boundary triage; full byte classification is
not proof that every instruction is a function or every indirect edge is
resolved. The repeatable pipeline emits before/after island coverage, recovery
records, pointer audits, and final exports. The embedded executable-pointer
scan of base and RoR identified switch-table case labels that are control-flow
blocks, plus base's six-entry stream-decoder/initialization table at
`0x0051eed4..0x0051eeec`; six exact table-referenced entries were recovered in
the base image. The RoR scan found and recovered seven copy-kernel entries.
Neither executable-pointer report should be interpreted as a list of
functions: computed-jump cases and intra-function labels are retained as
triage evidence. The report now marks entries within 256 bytes of a statically
identified computed-jump table base for focused review; this is an adjacency
hint, not a function classification. An embedded-executable-pointer pass also
found RoR copy-kernel tables inside `.text`; those seven return-
bounded exact targets were recovered as function entries. The scan exposes
jump-table labels that are case blocks, not function starts, reinforcing that
pointer targets require control-flow triage.

Revision addresses must never be substituted by proximity. For example,
disc RoR live root vtable `0x0054ce80` corresponds to patched 1.0a
`0x00547da8`; patched `0x0054ce80` is RTTI data. The verified mappings used
while moving translated systems onto the patched primary target are recorded
in `docs/ror_revision_map.tsv`. Current live-update mappings were located by
mnemonic-trigram comparison and then accepted only after decompilation and
vtable/constructor semantics agreed: disc `0x004a80e0` → patched
`0x004a6920` for vtable `+0x08`, and disc `0x004aa790` → patched
`0x004a8f40` for vtable `+0x24`. `tools/verify_ror_revision_map.sh`
hash-gates both executables, verifies these slots on root and type-90
vtables, and confirms the patched functions remain in the successful Ghidra
export. The patch import pipeline runs this verifier automatically.

The first cross-revision draw audit maps release RoR delta fixup `0x004a6970`
to 1.0a `0x004a51c0` with an exact mnemonic signature, composite dispatch
`0x004a7110` to `0x004a5920`, lower graphic dispatch `0x004a6dd0` to
`0x004a55e0`, and live-unit draw `0x004a8a60` to `0x004a7260`. Manual body
comparison confirms the recovered delta self-reference, layer/coordinate,
angle, selected-color-source, and live draw rules are retained in 1.0a.

The patch readme identifies behavior that must be reflected in the portable
code where applicable: idle-villager cycling, Shang/Phoenician balance
changes, tribute/build-placement/conversion/farm/train validation fixes,
multiplayer cheat restrictions, anti-tamper synchronization, launch fixes,
and the Palmyran full-tech-tree correction. The patched executables are the
authority for implementation details; this list is only an investigation
index.
