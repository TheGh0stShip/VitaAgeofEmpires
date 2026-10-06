# PS Vita: the port (VAOE00001)

`VitaAoE.vpk` (title `VAOE00001`, "Age of Empires RoR") runs the translated
Rise of Rome screens on the Vita: the original Main Menu and the screens
it leads to, drawn from your own game files by the translated panel code
(`src/screens/`) through the UI host (`src/ui_host.c`), the screen
director (`src/ui_director.c`) and the console shell (`src/shell_app.c`).
A campaign scenario can be started (Single Player, Campaign, OK): the
scenario's Mission Dialog, then the Game Screen with the scenario's world
running in the original view rectangle. The game screen's bars, minimap,
command buttons and unit orders are not assembled yet; see `TODO.md`.

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
| Circle | Escape (back / cancel; in a game, clear the selection) |
| Start | Escape; in a game, the game menu (F10) |
| Triangle | Enter |
| L / R (held) | Ctrl / Shift (e.g. L + typing C opens the credits) |

The 640 x 480 game picture is shown at 725 x 544 in the middle of the
screen with the game's own pointer (Interfac 51001) on top.

## Status (2026-10-06)

- Runs in Vita3K at 60 FPS: the Main Menu, Single Player Menu, Scenario
  Builder menu, Multiplayer screen (no networking), Help dialog and
  credits. Not yet checked on hardware.
- Campaign start: Campaign Selection's OK reads the chosen scenario from
  the campaign file, shows its Mission Dialog (instructions, hints,
  history; the briefing picture is not drawn yet) and, on OK, the Game
  Screen: the world in the 640 x 334 view at (0, 20), advanced on idle,
  scrolled with the pointer at a screen edge (or the arrow keys on the
  host). The view starts on the player's town center, as the original's
  player start (0x004f1ae0) puts it. Checked on the host (with ASan);
  the VPK builds, not yet run past the menus in Vita3K.
- In the game: the Roman (or the civilisation's) interface bars, the
  resource bar, the minimap (click to move the view), unit selection
  (Cross / left button) with the selection box, orders (Square / right
  button: gather, attack or move), and the game menu (Start) with Quit
  back to the Main Menu. Not yet: the object panel, command buttons
  (training, building), drag selection, saving, the end of a scenario.
- Screen changes use the original's blocking 0.13 s palette fades; the
  intermediate fade frames are not presented (the picture holds, then
  switches).
- Text is rendered without TrueType hinting, so it may differ slightly
  from Windows.
