#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: extract_ror_audio.sh DISC.cue OUTPUT_DIRECTORY" >&2
  exit 2
fi

cue_path=$1
output_dir=$2
cue_dir=$(dirname -- "$cue_path")
cue_stem=$(basename -- "$cue_path" .cue)

if [[ ! -f "$cue_path" ]] || ! command -v ffmpeg >/dev/null; then
  echo "cue sheet or ffmpeg not available" >&2
  exit 1
fi
if ! rg -q 'TRACK 02 AUDIO' "$cue_path" ||
   ! rg -q 'INDEX 01 00:01:00' "$cue_path"; then
  echo "unexpected Rise of Rome Rev 1 cue layout" >&2
  exit 1
fi

mkdir -p -- "$output_dir"
for number in {2..15}; do
  printf -v track '%02d' "$number"
  source_file="$cue_dir/$cue_stem (Track $track).bin"
  output_file="$output_dir/Track$track.wav"
  if [[ ! -f "$source_file" ]] || [[ -e "$output_file" ]]; then
    echo "missing source or existing output: Track$track" >&2
    exit 1
  fi
  skip_bytes=0
  if [[ "$track" == 02 ]]; then
    # The cue's INDEX 01 starts 75 CD sectors after Track 02's INDEX 00.
    skip_bytes=176400
  fi
  ffmpeg -hide_banner -loglevel error -nostdin -n \
    -f s16le -ar 44100 -ac 2 -skip_initial_bytes "$skip_bytes" \
    -i "$source_file" -c:a pcm_s16le -f wav "$output_file"
  echo "extracted Track$track.wav"
done
