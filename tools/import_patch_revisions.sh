#!/usr/bin/env bash
set -euo pipefail

project_root=$(cd "$(dirname "$0")/.." && pwd)
installer=${1:-"$project_root/AOEPATCH.exe"}
ghidra_root=${GHIDRA:-/mnt/d/Projects/PSP-GoExplore-NA/tools/ghidra_12.1.3_PUBLIC}
patch_dir="$project_root/build/patch_1_0c"
decompile_dir="$project_root/build/decompile"
ghidra_dir="$project_root/build/ghidra"

test "$(sha256sum "$installer" | awk '{print $1}')" = \
  b0d15f0ef1b1505054f2f1e343c0d1261ba7fb74033f1483b8b3f56631251f1a
mkdir -p "$patch_dir" "$decompile_dir" "$ghidra_dir"
7z x -y "$installer" -o"$patch_dir" >/dev/null
test "$(sha256sum "$patch_dir/EMPIRES.EXE" | awk '{print $1}')" = \
  f1d71a53389fe866ca5379a4d03046721fa091a741812f491b7d99be72abb8c8
test "$(sha256sum "$patch_dir/EMPIRESX.EXE" | awk '{print $1}')" = \
  948cd653c05f5a9557bae1584b77360d43864cb8559e8dc08a7861a1f1d8e3fb

analyze="$ghidra_root/support/analyzeHeadless"
for specification in \
  "AoE10c|EMPIRES.EXE|base_10c" \
  "AoERoR10a|EMPIRESX.EXE|ror_10a"
do
  IFS='|' read -r project binary stem <<<"$specification"
  case "$stem" in
    base_10c)
      island_starts=(00417d90 0041cfd0 00425580 00427fb0 00428380 \
        00438af0 0043cc40 00483bd0 0051f182 0051f31f 0051f409 \
        0051f544 0051f593 0051f61e 00522b40)
      ;;
    ror_10a)
      island_starts=(00418080 0041d640 00425b20 00428540 00428900 \
        00436e10 0043af60 00483bc0 00521e68 00521e6f 00521e7c \
        00521e8f 00521f2f 00521fcf 00522072 00522112 005256df \
        0052d780)
      ;;
    *)
      echo "no executable-island candidate list for $stem" >&2
      exit 1
      ;;
  esac
  if test -f "$ghidra_dir/$project.gpr"; then
    input=(-process "$binary")
  else
    input=(-import "$patch_dir/$binary")
  fi
  "$analyze" "$ghidra_dir" "$project" "${input[@]}" \
    -scriptPath "$project_root/tools" \
    -postScript AuditAoEFunctionPointerRuns.java \
      "$decompile_dir/${stem}_pointer_runs.tsv" \
    -postScript RecoverAoEPointerRunFunctions.java \
      "$decompile_dir/${stem}_pointer_runs.tsv" \
      "$decompile_dir/${stem}_pointer_recovery.tsv" \
    -postScript AuditAoEFunctionPointerRuns.java \
      "$decompile_dir/${stem}_pointer_runs_after.tsv" \
    -postScript AuditAoEExecutablePointers.java \
      "$decompile_dir/${stem}_exec_pointers.tsv" \
    -postScript RecoverAoEExecutablePointerFunctions.java \
      "$decompile_dir/${stem}_exec_pointers.tsv" \
      "$decompile_dir/${stem}_exec_pointer_recovery.tsv" \
    -postScript AuditAoEExecutablePointers.java \
      "$decompile_dir/${stem}_exec_pointers_after.tsv" \
    -postScript RecoverAoEInteriorPointerFunctions.java \
      "$decompile_dir/${stem}_exec_pointers_after.tsv" \
      "$decompile_dir/${stem}_interior_pointer_recovery.tsv" \
    -postScript AuditAoEExecutablePointers.java \
      "$decompile_dir/${stem}_exec_pointers_after_interior.tsv" \
    -postScript AuditAoEExecutableCoverage.java \
      "$decompile_dir/${stem}_exec_coverage_before_islands.tsv" \
      "$decompile_dir/${stem}_exec_gaps_before_islands.tsv" \
    -postScript RecoverAoEExecutableIslandFunctions.java \
      "$decompile_dir/${stem}_island_recovery.tsv" \
      "${island_starts[@]}" \
    -postScript AuditAoEFunctionPointerRuns.java \
      "$decompile_dir/${stem}_pointer_runs_after_islands.tsv" \
    -postScript AuditAoEExecutablePointers.java \
      "$decompile_dir/${stem}_exec_pointers_after_islands.tsv" \
    -postScript AuditAoEEmbeddedExecutablePointers.java \
      "$decompile_dir/${stem}_embedded_exec_pointers.tsv" \
    -postScript AuditAoEExecutableCoverage.java \
      "$decompile_dir/${stem}_exec_coverage.tsv" \
      "$decompile_dir/${stem}_exec_gaps.tsv" \
    -postScript ExportAoECallGraph.java "$decompile_dir/${stem}_calls.tsv" \
    -postScript ExportAoEDecompilation.java \
      "$decompile_dir/${stem}_functions.txt" \
      "$decompile_dir/${stem}_index.tsv"
done

"$project_root/tools/verify_ror_revision_map.sh"
