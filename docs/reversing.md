# Binary reconstruction notes

Input: `/mnt/d/Age of Empires/Empires.exe`, PE32 i386, linked September 1997.
These addresses use image base `0x00400000` and describe this specific binary.
SHA-256: `3ee26948e322a49121378cc6413d62b8e67d162d1654146e38f90f11fd48937e`.

## Rise of Rome target

The primary port target is the Rev 1 *Rise of Rome* expansion together with
the base game. The expansion disc image under `/mnt/e/Downloads/` has one
MODE1/2352 data track and 14 audio tracks. `tools/cdtrack_tool.c` verifies
sector sync and the ISO9660 primary descriptor while extracting the data
track to ignored `build/ror/track01.iso`. The ISO contains
`GAME/EMPIRESX.EXE`, `GAME/DATA2/EMPIRES.DAT`, three expansion DRS archives,
and four `.CPX` campaigns. `Empiresx.exe` SHA-256 is
`8e0e6d047b2f90d0769fb5a1b16c1b1973582efdbe73a5d11395a43557215845`.

`tools/ExportAoEDecompilation.java` made one-pass reference exports of all
currently discovered functions: 4,821 from `Empiresx.exe` and 5,440 from
`Empires.exe`, all reported decompiled. Searchable outputs and their current
`*_functions.tsv` address/line indexes are under ignored `build/decompile/`.
The initial exports covered only 4,059/4,486 functions; subsequent vtable
and function-pointer audits recovered additional starts. Decompiled C still
has guessed types, unresolved indirect calls, and Windows-specific
dependencies. These dumps do not establish that the game has been faithfully
reconstructed or can be compiled for Vita.

The expansion DAT still says `VER 3.7`, but inflates to 1,415,153 bytes
instead of the base file's 963,580. The common parsers currently reach map
geometry: 234 sounds/379 variants, 941 graphics, 23 enabled terrain slots,
seven border slots, and 64×32 tile geometry. These matching early layouts
do not prove later data structures or executable decisions are identical.

The four expansion `.CPX` files begin with `1.00`, then a scenario count at
`0x104`. A table of 0x208-byte records starts at `0x108`; each record gives
an embedded scenario's byte length and offset, followed by title and
filename fields. `aoe_cpx` extracts all 19 `.scx` files. Each starts with
scenario version `1.11` and a 1.15 player-data body. Unlike the installed
base multiplayer scenarios, every campaign map has an embedded 628×278
8-bit DIB in the optional player-data field. Base executable `0x004a33a0`
reads the 14-byte wrapper, then `0x004a37d0` rounds width to a four-byte
row stride and reads `height*stride + 0x428` DIB bytes. The portable parser
preserves the image and reads all 19 campaign scenarios through their final
player-detail record.

Expansion executable `0x00500b30` registers its `data2` sounds, graphics,
and interface DRS archives before the base `data` sounds, graphics, terrain,
border, and interface archives. `0x0046c190` appends each to a linked list
at `0x007c4b24`; `0x0046c450` walks that list from its head and returns the
first matching type/ID. `drs_find_first` reproduces this ordered lookup.
The expansion graphics archive contains 153 SLP resources and overlaps 82
IDs in the base graphics archive. A test confirms expansion ID 149 takes
priority and base-only ID 0 remains available. All 2,600 expansion
graphics frames and 299 expansion interface frames decode with the shared
SLP reader.

Expansion executable `0x004493a0` computes the same elevation-neighbor
shape codes 0–16 as base `0x00428970`, with the same branch precedence and
edge fallback. Its caller `0x00448cf0` stores the shape at tile `+4` and
then calls `0x00445880` to update tile origins. That projection uses the
same half-width, half-height, elevation step, and seven extra-height shape
codes as base `0x00424a60`. Expansion `0x004496b0` computes the same
style-0/style-1 border patterns as base `0x00428ce0`; helper `0x00449b10`
normalizes neighbors using the same invalid/disabled-border conditions as
base `0x00429130`. Thus the portable shape, border, and projection routines
are backed by both executables. Border calculations passed on all 19
expansion campaign maps with the expansion DAT. The host tool also projects
every tile origin on all 25 base/demo and 19 expansion maps under sanitizers.

## Archive startup and teardown

- `0x004018a0` calls `0x00506b50` five times for `sounds.drs`,
  `graphics.drs`, `Terrain.drs`, `Border.drs`, and `Interfac.drs`. The calls
  pass `tribe` as the expected archive signature. The sound archive uses a
  different last flag than the four graphics archives.
- `0x00506b50` constructs the data path, opens or maps the file, registers
  an archive node in a linked list at `0x00566ad0`, compares the signature
  at archive offset `0x2c` with `tribe`, and compares version bytes at
  offset `0x28` with `1.00`.
- `0x004014c0` calls `0x00506d70` to unregister the same five archives.

## Resource lookup

`0x00506e10` walks the archive list. Within each archive it reads a table
count from offset `0x38`, table records beginning at `0x40` with stride 12,
then index records with stride 12. It first compares the four-byte table type,
then compares the numeric resource ID. A matching index gives offset and
length. The table type bytes appear backwards when read as text on a
little-endian machine: for example SLP is stored as `" pls"`, WAV as
`" vaw"`.

The host/Vita reader in `src/drs.c` implements that lookup with file offsets
and adds bounds checks for damaged or incomplete archives. The binary's loader accepts memory mapped
and buffered modes; this reader uses buffered I/O for portability.

`0x00506f10` wraps lookup: it returns a pointer into a mapped archive or
allocates a buffer and reads the resource in buffered mode. `0x0042b8e0`
tries type `" phs"` first and then `" pls"` for graphics. On the SHP path
it compares the resource's first four bytes with `1.10`. `0x0044cdb0` requests
type `" vaw"` for sound. These call sites confirm the type byte order used
by the reader.

## SLP graphics and palette

The `" pls"` resources in this release begin with `2.0N`. Resource 0 in
`graphics.drs` has one 116 × 67 frame; its frame header points to row outlines
and command offsets. The portable decoder in `src/slp.c` handles color runs,
transparent runs, player-color runs, shadows, and outline markers. It
successfully decoded every frame in this installation's SLP archives:

| Archive | Sprites | Frames | Decode failures |
| --- | ---: | ---: | ---: |
| `graphics.drs` | 683 | 12,675 | 0 |
| `Border.drs` | 6 | 280 | 0 |
| `Interfac.drs` | 98 | 774 | 0 |
| `Terrain.drs` | 4 | 90 | 0 |

