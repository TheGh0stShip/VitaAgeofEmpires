# Rise of Rome training action update

Source: `build/ror/disc/GAME/EMPIRESX.EXE`, SHA-256
8e0e6d047b2f90d0769fb5a1b16c1b1973582efdbe73a5d11395a43557215845.
`src/train_action.c` translates activation 004b68c0–004b6995 and head update
004b69a0–004b6b35, plus inherited waiting update 004073d0–00407402. These
are not reconstructed training timers. The portable action is bound to the
translated building-production/action-list interfaces by `train_action_adapter`;
real runtime entity/template/world resolution remains a caller-owned boundary.
The adapter has an explicit edition selection: Rise of Rome uses its prepaid
constructor/activation/update path, while base-game uses the independently
recovered constructor, always-charge activation, and update path. This is a
binding decision, not a claim that the RoR building-production function itself
also matches the base executable.

Dispatch evidence: production calls constructor 004b6820, which installs
vtable 0054db00, type 102, unit ID +40, zero float progress +44, request
ID +48, and flag +4c. Vtable +28 points to 004b69a0; +5c points to
004073c0, a direct store to action state byte +0c. Waiting slot +24 points
to 004073d0, which performs target-lifetime checks, not a no-op. That method
is now translated. The recovered constructor/destructor ownership rules now
also have portable semantic allocation, described below.

## Portable constructor and teardown

Original factory 004b0020 allocates 12 bytes and calls 004b5ac0, which calls
inherited constructor 00403d70 with the building. Training constructor
004b6820 first invokes inherited 00406f50/00407110, then installs vtable
0054db00, action type 102, unit ID +40, progress 0 at +44, request ID +48,
and prepaid byte +4c. The inherited constructor zeroes state and target
pointers, sets cached target IDs +18/+1c to -1, and obtains an owned action
list through virtual +0xc into +34.

`aoe_train_action_create_ror` allocates a semantic action and that owned
list, initializes these recovered values, and binds resolved building,
player, resource, and template data. It does not reproduce the 32-bit object
ABI or vtable. Matching the original, internal-list allocation failure stores
NULL without failing action construction. Invalid callback bindings are
rejected before allocation.

Derived destructor 004b6860 jumps to inherited 00406f80.
`aoe_train_action_destroy_ror` preserves its material ownership order: set
primary target NULL, set secondary target NULL, clear/free the owned action
list, then free the action. Target detachment uses the translated cached-ID
setters. A missing portable dependency returns failure while the action stays
owned. Tests construct a heap action, populate its list, attach both targets,
and verify detach/destruction order under ASan/UBSan.

## Waiting update

`aoe_train_action_waiting_ror` translates the inherited vtable +0x24 method.
It reads primary target +0x10; if nonnull and target state byte +0x48 is at
least 7, it calls action virtual +0x54(NULL). It then independently rereads
secondary target +0x14 and applies the same test through +0x58(NULL). It
always returns byte 1. Primary cleanup can therefore alter or clear the
secondary before the second test, and the portable method preserves that
order. The waiting method invokes the translated +0x54/+0x58 setters
directly; their registry/attach/detach operations remain explicit
dependencies. Tests cover live/dead targets, callback ordering, and a
primary detach callback that clears the secondary before it is reread.

## Target setters

`aoe_train_action_set_primary_ror` and `_secondary_ror` translate 00407460–
0040753c. If the live pointer already equals the requested target, they
return without consulting dependencies. Otherwise, a nonnull old pointer
causes world-registry lookup 005230d0 using cached ID +18/+1c; a registry
hit receives target virtual +0xc4(building). A registry miss is valid.
The method then rereads the live pointer because detachment may change it.
If it now equals the requested target, no assignment or attach occurs.

Otherwise it writes the requested pointer. NULL writes cached ID -1. A
nonnull target caches target +4, then calls target +0xc0(building, action
type +4, 0). Training action type is 102. The methods differ only in their
pointer/ID slots. Tests cover replacement, same-pointer no-op, NULL,
registry miss, detach/write/attach order, and detach satisfying the second
comparison. Target reference ownership remains a real integration boundary,
not a silent pointer assignment.

World lookup is no longer a callback. `aoe_live_unit_registry_lookup`
translates 005230d0 against caller-owned semantic positive and negative
arrays. Nonnegative ID `n` selects positive slot `n`; negative ID `-n`
selects negative slot `n`, so ID -1 deliberately uses negative slot 1.
Bounds, null arrays, invalid negative counts, and `INT32_MIN` return NULL;
the last avoids the original x86 `NEG` overflow on corrupt input. Target
setters bind this registry through the action and treat a valid lookup miss
exactly like the executable. Dedicated tests cover both ID spaces and all
boundaries.

