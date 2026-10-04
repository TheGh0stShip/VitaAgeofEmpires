# Resume notes (2026-09-26)

The objective is a faithful native PS Vita port of PC *Age of Empires I*,
with the original *Rise of Rome* expansion as primary target and the base
game retained. The user explicitly disallowed diagnostic prototypes. The
invented skirmish and asset-browser Vita executable were removed from source;
no VPK is produced. Any old `build/vita/VitaAgeofEmpires.vpk` or installed
`VAOE00001` from earlier experiments is obsolete and must not be presented as
the port. The emulator profile contains test copies of six original data
files and one `.scn` under `ux0:data/VitaAgeofEmpires/`.

Completeness correction (2026-09-26): earlier counts of functions found by
Ghidra were incorrectly treated as a sufficient sweep. The refreshed pipeline
now runs full automatic analysis on patched 1.0c and Rise of Rome 1.0a, then
audits pointer runs, every aligned executable pointer in initialized data,
table-supported missing starts, and call sites before exporting all
discovered functions. Current reference exports contain 5,870 base and 5,850
RoR functions, all successfully decompiled. Call graphs contain 31,055/31,652
sites, with 4,435/4,537 indirect or unresolved. The earlier aligned-pointer
audit recovered three additional expansion entries; its residual targets were
raw-byte checked. There are still 42/39 interior pointers into known functions.
This is not exhaustive: computed/register-based control flow and non-pointer
entry mechanisms need continued audit. Never call these counts proof of a
complete decompilation.
The read-only `tools/AuditAoEEmbeddedExecutablePointers.java` pass now covers
both revisions. It found executable-section pointer tables for RoR's optimized
copy kernels (seven exact return-bounded entries recovered) and a base
stream-decoder/initialization table at `0x0051eed4..0x0051eeec` (six entries
recovered). It also reports switch-table case labels and computed-jump
candidates, so those require control-flow analysis and must not be promoted
blindly. Reports are
`build/decompile/base_10c_embedded_exec_pointers.tsv` and
`build/decompile/ror_10a_embedded_exec_pointers.tsv`. Their
`jump_table_base` column flags entries within 256 bytes of computed-jump table
bases as a triage hint only. The listing-backed audit
is implemented as `tools/AuditAoEExecutableCoverage.java` and integrated into
`tools/import_patch_revisions.sh`. Both patched images have zero undefined
executable bytes. Current exports contain 5,870 base and 5,850 RoR
functions, all decompiled; their call graphs have 31,055/31,652 sites, with
4,435/4,537 unresolved. Base accounts for 1,286,144 executable bytes
(1,215,443 instruction, 70,701 defined data), of which 6,185 instruction
bytes remain outside function bodies in 548 contiguous islands. RoR accounts
for 1,314,816 bytes (1,244,843 instruction, 69,973 defined data), with 7,528
outside-function instruction bytes in 555 islands. Fifteen base and eighteen
RoR entries supported by raw disassembly and exact references have been
recovered and are in the pipeline. The remaining islands are not automatically
missing functions; classify each against data/code references, flow boundaries,
exception metadata, and switch tables. Keep indirect calls unresolved until
their dynamic targets are supported by binary evidence.

Next sweep targets: base `0x0051c950` (221 bytes) and the remaining subregions
of the `0x0051f31f` copy/decoder island and `0x0051f61c` initialization region;
these contain multiple internal function candidates and shared tails, so
reconcile exact entries against current Ghidra function bodies before recovery.
RoR `0x005256df` is now a verified seven-byte call/jump thunk from table slot
`0x0052529c`, reaching shared code at `0x005256b7`. The residual
`0x005256e6..0x005259bf` region (730 bytes) remains outside functions and needs
boundary/control-flow triage. The `0x00521f2f` island follows recovered
copy-kernel `0x00521e8f`; inspect its internal labels without promoting them
blindly.

Revision-mixing guard (2026-09-26): `docs/ror_revision_map.tsv` is the
durable verified map between disc RoR and patched 1.0a addresses. Do not use
address proximity. Disc live root vtable `0x0054ce80` maps to patched
`0x00547da8`; the same patched address `0x0054ce80` is RTTI data. The common
live constructor maps `0x004a8200` → `0x004a6a40`, player-list cleanup slot
`+0x08` maps `0x004a80e0` → `0x004a6920`, and update predicate slot `+0x24`
maps `0x004aa790` → `0x004a8f40`. These were accepted only after mnemonic
matching, decompile comparison, constructor assignments, and vtable slots
agreed. Continue the live player-update binding from these patched methods.

The first source-complete portion of patched live predicate `0x004a8f40` is
now in `aoe_live_unit_update_prelude_ror`. It applies deferred byte-`+0x4b`
health-category registration through real player/world lists, returning the
original early-handled result, or stably prunes the owned `+0x64/+0x68/+0x70`
dependent-unit ID vector when lookup fails or the target state exceeds 2.
Teardown frees that vector. The following `+0x20` branch is also translated as
`aoe_live_unit_update_attached_target_ror`: the field is now typed as an
attached `AoeLiveUnitCore`, suppresses the normal switch whenever non-null,
and propagates attached states above 2 through a mandatory current-class
vtable-`+0x5c` callback. Focused tests cover absent, low-state, missing-hook,
and state-7 targets. The remaining normal switch is now translated in
`aoe_live_unit_update_normal_state_ror`: state 2's `+0x28` call and fallthrough,
negative-HP death/graphic/sound order, states 3/5 decay or undead/terminal
paths, state 7→8, and state 8's template-`+0xa4` result. The DAT parser now
retains template undead graphic/mode (`+0x20/+0x24`), decay rate (`+0x80`),
and selection/dying sound IDs (the latter resolves to runtime `+0x44`).
`aoe_live_unit_update_predicate_ror` composes all three portions.
The vector's patched writer `004abc80` is now translated too:
`aoe_live_unit_set_dependencies_ror` resets the active count, ignores negative
and duplicate IDs, retains/grows its allocation one explicit 32-bit element at
a time, and preserves source `+0x78` plus opaque 32-bit context bits `+0x80`.
Counts/capacity no longer inherit host `size_t`; they are `uint32_t`, and the
allocation gate retains the original/Vita 32-bit byte domain on LP64 hosts.
Single-ID `004abdf0`, `004ac030`, and `004ac1b0` are now translated too. Add
preserves the strict 25-active-entry gate and unread second ABI argument;
remove deliberately scans/shifts retained capacity (including stale slots)
and clears source `+0x78` when active count reaches zero; clear retains the
allocation. Focused tests cover these counterintuitive boundaries.
Patched group propagator `004abea0` is now composed over those operations: it
pre-resolves the target, copies current IDs in order, adds target to every
resolved non-self member, and rebuilds the caller with caller ID/context
metadata. The resolver is explicit rather than assuming arbitrary registry
objects use the portable container layout; nested add results remain ignored
as in the executable.
Group remove/clear `004ac0a0`/`004ac1d0` and group set `004abd80` complete
the family. They are bound to the patched dependency command opcodes
8/9/0x0e/0x0f of dispatcher `0x0042a180` by `aoe_command_execute_dependency_ror`,
which includes byte-exact builders for the 0x74/0x08/0x10/0x0c records, the
owner `+0x4a` word gate, and a documented host guard for set counts above 25.
Opcode 5 (player resource add) is translated with patched RoR player
vtable `0x00544d18` and its derived `+0x78` override `0x004f6c60`; the
player-resource header now carries patched rather than disc addresses.
The remaining dispatcher opcodes (0–4, 6, 7, 0x0a–0x0c, 0x10, 0x11) are
untranslated; `0x00520c60` (world refresh) is still an explicit callback.
`command_dispatch` now reproduces the `0x0042a180` routing (jump table
`0x0042a230`, opcode 7 via processor vtable `0x00543498` slot `+0x00` =
`0x0042ad40`) and reports routed-but-unbound opcodes rather than dropping
them. See `docs/RELATED_PROJECTS.md` for the 2026-10-03 upstream survey and
work-reduction plan.
Opcode 7 (tribute) is translated with its exact `__ftol` reporting. The
derived tribe router `0x004e83e0` is modelled by
`aoe_command_dispatch_tribe_ror`: one CRT `rand()` step per command, an
opcode-4 override, and opcodes 0x64-0x6f/0x77, all reported as UNBOUND until
translated. The tribe processor is now traced as the one real sessions
construct. Opcode 4 is translated for both processors, including the
tribe override's 4.0f elevation and instant building completion through
`0x004ad7b0`. Root setter labels are now the patched `0x004aa550` (RoR)
and `0x004a9430` (AoE 1.0c), and its state-3 dependency-group leadership
handoff is translated. Tribe opcode 0x77 (production queue add/cancel,
`0x004adab0`) is translated, along with the tribe-player cost
check/pay/refund `0x004f15f0`/`0x004f1750`/`0x004f17f0` (including the
`values[29]` gold-substitution budget) and queue-ready adapters. Base
opcode 0 (unit order, with its multiplayer validity gate, formation offsets
and the `+0x54` group vector) is translated. So is task selection, the
decision step of unit `+0x98 -> +0x1f8` (`0x004e7dc0`, `0x00508010`,
predicate `0x004e7f90`). Next: the rest of `0x00405f20` (action creation
through the action manager `0x004045b0/0x004045c0`), tribe opcodes
nothing: every tribe opcode (4, 0x64-0x6f, 0x77) is translated, including
fee tribute 0x6c, diplomacy, building placement and walls. The command layer
is complete. The order-to-action step `0x00405f20` is translated
(`src/unit_order.c`), including the villager job switch, head-action
retargeting, the action routing table and acknowledgements. Next: the
individual action classes behind the routed constructors. The move action
(`+0x9c`, type 1) is translated against an explicit movement interface;
next are gather 5, attack 7 and build 0x65, and the unit movement system
(`0x0044be90`..`0x0044c2d0`) itself. The pathfinder search `0x00458930`
and path builder `0x0045a970` are now translated (`src/pathfinder_search.c`).
Next: movement start `0x0044bf00` (path budget `0x0045c180/0x0045c1c0`),
the movement step `0x0044c2d0`, and obstruction registration
`0x004ab7f0/0x004ab6e0` that fills the occupancy bitmap. Validating against
original-game traces remains outstanding. Note `0x00403e70` is the base list's
stream loader (slot 0), not the order path.
Every base
opcode (0-0x11; 0x0d is a no-op) is translated.
`aoe_live_unit_player_update_step_ror` binds it into actual player +0x7c
object-list traversal by matching listed `+0x38` references against an
explicit owned-unit roster; unrelated list objects still go through required
dynamic-class predicate/action adapters. It forwards class-specific live
`+0x08` cleanup, cooperative pumping, world delta, and binary epsilon as
runtime bindings. Vtable audit established `+0x08` targets: `0x004a80e0`
for types 10/20/25/30/90, `0x00405a70` for types 40–70, and `0x004ae110`
for type 80; 40–70 and 80 chain through the root cleanup. Host and Vita
library builds pass. The patched common cleanup export confirms reusable
objects take the pool branch; other objects enter the dynamic destructor
`0x004a7d40`/`0x004a7e10`. Both paths have side effects beyond the existing
`aoe_live_unit_destroy_ror` (action collections, dependent-object callbacks,
watch nodes, and class-specific state). The durable gap is recorded in
`docs/reversing.md`. Next translate these cleanup chains and exercise the
bound player update on real campaign units and mixed object lists. The exact
pool eligibility gate is exposed by `aoe_live_unit_cleanup_uses_reuse_pool_ror`
(template `+0xa4` and type 10/20/25/30/40/50/60); it reports eligibility only.
The type-40–70 `0x00405a70` prefix drains its optional `+0x184` action list
through `aoe_live_unit_cleanup_derived_prefix_ror`. Type 80 now uses
`aoe_building_cleanup_type80_ror` for `+0x1cc` action drain, `+0x1c0` queue
release and field reset, and status-4 per-owner counter decrement; it then
uses the same inherited prefix. Both paths chain into required `common_cleanup`.
The type-80 wrapper must bind `AoeBuildingUpdate`, a matching deallocator, and
the owner counter resolver.