`Interfac.drs` type `"anib"`, ID 50500, is a JASC 256-color palette. The
first rendered graphic (ID 0) is tree artwork. The command layout was checked
against both the game data and the [openage SLP format reference](https://github.com/SFTtech/openage/blob/master/doc/media/slp-files.md).

The original executable's detailed SLP pixel drawing routine and palette
transform behavior still need further tracing; the current renderer is a
portable reconstruction of observed asset behavior, not a complete rendering
of every original effect.

## Game data decompression

The installed `data/Empires.dat` is a raw DEFLATE stream with no zlib wrapper.
Inflating 181,610 bytes yields 963,580 bytes beginning with `VER 3.7` and a
NUL terminator. `0x00502260` opens the file through `0x00465280`, reads eight
bytes through `0x004654e0`, and compares them with the binary's `VER 3.7`
string at `0x005668f4`. The read path uses a 64 KiB buffer and calls a
decompression routine at `0x00531a9b`. The shared loader in `src/dat.c`
reimplements this with zlib raw inflation. Detailed rule structures inside
the decompressed data still need to be mapped.

The first 12 decompressed bytes are the eight-byte version and two little
endian 16-bit counts: 11 terrain restriction tables and 23 terrain types.
One 32-bit pointer placeholder per table follows. The next 11 × 23 floats
contain only 0.0, 0.2, and 1.0 in this file. At offset `0x42c` comes a
16-bit count of 11 color records. Each 36-byte record contains a 30-byte
name, a sequential 16-bit ordinal, a zero 16-bit field, and a 16-bit value
whose role is still unconfirmed. The records run from `Blue.col` through
`Shadow.col`; the next section begins at `0x5ba`. `dat_parse_intro` exposes
this section with bounds checks, including on Vita.

At `0x5ba`, a 16-bit count of 212 sound records precedes variable-length
records. Each has a 10-byte header: sequential 16-bit ordinal, a zero 16-bit
field, a 16-bit variant count, and a 32-bit pointer-like value. Each variant
is 19 bytes: 13-byte NUL-terminated WAV filename, 32-bit DRS resource ID,
and 16-bit weight. There are 335 variants in this file; the weights sum to
100 for 208 of 212 records. Four records have only one variant with a lower
weight, so their selection behavior still needs tracing. Some ambient WAV
variants use `0xffffffff` as their resource ID and likely follow a different
loading path. The table ends at `0x26e1`. `dat_parse_sounds` preserves these
fields and checks record bounds; a former internal browser displayed counts.

At `0x26e1`, 847 graphic slots begin with a 16-bit count and an array of
847 four-byte pointer placeholders. Slots 837 and 839 have null pointers;
the remaining 845 records start at `0x341f`. Each graphic has a 77-byte
fixed header with a 21-byte name, 13-byte filename, 32-bit SLP resource ID,
layer/player-color and placement fields, animation frame/angle counts, timing
floats, a sequential graphic ID, and a mirror byte. A 16-byte record follows
for each graphic delta. When the attack-sound flag is set, each angle adds
three four-byte delay/sound-ID pairs. Parsing every record by these counts
ends at `0x16618`; the first is `Arrow`, graphic ID 0, SLP 243, and the last
is `ZCarFight`, graphic ID 846, SLP 713. `dat_parse_graphics` exposes the
metadata and nested records. The former browser resolved SLP previews to their
graphic names using this table. The original executable's exact playback and
animation behavior remains to be traced. The layout was cross-checked against
the [openage graphic schema](https://github.com/SFTtech/openage/blob/master/openage/convert/value_object/read/media/datfile/graphic.py)
and [genieutils graphic reader](https://github.com/Tapsa/genieutils/blob/master/src/dat/Graphic.cpp),
then validated against this installed DAT's sequential IDs and final offset.
Of 710 distinct nonnegative SLP IDs in these records, 681 occur in
`graphics.drs`; 29 do not occur in any of the four graphics-bearing archives.
Some are named unused-looking variants, while 41 records use SLP ID 0 and
many of those have blank names. Consequently, a DAT graphic reference alone
does not prove an SLP is available; resource lookup must check the DRS index.

The following terrain block starts at `0x16618`. Its 140-byte header has six
32-bit map fields, 19 six-byte tile-size triples, and a two-byte pad. The
triples give base tile dimensions such as 64 × 32 and elevation deltas of
0, -100, or +100. Then come 32 fixed 408-byte terrain records, ending at
`0x199a4`. The first 23 are enabled, matching the terrain count in the DAT
intro; the other nine are empty. Records include names, SLP and sound IDs,
five palette indices for minimap colors, passable/impassable terrain links,
animation fields, 19 elevation-frame triples, 32 border IDs, and 30 terrain
unit placement entries. `dat_parse_terrains` exposes these fields and checks
the enabled count. The four nonnegative direct terrain SLP IDs (15000–15003)
all exist in `Terrain.drs`; other terrain types appear to derive their art
through replacement/border relationships, which still need executable tracing.
The record layout was checked against the installed DAT and the
[openage terrain schema](https://github.com/SFTtech/openage/blob/master/openage/convert/value_object/read/media/datfile/terrain.py).

## Original terrain artwork

`Terrain.drs` contains exactly four SLP resources. The DAT identifies them
as desert (`15000`), grass (`15001`), water (`15002`), and dark water
(`15003`). Decoding frame zero of `15000` yields a 65×33 isometric diamond,
not the flat terrain-color tile used by the discarded prototype. Most other
enabled terrain records have no direct SLP ID; their DAT records contain
replacement, frame-grid, border, and terrain-object fields. For example,
beach has replacement ID 6 (desert), while forest has replacement ID 0
(grass). The direct frame selector is now identified, but replacement and
blending rules remain incomplete; these fields are not a complete renderer.

All 90 frames in those four original terrain SLPs decode using only opaque
palette-color and transparent pixels: no player-color, shadow, or outline
operations occur. The portable `slp_blit_terrain_rgba` composites precisely
that subset into a clipped RGBA buffer using the original JASC palette,
leaving transparent pixels untouched and rejecting any unsupported pixel
kind before drawing. This is an asset-rendering primitive, not terrain
placement, border blending, or a runnable game renderer.

Rise of Rome `0x00447900` iterates the 32 runtime terrain records at stride
`0x198`. For each enabled record it allocates a terrain-art object, passes
the record's direct SLP ID to the sprite loader at `0x004a0b80`, then
initializes that object at `0x004a0fb0`; a load failure clears the record's
enabled byte. The portable `terrain_art_load_direct` follows the verified
direct-art association using the existing DRS precedence and SLP decoder,
and validates all 17 DAT shape spans against the decoded frame count. On both
installed base and expansion DATs it loads terrain slots 0, 1, 6, and 22 as
SLPs 15001/15002/15000/15003 with 25/20/25/20 frames. It does not yet
resolve replacement terrains, borders, or the viewport renderer.

`0x004a0b80` first tries filename-based loose `.slp` and `.shp` files when
loose-file mode is active or the resource ID is negative. Only if those fail
and the ID is nonnegative does it query DRS for SLP, then SHP. The expansion
DAT's other 19 enabled terrain slots have ID `-1` and filename `None`, so
they do not provide a direct DRS sprite. Their visual treatment must be
derived from the terrain/replacement/border draw path, not by assuming the
direct-art cache represents every map terrain. `0x004479d0`, the text-data
terrain loader, constructs each frame triple's starting frame cumulatively
from `frame_count * angle_count`; this independently supports the DAT frame
span interpretation. The flat variant rule is traced below.

The original map reader `0x00424cb0` stores terrain ID in the low five bits
of tile byte `+5` and elevation in its high three bits, then calls
`0x004282e0` and `0x004286f0`. The first recomputes each tile's elevation
shape byte at tile `+4` via `0x00428970`; the second updates terrain borders
at tile `+6` via `0x00428ce0`. `src/terrain.c` now reproduces the
`0x00428970` eight-neighbor decision, yielding shape IDs 0–16. It uses the
same branch precedence and edge-neighbor fallback as the executable. A host
test covers all 17 codes; the function runs over all 25 installed/demo
scenario maps and cross-compiles for Vita.

In the expansion, region update `0x00448cf0` recomputes tile shape byte `+4`,
then calls projection `0x00445880` and tile-marking function `0x00445760`.
The latter walks 24-byte tile records, writing `0xcc` at byte `+7` and its
fifth argument at byte `+10`. This establishes an invalidation/update mark,
not the direct frame rule. The code that consumes these marks has not yet
been identified.

The region-paint admission gate at expansion `0x00449110` (base
`0x00428700`) reads the requested terrain's enabled byte from the runtime
`0x198`-byte record. If disabled, it reads the immediate `replacement_id` at
record offset `0xb8` and proceeds only if that replacement's enabled byte is
exactly one. `terrain_paint_target_available` ports this guard, with a bounds
check on the replacement that the original assumes from valid DAT data.
After admission the function edits packed tile terrain IDs, recalculates
borders, and marks the affected region; it is an edit/update path, not the
viewport frame selector.

The border table starts immediately after the 32 terrain records at DAT
offset `0x199a4`: 16 fixed 1,440-byte records, seven enabled, ending at
`0x1f3a4`. Each has a 66-byte header, 19×12 six-byte frame triples, and
three closing 16-bit fields (draw terrain, underlay terrain, style). The
original `0x00429130` normalizes a neighboring terrain ID to the center ID
when their terrain-to-border lookup is invalid or points to a disabled
border. `0x00428ce0` chooses the first differing normalized neighbor in
N/S/W/E/NW/NE/SW/SE order, then stores the border slot in the low nibble of
tile byte `+6`. Style 1 uses a four-bit N/S/W/E adjacency mask in the high
nibble; other styles choose one of twelve ordered corner/edge patterns.
`dat_parse_borders` and `terrain_border_code` now implement these layouts
and decisions. Unit tests exercise all twelve style-0 patterns and style-1
mask cases; all 25 original/demo maps pass host and sanitizer checks, and
the code cross-compiles for Vita.

The 20-byte DAT map-geometry header follows the border table at `0x1f3a4`.
It reports 23 enabled terrain slots, seven enabled border slots, 64×32
tiles, 32×16 half-steps, and a 16-pixel elevation step. Executable
`0x00424a60` projects each tile's origin as `((x+y)*32, (y-x)*16 -
elevation*16)`, with an additional 16-pixel upward shift for elevation
shapes 2, 6, 8, 10, 14, 15, and 16. `dat_parse_map_geometry` and
`terrain_project_tile` reproduce this behavior; tests cover flat, raised,
shaped, and edge tiles. This is positioning only, not the sprite renderer.

The terrain-only minimap color path is shared by both games: expansion
`0x0042d610` dispatches elevation-shape codes 0–16 through the jump table
at `0x0042d93c`; base `0x00456d03` uses the identical table at `0x00457090`.
The selected branches read runtime terrain bytes at `+0xb4`, `+0xb5`, or
`+0xb6`, corresponding to the first three DAT `map_colors` bytes within each
`0x198`-byte terrain record. In shape order, the chosen DAT color slots are
`1,2,0,0,2,2,0,2,2,2,0,0,2,2,0,2,0`. The portable
`terrain_minimap_palette_index` implements this selection; `scn_tool` uses
it for terrain-color inspection images. This does not reproduce minimap fog,
objects, visibility, or the main viewport renderer.

The terrain block continues 48 bytes beyond that geometry header. A 32-bit
random-map count and pointer placeholder follow: five maps in the base DAT,
ten in the expansion. Each map has a fixed first-pass header and a second
pass containing land, terrain, unit, and elevation entries; the portable
`dat_parse_random_maps` bounds-checks both passes. The next section begins at
`0x20ecc` (base) or `0x2598c` (expansion), with 140 or 218 technology effects.
`dat_parse_effects` reads their 31-byte names and 11-byte commands (type,
target unit, unit class, attribute, float amount). Both effect sections end
at the 16-bit civilization count: 13 at `0x23f48` in the base DAT and 17 at
`0x29e92` in the expansion DAT. `dat_parse_civilizations` traverses every
present unit record, including the type-specific moving, action, combat,
missile, creatable, and building tails, reaching the next technology section
at `0xe9517` (base) or `0x1576c3` (expansion). It exposes core unit fields;
the type-specific tails now expose moving graphic/speed, task target/work
records, attack/armor class amounts, range/reload/projectile values, and
resource cost/training fields. Missile and building-specific tails are still
traversed without decoding their parameters. This is static data, not a
combat/task simulation. Gaia has 299/374 present base units or 332/400 expansion units; each
other civilization has 175/374 or 203/400. All present IDs equal their slot.
The original scenario player-1 setup entry is index 0; player 0 uses Gaia.
`unit_refs_tool` uses this mapping and found zero missing DAT definitions for
every placed unit in all 44 installed/demo/expansion scenarios. This validates
the ID link, not the unit rendering or simulation behavior.

The final DAT section begins with a 16-bit technology count at `0xe9517`
(base) or `0x1576c3` (expansion). Each AoE I/RoR record has four 16-bit
prerequisites, three five-byte resource costs, required-count and research
location/time fields, an effect ID, UI IDs, and a length-prefixed name.
`dat_parse_techs` reads 118 base or 127 expansion records and ends exactly
at each decompressed file's EOF (`0xeb3fc` or `0x1597f1`). All nonnegative
prerequisite IDs, effect IDs, and Gaia research-location unit IDs resolve.
This establishes the data links only; technology availability, ordering, and
effect application still need executable tracing.

The six SLP resources present in `Border.drs` are 20000–20004 and 20006;
enabled "Non-visible" border slot 1 refers to absent 20005, so a DAT
reference alone cannot imply drawable art. Border placement/compositing and
terrain-object drawing remain incomplete.

Expansion border-frame selector `0x00517880` and base `0x004bbcc0` use the
same lookup. The tile-draw caller supplies the low-nibble border slot,
high-nibble pattern, elevation shape, and tile coordinates. The selector
flattens the DAT border frame table: nonzero pattern `p` accesses entry
`shape * 12 + p - 1`. Patterns 13–15 therefore reach the following stored
row; this is visible in real style-1 border data, not a parser offset error.
For a multi-frame entry, style 0 varies patterns 11/12 by tile Y and 9/10
by X; style 1 varies patterns 2/3 by Y and 1/4 by X. The selected SLP frame
is `frame_start + variant * angle_count`. Portable
`terrain_select_border_frame` implements this for nonzero patterns and
nonnegative map coordinates. A zero pattern is rejected because the original
offset points into non-frame header bytes; its draw-path handling still needs
checking. Real base and expansion DAT queries for style-1 slot 6, shape 0,
pattern 13 both select SLP frame 4. This selects a frame number, not border
art loading or compositing.

Expansion `0x00447cd0` mirrors direct terrain loading for 16 border records.
Portable `border_art_load_direct` uses the archive-ordered SLP lookup and
decodes all six resources present in `Border.drs`, validating every nonempty
frame span reached by shapes 0–16 and patterns 1–15. Each base and expansion
DAT yields four 36-frame border sprites and two 68-frame sprites. Slot 1's
missing SLP 20005 is left without art, as the original loader disables a
record when sprite initialization fails. All six resources use only opaque
palette-color and transparent pixels, so the existing clipped RGBA primitive
can composite their pixels. Full viewport placement and visual effects are
not yet implemented. Use `dat_tool FILE --border-art BORDER.DRS`
to audit.

The DAT's terrain elevation-frame triples confirm the shape lookup: for
grass and desert, shape 0 has nine frames beginning at SLP frame 0, while
shapes 1–16 each have one frame at indices 9–24. Water and dark water have
four base frames and shape frames at indices 4–19. Each sum equals the
respective SLP's 25 or 20 frames. The third value of each six-byte triple is
therefore represented as `frame_start` (formerly mislabeled `shape_id`), and
`terrain_shape_frame_span` exposes the original DAT-declared range for a
shape. The actual selector is `Empiresx.exe` `0x00517790` (base
`Empires.exe` `0x004bbbd0`), called by expansion tile-draw function
`0x00516710`. For a multi-frame shape it reads the low bytes of the two DAT
fields at terrain-record offsets `+0xba` and `+0xbc`: row and column counts.
The frame variant is `(tile_y % rows) * columns + (tile_x % columns)` when
either count exceeds one, with a zero count contributing zero. An index
outside the shape's frame count falls back to variant zero. The SLP index is
`frame_start + variant * angle_count`; single-frame shapes use variant zero.
Thus grass/desert tile flats repeat a 3×3 grid and water/dark-water a 2×2
grid. `terrain_select_frame` implements this rule for nonnegative map
coordinates, and `dat_tool FILE --terrain-frame TERRAIN SHAPE X Y` audits
both real DATs. It selects a frame number only; replacement, layer planning,
and hotspot-aware compositing are separate routines described below.

The expansion viewport caller `0x00514c10` (base `0x004b8e80`) reads the
raw tile terrain ID, checks that terrain record's `replacement_id` at runtime
offset `+0x144`, and substitutes it once when it is not `-1` before calling
the tile drawer. `terrain_resolve_draw_id` reproduces this single-step rule:
the real beach ID 2 draws through desert ID 6, and forest ID 10 through
grass ID 0 in both DATs. The drawer `0x00516710` (base `0x004ba930`) first
checks the selected terrain's enabled/art state. For an available style-0
border, it uses the border record's `draw_terrain` byte at offset `+0x59a`
to decide whether the base terrain is drawn before the border; all installed
style-0 borders set it to zero. Style-1 borders retain the base and draw
component frames in mask-bit order 1, 4, 8, 2 (frame patterns 1, 3, 2, 4).
`terrain_build_draw_plan` now records those ordered terrain/border frame
layers without drawing pixels. It has synthetic ordering tests and
cross-compiles for Vita. `0x004a1460` subtracts SLP frame-header hotspot
coordinates at `+0x18/+0x1c` from the screen anchor before issuing the draw;
`terrain_blit_draw_plan_rgba` reproduces that placement for the original
terrain/border assets' palette-color and transparent pixels. It prevalidates
every layer before changing the target buffer. Synthetic layer, hotspot,
clipping, and failure-atomicity tests pass on host and under sanitizers, and
the library cross-compiles for Vita. The viewport caller reads signed tile
origins from runtime map records `+0/+2` and subtracts renderer pixel-camera
offsets `+0x130/+0x134`; `terrain_project_tile_view` exposes that transform.
Its outer sweep starts at renderer signed tile fields `+0x12a/+0x12c`,
iterates `(short)[+0x118] * 2 + 12` diagonals of `(short)[+0x116] + 5`
cells, advances x and y together within each diagonal, then alternates
decrementing start-x and incrementing start-y. Negative map coordinates are
skipped; encountering a nonnegative coordinate beyond map width or height
ends that diagonal. `terrain_walk_view_tiles` reproduces this ordering and
map-bound rule. These routines do not derive camera state from the scenario,
check the viewport pixel rectangle, or draw any game objects. Complete
viewport/camera placement, fog/palette modes, objects, and gameplay are still
unimplemented.
The whole-program call-site export records the viewport chain
`0x00512d40` → `0x005136b0` → `0x00514c10`. In the same upstream update,
`0x00512d40` calls `0x00513440`, which writes camera pixel offsets
`+0x130/+0x134` and starting tile fields `+0x12a/+0x12c`. The base update
counterpart is `0x004b76b0` from `0x004b6f10`. These routines compare
floating camera positions with a cached pair before recomputing the sweep;
their x87-to-integer inputs are not represented by Ghidra's argument-free
`__ftol()` pseudocode. Disassembly resolves them: each camera float is
truncated toward zero to get a tile coordinate, and its fractional x/y
parts project to `(fraction_x + fraction_y) * tile_width / 2` and
`(fraction_y - fraction_x) * tile_height / 2`, each truncated toward zero.
The `__ftol` helper at expansion `0x005283e0` explicitly sets the x87
rounding mode to truncate before `fistp`. `terrain_compute_view_camera`
implements the resulting pixel offsets and sweep-start fields for a supplied
camera position and viewport rectangle. It does not own the runtime camera,
screen surface, visibility state, or complete viewport draw.
The upstream renderer binding `0x00512760` stores its world pointer at
renderer `+0x100`; `0x005126a0` copies the map pointer and DAT geometry into
renderer fields `+0x104/+0x108..+0x110`. The world camera setter
`0x0045e290` writes the float pair at world `+0x118/+0x11c`, clamping each
axis to `[0, map_dimension-1]`; the base setter is `0x00517c30` with fields
`+0x114/+0x118`. Expansion `0x005125a0` (base `0x004b6670`) then derives
renderer tile counts `+0x116/+0x118` by integer division of viewport width
and height by DAT tile width and height. `terrain_clamp_camera_tiles` and
`terrain_compute_view_camera` expose this setter/count arithmetic. Their
synthetic tests do not validate an actual runtime camera or finished screen.
`terrain_draw_view_terrain_rgba` now connects that camera state to the
original diagonal tile order, shape/border plan, projected anchor, hotspot,
and SLP palette compositor. It clips to a caller-supplied RGBA viewport
rectangle and rejects a camera computed for a different rectangle. This is
only the direct terrain/border layer; it does not reproduce viewport
visibility tests, fog palette choices, world objects, or UI. The real-asset
audit draws one fixed 640×480 terrain-only rectangle centered on each of
the 19 expansion, 24 installed base, and one demo scenarios under sanitizers;
each has visible opaque pixels. No output is compared with the original
game’s rendered screen, so this proves compatibility of the current layer,
not viewport fidelity.
The expansion viewport caller bounds-tests each tile before drawing:
projected x/y must not exceed the viewport right/bottom, and the viewport
left/top must be less than projected x plus full DAT tile width / projected
y plus `DatTerrains.tile_sizes[shape].height`. The tile record's `+4` byte
is the elevation shape used for that 19-entry size table. The terrain-only
pass now applies this cull; on expansion campaign 1_1 it reduces submitted
tiles from 630 to 341 while retaining a fully opaque 640×480 audit buffer.
All 44 real-file sanitizer viewport audits still pass.
The expansion viewport caller `0x00514c10` reads a 32-bit per-tile mask from
the runtime visibility grid. If it intersects the world's current-visible
mask at `+0xd8`, the tile mode is `0x0f`. Otherwise, intersection with the
world's explored mask at `+0xd4` (or a caller bypass gate) admits the tile;
it gets mode `0x80` when the map's fog flag `+0x8db9` is enabled, or `0x0f`
when fog is disabled. Other tiles get mode zero. Expansion
`0x0051a0d0` and base `0x00409f30` independently expose the same
visible/explored/hidden split. `terrain_view_visibility_code` ports this
classification; the current terrain-only compositor still assumes every
tile is fully visible and does not apply the `0x80` fog palette/edge path or
black out hidden tiles. The newer `terrain_draw_view_visibility_rgba` path
accepts caller-supplied runtime masks and uses this classification.
The renderer's edge generator at expansion `0x00511d70` (base
`0x004b5e50`) builds 256 entries from the four cardinal and four corner
neighbor-missing bits, assigning sequential IDs only to combinations where
a cardinal missing edge does not coexist with its adjacent corner bit.
Invalid combinations canonicalize by clearing those corner bits. This yields
47 patterns (IDs 0–46); the full-missing mask `0xff` canonicalizes to
`0xf0`, pattern 46. `terrain_build_visibility_edge_table` and
`terrain_visibility_edge_pattern` reproduce this rule and the neighbor
ordering from expansion `0x00516590` / base `0x004ba7a0`.
Both executables then load optional `TILEEDGE.DAT` offset tables: 17 shape
offsets, each leading to 47 pairs of optional stream offsets. The portable
`tileedge_parse` validates this index. The installed
base `data/TILEEDGE.DAT` is 147,304 bytes and has 1,581 nonzero offsets;
synthetic and real-file parser tests pass under sanitizers, and the library
cross-compiles for Vita. The extracted expansion disc does not contain a
separate `DATA2/TILEEDGE.DAT`. The fog/black edge streams and palette effects
are still not rendered.
The stream consumers at expansion `0x004a5690` and `0x004a56e0` walk each
non-null pointer as `(row, left, right)` byte triples until a row byte
`0xff`; they add tile screen x/y to the span before handing it to the
renderer’s inclusive scanline-interval routines. `tileedge_visit_spans`
exposes these triples with bounds checks, and `tileedge_apply_coverage`
performs the same inclusive add/remove operation on a clipped byte coverage
plane. Every indexed stream in the installed base file terminates and has
ordered left/right values: 1,581 streams and 45,367 spans pass the sanitizer
audit; rasterizing each one independently produces 856,575 covered pixels
in aggregate. This is a coverage primitive, not a fog-palette or complete
viewport renderer.
Expansion `0x00511d70` also loads optional `DATA2/BLKEDGE.DAT`: the same
17-shape header, but 47 single stream offsets per shape. Base
`0x004b5e50` has no black-edge file load. `blkedge_parse` supports that
single-stream layout, tested with synthetic bytes; the available expansion
disc and installed base directory do not contain the file.
At expansion `0x00514c10`, the disassembly at `0x00515294` and `0x005152b8`
shows that the two TILEEDGE streams feed distinct clip-plane objects at
renderer offsets `+0x178` and `+0x17c`; `0x005152de` and `0x005152fc`
subtract the same selected BLKEDGE stream from each active plane. Visible
tiles use both TILEEDGE sides at the visible-neighbor pattern when fog is
enabled, otherwise pattern 0. Explored-fog tiles use only the second plane,
with TILEEDGE pattern 0 side 0. The black pattern comes from the
exploration-neighbor lookup unless the exploration gate is bypassed.
`tileedge_build_view_plan` ports that selection, and
`tileedge_apply_view_plan` executes the add/subtract sequence on two
independent byte masks after validating both selected sources. The installed
TILEEDGE file passes an all-shapes/all-patterns paired-plane audit, yielding
856,575 covered pixels in aggregate. The visibility-aware viewport routine
now feeds both planes from a caller-supplied runtime grid; a running game
does not yet provide that grid.
Expansion `0x00516b40` draws the second clip plane first with mode 1, then
the first plane with mode 0 unless tile flags `0xa0` force mode 1. Flag
`0x10` instead draws once through the default clip, and flags `0x20`/`0x80`
change the palette-index fill constants before either plane. Disassembly
at `0x00516b79`–`0x00516d0b` confirms the call order and exact mask/fill
constants. `terrain_build_sprite_pass_plan` records those calls. The 8-bit
software blitter at `0x005459a0` chooses alternating 4-byte masks by row
before writing palette indices; `terrain_blit_draw_plan_passes_rgba` applies
the same mask/fill bytes to decoded direct terrain/border SLP colors through
caller-provided clip planes, then converts the resulting index through the
original JASC palette. The screen-origin parameters preserve dither phase
when a sub-viewport is drawn. This covers only the direct color/transparent
SLP subset; it does not yet receive live simulation state or verify
pixel-for-pixel equivalence against original screenshots.
`terrain_blit_draw_plan_passes_indexed` applies the same original pass order,
clip planes, and mask/fill operation while retaining 8-bit palette indices.
This makes the terrain result suitable as the destination for the recovered
sprite shadow lookup before final palette expansion. Synthetic tests compare
its fully expanded output with the RGBA path, and the real-asset audit checks
normal and dithered pixels for distinct terrain-frame sequences in base and
expansion maps. `terrain_draw_view_visibility_indexed` now reuses the same
viewport tile walk, visibility rules, edge masks, and draw plans as the RGBA
path. Base and expansion map audits compare painted viewport colors and tile
counts; hidden pixels remain untouched. Actual runtime visibility state and
unit-sprite composition have not yet been wired to that shared surface.
The object phase is a separate call in each edition's viewport tile loop:
Rise of Rome `0x00514c10` calls tile terrain `0x00516710` and then object-list
draw `0x00451fd0`; base `0x004b8e80` likewise calls `0x004ba930` and then
`0x0049ccd0`. The object-list functions walk the tile's existing linked list
in order and invoke each retained object's draw vtable slot `+0x0c`.
`aoe_tile_object_visit_draw` ports the expansion filtering: in the alternate
tile pass (state `0x80`), class 0 is skipped; class 2 is skipped when object
byte `+0x48` exceeds 2; class 3 draws only when its nested alternate graphic
exists, using object pointer `+0x90`; and class 4 is suppressed when a
multi-object tile list contains an object marked `0x19` at `+0x4e`.
`aoe_tile_object_visit_draw_base` ports the base routine, which has only the
class-0 and class-3 alternate-pass cases. The base actor graphic pointer is
`+0x108` versus expansion `+0x10c`. The separate `0x00452230` sorting routine
is used by selection paths, not called by the viewport draw traversal; it
must not be imposed on tile rendering. These filters are unit-tested and
Vita-cross-built but cannot render units until original live object lists,
per-object vtable drawing, and indexed viewport composition are connected.
The indexed viewport now exposes that exact post-terrain position through
`terrain_draw_view_visibility_indexed_with_tiles`. Its callback receives the
same projected anchor and runs for every projected non-hidden tile, including
tiles with no direct terrain art. `aoe_runtime_viewport_draw_tile_ror` binds
the callback to `AoeRuntimeMap.objects`, preserves linked-list order, derives
the alternate pass from runtime tile byte `+9 == 0x80`, and forwards common
live units into `aoe_live_unit_viewport_draw_ror`. DAT template byte `+0x65`
is retained as `DatUnit.draw_class`; class-specific `+0x90` alternate
graphics remain an explicit resolver boundary rather than a guessed field.
The real `EXPANS_1_1.scx` terrain audit invokes the composition seam for 263
projected non-hidden tiles without changing any indexed terrain pixel.
The real-asset audit now draws every distinct resolved frame sequence in all
19 Rise of Rome campaign maps through this new pass path. Its normal-color
first-clip output matches the existing portable compositor byte-for-byte;
the mode-1/dither pass is exercised under sanitizers but has no original
screen capture to compare against.
`terrain_draw_view_visibility_rgba` connects the original viewport tile
sweep, per-tile visibility classification and edge-pattern lookup, persistent
two-plane TILEEDGE coverage, ordered direct-art passes, and 8-bit mask/fill
index mapping. It accepts runtime masks and tile flags supplied by a caller,
and clears both clip planes at frame start as the original setup does. A
mixed visible/explored-fog/hidden test grid renders with original scenario,
DAT, DRS, palette, and installed TILEEDGE assets for all 19 expansion and
24 base maps under sanitizers. The grid is synthetic; this is still a
library audit, not a live simulation or a pixel-fidelity comparison.

`tests/terrain_draw_real.c` traverses each original scenario tile, rebuilding
its elevation shape and border code, then constructs a plan against actual
DAT/DRS terrain and border caches. It composites each distinct resolved
frame sequence once at a fixed test anchor using the base interface palette.
All 19 expansion campaign maps, 24 installed base maps, and the demo map
pass under sanitizers with zero tiles lacking a terrain/border layer. This
is a real-asset compatibility audit, not a viewport rendering comparison;
camera placement, fog, objects, and dynamic game state remain untested.

## Scenario maps

All 24 installed multiplayer `.scn` files start with the four-byte version
`1.11`, a 32-bit uncompressed-header length, and a header containing save
type, timestamp, a length-prefixed instruction string, victory type, and
player count. The remainder is raw DEFLATE. The decompressed body starts
with a next-unit ID and player-data version (1.15 in these files). It later
contains a map: 32-bit width and height followed by width × height triples
of terrain ID, elevation, and a zero byte. Observed map sizes are 120²,
144², and 200²; each terrain ID is below this DAT's terrain count of 23.
The layout was cross-checked against the
[genieutils scenario reader](https://github.com/Tapsa/genieutils/blob/master/src/script/ScnFile.cpp)
and [map reader](https://github.com/Tapsa/genieutils/blob/master/src/script/scn/MapDescription.cpp).

`src/scn.c` reads the header and decompresses the body portably. For body
version `1.15`, it now traverses the original pre-map record layout directly,
then validates the width/height and tile grid. This reached the same map
offset in all 24 installed scenarios, including unaligned offsets. The
Some player-data fields are not yet surfaced as game objects; unsupported
embedded bitmap variants and other body versions fail closed. The 19
expansion campaign DIBs are preserved and parsed.
The host `scn_tool` uses the original shape-dependent DAT terrain minimap
color and the JASC palette to produce a PPM preview. That preview verifies broad map structure,
but terrain blending, placement, visibility, units, and screen orientation
still require reconstruction from the executable.
The former Vita asset browser used the same reader for a scenario-map
inspection view. It was removed from the deliverable because it was not the
original game.

In this `Empires.exe`, `0x004d45c0` opens a candidate scenario and reads its
first four bytes. At `0x004d4644`–`0x004d469a`, it compares that value with
known scenario versions `1.03` through `1.11`, including the `1.11` constant
at `0x00564638`. Recognized versions dispatch through offset `0x34` of the
global manager vtable at `0x005509e0`, which points to `0x004d4730`. That
method allocates a small metadata object and calls `0x0045db40` to read the
scenario header; it is not the full map loader. `0x004d8f50` uses this path
to query a field at offset `0x0c` in the returned metadata. This establishes
that the installed `1.11` files use a version the executable explicitly
accepts, but the full player-data/map load path remains to be located.

A separate scenario-open path at `0x00504420` also reads a four-byte version.
For `1.09`, `1.10`, and `1.11`, it dispatches through its object's vtable
offset `0x98`, then calls offset `0xa8` and updates three manager fields via
`0x004d48b0`. The related path at `0x005041d0` opens a named scenario and
uses the same version dispatch. Both object vtables (`0x0054d938` and
`0x00551e48`) point their `+0x98` slot at `0x00505da0`. That routine reads
the variable-length header, then makes additional stream reads and object
calls; it is the concrete full-load target for reconstructing the remaining
scenario body. Its layout has not yet been translated into portable code.
The common `+0x70` method at `0x005046f0` delegates to the map loader at
`0x00424cb0`. That loader reads two 32-bit dimensions, allocates the map,
then reads three bytes per tile. It stores the first byte's low five bits as
terrain and the second byte in the upper three bits of the engine's packed
terrain/elevation byte; the third byte is read but not used in this routine.
This directly corroborates the map triple recognized by `src/scn.c`. The
remaining work is to interpret the preceding player-data fields and the
post-map objects, not merely skip them to reach the map.
The `+0xf4` call immediately before the map differs by object type:
`0x00462900` delegates to `0x0051dd50`, while `0x00501f60` delegates to
`0x0045ddf0`. `0x0051dd50` itself begins by calling `0x0045ddf0`, so both
paths share the same base pre-map reader. That routine reads the scenario-body
version, fixed player arrays, length-prefixed strings, and nested records
through `0x005126d0` and conditionally `0x004a33a0`. The installed bodies
report version `1.15`; `src/scn.c` now exposes this field. These nested
records were traced for the installed `1.15` variant. `0x005126d0` reads an
8-byte header and 30 bytes per nested record. `0x004a33a0` reads a 14-byte
header and an optional DIB; all installed base multiplayer scenarios have
zero bitmap dimensions, while the 19 expansion campaign maps have the
628×278 8-bit bitmap documented above.
The base reader first consumes sixteen 256-byte player-name fields and sixteen
four-word player setup records (enabled, human, civilization ID, and an
unidentified word). The enabled flags sum to the uncompressed header's player
count in all 24 installed scenarios. It then consumes ten length-prefixed
strings, 48 player strings,
and sixteen groups of three length-prefixed data files. `0x0051dd50` adds
14,272 bytes of version-1.15 fixed records. The resulting direct stream
position matches every installed map offset.

## Disc audio availability

The user-supplied `F:` mount is a virtual DVD with a CDFS data volume. Windows
MCI reports one track, so this mounted image exposes no CD audio tracks to
extract. `F:\GAME\SOUND` contains ambient/taunt WAV files, not the game's CD
music; copies of these WAVs already exist in the installed game directory.
The soundtrack cannot be imported from the currently mounted image. A rip
retaining its audio tracks (for example, BIN/CUE or separately ripped audio)
would be needed before the Vita port can include the original music.
The separately supplied *Rise of Rome* Rev 1 BIN/CUE is such a rip: its 14
audio tracks (02–15) were extracted by `tools/extract_ror_audio.sh` as
lossless 44.1 kHz stereo PCM WAVs in ignored `assets/ror_audio/`. Track 02's
cue INDEX 01 follows a 75-sector INDEX 00 pregap, which the extractor skips.
These are local source assets; Vita playback has not been implemented.
The `E:\msaoe.exe` demo installer separately supplies 12 Standard MIDI
music/cue files (`Music1.mid` through `Music9.mid`, `Open.mid`, `Won.mid`,
`Lost.mid`) and six ambient PCM WAVs. They are extracted under the ignored
`assets/demo_audio/` directory and are not retail CD-audio recordings.

## Post-map scenario records

Immediately after the tile triples, all 24 installed scenarios store the
32-bit value 9. In `0x00505da0`, the map read is followed by a player reset
loop (`0x00519420`), a 32-bit player-count read, per-player `+0x68` vtable
readers, object-placement loops, and later `+0x70` readers. `0x00519420`
only resets a player helper object; it performs no scenario stream reads.
The next 128 bytes are eight sets of food/wood/gold/stone floats. Then each
of nine players, including neutral player 0, has a 32-bit unit count and
that many 23-byte placement records: X/Y/Z floats, spawn ID, 16-bit object
ID, state byte, and rotation float. This matches the seven direct reads in
`0x00505da0`'s object-placement loop and the installed scenario data: for
example, Border Patrol (4-player) contains 10,012 neutral objects and seven
units for each active player. `src/scn.c` now loads these records; all 24
scenarios pass host and AddressSanitizer/LeakSanitizer checks, and the loader
cross-compiles with VitaSDK. The following 32-bit value is again 9, then
eight player-detail records follow. Each contains a 16-bit-length name,
two camera floats, two camera shorts, an allied-victory byte, a 16-bit
diplomacy count, one byte and one 32-bit value per diplomacy entry, a float
victory format version, a 32-bit victory-condition count, a victory byte,
44 bytes per condition, and (at version 1.0) two more 32-bit point fields.
The loader now parses these records and reaches the exact end of the
decompressed body in all 24 installed scenarios. The `E:\msaoe.exe` demo's
`Multip_1.scn` is outer version `1.10` but also has player-data version
`1.15`; the same direct parser reaches its 144×144 map at `0x4bf9` and the
end of its body. Border Patrol has two
victory-condition records per player; most other installed multiplayer
scenarios have none. All 25 files pass AddressSanitizer/LeakSanitizer and
Vita cross-build checks. Their gameplay semantics and the original rendering
and simulation are not yet implemented.

The expansion's equivalent scenario body loader is world vtable `+0x98`,
`0x00522be0` (vtable `0x0054fe58`), not the base loader address
`0x00505da0`. Its placement loop at `0x00522d4c`–`0x00522e08` reads the
same 23-byte records in order. For a mapped, non-null player it writes the
record's spawn ID to world `+0x70`, sets byte `+0x74` to 1, invokes player
vtable `+0x60`, then clears the byte. The expansion player vtable
`0x00549c70` resolves that slot to `0x0045d1c0`; base player vtable
`0x00552378` resolves it to `0x00516af0`. Both select a unit definition from
player `+0x24` using the record's 16-bit object ID, call the definition's
factory vtable `+0x18` with player and X/Y/Z, then branch on the record's
state byte: 0–2 invokes a unit method with rotation, 3–6 another state
method, and 7+ a third. The expansion uses unit vtable offsets `+0xb4`,
`+0x6c`, `+0x68` respectively; base uses `+0xac`, direct `0x004166c0`,
`+0x64`. Assembly, including each callee's `ret 0x18`, was needed because
Ghidra's decompiler misidentified the six-argument player method signature.
`aoe_scenario_spawn_one` preserves this wrapper/branch order against parsed
`ScnUnit` records. Its callbacks mark the unported factory and unit methods;
it does not instantiate actual gameplay units yet. Unlike original assembly,
it safely skips state methods when the supplied factory returns null.
The source-backed `unit_refs_tool --spawn-states` audit over all 19 extracted
expansion scenarios counts 150,062 records in the 0–2 branch, none in 3–6,
and 1,850 in 7+. Type 10 accounts for 1,718 of the high-state records and
type 90 for 114; this campaign coverage does not remove the middle branch
from the required full-game implementation. The RoR live-unit vtables show
why the callback boundary cannot yet be replaced with a single state-byte
assignment. Following each template's `+0x18` factory through its final
live constructor gives this complete dispatch inventory (original x86
addresses; columns are vtable `+0x5c/+0x68/+0x6c/+0xb4`):

| Template type | Live vtable | Setter | High-state | Middle-state | Low-state |
| --- | --- | --- | --- | --- | --- |
| 10 | `0054ce80` | `004abdf0` | `004abb80` | `004abbc0` | `00407370` |
| 20/25 | `00547b28` | `004abdf0` | `004abb80` | `004abbc0` | `00407370` |
| 30 | `005496e0` | `004abdf0` | `004abb80` | `004abbc0` | `0044ca40` |
| 40 | `005473c8` | `004abdf0` | `00405c80` | `004abbc0` | `0044ca40` |
| 50 | `00547fa0` | `004abdf0` | `00405c80` | `004abbc0` | `0044ca40` |
| 60 | `00549488` | `004abdf0` | `00405c80` | `004abbc0` | `0044ca40` |
| 70 | `0054d370` | `004b0250` | `00405c80` | `004b02a0` | `0044ca40` |
| 80 | `0054d0c8` | `004ae500` | `004ae450` | `004b02a0` | `0044ca40` |
| 90 | `0054d608` | `004b25f0` | `004abb80` | `004abbc0` | `00407370` |

The corresponding cleanup vtable slot `+0x08` has a separate class split:
types 10, 20/25, 30, and 90 use common `0x004a80e0`; types 40–70 use
`0x00405a70`; type 80 uses `0x004ae110`. The type-40–70 method conditionally
performs derived cleanup at live `+0x184`, then calls common cleanup. Type 80
conditionally releases its `+0x1c0` task state, adjusts a global per-player
counter when its task-state byte is 4, optionally processes `+0x1cc`, then
chains through `0x00405a70`. `AoeLiveUnitPlayerUpdateBinding` composes this
dynamic-class slot: types 40–70 use a translated action-list prefix followed
by `common_cleanup`; type 80 uses its translated building-queue prefix, the
same inherited action-list prefix, then `common_cleanup`; root classes call
`common_cleanup` directly.

The dependency vector feeding that update path now has its recovered writer,
not only its pruning reader. Patched RoR `004abc70/004abc80` tests whether
live `+0x68` is nonzero, then replaces the `+0x64` ID array: active count is
reset, negative IDs are skipped, the first occurrence of each nonnegative ID
is retained, capacity grows by exactly one 32-bit element when needed, and
the two supplied 32-bit values are stored at `+0x78/+0x80`. A negative input
count takes the executable's empty-success branch; a null array or zero count
fails before mutation. `aoe_live_unit_set_dependencies_ror` preserves those
rules while failing safely on allocation failure. The second trailing field
remains opaque 32-bit bits rather than being widened to or interpreted as a
host pointer. Active count and capacity are now `uint32_t`, and allocation
overflow is checked against the Vita/original 32-bit byte domain even when
tests run on an LP64 host. The common constructor initializes `+0x78 = -1`
and the exact `+0x80 = 0x40a00000` bit pattern.

The adjacent single-ID methods are translated as well. Patched `004abdf0`
rejects every call once active count reaches 25 (including an already-present
ID), otherwise returns success for a duplicate or appends with the same
one-element capacity growth; its second stack argument is confirmed unread by
raw disassembly. Patched `004ac030` has a non-obvious behavior preserved by
`aoe_live_unit_remove_dependency_ror`: it searches and shifts the entire
allocated capacity, not merely active count, so a stale inactive slot can
produce success and decrement the count with a floor at zero. An empty result
sets source `+0x78` to -1. Patched `004ac1b0` clears count/source while
retaining allocation. Tests pin the 25-entry limit, rejection-before-dedup,
stale-slot removal, full-capacity shifting, and retained allocation.

Patched `004abea0` now composes those primitives as
`aoe_live_unit_propagate_dependency_ror`. Raw call-site disassembly resolves
the decompiler's unstable prototype: the target ID must resolve before any
mutation and caller active count must be below 25; each current ID is copied
to a fixed 25-entry local vector; every resolved member other than the caller
receives the target through `004abdf0`; then target is appended and the caller
is rebuilt through `004abc80` with its own ID at `+0x78` and the supplied
32-bit context at `+0x80`. Nested add return values are intentionally ignored.
Resolution remains an explicit semantic callback, so a registry entry is not
blindly container-cast into a host `AoeLiveUnit`. Tests pin missing-target
rejection, self exclusion, member propagation, order, and metadata.

Patched `004ac0a0` (group remove) and `004ac1d0` (group clear) are translated
as `aoe_live_unit_remove_dependency_group_ror` and
`aoe_live_unit_clear_dependency_group_ror`. Removing an ID equal to both the
caller's ID and its `+0x78` source tail-calls `004ac1d0`; otherwise each
resolved non-self member gets an inline capacity scan/shift with a floored
decrement that, unlike `004ac030`, never clears the member's `+0x78`, and
then the caller is handled by `004ac030`. `004ac1d0` applies `004ac1b0` to
every resolved non-self member and then to the caller. Patched `004abd80`
(`aoe_live_unit_set_dependency_group_ror`) is thiscall, but raw disassembly
shows it uses the caller only to reach the world (`unit+0x0c -> owner+0x3c`)
for `005206d0`. A null list or zero count returns 0, a negative count returns
1, and every resolved listed unit, caller included, is rebuilt by `004abc80`
from the whole list.

These four operations are the only users of the patched command opcodes 8,
9, 0x0e and 0x0f in dispatcher `0x0042a180` (handlers `0042b460`,
`0042b520`, `0042b4a0`, `0042b4e0`). Raw disassembly of each handler shows
the same gate: resolve command `+0x04` through command-processor `+0x04`,
then compare the unit owner's `+0x4a` word with the zero-extended player byte
`+0x01`. A mismatch or failed lookup is a silent no-op. The builders
`0042bbe0`/`0042bcc0`/`0042bc40`/`0042bc80` calloc 0x74/0x08/0x10/0x0c
bytes: set stores context `+0x08`, up to 25 IDs at `+0x0c`, and the count
byte at `+0x70`; propagate stores context `+0x08` and target `+0x0c`;
remove stores the removed ID at `+0x08`. Neither the builder nor
`004abc80` bounds the set count, so `aoe_command_execute_dependency_ror`
rejects a count above 25 as a host-only bounds guard, not original behavior.
`src/command_dependency.c` encodes and decodes these records little-endian
byte by byte, so host layout and `size_t` width never reach the wire format.

Patched RoR 1.0a command opcode 5 (`0x0042ad10`, builder `0x0042bae0` called
from player vtable `+0xb4` = `0x0045e580`) is an 8-byte record: player byte
`+0x01`, resource byte `+0x02`, float `+0x04`. The handler indexes world
`+0x40` by the unbounded player byte and calls player vtable `+0x78` with the
zero-extended resource word, the amount, and flag 0. Earlier player-resource
notes quoted disc/unpatched addresses. The patched RoR player vtable is
`0x00544d18` (installed at `0x45b46f`, `0x45b6fa`, `0x45ba71`); its
`+0x74/+0x78` are `0x0045cfc0/0x0045cfe0`, and its `+0x7c` world-tick target
is `0x0045d040`. Patched AoE 1.0c's set/add pair is `0x0045e7a0/0x0045e7c0`.
Two derived RoR vtables, `0x00549a44` and `0x00549b80`, share the `+0x78`
override `0x004f6c60`:
- resource 3 with a nonzero flag first adds to raw value slot 49 (`+0xc4`)
  with no count check;
- resource `0x32` stores 1 at player `+0x104` and calls world `0x00520c60`
  (through player `+0x3c`), which resets per-player `+0xd4..+0x104` fields;
- every path ends in `0x0045cfe0`.
`aoe_player_resource_add_virtual_ror` and `src/command_player.c` translate
this; the slot-49 allocation check and the NULL-player rejection are
host-only guards. Which players are built with which derived vtable has not
yet been established.

Opcode 7 (tribute) is processor vtable slot `+0x00` = `0x0042ad40` in both
the base (`0x00543498`) and derived (`0x00549904`) processor vtables. Builder
`0x0042bb20` (slot `+0x0c`) writes sender `+0x01`, receiver `+0x02`, resource
`+0x03` and float `+0x04`. Raw disassembly shows:
- the gate `fld values[r]; fcomp amount; test ah,1`: it proceeds only when
  the sender's unchecked raw value is `>=` the amount, so NaN fails;
- sender `+0x78(r, -amount, 0)` (exact `fchs`) and receiver
  `+0x78(r, amount, 0)`, with no fee;
- game `*0x00580e38` vtable `+0x40(7, sender, receiver, r, __ftol(amount))`;
- receiver vtable `+0xe8(sender, receiver, 0x20a, r, __ftol(amount), 0)`.
  Base `0x0045e930` and derived `0x004f3350` implement that slot; Ghidra
  dropped the trailing zero argument.
`__ftol` (`0x0052602c`) truncates via a 64-bit `fistp` and returns the low
32 bits; `include/aoe_ftol.h` reproduces that, including integer-indefinite
inputs.

The derived processor's dispatch `0x004e83e0` (installed by `0x004e8390`
from `0x0050a090`; the base processor is built from `0x0051c790`) calls CRT
`rand()` once per command before routing, so command count affects the
shared seed. It overrides opcode 4 with `0x004e8570`, handles 0x64-0x6f and
0x77 itself (0x6c through its own vtable `+0x10` = `0x004e91b0`), and passes
every other value to `0x0042a180`. Patched CRT `srand`/`rand` are RoR
`0x00526053`/`0x0052605d` (seed `0x00560664`) and AoE 1.0c
`0x0051fc8f`/`0x0051fc99` (seed `0x005591ec`); the `aoe_rng` header had
quoted unpatched-release addresses.

The derived processor is the one RoR sessions run. Entry `0x00528f78`
calls WinMain `0x004436f0`, which allocates and constructs only the tribe
game (`0x004fdfa0`, vtable `0x0054a264`). Its world factory (`+0xbc` =
`0x004ff3c0`, 0x124 bytes) builds the tribe world `0x00509ae0` (vtable
`0x0054a4f8`), and that world's `+0x54` (`0x0050a090`) installs processor
vtable `0x00549904` at world `+0x58`. The base world vtable `0x0054a954`
(`+0x54` = `0x0051c790`) and base game factory `0x004198d0` exist, but they
are installed only as base-class parts of those constructors.

Opcode 4 (builder `0x0042ba90`, 0x14 bytes: int16 template index `+0x02`,
player `+0x04`, floats `+0x08/+0x0c/+0x10`) is handled in that session by
the tribe override `0x004e8570`, not base `0x0042acd0`:
- template `+0x68` (`elevation_mode`) equal to 1 forces z to 4.0f by
  writing the record;
- the template's vtable `+0x18` creates the object;
- if the new unit's template type is 80, `0x004ad7b0` gets 10000.0f of
  work.
`0x004ad7b0` adds construction work. With state `+0x48` nonzero it returns
1. Otherwise it clamps the work to the remaining `+0x15a` time, adds
`H*work/T` hit points (`+0x26`, clamped to H), accumulates `+0x1d4`, and
when the total reaches T calls vtable `+0x5c(2)`; NaN does not complete.
These are `aoe_command_execute_object_create_tribe_ror` and
`aoe_live_unit_add_build_work_ror`. Intermediates use double, on the MSVC
default 53-bit x87 precision-control assumption, which has not yet been
verified against the running game.

The root setter labels were unpatched-release addresses. Patched RoR root
vtable `0x00547da8` `+0x5c` is `0x004aa550` (disc `0x004abdf0`). Patched
AoE 1.0c's setter is `0x004a9430`: it was found by its unique `+0x48`
state-store shape, and it uses visibility slots `+0xd4/+0xd0` where RoR
uses `+0xdc/+0xd8`. In both games the state-3 branch tests the dependency
group: unit ID `+0x04` equals source `+0x78`, `+0x74` is nonzero, and
count `+0x68 > 1`. It is not an action list. The disc-address table above
(live vtables and their `+0x5c/+0x68/+0x6c/+0xb4` columns) remains
unpatched-release evidence.

That state-3 body is now `aoe_live_unit_dependency_handoff_ror`. Over the
source's IDs, re-reading count `+0x68` each step, it skips the source's own
ID. Each member that resolves, has state `<= 2` and a non-null `+0x74`
(getter RoR `0x00407a60`, base `0x004aa8b0`) gets `+0x78` = the first such
member's ID. That member then calls `0x00413050` (base `0x00412e40`) on its
`+0x74` object with the source's `+0x74 -> +0x12c` record. That call lazily
constructs its own 0x1c8-byte `+0x12c` record (`0x0040d2f0`) and copies the
old one in (`0x0040d670`). The source keeps its own `+0x78` and list.
`-1` doubles as the "no leader yet" sentinel, as in the executable. What the
`+0x74` object and its `+0x12c` record represent is not yet identified.

Tribe command 0x77 (`0x004e9450`, builder `0x004e9c20`, 0x0c bytes:
building ID `+0x04`, int16 unit `+0x08`, int16 count `+0x0a`) has no player
byte and no owner check. A positive count calls queue add `0x004adab0`;
otherwise `0x004add50` cancel receives the 16-bit negated count. Raw
disassembly of `0x004adab0`:
- A tail entry with the same unit ID gets up to `count` additions. Each is
  owner `0x004f15f0(unit, &slot, 1.0f, 1)` then `0x004f1750(unit, 1.0f, 1)`
  and a word increment.
- `0x004f15f0` always writes -1 (or the failing resource, or 0x20) to the
  argument slot, and event 0x7d reports that slot's low word.
- A failure with nothing added returns before the `+0x1d0` bump and event
  0x7c.
- A different unit ID first drains the whole queue with
  `0x004add50(head, 1)` (one unit type per building), then performs one
  up-front check. A full queue grows by 10 entries (`calloc`, copy,
  `free`; allocation failure returns silently), and the new entry is
  filled by the per-item loop. It is appended even when empty.
`aoe_building_queue_add_ror` and `aoe_command_execute_building_queue_ror`
translate this. The cost check and payment `0x004f15f0`/`0x004f1750`
are translated as `aoe_player_cost_check_ror`/`aoe_player_cost_pay_ror`
(see below), with queue-compatible adapters.

Patched RoR tribe-player cost methods (`this` = player; template from
player `+0x24[index]`, costs at `+0x148`, 6 bytes each, deduct byte
`+0x14c`; only the low byte of the 16-bit field is read):
- Check `0x004f15f0(index, *failing, mult, pop_flag)` writes -1 first.
  Gold (`values[3]`) acts as a substitution budget only when `values[29]`
  is at least the double 1.0 at `0x005425f8`; otherwise the budget is 0.0
  (`0x00542458`).
- Population (type 4) with `pop_flag` 0 fails with 0x20 when `+0x2c >=
  +0x80` (`values[11] >= values[32]`). Gold's own cost reduces the budget
  (fail 3 below zero).
- A short resource fails with its type unless its deduct byte is set and
  the budget covers the gap, which is then consumed. Costs are evaluated
  in order, and `amount*mult` is rounded to float.
- Pay `0x004f1750` re-checks with a NULL out-pointer. It then charges
  each deductible cost (population skipped for nonzero `pop_flag`) with
  an unrounded product. A cost above the value takes the remainder from
  gold and zeroes the value: gold is written first, so a gold cost ends
  at 0.
- Refund `0x004f17f0` adds `(float)amount` back for each deductible cost of
  a type >= 70 template, without the multiplier and including population.
  The production queue pays with `pop_flag` 1, so a cancelled queue item
  refunds a population unit that was never charged. This asymmetry is
  what the code does; whether other code compensates for it is not yet
  established.

Base opcode 0 (`0x0042a280`, used by both processors) is the unit order.
Its decompilation is unusable (registers are mistaken for pointers), so the
translation follows raw disassembly. The record has count `+0x01`, target
`+0x04`, x/y `+0x08/+0x0c`, issuing player `+0x10`, flag `+0x12`, masks
`+0x14/+0x18`, and IDs from `+0x1c`.
- A multiplayer validity gate can drop the command after storing CRT
  `rand()` in `0x00581ab0`, so dropped commands still advance the seed. It
  applies only when the target resolves, `0x0041b760` is true, the first
  unit's owner `+0x04` (`0x00454150`) is 0, the issuer is not the local
  player (world `+0x7c`), and the target type is 80/90/10/20.
- The gate drops the command when player `+0x104` is 0 while the flag is
  set, when the masks are disjoint, or when map cell
  `0x007d205c[__ftol(y)][__ftol(x)]` lacks mask `+0x18`'s high bits.
- With more than one unit whose min/max spread is at most 8.0 (double
  `0x005434a8`) on both axes, units keep their offsets from the centre.
  Coordinates are clamped to 1.0 when negative and to size-1 at or beyond
  the map edge (map `+0x08/+0x0c`). The target is kept only if its template
  `size[0]` or `size[1]` is positive.
- The min/max scan updates the maximum only in an `else` branch.
  Strictly decreasing coordinates therefore leave the maximum at 0, and a
  wide group can still move in formation. This is the original behaviour
  and is preserved and tested.
- Each unit runs `0x004ac320` (empty the `+0x54` group when owner `+0x04`
  is 1), vtable `+0x12c(1)`, `0x004ac280` for every other listed ID (an
  unbounded deduplicated vector at `+0x54/+0x58/+0x60`), then vtable
  `+0x98(target, x, y, 0)`.

For action-capable live classes, unit vtable `+0x98` is `0x00405d70`, which
calls `+0x1f8(target, x, y, z, 0)` unless the target is the unit itself and
then increments byte `+0x188`. The root `+0x98` (`0x00415300`) is empty.
`+0x12c` is `0x0044eb30`, which stores its byte at `+0x155`. Every
action-capable vtable found uses `+0x1f8` = `0x00405f20`. That function asks
the actor template's task list (`+0xd8`, list vtable `+4`) for the task an
order means, then builds actions.

There are two task-list classes:
- Base templates (factory `+0x38` = `0x0043e4d0`) select with `0x004e7dc0`.
- Tribe templates (template vtable `0x00549930`, factory `0x004ece50`,
  list vtable `0x005499ac`) select with `0x00508010`, falling back to
  `0x004e7dc0`.

The task loader `0x004e74f0` and the template loader `0x004413b0` fix the
offsets used:
- task: `+0x08` target class, `+0x0a` target unit, `+0x0c` enable
  targeting, `+0x0e` terrain, and `+0x10/+0x11/+0x12` from file bytes
  44..46;
- template: `+0x10` id, `+0x14` class, `+0x8c/+0x8f` from bytes 12 and 15
  of the `+0x64..+0x90` block.

The predicate `0x004e7f90` and both selectors are translated in
`src/unit_task.c`. The tribe selector covers drop-off (classes 4/21 onto own
buildings 109/103/68/45 by carried type), build (0x65), action 3 for
`+0x8f` = 7 targets, repair (0x6a) for damaged buildings other than id 50
and for ship classes 2/20/21/22, and priest convert/heal (0x68/0x69). The
priest branch never falls through to the base selector. `dat_tool --unit`
now prints the task and selector fields.

Base opcodes 1 and 2 are translated:
- Opcode 1 (`0x0042a730`, IDs at the unaligned offset `+0x02`) skips
  template 50 and otherwise runs `0x004ac320` and unit vtable `+0xa4`.
- Opcode 2 (`0x0042a780`) is opcode 0's simple path with `+0xa0` in place
  of `+0x98`. For action classes `+0xa0` is `0x00405ee0`, which calls
  `+0x1f8` with final flag 1.

The `+0xa4` stop variants are root `0x0045bcc0` (ret), action `0x004064e0`
(clear the `+0x184` list unless head `+0x2c` accepts), group `0x00426250`
(also `+0x74` object `+0x58(1)`), and building `0x004ae290` (also drain the
production queue through `0x004add50`). They are
`aoe_live_unit_stop_ror`.

The action-list labels were disc addresses. The patched list methods are
update `0x004043b0`, push `0x00404400`, append `0x00404450`, pop
`0x004044d0` and clear `0x00403e50`; patched `0x00403e70` is an action
factory switch.

Base opcode 3 (`0x0042a850`) repeats opcode 0's formation and simple paths
without the validity gate, ordering through `+0x9c`. After each order, a
unit whose `+0x74` group object exists calls three methods on it:
`0x0040f6c0(0x2c6)`, `0x0040f6d0(0x262)` and `0x00412f50(target ID, target
class, x, y, 0)`. The target ID and class are -1 when the target is missing
or has no positive size, and x/y are the coordinates the unit was ordered
to. In the simple path the order itself still receives the unfiltered
target. Opcodes 0 and 3 share one routine in `src/command_order.c`.

Base opcode 6 (`0x0042ae20`, int32 count `+0x04`, int32 pattern `+0x08`,
IDs from `+0x0c`) arranges a group around a leader. Nothing happens for
counts <= 1. The leader is the unit with the smallest unrounded
`sqrt(x*x + y*y)`, with the best value stored as a float. If the index-0
unit does not resolve, the original compares against an uninitialised
local; the translation then takes the first resolved unit, a documented
host deviation. Every resolved unit is prepared like an opcode-0 unit.
Non-leaders are then sent through `+0x9c(0, x, y, 1.0f)` to a point
stepped from the leader's position:
- 0: diagonal, steps 1.0;
- 1: anti-diagonal, steps 1.5;
- 2: row and 3: column, steps 2.0;
- 4/5: a zig-zag whose toggle alternates -/+ y and grows both 1.5 steps
  only on the + half (jump tables `0x0042b0ec`, `0x0042b104`).

The remaining base opcodes route through player virtuals that the base and
both derived player vtables share:
- **0x0a** (`0x0042b120`, builder `0x0042bcf0`, 0x28 bytes) resets the
  group of the unit at `+0x04`. The issuing player's `+0x34`
  (`0x0045beb0`) then receives 12 record fields; it forwards them to the
  unit's `+0x74` group object (`+0x10`) only when the unit is owned by that
  player.
- **0x11** (`0x0042b1a0`) is the multi-unit form, with IDs from `+0x24`.
  Coordinates whose bits equal `0xbf800000` (-1.0f) are replaced by the
  unit's own `+0x38/+0x3c/+0x40`.
- **0x0b** (`0x0042b290`) is resignation. Outside a session it calls player
  `+0x08` (`0x0045bd00`: set `+0x81`, `+4(2)`, and event 5 on the first
  resignation). In a session it marks game `+0xa15[slot]`, tears down a
  local slot, and resigns the player only when every slot 1..8 mapped to it
  (`0x0041c080`) has left.
- **0x0c** (`0x0042b3b0`) and **0x10** (`0x0042b3f0`) feed 3-byte records to
  unit `+0x1bc(record, 1)` through player `+0x48` (`0x0045bf90`).

Tribe opcodes 0x68/0x6a/0x6b/0x6d/0x6e/0x6f (`src/command_tribe_unit.c`):
- 0x68 (`0x004e8e20`) acts only when the world `+0x40` slot for player
  `+0x02` exists. Each unit (IDs at `+0x0c`) then runs `0x004ac320` and
  `+0x17c(__ftol(+0x04), __ftol(+0x08), 1)`.
- 0x6a calls unit `+0x244` on the unit at `+0x04`.
- 0x6b calls unit `+0x14c(+0x04, +0x08, 0, 1)` for the IDs at `+0x0c`.
- 0x6d writes the int16 at `+0x04` to unit word `+0x4c` (the carried
  resource type) for the IDs at `+0x08`.
- 0x6e runs repair-target `0x004b0870` with the source at `+0x04`. That
  method finds the actor's action-0x6a task matching the source (buildings
  take target class -1/0, other targets their class) and builds the action.
- 0x6f issues, for each unit, its owner's `+0x34` task (owner, unit,
  0x2d1, unit, owner, x, y, 0, 1.0f, 1, 0, 100) after `0x004ac320`.

Tribe opcodes 0x64/0x65 (`0x004e85e0`/`0x004e8780`; 0x10-byte records:
building `+0x04`, player `+0x08`, int16 item `+0x0a`, request `+0x0c`) are
train and research.
- If the building resolves and its active-action query `0x004ad870` (head
  action type) is neither 0x66 nor 0x67, the item's location is checked:
  train uses template `+0x15c`, which must equal the building ID or be -1;
  research uses tech tree `+0x224` entry `+0x28`, which must match exactly.
  On a match, the 0x50-byte train action `0x004b4f10` or the 0x48-byte
  research action `0x004b60b0` is added through building `+0x208`, and a
  group object hears `0x0040f6d0(0x26c)` and `0x00412f50(request, -1,
  -1.0f x3)`. A location mismatch does nothing.
- A missing or busy building defers to player `+0x10c` (train) or
  `+0x118` (research) with the raw building ID.

Tribe sessions also have a second tribute: opcode 0x6c, through derived
processor slot `+0x10` = `0x004e91b0`. It is built by slot `+0x14` =
`0x004e9a10`, the tribe override of the base `+0x0c` builder `0x0042bb20`.
The record is sender `+0x01`, receiver `+0x02`, resource `+0x03`, float
amount `+0x04` and float fee rate `+0x08`. Unlike opcode 7 it never
rejects the transfer:
- A negative or NaN amount becomes 0, and fee = rate*A.
- When A + fee exceeds the sender's stock S, fee = S*rate and A = S - fee.
- The receiver gains A, and the sender loses A + fee.
- Resource 0x35 records the balance: the sender gains the total, and the
  receiver loses A.
- Event 7 reports `__ftol(A)`, while the receiver notice reports the
  requested amount.
The decompiler misreads the subtraction operands as stray registers; the
translation follows the disassembly.

Tribe opcode 0x67 (`0x004e8eb0`, jump table `0x004e9104`) is game control.
Sub-code 0 changes diplomacy: it sets player `+0x88`, calls `0x0045da70`
with `__ftol` of the float stance, raises event 8, and refreshes the world.
`0x0045da70` writes stance byte `+0x84[other]` and relation dword
`+0x8c[other]`: 0 for gaia, 1 for self, 2 for stance 0, 3 for stance 1 and
4 otherwise. It then calls `+0x94(other, stance)` on every object in the
player's `+0x28` list. `0x0045da60` returns the raw stance byte, and the
task predicate and tribe selector read any nonzero value as hostile, so
stance 0 is allied. The other sub-codes are:
- 1: world `+0x18`;
- 2: player `+0x78` resource add;
- 4: game `+0x9a8`;
- 5: player `+0x108`;
- 6: cheat dispatcher `0x0050c570`;
- 7: world `+0x4c` object `+8(0x71, player)`;
- 8: out-of-sync handling (`syncerr%d.gam`).
`src/player_diplomacy.c` and `src/command_game.c` translate this; the
cheat dispatcher and sync-error screen remain callbacks.

Tribe opcode 0x66 (`0x004e8920`) places buildings. The record has builder
count `+0x01`, player `+0x02`, x/y `+0x04/+0x08`, int16 template `+0x0c`,
dword `+0x10` and builders from `+0x14`.
- Template `+0x20` checks placement and writes its out-value into the
  handler's argument slot. A failure is reported through player `+0x100`,
  but only when the first builder resolves.
- For template 109 whose own `enabled` byte (`+0x52`) is 0, a live town
  center found by `0x00451370` refuses the placement silently.
- Payment is `0x004f1750` with pop flag 0; a failed payment reports code 0.
- On success, template `+0x3c` creates the foundation, and every builder
  resets its group and gets `+0x98(foundation, x, y, 0)`.
`src/command_place.c` translates this. The translation starts the
out-value at 0 rather than truncating the host record pointer the original
would leave there.

Tribe opcode 0x69 (`0x004e8b00`) lays walls. The record has builder count
`+0x01`, player `+0x02`, endpoint bytes x1/y1/x2/y2 at `+0x03..+0x06`,
int16 template `+0x08`, dword `+0x0c` and builders from `+0x10`.
- If |dx| = |dy| the wall is one straight or 45-degree run. Otherwise it
  is an L: the longer axis is laid first from the start point, then the
  other axis from one cell past the corner.
- The run helper `0x004e8d00` steps -1/0/+1 per axis through both
  endpoints. Each cell is checked at its centre (+0.5f, `0x00542468`) with
  template `+0x20` arguments (NULL, 1, placement_class, 0, 1, 1, 1).
- Accepted cells are paid individually; a failed payment ends only that
  run. Pieces are created through template `+0x3c`.
- Builders are sent to the first piece created.

Unit vtable `+0x1f8` = `0x00405f20`, reached from `+0x98`/`+0xa0` (whose
final flag it never reads), is the order-to-action step. It is translated
as `aoe_unit_order_ror`:
- **Task selection and job switch.** The unit's list selects a task. If
  none fits and template `+0xec` (`task_swap_group`, the first of the five
  bytes after the drop sites) is set, the first owner template with the
  same class, type and swap group whose list yields a task becomes the
  unit's template (`+0x54`). In the RoR data, the Man, Builder, Fisherman,
  Forager, Hunter, Lumberjack and Miner templates all share class 4,
  type 70 and swap group 1: this is the villager job switch.
- **Reusing the head action.** A head action built from the same task is
  retargeted through `+0x34`. A type-9 head already aimed at the target is
  also retargeted and skips creation.
- **Building an action.** Otherwise the `+0x184` list factory builds one:
  the base list `0x00542600` uses `0x004040b0`, and the tribe list
  `0x005488a8` uses `0x004b4600`, which defers to the base. The routes are
  taken from the jump tables `0x0040434c/0x00404334` and
  `0x004b4c04/0x004b4bd4`:
  - base: 4 point; 5 target/point; 7 attack with group notices; 0xa
    point; 0x15 task-only;
  - tribe: 3 target, even for NULL; 0xc point; target/point pairs for
    0x65, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6e and 0x6f; 0x78 unit-only.
  Allocation sizes and constructor addresses are recorded per route.
- **Fallback.** Failing all of that, the unit moves through `+0x9c` with
  result 1.
- **Acknowledgement.** Group objects acknowledge the result through table
  `0x00406264/0x00406230`.
- **Attack arguments.** The attack constructors take task range and
  template `min_range` (`+0x138`), `projectile_unit_id` (`+0x124`) and
  `frame_delay` (`+0x12a`).

The move fallback `+0x9c` = `0x00405db0` has three steps:
- It ignores a self-target, and it stops when the head action's `+0x30`
  absorbs the move (`0x00404550`).
- Otherwise it allocates a 0x44-byte move action (type 1, vtable
  `0x005426dc`): `0x004052d0(unit, target, 0.4f, 0)` or
  `0x00405250(unit, x, y, z, 0.4f, 0)`.
- A group object is told (0x2c6, 0x262, target, x, y, z). The action is
  adopted and `+0x188` incremented.

Base actions (vtable `0x005429d8`) are initialised by `0x00407050`. Their
target setter `0x00407380` calls `+0xc4(unit)` on a released target and
`+0xc0(unit, type, 0)` on an attached one, and keeps `+0x18` in sync.

The move action translated in `src/action_move.c`:
- **Defaults.** A non-positive range becomes 0.4, and the graphic defaults
  to template `+0xbc` or `+0x18`.
- **Activation** (`0x00405430`): in range gives state 1; speed `+0xfc` 0
  or NaN gives 0xd; otherwise 3.
- **Update** (`0x00405500`, jump table `0x00405888/0x00405874`):
  - **Validation.** A lost or dead target is cleared. The update then
    returns 5 when quiet; otherwise it sends notice 0x1fb with the reset
    ID and returns 4.
  - **State 3.** It finishes if already in range. Otherwise it records the
    destination, sets range, target ID and size on the unit's movement
    system, and enters state 4 when `0x0044bf00` reports 0.
  - **State 4.** A target that drifted more than 1 tile forces a re-path:
    notice 0x1fc, state 3, then two nested updates. Here dx is compared
    unrounded and dy after float rounding. Otherwise movement status 0
    arrives at the point (result 3), and status 2 arrives near the target
    (result 2).
  - **States 1 and 0xd.** These report done (1) and arrival (3).

The movement system's setters `0x0044be90..0x0044bee0` only store the
destination (`+0x11c..+0x124`), range (`+0x134`), target ID (`+0x138`)
and target size (`+0x13c/+0x140`). Movement start `0x0044bf00`:
- A unit whose template `+0x38` is <= 0 (or NaN) gets a direct
  start-to-destination path. `+0x38` is the third DAT size float (height):
  `0x00404a60` adds half of it to `z` for a centre point, while the speed
  getter `0x0044ead0` reads `+0xb8`. An earlier note here called `+0x38` the
  speed.
- Otherwise the unit must pass its owner's path budget
  (`0x0045c180/0x0045c1c0`). Without budget, `+0x156` counts the retry and
  the call returns 2.
- With budget, the request goes to the global pathfinder object
  `0x006a1cc0`.
  `+0x156` resets and `+0x144` becomes the intermediate-waypoint count
  `+0x16c` plus 25 (owner player `0x00454150` returns 0) or 15. The search
  runs from the unit's tile to the intermediate tile at `*(+0x168)` (when
  `+0x16c > 0`) or the destination tile, with range `+0x134`, target
  `+0x138`, mode 1, no distance output, budget-limited, occupancy checked,
  step 1 and unit queries disabled (`-1, -1`).
- A failed search that leaves a one-waypoint path, for a unit not centred
  on its tile (fraction 0.5 on both axes) that fits at the centre
  (`0x0045a4f0(.., 1)`), moves the unit there through vtable `+0x34` and
  returns 2. Otherwise the path cursor rewinds and advances (`0x00458440`,
  `0x00458450`) and the call returns 0. The moving branch sits behind a
  constant test (`mov ecx,0x0044f1a0; test ecx,ecx`) whose dead side
  returns 1.
- `aoe_movement_start_ror` (`src/movement.c`) translates this, and
  `tests/movement_test.c` covers each branch.

The movement update `0x0044c2d0` (RoR 1.0a) runs once per tick:
- It measures the destination distance with the template footprint
  (`+0x30/+0x34`) plus the target size (`+0x13c/+0x140`) subtracted per
  axis and clamped at 0. A positive range `+0x134` that covers this
  distance stops the unit: `+0x88 = 0`, status `+0x148 = 1`, vtable
  `+0x1c0(0)`, `+0x58 = 0`, return 2.
- Under 0.5 tiles it steps straight to the destination through `0x0044cde0`,
  unless the unit already stands on its current waypoint (return 2 with no
  status change).
- Otherwise it advances past a waypoint it stands on (placing the unit there
  when `0x0045a4f0` fits) and steps toward the current waypoint.
  `0x0044cde0` results:
  - 0 keeps moving (status 2, return 1);
  - 1 also clears `+0x88`;
  - 5 stops;
  - 2 advances the path;
  - 3, 4 and 6 give up. Giving up clears `+0x88`, sets status 2, and
    returns 0 without intermediate waypoints or adds 5 to `+0x144` and
    returns 1.
- An exhausted path drops intermediate tiles (3 bytes each at `+0x168`, via
  vtable `+0x1c4(0)`) until one lies beyond footprint radius + range + 0.3
  tiles, then re-paths to it:
  - The re-path needs the player budget and increments counter
    `+0x11dcc0`.
  - From the first exit only, `+0x144` is decremented and gives up at 0.
  - The destination still counts as reached at radius + range, plus 3
    tiles when the next two intermediate tiles share a terrain-restriction
    zone (`0x004ab9e0`) and the distance exceeds 4.
- `aoe_movement_update_ror` translates this, with `0x0044cde0` as a callback,
  and `tests/movement_update_test.c` covers it.

The intermediate waypoint list lives in live vtable slots `+0x1b8..+0x1c4`
(base `0x00542748`, shared by six moving-unit vtables):
- **Entries.** Each entry is 3 bytes at `+0x168`, with count `+0x16c` and
  capacity `+0x170`.
- **Get** `0x0044eb40` (`+0x1b8`) returns entry i.
- **Append** `0x0044eb70` (`+0x1bc`) first clears the list when `+0x174` is set.
  `0x0044edd0` sets that byte; an earlier note mislabelled it "explored".
  Storage grows to 5 entries and then doubles, with zero fill.
- **Clear** `0x0044ed20` (`+0x1c0`) zeroes the count. With argument 1 it
  also calls unit `+0x74`'s vtable `+0x58(0)`.
- **Remove** `0x0044ed50` (`+0x1c4`) shifts later entries down by one.
  Index 0 therefore removes entry 0, but index k > 0 removes entry k − 1.
  Movement only uses index 0.
- These are translated as `aoe_movement_waypoint_*_ror`.

The tick toward a point `0x0044cde0` (RoR 1.0a):
- **Speed.** A zero speed `+0x88` restarts at template speed `+0xb8`
  times a positive sprite multiplier (sprite `+0x64`), and the heading is
  then aimed from the tile centre. The move distance is speed times world
  `+0x84`.
- **Blocked wait.** A wait `+0x130` drains by the move distance, and while
  it is positive the speed stays 0. On expiry it becomes −1.
- **Turning.** With template `+0xc4` > 0 (the DAT moving-unit float after
  the two walking graphics, a per-facet turn delay), the turn timer
  `+0x12c` drains by the tick. The facing `+0x9c` turns one facet
  (`6.2831853 / facets`, sprite `+0x60` or 8) per delay while the facet
  fraction exceeds 0.8 (0.9 after a re-path).
- **Facet helpers.** `0x0044ef90` wraps angles and snaps them to facets,
  reusing one stack slot for both the facet step and the best difference.
  `0x0044bbd0` writes the facet frame to unit `+0x35`.
- **Blocked step.** When the next step does not fit (template height
  `+0x38` > 0) and no wait is running, a wait of
  `rand() % 6 * 0.1 + 0.5 / speed` starts. After an expired wait the unit
  re-centres and returns 5 if the target is within footprint radius +
  range + 0.3. It returns 2 if the goal is occupied. Otherwise it spends
  `+0x144` (4 at zero) and the player budget (1 without), then re-paths
  (4 on failure).
- **Moving.** The unit snaps onto its waypoint when overshooting it at
  distance 0, or when within 0.5 tiles while oscillating between two
  facets (`+0x17c/+0x17d`). Constant `0x00544a78` is a denormal double
  holding the float bits of 0.25, so the overshoot test means distance 0.
  `+0x178` records the pre-move distance to the waypoint.
- **Slope speed.** Speed is multiplied by 0.9 when climbing more than 0.02
  and by 1.03 when descending. Within that band it resets to the base
  speed. Either way it then takes the terrain multiplier `0x004aa880`.
- `aoe_movement_step_to_ror` and `aoe_movement_tick_ror` translate this,
  and `tests/movement_step_test.c` covers it. Host libm atan2/sin/cos may
  differ from x87 in the last bit.

Occupancy and fit checks (RoR 1.0a):
- **Saturating counts.** The occupancy bitmap holds a saturating 2-bit
  count per quarter cell. `0x00458870` increments it up to 3, and
  `0x004588d0` decrements it while positive (clear masks `0x00557b2c`).
- **Restore and lift.** Unit restore `0x004ab6e0` adds its footprint and
  lift `0x004ab7f0` removes it. The footprint is
  `[ftol((x - sx) * 4) >= 0, ftol((x + sx) * 4))` on each axis. Only units
  with template height `+0x38` > 0 take part, and live `+0x84` and `+0x85`
  track the current and previous state. Selector `+0x11dcf4` chooses the
  main pathfinder (0), the secondary one at `0x00583bc8` (1), or both.
- **Callback names.** The pathfinder callbacks previously named
  register/unregister are `lift_obstruction`/`restore_obstruction`: the
  search lifts its own unit's footprint and restores it afterwards.
- **Fit check.** `0x0045a4f0(unit, x, y, check_terrain)` computes the
  quarter footprint with negative low edges nudged up by one. It runs the
  terrain sweep with no start-tile exemption, then the occupancy sweep
  with the unit lifted. Touching the last search's target rectangle (target
  `+0x11dc4c`, now kept in `AoePathfinderRor.target_id`) counts as not
  fitting.
- `aoe_path_fits_ror`, `aoe_path_occupancy_add/remove_ror` and
  `aoe_unit_obstruction_restore/lift_ror` translate these, with tests in
  `tests/pathfinder_search_test.c`.

The attack action (RoR 1.0a, vtable `0x005423f8`, action type 9) is routed
from task action type 7 to constructors `0x00401150` (target) and
`0x00401290` (point). Base template offsets used here are confirmed by
loader `0x004413b0`: `+0x10` ID, `+0x14` class, `+0x30/+0x34/+0x38` size.
- **Fields:**
  - graphics `+0x40/+0x44/+0x48` (move, attack, idle);
  - range `+0x4c` (default template `+0x114`, then 0.4) and minimum range
    `+0x50`;
  - projectile `+0x54` and fire frame `+0x56`;
  - firing `+0x58`, friendly `+0x59`, and the point-attack approach count
    `+0x5a` (starts at 2).
- **Set state** `0x00401520`:
  - 1/0xd/0xe show the idle graphic. Towers (IDs 79, 199, 69, 278) also
    clear their group target.
  - 4 spends an approach and pushes a quiet move sub-action.
  - 6 checks the range, then resets the reload (`0x00426a10`) and enters 7.
  - 7 starts firing.
- **Range check** `0x00401b60`:
  - The unit must be able to attack the target (`+0x1f0`).
  - Point attacks measure via `0x004aaf50`.
  - A target must be visible unless it is Gaia-owned or class 3/27; a
    fogged one needs draw class `+0x65`.
  - A target within 0.5 of the cached position counts as in range.
  - Otherwise the distance is measured via `0x004aaeb0` against the
    minimum range and against footprint radius + range + 0.3.
  - Too far approaches (4), or gives up (0xd) when the unit cannot move or
    its group holds (`0x0040f6b0() == 0x2d5`).
- **Update** `0x004017c0`:
  - It accumulates the tick at `+0x2c`.
  - A gone or dead target gives result 5. A refusing owner (player vtable
    `+0x1c`) or an attached target (`+0x20`) gives 4.
  - State 4 follows the sub-action, and state 7 reloads and fires at the
    fire frame (default half the sprite frames) through unit vtable
    `+0x228` (target) or `+0x224` (point).
- `src/action_attack.c` translates the class, and
  `tests/action_attack_test.c` covers it.

Combat core (RoR 1.0a; type-50 vtable `0x0054324c`, tribe `0x00547fa0`):
- **Reload.** `+0x7c` `0x004269f0` reports ready when the reload timer
  `+0x1b4` is <= 0. `0x00426a10` resets it from template `+0x120`.
- **Fire at target or point.** `+0x228` `0x00426520` (target) and `+0x224`
  `0x004266a0` (point) launch a projectile when template `+0x124` >= 0:
  - the spawn point is the template offsets `+0x12c/+0x130` rotated by the
    facing, with z offset `+0x134` (the DAT "graphic displacement", now
    parsed);
  - the launch radius is footprint + max range `+0x114` + 6.0;
  - the projectile itself is constructed via the owner's template list and
    `0x00449070` (target) / `0x00449120` (point).

  Otherwise `+0x22c` delivers a melee hit at the attacker's position.
- **Melee hit** `0x004267b0`. Blast templates (`+0x118` > 0) go to
  `+0x220`. Otherwise it notices 0x200 and rolls
  `rand() * 100 / 32767 <= accuracy` (`+0x126`; the magic-number division
  is verified for all rand values). A hit calls the target's `+0x70`.
- **Damage** `+0x74` `0x00426910` sums, per attack class,
  `(amount − armor) × terrain bonus × multiplier`, counting only positive
  terms:
  - armor is the last matching armor class entry, else base armor
    `+0x100`;
  - the terrain bonus is world `+0x54[+0x110][terrain]`.
- **Receive.** Base `+0x70` `0x00426890` subtracts the damage, notices
  0x1f4 with the truncated hit points and the template maximum, and clamps
  below 1.0 to 0. The tribe override `0x004ad9b0` skips dead targets and
  enforces a minimum of 1.0 damage (0.1 for attacker classes 0/23/26/36
  and tower-like IDs 79/69/199/383/380). It credits the attacker's player
  with `+0x78(0x2b, 1.0, 0)` when a non-Gaia target drops below 1.0.
- **Multiplier.** Base `+0xf0` is 1.0. The tribe `0x004aec00` gives ranged
  units 1.5 when `+0x250(target)` holds, and melee units 0.6666 when below
  their target's z.
- **Blast damage** `+0x220`: base `0x00426280`, tribe `0x004b04a0`. The
  call is `(x, y, z, source, excluded)` with radius template `+0x118`.
  - It scans tiles `ftol(x ± r) ∓ 2` (end exclusive, clamped, rows by y).
  - It skips the attacker, the excluded unit, and units whose blast
    defense `+0x8b` (DAT chunk byte 11, per loader order) is below the
    attacker's blast level `+0x11c`.
  - It hits when the squared distance to the victim's footprint edge is
    below r², subject to the accuracy roll.
  - The tribe version also spares same-owner units for attacker classes
    35/24, and calls `+0x84(0, 0, 1)` on victims left below 1.0.
- `src/combat.c` translates these, and `tests/combat_test.c` covers them.

Projectiles (RoR 1.0a). The fire methods create the projectile unit and
call `0x00449070` (target) or `0x00449120` (point). That pushes a type-8
flight action (vtable `0x0054267c`, constructor `0x00404990`, 0x54 bytes)
holding the aim point, the target and the launcher (secondary setter
`0x004073f0`), and stores the launch radius at projectile `+0x1bc`. Type-60
template fields come from loader `0x00440530`: flight mode `+0x148`, lead
`+0x149`, hit mode `+0x14a` and arc `+0x150`.
- **Activate** `0x00404a10` aims (3) unless it has no target and its point is marked
  -1.0.
- **Aiming** (state 3, `0x00404ab0`):
  - It targets the target's centre height.
  - With lead == 1 it leads a moving target by
    `t = d / (d − d' + speed)` when the denominator exceeds speed / 2.
    Launcher IDs 35/36/280 cap the target speed at 1.0.
  - Mode 0 stores the unit direction and mode 3 the whole offset. Modes 1
    and 2 do not move.
  - A positive arc sets gravity `−2·d·arc/(t/2)²` and climb `2·d·arc/(t/2)`
    with `t = d / speed`.
- **Update** `0x00404db0`:
  - It drops a vanished or dying target. When the launcher dies it first
    captures the launcher's attack data (`0x004495b0`).
  - Mode 0 moves `tick·speed` along the direction and expires at the
    launch radius. Mode 3 moves the full offset and strikes the target.
  - The overlap query `0x00449280` scans ±3 tiles for units whose
    footprints (plus the step) and heights (±0.1) overlap. Hit mode 0
    strikes only the target, 1 strikes foreign non-Gaia units, and 2
    strikes anything but the launcher, blasting at own or Gaia units.
  - The live launcher strikes (`+0x22c` / `+0x220`), otherwise the
    projectile itself. A landed projectile blasts and stops.
- `src/projectile.c` translates this, and `tests/projectile_test.c`
  covers it.

The pathfinder (vtable `0x00544cf0`, constructor `0x00458570`) works on
255 x 255 grids (bytes at `+0x10de28`, dwords at `+0xce624`, indexed
`[first*255 + second]`). Its distance table `0x006a18c0` holds
`__ftol(i*1.4142f*16 + 0.5)`: octile costs in sixteenths of a tile.

The search `0x00458930` takes 15 stack arguments, with integer
coordinates despite Ghidra's float types:
- Its entry checks compare all four coordinates with the width only.
- With range > 1, a goal closer than range*16 succeeds at once.
- A unit with speed 0 or NaN fails.
- Mode 1 clears the unit's active path (`+0xa4`, or `+0xd8` when `+0x154`
  is set) on every early exit.
The search body is translated from the disassembly in
`src/pathfinder_search.c`. The decompiler typed integers as floats and
merged registers here, so it was not usable as a source.

**Layout.** The pathfinder object holds:
- `+0x0c`: state bytes (generation tag | arrival direction);
- `+0xfe10`: f-costs;
- `+0x4f614`: a 1-based heap of {x, y, cost} entries, with heap[0] a
  -2,000,000,000 sentinel;
- `+0xce624`: the quarter-tile occupancy bitmap, 1020 rows of 255 bytes
  with 2 bits per quarter;
- `+0x10de28`: a per-cell penalty, scaled by 16;
- `+0x11dce4`: IDs of units met along the path.

**Setup.**
- The target rectangle in quarter tiles comes from the target's footprint.
- The unit's own obstruction is registered.
- The unit's sub-tile offset is computed, or set to 0.5 when the unit is
  centred.
- After ten retries, moving group members are registered as obstructions.
- The generation byte advances by 8. At 0xf0 it wraps to 0 and the state
  grid is refilled with 0xff.
- The goal radius is range*16, or step*16 when the step exceeds the range.
- Footprint offsets use rounding points traced from the x87 sequence
  (+100 and +0.00005 biases).
- The node budget is distance*2500/10 within 10 tiles and /100 beyond,
  capped at 100 for gaia; owner kind 1 always uses distance*25.

**Main loop.**
- It pops the head node and tests the Euclidean distance (sixteenths)
  against the goal radius.
- The pop's sift loop only runs while child < size/2, so small heaps are
  not re-sifted.
- Eight neighbours form a ring at the step stride. Each is pruned by the
  direction mask table `0x00557b34` and checked against bounds and the
  start cell.
- Its f-cost is parent - h(parent) + h(neighbour) + `__ftol(dist * 16)`.
- Its swept footprint (bounding box of parent and neighbour, plus
  offsets) is checked against terrain first. Negative low bounds are
  bumped by only one. Terrain is tested with byte-sized loop counters, the
  unit's free terrains and the overlay fallback (map `+0x3928`, stride
  0x5a0).
- Then occupancy: dword-pair span masks for spans up to 24 quarters, else
  per-cell checks. A footprint that touches the target rectangle ends the
  search with cost 0.
- Blocked cells may still be entered when unit queries (`0x00520590`) find
  only units. A penalty is then added: +0x3c when that unit's HP is at
  least 10, otherwise +0x10. Each cell adds its penalty grid value, and a
  direction change adds 1.

**Exits.** The search succeeds on reaching the goal or touching the target.
When the budget or heap runs out it falls back to the best node.

**Path builder** (`0x0045a970`). It walks the arrival directions back to
the start and prepends a waypoint on each direction change. A unit path
holds at most 1001 waypoints (`0x004581b0`). It also collects nearby unit
IDs.

The obstruction registration, footprint adjust, tile-centre check and unit
query remain explicit callbacks. The debug overlay (`0x007bf9b8`) is not
reproduced. Synthetic tests cover open ground, walls, a sealed goal,
in-range, generation rollover, occupancy blocking and target touching.
They test this translation's behaviour, not a trace of the original.

The type-40–70 prefix is now composed by
`aoe_live_unit_cleanup_derived_prefix_ror`: when semantic live `+0x184` is
present, it drains the list through `aoe_action_list_clear` (`0x00403e70`),
which unlinks and destroys actions and applies the original surviving-head
state rule. The type-80 prefix at `0x004ae110` is composed by
`aoe_building_cleanup_type80_ror` over `AoeBuildingUpdate`: it drains queue
actions `+0x1cc`, releases queue storage `+0x1c0`, resets the recovered
`+0x1c4/+0x1c6/+0x1c8/+0x1d0/+0x1d1` fields, and changes `+0x1d2` from 4 to
-1 while decrementing the selected per-owner counter. It then drains its
optional inherited `+0x184` action list. The type-80 live wrapper must bind
the building semantic state and its matching allocation-release callback;
the update binding resolves the owner counter.

The common cleanup maps disc `0x004a80e0` to patched `0x004a6920` in
`docs/ror_revision_map.tsv`. Patched decompilation confirms the same order:
when template `+0xa4` is set and the type-specific pool accepts the unit, it
exits active resource storage for states below 3, removes the unit from the
world's state list for states below 7, clears the action collection, removes
the owner-list link, detaches both associated object-list links, calls each
dependent object's vtable `+0xd4`, removes a visibility-watch node, runs the
template line-of-sight cleanup, and clears the template pointer for reuse.
Otherwise it invokes the live object's dynamic destructor with flag 1.
The destructor at disc `0x004a7d40` enters `0x004a7e10`, which also clears
pre-terminal state, sprite-manager ownership, owner registration, tile/list
links, dependent objects, visibility watch, and both dependency arrays. The
portable `aoe_live_unit_destroy_ror` currently covers owner/world registration,
state-2 vision departure, tile link, sprite instances, and its owned ID
vector; it does not yet implement those other original cleanup side effects
or the reusable-object pool path. Keep common cleanup mandatory until those
remaining lifecycle effects are translated and connected.
The exact pool eligibility branch is now exposed as
`aoe_live_unit_cleanup_uses_reuse_pool_ror`: template `+0xa4` plus dynamic
types 10, 20, 25, 30, 40, 50, or 60. Type 90 and the type-70/80 building
classes take the destructor route. This mirrors pool dispatch `0x00523400`
but does not enqueue or clean up the object.

A direct vtable dump confirms root and type-90 slots `+0x60 = 0x004abbf0`
and `+0x64 = 0x004abd10`. DAT bytes `+0xa5/+0xa6/+0xa7` are now retained
as notify mode, effect mode, and notify argument. The root pre-terminal slot
is an observable no-op when both mode bytes are zero. This permits the
portable high-state composition to bind the real player-list reclassification
and normal/alternate vision withdrawal without inventing an effect callback.
In `EXPANS_1_1.scx`, 35 of 37 state-7 records take that immediate no-effect
rule. The remaining two are Gaia tree templates with modes `1/2/0`, but their
live tile masks are zero. They therefore take no per-player notification
branches: `0x004abd10` adds a visibility-watch entry with the zero snapshot,
and `0x004abbf0` removes it. `aoe_visibility_watch_add` now ports
`0x00436dd0` including 32-slot growth, owner matching, duplicate rejection,
flag snapshot, and unit ID. All 37 campaign high-state records consequently
run their original branch.

For types 10–60, template byte `+0xa4` first tries world method
`0x00523510`, which selects a type-specific pool and reinitializes a
returned live object via its vtable `+0x04`. This is a reuse path, not an
alternate constructor family; the table above names the fresh-creation
vtable, and the pooled object's dynamic vtable must be preserved.

The live-vtable addresses in the preceding table are for the disc executable,
not patched 1.0a. Verified cross-revision mappings are kept in
`docs/ror_revision_map.tsv`; never infer them by address proximity. In
particular, disc root `0x0054ce80` maps to patched `0x00547da8`, while patched
`0x0054ce80` is RTTI data. Disc common player-list slots `+0x08 = 0x004a80e0`
and `+0x24 = 0x004aa790` map to patched `0x004a6920` and `0x004a8f40`.

All nine disc RoR live vtables share draw slot `+0x0c = 0x004a8a60`. The tile
list's `0x00451fd0` dispatches that slot after its own class-based filtering.
The shared method chooses the normal unit ID, or for class-code 25 skips an
active player in the unit's `+0xa4` mask and otherwise substitutes its
`+0x8c` controller's ID (or `-1`). It draws only while linked to a tile and
in state below 7. An action flag at `+0x36`, screen mode 3, or template
`+0xa8` bit 0 with screen subtype 7 invokes overlay `0x004a8d50` before
the unit's sprite-manager draw slot `+0x0c`. `aoe_live_unit_draw_ror` now
translates that order and gating with explicit overlay/sprite callbacks. It
also forwards the renderer and already-projected tile anchor and computes the
four sprite bounds with the original 16-bit wrapping additions of live-unit
fields `+0x28..+0x2e`. It forwards live frame byte `+0x35`, matching all seven
arguments at the sprite-manager call in `0x004a8a60`. The normal or
alternate graphic chosen by `0x00451fd0` is forwarded unchanged as well. It does not infer
the original camera projection, instantiate the per-unit sprite manager, or
complete the viewport draw, and is not a scene demo.
`aoe_live_unit_viewport_draw_ror` now joins the recovered tile traversal to
that draw method for persistent `AoeLiveUnit` objects. It retains the tile
filter's selected graphic and projected coordinates and resolves per-owner
color/type-25/screen fields through an explicit callback. The final backend
still needs to bind the sprite-manager operation to the indexed SLP blitter.
The root high-state method `0x004abb80` invokes an optional parent method,
an active/health toggle, and `+0x5c` state transition to 7; root middle
`0x004abbc0` clears health after that toggle. The root setter
`0x004abdf0` performs map-vision, sprite/action, and owner interactions
before changing its state byte. These effects remain incomplete in the
portable live object. The original placement-only audit stops before state
dispatch; the full-constructor audit now invokes only the source-complete
0–2 branch and reports 7+ records separately.
One directly recoverable dependency is now in `aoe_live_unit_core`:
RoR `0x004adc70` changes the unit's positive-health registration category.
A zero-to-one change sets byte `+0x4b` and defers reclassification; a
one-to-zero change with that byte set clears both bytes without player
calls. Otherwise it calls player `+0xe0` to remove the old category,
clears unit `+0x49`, and calls player `+0xdc` to register the new category,
storing its `+0x50` node. Required player callbacks preserve this ordering.
The root `+0x6c` method `0x004abbc0` first calls `0x0045ec70` when unit
flag `+0x36 & 1` is set, then composes that toggle with clearing HP at
`+0x30`. Base `0x004166c0` has the same conditional action-cleanup shape.
`aoe_live_unit_root_middle_state` now requires that action callback when
the flag is set. Derived `+0x6c` methods still need their extra effects, and
neither method is yet invoked by the live scenario runtime.
`aoe_live_unit_root_state_set_ror` now translates the root setter's outer
control flow: accept only a different state that is increasing or zero;
run template-effect exit and optional template `+0xa6 == 2` method when
crossing from state 0–2 to 3+; run pre-terminal `+0x60` for old state below
4 and new state 7+; change normal then alternate tile visibility before
writing the state; apply the conditional state-3 action-list callback; and
clear positive-health registration for new states other than 2 or 6.
The original `+0xa6` byte is now parsed into `DatUnit.state_effect_mode`.
The RoR binary constant at `0x0054d068` is zero, matching the line-of-sight
comparison used here. Each still-unported method is a required callback at
its exact call boundary. Type-70/type-80 setter overrides and the callback
bodies are not implemented, so this method is not yet called during scenario
construction or live update.
The base root vtable `0x0054a518` has `+0x5c = 0x004167f0`. Its setter
shares the normal/alternate visibility and health-registration sequence
and the conditional state-3 action work, but accepts *any* changed state;
it calls pre-terminal `+0x60` before the active-state exit helper
`0x00413440`, and has no `+0xa6 == 2` call in that setter. The portable
`aoe_live_unit_root_state_set_base` uses those edition-specific branches.
Base `0x00418890` matches the already translated health-registration
toggle. Both editions still require original callback bodies and real
owner/action bindings before they can drive live scenario units.
The `exit_active` helper is no longer opaque: RoR `0x004a8a00` and base
`0x00413440` both walk the template's three entries at `+0x6a` (resource
ID), `+0x70` (amount), and `+0x88` (mode). For mode 2 with a nonnegative
resource ID, they call player `+0x78` with the negated amount. The existing
DAT parser's three seven-byte `DatUnit.storage` records are exactly these
serialized fields, as confirmed by RoR template reader `0x00441fa0`.
`aoe_unit_storage_exit_active` now performs this original resource effect,
and both root setter paths call it at their edition-specific point in the
sequence. It does not implement the other state-transition callbacks.
The RoR unit vision hooks `0x004ac290` / `0x004ac380` also have an alternate
branch for nonzero mode. Assembly shows they require positive line of sight
and positive template sizes at `+0x30/+0x34`, then form inclusive rectangle
bounds by truncating `X-sizeX`, `Y-sizeY`, `X+sizeX-0.01`, and
`Y+sizeY-0.01` toward zero. `0x0051a5a0` / `0x0051a770` clip those bounds
and use the same signed-byte count, mask, tile-view, and explored-cell rules
as the circular branch. `aoe_visibility_rectangle_enter/leave` and
`aoe_live_unit_vision_change_ror` now port that path. This is a callable
visibility method, not yet a bound state-setter callback or full fog screen.

A previous seven-case reading of the unit-template construction path was
incomplete. The Rise of Rome world `0x0051ed20` reads the civilization count
and type byte, then calls world vtable `+0x2c` (`0x0050c6f0`). Type 1 creates
the derived civilization whose vtable is `0x0054edf8`. Its `+0x04` reader
`0x00450240` reads each present template's type byte and dispatches through
the derived `+0x08` method `0x004f1890`. That method handles 70 (`F`), 80
(`P`), and 90 (`Z`), delegating the other types to common `0x004502e0`.
The base game has the corresponding derived switch `0x0052a0e0` and common
switch `0x004acf20`. Original allocation sizes and constructors are:

| Type | Parent | Template bytes | RoR live bytes | Base live bytes | RoR constructor | Base constructor |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| 10 | — | `0xb8` | `0x88` | `0x88` | `0x00441c30` | `0x00457ce0` |
| 20 | 10 | `0xbc` | `0x8c` | `0x8c` | `0x0043f520` | `0x004f0530` |
| 25 | 20 | `0xbc` | `0x8c` | `0x8c` | `0x00440c00` | `0x00528040` |
| 30 | 20 | `0xd8` | `0x180` | `0x178` | `0x00441560` | `0x004d9280` |
| 40 | 30 | `0xfc` | `0x18c` | `0x184` | `0x0043ebd0` | `0x0046da90` |
| 50 | 40 | `0x148` | `0x1bc` | `0x1b4` | `0x0043f980` | `0x004a7e90` |
| 60 | 50 | `0x154` | `0x1c0` | `0x1b8` | `0x00440f80` | `0x00515510` |
| 70 | 50 | `0x164` | `0x1c0` | `0x1b8` | `0x004eedd0` | `0x004522e0` |
| 80 | 70 | `0x17c` | `0x1fc` | `0x1e0` | `0x004ee4a0` | `0x004fbbf0` |
| 90 | 10 | `0xb8` | `0x88` | `0x88` | `0x004ef600` | `0x00515230` |

All 17 Rise of Rome and 13 installed base civilizations have type 1 in their
DATs. `aoe_unit_template_factory_spec` ports the ten-way selection only; an
audit of every present definition in both DATs finds zero unsupported types.
The player constructor (`0x0045c650` in Rise of Rome) clones the selected
civilization's template list into player `+0x24`. Scenario placement later
selects from that player list by object ID and calls template vtable `+0x18`
to instantiate a live unit. The live-size column comes from each Rise of
Rome template vtable `+0x18` factory: type 10 `0x00442aa0`, 20/25
`0x0043f6e0`, 30 `0x004418b0`, 40 `0x0043f080`, 50 `0x00440190`,
60 `0x00441230`, 70 `0x004ef470`, 80 `0x004eea90`, and 90
`0x004ef700`. Base live sizes were checked against its corresponding vtable
`+0x18` methods; types 30–80 differ from Rise of Rome. The vtable `+0x1c`
clone methods each allocate the listed template size. These original 32-bit
sizes do not define Vita struct layouts. `aoe_player_templates_clone_civ`
now translates the player constructor's non-null slot loop and deep-copies
all currently parsed `DatUnit` fields (`name`, tasks, attacks, armors as
separate allocations). Both installed DATs pass a full civilization clone
audit. The ten original template constructors' unparsed fields, the full
per-type clone methods, and live-unit methods are not ported. Therefore the
type and clone audits
establishes dispatch coverage, not scenario-unit or gameplay coverage. The
separate player stream reader `0x0045d8c0` and its base equivalent
`0x005171e0` also use this type family when loading serialized templates.

The next live-object link is the template vtable `+0x18` factory. Type 90's
Rise of Rome factory `0x004ef700` allocates `0x88` bytes and calls
`0x004b2550`, which delegates to the common live constructor `0x004a7c80`
and initializer `0x004a8200`. The latter reads template byte `+0xa4`:
zero calls world positive-ID allocator `0x00523120`, nonzero calls the
negative-ID allocator `0x00523160`; both IDs are stored at live unit `+0x04`.
`DatUnit.negative_id_mode` now reads that exact serialized byte (the 13th
byte of the 16-byte help/hotkey/flags block), and
`aoe_live_unit_allocate_id` ports this branch using the translated world ID
cursors. In the installed Rise of Rome DAT, Gaia has 73 negative-ID
definitions and Egyptian has 69; the base DAT has 63 and 59 respectively.
The base common initializer `0x00412c20` reads the same `+0xa4` byte and
calls its positive/negative allocators `0x00506340`/`0x00506390`.
`aoe_live_unit_core_begin` now composes the directly recoverable initializer
fields with the already-translated storage and ID routines: template/ID,
floating-point HP and `HP > 0`, type-25 flag, initial state 2 and class code
10, X/Y/Z, and carried resource selection plus paid resource effects. Original
assembly confirms the float conversion at `0x004a8237`, flag checks at
`0x004a823e`/`0x004a825c`, coordinates at `0x004a82e4`–`0x004a8300`,
and the two ID calls at `0x004a82b8`/`0x004a82c5`; the base initializer
`0x00412c20` follows the same core rules. This still does not allocate the
full original object, select its graphic, place it on the map, or register it
with the world; those method calls in `0x004a8200` remain.

The common live-unit placement slot is RoR vtable `+0x34` = `0x004ab160`
for both root (`0x0054ce80`) and type 90 (`0x0054d608`); base root
`0x0054a518` uses `0x00415bf0`. These methods clamp X/Y to the map (upper
bound minus `0.001`), compute the tile's fractional X/Y, and select one of
17 shape formulas before adding the tile's integer elevation. The two
executables' shape cases agree. `terrain_shape_surface_offset` ports those
formulas and `terrain_surface_height_at` composes them with the existing
neighbor-derived scenario shape. Original template byte `+0x68` then chooses
Z: mode 0 snaps to surface, mode 1 raises a lower Z to the surface, and
other values retain requested Z. `DatUnit.elevation_mode`,
`terrain_unit_surface_z`, and `aoe_live_unit_core_apply_surface` port this
field update. The offset tail at `0x004ab58d..0x004ab643` is now translated
as `aoe_live_unit_draw_offsets_ror`: X is `(fraction_x + fraction_y) *
tile_half_width`; Y uses `(baseline + fraction_y + 1 - fraction_x - Z) *
tile_half_height`, where shapes 2/6/8/10/14/15/16 select baseline 1 and all
others select 0. Mode 0 copies the primary pair; mode 1 computes secondary Y
against terrain surface; other modes retain it. Conversion truncates and
stores the low word, matching `__ftol` followed by 16-bit stores. In the
installed expansion Gaia definitions, modes 0/1/other
occur 305/27/0 times (base 277/22/0); all present definitions in both DATs
use only 0 or 1. The RoR tile-list detach/attach and state-2 visibility
callbacks are now translated in the portable placement/vision modules;
placement now receives parsed `DatMapGeometry` and stores these four offsets
before callbacks, and the persistent constructor uses that same path. The real
`EXPANS_1_0.scx` audit computed nonzero offset sets for all 14,923 records.
The original unit's other placement side effects and full object layout
remain unported.
The optional `live_unit_surface_real` audit traverses actual
`EXPANS_1_0.scx` placements through DAT lookup, player-template clone, the
common live core, reserved scenario spawn IDs, and surface Z selection. It
checked all 14,923 records, with 124 Z adjustments and 25 negative reserved
IDs. This is a coverage/bounds check, not visual or gameplay validation: the
scenario tile X/Y orientation is established below for square maps, but no
live object is allocated or registered by the audit.

The Rise of Rome scenario map reader `0x00445aa0` (world vtable `+0x70` via
`0x005216c0`) consumes the tile triples in an outer-Y, inner-X loop and
writes each to runtime row pointer `map+0x8d8c[y]` plus `x*0x18`. The runtime
map allocator `0x004453c0` gives each row a width-sized stride. Base reader
`0x00424cb0` does the same. Both original readers use map height as the
inner-loop bound, so this direct `scenario.tiles[y*width+x]` mapping is
verified only for square maps. All 24 installed base scenarios are square
(2 at 120², 7 at 144², 15 at 200²), as are all 19 extracted expansion
campaign scenarios (8 at 144², 10 at 200², 1 at 250²). The portable
`scn_runtime_tile_order_supported` guard rejects rectangular maps before
the original-runtime tile interpretation is used; `live_unit_surface_real`
now requires it. This proves coordinate orientation for those scenario
tiles, not the rest of live map construction or gameplay placement.

## Whole-executable coverage and world update

The whole-executable Ghidra export is a reference corpus, not a complete or
buildable decompilation. A concrete gap was found in the first 40 entries of
the expansion world vtable at `0x0054fe58`: 25 had no function in
the original bulk export, despite pointing to valid code. The targeted
`tools/RecoverAoEVtableFunctions.java` pass created those missing function
starts in the Ghidra project and exported their pseudocode to
`build/decompile/ror_world_vtable.txt`. This was only a 40-entry subset:
the actual table has 64 entries through `+0xfc`, ending at `0x0054ff58`.
The regenerated expansion export
first contained 4,084 decompiled functions (up from 4,059). A broader
read-only `AuditAoEFunctionPointerRuns.java` pass then found 141 expansion
runs of at least four executable pointers with 737 unique missing starts,
and 102 such base-game runs with 953 unique missing starts. The guarded
`RecoverAoEPointerRunFunctions.java` pass created 736 expansion and 950 base
functions; the remaining candidates became known or fell inside functions
created earlier in the same pass. The Ghidra inventories are now 4,821
expansion and 5,440 base functions. These are discovered-function counts,
not proof that every executable function has been identified.

The first regenerated bulk exports gave one timeout in each executable:
expansion `0x005459a0` and base `0x005489a0`, both large low-level pixel
routines. A separate 60-second decompilation of the expansion routine
succeeded, with unresolved computed-jump warnings. With 60 seconds per
function, the final indexes report all 4,821 expansion and 5,440 base
functions decompiled (`ok`), including those two routines. A second
pointer-run audit reports zero missing starts in the same 141 expansion and
102 base runs. This closes that specific audit only; it does not establish
complete discovery of every function or correctness of the pseudocode.

The patched Rise of Rome 1.0a image was then checked with a broader aligned-
pointer scan over initialized non-executable sections, including isolated and
shorter-than-four pointer runs (excluding `.rsrc`, whose icon/string payloads
produce false code-looking values). The first pass found 2,478 unique
executable targets: 2,404 already-known function entries, 42 addresses inside
known functions, and 32 targets without function starts. Of those 32, 27 were
in contiguous two- or three-entry executable-pointer runs and did not split an
existing function or instruction. The guarded recovery pass created those 27
starts; the follow-up scan reports 2,431 known, 42 interior, and five
unresolved targets. A subsequent interior-boundary audit found five
instruction-aligned base targets and three RoR targets inside larger Ghidra
functions, each supported by repeated `.rdata` references or a short pointer
run. On refreshed full-autoanalysis projects, the pointer pass recovered
three further expansion entries; the base count did not change. Final scans
report 2,419 known, 42 interior, and one unresolved pointer target for patched
base 1.0c; RoR 1.0a reports 2,437 known, 39 interior, and two unresolved
targets, out of 2,462/2,478 unique targets. Raw-byte inspection classifies
the remaining three values as non-entry data or a mid-instruction reference.
The 42/39 interior references still require classification, while code
without data pointers is outside this audit. `AuditAoEExecutablePointers.java`
preserves source-table, instruction-boundary, and run context; the recovery
scripts promote only evidence-supported entries. The revised pipeline applies
them after full Ghidra automatic analysis and before refreshing patched
reference exports. This still does not establish complete function discovery.

The actual expansion world-update function is `0x00520380`, at world-vtable
slot `+0xbc`; the base equivalent is `0x00503390`, at slot `+0xbc` of the
64-entry base world vtable `0x00551e48`. Both first call the command/network
gate (`0x0041f250` expansion, `0x00507f30` base). A zero result clears the
frame delta and skips the update. `-1` selects local wall time, capped at
100 ms; a positive result supplies the synchronized delta. Both multiply
that delta by world speed, clear the one-shot pause flag, and run the player
update pass if game time advanced or this is the first local frame. The
expansion then flushes each player's queued work and runs two additional
per-player passes; the base game has one post-update player pass. Both
preserve/reset the RNG state around those passes. `src/world_clock.c` ports
the common clock-gating arithmetic. `src/world_tick.c` ports the post-gate
ordering with required hooks for original routines not yet translated. Its
tests verify call order and RNG state. The primary player pass now invokes
`aoe_player_update_step` directly; its remaining object/map/event methods
are explicit dependencies. The pre-gate status copy, command/network gate,
optional AI logging
and game-over reporting, and original timer audio/handle cleanup,
live player-object methods, follow-ups, and finalizer bodies are not part of
this core. This is
executable-derived orchestration, not a running game.

The first pre-player call in the simulation branch drains queued commands:
RoR `0x0042a5d0` checks its controller's `+0x14` active field, fetches one
command via `0x0041f0d0`, dispatches through controller virtual `+0x08`,
releases the command via `0x00528360`, and repeats until fetch returns null.
Base `0x0041f0b0` does the same with fetch `0x00507e20` and release
`0x0052c580`. `src/command_drain.c` translates the shared loop and is called
directly by `world_tick.c`. The fetch routines choose between local and
network command sources; their bodies, command dispatch, and command
ownership are not yet reconstructed as a live pipeline. The second
pre-player call, RoR `0x004777c0` / base `0x0045f360`, conditionally
advances timed scenario actions through RoR `0x004ee2b0` / base
`0x00512a90`. `src/scenario_timeline.c` now translates that scheduler and
is invoked directly by `world_tick.c`. Both binaries use the float constant
`0x3a83126f` (`0.001f`) to convert unsigned game milliseconds to seconds.
The scheduler first finds the first node later than its previous time while
clearing both object references whose class byte at `+0x48` is at least 8;
then it executes due nodes in linked-list order. Kind 0 calls an object-pair
method, kind 1 creates an object from a player template and propagates its
short reference ID through all nodes, kind 2 calls a position/parameter
method, and kind 4 calls a no-argument method. Kind 3 has no dispatch arm.
The semantic node preserves the 48-byte PC/Vita field layout on 32-bit
targets, but the live object virtuals, template lookup, and timeline setup
remain external dependencies. This is original scheduling logic, not a
running scenario system.

The post-checkpoint object-list pass is also direct now. RoR world tick at
`0x00520539` loads player `+0x30` and calls `0x004521e0` for each non-null
player, then separately loads player `+0x40` and calls `0x00436f20`.
The base player's vtable `0x00552378` has `0x00517950` at slot `+0x80`;
its assembly checks the cooperative flag, pumps if needed, calls
`0x0049ce80` on player `+0x30`, then `0x004f95d0` on player `+0x40`.
`world_tick.c` now traverses the second semantic list directly in both
editions, preserving the base per-player pump and the expansion's separate
all-player passes.

The shared `+0x40` visibility-watch behavior is now translated in
`src/visibility_watch.c` and invoked directly by `world_tick.c`. RoR
`0x00436f20` and base `0x004f95d0` both read parallel arrays at watch
`+0xc/+0x10/+0x14/+0x18`: tracked unit, pointer to live flags, prior
flags, and unit ID. Both use the world lookup (`0x005230d0` RoR,
`0x005062f0` base) to confirm that the ID still resolves to the same unit.
For a valid unit, bits 1–15 that changed from set to clear notify player
indexes 1–15 except the owner, passing the unit's floats at `+0x38`,
`+0x3c`, and `+0x40` to the recipient's virtual `+0x38` method. Their removal behavior
differs: RoR immediately removes invalid entries with swap-last compaction
(`0x00436e60`) and rechecks the swapped slot; base records only the last
non-null invalid unit and removes it after scanning (`0x004f9520`). Base
also writes current flags to the prior snapshot even for invalid entries.
The portable module preserves these branches and directly implements the
world lookup: nonnegative IDs index the positive array, while negative IDs
index the negative array by absolute value (so ID -1 uses slot 1). RoR
`0x00523190` / base `0x005063c0` insert into those arrays, growing a full
array to the larger of twice its old size or roughly 1.1 times the new ID;
RoR `0x00523360` / base `0x005065a0` clear a valid slot without decrementing
the insertion counters. `AoeOwnedVisibilityRegistry` translates those
operations and exposes a stable view object to `world_tick` even if an array
is reallocated. Both world constructors initialize 10,000 positive and 1,000
negative slots, with next-ID cursors 0 and -1. RoR
`0x00523120`/`0x00523160` and base `0x00506340`/`0x00506390` select the
next positive or negative ID: positive scans forward past occupied slots;
negative returns its descending cursor. Special-ID mode returns world
`+0x70` instead, with the negative cursor lowered if needed. These rules
are now translated. The portable form rejects malformed bounds and
`INT32_MIN`.
On occupied positive-slot replacement with world flag `+0x74` set, the
original insertion routine additionally clears that flag and calls RoR
`0x004a8820` / base `0x00413250` to rebase IDs; the owned API reports the
replaced object, clears the flag, and reports that reassignment is required;
the displaced unit's methods and live unit construction/destruction remain
to be connected. RoR world reset `0x0051f400` and base `0x00502390` clear
both ID arrays and reset their next-ID cursors to 0/-1. The owned API now
translates just that ID-table portion; the reset's unit destruction and other
world cleanup are not implemented. Recipient virtual methods are also still
unported.

The player vtable `+0xdc/+0xe0` pair is RoR `0x0045f810`/`0x0045f860`
and base `0x00519350`/`0x005193a0`. Registration selects the player list
at `+0x2c` when its first selector is nonzero; otherwise it chooses `+0x30`
for a nonzero second selector, or `+0x28`. It prepends a node through RoR
`0x00451f00` / base `0x0049cbf0`, then invokes world vtable `+0xf8`
(RoR `0x00523190`, base `0x005063c0`). Removal uses RoR `0x00451f40` /
base `0x0049cc30` to unlink a supplied node or the first matching object,
then world vtable `+0xfc` clears its unit-ID slot. The portable
`aoe_player_unit_register/remove` follows that path using the owned lists
in `AoePlayerUpdateState`. List nodes are owned, but unit objects are not.
Construction/destruction, object virtual methods, and collision-triggered
reassignment are not yet connected.

The base-unit construction path RoR `0x004a8200` / base `0x00412c20`
calls two storage helpers before allocating an ID. RoR
`0x004a8950`/`0x004a89a0` and base `0x004133a0`/`0x004133f0` consume the
three template entries that `DatUnit.storage` already parses. The first
helper resets unit `+0x4c` to -1 and `+0x44` to 0, then selects the last
entry with mode 0 and resource type other than -1. The second calls the
owner player's vtable `+0x78` resource-add method for each mode-1/2 entry
with a nonnegative resource type. `aoe_unit_storage_initialize` now connects
the parsed DAT records to the translated player-resource method. The rest
of the constructor, including virtual methods and player/world registration,
still needs to be composed around this step.

The base-unit graphic virtual method is RoR `0x004ab6b0` and base
`0x004161a0` (verified against both executables' machine code). A null
request substitutes the template's standing graphic at `+0x18`. If the
requested pointer differs from current unit graphic `+0x10`, the base game
always copies current to previous `+0x14`, removes current from the sprite
manager at unit `+0x18`, sets current, adds requested with argument `0x3c`,
then clears unit byte `+0x35`. Rise of Rome adds a guard: when unit byte
`+0x48` exceeds two, it does not replace a non-null current walking graphic
from template `+0x1c`. `aoe_unit_graphic_select` translates this exact
decision and side-effect order using opaque runtime graphic pointers and
sprite-manager callbacks. It is not yet wired to actual unit instances or
the Vita renderer.

Its sprite-manager calls lead to RoR `0x00415c80`/`0x00415d20` and base
`0x0044fb80`/`0x0044fc30`; clear is RoR `0x00415bd0` and base
`0x0044fad0`. Each list node contains a sprite object, byte priority,
byte extra-reference count, and next pointer. Add skips null graphics,
invalidates an animated graphic before searching, increments an existing
node's reference byte for the same runtime graphic pointer, or inserts a new
sprite after all nodes with priority less than or equal to the requested
priority. Remove decrements a nonzero extra-reference byte, otherwise
unlinks, invalidates if animated, and destroys the sprite. `aoe_sprite_list`
translates these operations with an object factory and renderer callbacks;
it still requires the original graphic-instance behavior and a Vita backend.

The animated sprite's update virtual method is RoR `0x00416540` and base
`0x004e8ed0`. The 40-byte instance stores current/previous frame, period,
elapsed time, frame-changed and looped bytes, and a run-state byte. It clamps
an overrun frame, advances by the low byte of elapsed whole frame periods,
then scales the next period by graphic speed multiplier and caller speed.
On an overrun it either enters the graphic's replay delay or wraps; sequence
flag `0x08` stops on a completed loop. `aoe_sprite_animation_tick` ports this
common state machine. Its graphic input corresponds to runtime fields
`+0x5e/+0x64/+0x68/+0x6c/+0x70`; binding the DAT record to that runtime
object and connecting drawing/audio remain separate work.

The sprite factory is RoR `0x004a7b10` / base `0x004b3d10`; it chooses
static if runtime graphic byte `+0x70` has bit `0x01` clear and animated
otherwise. Static constructors are RoR `0x00416060` / base `0x004e8900`;
animated constructors are RoR `0x00416300` / base `0x004e8bf0`. They set
graphic pointer, offsets, a frame period clamped to `0.001`, and initial
run state. Flag `0x04` draws from the original RNG and chooses a starting
position over frame-count × duration plus replay delay. `aoe_sprite_instance`
connects these constructors and the animated tick to `DatGraphic` and
`AoeRng`. Manual real-file validation instantiates all 845 base and 941
expansion present graphics. Base graphic 834 (`Space Ship`) has flags `0x0f`,
one frame, and zero duration/replay delay; the original random-start path
divides by zero, so the portable constructor consumes the RNG draw but
defines its start as stationary frame zero with the minimum period.

The list-wide update method is RoR `0x00415d90` / base `0x0044fcb0` and
calls each sprite instance's virtual update in priority-list order.
`aoe_sprite_list_tick` and `aoe_sprite_runtime` connect this traversal to
the DAT-backed `AoeSpriteInstance` factory and animation tick. The redraw
predicate remains renderer-provided because the original tests loaded
graphic state, not merely the DAT sequence flags. `aoe_unit_graphic_select_runtime`
then connects the unit method to this actual list. It acquires the new
sprite before releasing the old to preserve the unit on allocation failure;
the original calls remove then add, so successful final state is faithful
but callback ordering is intentionally different at this portable boundary.

The release RoR type-20/25 live update predicate `0x00415820` (patched 1.0a
`0x00415200`) reaches that list-wide
method through sprite-manager virtual `+0x08`. Its first argument is world
frame delta at player `+0x3c`, world `+0x84`; its second is the live object's
animation-rate float at `+0x88`. Release type-20 constructors
`0x00415770`/`0x00415790` (patched `0x00415130`/`0x00415170`) initialize that
field to zero. Types 30 through 80
reach `0x00415820` through inherited predicate chains, while root type 10 and
type 90 call `0x004aa790` directly and do not take this sprite-tick tail.
`aoe_live_unit_tick_sprites_ror` and
`aoe_scenario_runtime_tick_sprites_ror` preserve that dispatch and argument
contract over persistent units. This is only the recovered sprite tail, not
a substitute for the preceding action, movement, or combat work.

Graphic frame selection before drawing is RoR `0x004a6d10` and base
`0x004b2e60`. It uses runtime graphic frame count `+0x5e`, angle count
`+0x60`, and mirror byte `+0x74`; these correspond to the parsed DAT
`frame_count`, `angle_count`, and `mirror_mode`. Without mirroring, the SLP
index is `angle × frame_count + animation_frame`. Mirrored graphics use
the original quarter-angle offset and reverse angle mapping, with a special
two-angle case that shares one stored frame and mirrors the second angle.
`aoe_graphic_select_frame` and `aoe_sprite_instance_frame` now translate
this rule. Manual validation succeeds for 3,974 base and 4,515 Rise of Rome
nonblank graphic-angle selections. Actual SLP bounds are checked below;
the frames are not yet submitted to a Vita display surface.

`aoe_graphic_art_load_frame` now follows the executable's SHP-first,
SLP-second type lookup through registered DRS archives and decodes the
selected SLP frame. It reports absent direct art, unsupported SHP, and
malformed/out-of-range frames separately. The installed base DAT plus its
graphics/interface archives load 722 first frames; 83 active definitions
have no direct SLP, with 40 blank graphic records. The expansion overlay
plus base archives load 794 first frames; 105 have no direct SLP, with 42
blank records. No sampled first frame was malformed. A full angle/highest-
animation-frame bounds sweep found 3,516 base and 4,014 expansion selections
inside the actual SLP tables, but 91/92 out-of-range combinations. The
selector is kept faithful to the executable; the loader rejects those
requests rather than clamping or substituting art. Actual runtime angle use
must be traced before deciding whether these definitions are reachable.

The original normal SLP draw method RoR `0x004a1460` places an SLP frame
at `(anchor_x - hotspot_x, anchor_y - hotspot_y)`. Its mirrored counterpart
`0x004a2570` uses `(anchor_x + hotspot_x - width, anchor_y - hotspot_y)`
and reverses horizontal source pixels. `slp_blit_graphic_rgba` now implements
that placement and clipping. It resolves ordinary SLP palette pixels with
the parsed JASC palette and requires a caller-supplied effect resolver for
player-color, shadow, and outline markers. These effect semantics, render
order with terrain/units, and live Vita display submission remain unbound.

`aoe_sprite_draw_instance_rgba` joins the selected DAT-backed instance frame,
original-order DRS lookup, decoded SLP, palette, and hotspot blitter. Static
sprite virtual draw RoR `0x00416190` / base `0x004e8a40` adds instance
offsets `+0x0c/+0x10` to the anchor. Animated draw RoR `0x004166e0` /
base `0x004e9060` passes the anchor unchanged. The list draw dispatch is
RoR `0x00415dc0` / base `0x0044fce0`; `aoe_sprite_draw_runtime_rgba`
traverses that priority order. A real-file integration test with original
palette ID 50500 draws base and expansion `Arrow Fire` graphic 3, each with
126 ordinary palette pixels. `aoe_graphic_art_cache` adds an optional
fixed-slot, byte-bounded LRU of decoded frames. It resolves the DRS match
before keying on archive entry plus frame number, so expansion overlay
precedence remains intact. Cache-limit misses fall back to direct decoding;
the real-file integration test confirms cached, cache-hit, and fallback
draws have the same RGBA bytes as direct drawing in both editions. This is
still a software-memory path;
other color-table selection, outlines, sound triggers, terrain/object
ordering, and Vita display submission are still missing.
RoR `0x00415dc0` / base `0x0044fce0` calls every sprite's void draw method,
regardless of a failed art lookup. Both portable runtime draw variants now
continue after an error and report the first failure after traversal. Real
base/expansion tests put unresolved SLP 251 before valid graphic 3 and verify
the later sprite still draws in RGBA and indexed surfaces.

Player-color rendering now follows the Rise of Rome dispatch at `0x004a6dd0`:
its signed graphic field `+0x2a` chooses a fixed color when nonnegative, or
uses the caller's unit/player color otherwise. The dispatch calls
`0x00545760` with `(color + 1) * 16`; that routine repeats the low byte in
the renderer's color word. `0x005459a0` handles SLP player-color opcode 6
by ORing each source palette index with that byte. The portable
`aoe_graphic_player_palette_index` and
`aoe_sprite_draw_instance_player_cached_rgba` reproduce that rule on
ordinary/player-color-only frames. Real base and expansion `Arrow B fire`
graphic 1 has 17 player-color pixels and passes the rendered-RGBA check.
Opcode 0xb instead transforms existing destination indices through the
table pointer set by `0x005457a0`; that indexed path is described below.
The RGBA player-only draw rejects frames with shadows or outlines rather
than silently painting them.

The renderer's SLP shadow opcode 0xb in RoR `0x005459a0` performs
`destination = table[destination]`, where `0x005457a0` installs the table
pointer. RGBA alone loses the destination palette index, so
`slp_blit_graphic_indexed` and the cached indexed sprite draw path now retain
8-bit indices. They implement ordinary color replacement, the verified
player-color OR bank, and shadow-table lookup, while pre-rejecting outlines
and a missing required table. The original table source is now identified:
both DATs list `Shadow.col` at color ordinal 10, and graphic 118 `Cliffs s2`
uses that fixed color. The base installation's `data/shadow.col` is 256
decimal indices and is byte-for-byte identical to base `Interfac.drs`
`anib` resource 50909 (1,192 bytes). `aoe_shadow_table_load` parses that
registered DRS entry. A real base and expansion integration test draws the
82 shadow pixels of `Cliffs s2` against a known indexed background with the
actual table. This proves that graphic's table lookup, not all shadow/color
table selections or outlines.
An added first-frame effect audit across the installed DAT/DRS stacks finds
303 base and 361 expansion graphics with player-color pixels, 58/60 with
shadow pixels, and zero outline-marker first frames among 722/794 loadable
graphics. A complete direct-art sweep then decoded every frame of each unique
SLP referenced by a present DAT graphic: 681 base SLPs / 12,625 frames and
752 expansion SLPs / 15,127 frames, with zero malformed or outline frames.
Player-color markers occur in 8,750/10,992 frames and shadows in 386/392.
In the Rise of Rome executable, outline-renderer globals
`0x007dc058`/`0x007dc05c` are zero-initialized and direct instruction
references found so far read, not write, them; indirect mutation has not
been excluded, so the portable renderer still rejects those markers. The
sweep excludes negative DAT SLP IDs and absent direct art. Even with all
registered archives, 46 base/42 expansion definitions have negative SLP IDs
and 37/63 positive IDs are unresolved; no art is substituted for them.
`palette_expand_indexed_rgba` now converts the completed indexed frame to
opaque RGBA with palette 50500, keeping the original palette-domain effects
before the Vita display boundary. The `Cliffs s2` integration test checks
the resulting shadow pixel against the original palette entry. Base
`Interfac.drs` also contains color tables 50900–50911; 50909 is the broad
shadow remap, while most others alter only ten indices. Their runtime
selection relative to DAT color names is not yet verified, so no ordinal-to-
DRS-ID formula is assumed.

The per-player call following the `+0x7c` update is on the queue at player
`+0x58`, not another player virtual method: RoR world tick calls
`0x0045fad0`, and base calls `0x00519670`. Both clear queue fields `+4` and
`+8`; if field `+0xc` is nonzero they free storage at `+0x10`, set capacity
at `+0` to eight, allocate eight eight-byte entries, and clear the dirty
field. `src/player_queue.c` translates this directly and `world_tick.c`
invokes it after each player update. The portable implementation allocates
before freeing so an allocation failure leaves existing storage intact;
ordinary successful behavior matches the binary.
The adjacent queue methods are also translated: RoR constructor
`0x0045fb20`, append `0x0045fa50`, view `0x0045fb70`, pending-count drain
`0x0045fb90`, destructor `0x0045fb50`; base counterparts are
`0x005196b0`, `0x005195d0`, `0x00519720`, `0x00519740`, and
`0x00519700`. Entries are two 32-bit words. Append increments both count
fields, doubles storage when full, and copies all old slots. The base
constructor rejects capacities below 100 and append rejects growth when
the old capacity exceeds 65,000. The portable code adds explicit invalid
state, overflow, and allocation-failure checks; those branches should not
be mistaken for observed PC behavior. No live command producer or consumer
is connected to this semantic queue yet.

The two timer methods called before world-clock advancement are also shared
in structure: RoR `0x00520300`/`0x00520340`, base
`0x00503320`/`0x00503350`. The first wraps its `+0x20` index at the signed
16-bit count at `+0x2c`, selects an object from the `+0x30` pointer array,
calls its action, then increments. The second wraps its `+0x24` index at
count `+0x34`, selects from the `+0x38` array, invokes its action only for a
non-null selected object, then increments. The `+0x30` selection is clear
in the `0x00520300` assembly but absent from Ghidra's pseudocode.
`src/world_timers.c` ports both rotations and is called by `world_tick.c`.
The selected-object action methods are RoR `0x004a3a30`/`0x004a7c00` and
base `0x0042b6a0`/`0x004b3e30`. The first sweeps 36-byte timed-event
records and conditionally calls a target method after an unsigned elapsed
time exceeds a threshold. The second tracks an active object's elapsed
time and expires it after 179,999 ms; both PC images compare its initial
accumulator with a double constant that is exactly zero. `src/timer_actions.c`
translates both action state machines using portable semantic records. The
timed-event sweep uses unsigned 32-bit elapsed subtraction, a strict
greater-than threshold, and changes status from 1 to 2 after its target
callback. The lifetime action seeds its accumulator to 1 on the first active
tick, then updates it modulo 32 bits and expires at signed accumulated time
180,000. Binding functions connect these actions to `world_timers.c`.
`src/audio_target.c` now translates target expiry (RoR `0x004a49e0`, base
`0x0044d370`): follow `+4` links to the final node, conditionally stop
playback, release the `+0x34` voice, free `+0x2c` and `+0x30` buffers in
order, clear active `+8`, then always clear pending `+9`. Its callback binds
to the timed-event action. The stop methods (RoR `0x004a4b70`, base
`0x0044d510`) are also translated: follow the same chain and call voice
virtual `+0x48` only when the terminal name is nonempty, active flag is
nonzero, and voice exists. Voice virtual `+0x48`/`+0x08`, allocator free,
and lifetime handle cleanup/free remain required; there is no live
audio/handle implementation.

The apparent outer-frame path `0x0041ae90` calls `0x00419e90` for window and
network polling, then invokes the active screen's virtual `+0x54` method.
The game viewport constructor `0x00511790` installs vtable `0x0054faf0`,
whose `+0x54` entry is `0x00454580`: it iterates child UI controls and calls
their update methods. It is not the world simulation tick; that is the
separate `0x00520380` virtual method identified above. The next gameplay
work is to port its player dispatch and downstream update methods.

The expansion world creates player objects through `0x0045c650`, which
installs player vtable `0x00549c70`. The world tick's player virtual call at
`+0x7c` therefore resolves to `0x0045df70`. That method clears a per-player
field, processes pending objects (`0x004521e0`), prunes a 25-slot group
(`0x0045f170`), tests a player outcome (`0x005101c0`), clears a byte flag,
and conditionally applies map-related work (`0x00519a60`). Several downstream
bodies remain unported. The same player object stores a signed 16-bit
resource count at `+0x4c` and a float-array pointer at `+0x50`. Expansion
`0x0045def0`/`0x0045df10` and base `0x00517880`/`0x005178a0` set or add
one indexed float after the identical signed bounds check. The assembly
loads the add operands in opposite orders across versions; the portable
`aoe_player_resource_add` takes an explicit edition to preserve that detail.
`aoe_player_resource_set/add` implement this shared gameplay primitive.

The next player-update call is the same routine in both executables:
Rise of Rome `0x004521e0`, base `0x0049ce80`. It starts at list head
`+4`, caches `node->next` before touching the object, skips null object
pointers, calls object vtable `+0x24`, calls vtable `+0x08` only if the first
returns nonzero, and checks the cooperative flag after every tenth non-null
object. The flag is re-read at each checkpoint; Rise of Rome pumps through
`0x0044b490`, base through `0x004692d0`. `src/object_update_list.c` ports
this shared traversal over portable nodes with required method callbacks.
The original object vtable methods and actual player-owned list are not yet
bound to it, so this does not make player updates executable end-to-end.

Rise of Rome's player virtual `+0x7c` at `0x0045df70` and base's counterpart
at `0x005178f0` have the same sequence: optionally pump cooperative work;
clear player `+0x10`; run the object list; prune the player group; run the
outcome routine; call player virtual `+0x04` with state 2 only on outcome 1;
clear byte `+0x20`; optionally do map work when RoR `+0x114` / base `+0x110`
is nonzero. `src/player_update.c` composes the translated list, group, and
outcome-manager work in this order. The event and condition virtual bodies
are now translated over semantic references and called directly by player
update. Player virtual `+0x04` is RoR `0x0045cb90`, base `0x00516480`:
it changes byte `+0x80` only when the requested state differs and byte
`+0x81` is zero or the new state is 2. An unlocked transition to state 2
emits global event code 6 with signed player index `+0x4a`; RoR uses event
interface virtual `+0x40`, base uses `+0x3c`. `aoe_player_state_set` ports
the state/lock rule and leaves only event delivery as a platform binding.
Live-object binding, global event delivery, map work, and object methods remain,
so it is not yet a complete live player update.

The head of the common live-object predicate is now translated from patched
1.0a `0x004a8f40` (disc `0x004aa790`) as
`aoe_live_unit_update_prelude_ror`. When live byte `+0x4b` is set, it clears
the flag, removes the unit from the non-positive player category, registers
it in the positive category, updates its persistent list node, and returns
the predicate's early-handled condition. Otherwise it stably compacts the
unit-ID vector at live `+0x64/+0x68/+0x70`, retaining only world-resolved
units whose state byte is at most 2. `AoeLiveUnit` owns this vector and frees
it during original-order teardown. The next branch establishes that live
`+0x20` is another live-unit pointer, not merely an opaque non-map parent. A
non-null pointer suppresses the normal state switch; when its state at `+0x48`
is above 2, the current object's virtual `+0x5c` receives that exact state.
`aoe_live_unit_update_attached_target_ror` preserves this through a mandatory
class-specific setter callback, and placement uses the newly typed
`attached_unit` field for the same `+0x20` guard.

The no-target switch is translated as `aoe_live_unit_update_normal_state_ror`.
State 2 invokes virtual `+0x28` and deliberately falls through to state 0;
negative HP then sets state 3, selects the dying graphic, and runs the dying
sound path when present. States 3/5 use live carried-resource type/amount at
`+0x4c/+0x44`: an exhausted value either sets state 6 and selects the undead
graphic when template byte `+0x24` is nonzero, or sets state 7 and invokes
virtual `+0xb8`; otherwise template float `+0x80` decays the amount by world
delta and clamps it to zero. State 7 sets state 8. State 8 returns 1, or 2
when template byte `+0xa4` is nonzero. The template reader at disc
`0x00441fa0` proves the corresponding serialized fields, now retained as
`DatUnit.undead_graphic`, `undead_mode`, `decay_rate`, and `dying_sound`.
Class virtuals and sound delivery remain mandatory callback boundaries.
`aoe_live_unit_update_predicate_ror` composes these branches in their original
order. `aoe_live_unit_player_update_step_ror` installs it in the translated
RoR player `+0x7c` traversal, matching object-list pointers against an
explicit roster of owned `AoeLiveUnit` instances. Since the list may contain
other dynamic classes, those objects continue through required generic
predicate/action adapters; translated units use required class-specific
`+0x08` actions when the predicate returns nonzero. World delta and the
binary epsilon are explicit runtime values. This builds for host and Vita;
the class action bodies and a real mixed-list runtime exercise remain to be
recovered.

The outcome manager at player `+0x34` uses RoR `0x005101c0`/
`0x005100e0` and base `0x004338c0`/`0x004337d0`. The first function visits
event nodes at manager `+0x14` (next `+0x18`) and condition nodes at `+4`
(next `+0x30`), calling manager virtual `+0` and `+4` respectively when the
event status byte `+1` or condition status byte `+0x2e` is neither 1 nor 3.
The second resets condition `visited` at `+0x2f`, groups
by byte `+0x2c`, and derives outcome byte `+0x10`: 1 when every group has a
type-1 record, 2 when a group consists of type-2/type-3 records with at
least one type 2, otherwise 0. Its type-3 counter persists across groups;
`src/player_outcome.c` preserves that executable behavior rather than
normalizing it. With no condition nodes, it leaves the prior outcome byte
unchanged. Their kind-specific bodies are translated below over semantic
player, unit, resource, score, and progress references.

The manager constructor installs RoR vtable `0x0054fae0` with virtual
`+0` = `0x00510260` and `+4` = `0x005102c0`; base vtable `0x0054c1b0`
points to `0x00433960`/`0x004339c0`. Both virtuals switch on the separate
kind byte at node `+0`, not the status byte used by the outer traversal.
The event switch handles kinds 0–4; the condition switch handles 0–10 and
12, leaving kind 11 and all unknown values as no-ops. The prior portable
model conflated kind and status; it has been corrected in
`include/player_outcome.h`. `src/player_outcome_dispatch.c` now ports these
two switch layers and every recognized handler. All event kinds 0–4 are translated.
Kinds 0 and 4 both read the owner's
float resource at event index `+4`, truncate it toward zero with the
original `__ftol` rule, and replace the event contribution at `+0x10`
while adjusting manager score `+0x1c` by the difference, modulo 2^32.
Kind 0 (RoR `0x005113b0`, base `0x00434b30`) divides that current integer
resource by event `+8`, clamps a negative quotient to zero, then multiplies
by weight `+0xc`; it writes the integer resource back as float `+0x14`.
Kind 4 (RoR `0x005116a0`, base `0x00434e20`) first retains the maximum
integer resource seen in float `+0x14`, then truncates that high-water
value divided by `+8` before multiplying by `+0xc`. Invalid divisors or
out-of-range resource conversions fail closed in the portable model.
Event 1 (RoR `0x00511410`, base `0x00434b90`) awards its weighted score
and freezes at status 2 when the owner's truncated resource reaches the
target at `+8` and no other player, including neutral player 0, has a
float resource at or above that target. If any rival reaches it first,
status becomes 1 without changing the prior contribution. Event 2 (RoR
`0x005114d0`, base `0x00434c60`) continuously scores a qualifying owner
only when no non-neutral rival is ahead; a tie preserves the prior
contribution. Event 3 (RoR `0x005115c0`, base `0x00434d40`) retains an
owner high-water sample, raises its integer rival threshold when a
non-neutral rival's float resource exceeds it, and uses those histories
to replace its score contribution. All three use the same 32-bit manager
score difference update. Condition 12 (RoR `0x00510bb0`, base
`0x004342f0`) compares the signed average of that score across the owner
and, optionally, mutually eligible team-mode players against its target
at `+0x20`; it only sets status 2 and does not reset it when the average
falls. The other player's manager score getter is RoR `0x004550b0`,
base `0x00434e90`, both returning manager `+0x1c`.
The translated condition bodies are kinds 0–10 and 12. Condition 7 (RoR
`0x00510ad0`, base `0x00434200`) compares a signed target at condition
`+0x20` to 100 times a player progress ratio. The getter (RoR
`0x0051a270`, base `0x0040a110`) divides signed fields `+0x1c/+0x20`
of the object at player `+0x38`; both editions embed float 100.0.
With condition `+0x2d` and team mode, it scans eligible team-mode players
in descending index order, including the owner, and latches status 2 when
any reaches the target. Otherwise it checks only the owner. A zero
denominator fails closed in the portable model. The broader meaning of
the `+0x38` object is not yet established, and its live binding is absent.
Both
editions' kind-2 handler (RoR `0x005106e0`, base
`0x00433e00`) changes condition status to 2 and clears its `+0x28` unit
reference when that unit's byte `+0x48` equals 7. Kind 10 (RoR
`0x005108a0`, base `0x00433fd0`) sets condition status to 2 if its `+8`
player reference has byte `+0x80` equal to 2, otherwise to 0; a null
reference leaves status unchanged. These two bodies are in
`src/player_outcome_dispatch.c`. Their referenced unit/player objects are
still semantic placeholders rather than bound live game objects.
Kind 0 (RoR `0x00510380`, base `0x00433a80`) tracks a unit reference at
condition `+0x28`. A code below 3 and ownership by the manager owner sets
status 0 to 2. Optional allied ownership can also set 0 to 2, but requires
both team-play flags and zero diplomacy bytes in both directions. Once
status is 2, transfer away from the owner reverts to 0 only if allied
tracking is disabled or the manager owner's diplomacy byte toward the
new holder is nonzero; the holder's reciprocal byte and team flag are not
consulted in that reversion branch. A unit code at least 3 clears the
reference and changes status 0 to 1, leaving other statuses unchanged.
The portable handler preserves this asymmetry over semantic unit/player
links; live ownership binding remains absent.
Kind 3 (RoR `0x00510700`, base `0x00433e30`) runs only while condition
status is zero. It counts units with signed class `+4` and state byte
`+0x48 == 7`, adding them to the existing 32-bit counter at `+0x20` on
each update without resetting it. A referenced player at `+8` narrows the
scan to that player; otherwise it scans all owner-eligible world players,
including the owner if eligible. The 32-bit counter wraps as on x86, then
a signed comparison against threshold `+0x1c` sets status 2. The portable
handler preserves these details over semantic player/unit links; live
object binding remains absent.
Kind 6 (RoR `0x00510a20`, base `0x00434140`) reads the owner resource
at signed condition index `+0x1c` and compares the float total against
the signed integer target at `+0x20`, reevaluating status as 2 or 0.
If condition byte `+0x2d` and the owner's team-play flag are both nonzero,
it adds other players' same resource in descending player-index order.
Each added player must be mutually eligible by zero diplomacy bytes,
not the owner, and have its own team-play flag. The flag is at player
`+0x108` in RoR and `+0x104` in base; the portable semantic field hides
this edition-specific offset. Bounds and null checks reject malformed
links rather than reproducing original out-of-bounds behavior. Live
resource/player binding is still absent.
Kinds 1 and 8 (RoR `0x00510440`/`0x00510550`, base
`0x00433b40`/`0x00433c70`) count code-2 units matching the signed class
at condition `+4`, then set status 2 when the count meets the signed
target at `+0x1c`, otherwise 0. Kind 8 also requires each unit's
`+0x38/+0x3c` position to lie inclusively within four condition bounds.
Both always scan the owner first; with condition `+0x2d` and team mode,
they additionally scan mutually eligible team-mode players in ascending
index order starting at 1 (never neutral player 0). The portable code
keeps this distinction from kind 6's descending scan. Player/unit links
are still semantic and not yet bound to a running game world.
Kind 4 (RoR `0x005108d0`, base `0x00434000`) watches a unit reference at
condition `+0x24`: missing reference yields status 3; a unit code above 2
clears the reference and changes status 0 to 1; otherwise the unit's
`+0x38/+0x3c` position is tested inclusively against the condition's
four float bounds, yielding status 2 inside or 0 outside. This handler is
also translated over semantic unit fields, not yet bound to live units.
Kind 5 (RoR `0x00510940`, base `0x00434070`) compares two live units' X/Y
separations with the sum of their type extents plus one tile, inclusively.
The expansion's embedded constant is `-1.0` and is subtracted; the base
game's is `+1.0` and is added, so both produce the same threshold. A missing
unit reference sets status 3; a unit code at least 3 clears both references
and changes status 0 to 1; otherwise the proximity result sets status 2 or
0. `src/player_outcome_dispatch.c` preserves these branches over semantic
unit fields. Live object/type bindings are still absent.
Kind 9 (RoR `0x005107d0`, base `0x00433f10`) tests whether any unit of the
signed class at condition `+4` remains. If condition `+8` references a
player, it scans that player's unit list; otherwise it uses the manager's
owner at `+0xc`, checks nonzero owner eligibility bytes at `+0x84`, skips
the owner's player index at `+0x4a`, and scans each eligible world player.
A match sets condition status 0; no match sets 2. `src/player_outcome_dispatch.c`
ports both branches with checked semantic world/player/unit-list links.
Those links are not yet bound to live game objects.

