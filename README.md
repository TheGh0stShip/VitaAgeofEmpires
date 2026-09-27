# VitaAgeofEmpires

> A source-backed reconstruction of *Age of Empires I* targeting a faithful
> native PS Vita port. Rise of Rome is primary; the base game is retained.

**Status:** research and portable subsystem implementation in progress. There
is no playable port or installable VPK yet. See the [roadmap](TODO.md) and
[resume notes](docs/RESUME.md) for verified state and next work.

![Rise of Rome port progress status artwork](status/port-progress.png)

The image is a separately labeled status composite made from locally supplied
original-game assets; it is not a capture of the port. Its 10% is an approximate
full-port estimate, not a decompilation percentage.

## Build and test

Host tools and tests require CMake, a C11 compiler, and Python 3 for the status
art rule test:

```sh
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Release
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
```

VitaSDK is required for Vita cross-builds. Real-asset audits run only when the
user supplies their own game files at the documented local paths. No game
executables, archives, disc images, or music are included in this repository.

This is an in-progress reconstruction of PC *Age of Empires I* for a future
native PS Vita port. The primary target is the original *Rise of Rome*
expansion together with the base game. The base input is the 32-bit
`Empires.exe` and game data supplied locally; expansion input comes from the
Rise of Rome Rev 1 mixed-mode disc. No game files are included here.

For whole-program reference, `tools/ExportAoEDecompilation.java` exports
every Ghidra-discovered function from both executables in one pass. The
searchable dumps and address indexes are under ignored `build/decompile/`.
This does not guarantee that Ghidra discovered every function: a vtable audit
found 25 valid expansion-world methods missing from the initial export, and a
later pointer scan recovered hundreds more starts. Full Ghidra automatic
analysis, a broader aligned-pointer audit, and manually evidenced executable-
island recovery now yield 5,870 base and 5,849 Rise of Rome functions,
respectively, all reported decompiled. The pointer audit recovered three
additional expansion entries; remaining unclassified hits in that audit were
raw-byte checked as non-entry data or a mid-instruction reference.
The call graphs still contain 4,435/4,537 unresolved indirect call sites, and
42/39 data pointers land inside known functions. Both executable images have
zero undefined executable bytes, but 548/555 contiguous instruction islands
remain outside function bodies after fifteen/seventeen verified island
entries were recovered. An additional executable-section pointer audit found
address tables
for optimized copy kernels and recovered seven exact table-referenced entries.
The embedded-pointer report also includes jump-table case labels, which must
not be mistaken for functions. The equivalent base scan found the same kind
of switch case-table labels, plus a handful of computed-jump targets to triage.
Code without recognized aligned evidence and computed control flow still need
independent audits. These are
reference corpora, not complete or buildable decompilations. Inferred types,
unresolved control flow, platform APIs, and game behavior still require
verification and portable implementation.
`tools/ExportAoECallGraph.java` also exports call-site inventories for both
analyzed executables as revision-qualified TSV files under
`build/decompile/`.
Unresolved calls remain explicitly marked; the inventories are navigation
evidence, not proof of full dynamic-call coverage.
Microsoft's combined `AOEPATCH.exe` is also integrated through
`tools/import_patch_revisions.sh`. It verifies and indexes the complete AoE
1.0c and Rise of Rome 1.0a replacement executables in separate Ghidra
projects and revision-qualified exports. See `docs/PATCH_REVISIONS.md` for
hashes, counts, and the patch behavior audit queue. Supply your local patch
installer explicitly, for example:

```sh
GHIDRA=/path/to/ghidra tools/import_patch_revisions.sh /path/to/AOEPATCH.exe
```

The installer and extracted executables are intentionally ignored by Git.

There is **no playable port or installable VPK** yet. A previous diagnostic
asset browser and invented skirmish were removed because they did not
reproduce the original game. Vita builds currently cross-compile shared
reconstruction libraries only. Do not treat any old VPK left outside this source tree
as a current game build.

