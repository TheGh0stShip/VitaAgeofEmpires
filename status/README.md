# Durable Wonder status image

The source of truth is [current.json](current.json). `port-progress.png`
and `port-progress.jpg` are generated status artwork made with the original
AoE/Rise of Rome assets. The current 10% is an approximate full-port estimate;
the assessment and effort notes in the JSON explain it. Image changes do not
mean new gameplay has been implemented.

## Build and update

Dependencies: Python 3 standard library, CMake and a C compiler for the SLP
decoder, ImageMagick `convert`, DejaVu Sans fonts, and the original local DRS
files. No network access is needed to render. Asset locations, caption text,
camera padding, output path, percentage, and effort are configurable in JSON.
Paths are relative to the project root unless absolute.

```sh
cmake -S . -B build/host
cmake --build build/host --target status_art_plan
cmake --build build/host --target status_art -j4
```

`status_art` builds the SLP decoder when necessary, reads
`status/current.json`, writes the PNG/JPEG/manifest, and retains an exact
history recipe. The shell command `bash tools/render_status_art.sh` remains
the direct interface when temporary CLI overrides are useful.

For the next real update, edit `progress_percent`, `progress_assessment`,
`effort`, and `effort_assessment` in `current.json`, review the plan target,
then run the render target.
Do not automatically increase completion because time passed or more tests
were added. Base the estimate on full-port milestones; explain any increase
or decrease. Effort describes the current reporting period, independently
of already completed work, and may go up or down. Do not imply that the
villagers count people, hours, or autonomous agents.

Use the review command before any publishing render. It is asset-free and makes
the game-rule decisions visible: the exact original construction frame, whether
the completed Wonder must replace the construction sprite, and the count,
ground anchors, facing, and draw layer of every builder.

```sh
python3 tools/render_status_art.py --plan
```

The only adjustable visual policy is recorded in `current.json`: percentage,
current-period effort (or an explicit 0–12 `workers` override), labels, and
camera/output settings. The construction-stage mapping and inward-facing
footprint-edge slots are code and test governed; do not manually retouch the
PNG, change a Wonder frame, or move a villager in an editor. Update the JSON,
review `--plan`, render, inspect both outputs, then retain the generated
history snapshot. That sequence keeps each public image reproducible and
meaningful as the port estimate changes.

| Effort | Working villagers |
| --- | --- |
| `idle` | 0 |
| `low` | 2 |
| `steady` | 4 |
| `high` | 8 |
| `intensive` | 12 |

Set `workers` to an integer 0–12 to override a preset, or `null` to use it.
At 100% there are no active builders and the caption changes to completion.
Example preview (does not edit the saved current settings):

```sh
bash tools/render_status_art.sh --percent 50 --effort high \
  --progress-note 'Stage-layout example, not a new port estimate' \
  --effort-note 'Example showing eight builders' \
  --output build/status_work/example-50.png --no-history
```

CLI options also include `--config FILE`, `--workers N`, and `--output PNG`.
`--plan` validates and prints the derived scene without rendering or creating
history, which is useful when deciding whether an effort change should add or
remove builders. The renderer never derives a percentage from the number of
builders: completion and current effort are intentionally separate estimates.
The earlier `bash tools/render_status_art.sh 10 OUTPUT.png` invocation still
works. Ordinary runs always retain history. `--no-history` is for test or
layout previews only. CLI overrides do not edit `current.json`; the exact
effective settings are saved in the output manifest.

The original DRS files are not included. Before rendering on another machine,
edit the asset paths in `current.json` to point to locally supplied game data.
Generated screenshots and extracted audio/data are excluded from version
control; the checked-in progress artwork is only a reference output.

## Original construction stages

[The executable evidence](evidence/construction-stages.md) verifies
`floor(3 * progress / 100)` before completion, then the standing Wonder:

| Percentage | Original asset |
| --- | --- |
| 0 to less than 33⅓ | Construction SLP 302, frame 0 |
| 33⅓ to less than 66⅔ | Construction SLP 302, frame 1 |
| 66⅔ to less than 100 | Construction SLP 302, frame 2 |
| 100 | Completed Roman Wonder, SLP 735, frame 0 |

The original game uses discrete foundation stages. Do not invent extra
intermediate heights or fade in a finished building. Decimal percentages
are accepted; the stage calculation uses the input precision, while the
caption displays at most two decimals. This is a still representation of
the original rule, not a simulation of construction timing.

The layout was checked against this
[original AoE construction screenshot](https://1.bp.blogspot.com/-Wg1ZdIa0xeY/U2XLzPm-4nI/AAAAAAAACJE/YGRSlRigUnI/s1600/Age_of_Empires.jpg).
Wonder DAT size 2.5/2.5 gives a five-by-five footprint. The scene deliberately
paints dirt there; the Wonder's own terrain replacement field is -1.
Every worker's ground anchor stays outside the footprint and faces inward.
Additional workers populate the back edges and draw before the Wonder.
SLP hotspots, including mirrored hotspots, control placement. Never place
workers by eye against scaffold props or the sprite's transparent bounds.

The new fixed 960x640 composition accommodates the completed Colosseum's
full height and a separate footer. PNG is opaque RGB, 8-bit, non-interlaced;
the accompanying JPEG is baseline. The renderer rejects sprite clipping.
The original 960x544 composition remains available in the frozen v4 recipe.

## History and exact replay

Every ordinary render writes PNG, JPEG, and a JSON manifest beside them,
plus a unique, retained folder in `status/history/`. Each folder contains:

- the generated images and their SHA-256 values;
- the effective configuration, asset paths and SHA-256 values;
- the actual subprocess command arguments, selected SLP frames and hotspots;
- worker positions, facings, layering, percentage, and assessments;
- Python/ImageMagick versions and font hashes;
- copies of the renderer, decoder source/binary, and construction evidence.

DRS files are referenced by path and checksum rather than duplicated on
every update. Keep them available. These durable recipes are outside the
ignored `build/` directory. Back up the `status/` directory with the project.

```sh
python3 tools/replay_status_art.py status/history/SELECTED-SNAPSHOT
```

This verifies the original assets, fonts, decoder, renderer, and ImageMagick
version, reruns the archived recipe to `build/status_work/replayed.png`, and
checks a byte-identical PNG. It preserves the current published image.
If the archived Linux decoder cannot run on another machine, rebuild from
the sources under the snapshot's `recipe/` directory:

```sh
cc -std=c11 -Iinclude tools/slp_tool.c src/drs.c src/slp.c src/palette.c \
  -o build/host/slp_tool
```

Different compiler/font/ImageMagick versions can change a checksum; keep
the original manifests rather than relabeling a changed output as identical.

The pre-configurable image, exact shell recipe and decoder were separately
frozen in [recipes/v4](recipes/v4/README.md). Reproduce it with
`python3 tools/replay_status_v4.py`; this checks its original PNG hash.

Before publishing a new version, inspect the complete PNG and JPEG and
confirm the footprint edges, builder facings, full Wonder height, and footer.
The construction boundary/geometry checks run with:

```sh
python3 tests/status_art_test.py
```