The next per-frame player dependency is group maintenance: expansion
`0x0045f170` calls removal routine `0x0045ecc0`, while base `0x00518bf0`
calls `0x005186f0`. The expansion scans at most 25 slots, the base at most
40. Both keep members whose unsigned byte at object `+0x48` is under seven
and whose 32-bit field at `+0x20` is zero. Removal clears the object's byte
at `+0x36`, swaps the last counted member into the removed slot, decrements
the count, and replaces a removed leader with the first non-null slot.
The scan advances after a swap, so the replacement is not checked until a
later update. `src/player_group.c` ports this iteration/compaction order
over a portable semantic member model. It is not yet attached to live
player/unit objects.

Both executables use the same 32-bit game RNG: expansion seed/next routines
`0x00528410`/`0x00528420`, base `0x00532a70`/`0x00532a80`. The initialized
seed in each PE is 1. Each call updates the seed modulo 2^32 as
`seed = seed * 0x343fd + 0x269ec3`, then returns `(seed >> 16) & 0x7fff`.
The first eight values from seed 1 are 41, 18467, 6334, 26500, 19169,
15724, 11478, and 29358. `src/aoe_rng.c` implements this in explicit
unsigned arithmetic rather than using Vita libc `rand()`. `src/world_tick.c`
performs the tick's RNG checkpoint and restore around edition-specific
follow-up passes. The primary pass calls `aoe_player_update_step` directly,
but its live object methods and other external game bindings remain missing;
the unit tests do not establish a running simulation.

