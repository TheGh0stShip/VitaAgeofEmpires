# Provenance

Where every part of this repository comes from, what it rests on, and what in
it is not the project's own work. The aim is a reusable engine that owes
nothing to other reconstructions: no copied code, no translated code, no
reliance on their format documentation, and nothing that would require
crediting or relicensing someone else's work.

`tools/provenance_audit.py` checks the mechanical part of this document. It
fails when a tracked file names or links another reconstruction or format
library, and it classes every C file under `src/`. It runs as the
`provenance_no_outside_references` test.

## What the reconstruction is made from

| Source | Role |
| --- | --- |
| The user's own executables: patched Rise of Rome 1.0a `EMPIRESX.EXE` (primary), base 1.0c `EMPIRES.EXE`, disc revisions for cross-checks | The behaviour being reconstructed. Never committed. |
| A Ghidra decompilation and `objdump` disassembly of those executables, made locally (`build/decompile/`, never committed) | How each function is read. Where the decompiler is wrong (argument order, switch tables, the `THIS_COD` assembly) the disassembly decides. |
| The user's retail data (`data/`, `data2/`, campaigns, scenarios, language libraries) | Formats confirmed against real files; never committed. |
| Microsoft's public Windows API documentation | The meaning of the executables' Windows calls (window messages, GDI, DirectDraw, `mmio`, `LoadStringA`), and of public container formats the executable relies on Windows to read (PE resource tables, BMP, RIFF). |

Every translated function cites its address in the patched executable
(`0x004xxxxx` when translated, six digits when only called). Translations are
written by hand from the decompilation and disassembly; no output of another
project is used.

## Classes of code

Run `python3 tools/provenance_audit.py --table` for the per-file list. On
2026-10-06: 274 C files under `src/` are **translations** (they, or the
header documentation of the functions they define, cite executable
functions) and 9 are **port code** written for this project without an
original counterpart:

| Port file | What it is |
| --- | --- |
| `src/platform/vita_main.c`, `src/platform/vita_shell_main.c` | Vita entry points (display, pad, touch) |
| `src/shell_app.c`, `src/shell_osk.c` | The console shell and its on-screen keyboard |
| `src/soft_draw.c`, `src/soft_edit.c` | Software stand-ins for DirectDraw surfaces and the Windows EDIT control |
| `src/graphic_art.c`, `src/graphic_art_cache.c` | Loading and caching decoded SLP frames for the port's renderer |
| `src/unit_sprite_binding.c` | Binding the translated graphic-selection rule to the port's sprite records |

Integration files (the UI host, the screen director, the Game Screen host,
the world engine in `src/demo_app.c`) are counted as translations because
they call and document translated functions, but much of their own code is
port glue; their headers say which parts are original.

## File formats and their executable evidence

| Format | Reader in this repository | Executable evidence |
| --- | --- | --- |
| DRS archives | `src/drs.c` | Registration 0x0046afc0 (0x40-byte header, `tribe` at +0x2c, table count +0x38, first data +0x3c), lookup 0x0046b280 (12-byte tables from +0x40, 12-byte entries) |
| SLP shapes | `src/slp.c`, `src/shape.c` | Loader 0x0049f5f0, frame count 0x0049fa90, size 0x0049fa00, bounds 0x0049fac0, pixel test 0x0049fb90, draw 0x0049fe00, rasterizer 0x005409a0 (command loop 0x005410a0, tables 0x0053f440 / 0x0053f4c0, 0x00541b60) |
| SHP shapes | `src/shape.c` | The same shape functions' SHP branch |
| Compressed streams (DAT, scenario bodies, player file) | `src/inflate.c`, `src/deflate_writer.c` | Decoder 0x00525b0b (work size 0x00525b05), reached from the stream read 0x0046b800, with its tables at 0x00525060..0x00525290; encoder 0x005223e0 (state 0x00524ef0, size 0x00525050, tables 0x0055fa20..0x00560738), driven by the stream write 0x0046ba40 as (75, 2, 1, 64 KB) |
| DAT graphics | `src/dat.c` | Reader 0x004a4c40, writer 0x004a5220, delta use in 0x004a5d60 |
| DAT terrains, borders, tile geometry | `src/dat.c` | Map object read as one 0x8dd0-byte image (0x00443ce0); the map's text loader 0x00446bb0 and its format string name the terrain fields; the view's reads of +0x8d96..+0x8d9e |
| DAT units | `src/dat_units.c` | Template loader 0x004413b0 and the class loaders; copy 0x00441f50 |
| DAT civilisations | `src/dat_units.c` | Master player loader 0x0044fa10 |
| DAT techs | `src/dat_tech.c` | Tech table reader 0x004e9c60 |
| Scenarios (`.scn` / `.scx`) | `src/scn.c` | File open 0x00417670 (version, record length, raw header, decompressed body), record readers 0x00474ae0 / 0x00506770, scenario object writer 0x00476280, player and map readers cited in the file |
| Campaigns (`.cpn` / `.cpx`) | `src/cpx.c`, `src/campaign_progress.c` | Campaign loader 0x0041c2d0, scenario slice 0x0041c700 |
| Player file (`game*.nfo`) | `src/campaign_progress.c` | 0x0043c6b0 and the functions cited there |
| WAV | `src/wav_stream.c` | Reader 0x0043a6f0 / 0x0043a8a0 (`mmio` chunk walk) |
| Palettes (JASC) | `src/palette.c` | 0x0042eb90 |
| Language strings | `src/pe_strings.c` | 0x004ff530 / 0x00417490 through `LoadStringA`; the RT_STRING layout is Microsoft's PE format |
| `map.bmp` | `src/game_screen_host.c` | The diamond map's loader 0x0042c540 reads Interfac 50401; BMP is Microsoft's format |

