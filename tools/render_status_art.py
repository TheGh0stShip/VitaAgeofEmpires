#!/usr/bin/env python3
"""Reproducible status art: a Roman city drawn by the port's own engine.

The scene comes from `status_city_plan.py` (what stands where) and is
created and drawn by `status_city_tool` (the scenario runtime and the
viewport renderer of the port, on the original assets). This script only
decides the Wonder's stage and the builder count from `status/current.json`,
checks the engine agreed with the stage rule, and composes the title and
progress bands; see status/README.md.
"""
import argparse
from copy import deepcopy
from datetime import datetime, timezone
from decimal import Decimal, InvalidOperation
import hashlib
import importlib.util
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
HERE = Path(__file__).resolve().parent
EFFORT = {"idle": 0, "low": 2, "steady": 4, "high": 8, "intensive": 12}
BAND_TOP, BAND_BOTTOM = 44, 104
REGULAR = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
ASSETS = ("dat", "ror_graphics", "base_graphics", "terrain", "border",
          "ror_interface", "base_interface", "tileedge", "color_dir")


def load_city():
    spec = importlib.util.spec_from_file_location("status_city_plan",
                                                  HERE / "status_city_plan.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


city = load_city()


def progress_value(value):
    try:
        result = Decimal(str(value))
    except InvalidOperation as exc:
        raise ValueError("progress must be a number from 0 to 100") from exc
    if not result.is_finite() or not 0 <= result <= 100:
        raise ValueError("progress must be a number from 0 to 100")
    return result


def construction_stage(value):
    """RoR 0x004ae880: trunc(angle_count * build_accumulator / train_time).

    The Wonder's construction graphic 707 has 3 angles and one frame per
    angle, no mirror; at completion 0x004ae500 selects the standing graphic.
    The engine runs this rule itself on the rendered foundation; this
    prediction is checked against the angle it reports.
    """
    percent = progress_value(value)
    return None if percent == 100 else int(percent * 3 / 100)


def worker_plan(count, complete=False):
    """The builders' spawn tiles around the Wonder; the engine walks them
    to the footprint edge and runs their build actions."""
    if isinstance(count, bool) or not isinstance(count, int) or \
            not 0 <= count <= len(city.BUILDER_SLOTS):
        raise ValueError("workers must be an integer from 0 to 12")
    return [] if complete else [dict(x=x, y=y) for x, y in city.BUILDER_SLOTS[:count]]


def render_plan(config):
    """Return the game-rule-derived choices without creating or drawing
    anything, so a status update can be reviewed before it changes the
    published image or creates a history entry."""
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
                wonder=dict(x=city.WONDER_AT[0], y=city.WONDER_AT[1]),
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


class Render:
    def __init__(self, config, scratch):
        self.config = config
        self.scratch = Path(scratch)
        self.commands = []
        self.assets = {k: resolve(config["assets"][k]) for k in ASSETS}
        self.tool = ROOT / "build/host/status_city_tool"
        self.convert = shutil.which("convert")
        if not self.convert:
            raise ValueError("ImageMagick convert is required")
        for key, p in self.assets.items():
            if key == "color_dir" and not p.is_dir():
                raise ValueError(f"missing colour table directory: {p}")
            if key != "color_dir" and not p.is_file():
                raise ValueError(f"missing original asset {key}: {p}")
        if not self.tool.is_file():
            raise ValueError(f"missing {self.tool}; build the status_city_tool target")

    def run(self, args):
        args = [str(a) for a in args]
        self.commands.append(args)
        return subprocess.run(args, check=True, text=True, capture_output=True).stdout

    def make(self, png, jpg):
        config = self.config
        plan = render_plan(config)
        p = progress_value(plan["progress_percent"])
        stage = plan["construction_frame"]
        effort = plan["effort"]
        workers = plan["workers"]
        scene = city.city_plan(plan["progress_percent"], plan["requested_workers"],
                               lenient=True)
        if scene.problems:
            raise ValueError("city plan: " + "; ".join(scene.problems))
        if scene.builders != len(workers):
            raise ValueError("the city plan placed a different number of builders")
        self.plan_path = self.scratch / "city.plan"
        self.plan_path.write_text(scene.text())
        ppm = self.scratch / "city.ppm"
        a = self.assets
        report = self.run([self.tool, self.plan_path, a["dat"], a["ror_graphics"],
                           a["base_graphics"], a["terrain"], a["border"],
                           a["ror_interface"], a["base_interface"], a["tileedge"],
                           a["color_dir"], ppm])
        # The engine selected the foundation's angle itself (0x004ae880);
        # it must agree with the rule this script documents.
        found = re.search(rf"foundation {city.WONDER}: [^,]*, state (\d+), "
                          r"construction angle (\d+)", report)
        if not found:
            raise ValueError("the engine did not report the Wonder foundation")
        engine_stage = None if int(found.group(1)) == 2 else int(found.group(2))
        if engine_stage != stage:
            raise ValueError(f"the engine selected construction stage {engine_stage}, "
                             f"the rule predicts {stage}")
        width, height = city.VIEW
        total_height = BAND_TOP + height + BAND_BOTTOM
        sh = BAND_TOP + height
        percent_text = format(p.quantize(Decimal("0.01")).normalize(), "f")
        heading = "WONDER COMPLETE" if stage is None else "WONDER UNDER CONSTRUCTION"
        right_label = ("CONSTRUCTION COMPLETE" if stage is None else
                       f"EFFORT: {effort.upper()} / {len(workers)} BUILDERS")
        bar_x0, bar_x1 = 120, width - 41
        bar_y0, bar_y1 = sh + 45, sh + 84
        filled = int(p * (bar_x1 - bar_x0 - 8) / 100)
        labels = config["labels"]
        ops = [self.convert, ppm, "-background", "#14251e",
               "-gravity", "North", "-splice", f"0x{BAND_TOP}",
               "-gravity", "South", "-splice", f"0x{BAND_BOTTOM}",
               "-fill", "#e8d6a5", "-font", BOLD, "-pointsize", "20",
               "-gravity", "NorthWest", "-annotate", "+24+11", labels["title"],
               "-fill", "#d6cdb9", "-font", REGULAR, "-pointsize", "13",
               "-gravity", "NorthEast", "-annotate", "+24+15", labels["provenance"],
               "-fill", "#e8d6a5", "-font", BOLD, "-pointsize", "18",
               "-gravity", "SouthWest", "-annotate", f"+24+{BAND_BOTTOM - 38}", heading,
               "-fill", "#d6cdb9", "-font", REGULAR, "-pointsize", "12",
               "-gravity", "SouthEast", "-annotate", f"+24+{BAND_BOTTOM - 34}", right_label,
               "-fill", "#f6e3a7", "-font", BOLD,
               "-pointsize", "20" if "." in percent_text else "24",
               "-gravity", "SouthWest", "-annotate", f"+24+{BAND_BOTTOM - 83}",
               percent_text + "%%",
               "-gravity", "None",
               "-fill", "#f3e5ba", "-draw", f"rectangle {bar_x0},{bar_y0} {bar_x1},{bar_y1}",
               "-fill", "#304c3c",
               "-draw", f"rectangle {bar_x0 + 4},{bar_y0 + 4} {bar_x1 - 4},{bar_y1 - 4}"]
        if filled:
            ops += ["-fill", "#f4b63e", "-draw",
                    f"rectangle {bar_x0 + 4},{bar_y0 + 4} {bar_x0 + 3 + filled},{bar_y1 - 4}"]
        ops += ["-fill", "#d6cdb9", "-font", REGULAR, "-pointsize", "10",
                "-gravity", "SouthWest", "-annotate", f"+{bar_x0 + 4}+5",
                "ESTIMATED FULL-PORT COMPLETION",
                "-alpha", "off", "-depth", "8", "-strip", "-interlace", "none",
                f"PNG24:{png}"]
        self.run(ops)
        self.run([self.convert, png, "-alpha", "off", "-strip", "-interlace", "none",
                  "-sampling-factor", "4:4:4", "-quality", "95", jpg])
        return dict(construction_frame=stage, complete=stage is None,
                    requested_workers=plan["requested_workers"],
                    rendered_workers=len(workers), workers=workers,
                    wonder=plan["wonder"], plan_objects=len(scene.objects),
                    plan_sha256=digest(self.plan_path),
                    engine_report=report.strip().splitlines(),
                    dimensions=[width, total_height],
                    fonts={f: digest(f) for f in (REGULAR, BOLD)})


def git_commit():
    try:
        head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, check=True,
                              text=True, capture_output=True).stdout.strip()
        dirty = subprocess.run(["git", "status", "--porcelain"], cwd=ROOT, check=True,
                               text=True, capture_output=True).stdout.strip() != ""
        return head + ("-dirty" if dirty else "")
    except (OSError, subprocess.CalledProcessError):
        return None


