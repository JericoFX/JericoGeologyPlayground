"""Content-addressed procedural industrial evidence; never edits announced files."""
import sys,json,hashlib,argparse,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from industrial import build_industrial
from core import asset_hash,validate_scene
from blender_adapter import clear_scene,import_asset,setup_stage
from evidence import freeze_file,freeze_manifest

def main():
 p=argparse.ArgumentParser();p.add_argument('--preset',default='industrial_solid');p.add_argument('--angle',type=float,default=0);p.add_argument('--recipe');p.add_argument('--label',default='metal-first');p.add_argument('--samples',type=int,default=64);p.add_argument('--out',default=str(ROOT/'output/evidence'));p.add_argument('--detail',action='store_true');p.add_argument('--edge-detail',action='store_true');p.add_argument('--clay',action='store_true');p.add_argument('--comparison',choices=['doors','damage','windows']);a=p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
 out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True);clear_scene()
 source_bytes={f.name:f.read_bytes() for f in ROOT.glob('*.py')}
 source_hashes={name:hashlib.sha256(data).hexdigest() for name,data in source_bytes.items()}
 archive=out/(a.label+'-source-working.zip')
 with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
  for name,data in sorted(source_bytes.items()):
   info=zipfile.ZipInfo(name,(2026,10,6,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,data)
 source_snapshot=freeze_file(archive,a.label+'-source')
 recipes=[json.loads(Path(a.recipe).read_text()) if a.recipe else dict(preset=a.preset,angle=a.angle)]
 if a.comparison=='doors':recipes=[dict(preset='industrial_solid'),dict(preset='industrial_vented',angle=0),dict(preset='industrial_vented',angle=67)]
 if a.comparison=='windows':recipes=[dict(preset='industrial_window',glazing=s) for s in ['intact','broken','empty']]
 if a.comparison=='damage':
  left=json.loads((ROOT/'recipes/industrial_damage_left.json').read_text());recipes=[dict(left,operations=[])]+[json.loads((ROOT/('recipes/industrial_damage_'+s+'.json')).read_text()) for s in ['left','right']]
 records=[]
 for i,recipe in enumerate(recipes):
  data=build_industrial(recipe);errors=validate_scene(data)
  if errors:raise ValueError(errors)
  import_asset(data,'asset'+str(i),((i-(len(recipes)-1)/2)*1.52,0,0 if data['recipe']['kind']=='door' else .28))
  records.append(dict(recipe=data['recipe'],asset_hash=asset_hash(data),validation=errors))
 if a.clay:
  mat=bpy.data.materials.new('Diagnostic neutral clay');mat.use_nodes=True;bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(.18,.18,.18,1);bs.inputs['Roughness'].default_value=.82
  for obj in bpy.data.objects:
   if obj.type=='MESH':obj.data.materials.clear();obj.data.materials.append(mat)
 multi=len(recipes)>1
 scene=setup_stage(camera=(1.4,-9,3.2) if multi else (.9,-7,2.35),target=(0,0,1.06),ortho=4.9 if multi else 2.63,resolution=(1800,1000) if multi else (960,1280),samples=a.samples)
 if a.detail:
  from blender_adapter import aim
  scene.camera.location=(-2.5,-6,2.8);aim(scene.camera,(-.06,-.35,1.48));scene.camera.data.ortho_scale=.94
  scene.render.resolution_x=1200;scene.render.resolution_y=1200
 if a.edge_detail:
  from blender_adapter import aim
  scene.camera.location=(-3,-6,1.45);aim(scene.camera,(-.34,-.53,.81));scene.camera.data.ortho_scale=.70;scene.render.resolution_x=1200;scene.render.resolution_y=1200
 if a.comparison=='damage':
  from math import pi
  for i,label in enumerate(['SIN CORTE','CORTE IZQUIERDO','CORTE DERECHO']):
   cu=bpy.data.curves.new(label,'FONT');cu.body=label;cu.align_x='CENTER';cu.size=.058
   ob=bpy.data.objects.new('Caption.'+label,cu);bpy.context.collection.objects.link(ob);ob.location=((i-1)*1.52,-.88,.045);ob.rotation_euler=(pi/2,0,0)
 for o in bpy.data.objects:
  if o.type=='LIGHT':o.data.energy*=.82
 scene.world.color=(.025,.030,.032)
 working=out/(a.label+'-working.png');scene.render.filepath=str(working);bpy.ops.render.render(write_still=True)
 record=freeze_file(working,a.label);record.update(variants=records,blender=bpy.app.version_string,samples=a.samples,source_hashes=source_hashes,source_snapshot=source_snapshot)
 blend=out/(a.label+'-working.blend');bpy.ops.wm.save_as_mainfile(filepath=str(blend));record['blend']=freeze_file(blend,a.label+'-authoring')
 print('FROZEN_RENDER '+json.dumps(record),flush=True);print('FROZEN_MANIFEST '+str(freeze_manifest([record],out)),flush=True)
if __name__=='__main__':main()
