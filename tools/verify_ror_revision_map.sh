#!/usr/bin/env bash
set -euo pipefail

project_root=$(cd "$(dirname "$0")/.." && pwd)
disc="$project_root/build/ror/disc/GAME/EMPIRESX.EXE"
patched="$project_root/build/patch_1_0c/EMPIRESX.EXE"

test "$(sha256sum "$disc" | awk '{print $1}')" = \
  8e0e6d047b2f90d0769fb5a1b16c1b1973582efdbe73a5d11395a43557215845
test "$(sha256sum "$patched" | awk '{print $1}')" = \
  948cd653c05f5a9557bae1584b77360d43864cb8559e8dc08a7861a1f1d8e3fb

pointer() {
  r2 -q -c "pv4 @ $2" -c q "$1" | tr -d '\r'
}

# Common +0x08 cleanup and +0x24 update-predicate slots on root and type 90.
test "$(pointer "$disc" 0x0054ce88)" = 0x004a80e0
test "$(pointer "$disc" 0x0054cea4)" = 0x004aa790
test "$(pointer "$disc" 0x0054d610)" = 0x004a80e0
test "$(pointer "$disc" 0x0054d62c)" = 0x004aa790
test "$(pointer "$patched" 0x00547db0)" = 0x004a6920
test "$(pointer "$patched" 0x00547dcc)" = 0x004a8f40
test "$(pointer "$patched" 0x0054847c)" = 0x004a6920
test "$(pointer "$patched" 0x00548498)" = 0x004a8f40

for address in 004a6920 004a6a40 004a8f40; do
  awk -F '\t' -v address="$address" \
    'NR > 1 && $1 == address && $3 == "ok" { found=1 } END { exit !found }' \
    "$project_root/build/decompile/ror_10a_index.tsv"
done

# Pin the mapped building routines and the anomalous PUSH -1 at each
# production drain call so refreshed analysis cannot silently alter the note.
for address in 004add50 004adfc0; do
  awk -F '\t' -v address="$address" \
    'NR > 1 && $1 == address && $3 == "ok" { found=1 } END { exit !found }' \
    "$project_root/build/decompile/ror_10a_index.tsv"
done
test "$(r2 -q -c 'p8 2 @ 0x004af97a' -c q "$disc" | tr -d '\r')" = 6aff
test "$(r2 -q -c 'p8 2 @ 0x004ae03a' -c q "$patched" | tr -d '\r')" = 6aff

echo "verified disc RoR -> patched 1.0a revision map"
