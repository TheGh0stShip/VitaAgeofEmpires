# Checked-in decompilation exports

This repository includes the current Ghidra pseudocode/reference exports for
the patched AoE 1.0c and Rise of Rome 1.0a executables, plus selected final
coverage reports. They are derived analysis artifacts, not original source
code and not a claim that all functions or control-flow edges have been
discovered. The exact executable revisions and hashes are recorded in
[`PATCH_REVISIONS.md`](PATCH_REVISIONS.md).

For each revision, the checked-in artifacts are:

- `*_functions.txt`: Ghidra decompilation text for every discovered function.
- `*_index.tsv`: entry address, function name, export status, and text offset.
- `*_calls.tsv`: call-site inventory, including unresolved/indirect sites.
- `*_exec_coverage.tsv` and `*_exec_gaps.tsv`: final executable-byte summary
  and decoded instruction islands outside recognized function bodies.
- `*_exec_pointers_after_islands.tsv`, `*_pointer_runs_after_islands.tsv`,
  and `*_embedded_exec_pointers.tsv`: final initialized-data, pointer-run,
  and executable-section pointer evidence.
- `*_island_recovery.tsv`, `*_pointer_recovery.tsv`,
  `*_exec_pointer_recovery.tsv`, and `*_interior_pointer_recovery.tsv`:
  audited recovery decisions. RoR also has `ror_10a_copy_kernel_recovery.tsv`;
  base also has `base_10c_embedded_target_recovery.tsv`.

Intermediate pre-recovery pointer sweeps are omitted to avoid storing
redundant multi-megabyte TSVs. Regenerate them as part of the full import/audit
pipeline when needed.

## Regeneration

Install Ghidra (the current exports were generated with Ghidra 12.1.3), 7-Zip,
and the host build tools. Supply a local copy of Microsoft's combined
`AOEPATCH.exe`; neither it nor the extracted game executables are in this
repository:

```sh
GHIDRA=/path/to/ghidra \
  tools/import_patch_revisions.sh /path/to/AOEPATCH.exe
```

The script verifies exact SHA-256 hashes before import, runs automatic
analysis and evidence-based recovery, refreshes the reports and decompilation
exports, and verifies the disc-to-patched revision map. Ghidra databases stay
under ignored `build/ghidra/`. Some output may vary across Ghidra releases;
review changes to the checked-in reference corpus before committing an
updated export.

The host build and test suite does not require the game executable or Ghidra.
It validates the portable reconstruction components only, not completeness of
these decompilations.
