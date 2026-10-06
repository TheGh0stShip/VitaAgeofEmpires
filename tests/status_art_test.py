"""The status art's game rules and city layout, independent of pixel styling."""
import importlib.util
from pathlib import Path
import re
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools"


def load(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


art = load("render_status_art")
city = load("status_city_plan")


class StatusArtTests(unittest.TestCase):
    def test_original_three_angle_boundaries(self):
        expected = [("0", 0), ("33", 0), ("33.333333", 0),
                    ("33.333334", 1), ("34", 1), ("66", 1),
                    ("66.666666", 1), ("66.666667", 2),
                    ("67", 2), ("99.99", 2), ("100", None)]
        for value, stage in expected:
            with self.subTest(progress=value):
                self.assertEqual(art.construction_stage(value), stage)

    def test_invalid_estimates(self):
        for value in ("NaN", "Infinity", "-1", "100.01", "bad"):
            with self.subTest(progress=value), self.assertRaises(ValueError):
                art.construction_stage(value)

    def test_every_builder_spawns_outside_and_near_the_wonder(self):
        wx, wy = city.WONDER_AT
        radius = city.RADIUS[city.WONDER]
        for worker in art.worker_plan(12):
            dx, dy = abs(worker["x"] - wx), abs(worker["y"] - wy)
            self.assertGreater(max(dx, dy), radius)       # outside the footprint
            self.assertLess(max(dx, dy), radius + 2)      # within two tiles of its edge
            sx, sy = city.to_screen(worker["x"], worker["y"])
            self.assertTrue(0 < sx < city.VIEW[0] and 0 < sy < city.VIEW[1])

    def test_effort_is_independent_and_completed_workers_stop(self):
        self.assertEqual(len(art.worker_plan(art.EFFORT["idle"])), 0)
        self.assertEqual(len(art.worker_plan(art.EFFORT["intensive"])), 12)
        self.assertEqual(art.worker_plan(12, complete=True), [])
        for count in (-1, 13, 2.5, True):
            with self.assertRaises(ValueError):
                art.worker_plan(count)

    def test_render_plan_uses_game_stage_and_effort_not_a_visual_guess(self):
        plan = art.render_plan({"progress_percent": "66.666667", "effort": "high",
                                "workers": None})
        self.assertEqual(plan["construction_frame"], 2)
        self.assertEqual(plan["requested_workers"], 8)
        self.assertEqual(plan["rendered_workers"], 8)
        self.assertFalse(plan["completed"])
        completed = art.render_plan({"progress_percent": "100", "effort": "intensive",
                                     "workers": None})
        self.assertIsNone(completed["construction_frame"])
        self.assertEqual(completed["rendered_workers"], 0)

    def test_city_plan_has_no_overlaps_and_the_requested_builders(self):
        plan = city.city_plan("20", 8, lenient=True)
        self.assertEqual(plan.problems, [])
        self.assertEqual(plan.builders, 8)
        lines = plan.lines
        wonder = [i for i, l in enumerate(lines)
                  if l.startswith(f"foundation 1 {city.WONDER} ")]
        self.assertEqual(len(wonder), 1)
        self.assertTrue(lines[wonder[0]].endswith(" 20"))
        # The builders are the orders that directly follow the foundation.
        following = lines[wonder[0] + 1:wonder[0] + 9]
        self.assertTrue(all(l.startswith(f"order 1 {city.VILLAGER} ") for l in following))
        self.assertFalse(lines[wonder[0] + 9].startswith("order"))
        # Every tile is claimed at most once (buildings never share tiles);
        # the city covers a few hundred of them.
        self.assertGreater(len(plan.taken), 300)

    def test_completed_wonder_has_no_builders(self):
        plan = city.city_plan("100", 12, lenient=True)
        self.assertEqual(plan.problems, [])
        self.assertEqual(plan.builders, 0)
        self.assertIn(f"foundation 1 {city.WONDER} {city.WONDER_AT[0]} {city.WONDER_AT[1]} 100",
                      plan.lines)

    def test_plan_grammar(self):
        grammar = re.compile(r"^(map \d+|player \d \d+ \d|view \d+ \d+|camera [\d.]+ [\d.]+|"
                             r"terrain \d+ \d+ \d+ -?\d+|border \d+ \d+ -?\d+|"
                             r"fill \d+ \d+ \d+ \d+ \d+|object \d \d+ [\d.]+ [\d.]+( -?[\d.]+)?|"
                             r"foundation \d \d+ [\d.]+ [\d.]+ [\d.]+|order \d \d+ [\d.]+ [\d.]+|"
                             r"ticks \d+)$")
        for line in city.city_plan("45.5", 4, lenient=True).lines:
            with self.subTest(line=line):
                self.assertRegex(line, grammar)

    def test_shore_keeps_the_dock_straddling(self):
        # The dock's land-side row is beach, the rest water, so it stands on
        # the shore like the original docks.
        tiles = city.footprint(53.5, 36.5, city.RADIUS[city.DOCK])
        water = sum(city.water_at(*t) for t in tiles)
        beach = sum(city.beach_at(*t) for t in tiles)
        self.assertGreater(water, 0)
        self.assertGreater(beach, 0)
        self.assertEqual(water + beach, len(tiles))


if __name__ == "__main__":
    unittest.main()
