# Rise of Rome building update: 004ae880

`src/building_update.c` translates the complete field/control-flow body at
004ae880–004ae970 in `build/ror/disc/GAME/EMPIRESX.EXE` (SHA-256
8e0e6d047b2f90d0769fb5a1b16c1b1973582efdbe73a5d11395a43557215845).
It is not connected to live runtime buildings yet. The parent update and
derived state setter remain mandatory integration dependencies; tests use
spies, not substitutes for gameplay implementations. Both the terminal and
active-production branches now call translated implementations directly.

Evidence can be reproduced with:

```sh
objdump -d -Mintel --start-address=0x004ae880 --stop-address=0x004ae971 build/ror/disc/GAME/EMPIRESX.EXE
objdump -d -Mintel --start-address=0x005283e0 --stop-address=0x00528407 build/ror/disc/GAME/EMPIRESX.EXE
```

Order and details:

1. State zero alone computes graphic angle count times accumulated build
   time divided by signed template build time. Helper 005283e0 truncates to
   int64. Importantly, only its low **byte** is then compared with the signed
   angle count. This is not a saturating frame-selection function.
2. Write a changed in-range angle. At accumulated time >= template time,
   invoke virtual +0x5c with state 2, before shifting histories.
3. Copy +1e8 to +1ec, +1e4 to +1e8, +1f4 to +1f8, +1f0 to +1f4,
   and global 007d74d0 to +1f0. These are raw 32-bit copies; field semantics
   have not yet been established.
4. Call translated 004b0100 and retain AL. Examine the state after that call; state >2
   calls 004af890, otherwise 004af900.
5. Copy +1b0 to +1e4 after that final call, then return the retained AL.

The update does not itself advance the construction accumulator. Inputs
with corrupt/nonfinite/negative accumulation or nonpositive construction
times/counts are explicitly unsupported, rather than silently assigned a
made-up game result. The numeric calculation uses long double; this does
not emulate every x87 precision-control setting on ARM. Standard real-game
stage boundaries and low-byte wrapping are tested, but instruction-level
differential floating-point validation remains outstanding.

## Terminal branch and action ownership

`aoe_building_terminal_cleanup_ror` translates 004af890–004af8f0:
if +1d2 is 4, decrement the global per-owner counter at
`[00585e88]+0xa34+4*owner_id`. Reset nonnegative/other status values except
the -1 sentinel to -1, zero +1c6, clear the action list at +1cc, then
increment byte +1d0 with wraparound. The unsigned counter also wraps exactly
like x86 DEC. Repeated calls still increment the revision, even with an
empty list. No queue-array release or explicit refund occurs in this body.
The selected counter is passed as a pointer by the caller; its broader UI
meaning has not been established.

`src/action_list.c` implements the actual list ownership methods used here:

| RoR address | Portable method | Original behavior |
| --- | --- | --- |
| 00404410 | push | link at head, then action +1c(1) |
| 00404460 | append | link at tail, then action +1c(1) |
| 004044e0 | pop | unlink, action destructor(1), free node, reread head/owner state, optionally action +1c(0) |
| 00403e70 | clear | repeatedly pop until the live head is null |

Insertion at owner state >2 destroys the offered action instead. Pop only
activates a surviving head at owner state <3, inspecting state after the
destructor. Clear therefore includes intermediate activations for an active
owner; it is not equivalent to simply freeing a chain. Destructors may alter
the owner state or insert a new action, and these live changes are observed.
Tests cover both, plus terminal cleanup ordering and byte/counter wrapping.

Class-specific action destructors and state methods remain explicit required
operations, not no-ops. Lists own allocated nodes and successfully inserted
actions; allocation failure leaves the action with the caller. The portable
API rejects invalid dependencies/null active actions instead of reproducing
the original access violations. It requires valid acyclic list storage and
callbacks that preserve the owner and operation bindings.

## Active production and queue consumption

`src/building_production.c` translates 004af900–004afb3a,
004af7f0–004af884 and the positive-removal contract of
004af680–004af7e9, verified using disassembly of the same executable above.
Their patched 1.0a counterparts are 004adfc0, 004adeb0 and 004add50.
The constant at 0054d344 is float zero. The production branch operates on
original owner resource indices 11 (`+2c`), 32 (`+80`), and 4 (`+10`).
For an empty action list, production can start only when resource 11 < 32
and resource 4 > 0. This gate is bypassed when the action list has a head.

