# PS Vita: the port (VAOE00001)

`VitaAoE.vpk` (title `VAOE00001`, "Age of Empires RoR") runs the translated
Rise of Rome screens on the Vita: the original Main Menu and the screens
it leads to, drawn from your own game files by the translated panel code
(`src/screens/`) through the UI host (`src/ui_host.c`), the screen
director (`src/ui_director.c`) and the console shell (`src/shell_app.c`).
The game itself (a match) is not reachable yet; see `TODO.md`.

The earlier tech demo (scene, world update, two commands) is now its own
title, `VitaAoEDemo.vpk` (`VAOE00002`); see [VITA_DEMO.md](VITA_DEMO.md).

## Build

```
cmake --build build/vita
```

produces `build/vita/VitaAoE.vpk` and
`build/vita/cmake/vita_demo/VitaAoEDemo.vpk`. Neither contains game data
or fonts.

## Files to supply

Copy your own files to `ux0:data/VitaAoE/` (names match ignoring case):

| Path | From |
| --- | --- |
| `data/` | the base game's `data` folder (`Interfac.drs`, `graphics.drs`, `sounds.drs`, `Terrain.drs`, `Border.drs`, ...) |
| `data2/` | Rise of Rome's `DATA2` folder (`INTERFAC.DRS`, `GRAPHICS.DRS`, `SOUNDS.DRS`, `EMPIRES.DAT`) |
| `campaign/` | Rise of Rome's `CAMPAIGN` folder |
| `languagex.dll` | Rise of Rome's language library (`LANGUAGEX.DLL`; `LANGUA_1.DLL` on the disc) |
| `language.dll` | the base game's language library |
| `fonts/arial.ttf`, `fonts/arialbd.ttf` | Arial and Arial Bold from a Windows `Fonts` folder |
| `fonts/COPRGTL.TTF` | optional: Copperplate Gothic Light (shipped with Microsoft Office). Without it the titles use Arial, as the original did on PCs without the font. |

## Controls

| Input | Action |
| --- | --- |
| Touch | Move the pointer there and press / release the left button |
| Left stick / D-pad | Move the pointer |
| Cross | Left button |
| Square | Right button |
| Circle, Start | Escape (back / cancel) |
| Triangle | Enter |
| L / R (held) | Ctrl / Shift (e.g. L + typing C opens the credits) |

The 640 x 480 game picture is shown at 725 x 544 in the middle of the
screen with the game's own pointer (Interfac 51001) on top.

## Status (2026-10-06)

- Runs in Vita3K at 60 FPS: the Main Menu, Single Player Menu, Scenario
  Builder menu, Multiplayer screen (no networking), Help dialog and
  credits. Not yet checked on hardware.
- Screen changes use the original's blocking 0.13 s palette fades; the
  intermediate fade frames are not presented (the picture holds, then
  switches).
- Text is rendered without TrueType hinting, so it may differ slightly
  from Windows.
