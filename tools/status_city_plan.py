#!/usr/bin/env python3
"""The status-art city plan (status/README.md).

The plan is a text file for `status_city_tool`, which creates every object
through the port's scenario runtime and draws the scene with its viewport
renderer. This module only decides *what* stands *where*: the layout is
checked against footprint overlaps and the view, so the engine never has
to draw two objects on the same tiles.

Coordinates are world tiles (x, y). The engine projects a tile to the screen
as x_px = (x + y - 2 * camera) * 32 and y_px = (y - x) * 16 from the view
centre: larger x is up-right, larger y is down-right. Odd footprints (3x3,
5x5, 1x1) centre on .5 coordinates, even ones (2x2) on integers.
"""
from decimal import Decimal
import math

CAMERA = (40.0, 40.0)
MAP = 80
VIEW = (1280, 800)
TILE_W, TILE_H = 64, 32

ROMAN = 13
IRON_AGE = 3

# Template ids of the patched Rise of Rome DAT (civilization 13; the Iron
# Age graphics come from the age effects the runtime applies on load).
TOWN_CENTER, HOUSE, BARRACKS, GRANARY, STORAGE_PIT, DOCK = 109, 70, 12, 68, 103, 45
MARKET, ARCHERY_RANGE, STABLE, GOVERNMENT_CENTER, TEMPLE = 84, 87, 101, 82, 104
ACADEMY, SIEGE_WORKSHOP, WONDER, FARM = 0, 49, 276, 50
GUARD_TOWER, BALLISTA_TOWER, WALL = 69, 278, 155
VILLAGER, PRIEST, SCOUT = 83, 125, 299
LEGION, CENTURION, HEAVY_CAVALRY, SCYTHE_CHARIOT = 282, 291, 38, 339
COMPOSITE_BOWMAN, HORSE_ARCHER, HEAVY_CATAPULT, HELEPOLIS = 6, 39, 280, 279
FISHING_SHIP, MERCHANT_SHIP, TRIREME, CATAPULT_TRIREME, TRANSPORT = 14, 16, 21, 250, 18
# Gaia
FISH, WHALE, GOLD_MINE, STONE_MINE, BERRY_BUSH, GAZELLE = 53, 370, 66, 102, 59, 65
TREES = (134, 140, 141, 142, 143, 147, 138, 139)
# The base game's unused cobblestone Road tile (graphic 488 "Road", SLP 530
# of graphics.drs, three variants) is mapped onto the unused terrain slot 12
# ("Grass 2"); grass borders it with the grass/desert transition 6, so the
# paving sits on a dirt shoulder and runs continuously along either axis.
ROAD, ROAD_SLP, ROAD_FRAMES, ROAD_BORDER = 12, 530, 3, 6
ROCKS = (184, 307, 308, 309)
GRASS_CLUMPS = (167, 168, 170, 171)

# Footprint radius per template type (DAT size x/y): 1x1, 2x2, 3x3, 5x5.
RADIUS = {WONDER: 2.5, WALL: 0.5, HOUSE: 1.0, GUARD_TOWER: 1.0, BALLISTA_TOWER: 1.0}
for _t in (TOWN_CENTER, BARRACKS, GRANARY, STORAGE_PIT, DOCK, MARKET, ARCHERY_RANGE,
           STABLE, GOVERNMENT_CENTER, TEMPLE, ACADEMY, SIEGE_WORKSHOP, FARM):
    RADIUS[_t] = 1.5

# Builder spawn tiles around the Wonder footprint (32..36 x 32..36), in the
# order the effort presets add them; the engine walks them to the edge.
WONDER_AT = (34.5, 34.5)
BUILDER_SLOTS = [(38.5, 33.5), (30.5, 35.5), (33.5, 30.5), (35.5, 38.5),
                 (38.5, 36.5), (30.5, 32.5), (36.5, 30.5), (32.5, 38.5),
                 (38.5, 30.5), (30.5, 38.5), (31.5, 30.5), (37.5, 38.5)]

WATER, BEACH = 1, 2


def footprint(x, y, radius):
    """Tiles covered by a footprint of `radius` centred on (x, y)."""
    x0, y0 = math.floor(x - radius), math.floor(y - radius)
    n = int(round(radius * 2))
    return [(x0 + i, y0 + j) for i in range(n) for j in range(n)]