Before the resource gate, 004af1a0 is queried with final arguments 0,0.
A true result and first output 102 or 103 enters a queue-drain loop, then
returns without updating the action list. Both disc 004af900 and patched
004adfc0 pass signed `-1` to their cancellation routines. Both cancellation
routines subtract that signed argument from a normally positive quantity, so
the call grows the head quantity and the loop cannot terminate. Ordinary
callers pass positive removal counts; the network/order path also negates a
negative delta before calling cancellation. The portable drain therefore
passes positive `1` and retains a progress guard as a deliberate correction
of this verified original defect.

`aoe_building_queue_cancel_ror` implements the positive-count behavior:
reverse search for the requested unit ID, refund `min(quantity, amount)` one
item at a time, wrapping 16-bit subtraction and revision increment, exhausted
entry removal/shift, and head-removal status/action cleanup. It emits event
124 for every initially nonempty call, even when the ID is absent; an empty
queue emits nothing. Refund and global notification bodies remain explicit
integration dependencies. Head removal first invokes the current production
action's vtable `+0x2c` cancellation method, then clears the list.

If gated, set queue status 4 once, emit event 125 (reason 32 if resource 11
>=32, otherwise reason 4), then increment the per-owner counter. Repeated
gated updates do not repeat the notification. Disabled/empty queues clear
only status 4 and decrement its counter; other statuses are left unchanged.
When the gate clears, decrement a prior status-4 counter, set status -1,
allocate/construct the production action via 004b6820(building,head ID,-1,1),
and insert it through the translated action list. A null allocation still
reaches the list update, matching the original caller.

List virtual +0xc result 1, 2 or 3 pops the current action and consumes one
queued item. Other result bytes leave the queue unchanged. Consumption
decrements the head quantity with 16-bit wrapping and increments revision
with byte wrapping. At quantity <=0 it removes the head queue entry by
shifting subsequent ID/quantity pairs, leaving the unused trailing slot
unchanged. It then emits event 124(owner,0,0,0). Empty consumption does
nothing, including no notification.

Tests exercise all result classes 0–4, both resource gates, existing-action
bypass, notification/counter order, both cancellation codes, null action
creation, queue shifts, empty queues and revision/quantity wrapping.
The portable API bounds-checks queue storage and rejects nonfinite resource
inputs; x87 unordered behavior for corrupt resource data is not emulated.
Allocation and callback failures after earlier effects do not roll back the
simulation. Production now composes the translated query and queue
cancellation directly. Per-item refund, action-specific cancellation, action
factory/update and global notification effects remain explicit bindings.

## Verified production list dispatch

Building vtable 0054d0c8 +218 points to 004b0020. That factory invokes
004b5ac0, which installs list vtable 0054dac8. Its +0xc slot is 004043c0,
the same as root list vtable 00547240. This establishes the actual production
list update rather than assuming that a base-class method is inherited.

`aoe_action_list_update_ror` now translates 004043c0–00404408 and is called
directly by production. Empty lists return byte 1. Otherwise the method
calls the head action's +0x28 update and saves AL, rereads the live head,
calls each successor's +0x24 method, then pops the live head if the saved
result is 1. If head update emptied the list, it skips traversal and pop.
The saved result is returned unchanged, including values 2,3 and 255.
Waiting callbacks must preserve their current node's lifetime, as the
original loop reads its next pointer after the callback.

Consequently production's own pop for result 1 is an additional pop, not
an alternative to the list's pop. Preserve this even for multiple actions.
Also, if action allocation fails and the list stays empty, list result 1
causes the caller to consume a queued item. This potentially surprising
failure behavior follows the original instructions; it is not a retry.
Tests now cover waiting-action order, two successive pops, head replacement
or removal during update, result preservation and this null-allocation path.
Individual action +0x28/+0x24 bodies remain explicit dependencies.

