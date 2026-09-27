# Original Rise of Rome Wonder construction stages

Verified from the local executable on 2026-09-26. This supersedes the old
recipe's undocumented illustrative percentage thresholds.

Executable: `build/ror/disc/GAME/EMPIRESX.EXE`
SHA-256: `8e0e6d047b2f90d0769fb5a1b16c1b1973582efdbe73a5d11395a43557215845`

DAT: `build/ror/disc/GAME/DATA2/EMPIRES.DAT`
SHA-256: `d47c59d9ecaf5b83b467c647a69d28eced6a0dc5b2fa873e930c86136df5eb9d`

`0x004ae1a0` initializes a live building and selects template `+0x168`
(construction graphic) while state is 0. The accumulated build time is live
`+0x1d4`. `0x004ae880` advances its displayed construction angle:

```text
004ae893 mov di, WORD PTR [eax+0x60]        ; construction angle count
004ae897 movsx edx, WORD PTR [ebx+0x15a]   ; template train/build time
004ae8a5 fild DWORD PTR [esp+0xc]          ; angle count
004ae8ad fmul DWORD PTR [esi+0x1d4]        ; accumulated build time
004ae8b3 fidiv DWORD PTR [esp+0xc]         ; template build time
004ae8b7 call 0x005283e0                  ; truncate toward zero
004ae8c1 cmp cx,di                       ; only set if < angle count
004ae8cb mov BYTE PTR [esi+0x35],al       ; selected construction angle
004ae8ea mov eax,DWORD PTR [esi]
004ae8ec push 2                          ; at completion, state 2
004ae8f0 call DWORD PTR [eax+0x5c]
```

`0x005283e0` sets x87 rounding bits to truncate before `fistp`. The
type-80 state setter `0x004ae500` selects standing graphic `template+0x18`
and completed angle `template+0x16e` when switching to state 2.
`0x004ee630` reads the construction graphic and building fields from DAT.

The Roman Wonder (civilization 13, unit 276) has construction graphic 707,
standing graphic 900, completed angle 0, build time 8000, size 2.5/2.5.
Graphic 707 is `Wonder Con`, SLP 302, one animation frame and three angles.
Graphic 900 is `RWonder`, SLP 735, one frame and one angle.

| Actual progress | Display |
| --- | --- |
| 0 <= p < 100/3 | SLP 302 frame 0 |
| 100/3 <= p < 200/3 | SLP 302 frame 1 |
| 200/3 <= p < 100 | SLP 302 frame 2 |
| p = 100 | SLP 735 frame 0, completed Roman Wonder |

For integer percentages this is 0–33, 34–66, 67–99, and 100. It is a
discrete three-stage foundation, not a continuously growing completed
building. The status renderer uses exact decimal arithmetic for the
percentage rule; it does not emulate tick timing or x87 rounding at the
instant of a boundary crossing.

The Wonder's foundation-terrain field is -1: the dirt in this status scene
is deliberately painted scene terrain, not a claimed automatic DAT terrain
replacement. Its geometry matches the five-by-five footprint. Builder count
is a separate project-effort metaphor, not an original construction rule.

Recheck with:

```sh
objdump -d -Mintel --start-address=0x004ae880 --stop-address=0x004ae8f3 build/ror/disc/GAME/EMPIRESX.EXE
objdump -d -Mintel --start-address=0x005283e0 --stop-address=0x00528407 build/ror/disc/GAME/EMPIRESX.EXE
./build/host/dat_tool build/ror/disc/GAME/DATA2/EMPIRES.DAT --unit 13 276
./build/host/dat_tool build/ror/disc/GAME/DATA2/EMPIRES.DAT --graphics-all
```