Latest scenario-editor work: patched RoR vtable `0x00547360` exposed the
previously missed handler `0x00493580`. Catalog builders `0x0046ab00` and
`0x00492c60` filter cloned player templates only by byte `+0x4e == 0`; the
real expansion DAT audit finds 1,764 visible and 1,816 hidden civ/template
pairs. The map-click body `0x0051a2e0` is now composed by
`aoe_scenario_editor_place_ror`: template coordinate adjustment (`+0x24`),
placement rejection (`+0x20`, including parsed template byte `+0x64`), normal
player factory (`+0x94`), special submode-1 factory, and original redraw
status branch remain explicit source-backed boundaries. Host 75/75,
ASan/UBSan focused tests, real-DAT audit, and Vita static-library build pass.
The common release template `+0x24` method `0x00443e90` is now translated as
`aoe_scenario_editor_adjust_position_ror`, including its exact map-edge
epsilon, footprint clamp, signed-16-bit truncations, optional whole-tile snap,
and type-80 forced-snap correspondence at `0x004eed30`. The normal player
factory can now bind to `aoe_scenario_runtime_editor_create_ror`, which uses
stable individually allocated live units because runtime lists retain object
addresses. The real `EXPANS_1_1.scx` audit creates and ticks a 3,810th unit
through that path under ASan/UBSan; both libraries cross-build for Vita.
Template placement/collision virtual `+0x20` (release `0x00443700`) is now
translated as `aoe_unit_placement_check_ror`, retaining return reasons
0/1/2/3/5/6/7, blocker ID, bounds, center/side terrain, terrain-restriction,
shape, mode-7 visibility bypass, clearance choice, unit intersection, and
the original template `+0x4b` overlap exception. The DAT reader now retains
`+0x4a/+0x4b/+0x54..+0x66`. `aoe_scenario_editor_runtime_*` binds adjustment,
validation, and normal creation to the owned runtime. Real scenario testing
also corrected active-player switching to propagate the SCN diplomacy row
and active `+0x6c` explored-cell list into all unit visibility bindings.
`EXPANS_1_1.scx` now finds a validator-approved location and creates/ticks
the 3,810th unit through the full editor composition under ASan/UBSan.
The patched-binary post-create branch is now revision-correct: `0x004159d0`
queries the minimum layer across the unit's sprite collection, while
`0x004a6030` returns a graphic's own `+0x29` layer or the minimum direct
delta-graphic layer from sentinel 20. Zero selects redraw status 2 and
nonzero selects status 1. The owned runtime binds this exact rule through
`aoe_scenario_editor_runtime_nonzero_layer`; the real scenario's created unit
correctly takes status 2. The supposed special mode-1 factory was also a
misidentification: the editor stores an existing live unit at view `+0x174`,
marks it state 7 while dragging, and calls its common placement virtual
`+0x34` with `(x,y,0)`. `aoe_scenario_editor_runtime_relocate` now binds that
path without allocating a second unit, and the real scenario integration
checks the constructed count remains unchanged. The runtime now also owns the
surrounding `begin_move`/`finish_move` lifecycle: only a unit belonging to the
runtime can become the selected move object, begin writes state 7 exactly as
the patched selection handler does, and finish restores state 2 and clears
the reference as mouse-up does. Host 75/75 and Vita cross-builds pass. The
known control-flow branches of patched click handler `0x0051a2e0` and its
direct unit-state move lifecycle are translated; continue through remaining
editor selection/global-mode effects and gameplay systems.

Alpha milestone requested 2026-09-26: render an actual Rise of Rome scenario
town with its original units and art on the Vita, then show real unit fighting
once original actions/damage/update behavior is wired. The user no longer
requires the alpha to be playable. A static or animated scene may be shown
as a rendering milestone only if explicitly labeled as such; do not script
fake combat or revive the disallowed diagnostic skirmish.

A separate status-art request is fulfilled by `status/port-progress.png` and
`tools/render_status_art.sh`. It composes genuine local AoE/RoR grass, Roman
town buildings, Wonder construction stage, and builder sprites. It is
explicitly labeled original-asset status art, not a Vita/game screenshot.
The present full-port estimate is approximately 10%; `status/README.md`
documents the rationale and how to regenerate the single image at a later
percentage. This does not advance or satisfy the actual scene-alpha milestone.

Durable status-art workflow (2026-09-26): edit `status/current.json` with the
full-port estimate, its rationale, and current effort, then run
`cmake --build build/host --target status_art_plan` followed by
`cmake --build build/host --target status_art -j4`. The latter builds the
decoder as needed, renders, and archives the exact recipe. The direct
`bash tools/render_status_art.sh` interface remains available for previews
and CLI overrides. Do not increase progress automatically.
Effort presets select 0/2/4/8/12 working villagers independently of progress;
an explicit `workers` override accepts 0–12. Source-verified construction
uses SLP 302 frames 0/1/2 at thirds, then Roman standing Wonder SLP 735 at
100%, with no active builders. See `status/evidence/construction-stages.md`.
Every normal render archives images, effective settings, commands, input
hashes, renderer and decoder in `status/history/`. Use
`python3 tools/replay_status_art.py status/history/SELECTED-SNAPSHOT` to
verify byte-identical reproduction without replacing the current image.
The old 960×544 v4 recipe/image are frozen in `status/recipes/v4/`; the new
fixed 960×640 layout accommodates the completed Wonder and progress footer.
Always inspect full PNG/JPEG outputs, all footprint edges and inward worker
facings. Preserve the original-asset status-art label; this is not a game
screenshot. Construction/geometry tests are in `tests/status_art_test.py`.

Latest building work: `aoe_building_update_ror` translates the field and
call order of RoR 004ae880, including byte-narrowed construction angle,
completion request, raw history shifts and post-parent state dispatch.
Its direct parent call 004b0100 now has a translated derived prelude: class-18
state-2 buildings accumulate carried amount from owner resource 35 times world
delta, clamp at 100, set carried type 34, and then delegate to the still-
required root update 004272e0 callback.
Construction-percent query 004af240 is also translated with its state/build-
time 100 shortcuts and x87-style truncation to the low signed 16-bit result.
Production query 004af1a0 is translated as a dispatcher over the
separate building `+0x184` live action list (types 102 training and 103
research), not the `+0x1cc` production list. Production now calls the
translated dispatcher directly rather than an opaque callback.
The data-return portion of 004b7d60 and its manager percentage query 004ed8ef
are now translated in `aoe_research_action`: state-1 completion, manager
states 2/3/other, signed-ID bounds, DAT research time and low-word x87
truncation are covered. Its optional localized-name copy remains excluded.
The terminal branch now directly uses translated 004af890 cleanup and the
new `aoe_action_list` push/append/pop/clear methods (00404410, 00404460,
004044e0, 00403e70). Class-specific action destruction/state methods remain
required; do not substitute no-ops. The list observes owner/head changes
during destruction and preserves intermediate activation during clear.
The active branch now directly calls `aoe_building_production_ror`
(004af900), including original resource gates, status-4 notifications and
counter ordering. Queue consumption (004af7f0) is translated and connected
to action results 1/2/3. Queue cancellation's positive-count path is also
translated from disc 004af680 (patched 1.0a 004add50), including refund,
reverse selection, shifting, status/action cleanup and miss notification.
Head cleanup preserves the original action `+0x2c` cancellation call before
the translated list clear.
Disc 004af900 and patched 004adfc0 both pass `-1` in their 102/103 drain loop
even though the callee subtracts the signed argument; this grows a positive
quantity forever. The portable loop deliberately passes `1` and now invokes
the translated cancellation routine directly for every queued unit. Only its
per-item refund and action-specific cancellation effects remain callbacks;
queue mutation is no longer delegated. Action construction/update and global
notification bodies remain required dependencies; this is not yet a working
production simulation. See `tests/building_production_test.c`.
The next type-80 dependency is revision-mapped as disc setter 004ae500 to
patched 1.0a 004acbf0. Its completion inputs are now retained instead of
skipped by the DAT parser: adjacent mode, disappear-on-build, stack unit,
old terrain, automatic research, and completion sound, alongside the already
parsed construction graphic, angle, and foundation terrain. Synthetic byte
offset assertions and real disc RoR Town Center/Wonder values validate the
16-byte serialized tail. The setter's terminal queue branch is now translated:
for new state above 2 and old state above 1 it cancels the whole head under
status 4 (after clearing/decrementing that status), otherwise quantity minus
one so a single item remains. The zero-amount case still increments revision
and emits event 124, matching both executables. The exact state-2 to state-3
branch now composes virtual `+0x258`, manager removal and
semantic `+0x1e0` clear, the translated queue transition, and the translated
root state setter in original order. Its final probe is disc `004522e0`
(patched `00451370`) with template ID and `-1.0, -1.0, 1, 2, 0`; a zero result
removes optional template research `+0x178` and IDs `0x4a` through `0x4c`.
The manager, probe, and research bodies remain explicit effect boundaries.
The requested-state-2 completion prefix is now translated separately through
the disappear/visual branch. It sets elapsed construction from signed DAT
train time, preserves the executable's RNG advance/conditional-local-signal/
reseed sequence, sets semantic live completion byte `+0x1dd`, and invokes the
class completion hook. The ordinary path calls visibility virtuals `+0xdc`
then `+0xd8`, selects standing art, narrows completed angle `+0x16e` to the
live byte, and conditionally applies adjacency; the disappear path instead
calls virtual `+0x6c`. Tests cover both and pin seed 1 becoming 41. Stack,
foundation, research, notification, and owner-finalization now form the
separate translated world continuation. It preserves optional stack spawn at
the live XYZ; foundation bounds using signed-16 truncation and the original
double `0.001` upper-edge bias; one-tile-expanded invalidation; optional
research; event `0x69` with truncated XY; and owner virtual `+0x104` with
semantic live `+0x1d8`, in that order. The common queue/state-setter tail has
now been composed with both pieces as
`aoe_live_unit_building_state_two_ror`. Requested state 2 cannot enter the
setter's `new_state > 2` queue branch, so the composition proceeds directly
to the translated root setter and synchronizes both semantic state views.
The full test pins all 12 observable effects; untranslated class/world bodies
remain mandatory callbacks, not no-ops. The owned RoR runtime later resolves
the notification specifically: application vtable `00547de8` slot `+0x40`
is `00417910`, an unconditional return, so event `0x69` is now a verified
no-op there rather than a required callback. The generic composition retains
the hook for alternate integrations.
The optional stack-unit factory also has a concrete owned-scenario binding:
`aoe_scenario_runtime_building_spawn_stack_ror` uses the completing unit's
semantic owner, cloned player template, and the recovered live constructor,
while preserving `004ae500`'s ignored factory return. The generic completion
layer retains its injection point to avoid a library dependency cycle.
Map invalidation after foundation paint is also translated and bound:
`aoe_runtime_map_invalidate_ror` implements `00445760`'s reversed-bound
normalization, clipping, cache byte `+7 = 0xcc`, and draw byte `+10` update.
The bytes use distinct host arrays, not the packed x86 tile ABI.
`aoe_runtime_map_paint_terrain_ror` now translates `00449110`'s deterministic
availability, signed-16 clipping, shape/frame admission, terrain writes,
expanded border recomputation, and internal invalidation. Its per-cell
`004481b0` hook remains mandatory because live-object removal and spawning are
not bound yet. The terrain-unit arrays are already parsed at disk offsets
`+254/+314/+374/+404`.
The `004481b0` per-cell control flow is now
`aoe_terrain_transition_cell_ror`: tile-list removal filtering, successor-safe
traversal, density chunking, placement modes, exact RNG coordinate/probability
consumption, placement probing, and creation order are translated. Dynamic
object metadata, class removal, and template placement/factory methods remain
explicit callbacks pending owned-scenario binding.
Owned-scenario object lookup and removal are now bound for supported root
classes: semantic owner/template queries validate the map-list core pointer,
and removal uses the translated `+0x68` high-state path with persistent player,
vision, registry, and world state. The real scenario test transitions a Gaia
terrain-0 type-10 decoration to state 7. `dat_tool --terrain-units` confirms
terrain 0 has 28 entries and foundation terrains 7/8/9 have no spawn entries.
Placement probing and factory creation still require owned transition binding.
Completion visibility and graphic selection are no longer callback-only.
Vtable resolution proves type-80 `+0xdc` is leave handler `004ac380` in
alternate rectangle mode 1, while `+0xd8` is wrapper `004ae980`; after
completion byte `+0x1dd` is set it enters through `004ac290` in normal radius
mode 0. `aoe_live_unit_building_completion_visual_runtime_ror` binds those
calls to translated vision state and binds `+0x38` to DAT-resolved owned
sprite selection. Construction also retains the resolved walking graphic for
later selector rules. Only the genuinely class/world-specific visual effects
remain callbacks. `aoe_live_unit_building_state_two_runtime_ror` composes this
adapter with the world continuation and root setter as the preferred owned
runtime entry point.
Incomplete type-80 factory visibility is now consistent with that completion
transition. A mode-aware placement binding registers the constructor's
alternate size rectangle, and destruction removes the rectangle until
semantic completion byte `+0x1dd` is set; completed buildings use the normal
radius footprint. This prevents completion from leaving a footprint different
from the one registered by the portable factory. The common placement API
continues to select mode zero.
`aoe_live_unit_bind_building_update_ror` now initializes and attaches
caller-owned type-80 building/action-list storage from the recovered
constructor defaults. It preserves distinct live `+0x184` and production
`+0x1cc` lists with mandatory action callbacks, queue enabled 1/status -1,
template build time/angle information, owner resource semantics, and live
completion value `+0x1d8`. This replaces hand-built completion test state;
allocation and lifetime stay explicit until scenario runtime owns these
objects directly.
Type-80 high state is no longer an opaque scenario callback. Disc 004ae450
(patched 004acb40) now composes its required building prelude, translated
derived action-list clear and root high transition, building-manager removal,
and semantic `+0x1e0` clear in original order. The prelude and manager body
remain exact effect boundaries; the surrounding control flow is translated.
The production-list virtual update is now verified through its factory and
vtable and translated as `aoe_action_list_update_ror` (004043c0). Production
calls it directly. Preserve its automatic result-1 pop followed by the
caller's additional pop, and empty-list result 1 even after allocation
failure. Its generic callbacks now have a translated training-action head
and waiting implementation; other action classes remain dependencies.
Training action head update 004b69a0 is now translated as
`aoe_train_action_update_ror`: work-rate/delta accumulation, fast-mode 1000,
state-1 completion and the original failure/success notification sequence.
See `docs/TRAIN_ACTION.md` for vtable evidence and pending placement/create,
target-setter, constructor and destructor methods. It is not yet
installed as the production factory; never supply invented spawn units.
Activation 004b68c0 is also translated, including prepaid state-6 entry and
the distinct invalid-template/payment-failure paths. Refund 004f3b90 now
uses the three real DAT costs and owner resources directly. Charging policy,
spawn search and outer production-list factory binding remain.
Inherited waiting update 004073d0 is now translated: it clears terminal
primary/secondary targets through the translated setters and observes
primary-detach changes before rereading secondary. Target setters
00407460/004074d0 now preserve cached-ID lookup, detach, pointer reread,
assignment and attach ordering. World lookup 005230d0 is now translated as
`aoe_live_unit_registry_lookup`, including its separate positive and negative
ID arrays; target reference methods remain integration boundaries. Portable
create/destroy now initialize the
recovered fields and owned action list, then detach both targets and clear/free
the list in original teardown order. Spawn search 004b6b40 is now translated
and called directly from training completion: three expanding 60-probe rings,
original RNG reduction/sampling, collision test, then accepted-position unit
creation. Collision/create implementations and outer factory/tick binding
remain; no invented spawn location or unit is used.
The action now has an adapter binding it to the translated building production
queue/list interfaces. `prepare` supplies real runtime bindings; the adapter
uses the production constructor values `(unit_id, -1, prepaid=1)` and exposes
the existing list activation/tick/destroy callbacks. The new integration test
drives production through create, prepaid activation, progress, real RNG spawn
callback, completion, destruction and queue consumption. Runtime world
bindings (templates, collision/create, event dispatch) remain explicit;
no live game training is claimed. Latest verification: host 62/62, focused
building/train bridge ASan/UBSan test, and VitaSDK adapter library build.
Training vtable cancellation `004b6d90` is now also translated as
`aoe_train_action_cancel_ror`: state-1 transition, conditional unpaid refund,
then event 103 and building signal 505/619. It deliberately remains distinct
from destructor teardown. Focused cancellation tests pass on host and
ASan/UBSan, and the train-action Vita library cross-builds.
The adjacent training-action query `004b6e00` is translated as
`aoe_train_action_progress_ror`: it returns the queued unit ID and either
literal 100 for state 1 or the executable's x87 truncate-toward-zero percent.
Its unrelated `00528540` text-buffer branch remains explicitly unported.
Host 64/64, focused ASan/UBSan, and Vita library verification pass.
Training vtable serialization `004b6870` is now represented by
`aoe_train_action_serialize_ror`: inherited write first, then unit ID,
progress, request ID, and prepaid flag in the executable's 2/4/4/1-byte
order. Full stream implementation/deserialization remains unported. Focused
host and ASan/UBSan tests plus Vita library build pass.
Base-game training action correspondence is now source-checked: constructor
`004a50c0`, serializer `004a5190`, activation `004a51e0`, update `004a52c0`,
spawn `004a5450`, cancellation `004a5690`, and progress `004a5700`. Base is
not simply RoR without expansion data: it has no RoR prepaid byte `+0x4c`,
always charges on activation, always refunds a cancelled active action, and
serializes 2/4/4 rather than RoR's 2/4/4/1 fields. Public `*_base` entry
points preserve that distinction, including `aoe_train_action_create_base`
clearing the portable RoR-only prepaid field before allocation. Base's
signed-division RNG reduction is
exhaustively equivalent to RoR's reciprocal multiply for all 32,768 legal
random values and both attempt bands. `train_action_adapter` now selects the
base constructor, activation, and update methods when configured for base,
instead of applying the RoR prepaid semantics at the production-bridge
boundary. The base adapter assertion, focused ASan/UBSan tests, full 66-test
host suite, and Vita static-library cross-build pass. The original base
building-production routine has not yet been recovered; this adapter binding
must not be treated as proof of full base queue fidelity.
See `docs/BUILDING_UPDATE.md` for original addresses, supported numeric
domain, precision limitations and pending callees. It is not connected to
live runtime buildings; no construction gameplay is claimed.

