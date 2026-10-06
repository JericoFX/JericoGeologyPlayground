#!/usr/bin/env python3
"""Blender 4.3 renderer for the stand-alone Jerico cobweb laboratory.

blender -b --python scripts/render_lab.py -- --input outputs/radial \
    --output renders/radial_front.png --light frontal --view beauty

The silk is a single UV mesh, never one object or tube per fiber. Its alpha
comes exclusively from the mathematical coverage texture. The material is a
deliberate visual approximation, not a measured or physically validated silk
BSDF. Orientation RG stores (1 + weighted cos(2 theta))/2 and
(1 + weighted sin(2 theta))/2; its magnitude does not affect alpha.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
import time


def parse_args(argv=None):
    if argv is None:
        argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--light", choices=("frontal", "side", "back"), default="frontal")
    parser.add_argument("--view", choices=("beauty", "debug", "oblique", "edge"), default="beauty")
    parser.add_argument("--samples", type=int, choices=range(1, 33), default=24, metavar="1..32")
    parser.add_argument("--width", type=int, default=1200)
    parser.add_argument("--height", type=int, default=800)
    parser.add_argument("--save-blend", action="store_true", help="Also save a portable packed .blend beside the PNG")
    parser.add_argument("--overwrite", action="store_true", help="Explicitly permit replacing existing output files")
    parser.add_argument("--validate-only", action="store_true", help="Build and validate scene without tracing a render")
    args = parser.parse_args(argv)
    if args.width < 16 or args.height < 16:
        parser.error("Render dimensions must be at least 16 pixels")
    return args


def validate_output(args):
    """Reject accidental replacement of a delivered PNG, scene, or manifest."""
    targets = [args.output, args.output.with_suffix(".render.json")]
    if args.save_blend:
        targets.append(args.output.with_suffix(".blend"))
    if not args.overwrite:
        for target in targets:
            if target.exists():
                raise FileExistsError(f"Output already exists: {target}. Choose a versioned path or explicitly pass --overwrite.")


def read_obj(path):
    """Read only geometry and per-corner UVs, preserving X/Y/Z exactly."""
    vertices, texcoords, faces, face_uvs = [], [], [], []
    with Path(path).open() as source:
        for number, line in enumerate(source, 1):
            parts = line.split()
            if not parts or parts[0].startswith("#"):
                continue
            if parts[0] == "v":
                vertices.append(tuple(float(v) for v in parts[1:4]))
            elif parts[0] == "vt":
                texcoords.append(tuple(float(v) for v in parts[1:3]))
            elif parts[0] == "f":
                face, uv = [], []
                for corner in parts[1:]:
                    indices = corner.split("/")
                    if len(indices) < 2 or not indices[1]:
                        raise ValueError(f"mesh.obj:{number}: every face corner requires a UV index")
                    vi, ti = int(indices[0]), int(indices[1])
                    vi = vi - 1 if vi > 0 else len(vertices) + vi
                    ti = ti - 1 if ti > 0 else len(texcoords) + ti
                    if not 0 <= vi < len(vertices) or not 0 <= ti < len(texcoords):
                        raise ValueError(f"mesh.obj:{number}: vertex or UV index out of range")
                    face.append(vi)
                    uv.append(texcoords[ti])
                if len(face) < 3:
                    raise ValueError(f"mesh.obj:{number}: face has fewer than three vertices")
                faces.append(face)
                face_uvs.append(uv)
    if not vertices or not faces:
        raise ValueError("mesh.obj contains no usable surface")
    if any(len(v) != 3 or not all(math.isfinite(x) for x in v) for v in vertices):
        raise ValueError("mesh.obj contains invalid vertex coordinates")
    if any(len(v) != 2 or not all(math.isfinite(x) for x in v) for v in texcoords):
        raise ValueError("mesh.obj contains invalid UV coordinates")
    return vertices, faces, face_uvs


def load_assets(directory):
    directory = Path(directory).resolve()
    for name in ("mesh.obj", "coverage.tga", "orientation.tga", "metadata.json"):
        if not (directory / name).is_file():
            raise ValueError(f"Required input file is missing: {directory / name}")
    metadata = json.loads((directory / "metadata.json").read_text())
    if metadata.get("preset") not in ("radial", "corner", "corridor"):
        raise ValueError("metadata.json preset must be radial, corner, or corridor")
    for anchor in metadata.get("anchors", []):
        position = anchor.get("position", [])
        if len(position) != 3 or not all(isinstance(v, (int, float)) and math.isfinite(v) for v in position):
            raise ValueError("Each anchor requires a finite [x,y,z] position")
    vertices, faces, face_uvs = read_obj(directory / "mesh.obj")
    return {"directory": directory, "metadata": metadata, "vertices": vertices,
            "faces": faces, "face_uvs": face_uvs}


def plain_material(name, color, roughness=0.7, metallic=0.0, emission=0.0):
    import bpy
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    bsdf = material.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Emission Color"].default_value = (*color, 1)
    bsdf.inputs["Emission Strength"].default_value = emission
    return material


def silk_material(assets, debug=False):
    import bpy
    material = bpy.data.materials.new("Silk_Approximation_Coverage_Independent")
    material.use_nodes = True
    material.surface_render_method = "DITHERED"
    material.use_backface_culling = False
    material["material_model"] = "Approximate anisotropic sheen and thin-sheet translucency; not measured silk"
    material["alpha_contract"] = "coverage.tga Color only, independent of lighting, orientation, and view"
    tree = material.node_tree
    tree.nodes.clear()
    n, link = tree.nodes.new, tree.links.new
    output = n("ShaderNodeOutputMaterial")
    output.location = (1100, 100)
    uv = n("ShaderNodeTexCoord")
    uv.location = (-1100, 100)
    coverage = n("ShaderNodeTexImage")
    coverage.name = "Coverage_Mathematical"
    coverage.label = "Coverage ONLY: no light, Fresnel or alpha remap"
    coverage.location = (-850, 350)
    coverage.image = bpy.data.images.load(str(assets["directory"] / "coverage.tga"), check_existing=False)
    coverage.image.colorspace_settings.name = "Non-Color"
    coverage.extension = "CLIP"
    coverage.interpolation = "Linear"
    link(uv.outputs["UV"], coverage.inputs["Vector"])
    orientation = n("ShaderNodeTexImage")
    orientation.name = "Orientation_Double_Angle"
    orientation.location = (-850, -180)
    orientation.image = bpy.data.images.load(str(assets["directory"] / "orientation.tga"), check_existing=False)
    orientation.image.colorspace_settings.name = "Non-Color"
    orientation.extension = "CLIP"
    link(uv.outputs["UV"], orientation.inputs["Vector"])
    rgb = n("ShaderNodeSeparateColor")
    rgb.mode = "RGB"
    rgb.location = (-620, -160)
    link(orientation.outputs["Color"], rgb.inputs["Color"])
    decoded = []
    for channel, y in (("Red", -160), ("Green", -300)):
        node = n("ShaderNodeMath")
        node.operation = "MULTIPLY_ADD"
        node.location = (-420, y)
        node.inputs[1].default_value = 2
        node.inputs[2].default_value = -1
        link(rgb.outputs[channel], node.inputs[0])
        decoded.append(node)
    angle = n("ShaderNodeMath")
    angle.operation = "ARCTAN2"
    angle.location = (-220, -180)
    link(decoded[1].outputs[0], angle.inputs[0])
    link(decoded[0].outputs[0], angle.inputs[1])
    rotation = n("ShaderNodeMath")
    rotation.operation = "MULTIPLY"
    rotation.inputs[1].default_value = 1 / (4 * math.pi)
    rotation.location = (-20, -180)
    link(angle.outputs[0], rotation.inputs[0])
    tangent = n("ShaderNodeTangent")
    tangent.direction_type = "UV_MAP"
    tangent.uv_map = "UVMap"
    tangent.location = (0, -430)
    silk = n("ShaderNodeBsdfPrincipled")
    silk.name = "Silk_Principled"
    silk.location = (210, 0)
    silk.inputs["Base Color"].default_value = (0.22, 0.245, 0.27, 1)
    silk.inputs["Roughness"].default_value = 0.38
    silk.inputs["IOR"].default_value = 1.54
    silk.inputs["Specular IOR Level"].default_value = 0.6
    silk.inputs["Anisotropic"].default_value = 0.68
    silk.inputs["Sheen Weight"].default_value = 0.55
    silk.inputs["Sheen Roughness"].default_value = 0.42
    silk.inputs["Sheen Tint"].default_value = (0.72, 0.78, 0.88, 1)
    silk.inputs["Alpha"].default_value = 1.0
    link(rotation.outputs[0], silk.inputs["Anisotropic Rotation"])
    link(tangent.outputs["Tangent"], silk.inputs["Tangent"])
    translucent = n("ShaderNodeBsdfTranslucent")
    translucent.location = (210, -570)
    translucent.inputs["Color"].default_value = (0.7, 0.75, 0.8, 1)
    surface = n("ShaderNodeMixShader")
    surface.name = "Controlled_Translucency"
    surface.location = (510, -20)
    surface.inputs[0].default_value = 0.28
    link(silk.outputs["BSDF"], surface.inputs[1])
    link(translucent.outputs["BSDF"], surface.inputs[2])
    transparent = n("ShaderNodeBsdfTransparent")
    transparent.location = (500, 300)
    alpha = n("ShaderNodeMixShader")
    alpha.name = "Coverage_Only_Alpha"
    alpha.location = (760, 180)
    link(coverage.outputs["Color"], alpha.inputs[0])
    link(transparent.outputs["BSDF"], alpha.inputs[1])
    link(surface.outputs[0], alpha.inputs[2])
    final = alpha.outputs[0]
    if debug:
        wire = n("ShaderNodeWireframe")
        wire.name = "Debug_Actual_Mesh_Edges"
        wire.use_pixel_size = True
        wire.inputs["Size"].default_value = 0.65
        emission = n("ShaderNodeEmission")
        emission.inputs["Color"].default_value = (0.018, 0.33, 0.24, 1)
        emission.inputs["Strength"].default_value = 0.65
        overlay = n("ShaderNodeMixShader")
        overlay.name = "Debug_Wire_Overlay_Not_Beauty"
        link(wire.outputs["Fac"], overlay.inputs[0])
        link(final, overlay.inputs[1])
        link(emission.outputs[0], overlay.inputs[2])
        final = overlay.outputs[0]
    link(final, output.inputs["Surface"])
    return material


def box(name, location, scale, material, role="scenery", bevel=0):
    import bpy
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(material)
    obj["cobweb_role"] = role
    if bevel:
        modifier = obj.modifiers.new("Soft timber edges", "BEVEL")
        modifier.width, modifier.segments = bevel, 2
        obj.modifiers.new("Weighted corner normals", "WEIGHTED_NORMAL")
    return obj


def segment(name, a, b, radius, material, role="scenery"):
    """For support/debug lines only. Never called for silk fibers."""
    from mathutils import Vector
    a, b = Vector(a), Vector(b)
    obj = box(name, (a+b)/2, (radius, radius, (b-a).length), material, role)
    obj.rotation_euler = (b-a).to_track_quat("Z", "Y").to_euler()
    return obj


def make_scenery(assets, debug=False):
    import bpy
    meta = assets["metadata"]
    preset = meta["preset"]
    timber = plain_material("Aged warm timber", (0.12, 0.074, 0.043), 0.84)
    floor = plain_material("Quiet slate floor", (0.038, 0.043, 0.049), 0.91)
    backdrop = plain_material("Dark matte background", (0.012, 0.020, 0.027), 0.97)
    metal = plain_material("Dark rail metal", (0.056, 0.067, 0.071), 0.45, 0.6)
    pin = plain_material("Anchor brass", (0.34, 0.2, 0.063), 0.3, 0.72)
    if preset == "corridor":
        box("Corridor floor", (0, 4.5, -0.11), (5.5, 13, 0.2), floor)
        box("Corridor far darkness", (0, 10.3, 2.15), (6, 0.15, 4.6), backdrop)
        for index, y in enumerate((0, 3, 6, 9)):
            for x in (-2.08, 2.08):
                box(f"Timber post {index} {x}", (x, y, 1.95), (0.19, 0.27, 3.9), timber, bevel=0.012)
            box(f"Timber lintel {index}", (0, y, 3.83), (4.35, 0.30, 0.21), timber, bevel=0.012)
        for index, anchor in enumerate(meta.get("anchors", [])):
            x, y, z = anchor["position"]
            if abs(x) < 1.7:
                box(f"Ceiling anchor stand-off {index}", (x,y,(z+3.83)/2),
                    (0.062,0.09,max(0.07,3.83-z)), timber)
                dimensions = (0.038,0.13,0.038)
            else:
                dimensions = (0.25,0.035,0.035)
            fixing = box(f"Corridor anchor pin {index}", (x,y,z), dimensions, pin, "support_pin")
            fixing["anchor_id"] = str(anchor.get("id", index))
        for x in (-0.61, 0.61):
            box("Simple guide rail", (x, 4.2, 0.04), (0.046, 12, 0.055), metal, bevel=0.007)
        for index in range(19):
            box("Rail sleeper", (0, index*0.6-1.0, -0.005), (1.6, 0.14, 0.055), timber)
    else:
        box("Study floor", (0, 0.1, -0.13), (7, 6, 0.22), floor)
        box("Study dark backing", (0, 0.9, 1.7), (7, 0.15, 4.5), backdrop)
        if preset == "radial":
            for x in (-1.88, 1.88):
                box("Study frame upright", (x, 0.65, 1.6), (0.14, 0.22, 3.25), timber, bevel=0.012)
            box("Study frame header", (0, 0.65, 3.2), (3.9, 0.24, 0.14), timber, bevel=0.012)
        else:
            box("Corner beam", (-1.7, 0.64, 1.58), (0.23, 0.25, 3.35), timber, bevel=0.016)
            box("Corner lintel", (0, 0.64, 3.13), (3.65, 0.28, 0.2), timber, bevel=0.016)
        for index, anchor in enumerate(meta.get("anchors", [])):
            x, y, z = anchor["position"]
            box(f"Anchor support {index}", (x, (y+0.75)/2, z), (0.078, max(0.07, 0.75-y), 0.078), timber)
            box(f"Anchor pin {index}", (x, y+0.02, z), (0.026, 0.11, 0.026), pin)
    if debug:
        marker = plain_material("Debug anchor orange", (1.0, 0.24, 0.018), emission=1.6)
        verts, faces = [], []
        radius = 0.038 if preset == "corridor" else 0.026
        for anchor in meta.get("anchors", []):
            x, y, z = anchor["position"]
            start = len(verts)
            verts.extend(((x+radius,y,z),(x-radius,y,z),(x,y+radius,z),(x,y-radius,z),(x,y,z+radius),(x,y,z-radius)))
            faces.extend(tuple(start+i for i in f) for f in ((0,2,4),(2,1,4),(1,3,4),(3,0,4),(2,0,5),(1,2,5),(3,1,5),(0,3,5)))
        mesh = bpy.data.meshes.new("All debug anchor markers")
        mesh.from_pydata(verts, [], faces)
        obj = bpy.data.objects.new("DEBUG anchor positions", mesh)
        bpy.context.collection.objects.link(obj)
        obj["cobweb_role"] = "anchors"
        mesh.materials.append(marker)
        if preset == "corridor":
            clear = plain_material("Central clearance guide", (0.04, 0.55, 0.18), emission=0.8)
            for y in (0, 9):
                points = [(-0.72,y,0.07), (0.72,y,0.07), (0.72,y,2.4), (-0.72,y,2.4)]
                for index in range(4):
                    segment("DEBUG central free envelope", points[index], points[(index+1)%4], 0.009, clear, "clearance")
            for x in (-0.72,0.72):
                for z in (0.07,2.4):
                    segment("DEBUG clearance depth", (x,0,z), (x,9,z), 0.006, clear, "clearance")


def point_at(obj, target):
    from mathutils import Vector
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat("-Z", "Y").to_euler()


def make_camera(preset, view):
    import bpy
    if preset == "corridor":
        location, target, lens = (0, -5.9, 1.9), (0, 4.8, 1.93), 34
        if view == "oblique":
            location, target, lens = (4.3, -6.2, 3.2), (0, 3.8, 1.75), 35
        elif view == "edge":
            location, target, lens = (5.8, -1.9, 2.3), (0.25, 3.8, 2.0), 28
    else:
        location, target, lens = (0, -7.9, 1.8), (0, 0, 1.55), 49
        if view == "oblique":
            location, target, lens = (3.8, -6.8, 2.7), (0, 0.12, 1.55), 48
        elif view == "edge":
            location, target, lens = (5.8, -1.5, 1.85), (0, 0.1, 1.55), 43
    data = bpy.data.cameras.new("Laboratory camera")
    obj = bpy.data.objects.new("Laboratory camera", data)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    data.lens = lens
    data.clip_start, data.clip_end = 0.05, 100
    point_at(obj, target)
    bpy.context.scene.camera = obj
    return obj


def add_area(name, location, target, energy, size, color=(1,1,1)):
    import bpy
    data = bpy.data.lights.new(name, "AREA")
    data.energy, data.shape, data.size, data.color = energy, "DISK", size, color
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.location = location
    obj["cobweb_role"] = "light"
    point_at(obj, target)
    return obj


def make_lights(preset, light):
    import bpy
    world = bpy.data.worlds.new("Low neutral studio ambient")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.17, 0.21, 0.27, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.11
    bpy.context.scene.world = world
    if preset == "corridor":
        if light == "frontal":
            add_area("Front corridor softbox", (0,-3.2,3.0), (0,3.7,2.1), 1000, 3.4)
        elif light == "side":
            add_area("Left corridor rake", (-3.8,2.8,2.8), (0,4.2,2.1), 1000, 3.4)
        else:
            add_area("Rear corridor transmission", (0,9.6,3.1), (0,3.7,2.1), 1000, 3.4)
        add_area("Constant scenery fill", (0,-2,5.8), (0,3,0.6), 170, 5.0, (0.77,0.86,1.0))
    else:
        positions = {"frontal": (0,-3.0,3.0), "side": (-3.6,-0.2,2.7), "back": (0,0.56,2.7)}
        add_area("Controlled silk " + light, positions[light], (0,0,1.55), 430, 2.5)
        add_area("Constant scenery fill", (2,-3.5,4.8), (0,0,1.1), 55, 4.5, (0.78,0.86,1.0))


def build_scene(args):
    import bpy
    import _cycles
    assets = load_assets(args.input)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = args.samples
    # Some distro builds ship Cycles without OpenImageDenoise. Enabling it
    # unconditionally aborts rendering before the first sample.
    scene.cycles.use_denoising = bool(_cycles.with_openimagedenoise)
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = 0.02
    scene.cycles.max_bounces = 6
    scene.cycles.diffuse_bounces = 2
    scene.cycles.glossy_bounces = 3
    scene.cycles.transparent_max_bounces = 64
    scene.cycles.seed = 17
    scene.render.resolution_x, scene.render.resolution_y = args.width, args.height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.render.threads_mode = "FIXED"
    scene.render.threads = 8
    scene.render.filepath = str(args.output.resolve())
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = 0.0
    scene.view_settings.gamma = 1.0
    mesh = bpy.data.meshes.new("All silk islands - one UV mesh")
    mesh.from_pydata(assets["vertices"], [], assets["faces"])
    mesh.update()
    uv = mesh.uv_layers.new(name="UVMap")
    for polygon, coords in zip(mesh.polygons, assets["face_uvs"]):
        for loop, coord in zip(polygon.loop_indices, coords):
            uv.data[loop].uv = coord
    obj = bpy.data.objects.new("SILK single atlas mesh", mesh)
    bpy.context.collection.objects.link(obj)
    obj["cobweb_role"] = "silk"
    obj["representation"] = "One UV mesh carrying all islands; mathematical texture coverage"
    mesh.materials.append(silk_material(assets, args.view == "debug"))
    make_scenery(assets, args.view == "debug")
    camera = make_camera(assets["metadata"]["preset"], args.view)
    make_lights(assets["metadata"]["preset"], args.light)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    report = {"preset": assets["metadata"]["preset"], "view": args.view, "light": args.light,
              "blender_version": bpy.app.version_string, "engine": "CYCLES", "device": "CPU",
              "samples": args.samples, "resolution": [args.width,args.height], "exposure": 0.0,
              "denoising": scene.cycles.use_denoising,
              "silk_mesh_objects": 1, "silk_vertices": len(mesh.vertices), "silk_faces": len(mesh.polygons),
              "alpha": "Unmodified coverage.tga RGB, independent of light and view",
              "coverage_sha256": hashlib.sha256((assets["directory"] / "coverage.tga").read_bytes()).hexdigest(),
              "silk_model": "Approximate anisotropic sheen plus controlled thin-sheet translucency; not measured physical silk",
              "orientation": "RG decodes weighted cos(2theta),sin(2theta); half atan2 drives anisotropy rotation",
              "camera_position": list(camera.location), "input": str(assets["directory"]),
              "output": str(args.output.resolve()), "debug_overlay": args.view == "debug"}
    scene["cobweb_render_contract"] = json.dumps(report)
    return {"assets": assets, "silk": obj, "report": report}


def main():
    import bpy
    args = parse_args()
    validate_output(args)
    start = time.monotonic()
    result = build_scene(args)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.save_blend:
        bpy.ops.file.pack_all()
        bpy.ops.wm.save_as_mainfile(filepath=str(args.output.with_suffix(".blend").resolve()))
    if not args.validate_only:
        bpy.ops.render.render(write_still=True)
    result["report"]["rendered"] = not args.validate_only
    result["report"]["elapsed_seconds"] = round(time.monotonic()-start, 3)
    report_path = args.output.with_suffix(".render.json")
    report_path.write_text(json.dumps(result["report"], indent=2) + "\n")
    print("JERICO_RENDER_OK " + str(report_path))


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"JERICO_RENDER_ERROR: {error}", file=sys.stderr)
        # Blender normally returns zero for an uncaught Python exception unless
        # --python-exit-code was supplied. Preserve normal CLI failure semantics.
        raise SystemExit(1)
