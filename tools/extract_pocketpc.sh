#!/usr/bin/env bash
# Extract locally supplied Age of Empires Pocket PC installers for research.
#
# Usage: tools/extract_pocketpc.sh <installer.exe | Disk1 directory> <out-dir>
#
# The installers are InstallShield packages whose data1.cab holds a Windows
# CE MSCF cabinet with the ARM game files. Requires `7z` and `unshield`
# (https://github.com/twogood/unshield; set UNSHIELD to a local build if it
# is not on PATH). Output belongs under build/ (git-ignored). Never commit
# extracted executables or game data.
set -euo pipefail

if [ $# -ne 2 ]; then
    echo "usage: $0 <installer.exe | Disk1 directory> <out-dir>" >&2
    exit 2
fi
src=$1
out=$2
unshield=${UNSHIELD:-unshield}
command -v 7z >/dev/null || { echo "7z is required" >&2; exit 1; }
command -v "$unshield" >/dev/null || { echo "unshield is required" >&2; exit 1; }

mkdir -p "$out"
if [ -f "$src" ]; then
    7z x -y -o"$out/installer" "$src" >/dev/null
    disk1=$(find "$out/installer" -type d -name Disk1 | head -n 1)
else
    disk1=$src
fi
[ -f "$disk1/data1.cab" ] || { echo "no data1.cab under $disk1" >&2; exit 1; }

"$unshield" -d "$out/installshield" x "$disk1/data1.cab" >/dev/null
find "$out/installshield" -type f -iname '*.cab' -size +1k | while read -r cab; do
    name=$(basename "$cab" .cab)
    7z x -y -o"$out/ce/$name" "$cab" >/dev/null
done

# Windows CE cabinets store files under mangled names (NNNNname.NNN); the
# ARM PE images are identified by content, not name.
find "$out/ce" -type f | while read -r f; do
    if head -c 2 "$f" | grep -q MZ; then
        printf '%s  %s\n' "$(sha256sum "$f" | cut -d' ' -f1)" "$f"
    fi
done
