#!/usr/bin/env python3
"""Animated status art from the original AoE/Rise of Rome sprites.

A wider companion to tools/render_status_art.py: the Wonder at the
construction stage of status/current.json with its builders swinging, and a
skirmish between coloured teams beside it. Every sprite comes from the
user's original DRS files through build/host/slp_tool; only the layout,
captions and the timing of the loop belong to this script.

    python3 tools/render_status_gif.py [OUTPUT.gif]
"""
import json
import math
import re
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
import render_status_art as art  # noqa: E402  (shared rules and slots)

ROOT = art.ROOT
DECODER = ROOT / "build/host/slp_tool"
FRAMES = 64          # animation frames per loop
DELAY_MS = 90
SCENE_W, SCENE_H, SHIFT, SCALE = 720, 360, 75, 2
WONDER_X, WONDER_Y = 340, 125   # footprint centre before the shift
BLUE, YELLOW, RED, GREEN = 16, 24, 32, 72   # player colour palette bases

# Sprite resources (graphics.drs SLP ids) and frames per stored angle.
BUILDER = (628, 16)
LEGION_ATTACK = (221, 15)
LEGION_IDLE = (438, 7)
AXE_ATTACK = (205, 15)
AXE_DEATH = (311, 10)
AXE_DEAD = (370, 6)
SWORD_ATTACK = (206, 15)
CAV_ATTACK = (210, 13)
ARCHER_ATTACK = (204, 10)
LEGION_DEAD = (396, 6)
SWORD_DEAD = (371, 6)
AXE_IDLE = (415, 8)
LIGHT_CAV_ATTACK = (211, 13)
ARCHER_DEAD = (368, 6)
TOWER = 733          # RoR Roman guard tower (ror_graphics)
ARROW, ARROW_SHADOW = 243, 710   # 37 stored angles, 5 degrees apart
# Blue towers around the Wonder (ground points) and their targets: the
# yellow troops (see VOLLEYS).
TOWERS = [(200, 214), (340, 232), (532, 128), (472, 52)]
# Shooters (x, y, launch height, target, first frame), each every 16
# frames: towers from their platform, archers from the bow.
VOLLEYS = [
    (340, 232, 80, "axe_c", 2), (340, 232, 80, "axe_b", 10),
    (200, 214, 80, "axe_c", 6),
    (532, 128, 80, "axe_a", 0), (532, 128, 80, "archer_a", 8),
    (472, 52, 80, "axe_a", 12), (472, 52, 80, "cav", 4),
    (505, 170, 22, "sword", 3), (500, 222, 22, "axe_c", 11),
    # Yellow archers shoot back at the red line.
    (676, 104, 22, "legion_a", 5), (688, 158, 22, "red_cav", 13),
    (696, 212, 22, "legion_c", 1), (690, 264, 22, "red_cav", 9),
]
# Stored angles 0..4 face S, SW, W, NW, N; mirrors give SE, E, NE.
FACING = {"S": (0, False), "SW": (1, False), "W": (2, False),
          "NW": (3, False), "N": (4, False), "SE": (1, True),
          "E": (2, True), "NE": (3, True)}


def composite(dst, im, x, y):
    """alpha_composite clipped to the destination."""
    left, top = max(0, -x), max(0, -y)
    right = min(im.width, dst.width - x)
    bottom = min(im.height, dst.height - y)
    if right > left and bottom > top:
        dst.alpha_composite(im.crop((left, top, right, bottom)), (x + left, y + top))


class Sprites:
    def __init__(self, assets, scratch):
        self.assets = assets
        self.scratch = Path(scratch)
        self.cache = {}
        self.info = {}

    def _info(self, archive, sid):
        key = (archive, sid)
        if key not in self.info:
            out = subprocess.run([DECODER, "info", self.assets[archive], str(sid)],
                                 check=True, text=True, capture_output=True).stdout
            frames = {
                int(n): (int(w), int(h), int(x), int(y))
                for n, w, h, x, y in re.findall(
                    r"frame (\d+): (\d+)x(\d+) hotspot \((-?\d+),(-?\d+)\)", out)
            }
            self.info[key] = frames
        return self.info[key]

    def get(self, archive, sid, frame=0, mirror=False, player=BLUE):
        key = (archive, sid, frame, mirror, player)
        if key in self.cache:
            return self.cache[key]
        w, h, hx, hy = self._info(archive, sid)[frame]
        path = self.scratch / f"{archive}-{sid}-{frame}-{player}.pam"
        subprocess.run([DECODER, "render", self.assets[archive], str(sid),
                        self.assets["palette"], path, str(frame), str(player)],
                       check=True)
        data = path.read_bytes()
        body = data.index(b"ENDHDR\n") + 7
        image = Image.frombytes("RGBA", (w, h), data[body:body + w * h * 4])
        if mirror:
            image = image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
            hx = w - hx
        self.cache[key] = (image, hx, hy)
        return self.cache[key]

    def unit(self, anim, facing, step, player):
        sid, per = anim
        angle, mirror = FACING[facing]
        return self.get("base_graphics", sid, angle * per + step % per, mirror, player)