Latest rendering work: all nine Rise of Rome live-unit vtables share draw
slot `+0x0c = 0x004a8a60`. `aoe_live_unit_draw_ror` ports its tile/state
gate, type-25 player mask and substituted draw ID, and overlay-before-sprite
order using explicit callbacks for effects still awaiting viewport binding.
It now consumes the renderer and projected tile anchor forwarded by
`0x00451fd0`, passes the unchanged anchor to the overlay, and adds signed live
fields `+0x28..+0x2e` to produce the sprite-manager's four bounds exactly. The
additions now preserve the executable's low-word wrapping behavior; live frame
byte `+0x35` and the graphic selected by tile traversal are forwarded unchanged.
The tile renderer already has original class filtering, but real unit sprite
managers are now joined to that filtering by `aoe_live_unit_viewport_draw_ror`:
it consumes persistent `AoeLiveUnit` objects, retains the selected graphic and
projected coordinates, and resolves per-owner/screen fields explicitly. The
indexed SLP backend is not yet installed behind its sprite callback, and the
placement-offset formula is now invoked by placement and the persistent
constructor using the parsed DAT map geometry. The real `EXPANS_1_0.scx` audit
computed offsets for all 14,923 unit records. No scene, Vita screen, or combat
is claimed.

Latest unit-constructor work: `aoe_unit_graphic` translates base
`0x004161a0` / Rise of Rome `0x004ab6b0`, including the expansion-only
walking-graphic guard and original sprite-manager call order. Tests exercise
both editions. `aoe_sprite_list` now translates the add/remove/clear list
methods called by that rule, including priority ordering and duplicate
references. Neither module is yet bound to live units or a Vita renderer.
`aoe_sprite_animation` now ports the original animated-instance frame update
(RoR `0x00416540`, base `0x004e8ed0`), with loop, delay, and stop-state tests.
It still needs real graphic instances and drawing callbacks.
`aoe_sprite_instance` now constructs static/animated instances from the DAT
records and uses the original RNG for randomized starts. The installed base
and expansion DATs instantiate all 845/941 present graphics; base `Space
Ship` graphic 834 needs a documented safe fallback because its randomized
one-frame record has zero duration. Live Vita frame rendering remains unbound.
`aoe_sprite_runtime` now owns DAT-backed sprite instances in the translated
priority list and ticks them in list order. `aoe_unit_sprite_binding` joins
the original unit graphic-selection rule to this runtime. It pre-acquires a
replacement to make allocation failure non-destructive, unlike the original
remove-then-add order. These are still library components, not a live unit
constructor or Vita renderer.
`aoe_graphic_frame` now selects the original SLP frame and mirror bit from
the graphic angle plus sprite animation state (RoR `0x004a6d10`, base
`0x004b2e60`). Real DAT sweeps pass all 3,974 base and 4,515 expansion
nonblank angle selections. The SLP bounds sweep below found some DAT/SLP
mismatches; complete pixel effects and live drawing still need verification.
`aoe_graphic_art` now performs original-order SHP/SLP DRS lookup and direct
SLP frame decode. Installed-file sweeps load 722 base and 794 expansion
graphics' first frames, with 83/105 lacking direct art and no malformed
sampled frames. Full angle/highest-frame bounds checks found 91/92 DAT/SLP
range mismatches; no frame substitution has been made. An optional bounded
decoded-frame cache is available, but SLP pixels are not yet rendered in a
live game.
`slp_blit_graphic_rgba` now composites normal palette pixels with the
original hotspot and horizontal-mirror placement (RoR `0x004a1460` /
`0x004a2570`). Player-color, shadow, and outline pixels use an effect
callback; the player-color-only variant now supplies the original OR
transform. Shadow and outline effects remain untranslated. The surface
is RGBA software memory, not a Vita display or gameplay renderer.
`aoe_sprite_draw` now joins actual DAT sprite instances, frame selection,
DRS/SLP decode, and hotspot blit in the original priority-list order.
Static sprites add stored X/Y offsets; animated sprites do not. The real
base/expansion `Arrow Fire` integration check draws 126 palette pixels using
palette resource 50500. Cached and uncached draws match byte-for-byte on
that graphic in both editions, including a cache-limit fallback. It does
not resolve shadows, outlines, unit/world ordering, or Vita display output.
The original player-color effect is now ported for ordinary/player-only
frames: RoR `0x004a6dd0` selects the DAT graphic's fixed color or caller
color; `0x00545760` sets `(color + 1) * 16`, and `0x005459a0` ORs that bank
with each SLP player pixel's source index. The base/expansion `Arrow B fire`
graphic 1 test checks its 17 player pixels. Shadow opcode 0xb instead uses a
destination-index lookup table, and outlines remain unresolved; the
player-only draw rejects those frames.
An indexed sprite surface now preserves the destination palette indices that
the original shadow opcode 0xb transforms through a 256-byte table. Real
base/expansion `Cliffs s2` graphic 118 has 82 shadow pixels and uses fixed
DAT color 10 (`Shadow.col`). Base `Interfac.drs` `anib` ID 50909 is
byte-for-byte identical to installed `data/shadow.col` (256 decimal indices),
and `aoe_shadow_table_load` now parses it; the real-asset draw test uses this
actual table. Other caller-selected color tables and outlines still need
their original dispatch/data verified. RoR `0x004a6dd0` passes the selected
table pointer to `0x005457a0`.
`palette_expand_indexed_rgba` now converts that indexed result through the
original JASC palette for display; the real shadow test checks one converted
pixel. It is still only a software buffer, not a Vita display submission.
An audit of base `Interfac.drs` color tables 50900–50911 found 50909 to be
the broad shadow remap, while most others change only ten indices; the
runtime DAT-color-to-resource mapping remains unverified.
The direct-art first-frame sweep reports player-color effects on 303
base/361 expansion graphics, shadows on 58/60, and no outline markers in
the 722/794 loadable first frames. The all-frame sweep decoded 681 unique
base SLPs/12,625 frames and 752 expansion SLPs/15,127 frames: zero malformed
or outline frames, 8,750/10,992 player frames, 386/392 shadow frames. RoR
outline globals `0x007dc058`/`0x007dc05c` are initialized to zero and only
direct reads were located; indirect writes have not been excluded. With all
registered DRS archives, 46 base/42 expansion definitions have negative SLP
IDs and 37/63 positive IDs remain unresolved. No missing art is fabricated.
The original sprite-list draw dispatch continues after a missing sprite;
the portable RGBA/indexed runtime loops now do too, reporting the first
failure after drawing later sprites.
`terrain_blit_draw_plan_passes_indexed` now retains original 8-bit direct
terrain/border pixels through the recovered clip and dither passes. Its
expanded colors match the RGBA route in unit tests, and real base/RoR map
audits check every distinct encountered terrain frame sequence. This is
needed for sprite shadow remaps over terrain, but the complete viewport path
now also has `terrain_draw_view_visibility_indexed`, sharing the original
visibility/tile-order path with RGBA. Synthetic, base-map, and expansion-map
audits compare painted colors and tile counts; hidden pixels stay untouched.
It does not yet receive live visibility state or compose units. Host tests,
a sanitizer Rise of Rome map audit, and VitaSDK static-library cross-build
pass.
Original per-tile object traversal is now translated as
`aoe_tile_object_visit_draw` (RoR `0x00451fd0`) and its base variant
(`0x0049ccd0`). Both viewport loops call that traversal after the tile
terrain draw. Rise of Rome adds alternate-pass class-2 and class-4 filters;
the base routine has only class-0/class-3 cases. The functions preserve the
existing tile-list order and dispatch selected graphic pointers through a
caller callback; they do not create live objects or draw their sprites.
The nearby `0x00452230` sort is for selection paths, not viewport drawing.
The synthetic branch tests, sanitizer run, and VitaSDK library build pass.
`aoe_scenario_spawn_one` now ports the scenario placement dispatch from
RoR world `0x00522be0`/player `0x0045d1c0` and base world
`0x00505da0`/player `0x00516af0`. The player method consumes six stack
arguments despite Ghidra's incomplete prototype: X/Y/Z, 16-bit object ID,
state byte, rotation. The world sets spawn ID `+0x70`, raises byte `+0x74`,
calls player `+0x60`, and clears that byte. The player chooses a DAT unit
factory and then state category 0–2, 3–6, or 7+. Portable callbacks mark
the still-unported factory and unit methods; no live unit is allocated yet.
All 256 state values, missing-definition behavior, sanitizer build/run, and
VitaSDK static-library build passed. At that stage the host suite had 38
passing tests.
The original DAT civilization-template dispatch is now mapped as
`aoe_unit_template_factory_spec`. The common switch (RoR `0x004502e0`, base
`0x004acf20`) handles types 10, 20, 25, 30, 40, 50, 60. The type-1 derived
civilization override (RoR `0x004f1890`, base `0x0052a0e0`) adds 70, 80,
90. All 17 expansion and 13 base civilizations are type 1, and a full audit
of every present unit definition in both installed DATs found no unhandled
types. The prior conclusion that type-70 animals and type-90 trees lacked a
factory branch was wrong: it overlooked the derived override. Player setup
clones these civilization templates, then scenario placement invokes each
template's live-unit factory. Those constructors, clone methods, and
live-unit methods remain unported, so the type audit does not imply playable
units. `dat_tool FILE --factory-audit` fails on unhandled present definitions;
the host suite runs this against both installed DATs when available.
`unit_refs_tool` still reports only DAT definition resolution.
At that stage, 41/41 host tests, the factory test under ASan, and
the VitaSDK static-library build passed. Next, reconstruct the ten original
template constructors' field initialization, then the per-template clone
(`+0x1c`) and live-unit factory (`+0x18`) methods; selection metadata alone
cannot instantiate scenario objects.
The factory metadata also records each type's original live allocation size
in both editions. Base and Rise of Rome live layouts differ for types 30–80;
do not treat either original size as a portable C struct size.
`src/player_templates.c` translates the player constructor's slot-array clone
for all fields currently exposed by `DatUnit`, with independent owned copies
of names, tasks, attacks, and armors. The real-DAT factory audit now clones
all 17 Rise of Rome and 13 base civilizations and verifies slot/ID mapping.
This is not the complete original per-type clone: unparsed template fields
and live-unit methods still need reconstruction. The next path remains
implementing those fields and the template vtable `+0x18` live factories.
Current verification: 42/42 host tests; the parsed-field clone test and both
real-DAT audits also pass under ASan; the player-template library builds with
VitaSDK.
The next source-backed link is now `DatUnit.negative_id_mode`: RoR common live
initializer `0x004a8200` reads original template byte `+0xa4` and calls
positive world allocator `0x00523120` when zero, negative allocator
`0x00523160` otherwise. `src/live_unit_id.c` ports that dispatch via the
already-translated registry cursors. Type-90 trees reach this initializer via
`0x004ef700` → `0x004b2550` → `0x004a7c80`. This is ID allocation only;
graphic selection, map placement, world registration, and the rest of live
unit construction remain. The installed Rise of Rome Gaia/Egyptian DAT
definitions have 73/69 nonzero `+0xa4` flags (base 63/59).
Latest verification: 43/43 host tests; the DAT, ID-selection, and both
real-DAT audit tests pass under ASan; `aoe_dat` and `aoe_live_unit_id` build
with VitaSDK.
`src/live_unit_core.c` now translates the common live initializer's direct
template/ID, HP/flags, state/class, X/Y/Z, and storage/resource effects for
both editions, composing the previously translated ID and storage routines.
It intentionally stops before the original virtual calls for sprite/list
objects, standing graphic, placement, and owner/world registration. The
portable state is semantic, not the PC object ABI.
Current verification: 44/44 host tests; the new live-core test passes under
ASan; `aoe_live_unit_core` builds with VitaSDK. The next required work is the
original virtual graphic/placement/registration chain and complete live
allocation, not a substitute renderer or diagnostic unit.
The common unit placement method's 17 terrain-shape height formulas are now
in `terrain_shape_surface_offset`, and `terrain_surface_height_at` samples a
scenario tile after original-style X/Y clamping. `DatUnit.elevation_mode`
reads original template byte `+0x68`; `terrain_unit_surface_z` and
`aoe_live_unit_core_apply_surface` implement mode 0 snap-to-ground and mode 1
raise-to-ground. RoR `0x004ab160` and base `0x00415bf0` agree on this
calculation. The two installed DATs contain only modes 0/1 among present
definitions. This does not yet link objects into terrain tile lists or run
the placement method's other callbacks.
`tests/live_unit_surface_real.c` now walks `EXPANS_1_0.scx` through real DAT
lookup, player-template clone, common live core, the original scenario
reserved-ID mode, and terrain Z adjustment. It checked 14,923 placements,
124 Z adjustments, and 25 negative reserved IDs. This is a real-data
coverage/bounds audit, not proof of tile orientation or playable units.
Current verification: 45/45 host tests; the real scenario audit and terrain,
DAT, and live-core tests pass under ASan; `aoe_terrain`, `aoe_dat`, and
`aoe_live_unit_core` build with VitaSDK. The next original placement work is
tile-list detach/attach and the remaining map/owner callbacks, followed by
actual live-object allocation and registration.

