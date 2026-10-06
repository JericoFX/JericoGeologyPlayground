"""Single-pane fixed window state comparison. No reference-image pixels used."""
import sys,json,hashlib,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from singlepane import build_singlepane
from core import asset_hash
from blender_adapter import clear_scene,import_asset,setup_stage
from evidence import freeze_file,freeze_manifest


def main():
 ap=argparse.ArgumentParser();ap.add_argument('--samples',type=int,default=96);ap.add_argument('--out',default=str(ROOT/'output/evidence'));a=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
 out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True)
 source_hashes={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in ['singlepane.py','core.py','blender_adapter.py','render_singlepane.py']}
 clear_scene();states=['intact','broken','empty'];records=[]
 for index,state in enumerate(states):
  recipe=json.loads((ROOT/('recipes/singlepane_standard_'+state+'.json')).read_text())
  data=build_singlepane(recipe);import_asset(data,state,((index-1)*1.58,0,.28))
  records.append({'state':state,'recipe':data['recipe'],'asset_hash':asset_hash(data),'enabled_glass_socket':next(s['enabled'] for s in data['sockets'] if s['role']=='Glass')})
 scene=setup_stage(camera=(2.6,-9,3.25),target=(0,0,.90),ortho=4.85,resolution=(1800,1000),samples=a.samples)
 for o in bpy.data.objects:
  if o.type=='LIGHT':o.data.energy*=.80
 # Small labels below the comparison, not markings baked into the asset.
 for index,label in enumerate(['VIDRIO ENTERO','VIDRIO ROTO','SOLO MARCO']):
  cu=bpy.data.curves.new(label,'FONT');cu.body=label;cu.align_x='CENTER';cu.size=.074;cu.extrude=0
  ob=bpy.data.objects.new('Caption.'+label,cu);bpy.context.collection.objects.link(ob)
  ob.location=((index-1)*1.58,-.14,.035);ob.rotation_euler=(1.570796,0,0)
  mat=bpy.data.materials.get('caption') or bpy.data.materials.new('caption');mat.diffuse_color=(.65,.62,.56,1);cu.materials.append(mat)
 working=out/'singlepane-comparison-working.png';scene.render.filepath=str(working);bpy.ops.render.render(write_still=True)
 result=freeze_file(working,'singlepane-comparison')
 result.update(variants=records,blender=bpy.app.version_string,samples=a.samples,source_hashes=source_hashes)
 blend=out/'singlepane-working.blend';bpy.ops.wm.save_as_mainfile(filepath=str(blend));result['blend']=freeze_file(blend,'singlepane-authoring')
 print('FROZEN_RENDER '+json.dumps(result),flush=True);print('FROZEN_MANIFEST '+str(freeze_manifest([result],out)),flush=True)

if __name__=='__main__':main()