`aoe_building_parent_update_ror` now translates 004b0100's derived prelude.
When template class `+0x14` is 18 and live state `+0x48` is 2, it computes
`owner_resource[35] * world_delta + carried_amount`, stores the result at
live `+0x44`, clamps values at or above the executable's 100.0 constant, and
sets carried-resource type `+0x4c` to 34. It then tail-calls root update
004272e0 on every path. That root update is still a required callback; the
known prelude is no longer hidden inside it. Portable inputs require finite
arithmetic and a present owner resource slot 35.

The adjacent construction query 004af240 is now translated as
`aoe_building_construction_percent_ror`. It returns 100 when live state
`+0x48` is nonzero or signed template build time `+0x15a` is zero. Otherwise
it computes `elapsed_build_time * 100 / build_time`, truncates toward zero as
the executable's x87 helper does, and exposes the low signed 16-bit result
consumed by all three known callers. It does not clamp ordinary over/under
construction values. Portable arithmetic rejects nonfinite/out-of-int64 input.

The translated 004af1a0 establishes that its first action comes from
the distinct live action list at building `+0x184`, not production list
`+0x1cc`. It writes action type `+0x04`, dispatches type 102 to training query
004b6e00 and type 103 to research query 004b7d60, or writes ID -1/progress 0
and returns false for any other type. `aoe_building_active_action_query_ror`
now performs that dispatch directly and production calls it instead of an
opaque query callback. The production caller ignores the ID and progress but
the translated path still invokes the correct query body, like the original.
Portable action records expose the common type as their first semantic field;
this is not an assertion about host or original x86 object layout.

The data-return portion of that research dependency is now source-backed.
`aoe_research_action_progress_ror` translates 004b7d60 without its optional
localized-text copy. State 1 reports 100; other states call translated manager
query 004ed8ef. That manager bounds-checks the signed research ID, returns 100
for manager state 3, zero for states other than 2, and for state 2 truncates
`progress * 100 / DatTech.research_time` to the low signed 16-bit result. The
existing DAT parser supplies the exact research-time field. Building semantic
state now retains both `+0x184` live actions and `+0x1cc` production actions;
tests verify empty, unknown, training and research heads remain distinct.

Next: replace the setter's remaining class/world effect boundaries with
concrete runtime bindings, then bind the existing queue, training-action and
building semantic fields to runtime objects.
Do not wire no-op callbacks to claim a working building simulation.

## Type-80 completion inputs

Disc RoR type-80 state setter `004ae500` maps to patched 1.0a `004acbf0`.
Its state-2 completion branch reads the building tail through template
`+0x178`; these inputs can no longer remain parser skips. `DatUnit` now
retains construction graphic `+0x168`, adjacent mode `+0x16c`, completed
angle `+0x16e`, disappear byte `+0x170`, stack unit `+0x172`, foundation and
old terrains `+0x174/+0x176`, automatic research `+0x178`, and completion
sound `+0x17a`. They are semantic fields with explicit integer widths, not a
packed recreation of the original 32-bit object.

The disc RoR DAT provides an independent boundary check: civilization 1's
Town Center (109) decodes research 105 and completion sound 185; its Wonder
(276) decodes research -1 and sound 133. A synthetic parser test pins every
byte in the 16-byte serialized building tail. The setter's world, terrain,
research, visibility and owner callbacks still require translation before
these values are applied to live objects.

The setter's queue-transition subroutine is now translated as
`aoe_building_state_queue_transition_ror`. It runs only for a requested state
above 2 when the old state is above 1 and the queue is nonempty. Status 4 is
cleared, its unsigned owner counter is decremented, and the complete head
quantity is cancelled. Other statuses cancel `head quantity - 1`, deliberately
leaving one item. A quantity of one therefore passes zero to 004af680: no
refund or removal occurs, but the queue revision increments and event 124 is
still emitted. The portable queue primitive accepts that evidenced zero case
while continuing to reject negative counts; malformed nonpositive stored
quantities are outside the semantic API.

