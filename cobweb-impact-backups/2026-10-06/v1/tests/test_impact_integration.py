#!/usr/bin/env python3
"""End-to-end impact export checks using only the Python standard library.

Run scripts/build.sh first, then:
    python3 -m unittest discover -s tests -p 'test_*.py' -v

Every fixture is generated in a fresh temporary directory. No checked-in output
or directory naming convention is used as evidence. IMPACT_DEMO may select a
different already-built executable.
"""

import csv
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
FRAME_NAMES = (
    "01_before", "02_indent", "03_response", "04_remnants", "05_later"
)
FRAME_TICKS = (0, 47, 52, 70, 106)
CARRIER_FACES = 800
MAX_PATCHES = 4
MAX_REMNANT_VERTICES = MAX_PATCHES * 5 * 5
MAX_REMNANT_FACES = MAX_PATCHES * 4 * 4 * 2


def read_json(path):
    # Python otherwise accepts nonstandard NaN and Infinity tokens as JSON.
    def reject_constant(value):
        raise ValueError("nonfinite JSON constant: " + value)
    return json.loads(path.read_text(), parse_constant=reject_constant)


def read_obj(path):
    vertices, texcoords, faces = [], [], []
    for line in path.read_text().splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == "v":
            vertices.append(tuple(map(float, fields[1:])))
        elif fields[0] == "vt":
            texcoords.append(tuple(map(float, fields[1:])))
        elif fields[0] == "f":
            face = []
            for field in fields[1:]:
                indexes = field.split("/")
                vertex, texture = int(indexes[0]) - 1, int(indexes[1]) - 1
                if not 0 <= vertex < len(vertices) or not 0 <= texture < len(texcoords):
                    raise ValueError("OBJ index outside exported geometry")
                face.append((vertex, texture))
            if len(face) != 3:
                raise ValueError("impact export must contain triangle faces")
            faces.append(tuple(face))
    if any(len(v) != 3 or not all(map(math.isfinite, v)) for v in vertices):
        raise ValueError("invalid OBJ position")
    if any(len(uv) != 2 or not all(map(math.isfinite, uv)) for uv in texcoords):
        raise ValueError("invalid OBJ texture coordinate")
    return {"vertices": vertices, "texcoords": texcoords, "faces": faces}


def geometry_components(mesh):
    """Count connected face components, ignoring unused grid vertices."""
    neighbors = {}
    for face in mesh["faces"]:
        ids = [pair[0] for pair in face]
        for vertex in ids:
            neighbors.setdefault(vertex, set()).update(ids)
    remaining = set(neighbors)
    components = []
    while remaining:
        pending = [remaining.pop()]
        visited = set(pending)
        while pending:
            for neighbor in neighbors[pending.pop()]:
                if neighbor not in visited:
                    visited.add(neighbor)
                    remaining.discard(neighbor)
                    pending.append(neighbor)
        components.append(visited)
    return components


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def inverse_stone_transform(vertex, rock):
    """Undo translation and axis-angle rotation, independently of the CLI."""
    value = subtract(vertex, rock["center"])
    axis = rock["axis"]
    length = math.sqrt(dot(axis, axis))
    axis = tuple(component / length for component in axis)
    angle = -rock["angle"]
    cosine, sine = math.cos(angle), math.sin(angle)
    perpendicular = cross(axis, value)
    projection = dot(axis, value)
    return tuple(value[i] * cosine + perpendicular[i] * sine +
                 axis[i] * projection * (1 - cosine) for i in range(3))


def snapshot(directory):
    """Include empty directories as well as file contents for overwrite checks."""
    result = {}
    for path in directory.rglob("*"):
        name = path.relative_to(directory).as_posix()
        result[name] = None if path.is_dir() else hashlib.sha256(path.read_bytes()).hexdigest()
    return result


class ImpactIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.binary = Path(os.environ.get("IMPACT_DEMO", ROOT / "build/impact_demo")).resolve()
        if not cls.binary.is_file():
            raise RuntimeError("Build impact_demo first with scripts/build.sh")
        cls.temporary = tempfile.TemporaryDirectory(prefix="impact-integration-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.workspace = Path(cls.temporary.name)
        cls.outputs = {}
        cases = {
            "breakable": [],
            "repeat": [],
            "unbreakable": ["--breakable", "0", "--score", "1000"],
            "below": ["--score", "0.5"],
            "equal": ["--score", "1", "--threshold", "1"],
            "zero_wind": ["--wind", "0"],
            "wind": ["--breakable", "0", "--wind", "0.05"],
        }
        for index, (name, arguments) in enumerate(cases.items()):
            # Deliberately do not encode the mode in the output basename.
            destination = cls.workspace / ("sample " + str(index))
            result = cls.run_cli("--out", destination, *arguments)
            if result.returncode:
                raise RuntimeError("fixture generation failed: " + result.stderr)
            cls.outputs[name] = destination

    @classmethod
    def run_cli(cls, *arguments):
        return subprocess.run([str(cls.binary), *map(str, arguments)],
                              capture_output=True, text=True, timeout=60,
                              check=False, cwd=ROOT)

    def frames(self, mode):
        root = self.outputs[mode]
        return [(root / name, read_json(root / name / "frame.json"))
                for name in FRAME_NAMES]

    def trace(self, mode):
        with (self.outputs[mode] / "trace.csv").open(newline="") as stream:
            return list(csv.DictReader(stream))

    def assert_frame_matches_geometry(self, directory, frame):
        carrier = read_obj(directory / "mesh.obj")
        self.assertEqual(frame["mesh_faces"], len(carrier["faces"]))
        self.assertEqual(len(carrier["vertices"]), len(carrier["texcoords"]))
        remnant_file = directory / "remnants.obj"
        if frame["phase"] == "detached":
            self.assertEqual(carrier["vertices"], [])
            self.assertEqual(carrier["texcoords"], [])
            self.assertEqual(carrier["faces"], [])
            self.assertFalse(frame["collision_active"])
            self.assertFalse(frame["resistance_active"])
            self.assertTrue(remnant_file.is_file())
        else:
            self.assertEqual(len(carrier["faces"]), CARRIER_FACES)
            self.assertTrue(frame["collision_active"])
            self.assertTrue(frame["resistance_active"])
            self.assertFalse(remnant_file.exists())
        if remnant_file.exists():
            scraps = read_obj(remnant_file)
            self.assertEqual(frame["remnant_faces"], len(scraps["faces"]))
            self.assertEqual(frame["remnant_count"], len(geometry_components(scraps)))
            self.assertLessEqual(len(scraps["vertices"]), MAX_REMNANT_VERTICES)
            self.assertLessEqual(len(scraps["faces"]), MAX_REMNANT_FACES)
        else:
            self.assertEqual(frame["remnant_count"], 0)
            self.assertEqual(frame["remnant_faces"], 0)
        self.assertLessEqual(frame["remnant_count"], MAX_PATCHES)

    def test_break_is_one_shared_permanent_transition_at_tick_52(self):
        rows = self.trace("breakable")
        self.assertEqual([int(row["tick"]) for row in rows], list(range(107)))
        self.assertEqual(rows[41]["phase"], "intact")
        self.assertEqual(rows[42]["phase"], "indenting")
        self.assertGreater(float(rows[51]["dent"]), 0)
        events = [int(row["break_events"]) for row in rows]
        self.assertEqual(sum(b - a for a, b in zip(events, events[1:])), 1)
        for row in rows:
            detached = int(row["tick"]) >= 52
            with self.subTest(tick=row["tick"]):
                self.assertEqual(row["phase"] == "detached", detached)
                self.assertEqual(int(row["break_events"]), int(detached))
                self.assertEqual(int(row["collision"]), int(not detached))
                self.assertEqual(int(row["resistance"]), int(not detached))
                if detached:
                    self.assertEqual(float(row["dent"]), 0)
        for (directory, frame), tick in zip(self.frames("breakable"), FRAME_TICKS):
            with self.subTest(frame=directory.name):
                self.assertEqual(frame["tick"], tick)
                self.assertEqual(frame["phase"], rows[tick]["phase"])
                self.assertEqual(frame["mesh_faces"], CARRIER_FACES if tick < 52 else 0)
                self.assert_frame_matches_geometry(directory, frame)
        summary = read_json(self.outputs["breakable"] / "summary.json")
        self.assertEqual(summary["break_tick"], 52)
        self.assertEqual(summary["break_events"], 1)

    def test_unbreakable_and_subthreshold_keep_entire_carrier(self):
        for mode in ("unbreakable", "below"):
            with self.subTest(mode=mode):
                rows = self.trace(mode)
                self.assertTrue(any(float(row["dent"]) > 0 for row in rows))
                for row in rows:
                    self.assertNotEqual(row["phase"], "detached")
                    self.assertEqual(int(row["break_events"]), 0)
                    self.assertEqual(int(row["collision"]), 1)
                    self.assertEqual(int(row["resistance"]), 1)
                self.assertEqual(rows[-1]["phase"], "intact")
                self.assertEqual(float(rows[-1]["dent"]), 0)
                for directory, frame in self.frames(mode):
                    self.assertEqual(frame["mesh_faces"], CARRIER_FACES)
                    self.assertEqual(frame["break_events"], 0)
                    self.assert_frame_matches_geometry(directory, frame)
                summary = read_json(self.outputs[mode] / "summary.json")
                self.assertEqual(summary["break_tick"], -1)
                self.assertEqual(summary["break_events"], 0)

    def test_threshold_equality_breaks(self):
        summary = read_json(self.outputs["equal"] / "summary.json")
        self.assertEqual((summary["break_tick"], summary["break_events"]), (52, 1))
        frame = read_json(self.outputs["equal"] / "03_response/frame.json")
        self.assertEqual(frame["impact_score"], frame["threshold"])
        self.assert_frame_matches_geometry(self.outputs["equal"] / "03_response", frame)

    def test_indentation_moves_into_panel_away_from_incoming_stone(self):
        root = self.outputs["breakable"]
        before = read_obj(root / "01_before/mesh.obj")["vertices"]
        dented = read_obj(root / "02_indent/mesh.obj")["vertices"]
        frame = read_json(root / "01_before/frame.json")
        corners = [anchor["rest"] for anchor in frame["particles"]]
        normal = cross(subtract(corners[1], corners[0]), subtract(corners[3], corners[0]))
        displacements = [dot(subtract(b, a), normal) for a, b in zip(before, dented)]
        self.assertLess(min(displacements), -0.01)
        self.assertLessEqual(max(displacements), 1e-12)

    def test_indentation_has_no_flipped_triangles_or_large_edge_stretch(self):
        for mode in ("breakable", "unbreakable", "below", "wind"):
            root = self.outputs[mode]
            reference = read_obj(root / "reference_panel.obj")
            dented = read_obj(root / "02_indent/mesh.obj")
            self.assertEqual(dented["faces"], reference["faces"])
            stretches = []
            for triangle_index, face in enumerate(reference["faces"]):
                indices = [pair[0] for pair in face]
                before = [reference["vertices"][index] for index in indices]
                after = [dented["vertices"][index] for index in indices]
                before_normal = cross(subtract(before[1], before[0]),
                                      subtract(before[2], before[0]))
                after_normal = cross(subtract(after[1], after[0]),
                                     subtract(after[2], after[0]))
                self.assertGreater(dot(before_normal, after_normal), 0,
                                   f"{mode} triangle {triangle_index} is degenerate or turns >=90 degrees")
                for start, end in ((0, 1), (1, 2), (2, 0)):
                    stretches.append(math.dist(after[start], after[end]) /
                                     math.dist(before[start], before[end]))
            self.assertEqual(len(stretches), CARRIER_FACES * 3)
            # A generous, explicit visual-quality budget for this fixed demo.
            # The intended soft dent must not become a sharp stretched spike.
            self.assertLessEqual(max(stretches), 1.05,
                                 f"{mode} maximum edge stretch is {max(stretches):.9f}")

    def test_cosmetic_remnants_are_bounded_and_rigidly_stone_local(self):
        detached = [(directory, frame) for directory, frame in self.frames("breakable")
                    if frame["phase"] == "detached"]
        self.assertEqual(len(detached), 3)
        baseline = None
        centers, angles, world_positions = [], [], []
        for directory, frame in detached:
            self.assert_frame_matches_geometry(directory, frame)
            mesh = read_obj(directory / "remnants.obj")
            self.assertEqual(len(geometry_components(mesh)), MAX_PATCHES)
            self.assertGreater(len(mesh["faces"]), 0)
            local = [inverse_stone_transform(vertex, frame["rock"])
                     for vertex in mesh["vertices"]]
            for vertex in local:
                self.assertAlmostEqual(math.sqrt(dot(vertex, vertex)),
                                       frame["rock"]["radius"] * 1.03, delta=1e-12)
            if baseline is None:
                baseline = (local, mesh["faces"], mesh["texcoords"])
            else:
                self.assertEqual(len(local), len(baseline[0]))
                self.assertEqual(mesh["faces"], baseline[1])
                self.assertEqual(mesh["texcoords"], baseline[2])
                for actual, expected in zip(local, baseline[0]):
                    self.assertLess(math.dist(actual, expected), 1e-12)
            centers.append(tuple(frame["rock"]["center"]))
            angles.append(frame["rock"]["angle"])
            world_positions.append(mesh["vertices"])
        self.assertEqual(len(set(centers)), 3)
        self.assertEqual(len(set(angles)), 3)
        self.assertNotEqual(world_positions[0], world_positions[-1])

    def test_remnants_sample_threads_and_holes_not_solid_atlas_blocks(self):
        root = self.outputs["breakable"]
        texture = (root / "coverage.tga").read_bytes()
        size = struct.unpack_from("<H", texture, 12)[0]
        mesh = read_obj(root / "04_remnants/remnants.obj")
        for component in geometry_components(mesh):
            samples = []
            for face in mesh["faces"]:
                if face[0][0] not in component:
                    continue
                uv = [mesh["texcoords"][pair[1]] for pair in face]
                for i in range(1, 9):
                    for j in range(1, 10-i):
                        weights = (i/10, j/10, 1-(i+j)/10)
                        u, v = [sum(weights[k]*uv[k][axis] for k in range(3)) for axis in range(2)]
                        x, y = min(size-1,int(u*size)), min(size-1,int(v*size))
                        samples.append(texture[18+4*(y*size+x)]/255)
            mean = sum(samples)/len(samples)
            holes = sum(value <= .05 for value in samples)/len(samples)
            self.assertGreater(mean, .06)
            self.assertLess(mean, .35, "Remnant should not be a mostly solid white atlas crop")
            self.assertGreater(holes, .40)
            self.assertLess(holes, .90, "Remnant must still contain visible threads")

    def test_remnant_borders_are_irregular_and_stay_outside_stone(self):
        root = self.outputs["breakable"] / "04_remnants"
        frame = read_json(root / "frame.json")
        mesh = read_obj(root / "remnants.obj")
        local = [inverse_stone_transform(v, frame["rock"]) for v in mesh["vertices"]]
        for piece in range(4):
            center = local[piece*25+12]
            normal = tuple(v/math.sqrt(dot(center,center)) for v in center)
            ref = (0,1,0) if abs(normal[2])>.9 else (0,0,1)
            tangent = cross(ref,normal)
            tangent = tuple(v/math.sqrt(dot(tangent,tangent)) for v in tangent)
            factors = []
            for index in (1,3,5,9,15,19,21,23):
                v = local[piece*25+index]
                planar_x = dot(v,tangent)/dot(v,normal)
                expected_x = ((index%5)/4-.5)*1.05
                factors.append(planar_x/expected_x)
            self.assertGreater(max(factors)-min(factors), .10)
        for face in mesh["faces"]:
            centroid = tuple(sum(local[pair[0]][a] for pair in face)/3 for a in range(3))
            self.assertGreater(math.sqrt(dot(centroid,centroid)), frame["rock"]["radius"])

    def test_fixed_anchors_are_exact_even_with_wind(self):
        for mode in ("breakable", "unbreakable", "below", "wind"):
            reference = None
            for directory, frame in self.frames(mode):
                with self.subTest(mode=mode, frame=directory.name):
                    anchors = frame["particles"]
                    self.assertEqual(len(anchors), 4)
                    current = [(item["id"], item["p"], item["rest"]) for item in anchors]
                    if reference is None:
                        reference = current
                    self.assertEqual(current, reference)
                    positions = read_obj(directory / "mesh.obj")["vertices"]
                    for anchor in anchors:
                        self.assertEqual(anchor["p"], anchor["rest"])
                        if frame["phase"] != "detached":
                            self.assertIn(tuple(anchor["rest"]), positions)

    def assert_exports_identical_except_timing(self, first, second):
        first_paths = {p.relative_to(first) for p in first.rglob("*") if p.is_file()}
        second_paths = {p.relative_to(second) for p in second.rglob("*") if p.is_file()}
        self.assertEqual(first_paths, second_paths)
        for relative in sorted(first_paths):
            with self.subTest(file=str(relative)):
                if relative.as_posix() == "summary.json":
                    a, b = read_json(first / relative), read_json(second / relative)
                    for summary in (a, b):
                        timing = summary.pop("state_update_us_total")
                        self.assertTrue(math.isfinite(timing))
                        self.assertGreaterEqual(timing, 0)
                    self.assertEqual(a, b)
                else:
                    self.assertEqual((first / relative).read_bytes(),
                                     (second / relative).read_bytes())

    def test_identical_runs_match_all_exports_except_measured_timing(self):
        self.assert_exports_identical_except_timing(self.outputs["breakable"],
                                                   self.outputs["repeat"])

    def test_zero_wind_preserves_exact_rest_and_existing_deformation(self):
        root = self.outputs["breakable"]
        reference = read_obj(root / "reference_panel.obj")
        self.assertEqual(read_obj(root / "01_before/mesh.obj"), reference)
        for mode in ("unbreakable", "below"):
            self.assertEqual(read_obj(self.outputs[mode] / "05_later/mesh.obj"), reference)
        for name in FRAME_NAMES:
            for artifact in ("mesh.obj", "frame.json", "coverage.tga"):
                self.assertEqual((root / name / artifact).read_bytes(),
                                 (self.outputs["zero_wind"] / name / artifact).read_bytes())
        self.assertEqual((root / "trace.csv").read_bytes(),
                         (self.outputs["zero_wind"] / "trace.csv").read_bytes())
        # A positive-wind control makes the zero-wind check nonvacuous.
        self.assertNotEqual(read_obj(self.outputs["wind"] / "01_before/mesh.obj")["vertices"],
                            reference["vertices"])

    def test_texture_is_reused_and_export_budget_is_declared(self):
        for mode, root in self.outputs.items():
            with self.subTest(mode=mode):
                texture = (root / "coverage.tga").read_bytes()
                self.assertEqual(texture[2], 2)
                self.assertEqual(struct.unpack_from("<HH", texture, 12), (512, 512))
                self.assertEqual(texture[16], 32)
                self.assertEqual(len(texture), 18 + 512 * 512 * 4)
                self.assertTrue(any(texture[18::4]))
                for name in FRAME_NAMES:
                    self.assertEqual((root / name / "coverage.tga").read_bytes(), texture)
                summary = read_json(root / "summary.json")
                self.assertEqual(summary["panel_triangles_before"], CARRIER_FACES)
                self.assertEqual(summary["max_cosmetic_patches"], MAX_PATCHES)
                self.assertEqual(summary["state_updates"], 106)
                for field in ("physics_particles", "physics_constraints", "dynamic_texture_bakes"):
                    self.assertEqual(summary[field], 0)
                self.assertEqual(summary["engine_integration"], "none")

    def test_checkpoints_agree_with_frames_and_final_trace(self):
        phases = {"intact": 0, "indenting": 1, "detached": 2}
        for mode in self.outputs:
            for directory, frame in self.frames(mode):
                fields = (directory / "panel.checkpoint").read_text().split()
                self.assertEqual(fields[:2], ["WEB_PANEL_IMPACT", "1"])
                self.assertEqual(int(fields[2]), int(frame["breakable"]))
                self.assertEqual(int(fields[12]), frame["tick"])
                self.assertEqual(int(fields[13]), phases[frame["phase"]])
                self.assertEqual(int(fields[18]), frame["break_events"])
            self.assertEqual((self.outputs[mode] / "panel.checkpoint").read_bytes(),
                             (self.outputs[mode] / "05_later/panel.checkpoint").read_bytes())

    def test_existing_output_is_rejected_without_changing_any_file(self):
        output = self.outputs["breakable"]
        original = snapshot(output)
        result = self.run_cli("--out", output, "--breakable", "0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("output exists", result.stderr)
        self.assertEqual(snapshot(output), original)
        empty = self.workspace / "already present but empty"
        empty.mkdir()
        result = self.run_cli("--out", empty)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(list(empty.iterdir()), [])

    def test_concurrent_output_claim_has_one_winner_and_no_mixed_files(self):
        modes = (("breakable", []),
                 ("unbreakable", ["--breakable", "0", "--score", "1000"]))
        # Opposite modes make accidental partial overwrites observable. Multiple
        # fresh races also exercise the claim after both processes have baked.
        for attempt in range(3):
            output = self.workspace / ("concurrent claim " + str(attempt))
            processes = []
            try:
                for mode, arguments in modes:
                    command = [str(self.binary), "--out", str(output), *arguments]
                    process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                               stderr=subprocess.PIPE, text=True,
                                               cwd=ROOT)
                    processes.append((mode, process))
                results = []
                for mode, process in processes:
                    stdout, stderr = process.communicate(timeout=60)
                    results.append((mode, process.returncode, stdout, stderr))
                with self.subTest(attempt=attempt):
                    winners = [result for result in results if result[1] == 0]
                    losers = [result for result in results if result[1] != 0]
                    self.assertEqual(len(winners), 1, results)
                    self.assertEqual(len(losers), 1, results)
                    self.assertRegex(losers[0][3], r"output (?:already claimed|exists)")
                    self.assert_exports_identical_except_timing(
                        output, self.outputs[winners[0][0]])
            finally:
                for _, process in processes:
                    if process.poll() is None:
                        process.kill()
                    process.communicate()

    def test_signed_overflow_and_nonintegral_seeds_are_rejected(self):
        for index, seed in enumerate(("-1", "+1", "18446744073709551616",
                                      "999999999999999999999999999999", "1.5", "1x", " 1", "")):
            output = self.workspace / ("invalid seed " + str(index))
            with self.subTest(seed=seed):
                result = self.run_cli("--out", output, "--seed", seed)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("seed", result.stderr)
                self.assertFalse(output.exists())

    def test_invalid_numeric_options_leave_no_output(self):
        invalid = (("--score", "-1"), ("--score", "nan"), ("--score", "1001"),
                   ("--threshold", "-1"), ("--threshold", "inf"),
                   ("--wind", "-0.01"), ("--wind", "0.051"),
                   ("--wind", "nan"), ("--breakable", "2"))
        for index, arguments in enumerate(invalid):
            output = self.workspace / ("invalid option " + str(index))
            with self.subTest(arguments=arguments):
                result = self.run_cli("--out", output, *arguments)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
