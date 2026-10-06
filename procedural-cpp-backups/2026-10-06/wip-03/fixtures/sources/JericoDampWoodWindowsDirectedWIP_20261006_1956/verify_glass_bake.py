"""Inspect actual GLB embedded base-color pixels at glass cap/wall triangles.

This checks portable baked appearance, not just material declarations. Uses
Pillow for the evidence check only; generation remains standard-library only.
"""
import io,json,math,struct,statistics
from pathlib import Path

def sample_glass(path):
 from PIL import Image
 data=Path(path).read_bytes();chunks={};offset=12
 while offset<len(data):
  size,kind=struct.unpack_from('<II',data,offset);offset+=8;chunks[kind]=data[offset:offset+size];offset+=size
 g=json.loads(chunks[0x4e4f534a]);binary=chunks[0x004e4942]
 def acc(index):
  a=g['accessors'][index];v=g['bufferViews'][a['bufferView']];n={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];fmt='<'+str(n)+{5126:'f',5123:'H',5125:'I',5121:'B'}[a['componentType']];size=struct.calcsize(fmt);start=v.get('byteOffset',0)+a.get('byteOffset',0);stride=v.get('byteStride',size)
  return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]
 images={};groups={'caps':[],'walls':[]};triangles={k:0 for k in groups}
 for mesh in g['meshes']:
  if '.window.glass.shard.' not in mesh.get('name',''):continue
  for p in mesh['primitives']:
   material=g['materials'][p['material']];texture=material['pbrMetallicRoughness']['baseColorTexture']['index'];source=g['textures'][texture]['source']
   if source not in images:
    view=g['bufferViews'][g['images'][source]['bufferView']];start=view.get('byteOffset',0);images[source]=Image.open(io.BytesIO(binary[start:start+view['byteLength']])).convert('RGB')
   image=images[source];uv=acc(p['attributes']['TEXCOORD_0']);norm=acc(p['attributes']['NORMAL']);ids=[v[0] for v in acc(p['indices'])]
   for i in range(0,len(ids),3):
    tri=ids[i:i+3];normal=[sum(norm[v][k] for v in tri)/3 for k in range(3)]
    key='caps' if abs(normal[2])>.999 else 'walls' if abs(normal[2])<.001 else None
    if key is None:continue
    triangles[key]+=1
    u,v=[sum(uv[j][k] for j in tri)/3 for k in range(2)]
    x=min(image.width-1,max(0,int(u*image.width)));y=min(image.height-1,max(0,int(v*image.height)))
    groups[key].append(image.getpixel((x,y)))
 result={k:{'count':len(samples),'median_rgb':[statistics.median(x[c] for x in samples) for c in range(3)]} for k,samples in groups.items()}
 result['file_sha256']=__import__('hashlib').sha256(data).hexdigest();return result

if __name__=='__main__':
 import argparse
 ap=argparse.ArgumentParser();ap.add_argument('after');ap.add_argument('--before');ap.add_argument('--out');args=ap.parse_args()
 result={'after':sample_glass(args.after)}
 if args.before:result['before']=sample_glass(args.before)
 a=result['after'];result['cap_wall_color_distance']=math.dist(a['caps']['median_rgb'],a['walls']['median_rgb']);result['passed']=result['cap_wall_color_distance']>12
 if args.before:
  b=result['before'];result['wall_color_change']=math.dist(a['walls']['median_rgb'],b['walls']['median_rgb']);result['cap_color_change']=math.dist(a['caps']['median_rgb'],b['caps']['median_rgb']);result['passed'] &= result['wall_color_change']>12 and result['cap_color_change']<8
 text=json.dumps(result,indent=2)
 if args.out:Path(args.out).write_text(text+'\n')
 print(text)
 raise SystemExit(0 if result['passed'] else 1)
