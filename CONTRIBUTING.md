# Contributing

This is a source-backed reconstruction project. Changes should move toward a
faithful, playable PS Vita port of the original game, not add placeholder
gameplay or diagnostic prototypes.

## Reverse-engineering changes

- Record the exact executable revision and address. AoE 1.0c and Rise of Rome
  1.0a addresses are not interchangeable.
- Preserve evidence: raw instruction boundaries, references, relevant data
  layouts, control flow, and any runtime observations. Separate confirmed facts
  from hypotheses.
- Do not create functions from pointer values alone. Switch-table case labels,
  interior pointers, shared tails, and jump-table entries require boundary and
  control-flow analysis.
- Keep generated Ghidra databases and decompilation exports under `build/`;
  regenerate them with the documented scripts rather than committing caches.

## Portable implementation changes

- Translate observed original behavior and keep platform-independent logic in
  the shared C libraries. Avoid Windows APIs and test-only game behavior in the
  runtime path.
- Add host regression tests for new behavior and update the evidence notes.
- Keep code warning-clean under the existing CMake configuration.
- Do not add proprietary executables, disc images, data archives, music, or
  extracted game assets. Tests should use synthetic fixtures unless original
  files are locally available and legally supplied by the user.

## Build and test

```sh
cmake -S . -B build/host
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure
```

Vita cross-builds require VitaSDK. Some real-asset audit tests are enabled only
when their input files exist locally; absence of those tests is not evidence
that original-game behavior has been validated.

## Pull requests

Describe the source revision and behavior addressed, cite the evidence path or
binary address, list tests run, and call out remaining unknowns. Do not describe
Ghidra's successful decompilation of discovered functions as complete game
decompilation.
