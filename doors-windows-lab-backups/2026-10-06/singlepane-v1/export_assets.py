"""Portable, immutable GLB/OBJ exports of the original procedural lab assets.

Run: blender -b --threads 3 --python export_assets.py -- --out output/exports
The JSON sidecar deliberately stays in the source right-handed Z-up basis.
"""
import argparse
import array
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))


def read_glb_json(path):
    data = Path(path).read_bytes()
    magic, version, length = struct.unpack_from('<4sII', data)
    if magic != b'glTF' or version != 2 or length != len(data):
        raise ValueError('Invalid GLB 2 header')
    size, kind = struct.unpack_from('<II', data, 12)
    if kind != 0x4E4F534A:
        raise ValueError('GLB has no leading JSON chunk')
    return json.loads(data[20:20+size].decode('utf8'))


def _freeze(source, out, label, suffix=None):
    """Content-address a completed file; never replace an existing deliverable."""
    source, out = Path(source), Path(out)
    data = source.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    target = out / (label + '_' + digest[:12] + (suffix or source.suffix))
    if target.exists():
        if target.read_bytes() != data:
            raise RuntimeError('Content-address collision: ' + str(target))
    else:
        with target.open('xb') as stream:
            stream.write(data)
    return target


def _select(objects):
    import bpy
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects: obj.select_set(True)
    if objects: bpy.context.view_layer.objects.active = objects[0]


