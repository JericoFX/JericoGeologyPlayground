"""Renderer contracts. Run with python3; add --blender for runtime checks."""
import importlib.util
import contextlib
import io
import json
import math
import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
RENDERER = HERE / "render_lab.py"


def load_renderer():
    spec = importlib.util.spec_from_file_location("render_lab", RENDERER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def fixture(folder, preset="radial"):
    folder = pathlib.Path(folder)
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "mesh.obj").write_text(
        "o silk\nv -1 0 0.5\nv 1 0 0.5\nv 1 0 2.5\nv -1 0 2.5\n"
        "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nf 1/1 2/2 3/3 4/4\n")
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, 8, 8, 32, 0x28)
    for name, rgba in (("coverage", (180, 180, 180, 255)), ("orientation", (255, 128, 0, 255))):
        r, g, b, a = rgba
        (folder / (name + ".tga")).write_bytes(header + bytes((b, g, r, a)) * 64)
    (folder / "metadata.json").write_text(json.dumps({
        "preset": preset,
        "anchors": [{"id": "a", "position": [-1, 0, 0.5]}, {"id": "b", "position": [1, 0, 2.5]}],
        "stats": {"test": True},
    }))
    return folder


class RendererContract(unittest.TestCase):
    def test_production_renderer_present(self):
        self.assertTrue(RENDERER.is_file(), "The requested Blender renderer has not been implemented")

    @unittest.skipUnless(RENDERER.is_file(), "Renderer not implemented")
    def test_cli_contract_and_preview_limits(self):
        mod = load_renderer()
        args = mod.parse_args(["--input", "outputs/radial", "--output", "renders/a.png"])
        self.assertEqual((args.light, args.view), ("frontal", "beauty"))
        self.assertEqual((args.width, args.height, args.samples), (1200, 800, 24))
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                mod.parse_args(["--input", "x", "--output", "y.png", "--samples", "33"])

    @unittest.skipUnless(RENDERER.is_file(), "Renderer not implemented")
    def test_asset_validation_requires_coverage_orientation_and_uv(self):
        mod = load_renderer()
        with tempfile.TemporaryDirectory() as directory:
            assets = fixture(directory)
            data = mod.load_assets(assets)
            self.assertEqual(data["metadata"]["preset"], "radial")
            self.assertEqual(len(data["vertices"]), 4)
            self.assertEqual(data["vertices"][0], (-1.0, 0.0, 0.5))
            self.assertEqual(len(data["faces"]), 1)
            (assets / "coverage.tga").unlink()
            with self.assertRaisesRegex(ValueError, "coverage.tga"):
                mod.load_assets(assets)

    @unittest.skipUnless(RENDERER.is_file(), "Renderer not implemented")
    def test_invalid_uv_fails_instead_of_silently_showing_cards(self):
        mod = load_renderer()
        with tempfile.TemporaryDirectory() as directory:
            assets = fixture(directory)
            (assets / "mesh.obj").write_text("v 0 0 0\nv 1 0 0\nv 0 0 1\nf 1 2 3\n")
            with self.assertRaisesRegex(ValueError, "UV"):
                mod.load_assets(assets)

    @unittest.skipUnless(RENDERER.is_file(), "Renderer not implemented")
    def test_existing_outputs_require_explicit_overwrite(self):
        mod = load_renderer()
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            target = root / "frozen.png"
            target.write_bytes(b"delivered immutable image")
            args = mod.parse_args(["--input", str(root), "--output", str(target)])
            self.assertIn("overwrite", vars(args), "Output immutability needs an explicit overwrite option")
            self.assertFalse(args.overwrite)
            with self.assertRaisesRegex(FileExistsError, "overwrite"):
                mod.validate_output(args)
            self.assertEqual(target.read_bytes(), b"delivered immutable image")
            args = mod.parse_args(["--input", str(root), "--output", str(target), "--overwrite"])
            mod.validate_output(args)
            target.unlink()
            target.with_suffix(".render.json").write_text("{}")
            args.overwrite = False
            with self.assertRaisesRegex(FileExistsError, "overwrite"):
                mod.validate_output(args)