Rise of Rome `0x004ab160` now has a separate portable placement path in
`src/live_unit_placement.c`. It clamps X/Y, chooses the runtime tile,
prepends the unit to that tile's original-style object list, removes its old
node, updates coordinates and terrain-adjusted Z, and preserves the state-2
leave/enter callback order (including no callbacks on same-tile moves). It
honors the original early return for non-null unit `+0x20`. Unlike the PC
binary, it allocates the new list node before detaching the old one so an
allocation failure does not orphan the unit; completed relinks and callback
observations match. The method requires real leave/enter handlers for state-2
tile changes, so it is not yet a complete gameplay placement path. It also
requires terrain tiles in runtime map order; scenario X/Y orientation remains
to be traced. `tests/live_unit_placement_test.c` covers the list and callback
transitions. Current verification: 46/46 host tests, new test under
ASan/UBSan, and the new library cross-builds with VitaSDK. Next: reconstruct
the original state-2 leave/enter handler effects and connect this method to
the actual live factory and map.

The state-2 handlers `0x004ac290` (enter) and `0x004ac380` (leave) feed the
player visibility map. Their common radius-table branch calls
`0x0051a2b0`/`0x0051a490`; `src/visibility_footprint.c` now ports that
branch's exact signed-byte count transitions, explored/visible mask changes,
active-player tile view bytes, and explored-cell recording callbacks. Its
radius-0–16 half-width rows were extracted from the executable's
`PTR_DAT_00562790` table (original data at `0x00560b60` onward); it clamps
radii above 16 as the original does. The outer unit hook skips nonpositive
line-of-sight, truncates X/Y and line-of-sight with the original `__ftol`,
and derives the active-player relation flag from player/world state;
those bindings and the alternate rectangle branch are still unported.
`tests/visibility_footprint_test.c` covers overlap, fog transition,
exploration, clipping, the -7 sentinel, and suspended state. Current
verification: 47/47 host tests, this test under ASan/UBSan, and
`aoe_visibility_footprint` cross-builds with VitaSDK. Next: bind the original
player/map state to the placement callbacks without fabricating owner or
visibility flags, then continue the live-unit factory chain.

`src/live_unit_vision.c` now binds the RoR placement callback pair to the
common visibility path. It takes original template `+0x28` line-of-sight,
truncates it and old/new X/Y with `__ftol` semantics, skips vision changes
for nonpositive radius, selects active-player tile-view bytes only when
world active index `+0x7c` equals owner player `+0x4a`, and derives a second
active-player explored-cell append from that player's `+0xb0` relation entry for
that owner. The runtime values are explicit inputs; this does not invent
player or diplomacy state. A tile change now composes list relink with
leave-old and enter-new visibility effects, while a same-tile move invokes
neither callback. `tests/live_unit_vision_test.c` checks moving-unit overlap,
discoveries, relation-dependent double records, and preflight failure
without mutation. Current verification: 48/48 host tests, new test under
ASan/UBSan, and `aoe_live_unit_vision` cross-builds with VitaSDK. Remaining:
instantiate original player/world visibility grids and explored-cell vectors,
resolve scenario/runtime tile orientation, then invoke this path from the
actual player live-unit factory and registration chain.

Further disassembly of `0x0051a2b0` corrected a fidelity error: each newly
discovered cell always appends to owner player `+0x58`; when the active
player's relation to the owner is 1, it also appends to the active player's
separate `+0x6c` vector. It does not append twice to one vector. The
visibility-footprint API and tests now have distinct destinations.
`src/player_vision_state.c` translates the shared zeroed mask-grid
constructor `0x0051a890`, player visibility constructor `0x00519e10`
(all count bytes `0xff`, owner visible/explored masks), and the two vector
constructors `0x0045fb20` at player `+0x58/+0x6c`. It also ports vector
append `0x0045fa50` with 400 initial entries and doubling growth. The
owned-state test passes this real layout through the composed placement and
visibility path, confirming separate vectors and masks. Current verification:
49/49 host tests, the three visibility/placement tests under ASan/UBSan, and
both new libraries cross-build with VitaSDK. The vectors retain an explicit
allocation-failure flag for the caller to check before simulation continues.
Next: connect this owned state to the original player constructor/world map
and live-unit factory; verify scenario X/Y orientation before scenario
spawns are treated as gameplay.

Scenario tile order is now resolved for the installed square maps. Expansion
reader `0x00445aa0` and base `0x00424cb0` read triples with outer Y and
inner X into runtime `row[y][x]`; expansion allocator `0x004453c0` stores
rows at width stride. Both readers use height for the inner bound, so
rectangular maps are not claimed. `scn_runtime_tile_order_supported` and
`scn_runtime_tile_at` make that boundary explicit, and the real placement
surface audit now requires it. The 24 base scenarios are 120² (2), 144²
(7), or 200² (15); the 19 expansion scenarios are 144² (8), 200² (10), or
250² (1). Verification: 50/50 host tests, the new mapping test and real
14,923-placement audit under ASan/UBSan, and `aoe_scn` cross-builds with
VitaSDK. Next: build the original runtime map tile structure and connect
real scenario placements to the live factory/registration chain.

Current reconstruction: portable DRS/SLP/palette readers, `Empires.dat`
decompression plus terrain restriction/color/sound/graphic/terrain tables,
and a portable `1.11` scenario header/decompressor/map/unit reader. `scn_tool`
follows the original `1.15` player-data layout in all 24 installed scenarios and
rendered a terrain-color PPM inspection image. Its terrain palette selection
now follows the original shape-color dispatch in both executables; it is not
the complete minimap renderer. The four original terrain SLP
resources (`15000`–`15003`) are in `Terrain.drs`; basic frames are 65×33
isometric diamonds. No original terrain renderer is implemented yet.

Expansion source: `/mnt/e/Downloads/Age of Empires - The Rise of Rome
Expansion (USA) (Rev 1)/`. Its cue sheet identifies one MODE1/2352 data
track and 14 audio tracks. `tools/cdtrack_tool.c` converted Track 01 into
ignored `build/ror/track01.iso`; selected files are under `build/ror/disc/`.
The expansion executable is `GAME/EMPIRESX.EXE` (SHA-256
`8e0e6d047b2f90d0769fb5a1b16c1b1973582efdbe73a5d11395a43557215845`).
Its `GAME/DATA2/EMPIRES.DAT` (SHA-256
`d47c59d9ecaf5b83b467c647a69d28eced6a0dc5b2fa873e930c86136df5eb9d`)
decompresses to 1,415,153 bytes of `VER 3.7` data. The current parser
successfully reads 234 sound records, 941 graphic slots, 23 enabled terrain
slots, seven border slots, and unchanged 64×32 map geometry. Expansion
`DATA2` contains three DRS overlays (graphics, interface, sounds); the
base `Terrain.drs` and `Border.drs` remain necessary. Four `.CPX`
campaign packages contain 19 embedded `.scx` scenarios; `aoe_cpx` extracts
all of them under ignored `build/ror/scenarios/`. Their 1.15 bodies contain
628×278 8-bit briefing DIBs. `scn_load_map` preserves these and parses all
19 through the map and player-detail records. The expansion Ghidra
project is `build/ghidra/AoERoR`; never reuse base executable addresses
without re-verification.
`Empiresx.exe` at `0x00500b30` registers expansion sounds/graphics/interface
archives before the five base archives; `0x0046c190` appends them to a
linked list. `0x0046c450` searches it in append order and returns the first
matching type/ID. `drs_find_first` now matches that order; a host test uses
real overlapping and base-only graphics IDs. Expansion graphics overlaps
82 base resource IDs.

Next work:

`dat_tool FILE OFFSET LENGTH` now gives a bounds-checked hexadecimal view of
the decompressed DAT for locating later tables. `dat_parse_random_maps`
now follows the 48-byte tail of the terrain block and both passes over the
original random-map records. The expansion has ten maps, ending at DAT offset
`0x2598c`; the base has five, ending at `0x20ecc`. `dat_parse_effects` then
reads 218 expansion or 140 base technology effects and their 11-byte command
records. The next offsets are `0x29e92` and `0x23f48` respectively; each
begins with a 16-bit civilization count (17 expansion, 13 base), followed by
the `Gaia` civilization. `src/dat_units.c` now traverses all 17/13
civilizations and every present variable-length unit record through the next
technology section (`0x1576c3` expansion, `0xe9517` base). It exposes each
unit's ID, type, class, name, core graphics, HP, sight, size, and speed; it
also exposes 59-byte task records, attack/armor class amounts, key combat
fields, three resource-storage and cost entries, and training fields.
Missile and building-specific fields beyond these remain skipped. No original
combat/task execution has been ported.
Gaia has 332/400 present expansion slots or 299/374 base slots; each other
civilization has 203/400 or 175/374. Every present unit ID equals its slot.
`unit_refs_tool` resolves every placed unit against the correct civilization
in all 19 expansion, 24 base, and one demo scenarios: zero missing
definitions. Synthetic truncation/count tests, all 44 real-file sanitizer
runs, and the Vita static-library cross-build pass.
`src/dat_tech.c` now reads the final 127 expansion or 118 base technology
records, ending exactly at decompressed EOF (`0x1597f1` expansion, `0xeb3fc`
base). It exposes four prerequisite slots, three resource costs, research
time/location, icon/button/language IDs, and the linked effect ID. All
nonnegative prerequisites, effects, and Gaia research-location units resolve
in both files. This is static progression data; applying effects and gating
research still require original-executable behavior. Host, sanitizer, and
VitaSDK builds pass with the new parser.

