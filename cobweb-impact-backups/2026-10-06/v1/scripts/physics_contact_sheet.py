#!/usr/bin/env python3
"""Compose two actual Blender evidence PNGs without external dependencies.

blender -b --python scripts/physics_contact_sheet.py -- \
    --left before.png --right after.png --output comparison.png \
    --note 'Separate assay endpoints; each class has its own failure tick.'

The images are emission-textured flat cards, under Standard color management,
so this script adds only the background and labels. It does not repaint silk,
retouch tearing, or change the images' exposure.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys


def main():
    import bpy
    argv = sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--left", type=Path)
    p.add_argument("--images", type=Path, nargs="+", help="Three to six evidence PNGs for a 3x2 sheet")
    p.add_argument("--labels", nargs="+", help="One label per --images entry")
    p.add_argument("--right", type=Path)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--title", default="ENSAYO DE TRES CLASES DE SEDA")
    p.add_argument("--left-label", default="ANTES")
    p.add_argument("--right-label", default="DESPUES: EXTREMOS DE ENSAYOS INDEPENDIENTES")
    p.add_argument("--note", action="append", default=[])
    p.add_argument("--overwrite", action="store_true")
    p.add_argument("--class-labels", action="store_true", help="Label z=3/2/1 bench rows with the fixed 3.8 m camera")
    args = p.parse_args(argv)
    if args.output.exists() and not args.overwrite:
        raise FileExistsError(args.output)
    grid = bool(args.images)
    sources = args.images if grid else [args.left, args.right]
    if grid and (not 3 <= len(sources) <= 6 or not args.labels or len(args.labels) != len(sources)):
        p.error("Grid requires three to six --images and one --labels entry for each")
    if not grid and not (args.left and args.right):
        p.error("Provide --left and --right, or --images with --labels")
    if len(args.note) > (3 if grid else 4):
        p.error("Too many note lines for this contact-sheet layout")
    for image in sources:
        if not image.is_file():
            raise FileNotFoundError(image)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.eevee.taa_render_samples = 16
    scene.render.resolution_x, scene.render.resolution_y = (2400, 1650) if grid else (2300, 1300)
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.threads_mode, scene.render.threads = "FIXED", 8
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    scene.view_settings.exposure, scene.view_settings.gamma = 0.0, 1.0
    world = bpy.data.worlds.new("Contact sheet background")
    world.use_nodes = True
    # These are linear values, yielding a dark neutral sRGB background.
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.007, 0.009, 0.012, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    scene.world = world
    camdata = bpy.data.cameras.new("Contact sheet orthographic camera")
    camera = bpy.data.objects.new("Contact sheet orthographic camera", camdata)
    bpy.context.collection.objects.link(camera)
    camera.location = (0, 0, 5)
    camdata.type, camdata.ortho_scale = "ORTHO", 2.4 if grid else 2.3
    scene.camera = camera

    def emission(name, color):
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        tree = mat.node_tree
        tree.nodes.clear()
        shader = tree.nodes.new("ShaderNodeEmission")
        shader.inputs["Color"].default_value = (*color, 1)
        shader.inputs["Strength"].default_value = 1
        output = tree.nodes.new("ShaderNodeOutputMaterial")
        tree.links.new(shader.outputs[0], output.inputs["Surface"])
        return mat, shader

    textmat, _ = emission("Neutral label ink", (0.74, 0.79, 0.82))
    smallmat, _ = emission("Secondary label ink", (0.39, 0.45, 0.51))

    def label(body, location, size, mat):
        curve = bpy.data.curves.new(body, "FONT")
        curve.body, curve.size, curve.space_character = body, size, 1.06
        obj = bpy.data.objects.new(body, curve)
        bpy.context.collection.objects.link(obj)
        obj.location = (*location, 0.025)
        obj.data.materials.append(mat)

    image_info = []
    for index, source in enumerate(sources):
        image = bpy.data.images.load(str(source.resolve()))
        image.colorspace_settings.name = "sRGB"
        width, height = image.size
        if abs(width/height-1100/900) > 0.01:
            raise ValueError("Contact sheet expects 1100:900 evidence images; it will not stretch another aspect ratio")
        position = (-0.8 + 0.8*(index % 3), 0.36 if index < 3 else -0.34, 0) if grid else (-0.56 if index == 0 else 0.56, 0.03, 0)
        bpy.ops.mesh.primitive_plane_add(size=1, location=position)
        plane = bpy.context.object
        plane.name = "Unmodified evidence image " + source.name
        plane.scale = (0.76, 0.76*900/1100, 1) if grid else (1.1, 0.9, 1)
        mat, shader = emission("Image " + str(index), (1, 1, 1))
        tex = mat.node_tree.nodes.new("ShaderNodeTexImage")
        tex.image, tex.interpolation = image, "Linear"
        mat.node_tree.links.new(tex.outputs["Color"], shader.inputs["Color"])
        plane.data.materials.append(mat)
        image_info.append({"path": str(source.resolve()), "sha256": hashlib.sha256(source.read_bytes()).hexdigest()})
    if grid:
        label(args.title, (-1.17, 0.752), 0.045, textmat)
        for index, title in enumerate(args.labels):
            label(title, (-1.18+0.8*(index%3), 0.689 if index<3 else -0.011), 0.026, textmat)
    else:
        label(args.title, (-1.10, 0.583), 0.046, textmat)
        label(args.left_label, (-1.10, 0.500), 0.031, textmat)
        label(args.right_label, (0.020, 0.505), 0.026, textmat)
    if args.class_labels:
        for word, y in (("MONSTRUOSA", 0.319), ("NORMAL", 0.030), ("TENUE", -0.259)):
            for x in (-1.085, 0.035):
                label(word, (x, y), 0.028, textmat)
    for index, note in enumerate(args.note):
        label(note, (-1.17, -0.714-index*0.038) if grid else (-1.10, -0.478-index*0.038), 0.025 if grid else 0.027, smallmat)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    scene.render.filepath = str(args.output.resolve())
    bpy.ops.render.render(write_still=True)
    report = {"images": image_info, "title": args.title, "grid": grid, "labels": args.labels, "left_label": args.left_label,
              "right_label": args.right_label, "notes": args.note, "class_labels": args.class_labels, "unretouched_evidence_composition": True}
    args.output.with_suffix(".sheet.json").write_text(json.dumps(report, indent=2)+"\n")
    print("CONTACT_SHEET " + json.dumps(report))


if __name__ == "__main__":
    main()
