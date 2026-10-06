#!/usr/bin/env python3
"""Blender 4.3.2 / Cycles: fixed cloths and opacity; only a real spot light changes."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import time
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'scripts'),str(ROOT/'vendor')]
from contract import CLASSES,FIXED,STATES,asset_identity,require_new
import baseline_renderer as baseline

def parse_args():
    p=argparse.ArgumentParser()
    p.add_argument('--run',required=True)
    p.add_argument('--width',type=int,default=1500)
    p.add_argument('--height',type=int,default=620)
    p.add_argument('--samples',type=int,default=192)
    p.add_argument('--states',default='off,left,center,right,backlight')
    p.add_argument('--animation',type=int,default=0)
    p.add_argument('--validate-only',action='store_true')
    return p.parse_args(sys.argv[sys.argv.index('--')+1:])

def digest(value):
    return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()

def material_signature(mat):
    values=[]
    for n in mat.node_tree.nodes:
        vals=[]
        for s in n.inputs:
            if not hasattr(s,'default_value'):continue
            v=s.default_value
            vals.append((s.name,float(v) if isinstance(v,(int,float)) else str(v) if isinstance(v,str) else list(v)))
        values.append((n.name,n.bl_idname,getattr(n,'operation',None),vals))
    links=[(l.from_node.name,l.from_socket.name,l.to_node.name,l.to_socket.name) for l in mat.node_tree.links]
    return digest([values,links])

def mesh_signature(mesh):
    return digest([[list(v.co) for v in mesh.vertices],[[*p.vertices] for p in mesh.polygons],
       [list(x.uv) for x in mesh.uv_layers.active.data]])

def build(args,run):
    import bpy
    from mathutils import Vector
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    scene=bpy.context.scene
    scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=args.samples
    scene.cycles.use_denoising=False;scene.cycles.use_adaptive_sampling=False;scene.cycles.seed=FIXED['seed']
    scene.cycles.max_bounces=6;scene.cycles.diffuse_bounces=2;scene.cycles.glossy_bounces=3
    scene.cycles.transparent_max_bounces=32
    scene.render.resolution_x=args.width;scene.render.resolution_y=args.height;scene.render.resolution_percentage=100
    scene.render.threads_mode='FIXED';scene.render.threads=8
    scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.render.image_settings.color_depth='8'
    scene.view_settings.view_transform='AgX';scene.view_settings.look='AgX - Medium High Contrast'
    scene.view_settings.exposure=FIXED['exposure'];scene.view_settings.gamma=1
    world=bpy.data.worlds.new('Fixed low ambient');world.use_nodes=True
    world.node_tree.nodes['Background'].inputs['Color'].default_value=FIXED['ambient_color']
    world.node_tree.nodes['Background'].inputs['Strength'].default_value=FIXED['ambient_strength'];scene.world=world
    camera_data=bpy.data.cameras.new('Fixed measurement camera');camera_data.type='ORTHO';camera_data.ortho_scale=FIXED['ortho_scale']
    camera=bpy.data.objects.new('Fixed measurement camera',camera_data);bpy.context.collection.objects.link(camera)
    camera.location=FIXED['camera_position'];baseline.point_at(camera,FIXED['camera_target']);scene.camera=camera
    assets=baseline.load_assets(ROOT/'assets/normal')
    chosen=[i for i,coords in enumerate(assets['face_uvs']) if all(0<=u<.25 and 0<=v<.25 for u,v in coords)]
    original_ids=sorted({v for i in chosen for v in assets['faces'][i]});index={v:i for i,v in enumerate(original_ids)}
    # Reorient the same original corridor patch into a front-facing comparison jig.
    # A fixed shear removes the wall's average tilt; this is a display transform, not simulation.
    def transform(v):
        x,y,z=v
        return (y-1.5,(x+1.55)-.31*(z-2.0),z)
    vertices=[transform(assets['vertices'][i]) for i in original_ids]
    faces=[[index[v] for v in assets['faces'][i]] for i in chosen]
    uvs=[assets['face_uvs'][i] for i in chosen]
    backing=baseline.plain_material('Matte charcoal backing',FIXED['background_color'][:3],.97)
    timber=baseline.plain_material('Dark frame',(.12,.085,.05),.84)
    brass=baseline.plain_material('Anchor pins',(.20,.15,.075),.45,.55)
    baseline.box('Matte backing',(0,4.3,2),(14,.10,5),backing)
    baseline.box('Dark floor',(0,0,-.05),(14,9,.1),backing)
    cloths=[]
    for c in CLASSES:
        mesh=bpy.data.meshes.new('Identical carrier '+c['id']);mesh.from_pydata(vertices,[],faces);mesh.update()
        uv=mesh.uv_layers.new(name='UVMap')
        for poly,coords in zip(mesh.polygons,uvs):
            for loop,coord in zip(poly.loop_indices,coords):uv.data[loop].uv=coord
        obj=bpy.data.objects.new('Cloth '+c['id'],mesh);bpy.context.collection.objects.link(obj);obj.location.x=c['x'];obj['role']='silk'
        mat=baseline.silk_material({'directory':ROOT/'assets'/c['id']});mat.name='Static silk '+c['id']
        bsdf=mat.node_tree.nodes['Silk_Principled']
        bsdf.inputs['Base Color'].default_value=(c['base'],c['base']*1.02,c['base']*1.08,1)
        bsdf.inputs['Roughness'].default_value=c['roughness'];bsdf.inputs['Anisotropic'].default_value=c['anisotropy']
        mat.node_tree.nodes['Controlled_Translucency'].inputs[0].default_value=c['translucency']
        aov=mat.node_tree.nodes.new('ShaderNodeOutputAOV');aov.name='Static coverage audit';aov.aov_name='Coverage'
        mat.node_tree.links.new(mat.node_tree.nodes['Coverage_Mathematical'].outputs['Color'],aov.inputs['Value'])
        mesh.materials.append(mat);cloths.append(obj)
        for j,corner in enumerate(assets['metadata']['patches'][0]['corners']):
            a=Vector(transform(corner));a.x+=c['x']
            baseline.box('Support '+c['id']+str(j),(a.x,a.y+.20,a.z),(.055,.4,.055),brass)
        for x in (-1.47,1.47):baseline.box('Frame '+c['id'],(c['x']+x,.36,2.01),(.09,.16,3.66),timber)
        for z in (.20,3.82):baseline.box('Rail '+c['id'],(c['x'],.36,z),(3.02,.16,.09),timber)
    aov=scene.view_layers[0].aovs.add();aov.name='Coverage';aov.type='VALUE'
    scene.use_nodes=True;tree=scene.node_tree;tree.nodes.clear()
    render=tree.nodes.new('CompositorNodeRLayers')
    composite=tree.nodes.new('CompositorNodeComposite');tree.links.new(render.outputs['Image'],composite.inputs[0])
    mask=tree.nodes.new('CompositorNodeOutputFile');mask.name='Coverage audit export';mask.base_path=str(run)
    mask.format.file_format='OPEN_EXR';mask.format.color_depth='32';mask.format.color_mode='RGB'
    mask.file_slots[0].path='coverage_';tree.links.new(render.outputs['Coverage'],mask.inputs[0])
    light_data=bpy.data.lights.new('Actual flashlight','SPOT');light_data.shadow_soft_size=.035
    light=bpy.data.objects.new('Actual flashlight',light_data);bpy.context.collection.objects.link(light)
    return scene,cloths,light,mask

def set_light(light,state):
    light.location=state['position'];baseline.point_at(light,state['target'])
    light.data.energy=state['energy'];light.data.spot_size=state['spot_size'];light.data.spot_blend=state['spot_blend']

def snapshot(scene,cloths):
    import bpy
    material_checks={}
    for obj in cloths:
        mat=obj.data.materials[0];alpha=mat.node_tree.nodes['Coverage_Only_Alpha']
        edge=alpha.inputs[0].links
        assert len(edge)==1 and edge[0].from_node.name=='Coverage_Mathematical'
        assert all(n.bl_idname!='ShaderNodeEmission' for n in mat.node_tree.nodes)
        assert mat.node_tree.nodes['Silk_Principled'].inputs['Emission Strength'].default_value==0
        assert not mat.animation_data and not mat.node_tree.animation_data
        assert not obj.animation_data and not obj.data.animation_data
        material_checks[obj.name]=material_signature(mat)
    return {'asset_hashes':{c['id']:asset_identity(ROOT/'assets'/c['id']) for c in CLASSES},
      'mesh_hashes':{o.name:mesh_signature(o.data) for o in cloths},'material_hashes':material_checks,
      'transforms':{o.name:[list(row) for row in o.matrix_world] for o in cloths},
      'camera':[list(row) for row in scene.camera.matrix_world],
      'exposure':scene.view_settings.exposure,'ambient':scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value}

def save_linear(scene,path):
    import bpy
    import numpy as np
    fmt=scene.render.image_settings;fmt.file_format='OPEN_EXR';fmt.color_depth='32'
    bpy.data.images['Render Result'].save_render(str(path),scene=scene)
    img=bpy.data.images.load(str(path),check_existing=False)
    vals=np.asarray(img.pixels[:],dtype=np.float32).reshape((img.size[1],img.size[0],4))[::-1,:,:]
    np.save(path.with_suffix('.linear.npy'),vals)
    bpy.data.images.remove(img)
    fmt.file_format='PNG';fmt.color_depth='8'

def main():
    import bpy
    import math
    args=parse_args();run=ROOT/'renders'/args.run;require_new([run]);run.mkdir(parents=True)
    scene,cloths,light,mask=build(args,run)
    bpy.context.view_layer.update()
    baseline_signature=snapshot(scene,cloths)
    assert len(set(baseline_signature['mesh_hashes'].values()))==1
    report={'blender':bpy.app.version_string,'engine':'CYCLES','device':'CPU','samples':args.samples,'resolution':[args.width,args.height],
      'classes':CLASSES,'fixed':FIXED,'contract':baseline_signature,'frames':[],
      'material_limit':'Artistic thin-sheet anisotropic reflection + translucent BSDF; not validated physical silk',
      'orientation_decode':'theta = 0.5 atan2(2G-1, 2R-1); Principled rotation = theta/(2*pi); UV tangent base',
      'source_patch':0,'display_transform':'x = source.y-1.5; y = source.x+1.55-0.31*(source.z-2); z = source.z',
      'coverage_aov_limit':'Camera-filtered AOV. The authoritative light-independent opacity proof is the unchanged asset + material graph.'}
    if args.validate_only:
        report['validated_only']=True
    else:
        states=[(name,STATES[name]) for name in args.states.split(',') if name]
        if args.animation:
            states=[]
            for k in range(args.animation):
                x=-4.65+9.3*k/(args.animation-1)
                state=dict(STATES['center']);state['position']=[x,-5,2.4];state['target']=[x,0,2]
                states.append((f'sweep_{k:03d}',state))
        for frame,(name,state) in enumerate(states,1):
            set_light(light,state);scene.frame_set(frame);bpy.context.view_layer.update()
            current=snapshot(scene,cloths)
            assert current==baseline_signature,'Light sweep changed geometry, materials, assets, camera, or ambient'
            png=run/(name+'.png');require_new([png]);scene.render.filepath=str(png)
            mask.file_slots[0].path=name+'_coverage_'
            t=time.monotonic();bpy.ops.render.render(write_still=True)
            save_linear(scene,run/(name+'.exr'))
            elapsed=time.monotonic()-t
            report['frames'].append({'name':name,'light':state,'scene_identity':digest(current),'seconds':elapsed,'png':png.name})
            (run/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
            print('FLASHLIGHT_FRAME_OK',name,round(elapsed,2),flush=True)
    set_light(light,STATES['center']);bpy.context.view_layer.update();bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(run/'study.blend'))
    (run/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print('FLASHLIGHT_STUDY_OK',run,flush=True)

if __name__=='__main__':
    try:main()
    except Exception:
        import traceback;traceback.print_exc();raise SystemExit(1)