def to_screen(x, y, camera=CAMERA, view=VIEW):
    return (view[0] / 2 + (x + y - camera[0] - camera[1]) * TILE_W / 2,
            view[1] / 2 + (y - x) * TILE_H / 2)


def shore_edge(x, y):
    """The sea lies along the top of the view: an irregular waterline around
    x - y = 16 that stays regular where the dock straddles it."""
    s = x + y - 80
    return 16 + (0 if 6 <= s <= 14 else int(round(math.sin(s * 0.55))))


def water_at(x, y):
    return x - y >= shore_edge(x, y)


def beach_at(x, y):
    """Two tiles of beach between the grass and the water, as the original
    maps lay their shores (the engine borders grass/beach and beach/water)."""
    return not water_at(x, y) and x - y >= shore_edge(x, y) - 2


class Plan:
    def __init__(self, percent, workers, lenient=False):
        self.problems = []
        self.lenient = lenient
        self.lines = ["map %d" % MAP, "player 1 %d %d" % (ROMAN, IRON_AGE),
                      "view %d %d" % VIEW, "camera %g %g" % CAMERA,
                      "terrain %d %d %d -1" % (ROAD, ROAD_SLP, ROAD_FRAMES),
                      "border 0 %d %d" % (ROAD, ROAD_BORDER)]
        self.taken = {}
        self.objects = []
        self.percent = percent
        self.workers = workers
        self.builders = 0

    def claim(self, tiles, what, allow=()):
        for t in tiles:
            if not (0 <= t[0] < MAP and 0 <= t[1] < MAP):
                raise ValueError(f"{what} leaves the map at {t}")
            holder = self.taken.get(t)
            if holder is not None and holder not in allow:
                self.fail(f"{what} overlaps {holder} at {t}")
            self.taken[t] = what

    def fail(self, message):
        if not self.lenient:
            raise ValueError(message)
        self.problems.append(message)

    def visible(self, x, y, what, margin=48):
        sx, sy = to_screen(x, y)
        if not (margin <= sx <= VIEW[0] - margin and margin <= sy <= VIEW[1] - 8):
            self.fail(f"{what} at {x},{y} is outside the view ({sx:.0f},{sy:.0f})")

    def building(self, template, x, y, player=1):
        r = RADIUS[template]
        name = f"building {template}"
        tiles = footprint(x, y, r)
        for t in tiles:
            if water_at(*t) and template != DOCK:
                self.fail(f"{name} at {x},{y} stands in water at {t}")
        self.claim(tiles, name)
        self.visible(x, y, name)
        self.lines.append(f"object {player} {template} {x} {y}")
        self.objects.append((template, x, y))

    def unit(self, template, x, y, rotation=None, player=1, on_water=False):
        name = f"unit {template}"
        t = (math.floor(x), math.floor(y))
        if water_at(*t) != on_water:
            self.fail(f"{name} at {x},{y} is on the wrong terrain")
        self.claim([t], name, allow=("road",))
        self.visible(x, y, name, margin=16)
        rot = "" if rotation is None else f" {rotation}"
        self.lines.append(f"object {player} {template} {x} {y}{rot}")
        self.objects.append((template, x, y))

    def gaia(self, template, x, y, rotation=None, on_water=False):
        self.unit(template, x, y, rotation, player=0, on_water=on_water)

    def road(self, x, y):
        """One tile of paved road (the Road terrain; units may stand on it)."""
        t = (math.floor(x), math.floor(y))
        if water_at(*t) or beach_at(*t):
            self.fail(f"road at {t} runs into the shore")
        self.claim([t], "road")
        self.lines.append(f"fill {ROAD} {t[0]} {t[1]} {t[0]} {t[1]}")

    def decor(self, template, x, y):
        """Flat eye candy that may share a tile with nothing else."""
        t = (math.floor(x), math.floor(y))
        self.claim([t], "decor")
        self.visible(x, y, "decor", margin=16)
        self.lines.append(f"object 0 {template} {x} {y}")

    def order(self, template, x, y, player=1, on_water=False):
        """A worker created at (x, y) and sent to the most recent object."""
        name = f"worker {template}"
        t = (math.floor(x), math.floor(y))
        if water_at(*t) != on_water:
            self.fail(f"{name} at {x},{y} is on the wrong terrain")
        self.claim([t], name, allow=("road",))
        self.visible(x, y, name, margin=16)
        self.lines.append(f"order {player} {template} {x} {y}")

    def wonder(self):
        x, y = WONDER_AT
        tiles = footprint(x, y, RADIUS[WONDER])
        self.claim(tiles, "wonder")
        self.visible(x, y, "wonder")
        self.lines.append(f"foundation 1 {WONDER} {x} {y} {self.percent}")
        self.objects.append((WONDER, x, y))
        if Decimal(str(self.percent)) < 100:
            for bx, by in BUILDER_SLOTS[:self.workers]:
                self.order(VILLAGER, bx, by)
                self.builders += 1

    def ticks(self, n):
        self.lines.append(f"ticks {n}")

    def text(self):
        return "\n".join(self.lines) + "\n"


