# Pocket PC editions (research reference)

The locally supplied `Windows Mobile AoE/` folder (git-ignored, never
committed) contains Microsoft/ZIO Interactive Pocket PC releases of Age of
Empires. The executables credit Ensemble Studios' "Genie engine technology"
and say the "PocketPC version [is] based on PC game developed by Ensemble
Studios". They also credit FloodGate Entertainment, ZIO, and Firelight
Technologies (FMOD). They are ARM builds of the same engine lineage.

## Identified builds

`tools/extract_pocketpc.sh` reproduces these from the InstallShield
packages. It needs `7z` and `unshield`, and writes its output under `build/`.

| Build | Source | ARM image | PE time | Size | SHA-256 |
|---|---|---|---|---:|---|
| Gold Edition 1.07 | `.../Age of Empires Gold Edition (ARM)_1.07/Disk1` | `00aoeppc.001` | 2002-12-09 | 1,187,840 | `ee4fe16186849e94ca88f1d2bab62e3d203823c4a6f950f95bd049b6b3829b2a` |
| 1.25 | `AOE_Setup.exe` (`17ecf184...2ce29`) | `00aoeppc.001` | 2003-10-22 | 1,183,232 | `54c57c6729eeea09ef4df33eaba4be9f11bf21cf322927228d95e0741992e1db` |

- Both images are PE32 Windows CE, machine 0x1c0 (ARM, little-endian).
  They ship `00fmodce` (FMOD CE, SHA-256 `ae27997b...8318c`) and the GAPI
  display DLL `000000gx` (`b7fb4114...86d32`). No debug symbols or source
  paths survive.
- The data differs from the PC release. For example, the Tribe AI combat
  table is a plain-text `RULES.RPS`, and campaign and scenario scripts are
  small text files.
- The standalone Gold 1.07 `.exe` wrapper reports a 7-Zip data error. Use
  its unpacked `Disk1` folder instead.

## Use policy

- The PC targets remain authoritative: patched AoE 1.0c and Rise of Rome
  1.0a. A Pocket PC observation is evidence only about the Pocket PC
  revision it came from. It never establishes PC behaviour, and Pocket PC
  addresses are a separate revision address space.
- Allowed uses: locating subsystems and data flow to verify against the PC
  executables, comparing structure layouts, and studying what an ARM port of
  this engine changed (fixed memory budgets, screen size, stylus input,
  FMOD audio). These are design questions for the Vita platform layer.
- Its code is proprietary. Do not copy or translate it into this
  repository, and do not commit its binaries, data, or extracted output.
- The supplied folder includes a `Crack/` directory and registration
  details. They are out of scope: this project does not use, analyse, or
  distribute protection-circumvention material.

## Next steps

1. Import `00aoeppc.001` (Gold 1.07) into a separate Ghidra project as ARM
   v4 little-endian Windows CE, keeping its addresses apart from the PC
   projects.
2. Use string, constant and data-table anchors (for example the RPS combat
   table) to find the Pocket PC counterparts of PC functions already
   translated here. Record each correspondence as a cross-revision lead
   that the PC executables must still confirm.