## Composite sprite draw dispatch

Rise of Rome `0x004a7110` draws a graphic directly only when its delta count
or delta pointer is zero. Otherwise it walks the 16-byte delta records, draws
records whose signed display angle is negative or equals the requested angle,
scales that angle by `child_angle_count / root_angle_count`, and adds the
delta X/Y offsets. Layer bytes below 11 select the secondary coordinate pair;
other layers select the primary pair. Static sprite method `0x00416190` adds
its instance offsets to both pairs before this dispatch, while animated method
`0x004166e0` supplies its current animation frame without static offsets.

The post-load fixup at `0x004a6970` is essential: delta graphic ID `-1` maps
to the owning graphic, while every other ID indexes the runtime graphic table.
This accounts for the apparent null-pointer delta records serialized in both
DATs. `aoe_sprite_draw_instance_plan_ror` and the indexed cached draw path now
preserve these rules. Original-data audits cover 240 composite roots and 804
angle plans in Rise of Rome, plus 209 roots and 718 plans in the base game,
with no unresolved deltas.

The official patched expansion preserves this chain at 1.0a `0x004a51c0`,
`0x004a5920`, `0x004a55e0`, and `0x004a7260`. The last argument selected by
tile traversal is a player/visibility color source, not the SLP graphic being
drawn. Lower dispatch reads its signed color at `+0x26`, selects that player's
16-color bank, and passes the transformation table at `+0x28` to the SLP
renderer when the DAT graphic has no fixed color. `AoeLiveUnitIndexedBackend`
now exposes `resolve_color_source` to translate that selected source into the
portable player color and transform table; callers without a selected source
retain the resolved owner-color/default-table fallback.
For DAT graphics whose signed color is nonnegative, the original instead
indexes the registered color-table array. The backend's
`resolve_fixed_color_table` and `aoe_sprite_draw_transform_table_ror` now make
that per-composite-part selection explicit, so a fixed-color child does not
incorrectly inherit the tile-selected transformation table.
`AoeColorTableSet` is the concrete portable registry, but it deliberately
does not infer a DRS resource from the ordinal. Release RoR `0x0041d4a0`
constructs `data/<DAT filename>`, opens that file, and scans 256 values into
the color object at `+0x28`; failure leaves the allocator-zeroed table in
place. `aoe_color_table_set_load_directory` preserves that behavior and
tracks which files actually loaded. The DRS 50909 helper remains a separately
verified portable source specifically for `Shadow.col`.

