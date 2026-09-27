#!/usr/bin/env python3
"""Verify inputs and replay one status/history snapshot, preserving the original."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys


def digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("--output", type=Path,
                        default=Path(__file__).resolve().parents[1] / "build/status_work/replayed.png")
    args = parser.parse_args()
    snapshot = args.snapshot.resolve()
    manifest = json.loads((snapshot / "manifest.json").read_text())
    recipe = snapshot / "recipe"
    expected = [(item["path"], item["sha256"]) for item in manifest["inputs"].values()]
    expected += list(manifest["render"]["fonts"].items())
    expected += [(recipe / "build/host/slp_tool", manifest["decoder_sha256"]),
                 (recipe / "tools/render_status_art.py", manifest["renderer_sha256"])]
    for path, checksum in expected:
        if digest(path) != checksum:
            raise ValueError(f"recorded input has changed: {path}")
    convert = shutil.which("convert")
    if not convert:
        raise ValueError("ImageMagick convert is required")
    version = subprocess.check_output([convert, "-version"], text=True).splitlines()[0]
    if version != manifest["imagemagick"]:
        raise ValueError("ImageMagick version differs from the archived run")
    subprocess.run([sys.executable, str(recipe / "tools/render_status_art.py"),
                    "--no-history", "--output", str(args.output.resolve())], check=True)
    if digest(args.output) != manifest["outputs"]["png_sha256"]:
        raise ValueError("replayed PNG differs from the archived original")
    print("Verified byte-identical PNG replay.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        raise SystemExit(str(exc))