## Activation

`aoe_train_action_activate_ror` follows vtable +0x1c. Argument zero returns
without touching state or validating any other dependency. With argument
nonzero, a negative unit ID calls owner +0x10c(0,unit ID,building ID,request
ID) and returns without changing state. A nonnegative ID whose player unit
template slot is null follows the same path. The semantic port represents
that live slot lookup with `trained_template == NULL`.

For a present template, constructor flag +4c controls payment. Nonzero skips
payment and enters state 6. Zero invokes owner 004f3ae0(template,1.0,0).
Payment success enters state 6. Payment failure calls owner +0x10c, signals
building +0x148 with code 505 and constant 619, then enters state 1—in that
order. Unlike head-update placement failure, activation payment failure has
no global event 104 and no refund.

Production currently constructs this action with +4c = 1, so its normal
queued path is prepaid; the zero case remains necessary for other original
construction/load paths. Actual charging stays an explicit boundary until
the complete 004f3980 resource policy is translated and shared.

Recheck the authoritative bytes:

```sh
objdump -d -Mintel --start-address=0x004b6820 --stop-address=0x004b685f build/ror/disc/GAME/EMPIRESX.EXE
objdump -s --start-address=0x0054db00 --stop-address=0x0054db60 build/ror/disc/GAME/EMPIRESX.EXE
objdump -d -Mintel --start-address=0x004b68c0 --stop-address=0x004b6b36 build/ror/disc/GAME/EMPIRESX.EXE
```

State 1 immediately returns 1. States other than 6 return 0. State 6 adds
building-template work rate (+e4) times owner-world delta (+84) to progress,
storing a float. Global +9a8 instead assigns exactly 1000. The trained unit's
signed template train time (+15a) is compared with that stored value.

On completion state becomes 1 *before* placement/create routine 004b6b40.
If placement fails: owner 004f3b90(unit ID), global event 104 with zero
coordinates, owner +10c(0,unit ID,building ID,request ID), then building
+148(building ID,building ID,505,619,0,0). On success: global event 102 with
building x/y truncated toward zero, owner +110(unit ID,building ID,request
ID,created unit ID), then the same building signal with code 506. Both return
1. A subsequent update sees state 1 and does not repeat these effects.

## Spawn perimeter search

`aoe_train_action_spawn_ror` directly translates 004b6b40–004b6d75, and
head update calls it rather than an abstract spawn callback. Initial
half-width/half-height are building size plus trained-unit size plus 0.01;
full spans are twice those values. Each of three rings tries 60 candidates.
Attempts 0–39 use the executable's signed multiply/high-word reduction of
one original RNG result; attempts 40–59 add 2 to the reduced direction.
Directions 0–3 select the four perimeter edges, then consume another RNG
value to sample along that edge using exact float constant 0x38000100. The
theoretically reachable unmapped direction 4 retains the prior candidate,
matching the original switch table.

Every candidate calls trained-template virtual +0x20 with original placement
arguments `(owner,x,y,0,1,1,1,0,0,1)`. Nonzero means blocked. After 60
blocked probes, each half-span expands by twice the trained-unit size and
each full span by four times that size. After three blocked rings (180 probes)
it returns result 0 without creating. A zero collision result calls trained-
template +0x18 at the accepted coordinates and returns its live unit ID.

The RNG is the translated 00528420 `AoeRng`. Tests verify the exact first
seed-1 candidate `(11.0691986, -0.49000001)`, two RNG advances, accepted
creation, all 180 failures and ring expansion, and callback failure. Collision
and live-unit creation remain explicit runtime boundaries; no position or
unit is fabricated.

`aoe_train_action_refund_ror` now directly translates 004f3b90. Templates
below type 70 do nothing. Otherwise it examines all three six-byte DAT cost
records at +148: nonnegative resource type and nonzero **low byte** of paid
adds the signed amount to that owner resource. The portable function first
validates all referenced resource slots, preventing a partial refund caused
only by an invalid binding; valid-game arithmetic/order is unchanged.

## Base-game correspondence