Primary next step: feed the indexed viewport with actual original runtime
visibility/tile flags, connect the recovered per-tile object traversal, and
compose original unit sprites on that surface. Recover the DAT-to-live-template
path (including types 70/80/90) and state methods reached by scenario
placement, then use decoded unit/task/combat
definitions to reconstruct simulation and UI.
The base addresses below are useful
for comparison, not substitutes for expansion-address verification.
Expansion `0x004493a0`, `0x004496b0`/`0x00449b10`, and `0x00445880` now
verify the portable elevation-shape, border, and tile-projection behavior
against `Empiresx.exe` itself. All 19 expansion campaign maps pass border
and projection calculation with the expansion DAT. The full 44-map
base/demo/expansion corpus passes the sanitizer run. Direct terrain-frame
selection is now implemented; full drawing remains missing. A direct
reference query for the
projection routine `0x00445880` found calls at `0x004455c3`, `0x0044574d`,
`0x0044586e`, and `0x00448fe8`; these are starting points for caller
inspection, not evidence of the draw path. Decompilation places the first
two calls in map allocation (`0x004453c0`) and reset (`0x004456d0`). The
`0x00448fe8` call is in elevation-region update `0x00448cf0`: after writing
shape byte `+4` and projecting the affected tiles, it calls `0x00445760`.
Disassembly of `0x00445760` shows the map's 24-byte tile records being marked
with byte `+7 = 0xcc` and byte `+10 =` its fifth argument. The next useful
trace is the consumer of those tile marks; no sprite-frame rule follows from
these map-update functions alone.

1. Continue interpreting original scenario gameplay semantics and the
   executable's renderer/simulation. The unique-map scan has been replaced
   with a direct `1.15` player-data traversal. Embedded blobs in the optional
   `0x004a33a0` record are now parsed for the 19 expansion campaign maps;
   other body versions still fail closed. The
   version-check path at `0x004d45c0` dispatches through manager vtable
   offset `0x34` to `0x004d4730`, but this only reads header metadata via
   `0x0045db40`. `0x004d8f50` queries a field at metadata offset `0x0c`.
   A separate version dispatch at `0x00504420` sends `1.11` to the object's
   vtable offset `0x98`. Both identified vtables (`0x0054d938`, `0x00551e48`)
   point that slot to `0x00505da0`; its pre-map callees have been traced for
   body version `1.15`. `0x005041d0` is a related named-file path.
   The `+0x70` method is `0x005046f0`, delegating to map reader `0x00424cb0`:
   it reads two 32-bit dimensions and three bytes per tile. In `0x00505da0`,
   the `+0xf4` method runs immediately before the map method, and player/object
   loops follow it. The two `+0xf4` targets are `0x00462900` (calls
   `0x0051dd50`) and `0x00501f60` (calls `0x0045ddf0`); `0x0051dd50` also
   calls `0x0045ddf0` first. The common reader then calls nested readers
   `0x005126d0` and conditionally `0x004a33a0`. The direct traversal was
   checked against all 24 scenario map offsets. The pre-map player names and
   setup records are now exposed; enabled flags match the header player count
   in every installed scenario. After each map, eight resource
   sets and nine player-unit groups are now read from the original 23-byte
   unit records. The final eight player-detail records are now parsed through
   the end of the body, including names, camera coordinates, diplomacy, and
   victory conditions. All 24 installed scenarios and the Vita cross-build
   pass, but none of these fields drive a playable simulation yet. The demo
   `Multip_1.scn` from `E:\msaoe.exe` is outer version `1.10` with the same
   `1.15` body; it too parses through the end. The extracted copy lives in
   ignored `build/demo/Multip_1.scn`.
2. Continue the original terrain renderer. `src/terrain.c` now reproduces
   `0x00428970`'s elevation-neighbor shape byte (0–16) and `0x00428ce0`'s
   border byte; `dat_parse_borders` reads the seven enabled border slots.
   `dat_parse_map_geometry` reads the original 64×32 tile dimensions and
   `terrain_project_tile` follows `0x00424a60`'s screen-origin calculation,
   including the extra height shift for seven shape codes. Projection tests,
   all 25 installed/demo scenario maps, and the Vita cross-build pass.
   Both decisions were run across all 25 installed/demo maps. The DAT shape
   index maps to SLP frames 9–24 for grass/desert and 4–19 for water/dark
   water. Direct terrain/border frame selection and one-step replacement
   are now traced below; the next renderer work is actual sprite placement,
   palette/fog effects, and object layering.
   The terrain-only minimap color branch has also been traced in both games:
   expansion `0x0042d610` / jump table `0x0042d93c`, base `0x00456d03` /
   table `0x00457090`. `terrain_minimap_palette_index` matches all 17 shape
   branches, and `scn_tool` uses it for inspection PPMs. The minimap rule
   does not supply viewport art, fog, or object rendering.
   The third 16-bit field in each DAT terrain frame triple is the first SLP
   frame, not a shape ID. `terrain_shape_frame_span` now returns its DAT-declared
   range for shapes 0–16. Grass/desert ranges cover SLP frames 0–24 and
   water/dark-water 0–19. Expansion `0x00517790` and base `0x004bbbd0`
   select the actual direct SLP frame: a 3×3 flat grid for grass/desert,
   2×2 for water/dark water, and the DAT first frame for single-frame
   shapes. `terrain_select_frame` implements this and has real-DAT queries
   via `dat_tool FILE --terrain-frame TERRAIN SHAPE X Y`.
   `FindAoEFrameUsers.java` on the analyzed expansion binary yielded
   `0x004cb120`, `0x004d5870`, and `0x004e5480`; the first two are large
   non-renderer simulation/effect paths and are not the selectors. The
   actual tile-draw caller is expansion `0x00516710`, which invokes terrain
   selector `0x00517790` and border selector `0x00517880`. Base border
   selector is `0x004bbcc0`. Portable `terrain_select_border_frame` follows
   the original flattened DAT indexing and style-specific X/Y modulo rules;
   both DATs select frame 4 for slot 6, shape 0, pattern 13. Use
   `dat_tool FILE --border-frame BORDER SHAPE PATTERN X Y` to audit. A zero
   pattern is rejected because it indexes header bytes rather than a frame
   triple; its draw-path handling remains to be verified.
   `border_art_load_direct` now loads six real SLPs from base `Border.drs`
   (four 36-frame and two 68-frame resources) for both DATs, validating all
   nonempty selected shape/pattern spans. Enabled slot 1 references missing
   SLP 20005 and is left without drawable art, matching the original loader's
   failure behavior. All six SLPs use palette-color/transparent pixels only.
   `dat_tool FILE --border-art BORDER.DRS` audits the cache. Original border
   placement/compositing and replacement-terrain resolution still need work.
   `Empiresx.exe` `0x00449110` and base `0x00428700` also confirm the
   original terrain region-paint admission gate. The portable
   `terrain_paint_target_available` accepts an enabled terrain directly or
   a disabled one whose immediate replacement is enabled. This is map-edit
   behavior, not evidence of sprite-frame selection.
   The four original terrain SLPs contain 90 frames total; every decoded
   pixel is either opaque palette color or transparent. The new
   `slp_blit_terrain_rgba` provides clipped, palette-correct compositing for
   that subset and rejects other SLP pixel kinds. It is a renderer primitive,
   not a viewport or a diagnostic game prototype.
   Expansion `0x00447900` is the original direct terrain-art loading pass:
   it walks 32 records and associates each enabled record's SLP ID with a
   sprite object. Portable `terrain_art_load_direct` now loads and validates
   those SLPs with archive precedence. Base and expansion DATs both produce
   four direct entries (slots 0/1/6/22, 25/20/25/20 decoded frames), including
   under the sanitizer. The viewport caller now confirms single-step
   replacement (`terrain_resolve_draw_id`): beach 2→desert 6 and forest
   10→grass 0 in both DATs. `terrain_build_draw_plan` reproduces the
   original base/style-0/style-1 border layer order, with synthetic tests.
   `terrain_blit_draw_plan_rgba` now composites those layers at a supplied
   screen anchor, subtracting each frame's original SLP hotspot as
   `Empiresx.exe` `0x004a1460` does. It prevalidates every layer; host,
   sanitizer, and Vita static-library builds pass. This is still not a
   complete viewport, fog, object layer, or full terrain semantic model.
   Expansion viewport `0x00514c10` and base `0x004b8e80` subtract renderer
   pixel camera fields `+0x130/+0x134` from each projected tile origin;
   `terrain_project_tile_view` exposes that transform. Both callers sweep
   alternating diagonals from signed start-tile fields `+0x12a/+0x12c`;
   `terrain_walk_view_tiles` reproduces their map-bound visit order. These
   helpers have host/sanitizer tests and Vita static-library cross-builds,
   but camera state derivation and final viewport drawing remain open.
   `terrain_draw_real` walks original scenario tiles and blits every distinct
   resolved frame sequence once at a fixed test anchor. All 19 expansion,
   24 installed base, and one demo scenarios pass under sanitizers with zero
   empty tile plans. Example:

   ```sh
   build/asan/terrain_draw_real build/ror/scenarios/EXPANS_1_1.scx \
     build/ror/disc/GAME/DATA2/EMPIRES.DAT \
     '/mnt/d/Age of Empires/data/Terrain.drs' \
     '/mnt/d/Age of Empires/data/Border.drs' \
     '/mnt/d/Age of Empires/data/Interfac.drs'
   ```

   Add `'/mnt/d/Age of Empires/data/TILEEDGE.DAT'` as the final argument to
   audit the visibility-aware viewport with a mixed synthetic visibility
   grid and original edge streams.

   This is compatibility coverage for the terrain/border path, not visual
   comparison of a complete game screen.
   Use `dat_tool FILE --terrain-art TERRAIN.DRS` and
   `dat_tool FILE --draw-terrain ID` to audit the inputs.
   Expansion `0x004a0b80` tries loose SLP/SHP files for negative IDs and DRS
   SLP/SHP only for nonnegative IDs. The 19 other enabled expansion terrain
   records all say filename `None`, SLP ID `-1`; do not treat them as direct
   art. The text loader at `0x004479d0` constructs frame starts cumulatively
   from each shape's frame count and angle count.
3. Reconstruct combat/task/cost behavior from original executable routines
   using the decoded DAT parameters. The scenario unit IDs now
   resolve; authentic rendering, input, simulation, UI, and audio remain.