## Owned Rise of Rome scenario runtime

`AoeScenarioRuntimeRor` is the first production owner that joins the recovered
scenario loader, DAT civilizations, per-player template clones, shared map,
visibility masks, object lists, sprite instances, and ID registry. Construction
preserves scenario player order (Gaia then players 1..8) and record order. Each
record reserves its serialized ID before calling the recovered RoR live-unit
constructor and then selects the translated low (`state <= 2`) or high
(`state >= 7`) scenario virtual. States 3..6 deliberately fail until their
original callbacks are translated. Active-player selection gives exactly one
player vision grid the map's view-code array while retaining shared tile masks.

The real `EXPANS_1_1.scx` audit was reduced to a caller of this production API,
so its 3,809 simultaneous units now test the same owned runtime intended for
the Vita scene rather than duplicate setup code. The 3,772 low-state and 37
high-state calls all succeed under ASan/UBSan, all 70 host tests pass, and the
module cross-compiles with VitaSDK. This establishes scene ownership and load
lifetime; it does not by itself translate simulation, UI, input, AI, or every
state branch in other scenarios.

`aoe_scenario_runtime_draw_scene_ror` now closes the next production boundary.
The runtime owns a zero-initialized array for tile byte `+10` and growable
object-entry scratch. Each frame derives the selected player's visible and
explored bits from its recovered vision grid, combines the shared map masks,
TILEEDGE/BLKEDGE clip planes, camera, and original terrain art, and routes the
terrain walk's post-tile callback through the tile's actual linked live-unit
list into the same indexed surface. The resolver callback is intentionally
still required for original player color, type-25 controller state, screen
state, and class-3 alternate metadata; no display defaults were inferred.
The end-to-end owned scene seam has a sanitizer test and cross-builds for Vita.