## Reliance removed on 2026-10-06

Earlier work had leaned on other projects in four places, and on generated
helper code in one. Each now rests on the executables instead.

| Area | What it relied on | Now |
| --- | --- | --- |
| SLP commands | Another project's SLP documentation, which describes a later engine | Re-derived from the rasterizer 0x005410a0. This corrected `slp.c`: a count-0 transparent run reads no extra byte; the extended forms are 0x1e (one outline pixel), 0x2e (an outline run), 0x3e (nothing) and 0x4e (the next command's pixels skipped), and the other 0x?e bytes are traps. The installed archives use none of the extended forms; all 742,629 drawn rows of the 16,718 frames decode exactly under these rules. |
| DAT graphic record | Two projects' graphic schemas, as cross-checks | 0x004a4c40 / 0x004a5220 / 0x004a5d60 |
| DAT terrain record | Another project's terrain schema | 0x00443ce0 / 0x00446bb0 and the view's offsets |
| Scenario file | Another project's scenario and map readers, as cross-checks; its field names | 0x00417670 and the record readers; the header fields renamed to the executable's meaning (`record_version`, `has_conditions`) |
| WAV | A RIFF parser adapted from a generated helper-code bundle | Removed; `wav_tool` uses the translated reader 0x0043a6f0 |
| Compressed streams | zlib, to inflate the DAT, scenario and player-file bodies and to deflate the player file | The executable's decoder 0x00525b0b (`src/inflate.c`) and encoder 0x005223e0 (`src/deflate_writer.c`, with 0x0046ba40's configuration). The encoder reproduces the installed DAT and all 20 campaign scenario bodies byte for byte (`ror_original_deflate_roundtrip_real`), which also confirms the decoder, the hash chains, the block choice and the Huffman construction at once. |
| Planning | A survey of other reconstructions (architecture, subsystem checklists, planned use of format libraries as test oracles) | Removed; none of it is used |

The commit history before 2026-10-06 still contains the removed documents.

## Third-party code in the build

No compression library is linked: the executable's own decoder and
encoder are translated (`src/inflate.c`, `src/deflate_writer.c`), and zlib
was removed from the build on 2026-10-06.

| Component | Where | Licence | Note |
| --- | --- | --- | --- |
| stb_truetype | `third_party/stb/stb_truetype.h`, vendored | MIT or public domain (dual) | Renders the user's Windows fonts in place of GDI. Under the public-domain option no notice is required; it is still someone else's code. A port-owned TrueType rasterizer would remove it. |
| VitaSDK runtime | The Vita build links its C library and system stubs | VitaSDK's licences | Platform requirement for any Vita program. |
| Host C library | Host builds | — | Platform. |

## Developer tools (not part of the engine)

Ghidra, GNU binutils (`objdump`), GCC / Clang, CMake, VitaSDK, Python, and,
for Pocket PC research only, `7z` and `unshield` (`tools/extract_pocketpc.sh`).
They are run on the developer's machine; nothing from them enters the
engine's source.

## Open items

1. **DAT field vocabulary.** On 2026-10-06 the DAT structures in
   `include/dat.h` were checked against the executable's loaders (task
   0x004e74f0, unit classes 0x004413b0 / 0x0043f0c0 / 0x004ec310 and the
   others listed in `unit_template.h`, graphics 0x004a4c40, the map image
   0x00443ce0 / 0x00446bb0). Fields named after other format tools were
   renamed to what their translated use shows, with the offset and the
   function in the header (for example the task's `carry_resource`,
   `rate_attribute`, `deposit_resource`, `work_amount`, `action_range`,
   `approach_graphic`; the unit's `terrain_table`, `fire_frame`,
   `building_frame`, `state6_graphic`; the graphic's `reference_speed`,
   `loop_delay`, `sequence_flags`, `mirror_limit`; the terrain's
   `object_ids` / `object_amounts`; the border's `draw_base` and
   `base_terrain`; the effect command's `template_id` / `class_id` from the
   applier 0x0045d6a0; the sound variant's `chance` and the item's
   `stop_time` from 0x004a2370 / 0x004a2310). Fields no translated function
   reads now carry neutral offset names (`field_2c`, `field_176`,
   `sound_164`, the tech's `field_22` / `field_24`, ...). Generic names that
   match their use (`hit_points`, `line_of_sight`, `train_time`) were kept.
   The colour table and the random-map records keep their earlier names
   (the latter were already offset-named from 0x0046da10).
2. **Background knowledge.** The assistant that wrote much of this code has
   general knowledge of Genie-engine formats from its training, which cannot
   be separated out. Where a layout mattered it was confirmed against the
   executable; item 1 lists where that is not yet recorded.
3. **The executable's own copyright.** This audit removes reliance on other
   open-source projects. It does not address whether a function-by-function
   translation of Microsoft / Ensemble Studios' executable can be released
   under an open licence; that is a question for legal counsel before
   publication.
