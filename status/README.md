# Durable port-status image

The source of truth is [current.json](current.json). `port-progress.png`
and `port-progress.jpg` are generated status artwork: a Roman city made from
the original AoE/Rise of Rome assets and drawn by the port's own engine (its
scenario runtime, world actions and viewport renderer). The percentage is an
approximate full-port estimate; the assessment and effort notes in the JSON
explain it. Image changes do not mean new gameplay has been implemented,
and the image is labelled status art, not a Vita screenshot.

## Build and update

Dependencies: Python 3 standard library, CMake and a C compiler for
`status_city_tool`, ImageMagick `convert`, DejaVu Sans fonts, and the
original local game files (the expansion DAT, both `graphics.drs`, both
`Interfac.drs`, `Terrain.drs`, `Border.drs`, `TILEEDGE.DAT`, the colour
tables). No network access is needed to render. Asset locations, caption
text, output path, percentage and effort are configurable in JSON. Paths
are relative to the project root unless absolute.

```sh
cmake -S . -B build/host
cmake --build build/host --target status_art_plan
cmake --build build/host --target status_art -j4
```

`status_art` builds the tool when necessary, reads `status/current.json`,
writes the PNG/JPEG/manifest, and retains an exact history recipe. The shell
command `bash tools/render_status_art.sh` remains the direct interface when
temporary CLI overrides are useful.

For the next real update, edit `progress_percent`, `progress_assessment`,
`effort`, and `effort_assessment` in `current.json`, review the plan target,
then run the render target. Do not automatically increase completion
because time passed or more tests were added. Base the estimate on
full-port milestones; explain any increase or decrease. Effort describes the
current reporting period, independently of already completed work, and may
go up or down. Do not imply that the builders count people, hours, or
autonomous agents.

Use the review command before any publishing render. It is asset-free and
makes the game-rule decisions visible: the exact original construction
stage, whether the completed Wonder replaces the construction sprite, and
the builders' spawn tiles.

```sh
python3 tools/render_status_art.py --plan
```

| Effort | Builders at the Wonder |
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
`--plan` validates and prints the derived choices without rendering or
creating history. The renderer never derives a percentage from the number
of builders: completion and current effort are intentionally separate
estimates. Ordinary runs always retain history; `--no-history` is for test
or layout previews only. CLI overrides do not edit `current.json`; the exact
effective settings are saved in the output manifest.

## How the picture is made

1. `tools/status_city_plan.py` decides what stands where: the harbour with
   its dock and ships along the top, the forum (Town Center, Market,
   Government Center, Temple), the Wonder under construction with its
   builders, the eastern district (Academy, houses, Granary, berry bushes,
   fields), the walled military quarter (Barracks, Stable, Archery Range,
   Siege Workshop, the mustered army), and outside the walls the woods with
   lumberjacks, the mines with miners and the game. It checks every
   footprint against the others and against the view, so no two objects
   share a tile and nothing is cut by the frame, and writes a plan file.
2. `tools/status_city_tool.c` executes the plan with the port's engine: an
   in-memory scenario (Roman player at the Iron Age, so the age effects
   select the Iron Age building graphics), every object created through
   the scenario runtime's editor factory, type-80 buildings completed
   through add-build-work (0x004ad7b0), the Wonder left at the configured
   share of its build time so the building update (0x004ae880) selects its
   construction stage itself, villagers ordered onto their targets through
   the order step (0x00405f20: build, farm, forage, chop, mine, fish) and
   the world ticked for fifteen seconds so they walk there and work. The
   frame is then drawn by the viewport renderer (terrain, borders, the
   deferred sprite list in the original order, player colours, shadows,
   the water palette cycle).
3. `tools/render_status_art.py` checks the construction stage the engine
   reports against the rule below, adds the title and progress bands, and
   writes the PNG, JPEG and manifest.

Two data choices are documented here because they are not how a Rise of
Rome scenario would normally be laid out: the roads use the base game's
unused cobblestone Road tile (`graphics.drs` SLP 530, DAT graphic 488)
placed on the unused terrain slot 12, with the grass/desert border as a
dirt shoulder, so the paving is continuous along either axis (the path
decals of the DAT are separate slabs); and the shore is a two-tile beach
band between grass and water, as the original maps lay their coasts. The
Wonder's dirt is part of its construction sprite; nothing is retouched in
an editor.

## Original construction stages

[The executable evidence](evidence/construction-stages.md) verifies
`floor(3 * progress / 100)` before completion, then the standing Wonder;
in this image the translated building update performs the selection on the
live foundation and the renderer only confirms it:

| Percentage | Original asset |
| --- | --- |
| 0 to less than 33⅓ | Construction SLP 302, frame 0 |
| 33⅓ to less than 66⅔ | Construction SLP 302, frame 1 |
| 66⅔ to less than 100 | Construction SLP 302, frame 2 |
| 100 | Completed Roman Wonder, SLP 735, frame 0 |

The original game uses discrete foundation stages. Do not invent extra
intermediate heights or fade in a finished building. Decimal percentages
are accepted; the caption displays at most two decimals.

The composition is 1280x800 of engine output at the original pixel scale
plus a 44-pixel title band and a 104-pixel progress footer (1280x948). PNG
is opaque RGB, 8-bit, non-interlaced; the accompanying JPEG is baseline.

## History and exact replay

Every ordinary render writes PNG, JPEG, and a JSON manifest beside them,
plus a unique, retained folder in `status/history/`. Each folder contains:

- the generated images and their SHA-256 values;
- the effective configuration, asset paths and SHA-256 values;
- the plan file and the plan module, the renderer, the tool's source and
  binary, the engine commit, and the construction evidence;
- the actual subprocess command arguments and the engine's report
  (objects created, orders, ticks, units drawn, the Wonder's stage);
- Python/ImageMagick versions and font hashes.

DRS files are referenced by path and checksum rather than duplicated on
every update. Keep them available. These durable recipes are outside the
ignored `build/` directory. Back up the `status/` directory with the project.

```sh
python3 tools/replay_status_art.py status/history/SELECTED-SNAPSHOT
```

This verifies the original assets, fonts, tool, renderer, plan module and
ImageMagick version, reruns the archived recipe to
`build/status_work/replayed.png`, and checks a byte-identical PNG. It
preserves the current published image and also replays the earlier
Wonder-only snapshots (schema 1). If the archived Linux tool cannot run on
another machine, rebuild `status_city_tool` from the engine commit recorded
in the manifest.

Different compiler/font/ImageMagick versions can change a checksum; keep
the original manifests rather than relabeling a changed output as identical.

The first-generation image, exact shell recipe and decoder were separately
frozen in [recipes/v4](recipes/v4/README.md). Reproduce it with
`python3 tools/replay_status_v4.py`; this checks its original PNG hash.

Before publishing a new version, inspect the complete PNG and JPEG and
confirm the paving, the shore, the builders at the Wonder, the mustered
army inside the walls and the footer. The rule and layout checks run with:

```sh
python3 tests/status_art_test.py
```
