#!/usr/bin/env bash
set -euo pipefail

# Original-asset status artwork, not a capture from the unfinished Vita port.
# Usage: tools/render_status_art.sh PERCENT [OUTPUT.png]
project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
percent="${1:-}"
if [[ ! "$percent" =~ ^(100|[0-9]|[1-9][0-9])$ ]]; then
    echo "usage: $0 PERCENT [OUTPUT.png] (0..100)" >&2
    exit 2
fi
output_path="${2:-$project_dir/status/port-progress.png}"
scratch_dir="$project_dir/build/status_work"
base_graphics='/mnt/d/Age of Empires/data/graphics.drs'
base_terrain='/mnt/d/Age of Empires/data/Terrain.drs'
base_interface='/mnt/d/Age of Empires/data/Interfac.drs'
ror_graphics="$project_dir/build/ror/disc/GAME/DATA2/GRAPHICS.DRS"
renderer="$project_dir/build/host/slp_tool"
for required in "$base_graphics" "$base_terrain" "$base_interface" "$ror_graphics" "$renderer"; do
    if [[ ! -f "$required" ]]; then
        echo "missing original asset or built slp_tool: $required" >&2
        exit 1
    fi
done
if ! command -v convert >/dev/null 2>&1; then
    echo "ImageMagick convert is required" >&2
    exit 1
fi
mkdir -p "$scratch_dir" "$(dirname "$output_path")"

"$renderer" render "$base_terrain" 15001 "$base_interface" "$scratch_dir/grass.pam" 0
"$renderer" render "$ror_graphics" 720 "$base_interface" "$scratch_dir/towncenter.pam" 0
"$renderer" render "$ror_graphics" 723 "$base_interface" "$scratch_dir/house.pam" 0
for builder_frame in 52 54; do
    "$renderer" render "$base_graphics" 628 "$base_interface" \
        "$scratch_dir/builder${builder_frame}.pam" "$builder_frame"
done
convert "$scratch_dir/builder54.pam" -flop "$scratch_dir/builder54-ne.pam"
convert "$scratch_dir/builder52.pam" -flop "$scratch_dir/builder52-ne.pam"
for dirt_frame in $(seq 0 8); do
    "$renderer" render "$base_terrain" 15000 "$base_interface" \
        "$scratch_dir/dirt${dirt_frame}.pam" "$dirt_frame"
done

# The original construction art has three visibly different frames. This is
# an illustrative status mapping, not a claim about the game's build timer.
if (( percent == 100 )); then
    "$renderer" render "$ror_graphics" 735 "$base_interface" \
        "$scratch_dir/wonder.pam" 0
elif (( percent < 34 )); then
    "$renderer" render "$base_graphics" 302 "$base_interface" \
        "$scratch_dir/wonder.pam" 0
elif (( percent < 67 )); then
    "$renderer" render "$base_graphics" 302 "$base_interface" \
        "$scratch_dir/wonder.pam" 1
else
    "$renderer" render "$base_graphics" 302 "$base_interface" \
        "$scratch_dir/wonder.pam" 2
fi

# Tile the original grass SLP's diamond frames on an isometric grid. The
# oversized canvas avoids ImageMagick's off-edge MVG image-cache error.
terrain_commands=''
for terrain_row in $(seq 0 32); do
    terrain_y=$((terrain_row * 16))
    for terrain_col in $(seq 0 16); do
        terrain_x=$((terrain_col * 64 + (terrain_row % 2) * 32))
        terrain_commands+=" image over ${terrain_x},${terrain_y} 0,0 '$scratch_dir/grass.pam'"
    done
done
convert -size 1152x600 xc:'#416325' -draw "$terrain_commands" \
    -crop 480x220+96+64 +repage "$scratch_dir/terrain.png"

fill_width=$((percent * 792 / 100))
scene_commands="image over 5,26 0,0 '$scratch_dir/towncenter.pam'"
scene_commands+=" image over 5,150 0,0 '$scratch_dir/house.pam'"
scene_commands+=" image over 89,150 0,0 '$scratch_dir/house.pam'"
# Original Wonder DAT size is 2.5/2.5: a five-by-five-tile footprint.
# The in-game construction screenshot referenced in status/README.md shows
# continuous dirt underneath the scaffolding. Its projected diamond here is
# N(312,45), E(472,125), S(312,205), W(152,125).
for dirt_row in $(seq 0 4); do
    for dirt_col in $(seq 0 4); do
        dirt_x=$((280 + (dirt_col - dirt_row) * 32))
        dirt_y=$((45 + (dirt_col + dirt_row) * 16))
        dirt_frame=$(((dirt_row % 3) * 3 + dirt_col % 3))
        scene_commands+=" image over ${dirt_x},${dirt_y} 0,0 '$scratch_dir/dirt${dirt_frame}.pam'"
    done
done
scene_commands+=" image over 167,45 0,0 '$scratch_dir/wonder.pam'"
# Feet lie just OUTSIDE the two front footprint edges, facing inward:
# SW edge: (205,163), (268,194), facing NE (mirrored NW frames).
# SE edge: (407,169), (357,194), facing NW.
# Frame 54 hotspot is 11,26; frame 52 is 17,26 (mirrored: 12,26).
scene_commands+=" image over 194,137 0,0 '$scratch_dir/builder54-ne.pam'"
scene_commands+=" image over 256,168 0,0 '$scratch_dir/builder52-ne.pam'"
scene_commands+=" image over 396,143 0,0 '$scratch_dir/builder54.pam'"
scene_commands+=" image over 340,168 0,0 '$scratch_dir/builder52.pam'"

# Compose and scale the scene first. The opaque footer and its progress bar
# are drawn afterward at final resolution, with explicit text baselines.
convert "$scratch_dir/terrain.png" -draw "$scene_commands" \
    -filter point -resize 960x440\! -background '#14251e' \
    -gravity northwest -extent 960x544 -gravity none -alpha off \
    -fill '#14251e' -draw 'rectangle 0,0 959,43 rectangle 0,440 959,543' \
    -fill '#e8d6a5' -font DejaVu-Sans-Bold -pointsize 20 \
    -draw "text 24,29 'RISE OF ROME / VITA PORT'" \
    -fill '#d6cdb9' -font DejaVu-Sans -pointsize 13 \
    -draw "text 662,28 'ORIGINAL ASSETS / STATUS ART'" \
    -fill '#e8d6a5' -font DejaVu-Sans-Bold -pointsize 18 \
    -draw "text 24,469 'WONDER UNDER CONSTRUCTION'" \
    -fill '#d6cdb9' -font DejaVu-Sans -pointsize 13 \
    -draw "text 712,469 'FULL PORT ESTIMATE'" \
    -fill '#f6e3a7' -font DejaVu-Sans-Bold -pointsize 26 \
    -draw "text 24,514 '${percent}%'" \
    -fill '#f3e5ba' -draw 'rectangle 120,485 919,524' \
    -fill '#304c3c' -draw 'rectangle 124,489 915,520' \
    "$scratch_dir/status-unfilled.png"
if (( fill_width > 0 )); then
    convert "$scratch_dir/status-unfilled.png" -fill '#f4b63e' \
        -draw "rectangle 124,489 $((123 + fill_width)),520" \
        -background '#14251e' -alpha remove -alpha off -depth 8 \
        -strip -interlace none "PNG24:$output_path"
else
    convert "$scratch_dir/status-unfilled.png" \
        -background '#14251e' -alpha remove -alpha off -depth 8 \
        -strip -interlace none "PNG24:$output_path"
fi
echo "$output_path"