def city_plan(percent, workers, lenient=False):
    """The Roman city: the harbour along the top, the forum and the Wonder
    in the middle, the military quarter inside the southern walls, fields,
    woods and mines outside."""
    p = Plan(percent, workers, lenient)
    # --- the sea -----------------------------------------------------------
    for y in range(MAP):
        for x in range(MAP):
            if water_at(x, y):
                p.lines.append(f"fill {WATER} {x} {y} {x} {y}")
            elif beach_at(x, y):
                p.lines.append(f"fill {BEACH} {x} {y} {x} {y}")
    # --- roads along the tile axes through the forum crossing (40, 40) -------
    for y in range(29, 58):
        p.road(40.5, y + 0.5)
    for x in range(22, 56):
        if water_at(x, 40) or beach_at(x, 40):
            break
        if x != 40:
            p.road(x + 0.5, 40.5)
    for x in range(37, 40):                         # the Wonder's approach
        p.road(x + 0.5, 34.5)
    # --- the forum (north of the crossing) -----------------------------------
    p.building(TOWN_CENTER, 42.5, 37.5)
    p.building(MARKET, 42.5, 33.5)
    p.building(GOVERNMENT_CENTER, 46.5, 34.5)
    p.building(TEMPLE, 46.5, 38.5)
    p.building(DOCK, 53.5, 36.5)
    p.building(HOUSE, 50, 39)
    p.unit(PRIEST, 44.5, 36.5, 135)
    p.unit(PRIEST, 44.5, 38.5, 45)
    p.unit(VILLAGER, 44.5, 35.5, 90)
    p.unit(VILLAGER, 41.5, 35.5, 180)
    p.unit(VILLAGER, 44.5, 33.5, 45)
    p.unit(VILLAGER, 39.5, 36.5, 135)
    # --- the Wonder (west) and its builders ----------------------------------
    p.wonder()
    p.building(STORAGE_PIT, 30.5, 38.5)
    # --- the eastern district: academy, houses, granary, fields -------------
    p.building(ACADEMY, 45.5, 44.5)
    for hx, hy in ((42, 43), (42, 46), (46, 42), (49, 42), (49, 45), (49, 48), (52, 44), (51, 47)):
        p.building(HOUSE, hx, hy)
    p.building(GRANARY, 42.5, 49.5)
    for fx, fy, wx, wy in ((46.5, 49.5, 44.5, 49.5), (42.5, 55.5, 40.5, 56.5),
                           (37.5, 28.5, 39.5, 28.5), (33.5, 28.5, 35.5, 28.5)):
        p.building(FARM, fx, fy)
        p.order(VILLAGER, wx, wy)
    for bx, by in ((44.5, 47.5), (44.5, 48.5), (45.5, 47.5)):
        p.gaia(BERRY_BUSH, bx, by)
    p.order(VILLAGER, 43.5, 46.5)
    p.order(VILLAGER, 46.5, 46.5)
    # --- the military quarter (south, inside the walls) ----------------------
    p.building(BARRACKS, 31.5, 42.5)
    p.building(STABLE, 37.5, 42.5)
    p.building(ARCHERY_RANGE, 31.5, 49.5)
    p.building(SIEGE_WORKSHOP, 37.5, 49.5)
    for x, y in ((x, y) for y in (41, 42, 43) for x in (33, 34, 35)):
        p.unit(LEGION, x + 0.5, y + 0.5, 90)
    for x, y in ((x, y) for y in (48, 49, 50) for x in (33, 34, 35)):
        p.unit(CENTURION, x + 0.5, y + 0.5, 90)
    for x in (29, 30, 31, 32):
        p.unit(HEAVY_CAVALRY, x + 0.5, 44.5, 90)
        p.unit(COMPOSITE_BOWMAN, x + 0.5, 46.5, 90)
    for x in (36, 37, 38, 39):
        p.unit(SCYTHE_CHARIOT, x + 0.5, 44.5, 90)
        p.unit(HORSE_ARCHER, x + 0.5, 46.5, 90)
    p.unit(HEAVY_CATAPULT, 30.5, 47.5, 90)
    p.unit(HELEPOLIS, 32.5, 47.5, 90)
    p.unit(HEAVY_CATAPULT, 34.5, 47.5, 90)
    p.unit(HELEPOLIS, 36.5, 47.5, 90)
    p.unit(HEAVY_CATAPULT, 38.5, 47.5, 90)
    # --- the walls: an angle of stone wall with gates on both roads ----------
    for y in range(37, 52):
        if y not in (38, 39, 40, 41, 42):
            p.building(WALL, 28.5, y + 0.5)
    for x in range(29, 45):
        if x not in (38, 39, 40, 41, 42):
            p.building(WALL, x + 0.5, 52.5)
    p.building(WALL, 28.5, 52.5)
    p.building(GUARD_TOWER, 28, 39)
    p.building(GUARD_TOWER, 28, 42)
    p.building(BALLISTA_TOWER, 39, 52)
    p.building(BALLISTA_TOWER, 42, 52)
    p.building(GUARD_TOWER, 28, 36)
    p.building(GUARD_TOWER, 46, 52)
    p.unit(SCOUT, 27.5, 40.5, 0)
    p.unit(SCOUT, 40.5, 53.5, 90)
    # --- outside: the woods to the west, mines to the south, game -----------
    woods = [(26, 44), (25, 45), (26, 46), (24, 43), (25, 47), (26, 48), (24, 46),
             (25, 49), (26, 50), (23, 45), (24, 48), (26, 42), (23, 47), (25, 43),
             (22, 46), (24, 41), (25, 41), (23, 43), (22, 44), (26, 40), (25, 39)]
    for i, (x, y) in enumerate(woods):
        p.gaia(TREES[i % len(TREES)], x + 0.5, y + 0.5)
        if i < 3:
            p.order(VILLAGER, x + 1.5, y + 0.5)
    p.gaia(GOLD_MINE, 36.5, 54.5)
    p.gaia(GOLD_MINE, 37.5, 55.5)
    p.order(VILLAGER, 35.5, 54.5)
    p.gaia(STONE_MINE, 33.5, 55.5)
    p.gaia(STONE_MINE, 34.5, 56.5)
    p.order(VILLAGER, 33.5, 54.5)
    for i, (x, y) in enumerate(((31, 55), (35, 58), (38, 56), (23, 41))):
        p.decor(ROCKS[i % len(ROCKS)], x + 0.5, y + 0.5)
    for i, (x, y) in enumerate(((33, 57), (44, 54), (36, 39), (26, 38), (39, 57))):
        p.decor(GRASS_CLUMPS[i % len(GRASS_CLUMPS)], x + 0.5, y + 0.5)
    p.gaia(GAZELLE, 31.5, 53.5, 45)
    p.gaia(GAZELLE, 29.5, 53.5, 90)
    p.gaia(GAZELLE, 33.5, 53.5, 135)
    # --- the harbour --------------------------------------------------------
    p.gaia(FISH, 48.5, 28.5, on_water=True)
    p.order(FISHING_SHIP, 47.5, 30.5, on_water=True)
    p.gaia(FISH, 56.5, 38.5, on_water=True)
    p.order(FISHING_SHIP, 57.5, 40.5, on_water=True)
    p.unit(TRIREME, 52.5, 31.5, 180, on_water=True)
    p.unit(CATAPULT_TRIREME, 44.5, 26.5, 0, on_water=True)
    p.unit(MERCHANT_SHIP, 55.5, 35.5, 225, on_water=True)
    p.unit(TRANSPORT, 41.5, 23.5, 180, on_water=True)
    p.gaia(WHALE, 50.5, 26.5, on_water=True)
    p.ticks(300)
    return p


if __name__ == "__main__":
    import sys
    pct = sys.argv[1] if len(sys.argv) > 1 else "20"
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 8
    plan = city_plan(pct, n, lenient=True)
    for problem in plan.problems:
        print(problem, file=sys.stderr)
    if plan.problems:
        sys.exit(1)
    sys.stdout.write(plan.text())
    print(f"# {len(plan.objects)} objects, {plan.builders} builders", file=sys.stderr)