The first requested alpha milestone is a Vita-rendered scene from an actual
Rise of Rome scenario: original town, units, terrain, and art. It need not
accept gameplay input. Fighting on screen is a subsequent part of that
milestone and must come from ported original unit actions, attacks, damage,
and update rules; scripted imitation combat is out of scope.
The [Wonder progress artwork](status/README.md) is a configurable, reproducible
original-asset composite for status updates, not a capture of this alpha.

The source-backed terrain compositor can draw original campaign tiles and
borders, and the asset tools can render individual original sprites and
scenario minimaps. These are host-side audit outputs, not a packaged teaser
viewer or interactive game. A new owned runtime-map layer copies verified
square-scenario tiles into the original map's semantic terrain, border,
object-list, and vision surfaces; it currently has synthetic integration
tests and a real-asset audit of all 44 available scenarios, not a live
campaign loop. The audit checks the loaded tile values, derived terrain
shapes/borders, projected origins, and empty initial object/vision state.
An additional host audit feeds every original Rise of Rome scenario unit
record through the recovered unit-core initialization and placement/vision
path on that map; all 19 extracted expansion scenarios pass. It deliberately
does not perform the later original state dispatch, world/player registration,
AI, or object updates, so it is not a running campaign.

Reconstructed components include DRS archive lookup, `2.0N` SLP decoding,
JASC palette parsing, `Empires.dat` decompression and early tables, and a
`1.10`/`1.11` scenario header/decompressor/map reader. For `1.15`
scenario bodies, the loader follows the original player-data layout to the
tile grid and reads player setup, eight resource sets, and nine groups of
placed units. It also reads final player names, camera positions, diplomacy,
and victory-condition records, reaching the end of all 24 installed scenarios
and the demo's `Multip_1.scn`. The original terrain elevation-shape,
border-pattern, isometric tile-origin, direct terrain-frame, and border-frame
decisions are also reconstructed.

## Project policy

