#!/usr/bin/env python3
"""Reproducible original-asset status art; see status/README.md."""
import argparse
from copy import deepcopy
from datetime import datetime, timezone
from decimal import Decimal, InvalidOperation
import hashlib
import json
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[1]
EFFORT = {"idle": 0, "low": 2, "steady": 4, "high": 8, "intensive": 12}
# Ground anchors BEFORE camera shift; rear workers are drawn before Wonder.
SLOTS = [
    (205, 163, 54, True, "NE", "front"),
    (407, 169, 54, False, "NW", "front"),
    (268, 194, 52, True, "NE", "front"),
    (357, 194, 52, False, "NW", "front"),
    (250, 67, 22, True, "SE", "rear"),
    (374, 67, 22, False, "SW", "rear"),
    (202, 91, 20, True, "SE", "rear"),
    (422, 91, 20, False, "SW", "rear"),
    (237, 179, 54, True, "NE", "front"),
    (382, 182, 54, False, "NW", "front"),
    (226, 79, 22, True, "SE", "rear"),
    (398, 79, 22, False, "SW", "rear"),
]


def progress_value(value):
    try:
        result = Decimal(str(value))
    except InvalidOperation as exc:
        raise ValueError("progress must be a number from 0 to 100") from exc
    if not result.is_finite() or not 0 <= result <= 100:
        raise ValueError("progress must be a number from 0 to 100")
    return result


def construction_stage(value):
    """RoR 004ae880: trunc(angle_count * build_accumulator / train_time).

    Wonder graphic 707 has 3 angles and one frame per angle, no mirror.
    At completion 004ae500 selects the standing graphic.
    """
    percent = progress_value(value)
    return None if percent == 100 else int(percent * 3 / 100)


def worker_plan(count, complete=False):
    if isinstance(count, bool) or not isinstance(count, int) or not 0 <= count <= len(SLOTS):
        raise ValueError("workers must be an integer from 0 to 12")
    return [] if complete else [dict(x=x, y=y, frame=f, mirror=m, facing=d, layer=l)
                               for x, y, f, m, d, l in SLOTS[:count]]


def render_plan(config):
    """Return the game-rule-derived choices without decoding or drawing assets.

    This is deliberately separate from rendering so a status update can be
    reviewed before it changes the published image or creates a history entry.
    """
    progress = progress_value(config["progress_percent"])
    effort = config["effort"]
    if effort not in EFFORT:
        raise ValueError(f"effort must be one of {', '.join(EFFORT)}")
    requested = config.get("workers")
    requested = EFFORT[effort] if requested is None else requested
    stage = construction_stage(progress)
    workers = worker_plan(requested, stage is None)
    return dict(progress_percent=format(progress, "f"),
                construction_frame=stage,
                completed=stage is None,
                effort=effort,
                requested_workers=requested,
                rendered_workers=len(workers),
                workers=workers)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def resolve(path):
    p = Path(path).expanduser()
    return p if p.is_absolute() else ROOT / p


def quoted(value):
    return "'" + str(value).replace("\\", "\\\\").replace("'", "\\'") + "'"