def archive_run(png, jpg, plan_path, config, manifest, commands):
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    folder = ROOT / "status/history" / f"{stamp}-p{config['progress_percent']}-{uuid.uuid4().hex[:8]}"
    folder.mkdir(parents=True, exist_ok=False)
    shutil.copy2(png, folder / "image.png")
    shutil.copy2(jpg, folder / "image.jpg")
    # The renderer, the plan module and the plan, the tool's source and
    # binary, and the rule evidence; the engine sources are the commit.
    for rel in ("tools/render_status_art.py", "tools/status_city_plan.py",
                "tools/status_city_tool.c", "tools/render_status_art.sh",
                "status/evidence/construction-stages.md", "status/README.md"):
        dest = folder / "recipe" / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / rel, dest)
    tool = folder / "recipe/build/host/status_city_tool"
    tool.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / "build/host/status_city_tool", tool)
    shutil.copy2(plan_path, folder / "recipe/status/city.plan")
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
                        help="print construction frame and builder placement; do not render")
    args = parser.parse_args()
    config = json.loads(Path(args.config).read_text())
    if config.get("schema_version") != 2:
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
        print(json.dumps(render_plan(config), indent=2))
        return
    png = resolve(config["output"])
    if png.suffix.lower() != ".png":
        raise ValueError("output must have a .png suffix; JPEG is exported alongside it")
    jpg = png.with_suffix(".jpg")
    png.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="aoe-status-") as scratch:
        render = Render(config, scratch)
        temp_png, temp_jpg = Path(scratch) / "image.png", Path(scratch) / "image.jpg"
        result = render.make(temp_png, temp_jpg)
        inputs = {}
        for key, p in render.assets.items():
            if key == "color_dir":
                shadow = p / "shadow.col"
                inputs[key] = dict(path=str(p), shadow_col_sha256=digest(shadow) if shadow.is_file() else None)
            else:
                inputs[key] = dict(path=str(p), sha256=digest(p))
        manifest = dict(schema_version=2, created_utc=datetime.now(timezone.utc).isoformat(),
                        effective_config=config, render=result,
                        source_rule="status/evidence/construction-stages.md",
                        inputs=inputs,
                        tool_sha256=digest(render.tool), renderer_sha256=digest(__file__),
                        plan_module_sha256=digest(HERE / "status_city_plan.py"),
                        source_commit=git_commit(),
                        python=platform.python_version(),
                        imagemagick=render.run([render.convert, "-version"]).splitlines()[0],
                        outputs={"png_sha256": digest(temp_png), "jpg_sha256": digest(temp_jpg)})
        if not args.no_history and png.exists() and not png.with_suffix(".json").exists():
            prior = ROOT / "status/history" / ("pre-managed-" + digest(png)[:16] + ".png")
            prior.parent.mkdir(parents=True, exist_ok=True)
            if not prior.exists():
                shutil.copy2(png, prior)
        if not args.no_history:
            history = archive_run(temp_png, temp_jpg, render.plan_path, config, manifest,
                                  render.commands)
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