def looped(t, per):
    """A whole number of `per`-frame cycles spread over the loop."""
    cycles = max(1, round(FRAMES / per))
    return (t * cycles * per // FRAMES) % per


TARGETS = {"axe_a": (600, 148), "sword": (590, 204), "axe_c": (582, 258),
           "axe_b": (634, 184), "archer_a": (676, 104), "cav": (624, 92),
           "legion_a": (566, 148), "red_cav": (556, 204), "legion_c": (548, 258)}


def battle(t):
    """(animation, facing, step, player, x, y) for the skirmish at frame t."""
    T = TARGETS
    units = [
        # Fallen yellow troops before the line, and one red legionary.
        (AXE_DEAD, "W", 5, YELLOW, 614, 122),
        (SWORD_DEAD, "W", 4, YELLOW, 606, 232),
        (ARCHER_DEAD, "SW", 5, YELLOW, 650, 262),
        (AXE_DEAD, "NW", 5, YELLOW, 616, 270),
        (LEGION_DEAD, "SE", 2, RED, 528, 182),
        # The defenders: red legions and cavalry, green archers behind.
        (LEGION_ATTACK, "E", looped(t + 5, 15), RED, *T["legion_a"]),
        (CAV_ATTACK, "E", looped(t, 13), RED, *T["red_cav"]),
        (ARCHER_ATTACK, "E", looped(t, 10), GREEN, 505, 170),
        (ARCHER_ATTACK, "E", looped(t + 4, 10), GREEN, 500, 222),
        # The yellow front line.
        (AXE_ATTACK, "W", looped(t, 15), YELLOW, *T["axe_a"]),
        (SWORD_ATTACK, "W", looped(t + 9, 15), YELLOW, *T["sword"]),
        # The yellow host behind it.
        (AXE_IDLE, "W", looped(t + 2, 8), YELLOW, *T["axe_b"]),
        (AXE_IDLE, "W", looped(t + 5, 8), YELLOW, 640, 130),
        (AXE_IDLE, "SW", looped(t + 1, 8), YELLOW, 652, 236),
        (AXE_IDLE, "W", looped(t + 6, 8), YELLOW, 668, 186),
        (AXE_IDLE, "W", looped(t + 3, 8), YELLOW, 700, 132),
        (AXE_IDLE, "W", looped(t + 7, 8), YELLOW, 700, 238),
        (SWORD_ATTACK, "SW", looped(t + 4, 15), YELLOW, 624, 156),
        (LIGHT_CAV_ATTACK, "SW", looped(t + 3, 13), YELLOW, *T["cav"]),
        (ARCHER_ATTACK, "W", looped(t + 1, 10), YELLOW, *T["archer_a"]),
        (ARCHER_ATTACK, "W", looped(t + 6, 10), YELLOW, 688, 158),
        (ARCHER_ATTACK, "W", looped(t + 3, 10), YELLOW, 696, 212),
        (ARCHER_ATTACK, "W", looped(t + 8, 10), YELLOW, 690, 264),
    ]
    # A yellow axeman falls halfway through the loop; the legion rests.
    if t < 32:
        units.append((AXE_ATTACK, "W", looped(t + 3, 15), YELLOW, *T["axe_c"]))
        units.append((LEGION_ATTACK, "E", looped(t + 11, 15), RED, *T["legion_c"]))
    else:
        if t < 42:
            units.append((AXE_DEATH, "W", t - 32, YELLOW, *T["axe_c"]))
        else:
            units.append((AXE_DEAD, "W", min(5, (t - 42) // 4), YELLOW, *T["axe_c"]))
        units.append((LEGION_IDLE, "E", looped(t, 7), RED, *T["legion_c"]))
    return units


def arrow_frame(dx, dy):
    """Stored frame and mirror for a direction (x right, y down)."""
    theta = math.degrees(math.atan2(-dx, dy))   # 0 south, +90 west
    mirror = theta < 0
    return min(36, round(abs(theta) / 5)), mirror


def arrows(t):
    """(frame, mirror, x, y, shadow) for each arrow in flight at frame t."""
    out = []
    for tx, ty, height, target, first in VOLLEYS:
        gx, gy = TARGETS[target]
        dist = math.hypot(gx - tx, gy - ty)
        flight = max(6, round(dist / 14))
        age = (t - first) % 16
        # No new shots at the second axeman once he is down.
        if age >= flight or (target == "axe_c" and t - age >= 32):
            continue
        sx, sy = tx - 4, ty - height
        ex, ey = gx, gy - 18              # the target's chest
        arc = dist * 0.18
        u0, u1 = age / flight, (age + 0.5) / flight
        pos = lambda u: (sx + (ex - sx) * u,
                         sy + (ey - sy) * u - arc * 4 * u * (1 - u))
        (x0, y0), (x1, y1) = pos(u0), pos(u1)
        frame, mirror = arrow_frame(x1 - x0, y1 - y0)
        ground = (tx + (gx - tx) * u0, ty + (gy - ty) * u0)
        sframe, smirror = arrow_frame(gx - tx, gy - ty)
        out.append((frame, mirror, round(x0), round(y0),
                    (sframe, smirror, round(ground[0]), round(ground[1]))))
    return out


def builders(plan, t):
    out = []
    for i, w in enumerate(plan["workers"]):
        base = w["frame"] - w["frame"] % 16
        step = (w["frame"] + t + i * 5) % 16
        out.append((base + step, w["mirror"], w["layer"],
                    w["x"] + WONDER_X - 312, w["y"] + WONDER_Y - 125))
    return out


def scene_frame(sprites, plan, terrain, t):
    image = terrain.copy()

    def put(sprite, x, y):
        im, hx, hy = sprite
        composite(image, im, x - hx, y - hy + SHIFT)

    def put_corner(sprite, x, y):
        composite(image, sprite[0], x, y + SHIFT)

    put_corner(sprites.get("ror_graphics", 720), 5, -24)
    put_corner(sprites.get("ror_graphics", 723), 5, 100)
    put_corner(sprites.get("ror_graphics", 723), 89, 100)
    for row in range(5):
        for col in range(5):
            dirt = sprites.get("terrain", 15000, row % 3 * 3 + col % 3)
            put_corner(dirt, WONDER_X - 32 + (col - row) * 32, 45 + (col + row) * 16)
    # Towers behind the Wonder stand behind its builders too.
    for x, y in TOWERS:
        if y < WONDER_Y:
            put(sprites.get("ror_graphics", TOWER, 0, False, BLUE), x, y)
    stage = plan["construction_frame"]
    workers = builders(plan, t)
    for layer in ("rear", "front"):
        if layer == "front":
            wonder = sprites.get("ror_graphics", 735) if stage is None else \
                sprites.get("base_graphics", 302, stage)
            put(wonder, WONDER_X, WONDER_Y)
        for frame, mirror, wl, x, y in sorted((w for w in workers if w[2] == layer),
                                               key=lambda w: w[4]):
            put(sprites.get("base_graphics", 628, frame, mirror, BLUE), x, y)
    flying = arrows(t)
    for _, _, _, _, (frame, mirror, x, y) in flying:
        put(sprites.get("base_graphics", ARROW_SHADOW, frame, mirror), x, y)
    standing = [(y, "unit", (anim, facing, step, player, x))
                for anim, facing, step, player, x, y in battle(t)]
    standing += [(y, "tower", x) for x, y in TOWERS if y >= WONDER_Y]
    for y, kind, data in sorted(standing, key=lambda u: u[0]):
        if kind == "tower":
            put(sprites.get("ror_graphics", TOWER, 0, False, BLUE), data, y)
        else:
            anim, facing, step, player, x = data
            put(sprites.unit(anim, facing, step, player), x, y)
    for frame, mirror, x, y, _ in flying:
        put(sprites.get("base_graphics", ARROW, frame, mirror), x, y)
    return image


def terrain_layer(sprites):
    grass = sprites.get("terrain", 15001)[0]
    big = Image.new("RGBA", (SCENE_W + 256, SCENE_H + 160), "#416325")
    for row in range((SCENE_H + 160) // 16 + 2):
        for col in range((SCENE_W + 256) // 64 + 2):
            composite(big, grass, col * 64 + row % 2 * 32, row * 16)
    return big.crop((96, 64, 96 + SCENE_W, 64 + SCENE_H))


def chrome(config, plan):
    """Header and footer around the scaled scene, as one RGB template."""
    sw, sh = SCENE_W * SCALE, SCENE_H * SCALE
    total = sh + 152
    regular = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    bold = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
    f = lambda path, size: ImageFont.truetype(path, size)
    image = Image.new("RGB", (sw, total), "#14251e")
    d = ImageDraw.Draw(image)
    stage = plan["construction_frame"]
    d.text((24, 11), config["labels"]["title"], font=f(bold, 20), fill="#e8d6a5")
    d.text((sw - 24, 14), "ORIGINAL ASSETS / ANIMATED STATUS ART", font=f(regular, 13),
           fill="#d6cdb9", anchor="ra")
    heading = "WONDER COMPLETE" if stage is None else "WONDER UNDER CONSTRUCTION"
    right = ("CONSTRUCTION COMPLETE" if stage is None else
             f"EFFORT: {plan['effort'].upper()} / {len(plan['workers'])} BUILDERS")
    d.text((24, sh + 13), heading, font=f(bold, 18), fill="#e8d6a5")
    d.text((sw - 24, sh + 16), right, font=f(regular, 12), fill="#d6cdb9", anchor="ra")
    left, right_edge = 120, sw - 41

    def bar(value, top, height, colour, size, note):
        p = art.progress_value(value)
        text = format(p.quantize(art.Decimal("0.01")).normalize(), "f") + "%"
        d.text((24, top + height // 2), text, font=f(bold, size), fill="#f6e3a7",
               anchor="lm")
        d.rectangle((left, top, right_edge, top + height), fill="#f3e5ba")
        d.rectangle((left + 4, top + 4, right_edge - 4, top + height - 4), fill="#304c3c")
        filled = int(p * (right_edge - left - 7) / 100)
        if filled:
            d.rectangle((left + 4, top + 4, left + 3 + filled, top + height - 4), fill=colour)
        d.text((left + 4, top + height + 4), note, font=f(regular, 10), fill="#d6cdb9")

    bar(plan["progress_percent"], sh + 45, 39, "#f4b63e", 24,
        "ESTIMATED FULL-PORT COMPLETION")
    if "coverage" in config:
        value, note = config["coverage"]
        bar(value, sh + 108, 22, "#8fb8d8", 16, note)
    return image


def coverage():
    """Translated game functions and bytes from tools/port_worklist.py."""
    out = subprocess.run([sys.executable, str(ROOT / "tools/port_worklist.py"),
                          "--summary"], check=True, text=True,
                         capture_output=True).stdout
    m = re.search(r"(\d+)/(\d+) functions cited \(([\d.]+)%\), "
                  r"\d+/\d+ bytes \(([\d.]+)%\)", out)
    if not m:
        raise SystemExit("could not read coverage from port_worklist.py")
    return int(m[1]), int(m[2]), m[3], m[4]


def main():
    output = Path(sys.argv[1]) if len(sys.argv) > 1 else \
        ROOT / "build/status_work/port-progress-animated.gif"
    config = json.loads((ROOT / "status/current.json").read_text())
    # The main bar and the Wonder stage show the full-port estimate in
    # current.json; a second bar shows measured translation coverage.
    done, total, func_pct, byte_pct = coverage()
    config["coverage"] = (func_pct, f"GAME CODE TRANSLATED: {done} / {total} FUNCTIONS "
                                    f"({byte_pct}% OF CODE BYTES)")
    plan = art.render_plan(config)
    assets = {k: art.resolve(v) for k, v in config["assets"].items()}
    for p in [DECODER, *assets.values()]:
        if not Path(p).is_file():
            raise SystemExit(f"missing decoder or original asset: {p}")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as scratch:
        sprites = Sprites(assets, scratch)
        terrain = terrain_layer(sprites)
        template = chrome(config, plan)
        frames = []
        for t in range(FRAMES):
            scene = scene_frame(sprites, plan, terrain, t).convert("RGB")
            scene = scene.resize((SCENE_W * SCALE, SCENE_H * SCALE), Image.NEAREST)
            frame = template.copy()
            frame.paste(scene.crop((0, 44, scene.width, scene.height)), (0, 44))
            frames.append(frame)
        # One shared palette for every frame keeps colours steady.
        palette = frames[0].quantize(colors=255, method=Image.Quantize.MEDIANCUT,
                                     dither=Image.Dither.NONE)
        indexed = [f.quantize(palette=palette, dither=Image.Dither.NONE)
                   for f in frames]
        indexed[0].save(output, save_all=True, append_images=indexed[1:],
                        duration=DELAY_MS, loop=0, optimize=False, disposal=1)
    print(f"wrote {output} ({FRAMES} frames, {output.stat().st_size // 1024} KiB)")


if __name__ == "__main__":
    main()