The adjacent type-80 high-state virtual is now translated too: disc
`004ae450` maps to patched `004acb40`. The new
`aoe_live_unit_building_high_state_ror` calls the required building-specific
`+0x258` prelude, directly clears the inherited `+0x184` action list, runs the
translated root high-state transition, invokes the building-manager removal
boundary, and clears semantic live `+0x1e0`. Scenario dispatch now selects
this composition for type 80 instead of accepting an opaque whole-method
callback. Tests pin the observable prelude, action destruction, manager
removal order and final state 7.

The setter's exact state-2 to state-3 branch is now composed as
`aoe_live_unit_building_state_three_ror`. It requires the live and building
semantic views to share a type-80 template and both begin at state 2. In
original order it invokes virtual `+0x258`, removes and clears the building
manager link, applies the translated queue transition, applies the translated
root state setter, and calls disc `004522e0` / patched `00451370` with the
template ID and constants `-1.0, -1.0, 1, 2, 0`. A zero result removes the
optional template research `+0x178`, followed by IDs `0x4a`, `0x4b`, and
`0x4c`. The host test pins that callback order and the two semantic state
views. The separate requested-state-2 composition follows below.

The requested-state-2 completion prefix is now translated through its first
class-specific branch as `aoe_live_unit_building_completion_visual_ror`.
It copies signed DAT train time `+0x15a` to elapsed construction `+0x1d4`,
advances the translated 32-bit RNG, conditionally emits the local-player
completion signal, and then reseeds with the returned 15-bit value exactly as
the executable does. It sets semantic completion byte `+0x1dd` before virtual
`+0x25c` (patched `+0x260`). A disappearing building takes only virtual
`+0x6c`; otherwise the function calls virtuals `+0xdc` then `+0xd8`, selects
standing graphic `+0x18`, stores the low byte of completed angle `+0x16e`,
and conditionally invokes adjacency completion for `+0x16c`. Tests pin both
branches, callback order, and the unusual RNG state transition from seed 1 to
41. The template `+0x164` pointer behind the local-player condition remains an
explicit caller-resolved boundary rather than a fabricated DAT field.

Stack-unit creation, foundation terrain painting/invalidation, automatic
research, event 105, and owner finalization are now translated as the adjacent
`aoe_live_unit_building_completion_world_ror` continuation. Optional stack
unit `+0x172` is spawned at the building's exact XYZ first. For nonnegative
foundation terrain `+0x174`, the four paint edges are x87-style truncations of
`x-size_x`, `y-size_y`, `x+size_x-0.001`, and `y+size_y-0.001`, narrowed to
the signed 16-bit values consumed by the original terrain routine. The map
invalidation rectangle expands each edge by one. Portable inputs must be
finite and fit `int16_t`; malformed coordinates fail before callbacks instead
of invoking C narrowing behavior that could differ between x86-64 and ARMv7.
The continuation then applies optional research `+0x178`, emits event `0x69`
with owner ID, unit ID, and truncated XY, and calls owner virtual `+0x104`
with semantic live value `+0x1d8`. Tests pin all six effects and their order.

`aoe_live_unit_building_state_two_ror` now composes those two completion
pieces and the translated root state setter as the complete requested-state-2
control flow. The intervening queue condition is false for requested state 2,
exactly as in both executables, so it has no queue effect. A focused test pins
the full 12-effect order through owner finalization and confirms the live and
building semantic state views finish synchronized. Class virtuals, stack
construction, terrain mutation, research-manager mutation, notification, and
owner-finalization bodies remain explicit integration boundaries rather than
invented no-ops.

The ordinary visual branch now has a concrete owned-runtime adapter instead
of three opaque virtual callbacks. Type-80 vtable `0054d0c8 + 0xdc` resolves
to leave handler `004ac380`; completion passes alternate mode 1. Slot `+0xd8`
resolves to wrapper `004ae980`, which observes completed byte `+0x1dd` and
enters through `004ac290` with normal radius mode 0. The new
`aoe_live_unit_building_completion_visual_runtime_ror` calls the translated
unit-vision helper with those exact modes, resolves the requested standing
graphic through `DatGraphics`, and uses the owned sprite runtime selector.
The live constructor now also retains the resolved walking graphic in its
semantic selector template. A focused test runs the real visibility and
sprite paths while pinning the remaining local/prelude/adjacency effects.
`aoe_live_unit_building_state_two_runtime_ror` carries that binding through
the world continuation and translated root setter, so runtime callers no
longer need to reconstruct the completion ordering themselves.

