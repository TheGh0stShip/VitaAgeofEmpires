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
simulation. Production still depends on the real query, cancellation,
action factory, individual action-update and global-notification implementations.

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

Next: inspect 004af680, 004b6820, the production action
virtual methods, and the type-80 setter 004ae500;
translate their bodies and bind semantic building fields to runtime objects.
Do not wire no-op callbacks to claim a working building simulation.
