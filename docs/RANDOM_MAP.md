# Random-map generation

Status: **records retained, generator not translated.** The DAT random-map
records are now decoded and kept by `dat_parse_random_maps`. The generator
itself still needs raw disassembly before it can be translated (see
[Blocker](#blocker-x87-expressions-missing-from-the-exports)).

Addresses are patched Rise of Rome 1.0a unless marked base 1.0c. Vtable slot
targets come from `build/decompile/ror_10a_exec_pointers_after_islands.tsv`
(data-to-code pointers); everything else is from the checked-in Ghidra exports.

## Entry and seeding

`0x00446550` (base 1.0c `0x00448150`) generates a random map:

1. Seed = global `+0x2c`, or `rand()` when that is -1; stored to `+0x34` and
   logged as "Random Map Seed".
2. `srand(seed)`, then base map creation `0x00444580`.
3. `srand(seed)`, then controller vtable `+0x14` (setup).
4. `srand(seed)`, then controller `+0x00` (generate) and `+0x18` (cleanup).
5. Logs `rand()` as "After Random Map Generation".

`srand`/`rand` are the MSVC CRT pair `0x00526053`/`0x0052605d` modeled by
`aoe_rng`. The three reseeds mean setup and generation each start from the
same seed state.

## Classes

| Vtable | Constructors | Slots | Role (evidence) |
| --- | --- | --- | --- |
| `0x00545cc8` | `0x0046bd00`, dtor `0x0046bdb0` | `0x0046be30` | Module-list base. `0x0046be30` insertion-sorts the list at `+0x08` by float priority `0x0046be20`, then runs each module's slot 0 ("RGE_RANDOM_MAP_MODULE" log strings). |
| `0x00545cdc` | `0x0046da10` (DAT), `0x0046d580` (text), dtor `0x0046dbe0` | `0x0046de20` generate, `0x0046e3a0` land setup, `0x0046ead0` terrain setup, `0x0046ec90` object setup, `0x0046ede0` elevation setup, `0x0046df30` setup driver, `0x0046df90` cleanup | Controller base. Writer `0x0046dcc0` (called by `0x00446a20`) serializes the same layout. |
| `0x00545d18` | `0x00471d20` (DAT, from `0x004ed8a0`), `0x00471cf0` (text, from `0x004ed900`) | `0x00472d80`, `0x00471f60`, `0x00472370`, `0x00472520`, `0x00472c20`, `0x00471d40`, `0x00471f50` | RoR controller. Each slot wraps the base slot with adjustments for particular map indices. |
| `0x00545d04` | `0x0046fae0` | `0x0046fb20` | Module given controller `+0x3c` (land work). |
| `0x00545d4c` | `0x00472df0` | `0x00472e30` | Module given controller `+0x1474` (terrain work). |
| `0x00545d10` | `0x004704d0` (also from `0x0050aaa0`) | `0x00470640` | Module given controller `+0x270c` (object work) and `+0x30`. |
| `0x00545d00` | `0x0046f030` | `0x0046f070` | Module given controller `+0x3fd4` (elevation work). |
| `0x00545cd8` | `0x0046cdb0` | `0x0046cdf0` | Extra module created by RoR setup `0x00471d40` after `rand() >= 0x2000`, with counts from the map size (1/4/6/8/9/10 up to sizes 72/96/120/144/200/250) and map index `+0x34`. |
| `0x00545d14` | `0x004716d0` | `0x00471710` | Created by `0x00472cb0` (logged as shallows initialization). |

Base generate `0x0046de20` constructs the land, terrain, object and
elevation modules, then runs `0x0046be30`. RoR generate `0x00472d80` calls
it and then clears the low five bits of tile byte `+5` wherever they equal
`0x10`.

Setup driver `0x0046df30` stores an argument at `+0x30` (later passed to
the object module), the selected map index at `+0x34` (low byte, used to
index the record table) and the player count at `+0x38` (the per-player land
multiplier), then calls slots 1–4 in order. The generator's dimensions are `+0x14` (x) and `+0x18`
(y).

## Record layout and consumers

`0x0046da10` reads `count` and a pointer placeholder, then all 72-byte map
headers. For each map it then re-reads header `+0x04..+0x2f` (nine settings,
land count and pointer), the lands, and a count/pointer pair plus records for
terrains, objects and elevations. Only header `+0x00` survives from the first
pass. Base 1.0c `0x0046e5e0` is identical, with the table at object `+0x479c`
(RoR `+0x4f58`). Counts are signed: a nonpositive count reads nothing. The
portable parser is stricter: it requires both passes to agree and rejects
counts that overrun the blob.

Text loader `0x0046d580` gives the field widths. Byte fields are read with
`%d` and narrowed. It also supplies defaults the DAT stores explicitly: land
`+0x28 = 8` and terrain `+0x14 = 20`.

Header (`0x0046e3a0` and wrappers, both revisions):

| Offset | Use |
| --- | --- |
| `+0x04..+0x10` | Edge margins, stored per land (work `+0x5c..+0x68`) as x in `[+0x04, gen_x - +0x0c]` and y in `[+0x08, gen_y - +0x10]`. |
| `+0x14` | Low byte copied to every land work entry `+0x6c`. |
| `+0x18` | Edge distance passed to placement `0x0046dff0`. RoR `0x00471f60` temporarily uses 25 (x size >= 80) or 20 for map indices 5 and 8 when the x size is below 100. |
| `+0x1c` | Low byte copied to generator `+0x145c`. |
| `+0x20` | Percent of the map area given to lands. It also scales the fixed elevation layers. |
| `+0x24` | Not read by either revision's generator. |

Land record (44 bytes):

| Offset | Use |
| --- | --- |
| `+0x00` (int) | Copied into the object setup's land table. |
| `+0x04` (byte) | Zero-extended into land work `+0x40`. |
| `+0x08`, `+0x0c` (int) | Land work `+0x50`, `+0x4c`. |
| `+0x10` (byte) | Land work `+0x54` when `+0x1d` is 0 or 1. Other per-player modes store the player index instead. |
| `+0x11` (byte) | Placement mode in `0x0046dff0`: 0 random within the edge distance, 1 fixed at `+0x14/+0x18`, 2 random with spacing and a central exclusion box. |
| `+0x14`, `+0x18` (int) | Fixed position for mode 1. |
| `+0x1c` (byte) | Percent of the total land area. Per-player lands split it by player count. |
| `+0x1d` (byte) | 0 makes one land; nonzero makes one per player. |
| `+0x20`, `+0x24` (int) | When `+0x20 >= 0`, the land's position and these two values are appended to position lists for terrain (`+0x20d8`) and elevation (`+0x4920`). |
| `+0x28` (int) | Land work `+0x58`. |

Terrain records (24 bytes) feed `0x0046ead0`: `+0x04..+0x14` are copied to
work entries. Object records (44 bytes) are copied unchanged by `0x0046ec90`.

Elevation records (24 bytes) are loaded, saved and freed, but nothing in
either revision reads them. Of the functions that reference the record table
(RoR `+0x4f58`, base `+0x479c`), only the DAT loader, the writer and the
destructor touch its `+0x40/+0x44` fields. Elevation setup
(`0x0046ede0`, base `0x0046f960`) instead installs four fixed layers of
10/8/6/4 percent of the header `+0x20` land area.

## Blocker: x87 expressions missing from the exports

Ghidra emits bare `__ftol()` calls when the operand is left on the x87
stack. The value being truncated is not in the export. There are 25 such
sites across the generator:

| Function | Sites | Function | Sites |
| --- | ---: | --- | ---: |
| `0x0046dff0` placement bounds | 4 | `0x00470e20` | 2 |
| `0x0046ead0` terrain setup | 1 | `0x00471180` | 2 |
| `0x0046f3b0` | 1 | `0x00471350` | 2 |
| `0x0046f620` | 1 | `0x00471f60` land setup | 1 |
| `0x0046fbb0` | 1 | `0x00472370` terrain setup | 1 |
| `0x004709d0` | 2 | `0x00472520` object setup | 2 |
| `0x00470c10` | 2 | `0x00472c20` elevation setup | 1 |
| `0x00473260` | 1 | `0x00473500` | 1 |

Each value feeds a range, count or position, and several sit next to
`rand()` calls whose order matters. Translate these functions only with
`objdump -d -Mintel` of the patched executables at hand. Record each x87
expression, including the float constants at `0x00545cf8`/`0x00545cfc` and
similar addresses, and keep the CRT `rand()` call order exact.

## Next steps

1. With local executables: recover the 25 expressions, then translate the
   module scheduler (`0x0046be30`/`0x0046be20`), placement `0x0046dff0`
   and land setup `0x0046e3a0`, with RNG-order tests.
2. Translate the land, terrain, object and elevation modules and the RoR
   wrappers.
3. Capture original generated maps for fixed seeds and sizes as reference
   traces. No such reference exists in this repository yet.