Ghidra 12.1.3 is at
`/mnt/d/Projects/PSP-GoExplore-NA/tools/ghidra_12.1.3_PUBLIC/`.
The analyzed projects are `build/ghidra/AoE97` and `build/ghidra/AoERoR`.
`tools/ExportAoEDecompilation.java` exports every function currently
discovered in each analyzed executable. Ignored reference dumps live at
`build/decompile/ror_functions.txt` (4,821 functions) and
`build/decompile/base_functions.txt` (5,440 functions), with address/line
indexes in the adjacent `*_functions.tsv` files. Both latest exports report
all functions decompiled. The older `*_index.tsv` files reflect an earlier
4,059/4,486-function snapshot and must not be used as the current index.
These dumps are navigation/reconstruction evidence, not compilable or
behaviorally validated source. Re-run the script after changing Ghidra's
analysis or types; keep expansion addresses primary.
`tools/ExportAoECallGraph.java` exports one row per direct call reference or
unresolved call site. Its ignored outputs are `build/decompile/ror_calls.tsv`
and `base_calls.tsv`. The current expansion graph has 30,531 rows: 24,706
internal references, 1,295 external references, and 4,530
indirect/unresolved sites. The base graph has 30,493 rows: 24,661 internal,
1,334 external, and 4,498 indirect/unresolved. A function may have no call
instruction, so these row counts are not function counts.
These counts cover Ghidra-recognized call instructions, not all dynamic
dependencies or function-pointer destinations.
The expansion graph traces viewport draw through `0x00512d40` →
`0x005136b0` → `0x00514c10`. It also identifies camera update
`0x00513440`, called from `0x00512d40`; base counterpart `0x004b76b0`
is called from `0x004b6f10`. Both update renderer pixel-camera fields
`+0x130/+0x134` from runtime tile origins, elevation, view center, and
floating camera coordinates. The expansion reads camera floats at world
offsets `+0x118/+0x11c`; base reads `+0x114/+0x118`. Ghidra's `__ftol()`
calls omit x87 operands, but direct disassembly resolves the fractional
projection and truncation. `terrain_compute_view_camera` now reconstructs
the camera pixel offsets, signed sweep start, and sweep sizes for supplied
runtime camera floats and viewport geometry. It has synthetic host/sanitizer
tests and cross-compiles for Vita. It is not wired to a live game camera,
surface, or complete draw loop.
Expansion `0x0045e290` clamps world camera floats to the map's last tile;
`terrain_clamp_camera_tiles` now mirrors this setter. Renderer binding
`0x00512760` stores the world pointer at renderer `+0x100`, while
`0x005126a0` copies map geometry. Expansion `0x005125a0` and base
`0x004b6670` calculate the horizontal/vertical sweep counts by viewport
width/height divided by DAT full tile width/height. The portable camera
helper now derives those counts itself rather than accepting caller guesses.
`terrain_draw_view_terrain_rgba` now traverses the original diagonal order
and composites the current direct terrain/border plan into a clipped RGBA
rectangle, checking that camera and rectangle agree. This remains a library
terrain layer, not a prototype game executable. The real-asset sanitizer
audit now also draws one fixed 640×480 centered terrain-only viewport for
each of the 44 available scenarios, requiring opaque pixels. It does not
compare original screenshots or test fog, units, overlays, UI, or gameplay.
The viewport caller's 32-bit tile visibility mask is now classified by
`terrain_view_visibility_code` into original mode bytes hidden `0x00`,
visible `0x0f`, and explored-fog `0x80`, including its bypass and fog-disabled
branches. The original terrain-only path continues to assume full visibility,
while `terrain_draw_view_visibility_rgba` now accepts caller-supplied runtime
masks and draws direct art with the corresponding clip planes.
The original fog-edge table generator (`0x00511d70` expansion,
`0x004b5e50` base) reduces 256 neighbor masks to 47 canonical edge IDs.
`terrain_build_visibility_edge_table` and `terrain_visibility_edge_pattern`
port its lookup and eight-neighbor ordering. `tileedge_parse` validates the
installed base `data/TILEEDGE.DAT` as 17 shape tables × 47 pairs of optional
stream offsets: 147,304 bytes, 1,581 nonzero offsets. Its synthetic ctest,
real-file sanitizer run, and Vita static-library build pass. The extracted
expansion disc does not include a separate `DATA2/TILEEDGE.DAT`; optional
edge-art loading in the executable is not yet recreated. The direct-art
8-bit mask/fill operation is ported, but complete fog/palette fidelity has
not been compared against original screenshots.
The original edge consumers `0x004a5690`/`0x004a56e0` read each non-null
stream as `(row,left,right)` byte triples until row `0xff`. Portable
`tileedge_visit_spans` validates and visits them. `tileedge_apply_coverage`
applies their inclusive add/remove spans to a clipped byte mask, with source
validation before destination mutation. The real-file sanitizer audit
traverses and rasterizes all 1,581 streams and 45,367 ordered spans,
counting 856,575 covered pixels across separate masks. The newer viewport
entry point uses these masks with caller-supplied visibility; no live game
state or real BLKEDGE file is available yet.
Rise of Rome's optional `DATA2/BLKEDGE.DAT` has 17 shape tables with 47
single-stream offsets rather than TILEEDGE's 47 pairs. `blkedge_parse`
handles that layout; only synthetic tests are possible because neither the
available expansion disc nor the installed base directory has the file.
Disassembly at expansion `0x00515294`/`0x005152b8` confirms two separate
clip-plane objects. `tileedge_build_view_plan` now selects their source
streams from the original visible/explored/hidden mode, fog flag, and two
neighbor patterns. `tileedge_apply_view_plan` executes the original
add/subtract sequence on two caller-owned byte masks with all selected
streams prevalidated; the installed TILEEDGE file passes all-shapes and
all-patterns paired-plane audit. The viewport path can now feed them from
caller-supplied runtime visibility, but not a running game simulation.
Expansion `0x00516b40`'s exact draw-pass order and palette mask constants
are now in `terrain_build_sprite_pass_plan`: second clip draws first in
mode 1, then first clip draws in mode 0 unless flags force mode 1; flag
`0x10` overrides with one default-clip pass. The 8-bit blitter's per-row,
per-byte mask/fill step is applied by
`terrain_blit_draw_plan_passes_rgba` to original direct terrain/border SLP
colors and caller-supplied clip planes. It is tested with synthetic art and
Vita cross-compiled. The new viewport entry point connects it to a supplied
runtime visibility grid and the viewport sweep, but still lacks non-direct
art, objects, UI, live state, and gameplay.
All 19 expansion campaign maps pass the sanitizer real-asset audit for this
path: each distinct frame sequence's normal clipped pass matches the prior
compositor byte-for-byte, and the mode-1 pass executes. This does not prove
visual equivalence to the original executable.
The mixed visible/explored-fog/hidden viewport audit now passes under
sanitizers for all 19 expansion and 24 installed base maps using original
DAT/DRS/scenario/palette assets plus installed `TILEEDGE.DAT`. The grid and
tile flags are synthetic inputs, not a functioning game simulation.
The terrain-only pass now applies the original viewport tile bounds gate,
using the elevation-shape-indexed DAT tile-size height. Expansion campaign
1_1 submits 341 rather than 630 cells to the blitter after culling, while
all 44 real-file sanitizer viewport audits still have opaque output.
`tools/DumpAoEFunctions.java` decompiles chosen addresses, while
`tools/CompareAoEFunctions.java` emits mnemonic signatures and ranks
cross-version candidates; every match still requires semantic review.
The bulk expansion Ghidra export missed 25 valid function starts referenced
by the first 40 entries of the 64-entry world vtable at `0x0054fe58`. Run
`tools/RecoverAoEVtableFunctions.java 0x0054fe58 40
build/decompile/ror_world_vtable.txt` against the expansion Ghidra project
to recover and export them. This has been run; the regenerated whole-function
export first contained 4,084 functions. A broader pointer-run audit then
recovered 736 more expansion and 950 base function starts; current analyzed
inventories are 4,821 expansion and 5,440 base functions. The refreshed
expansion call-site inventory contains 30,531 sites (4,530 unresolved), and
the base inventory 30,493 sites (4,498 unresolved). The 60-second bulk
exports now report all 4,821 expansion and 5,440 base function statuses as
`ok`, including the two large pixel routines. A post-recovery audit found
zero missing starts in the same 141 expansion and 102 base pointer runs;
this does not prove complete executable coverage. The expansion world tick is
`0x00520380` at vtable slot `+0xbc`, with base equivalent `0x00503390` at
the same slot of base table `0x00551e48`. The portable clock gate/time advance
is in `src/world_clock.c`. `src/world_tick.c` now carries the original
post-gate timer/player/follow-up/finalizer/seed-sync order and RNG checkpoint
and restore, with required callbacks for still-unported original routines.
`world_tick_test` passes host and sanitizer execution; the library cross-builds
for Vita. The external gate, live timer/player-object methods, edition-specific
player follow-ups, finalizer body, and optional
AI/game-over work remain unported. The outer-frame `0x0041ae90` virtual
`+0x54` dispatch is the active screen's UI update (`0x00454580` for the
game viewport), not the world simulation tick. Do not derive gameplay timing
from that UI dispatch.
The first pre-player world-tick call is now direct portable code:
`src/command_drain.c` translates RoR `0x0042a5d0` / base `0x0041f0b0`
(active-gated fetch, virtual dispatch, release, repeat). It replaces the
opaque `before_players_a` hook. Actual command fetching/decoding,
dispatch, and ownership remain external dependencies. `command_drain_test`
verifies the ordering.
The second pre-player call is also direct now: `src/scenario_timeline.c`
translates RoR `0x004777c0` → `0x004ee2b0` / base `0x0045f360` →
`0x00512a90`. It handles the original time gate, stale object references,
action kinds 0/1/2/4, and spawned-reference propagation. Its 32-bit node
layout is compile-checked in the Vita build. Live object virtual methods,
player-template lookup, and timeline construction still need binding.
`scenario_timeline_test` and `world_tick_test` pass; all 24 host/sanitizer
tests and the Vita library build pass.
The world tick's primary player call is no longer an opaque callback:
`world_tick.c` invokes `aoe_player_update_step` directly for each edition,
then resets that player's queue. `world_tick_test` now supplies semantic
player/outcome/object-list state and verifies an outcome-driven transition to
state 2 with event code 6. Actual object virtual methods, global event
delivery, map updates, and live player construction are still dependencies;
this is not yet a running game loop.
The post-checkpoint player `+0x30` object-list traversal is now direct in
both editions. RoR world tick calls `0x004521e0` for all players before
its `+0x40` pass (`0x00436f20`). Base player virtual `+0x80` is
`0x00517950`: it pumps cooperative work per player when pending, calls
`0x0049ce80` on `+0x30`, then `0x004f95d0` on `+0x40`. The shared
portable traversal replaces the former RoR follow-up callback and the
list portion of the base callback. `world_tick_test` covers both orderings
and the base per-player pump.
The `+0x40` visibility-watch bodies are now translated in
`src/visibility_watch.c` and called directly by `world_tick.c`: RoR
`0x00436f20` removes each invalid tracked unit immediately and revisits a
swapped slot; base `0x004f95d0` removes only the last non-null invalid unit
after scanning. Both notify recipients for cleared visibility bits 1–15,
excluding the owner. Swap-last helpers are RoR `0x00436e60` / base
`0x004f9520`. World unit-ID lookup (RoR `0x005230d0`, base `0x005062f0`)
is now direct code with separate positive and negative registry arrays.
The matching insertion and clear routines are also translated into
`AoeOwnedVisibilityRegistry`; `world_tick` references its stable view rather
than copying pointers that could become stale after growth. The constructor
now uses the original 10,000/1,000 slot counts and 0/-1 ID cursors, and the
matching positive/negative next-ID routines are translated. Live unit
construction/destruction, the positive-ID collision's displaced-unit
methods, and recipient virtual methods still need binding. On a flagged
positive collision the owned insertion now clears `+0x74` and reports the
required displaced-unit reassignment. The ID-table portion of world reset
also clears both arrays and resets the 0/-1 cursors, but does not replace
the reset's original unit teardown.
The original player `+0xdc/+0xe0` unit register/remove path is now in
`src/player_unit_lifecycle.c`: RoR `0x0045f810`/`0x0045f860`, base
`0x00519350`/`0x005193a0`. It routes into all three player object lists,
whose RoR `0x00451f00`/`0x00451f40` and base
`0x0049cbf0`/`0x0049cc30` link/unlink rules are implemented in
`src/object_update_list.c`, then inserts/clears in the world registry.
`player_unit_lifecycle_test` covers routing, links, and registry updates.
Actual unit constructors/destructors and displaced-unit methods remain.
`src/unit_storage.c` now translates the DAT-driven storage portion of base
unit construction: RoR `0x004a8950`/`0x004a89a0`, base
`0x004133a0`/`0x004133f0`. It selects the last mode-0 carried-resource
entry and applies mode-1/2 grants through `aoe_player_resource_add`.
`unit_storage_test` covers both editions; the remaining constructor virtual
methods and live unit allocation/registration are not yet implemented.
`dat_rules_test` also exercises the path from parsed storage bytes through
the runtime helper into the owner's resource array.
`visibility_watch_test` covers the edition differences. All 24 host and
sanitizer tests pass, and the affected Vita libraries build.
The post-update queue reset is now directly translated in
`src/player_queue.c` (RoR `0x0045fad0`, base `0x00519670`, player `+0x58`)
and called by `world_tick.c`; its former callback has been removed. It
clears two queue counters and conditionally replaces the eight-entry
storage. Host, sanitizer, and Vita library builds pass.
The same module now includes original queue construction, two-word append
and capacity doubling, storage/count view, pending-count drain, and cleanup
for both editions. Base constructor's minimum 100 and append's old-capacity
65,000 growth guard are preserved. These routines are tested in
`player_queue_test`, but not yet bound to live command producers/consumers.
The expansion player vtable `0x00549c70` resolves the world tick's `+0x7c`
player update to `0x0045df70`. Its downstream object/group/outcome sequence
is partially translated, but real object methods remain. The player resource set/add routines are fully identified in
both versions and now live in `src/player_resources.c`; host, sanitizer, and
Vita library builds pass.
The first shared downstream object-list update is now in
`src/object_update_list.c`: Rise of Rome `0x004521e0` / base `0x0049ce80`.
It preserves cached-next traversal, the `+0x24` predicate / conditional
`+0x08` action dispatch, and a live cooperative-flag check every ten
non-null objects. Host, sanitizer, and Vita builds pass. This is still
unbound from actual player objects; the object virtual methods remain to
be ported.
`src/player_update.c` now composes the shared player virtual method order
(RoR `0x0045df70`, base `0x005178f0`) using that list routine and group
prune. It now invokes the concrete outcome dispatcher and state setter rather
than gameplay callbacks. State setter RoR `0x0045cb90`, base `0x00516480`
preserves bytes `+0x80/+0x81` and emits event code 6 on an unlocked
transition to state 2. Host/sanitizer tests and Vita library build pass.
Global event delivery, map/object virtual methods, and real player-object
binding remain; `world_tick.c` does not yet
drive a complete player implementation.
`src/player_outcome.c` now translates the shared outcome manager
(RoR `0x005101c0`/`0x005100e0`, base `0x004338c0`/`0x004337d0`) and is
called by `player_update.c`. It preserves event/condition list dispatch,
grouped outcome calculation, the cumulative type-3 counter, and the
no-condition retained result. Original manager virtual `+0`/`+4` bodies
are now translated as kind dispatch in `src/player_outcome_dispatch.c`.
The manager vtables are RoR `0x0054fae0` (dispatch `0x00510260`/
`0x005102c0`) and base `0x0054c1b0` (`0x00433960`/`0x004339c0`). The
outer traversal gates on status (`+1` event, `+0x2e` condition), while the
virtuals dispatch on the distinct kind byte `+0`; the prior semantic model
conflated these and has been fixed. Five event kinds and twelve condition
kinds dispatch, with condition kind 11 a no-op. All five event kinds are
translated: RoR `0x005113b0`–`0x005116a0`, base
`0x00434b30`–`0x00434e20`. They read owner resources, preserve the
original threshold, tie, and high-water branches, and replace weighted
contributions in the manager's 32-bit score sum. All recognized condition
kinds are now translated over semantic references. Kinds 0 (tracked unit ownership),
1 (code-2 unit count),
2 (unit state), 3 (accumulating count of code-7 units), 4 (unit in region),
5 (two-unit proximity), 6 (resource target with optional mutually allied
total), 7 (progress percentage), 8 (code-2 unit count in a region), 9 (no matching units), and
10 (player state), and 12 (team-average score) are translated in
that dispatch module. RoR kind 5 is `0x00510940`, base `0x00434070`; their
opposite-signed embedded constants yield the same extents-plus-one
threshold. Kind 9 (RoR `0x005107d0`, base `0x00433f10`) scans either a
referenced player's unit list or eligible non-self players from the owner
and world. Kind 3 (RoR `0x00510700`, base `0x00433e30`) instead scans all
eligible players, including the owner, accumulates matching-unit counts
across updates with 32-bit wrap, and compares the result as signed against
the threshold. Kind 6 (RoR `0x00510a20`, base `0x00434140`) sums the owner
resource and, when requested in team mode, mutually eligible players'
resources before an inclusive signed-int-to-float target comparison.
Kinds 1 and 8 (RoR `0x00510440`/`0x00510550`, base
`0x00433b40`/`0x00433c70`) count the owner's code-2 units plus those of
mutually eligible team-mode players starting at index 1, with kind 8
also enforcing inclusive map bounds.
Kind 0 (RoR `0x00510380`, base `0x00433a80`) tracks a referenced unit's
owner, including mutual-alliance acquisition and the original asymmetric
alliance-loss rule; a code-3-or-higher unit clears the reference.
Kind 12 (RoR `0x00510bb0`, base `0x004342f0`) reads manager score `+0x1c`,
adds mutually allied players' scores through their manager at player
`+0x34`, divides as signed 32-bit by the member count, and latches status 2
when the threshold is met. The other-score getter is RoR `0x004550b0`,
base `0x00434e90`.
Kind 7 (RoR `0x00510ad0`, base `0x00434200`) uses the signed
`+0x1c/+0x20` ratio of player `+0x38` (getters RoR `0x0051a270`,
base `0x0040a110`) multiplied by the shared 100.0 constant, checking
owner or eligible team players and latching status 2. That progress
object's broader role and live binding remain unverified.
These handlers still
need live referenced objects. All 24 host/sanitizer tests and Vita library
build pass.
The two world-timer rotations are now translated in `src/world_timers.c`
and called by `world_tick.c`: RoR `0x00520300`/`0x00520340`, base
`0x00503320`/`0x00503350`. Assembly confirms the first selects an object
from world `+0x30` (omitted by Ghidra pseudocode); the second selects from
`+0x38` and skips null. Both array selections have portable bounds checks.
The action bodies (`0x004a3a30`/`0x004a7c00` RoR and
`0x0042b6a0`/`0x004b3e30` base) are translated in
`src/timer_actions.c` and have selector bindings. Timed-event target expiry
(RoR `0x004a49e0`, base `0x0044d370`) is now translated in
`src/audio_target.c` and bound back to the timed-event action. It follows
the target chain to the final node and preserves stop/voice/buffer cleanup
order. The stop condition itself (RoR `0x004a4b70`, base `0x0044d510`) is
also translated. Original voice stop/release, allocator-free, and lifetime
handle cleanup/free bodies remain unported. All 16 host/sanitizer tests pass
and the new library cross-builds for Vita.
The next original player-update dependency, group pruning, is now in
`src/player_group.c`: 25 Rise of Rome or 40 base slots, original keep/remove
predicate, swap-last compaction, leader replacement, and deferred recheck of
a swapped-in entry. It passes host/sanitizer tests and Vita cross-build, but
is not connected to live unit objects.
The original RNG is now ported in `src/aoe_rng.c`: both executables start
from seed 1 and use the same 32-bit `0x343fd`/`0x269ec3` recurrence with a
15-bit return value. The verified seed-1 sequence is in `aoe_rng_test`.
The world-tick core wires this state into call ordering and calls the
translated primary player-update routine. Its object/map/event callbacks
have no production implementation yet.
Example:

```sh
GHIDRA=/mnt/d/Projects/PSP-GoExplore-NA/tools/ghidra_12.1.3_PUBLIC
"$GHIDRA/support/analyzeHeadless" build/ghidra AoE97 -process Empires.exe \
  -noanalysis -scriptPath tools -postScript DumpAoEFunctions.java 0x004d45c0
"$GHIDRA/support/analyzeHeadless" build/ghidra AoERoR -process EMPIRESX.EXE \
  -noanalysis -scriptPath tools -postScript ExportAoECallGraph.java \
  build/decompile/ror_calls.tsv
```

Microsoft's `AOEPATCH.exe` is now part of the reversing pipeline. It contains
complete AoE 1.0c and Rise of Rome 1.0a replacement executables. Their exact
hashes, official version distinction, behavior index, and reproducible Ghidra
import/export command are recorded in `docs/PATCH_REVISIONS.md` and
`tools/import_patch_revisions.sh`. New recovery work should compare the
release and patched revision rather than assuming addresses or fixed behavior
carry across both images.

The first persistent per-frame live-unit seam is now connected.
`aoe_live_unit_tick_sprites_ror` ports the sprite-manager tail of release RoR
type-20/25 update predicate `0x00415820`, independently confirmed at patched
1.0a `0x00415200`. It passes world frame delta (`player +0x3c`, world
`+0x84`) and the unit animation-rate float (`live +0x88`) exactly as the
binary does. Types 30..80 inherit this tail; root type 10 and type 90 do not.
The matching constructors zero the rate, now represented explicitly in
`AoeLiveUnit`. `aoe_scenario_runtime_tick_sprites_ror` applies the seam over
the persistent scenario population without pretending to implement the
preceding action/movement/combat update. The real `EXPANS_1_1.scx` audit
visited all 3,809 units and reached this inherited path for 624 real sprite
instances. All 71 host tests pass, the new path passes ASan/UBSan with leak
detection, and both affected libraries cross-build with VitaSDK.

The mounted `F:` virtual DVD exposes only one data track, so it cannot supply
the base disc's original CD music. The expansion BIN/CUE does preserve 14
audio tracks. `tools/extract_ror_audio.sh` decoded Track 02–15 into lossless
44.1 kHz stereo WAVs under ignored `assets/ror_audio/`, skipping Track 02's
one-second INDEX 00 pregap. All 14 WAV payload sizes and formats were verified
against the BIN tracks. Vita music playback is not implemented.
The separate `E:\msaoe.exe`
demo installer contains 12 MIDI music/cue files and six ambient WAVs;
these are extracted under ignored `assets/demo_audio/`.

The next map-runtime seam is `aoe_runtime_map` (`include/runtime_map.h`,
`src/runtime_map.c`). It creates owned semantic tile, shape, border,
projection, object-list, view-byte, and visibility-mask surfaces from a
verified square scenario, using parsed DAT geometry and terrain/border
tables. `original_runtime_map_from_scenario` tests placement plus the
player-vision binding on synthetic data; the host suite reached 51/51 and
the library cross-built for VitaSDK. It is not the PC tile ABI, has not
been validated as a live game map, and supplies no playable world loop.
`tests/runtime_map_real.c` now audits the owned map against the loader and
terrain functions for all 19 installed RoR expansion, 24 base, and one demo
scenarios; all 44 pass. The 250x250 expansion scenario also passed ASan/UBSan,
and the full 51-test host suite passes. This is an internal consistency and
real-data coverage check, not an independent pixel or binary-state fidelity
oracle. Next: bind original unit construction/updates and viewport input to
this owned map, then compare live state against the PC game.

`tests/runtime_units_real.c` now composes the existing source-backed
unit-core constructor, DAT player-template clone, reserved scenario spawn
IDs, RoR tile placement, and owner visibility on `AoeRuntimeMap`. Every
record in all 19 extracted expansion campaign scenarios passed; the
14,923-record `EXPANS_1_0.scx` run also passed ASan/UBSan. The two real-map
audits are conditionally registered with CTest when the extracted files are
present; the host suite now passes 53/53. The audit detaches each player’s
units before their storage is freed. This is a construction/placement-phase
coverage check only: it omits original scenario state dispatch and player/
world registration, and does not create persistent, updateable game units.
Next: recover and bind those missing original methods in the production
runtime, not a stand-in skirmish.

The first production-facing composition is now `aoe_live_unit_factory`.
Its Rise of Rome constructor combines the recovered common initializer
`0x004a7c80`/`0x004a8200`, DAT standing-graphic selection, sprite-manager
creation, map placement with the original vision hooks, and player/world
registration into one persistent `AoeLiveUnit`; teardown removes the matching
player, registry, tile, visibility, and sprite links. A later assembly audit
corrected its registration API: `0x004a8200` passes live `+0x49` (positive HP)
and `+0x4a` (type 25) directly as the player `+0xdc` list selectors, so callers
can no longer inject guessed values. It still refuses the world-replacement
branch because that needs the displaced unit's untranslated rebase method. It
does not apply the
scenario's class-specific state `+0xb4/+0x6c/+0x68` methods, create a
viewport, or claim a scene. Host 67/67, ASan/UBSan focused test, and Vita
static-library build pass.

The factory now also exposes the source-selected RoR scenario dispatch used
after construction: low states call the root no-op or derived
`0044ca40`/`0044c7b0` rotation/frame path, root middle/high states use the
translated methods, and type-70/80 middle states include the recovered
`004b02a0` timer/lookup/kind-`0x68` prelude before root middle behavior.
The class-specific lookup effect, type-40/50/60/70 high action clear, and
type-80 building high method remain required bindings. This is a persistent
unit dispatch layer, not proof that a scenario can yet be drawn or played.

Spawn-state audit: `unit_refs_tool SCENARIO EMPIRES.DAT --spawn-states` now
reports branch counts by original DAT unit type. Across all 19 extracted
expansion campaigns there are 150,062 records with state 0–2, zero with
state 3–6, and 1,850 with state 7+. `docs/reversing.md` now records all nine
relevant RoR live-unit vtables and their distinct `+0x5c/+0x68/+0x6c/+0xb4`
targets; the earlier four-vtable inventory omitted types 20–60. The
`+0xa4` branch for types 10–60 is a type-specific live-object pool/reinitialize
path, not a different constructor family. The next state work must preserve
class-specific dispatch and
the setter's visibility/owner/action effects; setting `core.state` alone
would misrepresent the original game. This is analysis progress, not a
live state transition implementation.

The live viewport now has a portable indexed sprite backend for the recovered
RoR draw chain. `aoe_live_unit_sprite_indexed_ror` connects live-unit bounds
to sprite-runtime traversal, composite graphic planning, cached SLP loading,
player colors, and the original shadow table. RoR `0x004a7110` supplies the
composite rules; crucially, post-load fixup `0x004a6970` maps delta ID `-1`
back to the owning graphic rather than leaving a null child. Real DAT audits
pass for all 240/804 expansion composite roots/angle plans and all 209/718
base roots/plans. Host 68/68, focused ASan/UBSan real-asset runs, and the new
backend's VitaSDK cross-build pass. The selected-graphic/global draw-ID
semantics, outlines/remaining color-table modes, terrain/object interleave,
and actual Vita display submission remain incomplete.

The new patched-executable comparison confirms the draw chain is unchanged in
RoR 1.0a at `0x004a51c0/0x004a5920/0x004a55e0/0x004a7260`. The tile-selected
graphic argument is now consumed through the backend's `resolve_color_source`
boundary, reproducing its original `+0x26` player bank and `+0x28` transform-
table role instead of ignoring it. Focused host, ASan/UBSan, and VitaSDK
builds pass.

Fixed DAT colors are also separated from dynamic selected colors now.
`aoe_sprite_draw_transform_table_ror` uses the selected `+0x28` table only
when graphic `+0x2a` is negative; otherwise the backend resolves that fixed
color ID through `resolve_fixed_color_table`, matching both release
`0x004a6dd0` and 1.0a `0x004a55e0`. Original shadow-art rendering, focused
sanitizer runs, and the VitaSDK backend build pass.

`aoe_color_table` now owns the fixed-table registry rather than leaving it
entirely to a future callback implementer. A loader audit corrected an unsafe
initial DRS-ordinal assumption: DAT ordinal 10 `Shadow.col` is DRS 50909, not
50910. Original RoR `0x0041d4a0` actually opens `data/<DAT filename>` and
leaves an allocator-zeroed table when the file is absent. The portable
directory loader now reproduces that rule, records loaded versus missing
tables, and exposes the checked resolver consumed by the sprite backend.
DRS 50909 remains the independently verified portable source for
`Shadow.col`; no formula is claimed for the other DRS resources.

The RoR `0x004adc70` positive-health/owner-registration toggle and root
middle-state `0x004abbc0` are now ported to `aoe_live_unit_core`. The new
`registration_pending` field represents byte `+0x4b`; caller-supplied
player remove/register callbacks are mandatory for immediate list changes.
Unit tests cover no-op, deferred, immediate, and root middle-state order;
the host suite remains 53/53, focused ASan/UBSan tests pass, and the library
cross-builds for VitaSDK. This is not yet connected to post-spawn dispatch
or real player registration; derived middle/high methods and the root
`+0x5c` setter's dependent effects remain incomplete.

