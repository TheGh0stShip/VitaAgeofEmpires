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
- **Known rendering defects** (reported from Vita3K, under investigation):
  - wall pieces do not join (the adjacency refresh `0x004ad550` is not
    bound);
  - cliffs show one angle;
  - one transport sprite is clipped;
  - a skeleton decoration is reported in the water.
- **Fixed during bring-up:**
  - **Slope shapes.** The terrain elevation shape's diagonal rules were
    permuted against RoR `0x004493a0` / patched `0x004484f0`, leaving
    black triangles on hills.
  - **printf.** VitaSDK's newlib printf does not consume `%zu`/`%llu`,
    which crashed the overlay after about 38 seconds.
- **Unverified on hardware.** Vita3K runs it at about 27 FPS. That is not
  a hardware measurement, and nothing here has yet been run on a physical
  Vita or PSTV.