def blender_contract():
    import bpy
    import _cycles
    mod = load_renderer()
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        for preset, view, light in (("radial", "beauty", "frontal"), ("corner", "oblique", "side"),
                                    ("corridor", "debug", "back"), ("radial", "edge", "back")):
            assets = fixture(root / preset, preset)
            args = mod.parse_args(["--input", str(assets), "--output", str(root / "test.png"),
                                   "--view", view, "--light", light, "--validate-only"])
            result = mod.build_scene(args)
            scene = bpy.context.scene
            silk = [o for o in scene.objects if o.get("cobweb_role") == "silk"]
            assert len(silk) == 1, "Exactly one silk object is required"
            assert silk[0].type == "MESH" and len(silk[0].data.uv_layers) == 1
            assert scene.render.engine == "CYCLES" and scene.cycles.device == "CPU"
            assert (scene.render.resolution_x, scene.render.resolution_y) == (1200, 800)
            assert scene.cycles.samples <= 32 and scene.view_settings.exposure == 0.0
            assert not scene.cycles.use_denoising or _cycles.with_openimagedenoise, "Cannot enable an unavailable CPU denoiser"
            tree = silk[0].data.materials[0].node_tree
            coverage = tree.nodes["Coverage_Mathematical"]
            assert coverage.image.colorspace_settings.name == "Non-Color"
            alpha = tree.nodes["Coverage_Only_Alpha"]
            assert len(alpha.inputs[0].links) == 1
            assert alpha.inputs[0].links[0].from_node == coverage, "Only coverage may drive alpha"
            assert tree.nodes["Orientation_Double_Angle"].image.colorspace_settings.name == "Non-Color"
            assert tree.nodes["Silk_Principled"].inputs["Anisotropic Rotation"].is_linked
            assert result["report"]["silk_mesh_objects"] == 1
            if view == "debug":
                assert any(o.get("cobweb_role") == "anchors" for o in scene.objects)
                assert any(o.get("cobweb_role") == "clearance" for o in scene.objects)
            if preset == "corridor":
                pins = [o for o in scene.objects if o.get("cobweb_role") == "support_pin"]
                assert len(pins) == len(result["assets"]["metadata"]["anchors"]), "All corridor anchors need visible physical fixing"
                posts = [o for o in scene.objects if o.name.startswith("Timber post")]
                assert sorted(set(round(o.location.y, 3) for o in posts)) == [0, 3, 6, 9]
                lintel = next(o for o in scene.objects if o.name.startswith("Timber lintel"))
                assert abs(posts[0].dimensions.y-lintel.dimensions.y) > 0.005, "Coplanar overlapping timber faces cause black square artifacts"
            if preset == "corner":
                assert abs(scene.objects["Corner beam"].dimensions.y-scene.objects["Corner lintel"].dimensions.y) > 0.005
            assert not any(o.type == "CURVE" and o.get("cobweb_role") == "silk" for o in scene.objects)
    print("BLENDER_RUNTIME_CONTRACT_OK")


def blender_cli_contract():
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        assets = fixture(root / "assets")
        target = root / "delivered.png"
        target.write_bytes(b"immutable delivered preview")
        process = subprocess.run(["/usr/bin/blender", "-b", "--python", str(RENDERER), "--",
                                  "--input", str(assets), "--output", str(target)], text=True, capture_output=True)
        assert process.returncode != 0, "Blender CLI must return failure when a frozen output is rejected"
        assert target.read_bytes() == b"immutable delivered preview"
        assert "overwrite" in process.stderr + process.stdout
    print("BLENDER_CLI_IMMUTABILITY_OK")


if __name__ == "__main__":
    if "--inside-blender" in sys.argv:
        blender_contract()
    else:
        run_blender = "--blender" in sys.argv
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(RendererContract)
        result = unittest.TextTestRunner(verbosity=2).run(suite)
        if not result.wasSuccessful():
            raise SystemExit(1)
        if run_blender:
            blender_cli_contract()
            process = subprocess.run(["/usr/bin/blender", "-b", "--factory-startup", "--python", str(__file__),
                                      "--", "--inside-blender"], text=True, capture_output=True)
            print(process.stdout)
            print(process.stderr, file=sys.stderr)
            if process.returncode or "BLENDER_RUNTIME_CONTRACT_OK" not in process.stdout:
                raise SystemExit(1)