The root RoR `+0x5c` setter's outer control flow is now in
`src/live_unit_state.c`, with the DAT template `+0xa6` byte parsed as
`state_effect_mode`. It preserves state-change gating and ordered callback
boundaries for effect exit, mode-2/pre-terminal methods, normal/alternate
vision changes, conditional state-3 action-list work, and the new health
registration toggle. Missing original methods are required callbacks, not
silent no-ops. The host suite passes 54/54, focused ASan/UBSan tests pass,
and `aoe_live_unit_state` plus `aoe_dat` cross-build for VitaSDK. Next:
implement and bind the original callback bodies and derived setter overrides
before invoking this path for scenario units or claiming live state changes.

The base root `+0x5c` setter at `0x004167f0` is now translated alongside
RoR in `src/live_unit_state.c`. Base permits any changed state, orders
pre-terminal work before exit-active work, and omits RoR's template `+0xa6`
mode-two call. Comparison also found a missing conditional action-cleanup
effect in the root middle-state method: both editions call it when unit
`+0x36 & 1` is set. `AoeLiveUnitCore` now stores that flag and requires an
action callback in that branch. The full host suite passes 54/54, focused
ASan/UBSan tests pass, and the state library cross-builds for VitaSDK.
These are source-backed method boundaries, not a live post-spawn dispatcher.

The first root-setter callback has now been replaced with original code:
RoR `0x004a8a00` / base `0x00413440` subtract mode-2 entries from the
owner's resources. `aoe_unit_storage_exit_active` uses the existing parsed
three-entry DAT storage table and edition-correct player resource add;
`aoe_live_unit_state` invokes it directly in each edition's original order.
Host 54/54, focused ASan/UBSan, and VitaSDK library build pass. The state
setters still require their remaining sprite/action, vision, and owner
callbacks and are not wired into a playable world.

The RoR alternate unit-vision branch is now ported. The original hooks
`0x004ac290`/`0x004ac380` use template size, truncate four inclusive
rectangle bounds with a `0.01` upper-edge subtraction, and call
`0x0051a5a0`/`0x0051a770`. The portable rectangle enter/leave shares
the circle branch's exact count/mask/discovery cell update; a unit-level
helper selects circle or rectangle and reads real template LOS/size fields.
Focused host and ASan/UBSan tests, the full 54-test host suite, and the
VitaSDK library build pass. This is not yet bound into the root state
setter callbacks, player/world registration, or a game screen.

The terrain and live-unit render paths now share an original-order viewport
composition boundary. `terrain_draw_view_visibility_indexed_with_tiles`
invokes a caller after each projected non-hidden tile's terrain passes and
passes the unchanged absolute tile anchor. Empty direct-art plans do not
suppress the callback. `aoe_runtime_viewport_draw_tile_ror` consumes the
corresponding `AoeRuntimeMap.objects` list, preserves its link order, selects
the alternate object pass from runtime tile byte `+9 == 0x80`, and dispatches
constructed `AoeLiveUnit` objects through the recovered live viewport draw.
DAT template byte `+0x65` is now parsed as `draw_class`; the unresolved
class-3 nested `+0x90` alternate graphic remains caller-resolved. All 69 host
tests pass, focused ASan/UBSan tests pass, the real Rise of Rome campaign
audit reports 263 post-terrain object seams with unchanged pixels, and the
combined runtime viewport library cross-builds for VitaSDK. The next scene
milestone is to construct actual scenario units with their player/runtime
owners, provide class-specific entry metadata, and use the indexed sprite
backend as this callback's real renderer.

That construction milestone now has real campaign coverage. The new
`runtime_full_units_real` audit holds all nine players and the shared world
alive simultaneously, then constructs every `EXPANS_1_1.scx` record through
DAT graphic selection, sprite-manager creation, map placement, vision,
player registration, and world-ID registration. It constructs all 3,809
units and runs the recovered 0–2 scenario virtual for 3,772 of them. The 37
state-7 records are reported but not forced into state 7 because their
remaining required virtual effects are not all translated. Constructor
registration was corrected directly from `0x004a8200`: live bytes `+0x49`
(positive HP) and `+0x4a` (type 25) select player lists `+0x2c/+0x30`; they
are no longer caller parameters. Unit tests cover both precedence cases.
All 70 host tests, focused ASan/UBSan construction of the 3,809-unit world,
and the affected VitaSDK libraries pass. Next, finish the state-7 callbacks
and retain this owned world for actual terrain-plus-unit scene rendering.

The root state-7 gap for the selected campaign is now closed. Ghidra and raw
vtable inspection confirm types 10 and 90 share `+0x60 = 0x004abbf0`,
`+0x64 = 0x004abd10`, and high method `0x004abb80`. DAT template bytes
`+0xa5/+0xa6/+0xa7` are parsed, and `aoe_live_unit_scenario_high_root_ror`
binds effect-free root units to actual owner-list removal/re-registration and
normal/alternate visibility hooks. The campaign audit now dispatches 3,772
low-state plus 35 immediately effect-free state-7 units. The other two Gaia
trees have modes `1/2/0`, but their live tile masks are zero, selecting no
per-player notification branch. Their exact remaining behavior is a
visibility-watch add in `0x004abd10` followed by removal in `0x004abbf0`.
`aoe_visibility_watch_add` now ports `0x00436dd0` with 32-entry growth,
owner/duplicate checks, flag snapshot, and ID retention. The audit therefore
dispatches all 3,772 low-state and all 37 high-state records. Full sanitizer
and VitaSDK builds pass. Nonzero high-mask notification branches remain part
of the wider full-game scope, but no longer block this authentic scene.

The owned nine-player campaign construction is now production code rather
than test-only scaffolding. `include/scenario_runtime.h` and
`src/scenario_runtime.c` expose `AoeScenarioRuntimeRor`, which owns the
runtime map, player template clones/resources/vision, live units, sprite
instances, object links, and world ID registry. It builds in scenario player
and record order, runs both recovered scenario-state roots, supports switching
the active player's map view-code binding, and tears the world down in reverse
unit order. `runtime_full_units_real` now calls this public module directly;
the same `EXPANS_1_1.scx` result remains 3,809 constructed units, 3,772 low
state dispatches, and 37 high state dispatches. All 70 host tests pass, the
real world builds/destroys cleanly under ASan/UBSan, and
`aoe_scenario_runtime` cross-builds with VitaSDK. Next: retain one of these
production runtimes behind a real scene entry point and feed its active-player
map, visibility, and object lists into `aoe_runtime_viewport`; do not recreate
the old audit-only constructor. Wider scenarios with state 3..6 or nonzero
high-mask notification callbacks still require their original bodies.

That real scene entry point now exists as `aoe_scenario_scene`. Runtime map
ownership includes the original tile `+10` draw-flag byte array, and
`aoe_scenario_runtime_draw_scene_ror` derives the active player's visible and
explored masks, supplies those flags, maintains reusable maximum-tile-list
scratch storage, creates the indexed sprite surface, and calls the recovered
visibility-aware terrain walk with `aoe_runtime_viewport_draw_tile_ror` at its
per-tile object seam. Thus terrain/borders and the runtime's linked live units
now reach one indexed frame in original tile/list order through a single
production API. Player color/type-25 and class-3 state remain required
callbacks rather than fabricated defaults. A focused owned-world test covers
the complete seam, all 71 host tests pass, both it and the 3,809-unit real
world pass ASan/UBSan, and `aoe_scenario_scene` cross-builds for VitaSDK.
Next: construct the concrete asset/color resolver and Vita frame owner, then
run this API with the original DRS/TILEEDGE assets to emit the first authentic
campaign scene frame.

The first authentic campaign scene frame is now emitted. The host-only
`scenario_scene_tool` loads the patched expansion DAT and scenario, expansion-
before-base `graphics.drs` and `Interfac.drs`, base terrain/border archives,
TILEEDGE, DAT-named color tables, and the complete owned runtime. Units now
retain a semantic runtime owner index so a scene resolver can supply explicit
player colors without pointer scans. On `EXPANS_1_1.scx` at its serialized
player camera the tool processes 450 terrain/object tiles and visits 232 real
objects, drawing 231 through original SLPs (one original draw filter omits the
other), with 20 graphic cache misses and 211 hits. The inspected 960x544 frame
is `assets/ror-scene-alpha.png` (SHA-256
`ad5477e407a6f74b230244b09137b18c3e2632c4f9e9211e973926a517fa5487`).

Visual inspection found and fixed a real portability defect: Windows accepts
the DAT name `Shadow.col` for installed lowercase `shadow.col`, while Linux
left the original color object zero-filled and rendered shadows as black
polygons. `aoe_color_table_set_load_directory` now retries an ASCII-lowercase
filename, preserving the original Windows behavior. The corrected real render
passes ASan/UBSan; all 71 host tests pass, and affected scene/color/runtime
libraries cross-build for VitaSDK. The tool currently uses the recovered
exploration-gate bypass because scenario fog-history initialization is not yet
ported, and supplies the conventional Gaia/player color map explicitly because
the original serialized player-color field at constructor source `+0x28` is
not yet parsed. Recover that field before making color selection implicit.

Regenerate the raw frame from the project root with:

```
build/host/scenario_scene_tool build/ror/scenarios/EXPANS_1_1.scx \
  build/ror/disc/GAME/DATA2/EMPIRES.DAT \
  build/ror/disc/GAME/DATA2/GRAPHICS.DRS \
  '/mnt/d/Age of Empires/data/graphics.drs' \
  '/mnt/d/Age of Empires/data/Terrain.drs' \
  '/mnt/d/Age of Empires/data/Border.drs' \
  build/ror/disc/GAME/DATA2/INTERFAC.DRS \
  '/mnt/d/Age of Empires/data/Interfac.drs' \
  '/mnt/d/Age of Empires/data/TILEEDGE.DAT' \
  '/mnt/d/Age of Empires/data' build/ror/expans_1_1_scene.ppm
```

Next: recover the scenario player color/fog initialization, then select a
real campaign camera containing a built town and opposing units for the
requested fighting-town alpha rather than fabricating a scene.

Real-scene selection and missing-resource behavior are now substantially
closer to the executable. `scenario_scene_tool --auto-dense` scans constructed
non-Gaia live units and deterministically favors a viewport containing the
greatest number of distinct owners, then local density. On `EXPANS_1_1.scx`
it selects camera `(81.5,119.5)`, with 21 nearby non-Gaia units, a real
settlement, military formation, resource sites, and multiple player-owned
objects. The corrected frame processes 542 terrain/object tiles and visits
108 live objects, drawing 107. It is archived as
`assets/ror-battle-town-alpha.png` (SHA-256
`4e1c546e08bb74fb5961f1865e9bea2639710693149a5cc9f91e453b1e716366`).
It is an authentic populated-town/army frame, not yet proof of animated
combat because action/update simulation is not connected.

The dense viewport exposed two original asset behaviors. First, the sprite
lookup now uses the complete registered archive stack rather than only
`graphics.drs`. Second, real graphic `RGranary1` requests SLP 722, which is
absent from every supplied base/expansion DRS and extracted disc file. The
original sprite dispatch is void and continues after unavailable art;
`AoeLiveUnitIndexedBackend.skip_missing_art` now provides that explicit
policy, records `missing_skipped` (six attempts in this frame), and still
fails malformed or unsupported art. Backend diagnostics retain the last unit
ID and DAT graphic.

Missing loose color files also no longer leave black polygon transforms.
`aoe_color_table_set_load_drs_fallback` fills known VER 3.7 DAT ordinals from
the original interface text resources: ordinals 0..8 from 50900..50908,
`Red_lt.col` from 50910, and the independently byte-verified `Shadow.col`
from 50909. Loose files still take precedence. The real dense render passes
ASan/UBSan, all 71 host tests pass, and the scene, sprite-backend, color-table,
and affected runtime libraries cross-build for VitaSDK.

The SCN parser now retains `player_data_fixed_offset`; inspection of the 1.15
tail proved its large fixed block contains resource/AI/settings records, not
the live player color byte. Binary construction evidence remains: RoR
`0x0051fa40` passes one 65-byte player-definition record to `0x0045c650`,
which copies record byte `+0x28` to player byte `+0x54`; civilization comes
from the separate scenario-object byte array at `+0x251`. Do not replace this
unrecovered field with the current tool's explicit conventional color map.
Next: trace construction of those 65-byte records, port action/update methods,
and advance real units into combat for the requested fighting alpha.

The owned terrain-completion path now fully composes RoR 00449110 with
004481b0. Placement uses Gaia's cloned template, its restriction row retained
from `DatIntro`, and the exact recovered validator arguments (visibility off,
shape mode 1, reserved zero, clearance on, overlap exception off, unit
collisions on). Creation uses the owned Gaia live factory at Z zero. Parsed
intro, terrains, and borders are caller-owned and must outlive the runtime.
The real campaign regression repaints terrain 0 under a Gaia decoration to
check state-7 displacement, then paints terrain 7 and checks cache/draw bytes.
That two-step test preserves 00449110's write-before-transition ordering;
installed foundation terrains 7/8/9 have no terrain-unit entries. It also
separately exercises terrain-object creation and constructs/ticks 3,812 live
units. `aoe_scenario_runtime_building_completion_world_ops_ror` now installs
that painter, the owned stack factory, and expanded invalidator into type-80
completion while leaving research/UI state explicit. Its runtime-only paint
callback propagates owned transition failure after preserving the recovered
world-effect order, and the real campaign regression enters paint through
this adapter. Next continue through the live cleanup/update chain.
