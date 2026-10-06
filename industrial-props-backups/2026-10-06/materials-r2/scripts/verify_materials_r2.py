"""Read-only proof of R2 source and actual binary GLB invariance."""
import collections,hashlib,json,struct,sys
from pathlib import Path
from PIL import Image
import numpy as np
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from portable_validation import validate_glb_geometry
BASE=ROOT.parent/'JericoIndustrialPropsLab/output/exports_verified/industrial-export-manifest_9e7765b7101e.json'
AFFECTED={'industrial_louvered','industrial_vented','industrial_window_broken','industrial_damage_left','industrial_edge_damage','industrial_edges_cracks'}
def sha(b):return hashlib.sha256(b).hexdigest()
def canonical(v):return json.dumps(v,sort_keys=True,separators=(',',':')).encode()
def parse(path):
 b=Path(path).read_bytes();at=12;chunks={}
 while at<len(b):
  size,kind=struct.unpack_from('<II',b,at);at+=8;chunks[kind]=b[at:at+size];at+=size
 return json.loads(chunks[0x4e4f534a]),chunks[0x004e4942]
def mesh_payload(path):
 d,b=parse(path);out={};nodes={}
 def accessor(i):
  a=d['accessors'][i];v=d['bufferViews'][a['bufferView']]
  assert 'sparse' not in a
  width={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
  size=width*{5120:1,5121:1,5122:2,5123:2,5125:4,5126:4}[a['componentType']]
  stride=v.get('byteStride',size);offset=v.get('byteOffset',0)+a.get('byteOffset',0)
  raw=b''.join(b[offset+n*stride:offset+n*stride+size] for n in range(a['count']))
  assert len(raw)==a['count']*size
  return dict(componentType=a['componentType'],type=a['type'],count=a['count'],normalized=a.get('normalized',False),sha256=sha(raw),bytes=len(raw))
 for n in d['nodes']:
  sid=n['extras']['stable_id'];assert sid not in nodes
  nodes[sid]={k:n[k] for k in ['matrix','translation','rotation','scale','extras'] if k in n}
  nodes[sid]['children']=sorted(d['nodes'][i]['extras']['stable_id'] for i in n.get('children',[]))
  if 'mesh' in n:
   out[sid]=[]
   for p in d['meshes'][n['mesh']]['primitives']:
    attrs={k:accessor(v) for k,v in p['attributes'].items()}
    if 'indices' in p:attrs['INDICES']=accessor(p['indices'])
    out[sid].append(dict(mode=p.get('mode',4),attributes=attrs))
 return out,nodes
old=json.loads(BASE.read_text());new=json.loads((ROOT/'output/exports_verified/industrial-export-manifest-working.json').read_text());new={r['label']:r for r in new};records=[]
for a in old:
 label=a['label'];b=new[label];sa=json.loads(Path(a['metadata']).read_text());sb=json.loads(Path(b['metadata']).read_text());sa.pop('export');sb.pop('export');assert sa==sb,label+' source mutation'
 pa,na=mesh_payload(a['glb']);pb,nb=mesh_payload(b['glb']);assert pa==pb,label+' binary mesh mutation';assert na==nb,label+' node mutation'
 validation=validate_glb_geometry(b['glb']);assert validation['passed'],validation;assert b['roundtrip']['passed']
 tex={}
 for k in a['textures']:
  aa=Path(a['textures'][k]);bb=Path(b['textures'][k]);x=np.asarray(Image.open(aa));y=np.asarray(Image.open(bb));assert x.shape==y.shape
  change=np.any(x!=y,axis=2);entry=dict(old_sha256=sha(aa.read_bytes()),new_sha256=sha(bb.read_bytes()),bytes_identical=aa.read_bytes()==bb.read_bytes(),changed_pixels=int(change.sum()),total_pixels=int(change.size),max_abs_channel_delta=int(np.abs(x.astype(int)-y.astype(int)).max()))
  tex[k]=entry
  if label not in AFFECTED:assert entry['bytes_identical'],label+' control map changed '+k
 assert any(not t['bytes_identical'] for t in tex.values())==(label in AFFECTED)
 source_materials=[dict(part_id=p['id'],materials=sorted(set(p.get('face_materials',[])))) for p in sa['parts'] if len(set(p.get('face_materials',[])))>1]
 records.append(dict(label=label,affected=label in AFFECTED,source_sha256=sha(canonical(sa)),source_mesh_nodes_sha256=sha(canonical({'parts':sa['parts'],'nodes':sa['nodes']})),source_exact=True,old_source_asset_hash=a['source_asset_hash'],new_source_asset_hash=b['source_asset_hash'],binary_mesh_exact=True,binary_mesh_sha256=sha(canonical(pa)),binary_nodes_exact=True,binary_nodes_sha256=sha(canonical(na)),mesh_count=len(pa),compared_attributes=sorted({key for ps in pa.values() for p in ps for key in p['attributes']}),texture_comparison=tex,multi_material_parts=source_materials,payload_validation=validation,roundtrip=b['roundtrip'],old_glb_sha256=sha(Path(a['glb']).read_bytes()),new_glb_sha256=sha(Path(b['glb']).read_bytes())))
 print(label, 'SOURCE AND BINARY EXACT', {k:v['changed_pixels'] for k,v in tex.items()},flush=True)
report=dict(scope='Material-assignment regression only; no geometry, shader or recipe changes',baseline_manifest=str(BASE),method='Source sidecars equal after removing export record. GLB byte payloads matched by stable_id, semantic, primitive index; accessor index, byte offset and arbitrary object names excluded. Raw bytes, component types and counts compared for all accessor rows. Nodes matched by stable_id.',pixel_limit='Texture differences counted; no claim of exact outside-region identity until a per-face atlas mask has been measured.',assets=records)
out=ROOT/'output/verification/materials-r2-verification.json';out.write_text(json.dumps(report,indent=2)+'\n');print(out)
