"""Original construction boundaries and geometry, independent of pixel styling."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "status_art", Path(__file__).resolve().parents[1] / "tools/render_status_art.py")
art = importlib.util.module_from_spec(spec)
spec.loader.exec_module(art)


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

    def test_every_worker_outside_and_facing_inward(self):
        directions = {"NE": (1, -.5), "NW": (-1, -.5),
                      "SE": (1, .5), "SW": (-1, .5)}
        workers = art.worker_plan(12)
        for worker in workers:
            x, y = worker["x"] - 312, worker["y"] - 125
            self.assertGreater(abs(x) / 160 + abs(y) / 80, 1)
            dx, dy = directions[worker["facing"]]
            self.assertGreater(-x * dx - y * dy, 0)
            # Feet remain close to the edge, not floating far outside it.
            self.assertLess(abs(x) / 160 + abs(y) / 80, 1.2)

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


if __name__ == "__main__":
    unittest.main()