The concrete real-asset caller is `tools/scenario_scene_tool.c`. It preserves
the executable's archive registration order (expansion graphics/interface
before base), uses the base terrain/border resources and TILEEDGE, maintains a
64 MiB decoded graphic LRU, and expands the resulting indexed surface only at
the output boundary. `EXPANS_1_1.scx` produces a 960x544 frame with 450 terrain
tiles and 231 drawn live objects. This is the first frame rendered from the
owned 3,809-unit world rather than a separate terrain/unit audit.

The render also exposed a filesystem semantic dependency: DAT color name
`Shadow.col` and installed file `shadow.col` match on the original Windows
filesystem. A case-sensitive host previously kept the allocated color table
zeroed, exactly as the original does for a missing file, causing black shadow
polygons. The portable loader now retries the DAT filename with ASCII case
folding, and the resulting shadow remap matches the original table. Player
color is still explicit caller state: constructor `0x0045c650` copies a byte
from its player-definition argument `+0x28` to live player `+0x54`; the current
SCN reader has not yet exposed that source field, so the engine does not infer
color from civilization or player index.

The scenario player-definition boundary is now more precise. World creation
at `0x0051fa40` indexes enable bytes at scenario object `+0x248`, civilization
bytes at `+0x251`, and passes `(player-1)*0x41` as the definition record to
player constructor `0x0045c650`. That constructor copies definition byte
`+0x28` to player byte `+0x54`, later serialized by `0x0045e000`; this is the
render color source. The reader's exposed `player_data_fixed_offset` and a
cross-campaign byte audit show that the final sixteen `0x2d0` records are AI
settings, not these 65-byte live definitions. Their construction path remains
to be traced.

