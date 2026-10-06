"""Independent expanded invariant check on the saved scene; this does not render."""
import hashlib
import json
from pathlib import Path
import sys
import bpy
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
from contract import STATES
from render_study import set_light,material_signature,mesh_signature

def digest(x):return hashlib.sha256(json.dumps(x,sort_keys=True).encode()).hexdigest()
def capture(scene):
    objs={}
    for o in scene.objects:
        if o.type=='LIGHT':continue
        objs[o.name]={'type':o.type,'transform':[list(x) for x in o.matrix_world]}
        if o.type=='MESH':objs[o.name]['mesh']=mesh_signature(o.data) if o.data.uv_layers.active else digest([[list(v.co) for v in o.data.vertices],[[*p.vertices] for p in o.data.polygons]])
    return {'objects':objs,'camera':{'type':scene.camera.data.type,'ortho':scene.camera.data.ortho_scale,'lens':scene.camera.data.lens,
      'shift_x':scene.camera.data.shift_x,'shift_y':scene.camera.data.shift_y,'clip_start':scene.camera.data.clip_start,'clip_end':scene.camera.data.clip_end},
      'render':[scene.render.resolution_x,scene.render.resolution_y,scene.render.resolution_percentage,scene.render.pixel_aspect_x,scene.render.pixel_aspect_y,
        scene.cycles.samples,scene.cycles.seed,scene.cycles.use_animated_seed,scene.cycles.use_denoising,scene.cycles.use_adaptive_sampling],
      'world':{'color':list(scene.world.node_tree.nodes['Background'].inputs['Color'].default_value),'strength':scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value},
      'materials':{m.name:material_signature(m) for m in bpy.data.materials if m.use_nodes},
      'texture_bindings':{m.name:[{'node':n.name,'image':n.image.name,'color_space':n.image.colorspace_settings.name,'extension':n.extension,'interpolation':n.interpolation} for n in m.node_tree.nodes if n.type=='TEX_IMAGE'] for m in bpy.data.materials if m.use_nodes}}

run=Path(sys.argv[sys.argv.index('--')+1]).resolve()
bpy.ops.wm.open_mainfile(filepath=str(run/'study.blend'))
scene=bpy.context.scene;lamp=bpy.data.objects['Actual flashlight'];states={};reference=None
for name,state in STATES.items():
    set_light(lamp,state);bpy.context.view_layer.update();captured=capture(scene)
    if reference is None:reference=captured
    assert captured==reference,f'Expanded non-light scene changed for {name}'
    states[name]=digest(captured)
report={'scope':'Saved-scene reconstruction audit: state setter only changes flashlight, no rerender',
        'state_hashes':states,'identical_non_light_scene':True,'captured_fields':reference}
target=run/'expanded_scene_audit.json'
assert not target.exists(),target
target.write_text(json.dumps(report,indent=2)+'\n')
print('EXPANDED_SCENE_AUDIT_OK',list(states),states['off'])
