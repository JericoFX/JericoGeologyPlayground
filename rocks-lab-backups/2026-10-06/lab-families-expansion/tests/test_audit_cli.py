import json
from fractions import Fraction
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
CLEAR = "v 0 0 0\nv 2 0 0\nv 0 2 0\nf 1 2 3\n"
CROSSING = CLEAR + "v .5 .5 -1\nv .5 .5 1\nv 1.5 .5 0\nf 4 5 6\n"
DEGENERATE = "v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3\n"


def exact_pair_classification(obj):
    """Independent rational oracle for these noncoplanar captured pairs.

    Rationalize the parsed binary doubles, matching the C++ audit input.
    No geometric epsilon or normalized floating-point plane is used.
    """
    vertices, faces = [], []
    for line in obj.splitlines():
        parts = line.split()
        if parts[0] == "v":
            vertices.append(tuple(Fraction(float(x)) for x in parts[1:]))
        elif parts[0] == "f":
            faces.append([int(x) - 1 for x in parts[1:]])
    def sub(a, b):
        return tuple(x - y for x, y in zip(a, b))
    def dot(a, b):
        return sum(x * y for x, y in zip(a, b))
    def cross(a, b):
        return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
    def plane_slice(triangle, normal, origin):
        distances = [dot(sub(p, origin), normal) for p in triangle]
        points = [p for p, d in zip(triangle, distances) if d == 0]
        for i in range(3):
            j = (i + 1) % 3
            if distances[i] * distances[j] < 0:
                fraction = distances[i] / (distances[i] - distances[j])
                points.append(tuple(triangle[i][k] + (triangle[j][k] - triangle[i][k]) * fraction for k in range(3)))
        return points
    a, b = [[vertices[i] for i in face] for face in faces]
    na, nb = [cross(sub(t[1], t[0]), sub(t[2], t[0])) for t in (a, b)]
    line = cross(na, nb)
    assert any(line), "Oracle fixture must be noncoplanar"
    axis = max(range(3), key=lambda i: abs(line[i]))
    ca, cb = plane_slice(a, nb, b[0]), plane_slice(b, na, a[0])
    if not ca or not cb:
        return "strictly_separated"
    lo = max(min(p[axis] for p in ca), min(p[axis] for p in cb))
    hi = min(max(p[axis] for p in ca), max(p[axis] for p in cb))
    if lo > hi:
        return "strictly_separated"
    shared = set(faces[0]) & set(faces[1])
    if lo == hi and any(vertices[i][axis] == lo for i in shared):
        return "shared_vertex_only"
    return "intersection"


class AuditCLI(unittest.TestCase):
    def run_audit(self, *meshes):
        with tempfile.TemporaryDirectory() as directory:
            paths = []
            for index, mesh in enumerate(meshes):
                path = pathlib.Path(directory) / f"mesh-{index}.obj"
                path.write_text(mesh)
                paths.append(str(path))
            return subprocess.run(
                [str(ROOT / "build/audit_intersections"), *paths],
                capture_output=True, text=True,
            )

    def test_clear_mesh_succeeds(self):
        result = self.run_audit(CLEAR)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("intersections=0 coplanar=0", result.stdout)

    def test_crossing_mesh_fails(self):
        result = self.run_audit(CROSSING)
        self.assertIn("intersections=1 coplanar=0", result.stdout)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)

    def test_degenerate_mesh_fails(self):
        result = self.run_audit(DEGENERATE)
        self.assertIn("degenerate=1", result.stdout)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)

    def test_batch_failure_is_not_overwritten_by_clear_mesh(self):
        for meshes in ((CROSSING, CLEAR), (CLEAR, CROSSING), (DEGENERATE, CLEAR)):
            with self.subTest(meshes=meshes):
                result = self.run_audit(*meshes)
                self.assertEqual(result.stdout.count("RESULT candidates="), 2)
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)

    def test_near_parallel_slices_do_not_expand_to_epsilon_slabs(self):
        # Captured immutable v6 OBJ pairs. Exact rational plane predicates and
        # edge-plane intervals verified five shared-vertex-only pairs and one
        # strictly separated pair; none has an intersection beyond adjacency.
        # Unreferenced bounds vertices preserve the original mesh epsilon.
        fixtures = json.loads((ROOT / "tests/fixtures/audit_near_parallel_pairs.json").read_text())
        for fixture in fixtures:
            with self.subTest(triangles=fixture["triangles"]):
                self.assertEqual(exact_pair_classification(fixture["obj"]), fixture["expected"])
                result = self.run_audit(fixture["obj"])
                self.assertIn("intersections=0 coplanar=0", result.stdout)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_shallow_angle_crossing_is_still_detected(self):
        crossing = CLEAR + "v .2 .2 -1e-8\nv 1 .2 1e-8\nv .2 1 1e-8\nf 4 5 6\n"
        self.assertEqual(exact_pair_classification(crossing), "intersection")
        result = self.run_audit(crossing)
        self.assertIn("intersections=1 coplanar=0", result.stdout)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