def _prepare_atlas(meshes, size):
    import bpy
    # Apply bevels before UV packing so edge strips have real non-overlapping UVs.
    for obj in meshes:
        _select([obj])
        for modifier in list(obj.modifiers):
            bpy.ops.object.modifier_apply(modifier=modifier.name)
        triangulate=obj.modifiers.new('Portable tangent triangles','TRIANGULATE')
        triangulate.keep_custom_normals=True
        bpy.ops.object.modifier_apply(modifier=triangulate.name)
        metric = [tuple(loop.uv) for loop in obj.data.uv_layers['MetricUV'].data]
        obj.data.uv_layers.remove(obj.data.uv_layers['MetricUV'])
        obj.data.uv_layers.new(name='AtlasUV')
        metric_layer = obj.data.uv_layers.new(name='MetricUV')
        for loop, uv in zip(metric_layer.data, metric): loop.uv = uv
        obj.data.uv_layers.active_index = 0
        obj.data.uv_layers[0].active_render = True
    _select(meshes)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=(2*max(1,min(4,size//256))+2)/size,
                             margin_method='FRACTION', area_weight=0.5, scale_to_bounds=True)
    bpy.ops.object.mode_set(mode='OBJECT')


def _image(name, size, color, color_space):
    import bpy
    image = bpy.data.images.new(name, width=size, height=size, alpha=False)
    image.generated_color = color
    image.colorspace_settings.name = color_space
    return image


def _joined_bake_mesh(meshes):
    """Merge temporary copies for one bake, retaining each source Object space.

    The export objects, hierarchy, UVs and transforms remain untouched.
    Rigid node transforms leave tangent-space normal vectors invariant.
    """
    import bpy
    bpy.context.view_layer.update()
    copies=[]; material_cache={}
    for obj in meshes:
        duplicate=bpy.data.objects.new('BakeOnly.'+obj.name,obj.data.copy())
        bpy.context.collection.objects.link(duplicate)
        duplicate.matrix_world=obj.matrix_world.copy()
        duplicate.data.materials.clear()
        matrix_key=tuple(round(value,10) for row in obj.matrix_world for value in row)
        for material in obj.data.materials:
            key=(material.name,matrix_key)
            if key not in material_cache:
                material_copy=material.copy()
                for node in material_copy.node_tree.nodes:
                    if node.type=='TEX_COORD': node.object=obj
                material_cache[key]=material_copy
            duplicate.data.materials.append(material_cache[key])
        copies.append(duplicate)
    _select(copies)
    bpy.ops.object.join()
    return bpy.context.object


def _bake_maps(meshes, size, samples):
    import bpy
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = samples
    scene.render.threads_mode = 'FIXED'
    scene.render.threads = 3
    scene.render.bake.margin = max(1, min(4, size//256))
    materials = list({slot.material for obj in meshes for slot in obj.material_slots})
    targets = {}
    for material in materials:
        tree = material.node_tree
        target = tree.nodes.new('ShaderNodeTexImage')
        target.name = 'Portable bake target'
        targets[material] = target
    images = {}
    for name, socket_name, color_space in [('basecolor','Base Color','sRGB'),
                                          ('roughness','Roughness','Non-Color'),
                                          ('metallic','Metallic','Non-Color'),
                                          ('normal',None,'Non-Color')]:
        image = _image('Atlas.'+name, size, (.5,.5,1,1) if name == 'normal' else (0,0,0,1), color_space)
        restore = []
        for material in materials:
            tree = material.node_tree
            tree.nodes.active = targets[material]
            targets[material].image = image
            if socket_name:
                output = next(n for n in tree.nodes if n.type == 'OUTPUT_MATERIAL')
                original = output.inputs['Surface'].links[0].from_socket
                bsdf = next(n for n in tree.nodes if n.type == 'BSDF_PRINCIPLED')
                emission = tree.nodes.new('ShaderNodeEmission')
                socket = bsdf.inputs[socket_name]
                if socket.is_linked:
                    tree.links.new(socket.links[0].from_socket, emission.inputs['Color'])
                else:
                    value = socket.default_value
                    emission.inputs['Color'].default_value = tuple(value) if hasattr(value,'__len__') else (value,value,value,1)
                tree.links.new(emission.outputs[0], output.inputs['Surface'])
                restore.append((tree, output, original, emission))
        _select(meshes)
        bpy.ops.object.bake(type='EMIT' if socket_name else 'NORMAL',
                            normal_space='TANGENT', normal_r='POS_X', normal_g='POS_Y', normal_b='POS_Z',
                            use_clear=False, use_selected_to_active=False, uv_layer='AtlasUV')
        for tree, output, original, emission in restore:
            tree.links.new(original, output.inputs['Surface'])
            tree.nodes.remove(emission)
        images[name] = image
    # glTF metallic/roughness: R=1 (no AO), G=roughness, B=metallic.
    count = size*size*4
    rough = array.array('f', [0])*count
    metal = array.array('f', [0])*count
    images['roughness'].pixels.foreach_get(rough)
    images['metallic'].pixels.foreach_get(metal)
    packed = array.array('f', [1])*count
    packed[1::4] = rough[0::4]
    packed[2::4] = metal[0::4]
    images['orm'] = _image('Atlas.orm',size,(1,1,0,1),'Non-Color')
    images['orm'].pixels.foreach_set(packed)
    images['orm'].update()
    return images


def _portable_material(name, images, glass=False):
    import bpy
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    bsdf = tree.nodes.get('Principled BSDF')
    bsdf.inputs['Roughness'].default_value = .85
    if glass:
        bsdf.inputs['Transmission Weight'].default_value = .64
        bsdf.inputs['IOR'].default_value = 1.52
    uv = tree.nodes.new('ShaderNodeUVMap'); uv.uv_map = 'AtlasUV'
    textures = {}
    for key in ['basecolor','orm','normal']:
        node = tree.nodes.new('ShaderNodeTexImage'); node.image = images[key]
        tree.links.new(uv.outputs['UV'],node.inputs['Vector'])
        textures[key] = node
    tree.links.new(textures['basecolor'].outputs['Color'],bsdf.inputs['Base Color'])
    separate = tree.nodes.new('ShaderNodeSeparateColor')
    tree.links.new(textures['orm'].outputs['Color'],separate.inputs['Color'])
    tree.links.new(separate.outputs['Green'],bsdf.inputs['Roughness'])
    tree.links.new(separate.outputs['Blue'],bsdf.inputs['Metallic'])
    normal = tree.nodes.new('ShaderNodeNormalMap'); normal.uv_map = 'AtlasUV'
    tree.links.new(textures['normal'].outputs['Color'],normal.inputs['Color'])
    tree.links.new(normal.outputs['Normal'],bsdf.inputs['Normal'])
    return material


def _write_obj(meshes, path, mtl_name):
    """OBJ vertices are flattened source-basis world coordinates; no live pivots."""
    vertex_offset = uv_offset = 0
    with Path(path).open('w') as stream:
        stream.write('# Original procedural lab geometry; meters; right-handed Z up\nmtllib '+mtl_name+'\n')
        for obj in meshes:
            mesh = obj.data
            mesh.calc_loop_triangles()
            normal_matrix = obj.matrix_world.to_3x3().inverted().transposed()
            stream.write('\no '+obj['stable_id']+'\nusemtl '+obj.data.materials[0].name+'\n')
            for vertex in mesh.vertices:
                p = obj.matrix_world @ vertex.co
                stream.write('v %.9g %.9g %.9g\n' % tuple(p))
            for loop in mesh.uv_layers['AtlasUV'].data:
                stream.write('vt %.9g %.9g\n' % tuple(loop.uv))
            for corner in mesh.corner_normals:
                n = (normal_matrix @ corner.vector).normalized()
                stream.write('vn %.9g %.9g %.9g\n' % tuple(n))
            for triangle in mesh.loop_triangles:
                points = ['%d/%d/%d' % (vertex_offset+v+1,uv_offset+l+1,uv_offset+l+1)
                          for v,l in zip(triangle.vertices,triangle.loops)]
                stream.write('f '+' '.join(points)+'\n')
            vertex_offset += len(mesh.vertices)
            uv_offset += len(mesh.loops)


def _roundtrip(glb, expected):
    import bpy
    from blender_adapter import clear_scene
    clear_scene()
    bpy.ops.import_scene.gltf(filepath=str(glb))
    imported = {obj.get('stable_id'):obj for obj in bpy.context.scene.objects}
    errors = []
    bpy.context.view_layer.update()
    for identity, record in expected.items():
        obj = imported.get(identity)
        if obj is None:
            errors.append('Missing stable ID '+identity); continue
        parent = obj.parent.get('stable_id') if obj.parent else None
        if parent != record['parent']: errors.append('Changed parent '+identity)
        delta = max(abs(obj.matrix_world[i][j]-record['matrix'][i][j]) for i in range(4) for j in range(4))
        if delta > 2e-5: errors.append('Changed world transform '+identity)
        if obj.type == 'MESH' and not obj.data.uv_layers: errors.append('Missing texture UV '+identity)
    if set(imported) != set(expected): errors.append('Object identities differ')
    if errors: raise RuntimeError('GLB roundtrip failed: '+str(errors))
    return dict(passed=True,objects=len(imported),checks=['stable IDs','parent relationships','Z-up world transforms after re-import','texture UV presence'],tolerance_m=2e-5)


def export_asset(data, out, label, texture_size=1024, bake_samples=2):
    import bpy
    from blender_adapter import clear_scene, import_asset
    out = Path(out).resolve(); out.mkdir(parents=True,exist_ok=True)
    if texture_size < 64 or texture_size > 4096: raise ValueError('Texture size must be 64–4096')
    clear_scene()
    objects = import_asset(data,prefix=label)
    meshes = [obj for obj in objects.values() if obj.type == 'MESH']
    if not meshes: raise ValueError('Cannot export an asset without mesh parts')
    _prepare_atlas(meshes,texture_size)
    joined = _joined_bake_mesh(meshes)
    images = _bake_maps([joined],texture_size,bake_samples)
    bpy.data.objects.remove(joined,do_unlink=True)
    with tempfile.TemporaryDirectory(prefix='portable_',dir=out) as temporary:
        tmp = Path(temporary)
        textures = {}
        for key,image in images.items():
            path = tmp/(key+'.png')
            image.filepath_raw = str(path); image.file_format='PNG'; image.save()
            textures[key] = _freeze(path,out,label+'_'+key)
            image.filepath_raw = str(textures[key])
        portable = {False:_portable_material('portable_opaque',images),True:_portable_material('portable_glass',images,True)}
        for obj in meshes:
            is_glass = obj.get('material_role') == 'glass'
            obj.data.materials.clear(); obj.data.materials.append(portable[is_glass])
        bpy.context.view_layer.update()
        expected = {key:dict(parent=obj.parent.get('stable_id') if obj.parent else None,
                            matrix=[list(row) for row in obj.matrix_world]) for key,obj in objects.items()}
        _select(list(objects.values()))
        raw_glb = tmp/'asset.glb'
        bpy.ops.export_scene.gltf(filepath=str(raw_glb),export_format='GLB',use_selection=True,
                                 export_yup=True,export_extras=True,export_animations=False,
                                 export_cameras=False,export_lights=False,export_apply=False,
                                 export_texcoords=True,export_normals=True,export_tangents=True,
                                 export_materials='EXPORT',export_image_format='AUTO')
        glb = _freeze(raw_glb,out,label)
        gltf = read_glb_json(glb)
        if not gltf.get('images') or not gltf.get('textures'):
            raise RuntimeError('Portable GLB unexpectedly has no baked textures')
        mtl = tmp/'asset.mtl'
        with mtl.open('w') as stream:
            for is_glass,material in portable.items():
                stream.write('newmtl '+material.name+'\nKd 1 1 1\nPr 0.85\nPm 0.15\n')
                stream.write('map_Kd '+textures['basecolor'].name+'\n')
                stream.write('map_Pr '+textures['roughness'].name+'\n')
                stream.write('map_Pm '+textures['metallic'].name+'\n')
                stream.write('norm '+textures['normal'].name+'\n')
                if is_glass: stream.write('Ni 1.52\n# Transmission requires GLB KHR_materials_transmission\n')
                stream.write('\n')
        mtl = _freeze(mtl,out,label)
        raw_obj = tmp/'asset.obj'; _write_obj(meshes,raw_obj,mtl.name)
        obj_path = _freeze(raw_obj,out,label)
        roundtrip = _roundtrip(glb,expected)
        metadata = dict(data)
        metadata['export'] = dict(
            glb=glb.name,obj=obj_path.name,mtl=mtl.name,textures={k:p.name for k,p in textures.items()},
            glb_basis=dict(handedness='right',up='+Y',front='+Z'),obj_basis=data['basis'],
            source_to_glb=dict(position='(x, y, z) -> (x, z, -y)',matrix_rows=[[1,0,0],[0,0,1],[0,-1,0]],
                               transforms='Conjugate source transforms with the basis conversion. Socket/grip JSON is unchanged source-basis data.'),
            material_fidelity='baked_pbr_atlas',texture_resolution=[texture_size,texture_size],
            texture_channels=dict(basecolor='sRGB RGB',orm='linear R=1, G=roughness, B=metallic',normal='linear tangent-space +X +Y +Z'),
            uv_channels=dict(TEXCOORD_0='AtlasUV, normalized non-overlapping bake atlas',TEXCOORD_1='MetricUV, original meter-scaled authoring coordinates'),
            geometry='Bevel modifiers applied; GLB preserves hierarchy/local pivots; OBJ is flattened at the recipe angle.',
            limitations=['Finite atlas resolution approximates procedural microdetail; no live Blender shader nodes in exports.',
                         'GLB preserves glass transmission and IOR through material extensions; renderer support varies.',
                         'OBJ has no live hierarchy, sockets or pivots; use this JSON to reconstruct them.',
                         'OBJ PBR map_Pr/map_Pm/norm support is importer-dependent; glass transmission is not preserved in OBJ.',
                         'No ambient-occlusion bake, lightmaps, collision hulls, skeleton, or animation clips.'],
            roundtrip=roundtrip)
        raw_metadata = tmp/'asset.json'
        raw_metadata.write_text(json.dumps(metadata,sort_keys=True,indent=2)+'\n')
        metadata_path = _freeze(raw_metadata,out,label)
    return dict(label=label,glb=str(glb),obj=str(obj_path),metadata=str(metadata_path),mtl=str(mtl),
                textures={k:str(v) for k,v in textures.items()},roundtrip=roundtrip,
                bytes={key:path.stat().st_size for key,path in [('glb',glb),('obj',obj_path),('metadata',metadata_path)]})


def build_export_scene(recipe, preset=None):
    from recipe_router import build_recipe
    recipe=dict(recipe)
    if preset is not None:recipe['preset']=preset
    return build_recipe(recipe)


def default_recipes():
    return [('door_standard',dict(kind='door',width=1.0,height=2.1,thickness=.052,damage='aged',handle='lever',seed=23,angle=0)),
            ('door_large',dict(kind='door',width=1.35,height=2.35,thickness=.064,damage='deteriorated',handle='lever',seed=23,angle=0)),
            ('window_standard',dict(kind='window',width=1.4,height=1.25,thickness=.045,damage='aged',handle='lever',seed=23,angle=0)),
            ('window_large',dict(kind='window',width=1.85,height=1.55,thickness=.052,damage='deteriorated',handle='lever',seed=23,angle=0)),
            ('door_handle_pull',dict(kind='door',width=1.0,height=2.1,thickness=.052,damage='aged',handle='pull',seed=23,angle=0))]


def main():
    from core import build_asset, asset_hash
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',default='output/exports')
    parser.add_argument('--texture-size',type=int,default=1024)
    parser.add_argument('--bake-samples',type=int,default=2)
    parser.add_argument('--preset',choices=['mine_timber','reinforced_timber','single_pane_fixed'],default=None)
    parser.add_argument('--recipe',type=Path,help='Optional custom recipe JSON instead of the five example assets')
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    recipes = [(args.recipe.stem,json.loads(args.recipe.read_text()))] if args.recipe else default_recipes()
    records = []
    for label,recipe in recipes:
        scene = build_export_scene(recipe,args.preset)
        if scene['recipe'].get('preset'): label += '_'+scene['recipe']['preset']
        print('EXPORT_BEGIN '+label+' '+asset_hash(scene),flush=True)
        record = export_asset(scene,args.out,label,args.texture_size,args.bake_samples)
        record['source_asset_hash'] = asset_hash(scene)
        records.append(record)
        print('EXPORT_COMPLETE '+json.dumps(record,sort_keys=True),flush=True)
    out = Path(args.out).resolve()
    with tempfile.TemporaryDirectory(dir=out) as tmp:
        source = Path(tmp)/'manifest.json'
        portable_records=[]
        for record in records:
            entry=dict(record)
            for key in ['glb','obj','metadata','mtl']: entry[key]=Path(entry[key]).name
            entry['textures']={key:Path(path).name for key,path in entry['textures'].items()}
            portable_records.append(entry)
        source.write_text(json.dumps(dict(schema_version=1,assets=portable_records),sort_keys=True,indent=2)+'\n')
        manifest = _freeze(source,out,'export_manifest')
    print('EXPORT_MANIFEST '+str(manifest),flush=True)


if __name__ == '__main__': main()
