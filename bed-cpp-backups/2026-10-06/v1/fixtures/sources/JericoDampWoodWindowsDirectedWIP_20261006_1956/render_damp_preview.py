"""Low-cost material/edge review before the final six-state render."""
import sys,math,json
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from damp_wood import build_damp_window
from blender_adapter import clear_scene,import_asset,setup_stage,_material
from evidence import freeze_file
from core import asset_hash

def main():
 out=ROOT/'output/evidence';out.mkdir(parents=True,exist_ok=True);clear_scene();assets=[]
 for family,x in [('old',-.83),('new',.83)]:
  s=build_damp_window(dict(family=family,glazing='empty',seed=29));assets.append({'family':family,'hash':asset_hash(s)})
  import_asset(s,family,(x,0,.22))
 scene=setup_stage(camera=(.35,-8,2),target=(0,0,.98),ortho=3.65,resolution=(1250,850),samples=24);scene.render.threads=6
 wallmat=_material('Stage.wall',(.066,.076,.078),.92)[0]
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.6,1.0));o=bpy.context.object;o.name='Stage.wall';o.scale=(4,.08,2.5);o.data.materials.append(wallmat)
 mat=_material('Stage.caption',(.82,.85,.83),.75)[0]
 for body,x,z,size in [('MADERA / HUMEDAD LOCALIZADA',0,1.99,.084),('VIEJA HUMEDA',-.83,.010,.073),('NUEVA HUMEDA',.83,.010,.073)]:
  cu=bpy.data.curves.new('Caption','FONT');cu.body=body;cu.align_x='CENTER';cu.size=size
  ob=bpy.data.objects.new('Stage.caption.'+body,cu);bpy.context.collection.objects.link(ob);ob.location=(x,-.17,z);ob.rotation_euler=(math.pi/2,0,0);cu.materials.append(mat)
 path=out/'damp-r2-preview-working.png';scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)
 record=freeze_file(path,'damp-wood-directed-preview');record.update(assets=assets,samples=24,preview=True)
 (out/'directed-preview-record.json').write_text(json.dumps(record,indent=2));print(json.dumps(record))
if __name__=='__main__':main()
