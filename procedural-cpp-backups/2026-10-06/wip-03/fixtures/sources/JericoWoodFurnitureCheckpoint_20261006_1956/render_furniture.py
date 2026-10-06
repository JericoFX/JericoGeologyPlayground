"""Real Cycles evidence: procedural geometry, not generated reference art."""
import sys,json,hashlib,argparse,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from furniture import build_furniture,validate_furniture,asset_hash,sweep_clearance
from blender_adapter import clear_scene,import_asset,setup_stage,aim
from evidence import freeze_file,freeze_manifest

def main():
 p=argparse.ArgumentParser();p.add_argument('--preset',choices=['cabinet','workbench'],default='cabinet');p.add_argument('--samples',type=int,default=40);p.add_argument('--clay',action='store_true');p.add_argument('--preview',action='store_true');a=p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
 out=ROOT/'output/evidence';out.mkdir(parents=True,exist_ok=True);clear_scene();records=[]
 spacing=1.10 if a.preset=='cabinet' else 1.93
 for i,fraction in enumerate([0,.58]):
  data=build_furniture(dict(preset=a.preset,open_fraction=fraction));errors=validate_furniture(data)
  if errors:raise ValueError(errors)
  import_asset(data,'asset'+str(i),((i-.5)*spacing,0,0));records.append(dict(recipe=data['recipe'],hash=asset_hash(data),source_validation=errors,clearance=sweep_clearance(data)))
 if a.clay:
  mat=bpy.data.materials.new('Neutral clay');mat.use_nodes=True;bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(.22,.22,.22,1);bs.inputs['Roughness'].default_value=.78
  for ob in bpy.data.objects:
   if ob.type=='MESH':
    for i in range(len(ob.data.materials)):ob.data.materials[i]=mat
 scale=3.08 if a.preset=='cabinet' else 4.25
 scene=setup_stage(camera=(2.5,-7,3.5),target=(0,0,.68 if a.preset=='cabinet' else .42),ortho=scale,resolution=(1600,1000),samples=a.samples)
 # Favor construction readability over near-black concept lighting.
 scene.view_settings.look='AgX - Medium High Contrast';scene.view_settings.exposure=0;scene.cycles.use_denoising=False
 for ob in bpy.data.objects:
  if ob.type=='LIGHT':ob.data.energy*=.76
 for i,label in enumerate(['CERRADO','CAJON ABIERTO 58%']):
  cu=bpy.data.curves.new(label,'FONT');cu.body=label;cu.align_x='CENTER';cu.size=.047 if a.preset=='cabinet' else .055
  ob=bpy.data.objects.new('Caption.'+label,cu);bpy.context.collection.objects.link(ob);ob.location=((i-.5)*spacing,-.59 if a.preset=='cabinet' else -.70,.018);ob.rotation_euler=(1.57079632679,0,0)
 label=a.preset+('-clay' if a.clay else '-reference-increment')+('-preview' if a.preview else '')
 if a.preview:scene.render.resolution_percentage=65;scene.cycles.samples=24
 image=out/(label+'-working.png');scene.render.filepath=str(image);bpy.ops.render.render(write_still=True);record=freeze_file(image,label)
 blend=out/(label+'-working.blend');bpy.ops.wm.save_as_mainfile(filepath=str(blend));record['blend']=freeze_file(blend,label+'-authoring')
 record.update(variants=records,blender=bpy.app.version_string,samples=scene.cycles.samples,source_hashes={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in ROOT.glob('*.py')})
 print('FROZEN_MANIFEST '+str(freeze_manifest([record],out)),flush=True)
if __name__=='__main__':main()
