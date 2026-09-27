# Frozen accepted footprint recipe

Saved before replacing the status renderer on 2026-09-26. These files retain
the exact shell recipe, decoder source/binary, and 960x544 10% PNG used for
the corrected five-by-five footprint image. Do not edit this snapshot.

| File | SHA-256 |
| --- | --- |
| `render_status_art.sh` | `8624725f6467d7e0ae4e0aec4ec5f16246fb842dd3e2d557951a0034d62d23c0` |
| `port-progress.png` | `986d1e625b2f8c401e3f9a54a85c8093c8e7f708ec19cb7ed67f03d80e4a2c94` |
| `slp_tool.c` | `5fcfeb4e7c297a74580259f55d1b01052d2db7fc0695ab3ab1a39d9e23df0ba9` |

Original image command: `bash tools/render_status_art.sh 10`.
Environment: Linux x86-64, ImageMagick 6.9.12-98 Q16, DejaVu Sans fonts,
original game DRS paths embedded in the archived script.

Replay safely without replacing current source or published artwork:

```sh
python3 tools/replay_status_v4.py
```

The helper executes the preserved recipe with only the project-root and
decoder paths redirected to this snapshot. It writes
`build/status_work/replayed-v4.png` and checks the result against the saved
PNG's SHA-256. Future images use `status/current.json` and the new renderer.
