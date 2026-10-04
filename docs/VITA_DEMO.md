# PS Vita tech demo

`VitaAoE.vpk` (title `VAOE00001`) runs the reconstructed Rise of Rome
runtime on the Vita. Everything runs on the Vita itself:

- parsing the user's original DAT, DRS and campaign files;
- building the scenario runtime and the game start (`0x0050aaa0`);
- the translated world update at 20 ticks per second;
- drawing the scene with the original art through the translated
  terrain/unit renderer.

It is a demonstration shell, not a playable game. Its own pieces are:

- the overlay text and its 5x7 font;
- the camera/scroll mapping;
- two demo commands, which call translated functions:
  - train a villager at player 1's town center (`0x004adab0`);
  - order player-1 soldiers near the camera to attack the nearest enemy
    (the order step `0x00405f20`).

The original game interface is not reconstructed yet.

## Build

```
cmake --build build/vita
```

This produces `build/vita/VitaAoE.vpk`. It contains only the eboot and the
LiveArea images, which `tools/make_vita_livearea.py` draws from scratch. No
game data is packaged.

## Data (user-supplied)

Copy your own files to `ux0:data/VitaAoE/`. Names are matched ignoring
case.

| Folder | Contents |
| --- | --- |
| `data/` | from the base game's `data`: `graphics.drs`, `Terrain.drs`, `Border.drs`, `Interfac.drs`, `TILEEDGE.DAT` (loose `*.col` are optional; Interfac.drs carries them) |
| `data2/` | from Rise of Rome's `DATA2`: `EMPIRES.DAT`, `GRAPHICS.DRS`, `INTERFAC.DRS` |
| `campaign/` | Rise of Rome `EXPANS_*.CPX` (scenarios are read from them directly) |
| `scenario/` | optional loose `*.scx` / `*.scn` |

The demo writes `ux0:data/VitaAoE/demo_cache.scx` when it extracts a
campaign scenario.

## Controls

| Input | Action |
| --- | --- |
| Left stick / D-pad | Scroll |
| Cross | Train a villager at player 1's town center |
| Square | Player-1 soldiers near the camera attack the nearest enemy |
| Circle | Next point of interest (player cameras, then army centres) |
| L / R | Previous / next scenario |
| Triangle | Toggle overlay |
| Select | Pause the simulation |
| Start | Quit |

## Host harness

`demo_host ROOT FRAMES OUT.ppm [SCENARIO]` runs the same `aoe_demo_*` loop
headless with scripted input. `ror_vita_demo_loop_real` runs it when
`build/demo_root` holds the same layout (symlinks are fine).

## Status (2026-10-04)

- **Loading.** 11 of the 19 Rise of Rome campaign scenarios build. The
  other 8 contain high-state records whose per-player state-notify methods
  (template `+0xa5`/`+0xa6`) or type-40 high states are not translated
  yet. The demo skips those scenarios.
- **Fog.** Scenario fog history is not reconstructed, so the whole map is
  shown (the original's exploration-gate bypass).
- **Rendering fixes from Vita3K testing** (each against patched RoR 1.0a
  or the disc build):
  - **Sprite draw order.** Sprites were drawn during the terrain tile walk,
    so later terrain cut through large sprites (half transports, broken
    cliff faces). The original queues them in the view's sprite list
    (`0x00518d90`) and draws them after all terrain, ordered by layer, then
    screen row, then queue order (`0x00515844`, `0x0051606f`);
    `AoeSpriteDrawQueue` reconstructs this.
  - **Gaia spawn.** A new game builds Gaia as a computer player
    (`0x0050a470` -> `0x004f7290`), so Gaia objects use the tribe spawn
    `0x004efe70` and store `ftol(rotation)` as their facet. Before this fix
    every cliff piece showed angle 0.
  - **Facet formula.** The type-30+ facet formula (`0x0044c7b0`) is
    `(rotation - pi/4) * angles / 2pi + 0.5`, wrapped; it was a plain
    truncation.
  - **Wall adjacency.** Wall adjacency (`0x004ad550` / `0x004ad620` /
    `0x004ad380` / `0x0044f090`) now runs on completion and on upgrades, so
    wall pieces join into corners and runs.
  - **Water palette.** Water uses palette 248..254, which palette 50500
    leaves black. The game fills them with a 7-entry ramp and rotates it
    every 200 ms (`0x0047e8a3`, `0x004843d0`).
  - **Skeletons.** The bones reported in the water are scenario objects on
    beach tiles (terrain 2) at the waterline; all 69 skeletons on
    EXPANS_1_1 are on beach (10) or terrain 6 (59).
- **Not translated yet.** Gaia's tribe scenario load (`0x004f02d0`) reads
  per-player stream data our parser does not expose yet, so Gaia has no
  research manager.
- **Hardware performance (2026-10-04).** About 45 fps on a Vita.
  - Breakdown: terrain 2.1 ms (cached layer), objects 8.9 ms, colour
    conversion 8.2 ms on two cores. The conversion is limited by writing
    2 MB per frame into uncached video memory.
  - Deferred: `aoe_demo_render_indexed` / `aoe_demo_palette_abgr` /
    `aoe_demo_overlay` exist so a GPU path (P8 texture via vita2d) can take
    the conversion off the CPU.
- **Unverified on hardware.** Vita3K runs it at about 27 FPS. That is not
  a hardware measurement, and nothing here has yet been run on a physical
  Vita or PSTV.