The adapter also exposed and fixed the matching constructor invariant.
Incomplete type-80 `+0xd8` forces alternate rectangle mode, so the factory
must register that footprint initially; otherwise completion's rectangle
leave would not match the registered circular cells. The placement binding
now accepts an explicit dynamic-class mode, type-80 construction selects
alternate mode while semantic `+0x1dd` is clear, and destruction selects the
same mode until completion switches the unit to the normal footprint. The
mode-specific placement test verifies all 16 cells of a size-derived
rectangle; the ordinary public placement entry remains mode zero.

Type-80 live units can now acquire valid semantic building state without
hand-written test structs. `aoe_live_unit_bind_building_update_ror` translates
the direct constructor initialization shared by disc `004adcf0`, `004ade50`,
`004adf70`, and initialized body `004ae1a0`: shared template/state, completed
angle, current graphic angle count, signed build time, empty queue storage,
enabled byte 1, status -1, owner resource view/ID, and completion value
`+0x1d8`. It initializes both caller-owned action lists with mandatory real
destroy/state callbacks, points their owner-state bytes at building state,
and binds live actions `+0x184` separately from production actions `+0x1cc`.
The storage remains explicitly caller-owned; it must outlive the live unit and
be cleaned/detached in original order. The completion integration test now
uses this binding instead of fabricating fields.

That lifetime now has a translated teardown boundary rather than a prose-only
requirement. `aoe_live_unit_unbind_building_update_ror` runs the type-80
`004ae110` production-list/queue cleanup first, including caller-supplied
deallocation and status-4 counter maintenance, then clears the distinct
inherited live-action list and detaches both semantic pointers. Preflight
rejects missing list methods, queue deallocation, or status-counter
dependencies before mutation. This makes scenario-owned arrays safe to add
without hiding action destruction behind no-op callbacks.

The owner-finalization virtual is no longer misattributed to the base player
vtable. Gameplay player construction calls base `0045c390/0045c650` and then
overwrites vtable `00549c70` with derived vtable `0054ee68`; its `+0x104`
entry is the two-argument `004f4fc0`, not base entry `004603a0` (a four-arg
GUI rectangle thunk). The derived method conditionally forwards through the
player's `+0x23c` UI pointer to `004ba540`. The new
`aoe_live_unit_owner_complete_ui_ror` translates that routine's data/control
behavior: find the first inactive entry matching template ID and nonnegative
completion key, mark it complete, bind the live object ID, set the Wonder-276
flag, retain the original 63-byte name-copy behavior, and rebuild the summary.
A null UI is the exact headless outer branch and performs no effect. Tests pin
matching, nonmatching, refresh, object-ID, name, and headless behavior.

The state-two owned-runtime adapter now invokes that translation directly.
`AoeBuildingCompletionWorldRuntimeOps` retains only the still-untranslated
stack-unit and terrain boundaries; it takes an optional
semantic owner UI instead of an arbitrary owner-finalization callback. The
full runtime composition test now reaches the recovered first-match UI update
and refresh after its concrete vision and sprite transitions.

Automatic building research now uses the structured translation of manager
method `004ecd0d` rather than one opaque runtime callback. The new
`aoe_research_complete_ror` bounds-checks the signed ID, accepts only manager
states 0..2, writes completed state 3, applies technology effects, rebuilds
research availability, and signals Wonder stages 0..2 for IDs 101..103 in
the executable's order. The remaining effect/rebuild bodies are explicit
manager dependencies. Focused tests pin invalid-state rejection, ordinary
completion, Wonder mapping, and callback order; the building runtime test
also proves automatic research reaches state 3 before event 105 and owner UI
finalization.

The apparent event-105 boundary is now resolved too. `DAT_00585e88` is the
application singleton constructed at `00416950` with vtable `00547de8`; its
`+0x40` entry is `00417910`, an unconditional return. The owned runtime binds
this verified no-op directly, so completion no longer requires an invented
event callback. The generic callback composition remains available for
source-level tests and alternate integrations.