Reverse-engineering notes distinguish verified behavior from hypotheses.
Function export counts do not imply complete discovery. See
[`CONTRIBUTING.md`](CONTRIBUTING.md) for evidence and test expectations and
[`TODO.md`](TODO.md) for the full port roadmap. The repository does not grant a
license to redistribute or use the original game; original game assets and
executables remain the user's responsibility and are excluded from version
control.
`aoe_scenario_spawn_one` now translates the original scenario-placement
dispatch for both games: it sets the world spawn ID/loading flag, passes the
record's object ID and X/Y/Z to the player unit factory, and selects one of
three original state-method categories (0–2, 3–6, or 7+). The complete unit
factory and post-spawn state dispatch remain unported; this does not create
gameplay units yet.
`aoe_unit_template_factory_spec` now records the original civilization
template allocation dispatch: seven common types plus the derived
civilization's type-70, type-80, and type-90 overrides. All present unit
definitions in the installed base and Rise of Rome DATs match this dispatch.
The template constructors, player clones, and live-unit factory methods are
still incomplete; matching a type does not create a gameplay unit.
`aoe_player_templates_clone_civ` now ports the player constructor's slot-array
cloning and independent ownership for every field currently exposed by the
DAT parser. It has been exercised against all installed base and expansion
civilizations, but unparsed original template fields and methods remain.
The common live-unit constructor's positive/negative world-ID choice is also
translated from original template byte `+0xa4`, using the existing portable
world ID allocators. Complete object allocation, graphics, and registration
remain incomplete. `aoe_live_unit_core_begin` also translates the common
constructor's directly recovered initial HP/state/position and storage
effects, but it is not a complete live unit. The original 17-case terrain
surface height and template elevation-mode Z adjustment are now translated
for scenario tiles. The Rise of Rome placement method's coordinate clamp,
per-tile list relink, and state-2 leave/enter call order are also available in
`aoe_live_unit_place_ror`; the remaining leave/enter effects and full
registration are unported. Scenario-to-runtime tile order is verified for
the installed square maps, not arbitrary rectangular scenarios.
The callbacks' common visibility-map branch is now translated separately in
`aoe_visibility_footprint`: it uses the original radius-0–16 row spans,
per-player signed visibility counts, shared tile masks, and explored-cell
records into distinct owner and active-player destinations.
`aoe_live_unit_place_ror_with_vision` now composes tile placement with that
visibility path using parsed unit line of sight and explicit runtime owner,
active-player, and relation fields. `aoe_player_vision_state` now allocates
the owner count grid and discovered-cell vectors. The alternate RoR unit
visibility branch now draws its inclusive template-size rectangle into those
same count/mask surfaces, but state methods are not yet bound to it. The
original RoR
positive-health registration toggle and root middle-state HP clear are also
ported in `aoe_live_unit_core`, with required owner-list callbacks; they are
not yet wired into post-spawn state dispatch. `aoe_live_unit_state` now ports
the root Rise of Rome and base-game state setters' distinct acceptance rules
and ordered effect boundaries, including vision mode changes and health
reclassification. Its
exit-active mode-2 resource subtraction is now translated directly from
the three original template storage entries. The other
unported sprite/action, derived-class, and owner methods remain mandatory
callbacks, so it cannot stand in for a live game unit.
The shared zeroed tile-mask grid and owned runtime map are available, but
these pieces are not yet connected to a complete live factory or world loop.
The original map reader confirms that installed square scenarios use
`row[y][x]` tile order; rectangular scenario mapping is deliberately not
assumed.
An optional real-data audit runs all 14,923 placements in
`EXPANS_1_0.scx` through the DAT/template/core/surface path with the
scenario's reserved spawn IDs; it does not render or register units.
The original direct terrain SLPs can now be loaded, validated against DAT
shape ranges, and composited as palette-color/transparent pixels, but
non-direct terrain semantics and the viewport renderer are unfinished.
The six available direct border SLPs also load and validate; the unavailable
"Non-visible" border slot remains undrawable.
The original tile-draw caller's one-step terrain replacement and ordered
base/border layer decisions are implemented as a portable draw plan, but
only hotspot-aware sprite compositing is implemented. Full viewport/camera
placement, fog/palette effects, object drawing, and gameplay remain.
The original viewport's camera-pixel subtraction and diagonal map-tile visit
order are now portable library routines. Obtaining live camera state and
compositing a complete clipped game screen remain unfinished. The original
float-camera-to-pixel and sweep-start calculation is also available for a
caller-supplied runtime camera and viewport rectangle, but is not connected
to live game state.
The expansion's camera clamp and viewport-to-tile-count arithmetic are also
implemented; they are primitives, not a running game camera controller.
The portable terrain layer can now traverse the original viewport order and
composite original direct terrain/border art into a clipped RGBA rectangle.
The shared original world-clock gate/time-advance rules are also ported in
`aoe_world_clock`: local elapsed-time capping, synchronized deltas, speed,
one-shot pause, and first-frame update gating. This does not yet run the
original player update passes.
The original signed-bounds player-resource set/add methods are ported in
`aoe_player_resources`; technology effects and economy rules are not yet
connected to them.
The original per-frame player-group pruning and swap-last removal are also
ported, with the base game's 40 slots and Rise of Rome's 25 slots. They are
still data-model components, not live game-unit updates.
The original 32-bit deterministic RNG is also available as `aoe_rng`, with
the same seed and result sequence as both PC executables. `aoe_world_tick`
now connects it to the executable-derived base/Rise of Rome world-update
order: timers, primary player pass, edition-specific follow-ups, RNG
checkpoint/restore, and seed synchronization. Its gameplay callbacks are
required but not implemented; this is not a running simulation or playable port.
The pre-player queued-command drain (RoR `0x0042a5d0`, base `0x0041f0b0`)
is now direct code in `aoe_command_drain` and the world tick. Real command
fetch/decode, dispatch, and release bindings remain necessary; this does not
create a functioning command pipeline.
The second pre-player call is now an original timed-scenario-action scheduler
in `aoe_scenario_timeline`: it advances against game time, invalidates stale
object references, runs due action kinds 0/1/2/4, and propagates spawned
object references. The referenced live object methods and scenario timeline
construction remain unported, so this is not a playable campaign system.
The primary player pass in `aoe_world_tick` now calls
`aoe_player_update_step` directly for each player, using the appropriate
base/Rise of Rome group size. This connects the translated object-list,
group, outcome, state-transition, and queue-reset sequence; object virtual
methods, map work, and event delivery still need real game bindings.
The post-checkpoint object-list pass also now uses the original player
`+0x30` list directly in both editions. Base player virtual `+0x80`
combines a per-player cooperative check, that list traversal, and a separate
`+0x40` routine; Rise of Rome runs the list pass for all players before its
separate `+0x40` pass. The shared `+0x40` visibility-watch logic is now
translated in `aoe_visibility_watch`: it detects cleared recipient bits,
notifies non-owner players, and preserves each edition's different invalid
unit removal rule. The original world unit-ID lookup, insertion, and slot
clearing are now translated too, using owned positive-ID and negative-ID
arrays. Their constructor starts with the original 10,000/1,000 slots, and
the shared positive/negative ID-allocation rules are translated. The world
tick references the live registry view so growth cannot
leave it with a stale pointer. Binding actual unit creation/destruction,
the special positive-ID replacement's displaced-unit methods, and recipient
methods remains. Insertion explicitly reports when that reassignment is
required, and the registry also has the original reset's ID-table clearing
portion; neither substitutes for the complete unit teardown.
The shared player-side unit register/remove path now routes objects to the
original `+0x28/+0x2c/+0x30` lists, links/unlinks list nodes, and updates the
world unit-ID registry. Unit constructors, full object methods, and the
displaced-unit reassignment still need to be bound before a live game runs.
The first DAT-driven unit-constructor behavior is also direct code:
`aoe_unit_storage` selects the original carried resource from the three
parsed storage entries and applies mode-1/2 resource grants to the owner.
This is not full unit construction or a running economy.
`aoe_unit_graphic` now translates the base-unit graphic-switch method from
both executables. It preserves runtime graphic-pointer identity, the
standing-graphic fallback, sprite-manager remove/add order, and the Rise of
Rome-only guard that keeps a walking graphic during animation states above
two. `aoe_sprite_list` now translates the original sprite manager's
priority-ordered add/remove/clear operations, including duplicate references
and animated-graphic invalidation. Real graphic-object construction, SLP
animation, and Vita drawing still need to be bound before units render.
`aoe_sprite_animation` translates the animated sprite's frame-update method
from both executables, including timing, loop/replay delay, and one-shot
stopping. `aoe_sprite_instance` now constructs static or animated instances
directly from parsed DAT graphics, using the original RNG for randomized
starts. Every present graphic in the installed base and Rise of Rome DATs
instantiates; base graphic 834 has a documented zero-duration, one-frame
exception. Drawing the resulting frames still requires the Vita renderer.
`aoe_sprite_runtime` now binds DAT-backed instances to the original
priority list and advances them in list order. `aoe_unit_sprite_binding`
connects the unit graphic-switch rule to that runtime, so a switch acquires
and releases actual instances. It acquires the replacement first to keep
the old graphic intact if allocation fails; this is a documented portable
failure-safety difference from the original call order. Unit construction
and on-screen drawing remain incomplete.
`aoe_graphic_frame` now selects the original SLP frame and horizontal-mirror
flag for a sprite's angle and animation frame. It is connected to
`aoe_sprite_instance`, so the next renderer step can fetch the selected
frame from the original DRS art rather than inventing imagery.
`aoe_graphic_art` now resolves that frame through the original SHP-then-SLP
DRS lookup order and decodes the real SLP pixels. An optional bounded
decoded-frame cache is available; drawing is joined through `aoe_sprite_draw`
below. Installed-file sweeps loaded one frame
from 722 base and 794 expansion graphic records; others lack direct DRS art.
Some DAT angle declarations exceed their SLP frame tables (91 base and 92
expansion angle/frame combinations), so out-of-range requests are rejected
explicitly rather than redirected to fabricated artwork.
An all-frame audit decoded 12,625 base frames from 681 unique direct SLPs and
15,127 expansion frames from 752, with no malformed frames or outline pixels.
Even with every registered DRS archive, 37 base and 63 expansion positive
SLP IDs remain unresolved; these original asset gaps are reported rather
than replaced with invented sprites.
`slp_blit_graphic_rgba` now places decoded frames with the original normal
or mirrored hotspot offset and clips them to an RGBA target. Ordinary palette
pixels use the original JASC palette. Player-color, shadow, and outline
pixels are passed to a required resolver callback. The original player-color
OR transform is now available for frames without shadow/outline pixels;
shadows use the indexed path below, while outlines remain unsupported.
`aoe_sprite_draw` now joins DAT sprite instances, original frame selection,
DRS/SLP decode, and RGBA placement in sprite-list order. Static sprite
offsets are applied where the original vtable applies them; animated
instances keep the unshifted anchor. A real base and Rise of Rome test draws
the original `Arrow Fire` graphic with 126 palette pixels in each edition.
The optional cache keys frames by resolved archive entry and frame number,
preserving expansion-over-base precedence; oversized frames fall back to
direct decoding. Real-file tests confirm cached and uncached pixels match in
both editions. No frames are submitted to the Vita yet.
The player-color draw variant uses the graphic's nonnegative DAT color ID or
the owning unit's supplied color ID, matching the original dispatch. Both
editions render the original `Arrow B fire` graphic's 17 player-color pixels;
the unresolved-effects path rejects it instead of drawing incorrect colors.
The sprite path can also draw into an indexed 8-bit surface, matching the
original palette-based renderer. Ordinary and player-color pixels write
indices; shadow pixels transform the existing destination index through a
caller-provided 256-byte table. `aoe_shadow_table_load` resolves the original
table from base `Interfac.drs` resource 50909; it is byte-for-byte identical
to the installed `data/shadow.col`. The real `Cliffs s2` graphic selects DAT
color 10 (`Shadow.col`) and its 82 shadow pixels pass with that table in
both editions. Outlines are still rejected.
`palette_expand_indexed_rgba` converts the resulting original 8-bit indices
through the parsed game palette for a display buffer. The real shadow test
checks this conversion as well; it is not yet connected to a Vita screen.
The direct terrain/border pass also has an indexed 8-bit draw path. It keeps
the original clip and dither results as indices, allowing later sprite
shadows to remap actual terrain pixels rather than an RGBA approximation.
This per-plan path passes original base and Rise of Rome terrain-art audits;
the visibility-aware viewport can now write the same indexed surface and
matches the RGBA route on base and expansion map audits. A live game still
must provide runtime visibility and draw its units into that surface.
Original per-tile object-list filtering is now translated for both editions
(`aoe_tile_object_visit_draw` for Rise of Rome and the base variant). The
viewport calls this object phase after each tile's terrain pass and preserves
the tile list's existing order. The portable rule still needs live object
records and unit draw callbacks before it can place units on the viewport.
The sprite-list draw dispatch continues past missing art, as the original
void traversal does. The portable API reports the first error after visiting
all sprites; real-data tests verify a later valid sprite still renders.
The per-player queue reset immediately after the primary update is now a
direct translation of RoR `0x0045fad0` / base `0x00519670`, operating on
player `+0x58`; it no longer needs a placeholder callback. The reset clears
two counters and, when marked dirty, replaces storage with eight eight-byte
entries. Remaining world-tick callbacks still require original game code.
The same queue module now also translates the original initializer, two-word
append with capacity doubling, storage/count view, pending-count drain, and
destructor for both editions. It preserves the base game's minimum initial
capacity and growth limit; this is queue behavior, not live game commands.
The timer rotation itself is now translated in `aoe_world_timers`, including
both selected-object arrays and the second timer's null-slot skip. The
selected objects' timed-event and lifetime action state machines are also
translated in `aoe_timer_actions`, with bindings back to the selector.
`aoe_audio_target` also ports the timed event's terminal-chain cleanup
order and the original playback-stop condition. The voice stop/release,
buffer-free, and lifetime handle-cleanup bodies remain platform-specific
dependencies.
The next shared player dependency, `aoe_object_update_list`, ports base
`0x0049ce80` / Rise of Rome `0x004521e0`: linked-list traversal with a
cached next node, object virtual-method dispatch, and a cooperative pump
after each tenth non-null object. The object methods themselves are still
unported and must be supplied before the player update can run.
`aoe_player_update` now composes that traversal and player-group pruning in
the original player virtual-method order (RoR `0x0045df70`, base
`0x005178f0`). `aoe_player_outcome` also translates the shared event and
condition list pass and the grouped win/loss decision, and is connected to
player update. `aoe_player_outcome_dispatch` ports all five original event
handlers and the condition kind switch, including condition handlers 0–10
and 12. Kind 0 tracks a referenced unit's owner; kinds 1 and 8 count
active units, with kind 8 applying a map
region; kind 3 preserves the original accumulating count of code-7 units
and its signed threshold; kind 6 checks a resource total with
the original optional mutual-alliance rule. Kind 7 checks a player progress
percentage; kind 12 averages team score. Player update now calls that
dispatcher and the original state-change rule directly. Live-object binding,
global event delivery, map work, and object methods remain required
original-game dependencies, not substitute gameplay.
The original terrain-only path omits fog, units, overlays, UI, and live simulation; it is
not a diagnostic game or a playable port. A real-asset audit draws a centered
terrain-only viewport for all 44 available scenarios under sanitizers.
The original fog-edge neighbor lookup is ported, and the installed base
`TILEEDGE.DAT` offset tables are parsed and validated. Its 1,581 non-null
streams decode to 45,367 bounded scanline spans; those spans can now add to
or subtract from a clipped byte coverage mask. The direct-art 8-bit
mask/fill path is implemented, but complete fog and black-edge fidelity is
not verified against the original screen.
Rise of Rome's optional `BLKEDGE.DAT` one-stream tables are also understood,
and the viewport's visible/explored/hidden mode now selects the original
two clip-plane streams. `tileedge_apply_view_plan` executes their add and
optional black-edge subtraction sequence on two caller-owned masks. No
`BLKEDGE.DAT` is present in the available assets, so that format has
synthetic tests only; neither mask is connected to a running game yet.
The expansion tile-draw routine's two-pass order and 8-bit palette-index
masks are now ported as a draw-pass plan and a clipped terrain/border SLP
compositor. `terrain_draw_view_visibility_rgba` now accepts a caller-supplied
runtime visibility grid and tile flags, builds both clip planes across the
original viewport sweep, and draws direct terrain/borders through them.
There is still no running game state or finished Vita viewport.
All 19 available Rise of Rome campaign maps pass a sanitizer audit that
compares the new normal pass against the existing terrain compositor for
each distinct original frame sequence and runs the dither pass as well.
Mixed visible, explored-fog, and hidden-grid viewport audits also pass for
all 19 expansion and 24 installed base maps with original terrain art and
the installed `TILEEDGE.DAT`; those grids are test inputs, not simulation.
`terrain_draw_real` audits original scenario tiles against the real DAT/DRS
assets; all 19 expansion, 24 installed base, and one demo maps pass under
sanitizers. This checks the current terrain/border path, not visual fidelity
of a complete game screen.
Gameplay rules, UI, and audio playback are not implemented.