The original global archive behavior matters in populated scenes. The real
dense viewport first reached DAT graphic `RGranary1`/SLP 722, which is absent
from all supplied archives/media. Original sprite dispatch continues after an
unavailable graphic, so the indexed backend now has an explicit, counted
`skip_missing_art` policy while keeping invalid/SHP/cache-limit failures hard.
The same scene required the interface archive's original `.col` text tables
when loose files were absent. Known VER 3.7 mapping is 50900..50908 for DAT
ordinals 0..8, 50910 for `Red_lt.col`, and verified 50909 for `Shadow.col`;
loose file tables retain precedence.

The deterministic dense-camera audit selects `(81.5,119.5)` in
`EXPANS_1_1.scx`: 542 projected tiles and 108 linked objects reach the scene,
107 draw, and six unavailable SLP attempts are skipped. This produces a real
settlement/army frame from original campaign state. It does not yet establish
combat animation because live action objects and per-frame unit updates remain
untranslated.

## Rise of Rome scenario-editor unit placement

The patched 1.0a editor screen vtable at `0x00547360` reaches the recovered
input handler `0x00493580`. Its unit-list builders `0x0046ab00` and
`0x00492c60` walk the selected player's cloned template array in object-ID
order and retain every non-null template whose byte `+0x4e` is zero. The UI
adds its separate “None” row. `aoe_scenario_editor_unit_*` reproduces that
catalog rule; the installed expansion DAT exposes 1,764 visible and 1,816
hidden civilization/template pairs across all civilizations.