class Render:
    def __init__(self, config, scratch):
        self.config = config
        self.scratch = Path(scratch)
        self.commands = []
        self.assets = {k: resolve(v) for k, v in config["assets"].items()}
        self.decoder = ROOT / "build/host/slp_tool"
        self.convert = shutil.which("convert")
        self.info_cache = {}
        self.frame_cache = {}
        self.used_art = []
        if not self.convert:
            raise ValueError("ImageMagick convert is required")
        for p in [self.decoder, *self.assets.values()]:
            if not p.is_file():
                raise ValueError(f"missing decoder or original asset: {p}")

    def run(self, args):
        args = [str(a) for a in args]
        self.commands.append(args)
        return subprocess.run(args, check=True, text=True, capture_output=True).stdout

    def frame(self, archive, resource, frame=0, mirror=False):
        key = (archive, resource, frame, mirror)
        if key in self.frame_cache:
            return self.frame_cache[key]
        info_key = (archive, resource)
        if info_key not in self.info_cache:
            info = self.run([self.decoder, "info", self.assets[archive], resource])
            self.info_cache[info_key] = {
                int(n): (int(w), int(h), int(x), int(y))
                for n, w, h, x, y in re.findall(
                    r"frame (\d+): (\d+)x(\d+) hotspot \((-?\d+),(-?\d+)\)", info)
            }
        if frame not in self.info_cache[info_key]:
            raise ValueError(f"resource {resource} has no frame {frame}")
        w, h, hx, hy = self.info_cache[info_key][frame]
        path = self.scratch / f"{archive}-{resource}-{frame}-{int(mirror)}.pam"
        self.run([self.decoder, "render", self.assets[archive], resource,
                  self.assets["palette"], path, frame])
        if mirror:
            original = path.with_suffix(".original.pam")
            path.rename(original)
            self.run([self.convert, original, "-flop", path])
            hx = w - hx
        result = dict(path=path, width=w, height=h, hotspot_x=hx, hotspot_y=hy)
        self.frame_cache[key] = result
        self.used_art.append(dict(archive=archive, resource=resource, frame=frame,
                                 mirror=mirror, width=w, height=h, hotspot_x=hx, hotspot_y=hy))
        return result

    def at(self, art, x, y, checked=True):
        scene = self.config["scene"]
        if checked and (x < 0 or y < 22 or x + art["width"] > scene["width"] or
                        y + art["height"] > scene["height"]):
            raise ValueError(f"sprite would be clipped: {art['path'].name} at {x},{y}; "
                             "increase scene height/vertical shift")
        return f"image over {x},{y} 0,0 {quoted(art['path'])}"

    def anchored(self, art, x, y):
        return self.at(art, x - art["hotspot_x"], y - art["hotspot_y"])

    def make(self, png, jpg):
        config = self.config
        plan = render_plan(config)
        p = progress_value(plan["progress_percent"])
        stage = plan["construction_frame"]
        effort = plan["effort"]
        count = plan["requested_workers"]
        workers = plan["workers"]
        scene = config["scene"]
        shift, width, height, scale = (scene[k] for k in
                                      ("vertical_shift", "width", "height", "scale"))
        if width != 480 or not all(isinstance(v, int) for v in (height, shift, scale)):
            raise ValueError("scene width is 480; height, shift, scale must be integers")
        if not 220 <= height <= 600 or not 0 <= shift <= 200 or scale != 2:
            raise ValueError("height 220..600, vertical_shift 0..200, scale 2 required")
        grass = self.frame("terrain", 15001)
        # Oversized grid: no sprite crosses an MVG canvas edge.
        terrain = [self.at(grass, col * 64 + row % 2 * 32, row * 16, False)
                   for row in range(48) for col in range(17)]
        terrain_path = self.scratch / "terrain.png"
        self.run([self.convert, "-size", "1152x840", "xc:#416325", "-draw", " ".join(terrain),
                  "-crop", f"{width}x{height}+96+64", "+repage", terrain_path])
        drawing = [self.at(self.frame("ror_graphics", 720), 5, 26 + shift),
                   self.at(self.frame("ror_graphics", 723), 5, 150 + shift),
                   self.at(self.frame("ror_graphics", 723), 89, 150 + shift)]
        for row in range(5):
            for col in range(5):
                dirt = self.frame("terrain", 15000, row % 3 * 3 + col % 3)
                drawing.append(self.at(dirt, 280 + (col - row) * 32,
                                       45 + (col + row) * 16 + shift))
        for layer in ("rear", "front"):
            if layer == "front":
                art = self.frame("ror_graphics", 735) if stage is None else self.frame("base_graphics", 302, stage)
                drawing.append(self.anchored(art, 312, 125 + shift))
            for worker in sorted((w for w in workers if w["layer"] == layer), key=lambda w: w["y"]):
                distance = abs(worker["x"] - 312) / 160 + abs(worker["y"] - 125) / 80
                if distance <= 1:
                    raise ValueError("worker is inside the Wonder footprint")
                art = self.frame("base_graphics", 628, worker["frame"], worker["mirror"])
                drawing.append(self.anchored(art, worker["x"], worker["y"] + shift))
        sh = height * scale
        total_height = sh + 104
        regular = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
        bold = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
        percent_text = format(p.quantize(Decimal("0.01")).normalize(), "f")
        heading = "WONDER COMPLETE" if stage is None else "WONDER UNDER CONSTRUCTION"
        right_label = ("CONSTRUCTION COMPLETE" if stage is None else
                       f"EFFORT: {effort.upper()} / {len(workers)} BUILDERS")
        label = lambda x, y, s: f"text {x},{y} {quoted(s)}"
        bar_start, bar_end = sh + 45, sh + 84
        filled = int(p * 792 / 100)
        ops = [self.convert, terrain_path, "-draw", " ".join(drawing), "-filter", "point",
               "-resize", f"960x{sh}!", "-background", "#14251e", "-gravity", "northwest",
               "-extent", f"960x{total_height}", "-gravity", "none", "-alpha", "off",
               "-fill", "#14251e", "-draw", f"rectangle 0,0 959,43 rectangle 0,{sh} 959,{total_height-1}",
               "-fill", "#e8d6a5", "-font", bold, "-pointsize", "20", "-draw", label(24, 29, config["labels"]["title"]),
               "-fill", "#d6cdb9", "-font", regular, "-pointsize", "13", "-draw", label(662, 28, config["labels"]["provenance"]),
               "-fill", "#e8d6a5", "-font", bold, "-pointsize", "18", "-draw", label(24, sh+29, heading),
               "-fill", "#d6cdb9", "-font", regular, "-pointsize", "12", "-draw", label(654, sh+29, right_label),
               "-fill", "#f6e3a7", "-font", bold, "-pointsize", "20" if "." in percent_text else "24", "-draw", label(24, sh+73, percent_text + "%"),
               "-fill", "#f3e5ba", "-draw", f"rectangle 120,{bar_start} 919,{bar_end}",
               "-fill", "#304c3c", "-draw", f"rectangle 124,{bar_start+4} 915,{bar_end-4}"]
        if filled:
            ops += ["-fill", "#f4b63e", "-draw", f"rectangle 124,{bar_start+4} {123+filled},{bar_end-4}"]
        ops += ["-fill", "#d6cdb9", "-font", regular, "-pointsize", "10", "-draw",
                label(124, sh+98, "ESTIMATED FULL-PORT COMPLETION"), "-background", "#14251e",
                "-alpha", "remove", "-alpha", "off", "-depth", "8", "-strip", "-interlace", "none", f"PNG24:{png}"]
        self.run(ops)
        self.run([self.convert, png, "-alpha", "off", "-strip", "-interlace", "none",
                  "-sampling-factor", "4:4:4", "-quality", "95", jpg])
        return dict(construction_frame=stage, complete=stage is None,
                    requested_workers=count, rendered_workers=len(workers), workers=workers,
                    dimensions=[960, total_height], used_art=self.used_art,
                    fonts={p: digest(p) for p in (regular, bold)})


