#!/usr/bin/env python3
"""Render exported physics geometry without rerunning or cosmetically changing it.

blender -b --python scripts/render_physics.py -- --input run/rest \
    --output renders/rest.png --scene renders/rest.blend

Use the same --view, camera overrides, resolution, engine and samples for every
frame of a collection. No framing, alpha, lighting or width depends on tick,
load, damage, component or event count. Curves and carrier vertices retain the
exported coordinates. Geometry is the actual simulation export; this renderer
is not a simulation and adds no tear effects. Widths are interpreted as metres
across a strand, not radii. Native curves are grouped into three material
classes, not represented by one Blender object per strand.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
import time

CAMERAS = {
    "panel": {"location": (5.4, -1.0, 2.9), "target": (-1.55, 1.50, 1.86), "ortho_scale": 5.35},
    "bench": {"location": (1.20, -7.0, 2.9), "target": (1.0, 0.0, 1.65), "ortho_scale": 4.9},
}
CLASS_COLORS = {
    "tenuous": (0.57, 0.57, 0.54),
    "normal": (0.68, 0.67, 0.62),
    "monstrous": (0.79, 0.77, 0.71),
}


def parse_args(argv=None):
    if argv is None:
        argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--input", type=Path, required=True, help="Folder containing frame.json, and optional mesh.obj + coverage.tga")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--scene", type=Path, help="Optional portable, packed Blender scene")
    parser.add_argument("--view", choices=CAMERAS, default="panel")
    parser.add_argument("--camera", type=float, nargs=3, metavar=("X", "Y", "Z"))
    parser.add_argument("--target", type=float, nargs=3, metavar=("X", "Y", "Z"))
    parser.add_argument("--ortho-scale", type=float)
    parser.add_argument("--width", type=int, default=1100)
    parser.add_argument("--height", type=int, default=900)
    parser.add_argument("--samples", type=int, default=48)
    parser.add_argument("--engine", choices=("eevee", "cycles"), default="eevee")
    parser.add_argument("--overwrite", action="store_true")
    parser.add_argument("--validate-only", action="store_true")
    args = parser.parse_args(argv)
    if args.width < 16 or args.height < 16 or args.samples < 1:
        parser.error("Render dimensions must be at least 16 pixels, samples at least 1")
    if args.ortho_scale is not None and (not math.isfinite(args.ortho_scale) or args.ortho_scale <= 0):
        parser.error("--ortho-scale must be a positive finite number")
    for key in ("camera", "target"):
        if getattr(args, key) and not all(math.isfinite(x) for x in getattr(args, key)):
            parser.error(f"--{key} coordinates must be finite")
    return args


def camera_config(args):
    default = CAMERAS[args.view]
    return {"location": tuple(args.camera or default["location"]),
            "target": tuple(args.target or default["target"]),
            "ortho_scale": args.ortho_scale or default["ortho_scale"]}


def finite_point(point, label):
    if len(point) != 3 or not all(isinstance(x, (int, float)) and math.isfinite(x) for x in point):
        raise ValueError(f"{label}: expected a finite [x,y,z]")


def validate_frame(frame):
    ids = set()
    for p in frame.get("particles", []):
        if p["id"] in ids:
            raise ValueError("Duplicate particle ID")
        ids.add(p["id"])
        finite_point(p["p"], "Particle position")
        finite_point(p.get("rest", p["p"]), "Particle rest position")
    for e in frame.get("edges", []):
        if e["a"] not in ids or e["b"] not in ids:
            raise ValueError(f"Edge {e.get('id')} references a missing particle")
        if not math.isfinite(e.get("width", 0.001)) or e.get("width", 0.001) <= 0:
            raise ValueError("Edge width must be positive and finite")
    for c in frame.get("curves", []):
        if not math.isfinite(c.get("width", 0.001)) or c.get("width", 0.001) <= 0:
            raise ValueError("Curve width must be positive and finite")
        for p in c["points"]:
            finite_point(p, "Curve position")
    if frame.get("sphere"):
        finite_point(frame["sphere"]["center"], "Sphere center")
        radius = frame["sphere"]["radius"]
        if not math.isfinite(radius) or radius <= 0:
            raise ValueError("Sphere radius must be positive and finite")


def active_edges(frame):
    return [edge for edge in frame.get("edges", []) if edge.get("active", True)]


def class_name(kind, width):
    aliases = {"0": "tenuous", "1": "normal", "2": "monstrous", "tenue": "tenuous", "thin": "tenuous", "fine": "tenuous", "monstruosa": "monstrous", "thick": "monstrous"}
    kind = aliases.get(str(kind).lower(), str(kind).lower())
    if kind in CLASS_COLORS:
        return kind
    # Non-class authoring labels such as structural/weft use fixed thresholds.
    return "tenuous" if width < 0.0015 else "normal" if width < 0.006 else "monstrous"


def is_anchor(particle):
    value = particle.get("anchor", False)
    if isinstance(value, bool):
        return value
    return value is not None and (value >= 0 if isinstance(value, (int, float)) else bool(value))


def read_obj(path):
    vertices, uv, faces, corner_uvs = [], [], [], []
    for number, line in enumerate(path.read_text().splitlines(), 1):
        fields = line.split()
        if not fields:
            continue
        if fields[0] == "v":
            point = tuple(map(float, fields[1:4])); finite_point(point, f"OBJ line {number}"); vertices.append(point)
        elif fields[0] == "vt":
            tex = tuple(map(float, fields[1:3]))
            if len(tex) != 2 or not all(math.isfinite(x) for x in tex):
                raise ValueError(f"OBJ line {number}: invalid UV")
            uv.append(tex)
        elif fields[0] == "f":
            face, tex = [], []
            for field in fields[1:]:
                indices = field.split("/")
                if len(indices) < 2 or not indices[1]:
                    raise ValueError(f"OBJ line {number}: missing UV")
                vi, ti = int(indices[0]), int(indices[1])
                vi = vi-1 if vi > 0 else len(vertices)+vi
                ti = ti-1 if ti > 0 else len(uv)+ti
                if not 0 <= vi < len(vertices) or not 0 <= ti < len(uv):
                    raise ValueError(f"OBJ line {number}: index out of bounds")
                face.append(vi); tex.append(uv[ti])
            if len(face) < 3:
                raise ValueError(f"OBJ line {number}: degenerate face")
            faces.append(face); corner_uvs.append(tex)
    return vertices, faces, corner_uvs


def material(name, color, roughness=0.58, metallic=0.0, alpha=1.0):
    import bpy
    result = bpy.data.materials.new(name)
    result.use_nodes = True
    shader = result.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*color, 1)
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Alpha"].default_value = alpha
    if alpha < 1:
        set_hashed_transparency(result)
    return result


def set_hashed_transparency(mat):
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = "DITHERED"  # Blender >= 4.2 replaces HASHED.
    elif hasattr(mat, "blend_method"):
        mat.blend_method = "HASHED"
    if hasattr(mat, "use_transparency_overlap"):
        mat.use_transparency_overlap = False
    mat.use_backface_culling = False


def silk_material(name, color):
    mat = material(name, color, 0.39)
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["IOR"].default_value = 1.54
    if "Sheen Weight" in shader.inputs:
        shader.inputs["Sheen Weight"].default_value = 0.28
        shader.inputs["Sheen Roughness"].default_value = 0.45
    if "Anisotropic" in shader.inputs:
        shader.inputs["Anisotropic"].default_value = 0.35
    mat["description"] = "Art-directed off-white silk shading, not a measured biological BSDF"
    return mat


def make_carrier(directory):
    import bpy
    mesh_path, coverage_path = directory/"mesh.obj", directory/"coverage.tga"
    if not mesh_path.exists() and not coverage_path.exists():
        return {"vertices": 0, "faces": 0, "present": False}
    if not mesh_path.exists() or not coverage_path.exists():
        raise ValueError("Carrier rendering requires both mesh.obj and coverage.tga")
    vertices, faces, corner_uvs = read_obj(mesh_path)
    mesh = bpy.data.meshes.new("Actual exported torn carrier")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    uv = mesh.uv_layers.new(name="UVMap")
    for polygon, coords in zip(mesh.polygons, corner_uvs):
        for loop_index, coord in zip(polygon.loop_indices, coords):
            uv.data[loop_index].uv = coord
    obj = bpy.data.objects.new("Silk coverage on exported support faces", mesh)
    bpy.context.collection.objects.link(obj)
    mat = silk_material("Coverage silk: alpha from RGB coverage only", (0.65, 0.65, 0.60))
    set_hashed_transparency(mat)
    tree = mat.node_tree
    tex = tree.nodes.new("ShaderNodeTexImage")
    tex.name = "Actual exported coverage RGB, NOT opaque TGA alpha"
    tex.image = bpy.data.images.load(str(coverage_path), check_existing=False)
    tex.image.colorspace_settings.name = "Non-Color"
    tex.extension = "CLIP"
    tex.interpolation = "Linear"
    coords = tree.nodes.new("ShaderNodeTexCoord")
    tree.links.new(coords.outputs["UV"], tex.inputs["Vector"])
    tree.links.new(tex.outputs["Color"], tree.nodes.get("Principled BSDF").inputs["Alpha"])
    # Coverage also controls a small, fixed neutral tint range. No global-alpha
    # gain or frame-dependent damage correction is inserted here.
    tint = tree.nodes.new("ShaderNodeMixRGB")
    tint.blend_type = "MIX"
    tint.inputs[1].default_value = (0.52, 0.52, 0.49, 1)
    tint.inputs[2].default_value = (0.75, 0.74, 0.68, 1)
    tree.links.new(tex.outputs["Color"], tint.inputs[0])
    tree.links.new(tint.outputs[0], tree.nodes.get("Principled BSDF").inputs["Base Color"])
    mesh.materials.append(mat)
    obj["role"] = "actual_exported_carrier"
    obj["geometry_source"] = str(mesh_path)
    mat["alpha_source"] = "Unmodified grayscale Color of coverage.tga; image Alpha is intentionally unused"
    return {"vertices": len(vertices), "faces": len(faces), "present": True}


def make_strands(frame):
    import bpy
    curves = frame.get("curves", [])
    source = "actual transformed visual curves"
    if not curves:
        points = {p["id"]: p["p"] for p in frame.get("particles", [])}
        curves = [{"kind": e.get("kind", "normal"), "width": e.get("width", 0.001),
                   "points": [points[e["a"]], points[e["b"]]]} for e in active_edges(frame)]
        source = "active physical segments (no visual curves exported)"
    materials = {name: silk_material("Silk class " + name, color) for name, color in CLASS_COLORS.items()}
    groups = {}
    for name, mat in materials.items():
        data = bpy.data.curves.new("Exported silk " + name, "CURVE")
        data.dimensions = "3D"
        data.resolution_u = 1
        data.bevel_depth = 1.0
        data.bevel_resolution = 0
        data.resolution_u = 1
        data.use_fill_caps = True
        data.materials.append(mat)
        obj = bpy.data.objects.new("Actual silk " + name, data)
        bpy.context.collection.objects.link(obj)
        obj["role"], obj["geometry_source"] = "actual_exported_strands", source
        groups[name] = data
    counts = {name: 0 for name in groups}
    for curve in curves:
        coords = curve["points"]
        if len(coords) < 2:
            continue
        width = curve.get("width", 0.001)
        name = class_name(curve.get("kind", "normal"), width)
        spline = groups[name].splines.new("POLY")
        spline.points.add(len(coords)-1)
        for point, coord in zip(spline.points, coords):
            point.co = (*coord, 1)
            point.radius = width / 2.0
        counts[name] += 1
    return {"source": source, "curves": counts, "diameter_multiplier": 1.0}


def sphere(name, position, radius, mat, segments=20, rings=12):
    import bpy
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, radius=radius, location=position)
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(mat)
    for face in obj.data.polygons:
        face.use_smooth = True
    return obj


def box(name, location, dimensions, mat):
    import bpy
    bpy.ops.mesh.primitive_cube_add(size=1, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    obj.data.materials.append(mat)
    return obj


def point_at(obj, target):
    from mathutils import Vector
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat("-Z", "Y").to_euler()


def add_light(name, position, target, energy, size):
    import bpy
    light = bpy.data.lights.new(name, "AREA")
    light.energy, light.shape, light.size, light.color = energy, "DISK", size, (1.0, 1.0, 1.0)
    obj = bpy.data.objects.new(name, light)
    bpy.context.collection.objects.link(obj)
    obj.location = position
    point_at(obj, target)
    return {"name": name, "position": position, "target": target, "energy": energy, "size": size, "color": [1, 1, 1]}


def build_scene(args, frame):
    import bpy
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT" if args.engine == "eevee" else "CYCLES"
    if args.engine == "eevee" and hasattr(scene, "eevee"):
        scene.eevee.taa_render_samples = args.samples
        if hasattr(scene.eevee, "use_raytracing"):
            scene.eevee.use_raytracing = False
    if args.engine == "cycles":
        scene.cycles.device, scene.cycles.samples, scene.cycles.seed = "CPU", args.samples, 17
        scene.cycles.use_denoising = False
        scene.cycles.transparent_max_bounces = 64
        scene.cycles.max_bounces = 5
    scene.render.resolution_x, scene.render.resolution_y = args.width, args.height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.render.threads_mode, scene.render.threads = "FIXED", 8
    scene.render.filepath = str(args.output.resolve())
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure, scene.view_settings.gamma = 0, 1
    scene.render.fps = 24
    world = bpy.data.worlds.new("Constant neutral laboratory world")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.16, 0.16, 0.16, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.18
    scene.world = world
    camera = camera_config(args)
    camdata = bpy.data.cameras.new("Fixed collection camera")
    camdata.type, camdata.ortho_scale = "ORTHO", camera["ortho_scale"]
    camdata.clip_start, camdata.clip_end = 0.05, 100
    cam = bpy.data.objects.new("Fixed collection camera", camdata)
    bpy.context.collection.objects.link(cam)
    cam.location = camera["location"]
    point_at(cam, camera["target"])
    scene.camera = cam
    dark = material("Neutral charcoal lab backing", (0.016, 0.018, 0.020), 0.95)
    floor = material("Neutral lab bench", (0.034, 0.037, 0.040), 0.80)
    steel = material("Brushed steel stationary anchors", (0.32, 0.35, 0.37), 0.28, 0.84)
    box("Lab bench", (0, 1, -0.125), (30, 30, 0.18), floor)
    if args.view == "panel":
        box("Matte back wall", (-2.85, 1.4, 2.0), (0.1, 15, 12), dark)
        lights = [add_light("Constant broad key", (3.3, -1.1, 5.3), (-1.6, 1.5, 1.8), 1000, 4.3),
                  add_light("Constant silk rim", (-0.9, 4.2, 4.6), (-1.65, 1.5, 1.8), 480, 3.0),
                  add_light("Constant low fill", (3.1, 4.0, 1.2), (-1.65, 1.5, 1.8), 160, 4.0)]
    else:
        box("Matte back wall", (0, 1.0, 2.0), (15, 0.1, 12), dark)
        lights = [add_light("Constant broad key", (-2.5, -4.2, 5.0), (0, 0, 1.3), 1000, 4.3),
                  add_light("Constant silk rim", (3.2, 0.4, 4.0), (0, 0, 1.3), 480, 3.0),
                  add_light("Constant low fill", (2.8, -3.0, 1.0), (0, 0, 1.3), 160, 4.0)]
    markers = set()
    for particle in frame.get("particles", []):
        if is_anchor(particle):
            pos = tuple(particle.get("rest", particle["p"]))
            if pos not in markers:
                marker = sphere("Fixed steel anchor " + str(particle["id"]), pos, 0.047, steel)
                marker["role"] = "fixed_world_anchor"
                markers.add(pos)
    if frame.get("sphere"):
        collider = frame["sphere"]
        mat = material("Subtle transparent kinematic sphere", (0.14, 0.19, 0.23), 0.3, 0.15, 0.13)
        obj = sphere("Scripted kinematic collider, not a silk surface", collider["center"], collider["radius"], mat, 32, 20)
        obj["role"] = "kinematic_sphere"
    carrier = make_carrier(args.input)
    strands = make_strands(frame)
    style = {"camera": camera, "lights": lights, "world_color": [0.16]*3, "world_strength": 0.18,
             "exposure": 0, "gamma": 1, "view_transform": "AgX", "look": "AgX - Medium High Contrast",
             "silk_colors": CLASS_COLORS, "silk_diameter_multiplier": 1.0, "carrier_alpha": "coverage RGB directly",
             "engine": args.engine, "resolution": [args.width, args.height], "samples": args.samples}
    return {"camera_and_style": style, "style_sha256": hashlib.sha256(json.dumps(style, sort_keys=True).encode()).hexdigest(),
            "carrier": carrier, "silk": strands, "stationary_anchor_count": len(markers), "blender_version": bpy.app.version_string}


def main():
    args = parse_args()
    args.input = args.input.resolve()
    output_paths = [args.output, args.output.with_suffix(".render.json")]
    if args.scene:
        output_paths.append(args.scene)
    for path in output_paths:
        if path.exists() and not args.overwrite:
            raise FileExistsError(f"Refusing to replace {path}; choose a new output or pass --overwrite")
        path.parent.mkdir(parents=True, exist_ok=True)
    frame_path = args.input / "frame.json"
    frame = json.loads(frame_path.read_text())
    validate_frame(frame)
    start = time.perf_counter()
    report = build_scene(args, frame)
    report.update({"tick": frame.get("tick"), "event_count": len(frame.get("events", [])),
                   "active_edge_count": len(active_edges(frame)), "input": str(args.input),
                   "renderer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                   "input_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                    for p in (frame_path, args.input/"mesh.obj", args.input/"coverage.tga") if p.exists()}})
    report["scene_build_seconds"] = time.perf_counter()-start
    import bpy
    if args.scene:
        bpy.ops.file.pack_all()
        bpy.ops.wm.save_as_mainfile(filepath=str(args.scene.resolve()))
    if not args.validate_only:
        start = time.perf_counter()
        bpy.ops.render.render(write_still=True)
        report["render_seconds"] = time.perf_counter()-start
    report["rendered"] = not args.validate_only
    args.output.with_suffix(".render.json").write_text(json.dumps(report, indent=2)+"\n")
    print("PHYSICS_RENDER " + json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