Unit selection calls controller method `0x004a4320`, which stores the template
at controller `+0x214`, owner at `+0x110`, and owner color byte `+0x54` at
`+0x218`. `0x004920b0` then selects global editor mode 8, except for the
original special type-80/template-`+0x16c` branch into mode `0x15`.

The map-click placement body is patched `0x0051a2e0`. After hit-test code
`'3'`, it converts the screen point, resolves selected object ID at global
`+0x3fc`, calls template virtual `+0x24` to adjust coordinates, and passes
template byte `+0x64` to virtual `+0x20`'s placement test. A false test result
allows creation. Editor mode 8 normally calls player virtual `+0x94` with
`(object_id, x, y, 0, 1)`. Submode 1 is not a factory: the preceding editor
selection flow stores an existing live unit at view `+0x174`, writes state 7
to its byte `+0x48`, and the click invokes that unit's common placement
virtual `+0x34` with `(x, y, 0)`. The later mouse-up path restores state 2
and clears view `+0x174`. A null normal factory result is failure. The
resulting unit's `+0x18`
sprite collection is queried by patched `0x004159d0`; if absent, `+0x10` is
queried directly by `0x004a6030`. The latter returns the graphic's `+0x29`
layer when it has no deltas, otherwise the minimum direct delta-graphic layer
from sentinel 20. The collection query takes the same minimum over every
sprite, also returning 20 for an empty list. A zero result selects redraw
status 2; nonzero selects status 1.

`aoe_scenario_editor_place_ror` now preserves that branch and call ordering.
`aoe_scenario_editor_adjust_position_ror` translates the common release RoR
template `+0x24` method `0x00443e90`: footprint-edge clamping uses the exact
`0x3727c5ac` map-edge epsilon, and optional snapping uses the original four
signed-16-bit truncations and nearest whole-tile edge choice. Type 80's
`0x004eed30` is only a wrapper forcing that snap argument to one.

The normal editor player factory can now bind to
`aoe_scenario_runtime_editor_create_ror`. It resolves the selected player's
cloned DAT template and calls the same recovered live constructor used during
scenario loading. Editor units use stable individual allocations because
tile, owner, and ID lists retain their addresses; they participate in sprite
ticks and original teardown. The real `EXPANS_1_1.scx` test constructs its
3,809 serialized units, creates one additional editor unit through this
factory, and ticks all 3,810 under ASan/UBSan.

The shared release template `+0x20` method `0x00443700` is now translated as
`aoe_unit_placement_check_ror`. Its portable result values retain the
executable meanings: 0 accepted, 1 placement-terrain/side-terrain mismatch,
2 terrain restriction, 3 elevation-shape class, 5 visibility, 6 overlapping
unit (with blocker ID), and 7 map bounds. The method selects either collision
size `+0x30/+0x34` or placement clearance `+0x5c/+0x60`, checks the center and
outside footprint perimeter against `+0x54..+0x5a`, walks the complete tile
rectangle using terrain restriction `+0x66`, and applies the original
`+0x4b` build-on overlap exception before axis-aligned unit intersection.
The ABI's otherwise unread flag remains present and documented.

`aoe_scenario_editor_runtime_*` binds coordinate adjustment, the exact editor
argument set for that validator, and normal player creation to the owned
runtime. This work also exposed an active-player omission: runtime player
switching now installs the scenario's diplomacy row and active player's
distinct `+0x6c` explored-cell destination into every live-unit visibility
binding, rather than only rebinding tile view bytes.

`aoe_scenario_editor_runtime_relocate` binds the submode-1 call to the
already-translated common live placement and vision composition; no second
unit is allocated. `aoe_scenario_editor_runtime_begin_move` and
`aoe_scenario_editor_runtime_finish_move` retain the surrounding direct
state-7/state-2 writes and selected-unit lifetime, accepting only units owned
by that scenario runtime. `aoe_scenario_editor_graphic_layer_ror` and
`aoe_scenario_editor_live_layer_ror` translate that post-create layer query;
the owned runtime binds it directly rather than treating it as a generic
readiness callback. The real `EXPANS_1_1.scx` integration test searches for a placement accepted
by the recovered validator and then sends it through the recovered editor
click composition and runtime factory. It constructs and ticks 3,810 live
units under ASan/UBSan, then sends that same editor unit through the move
branch without changing the constructed-unit count. This completes the
known branches inside patched editor click `0x0051a2e0`. Host 75/75 and Vita
static-library builds pass.

The RoR building-completion event boundary is resolved. Global
`DAT_00585e88` is assigned by application constructor `00416950`, which
installs vtable `00547de8`; its `+0x40` slot is `00417910`, an unconditional
return. The completion call carrying event `0x69`, owner, template ID, and
truncated coordinates therefore has no observable effect in this executable.
The owned runtime binds that no-op directly; only the generic composition
retains an event callback for alternate integrations.

The adjacent optional stack-unit call is template vtable `+0x18` with the
same owner and live XYZ; its return value is ignored. The owned scenario
binding `aoe_scenario_runtime_building_spawn_stack_ror` now resolves the
building's semantic owner and cloned player template, then enters the
already-recovered live constructor. Dynamically created storage participates
in sprite ticks and symmetric runtime teardown. The generic completion layer
keeps the injection point so it does not acquire a dependency cycle on the
scenario owner.

The post-paint invalidator `00445760` is independently complete. It swaps
reversed signed bounds, clips them to the map, then writes `0xcc` to tile byte
`+7` and its argument to tile byte `+10` over the inclusive rectangle.
`aoe_runtime_map_invalidate_ror` preserves those bytes in separate semantic
arrays and the scenario completion adapter supplies draw value zero. The
surrounding deterministic paint body is now
`aoe_runtime_map_paint_terrain_ror`: it preserves signed-16 coordinates,
target availability/replacement admission, the target terrain's positive
frame-count gate for each existing elevation-shape byte, accepted terrain-ID
writes, one-tile expansion, `004496b0` border recomputation, and invalidation.
Each accepted write requires an explicit `004481b0` transition callback.
Its terrain-unit IDs, densities, placement flags, and count are already
parsed at DAT record `+254/+314/+374/+404`; only live-object transitions and
spawning remain.

`aoe_terrain_transition_cell_ror` now translates the `004481b0` control and
data behavior around those dynamic methods. It safely captures the next tile
node before removal, matches the terrain owner and exclusion interval, scans
the filter terrain's unit list, and applies every current-terrain unit entry.
Density is consumed in 1000-unit chunks. Placement mode 0 uses consecutive
world RNG values times the binary-verified `1/32768` offset; mode 1 uses
`+0.5,+0.5`; other modes preserve the routine's prior local coordinates. A
sub-1000 chunk consumes the original integer probability test
`rng*1000/32767 < remaining`. Only object owner/template access, class removal,
template placement, and template creation remain callbacks.

The owned scenario now resolves map-list core pointers back to their actual
portable live units and binds owner/template reads plus `+0x68` removal.
Removal uses `aoe_live_unit_scenario_high_root_ror` with the owning player's
resources, registry, vision, and world state, and fails rather than
approximating unsupported derived classes. A real campaign Gaia type-10
terrain decoration reaches state 7 through this path. The new
`dat_tool --terrain-units` audit confirms terrain 0 has 28 removable entries;
foundation terrains 7, 8, and 9 have no spawn entries in the installed RoR
DAT, although the general translated spawn rules remain required.

The remaining 004481b0 dynamic boundaries now have concrete owned-runtime
bindings. Its template `+0x20` check selects Gaia's cloned template and the
restriction row retained from `DatIntro`, using the verified argument tuple
`(require_visible=0, shape_mode=1, reserved=0, use_clearance=1,
honor_overlap=0, check_units=1)`. Its template `+0x18` creation path enters
the owned live factory for Gaia at Z zero. The scenario runtime therefore
retains caller-owned pointers to the parsed intro, terrain, and border tables;
all three must outlive it.

`aoe_scenario_runtime_paint_foundation_ror` is now the full owned composition
of 00449110 and 004481b0: admitted terrain writes, live-object displacement,
terrain density/placement spawning, expanded border refresh, and invalidation.
The real campaign regression first repaints Grass under an actual terrain-0
Gaia decoration and checks its state-7 removal, then paints Crop and checks
tile `+7 = 0xcc` and `+10 = 0`. This preserves the recovered ordering:
00449110 writes the target terrain before 004481b0 reads its terrain-unit
table. Because installed foundation terrains 7/8/9 contain no entries, the
test separately invokes the placement/create adapter with a terrain-0 DAT
entry. The resulting scenario constructs and ticks 3,812 live units.

`aoe_scenario_runtime_building_completion_world_ops_ror` now closes the
integration boundary between this owned paint path and type-80 completion. It
installs the owned stack factory, complete 00449110/004481b0 painter, and the
caller's following expanded 00445760 invalidation. Research and derived-player
UI state remain caller-selected. The runtime-specific paint callback returns a
status so unsupported removal/creation is not silently accepted; its wrapper
still lets the recovered generic continuation execute subsequent effects in
their original order before reporting the accumulated failure. The real
campaign test reaches painting through this installed callback.