The stack-unit factory now has a concrete owned-scenario binding.
`aoe_scenario_runtime_building_spawn_stack_ror` selects the completing
unit's semantic owner, resolves the requested ID from that player's cloned
template table, and uses the same recovered live constructor and owned
lifetime as the existing template `+0x18` path. It deliberately ignores a
failed/null creation result, as `004ae500` does. The real-scenario test pins
the spawned unit's owner, cloned template, constructed count, sprite tick,
and eventual teardown.

Foundation invalidation is now concrete in the owned map. The translated
`aoe_runtime_map_invalidate_ror` implements all of `00445760`: normalize
reversed inclusive bounds, reject wholly disjoint rectangles, clamp partial
overlap, write cache byte `+7 = 0xcc`, and write draw byte `+10`. The scenario
binding supplies completion's zero draw value. Cache byte `+7` is retained in
its own byte array rather than pretending the host has the original packed
24-byte x86 tile layout. The deterministic shell of painting is now
`aoe_runtime_map_paint_terrain_ror`: target availability, signed-16 bound
handling, per-elevation-shape frame admission, terrain writes, one-cell border
recomputation, and internal invalidation match `00449110`. Its mandatory
per-cell transition hook preserves the remaining `004481b0` behavior. The
terrain-unit IDs, densities, placement modes, and used count were confirmed
already parsed from disk offsets `+254/+314/+374/+404`; remaining work is the
live-object removal/state transition and spawn behavior.

That per-cell body is now translated as
`aoe_terrain_transition_cell_ror`. It captures each tile-list successor before
dynamic removal, filters only the terrain owner and template IDs outside the
original exclusion interval, scans the requested terrain-unit filter, and
then applies the newly painted terrain's unit IDs, densities, and placement
modes. Mode 0 consumes two RNG values for offsets using the executable's
`1/32768` constant; mode 1 uses the tile center; density below 1000 consumes
the probability RNG value using integer division by 32767. Placement and
creation remain dynamic template callbacks, as does the class-specific
removal method. Tests pin removal during traversal, callback order, exact
coordinates, and the resulting 32-bit RNG state.

The removal side is now bound to owned scenario units. Runtime lookup first
verifies the map-list core pointer belongs to a loaded or dynamically created
unit, exposes its semantic owner/template IDs, then dispatches the already
translated root `+0x68` high-state path with the correct player resources,
registration, visibility, and world bindings. Transition removal and creation
callbacks now report failure so unsupported derived classes cannot be treated
as successful no-ops. The real `EXPANS_1_1.scx` integration test finds an
actual Gaia terrain-0 decoration, verifies its metadata, and transitions it
to state 7 through this binding before the full sprite tick and teardown.

`dat_tool --terrain-units` provides a durable source-data audit. In the
installed RoR DAT, terrain 0 has 28 entries (mostly type-10 decorations plus
type-90 trees), while foundation terrains Crop/Rows/Wheat (7/8/9) have zero
spawn entries. This narrows building completion without weakening the general
translated density/spawn path.

The transition's remaining placement and creation boundaries are now bound to
the owned scenario runtime. Placement selects Gaia's cloned template and its
restriction row from the retained DAT intro, then calls
`aoe_unit_placement_check_ror` with 004481b0's verified arguments: visibility
off, shape mode 1, reserved zero, clearance on, overlap exception off, and
unit collision on. Creation enters the existing owned live-unit factory as
Gaia at Z zero. `aoe_scenario_runtime_paint_foundation_ror` composes these
bindings with terrain writes, object displacement, general terrain-unit spawn
rules, border recomputation, and map invalidation. Parsed intro, terrain, and
border tables remain caller-owned and must outlive the scenario runtime.

The real campaign regression repaints terrain 0 under an actual Gaia type-10
terrain-0 decoration, observes its state-7 removal, then paints terrain 7 and
checks the exact tile cache/draw bytes. This ordering matters: 00449110 writes
the target terrain before 004481b0 consults its terrain-unit table, and the
installed foundation terrains have no entries. A separate DAT-selected
terrain-decoration creation keeps the general Gaia factory path covered. The
scenario now constructs and ticks 3,812 live units.