The expansion disc's MODE1 data track can be converted with
`cdtrack_tool` into an ISO under ignored `build/ror/`. Its `Empiresx.exe`,
`Empires.dat`, three added DRS archives, and four campaign files have been
extracted there for analysis. The shared DAT reader parses the expansion's
sound, graphic, terrain, border, map-geometry, random-map, and effect tables.
It also parses all 127 expansion and 118 base technologies through the exact
end of each decompressed DAT, including prerequisites, costs, research sites,
times, UI identifiers, and links to effects.
It exposes effect commands, civilization resources, and unit identity,
graphics, size, health, sight, and speed in portable C. Variable-length unit
records expose task targets/work values, attack and armor class amounts,
combat range/reload/projectile values, resource storage and costs, and
training fields. These are data definitions, not implemented combat or AI.
Every placed unit in the 44 parsed base/demo/expansion scenarios resolves
to a corresponding DAT unit definition. The campaign
reader extracts all 19 embedded `.scx` files, and the scenario reader parses
their maps and preserves their briefing bitmaps. Later game rules and
executable behavior still need reconstruction. The terrain elevation-shape,
border, and isometric tile-origin routines have now been located and
verified in both executables; direct terrain-frame selection is also verified,
while full drawing remains open.

Build and inspect the original data on the host:

```sh
export AOE_ROOT="/path/to/Age of Empires"
cmake -S . -B build/host
cmake --build build/host
./build/host/drs_tool list "$AOE_ROOT/data/graphics.drs"
./build/host/slp_tool info "$AOE_ROOT/data/Terrain.drs" 15000
./build/host/dat_tool "$AOE_ROOT/data/Empires.dat"
./build/host/dat_tool build/ror/disc/GAME/DATA2/EMPIRES.DAT --factory-audit
./build/host/live_unit_surface_real build/ror/scenarios/EXPANS_1_0.scx \
  build/ror/disc/GAME/DATA2/EMPIRES.DAT
./build/host/runtime_map_real build/ror/scenarios/EXPANS_1_0.scx \
  build/ror/disc/GAME/DATA2/EMPIRES.DAT
./build/host/runtime_units_real build/ror/scenarios/EXPANS_1_0.scx \
  build/ror/disc/GAME/DATA2/EMPIRES.DAT
./build/host/dat_tool build/ror/disc/GAME/DATA2/EMPIRES.DAT 0x21e8c 0x80
./build/host/dat_tool build/ror/disc/GAME/DATA2/EMPIRES.DAT --unit 1 83
./build/host/scn_tool "$AOE_ROOT/scenario/Multiplayer Intruder (2-Player only).scn" \
  "$AOE_ROOT/data/Empires.dat"
```

Cross-compile the shared libraries with VitaSDK:

```sh
export VITASDK="/path/to/vitasdk"
cmake -S . -B build/vita \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake"
cmake --build build/vita
```

See [docs/reversing.md](docs/reversing.md) for executable addresses and
format evidence, and [docs/RESUME.md](docs/RESUME.md) for the next original-game
targets. The mounted `F:` disc exposes no CD audio tracks, but the expansion
BIN/CUE's 14 audio tracks have been extracted locally as lossless 44.1 kHz
stereo WAV files in ignored `assets/ror_audio/` using
`tools/extract_ror_audio.sh`. They are not yet used by a Vita audio player.
The separate
`E:\msaoe.exe` demo installer contains 12 MIDI music/cue files and six
ambient WAV files, extracted locally to `assets/demo_audio/` (ignored by
source control). These are demo assets, not a rip of retail CD audio.