The base executable has now been independently checked rather than assumed
from the expansion: constructor is `004a50c0`, derived serialization is
`004a5190`, activation is `004a51e0`, update is `004a52c0–004a5443`,
perimeter spawn is `004a5450–004a566e`, cancellation is `004a5690–004a56f3`,
and the nearby progress helper begins at `004a5700`. Both editions retain
action offsets `+40/+44/+48`, states 1 and 6, event sequence 102/103/104,
and signals 505/506. **Base has no `+4c` prepaid byte**: activation always
charges, cancellation refunds whenever it changes a non-state-1 action, and
serialization writes only the 2/4/4-byte derived fields. RoR alone owns the
prepaid byte and its final one-byte serialization field. Consequently,
`aoe_train_action_create_base` forcibly clears the portable RoR-only field
before it calls the shared semantic allocator; a base caller cannot silently
create a prepaid training action.
Base uses signed `idiv 0x7fff` to reduce its doubled RNG sample in the spawn
switch; RoR uses a reciprocal multiply/high-word sequence. Both produce the
same 0–4 edge-case contract: the test exhaustively compares all 32,768 legal
RNG results with and without the late-attempt adjustment. The portable
semantic update/spawn/progress entry points intentionally share tested logic
with RoR. Base activation, cancellation, and serialization have distinct
portable implementations and tests. No claim is made that either edition has
a complete live unit factory yet.

## Cancellation

`aoe_train_action_cancel_ror` translates training vtable slot `+0x2c`, RoR
`004b6d90–004b6dfc`. If action state is not 1, it calls vtable `+0x5c`
(`004073c0` for this vtable, the inherited direct state-1 store) and refunds
the queued unit through `004f3b90` only when constructor byte `+0x4c` is zero.
It always then calls the global event dispatcher with event 103, owner ID,
unit ID, and zero coordinates, followed by building `+0x148` with code 505
and constant 619. A state-1 action therefore still sends the two cancellation
notifications but cannot refund twice. The semantic API keeps global and
building dispatch explicit callbacks; its test verifies unpaid, prepaid, and
already-complete paths and exact ordering under ASan/UBSan.

## Queue progress query

The adjacent training-action helper `004b6e00–004b6e89` (not a
`0054db00` vtable slot) is now represented by
`aoe_train_action_progress_ror`. It writes unit ID first. State 1 then writes
the literal 100 without reading a template. Other states calculate
`progress * 100 / signed train_time` and use helper `005283e0`: its x87
control-word change makes `FISTP` truncate toward zero, not round to nearest.
The semantic return preserves the low 16 bits stored by `mov [out],ax`.
The final optional text-buffer path calls unrecovered `00528540` and is
intentionally not claimed or modeled; the data-return path is independently
test-covered for fractional, negative, state-1, and invalid bindings.

## Derived serialization

Vtable `0054db00 +0x14` selects `004b6870–004b68bc`. It first delegates to
inherited serialization `00407220`, then passes action `+0x40` for 2 bytes,
`+0x44` for 4, `+0x48` for 4, and `+0x4c` for 1 to stream helper `0046cc40`.
`aoe_train_action_serialize_ror` preserves that order through an explicit
base-serialization and bounded-write interface. The test asserts the exact
11-byte derived payload. Deserialization and the full original stream object
remain separate recovery work; this is only the directly evidenced write path.

The portable semantic object binds original IDs, positions, owner resources
and DAT templates; it does not claim to instantiate the original action.
Placement/create and notification boundaries must be supplied by real
translated implementations. Test callbacks only verify branch/order/argument
behavior.
Tests include normal accumulation, exact completion, both spawn outcomes,
no repeated completion, fast mode overwriting higher progress, fast mode
not reaching a >1000 train time, irrelevant states and invalid dependencies.

Finite float inputs and signed-32-bit notification coordinates are supported.
Long-double intermediate arithmetic approximates the original x87 calculation
without claiming all precision-control modes or corrupt NaN behavior on ARM.
The collision and creation routines behind full unit placement are still
required, so this is not a functioning training system yet.

## Building-production bridge

`aoe_train_action_adapter` is the narrow bridge from the translated
`004af900` building queue to `004b6820`/`004b68c0`/`004b69a0`. Its `prepare`
callback resolves an actual building's templates, owner resource storage,
coordinates, owned-action callbacks, and optional live-unit registry into an
`AoeTrainActionCreate` record. It then overwrites the constructor arguments
with the exact queued path values: queued unit ID, request ID `-1`, and
prepaid byte `1`. The adapter exposes `AoeActionListOps` and
`AoeActionTickOps`, so the existing translated list calls activation on
insertion, ticks the training head, invokes waiting on successors, and tears
the action down on pop. Dependency failures are recorded and are observable
through `aoe_train_action_adapter_ok` rather than treated as a successful
game action.

`train_action_adapter_test` drives this complete connected path: one building
queue entry creates the prepaid action, activates it, advances it to
completion, invokes the real RNG perimeter branch and accepted unit-create
callback, sends completion signals, destroys the action, and consumes the
queue entry with original event 124. It is an integration proof for the
translated control flow, not a claim that a live game world can yet create a
unit. Next bind `prepare`, collision, live-unit creation, event dispatch, and
target attachment to the runtime world, then translate persistence/state
loading for action objects. Preserve the recovered RNG/placement order; never
fabricate a successful placement or live unit.
