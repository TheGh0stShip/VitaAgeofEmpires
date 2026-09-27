#!/usr/bin/env python3
"""Replay the frozen v4 image using its original shell recipe and decoder."""
import hashlib
from pathlib import Path
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
archive = root / "status/recipes/v4"
output = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build/status_work/replayed-v4.png"
output = output.resolve()
recipe = (archive / "render_status_art.sh").read_text()
root_line = next(line for line in recipe.splitlines() if line.startswith("project_dir="))
recipe = recipe.replace(root_line, "project_dir=" + shlex.quote(str(root)), 1)
recipe = recipe.replace('renderer="$project_dir/build/host/slp_tool"',
                        "renderer=" + shlex.quote(str(archive / "slp_tool-linux-x86_64")), 1)
subprocess.run(["bash", "-s", "--", "10", str(output)], input=recipe, text=True, check=True)
expected = hashlib.sha256((archive / "port-progress.png").read_bytes()).hexdigest()
actual = hashlib.sha256(output.read_bytes()).hexdigest()
if actual != expected:
    raise SystemExit(f"Replay differs: {actual}; expected {expected}. Check ImageMagick/fonts/assets.")
print("Verified byte-identical replay: " + actual)