def archive_run(png, jpg, config, manifest, commands):
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    folder = ROOT / "status/history" / f"{stamp}-p{config['progress_percent']}-{uuid.uuid4().hex[:8]}"
    folder.mkdir(parents=True, exist_ok=False)
    shutil.copy2(png, folder / "image.png")
    shutil.copy2(jpg, folder / "image.jpg")
    # Preserve a standalone decoder's sources and binary, renderer and rule.
    for rel in ("tools/render_status_art.py", "tools/render_status_art.sh", "tools/slp_tool.c",
                "src/slp.c", "src/drs.c", "src/palette.c", "include/slp.h", "include/drs.h",
                "include/palette.h", "status/evidence/construction-stages.md", "status/README.md"):
        dest = folder / "recipe" / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / rel, dest)
    decoder = folder / "recipe/build/host/slp_tool"
    decoder.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / "build/host/slp_tool", decoder)
    effective = deepcopy(config)
    effective["output"] = "status/replayed.png"
    effective["assets"] = {k: str(resolve(v)) for k, v in effective["assets"].items()}
    (folder / "recipe/status/current.json").write_text(json.dumps(effective, indent=2) + "\n")
    (folder / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (folder / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    return folder


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("legacy_percent", nargs="?", help="compatible positional percentage")
    parser.add_argument("legacy_output", nargs="?", help="compatible positional PNG path")
    parser.add_argument("--config", default=str(ROOT / "status/current.json"))
    parser.add_argument("--percent")
    parser.add_argument("--effort", choices=EFFORT)
    parser.add_argument("--workers", type=int)
    parser.add_argument("--output")
    parser.add_argument("--no-history", action="store_true", help="for previews; still writes a manifest")
    parser.add_argument("--progress-note", help="evidence for a changed completion estimate")
    parser.add_argument("--effort-note", help="basis for the chosen activity level")
    parser.add_argument("--plan", action="store_true",
                        help="print construction frame and worker placement; do not render")
    args = parser.parse_args()
    config = json.loads(Path(args.config).read_text())
    if config.get("schema_version") != 1:
        raise ValueError("unsupported configuration schema")
    for key, val in (("progress_percent", args.percent or args.legacy_percent),
                     ("effort", args.effort), ("workers", args.workers),
                     ("output", args.output or args.legacy_output),
                     ("progress_assessment", args.progress_note),
                     ("effort_assessment", args.effort_note)):
        if val is not None:
            config[key] = val
    config["progress_percent"] = format(progress_value(config["progress_percent"]), "f")
    if args.plan:
        print(json.dumps(render_plan(config), indent=2) + "")
        return
    png = resolve(config["output"])
    if png.suffix.lower() != ".png":
        raise ValueError("output must have a .png suffix; JPEG is exported alongside it")
    jpg = png.with_suffix(".jpg")
    png.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="aoe-status-") as scratch:
        render = Render(config, scratch)
        temp_png, temp_jpg = Path(scratch)/"image.png", Path(scratch)/"image.jpg"
        result = render.make(temp_png, temp_jpg)
        manifest = dict(schema_version=1, created_utc=datetime.now(timezone.utc).isoformat(),
                        effective_config=config, render=result,
                        source_rule="status/evidence/construction-stages.md",
                        inputs={k: dict(path=str(p), sha256=digest(p)) for k, p in render.assets.items()},
                        decoder_sha256=digest(render.decoder), renderer_sha256=digest(__file__),
                        python=platform.python_version(),
                        imagemagick=render.run([render.convert, "-version"]).splitlines()[0],
                        outputs={"png_sha256": digest(temp_png), "jpg_sha256": digest(temp_jpg)})
        if not args.no_history and png.exists() and not png.with_suffix(".json").exists():
            prior = ROOT / "status/history" / ("pre-managed-" + digest(png)[:16] + ".png")
            prior.parent.mkdir(parents=True, exist_ok=True)
            if not prior.exists():
                shutil.copy2(png, prior)
        if not args.no_history:
            history = archive_run(temp_png, temp_jpg, config, manifest, render.commands)
            manifest["history"] = str(history)
        shutil.copy2(temp_png, png)
        shutil.copy2(temp_jpg, jpg)
        png.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(png)
    print(jpg)
    if not args.no_history:
        print(history)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        print(f"status render failed: {exc}", file=sys.stderr)
        if isinstance(exc, subprocess.CalledProcessError):
            print(exc.stderr, file=sys.stderr)
        sys.exit(1)
