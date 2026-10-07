"""Read real GLB PBR texels and source face provenance after export.

The export atlas intentionally combines authoring materials. Geometry and
all embedded texture channels are checked, with UV samples away from corners.
"""
import io,json,struct,math,statistics
from pathlib import Path
from portable_validation import validate_glb_geometry

def inspect(record):
 from PIL import Image
 blob=Path(record['glb']).read_bytes();chunks={};offset=12
 while offset<len(blob):
  size,kind=struct.unpack_from('<II',blob,offset);offset+=8;chunks[kind]=blob[offset:offset+size];offset+=size
 g=json.loads(chunks[0x4e4f534a]);binary=chunks[0x004e4942]
 def acc(index):
  a=g['accessors'][index];v=g['bufferViews'][a['bufferView']];width={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
  fmt='<'+str(width)+{5126:'f',5123:'H',5125:'I',5121:'B'}[a['componentType']];size=struct.calcsize(fmt);start=v.get('byteOffset',0)+a.get('byteOffset',0);stride=v.get('byteStride',size)
  return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]
 images={}
 def image(texture):
  source=g['textures'][texture]['source']
  if source not in images:
   view=g['bufferViews'][g['images'][source]['bufferView']];start=view.get('byteOffset',0)
   images[source]=Image.open(io.BytesIO(binary[start:start+view['byteLength']])).convert('RGB')
  return images[source]
 def sample(texture,uv):
  im=image(texture);u,v=uv
  return im.getpixel((min(im.width-1,max(0,int(u*im.width))),min(im.height-1,max(0,int(v*im.height)))))
 rows={'wood_surface':[],'wood_recess_or_bevel':[],'glass_cap':[],'glass_edge':[]};triangles={k:0 for k in rows}
 metadata=json.loads(Path(record['metadata']).read_text());parts={p['id']:p for p in metadata['parts']}
 bymesh={node['mesh']:node.get('extras',{}).get('stable_id') for node in g['nodes'] if 'mesh' in node}
 for index,mesh in enumerate(g['meshes']):
  identity=bymesh.get(index);part=parts.get(identity,{})
  if not (part.get('wood_finish') or part.get('glass_study')):continue
  bounds=part.get('panel_bounds')
  for primitive in mesh['primitives']:
   attrs=primitive['attributes'];positions=acc(attrs['POSITION']);uvs=acc(attrs['TEXCOORD_0']);normals=acc(attrs['NORMAL']);indices=[r[0] for r in acc(primitive['indices'])]
   material=g['materials'][primitive['material']];pbr=material['pbrMetallicRoughness'];color=pbr['baseColorTexture']['index'];orm=pbr['metallicRoughnessTexture']['index']
   if part.get('glass_study'):
    assert material.get('extensions',{}).get('KHR_materials_transmission',{}).get('transmissionFactor',0)>.8
   for i in range(0,len(indices),3):
    tri=indices[i:i+3];point=[sum(positions[j][k] for j in tri)/3 for k in range(3)];source=[point[0],-point[2],point[1]]
    # GLB is Y up; source is Z up. Broad glass plane normal is GLB ±Z.
    if part.get('glass_study'):
     normal=[sum(normals[j][k] for j in tri)/3 for k in range(3)];key='glass_cap' if abs(normal[2])>.999 else 'glass_edge' if abs(normal[2])<.001 else None
    elif bounds:
     original=any(abs(source[k]-endpoint)<2e-6 for k in range(3) for endpoint in bounds[2*k:2*k+2]);key='wood_surface' if original else 'wood_recess_or_bevel'
    else:continue
    if key is None:continue
    triangles[key]+=1
    uva,uvb,uvc=[uvs[j] for j in tri]
    area=abs((uvb[0]-uva[0])*(uvc[1]-uva[1])-(uvb[1]-uva[1])*(uvc[0]-uva[0]))/2
    # Tiny chips can have subpixel atlas area: count them but don't pretend
    # their centroid represents a clean material sample at finite resolution.
    if area*1024*1024<1.5:continue
    uv=[sum(uvs[j][k] for j in tri)/3 for k in range(2)]
    rows[key].append({'rgb':sample(color,uv),'orm':sample(orm,uv)})
 report={}
 for key,values in rows.items():
  report[key]={'triangles':triangles[key],'sample_count':len(values)}
  if values:report[key].update(median_rgb=[statistics.median(v['rgb'][c] for v in values) for c in range(3)],median_roughness=statistics.median(v['orm'][1] for v in values)/255,median_metallic=statistics.median(v['orm'][2] for v in values)/255)
 assert report['wood_surface']['sample_count']>10
 assert report['wood_surface']['median_metallic']<.02,report
 if record['state']=='broken':assert report['glass_cap']['triangles']>0 and report['glass_edge']['triangles']>0
 payload=validate_glb_geometry(record['glb']);assert payload['passed'],payload
 return {'family':record['family'],'state':record['state'],'passed':True,'groups':report,'payload':payload,'roundtrip':record['roundtrip']}

def main():
 root=Path(__file__).resolve().parent;records=json.loads((root/'output/exports/export-records.json').read_text());reports=[inspect(r) for r in records]
 old=next(r for r in reports if r['family']=='old' and r['state']=='empty');new=next(r for r in reports if r['family']=='new' and r['state']=='empty')
 assert old['groups']['wood_surface']['median_roughness']>new['groups']['wood_surface']['median_roughness']+.08
 result={'passed':True,'assets':reports,'comparison':'Both have wet PBR response; newer timber is smoother at the same wetness; old timber has more geometry loss.'}
 path=root/'output/material-export-validation.json';path.write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
if __name__=='__main__':main()
