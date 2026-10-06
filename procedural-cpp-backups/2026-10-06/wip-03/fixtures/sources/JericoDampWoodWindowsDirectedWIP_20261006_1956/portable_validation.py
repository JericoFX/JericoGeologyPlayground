"""Independent stdlib validation of binary glTF mesh payloads, not just nodes."""
import json,struct,math
from pathlib import Path

def validate_glb_geometry(path):
 data=Path(path).read_bytes()
 if len(data)<20 or struct.unpack_from('<4sII',data)!=(b'glTF',2,len(data)):
  raise ValueError('Invalid GLB 2 container')
 chunks={};offset=12
 while offset<len(data):
  size,kind=struct.unpack_from('<II',data,offset);offset+=8
  if offset+size>len(data):raise ValueError('Truncated GLB chunk')
  chunks[kind]=data[offset:offset+size];offset+=size
 document=json.loads(chunks[0x4e4f534a]);binary=chunks.get(0x004e4942,b'')
 widths={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}
 formats={5120:'b',5121:'B',5122:'h',5123:'H',5125:'I',5126:'f'}
 def accessor(index):
  a=document['accessors'][index];v=document['bufferViews'][a['bufferView']]
  if a.get('sparse'):raise ValueError('Sparse accessors unsupported by validator')
  fmt='<'+formats[a['componentType']]*widths[a['type']];size=struct.calcsize(fmt)
  stride=v.get('byteStride',size);start=v.get('byteOffset',0)+a.get('byteOffset',0)
  if stride<size or start+(a['count']-1)*stride+size>len(binary):raise ValueError('Accessor exceeds binary buffer')
  return [struct.unpack_from(fmt,binary,start+i*stride) for i in range(a['count'])]
 errors=[];zero_tangents=zero_triangles=invalid_uv=triangles=vertices=0
 for mesh in document.get('meshes',[]):
  for primitive in mesh['primitives']:
   label=mesh.get('name','unnamed');attrs=primitive['attributes'];p=accessor(attrs['POSITION']);vertices+=len(p)
   n=accessor(attrs['NORMAL']) if 'NORMAL' in attrs else []
   uv=accessor(attrs['TEXCOORD_0']) if 'TEXCOORD_0' in attrs else []
   tan=accessor(attrs['TANGENT']) if 'TANGENT' in attrs else []
   if len(n)!=len(p) or len(uv)!=len(p) or len(tan)!=len(p):errors.append(label+': missing attribute rows');continue
   if any(not all(math.isfinite(x) for x in row) for rows in (p,n,uv,tan) for row in rows):errors.append(label+': non-finite attribute')
   for normal in n:
    length=math.sqrt(sum(x*x for x in normal))
    if abs(length-1)>.01:errors.append(label+': nonunit normal');break
   for tangent,normal in zip(tan,n):
    if abs(sum(a*b for a,b in zip(tangent[:3],normal)))>.01:errors.append(label+': nonorthogonal tangent frame')
    length=math.sqrt(sum(x*x for x in tangent[:3]))
    if length<1e-6:zero_tangents+=1
    elif abs(length-1)>.01 or abs(abs(tangent[3])-1)>.001:errors.append(label+': invalid tangent frame');break
   if primitive.get('mode',4)!=4:errors.append(label+': nontriangle primitive');continue
   indices=[v[0] for v in accessor(primitive['indices'])] if 'indices' in primitive else list(range(len(p)))
   if len(indices)%3:errors.append(label+': incomplete triangle');continue
   for start in range(0,len(indices),3):
    ids=indices[start:start+3];triangles+=1
    if any(i<0 or i>=len(p) for i in ids):errors.append(label+': invalid index');continue
    a,b,c=[p[i] for i in ids];u=[b[k]-a[k] for k in range(3)];v=[c[k]-a[k] for k in range(3)]
    cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
    if sum(x*x for x in cross)<=1e-28:zero_triangles+=1
    a,b,c=[uv[i] for i in ids];det=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    if det==0:invalid_uv+=1
 if zero_tangents:errors.append(str(zero_tangents)+' zero tangent vectors')
 if zero_triangles:errors.append(str(zero_triangles)+' zero area triangles')
 if invalid_uv:errors.append(str(invalid_uv)+' zero area atlas UV triangles')
 return {'passed':not errors,'errors':errors,'vertices':vertices,'triangles':triangles,'zero_tangent_count':zero_tangents,'zero_triangle_count':zero_triangles,'zero_uv_triangle_count':invalid_uv,'method':'Decode binary position, index, normal, tangent and AtlasUV accessors; inspect every triangle and attribute row.'}


def repair_singular_tangents(path):
 """Repair only singular glTF tangent frames using double UV derivatives.

 Float MikkTSpace can round an extremely acute corner angle to zero. This
 fallback uses atan2(cross,dot), per-corner angle weights, Gram-Schmidt against
 the exported normal, and derives handedness from dP/dV. Positions, indices,
 UVs, normals and all already-valid tangent bytes are retained exactly.
 Invalid UVs or geometry fail transactionally before any file is written.
 """
 data=bytearray(Path(path).read_bytes());offset=12;document=None;bin_start=None
 if len(data)<20 or struct.unpack_from('<4sII',data)!=(b'glTF',2,len(data)):raise ValueError('Invalid GLB 2 container')
 while offset<len(data):
  size,kind=struct.unpack_from('<II',data,offset);offset+=8
  if kind==0x4e4f534a:document=json.loads(data[offset:offset+size])
  if kind==0x004e4942:bin_start=offset
  offset+=size
 if document is None or bin_start is None:raise ValueError('GLB needs JSON and binary chunks')
 widths={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4};formats={5121:'B',5123:'H',5125:'I',5126:'f'}
 def read(index):
  a=document['accessors'][index];v=document['bufferViews'][a['bufferView']]
  if a.get('sparse'):raise ValueError('Sparse tangent repair unsupported')
  fmt='<'+formats[a['componentType']]*widths[a['type']];size=struct.calcsize(fmt);stride=v.get('byteStride',size)
  start=bin_start+v.get('byteOffset',0)+a.get('byteOffset',0)
  if start+(a['count']-1)*stride+size>len(data):raise ValueError('Accessor out of bounds')
  return [struct.unpack_from(fmt,data,start+i*stride) for i in range(a['count'])],[start+i*stride for i in range(a['count'])]
 def sub(a,b):return [a[k]-b[k] for k in range(3)]
 def dot(a,b):return sum(x*y for x,y in zip(a,b))
 def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
 def norm(v):return math.sqrt(dot(v,v))
 accum={}
 for mesh in document.get('meshes',[]):
  for primitive in mesh['primitives']:
   attrs=primitive['attributes']
   if 'TANGENT' not in attrs:continue
   p,_=read(attrs['POSITION']);uv,_=read(attrs['TEXCOORD_0']);normals,_=read(attrs['NORMAL']);tangents,addresses=read(attrs['TANGENT'])
   ta=document['accessors'][attrs['TANGENT']]
   if ta['componentType']!=5126 or ta['type']!='VEC4':raise ValueError('TANGENT must be float VEC4')
   bad={i for i,t in enumerate(tangents) if norm(t[:3])<.5 or abs(dot(t[:3],normals[i]))>.01}
   if not bad:continue
   if primitive.get('mode',4)!=4:raise ValueError('Tangent repair needs triangles')
   indices=[i[0] for i in read(primitive['indices'])[0]] if 'indices' in primitive else list(range(len(p)))
   for start in range(0,len(indices),3):
    ids=indices[start:start+3]
    if not bad.intersection(ids):continue
    if len(ids)!=3:raise ValueError('Incomplete triangle')
    a,b,c=[p[i] for i in ids];e1,e2=sub(b,a),sub(c,a)
    ua,ub,uc=[uv[i] for i in ids];du1,dv1=ub[0]-ua[0],ub[1]-ua[1];du2,dv2=uc[0]-ua[0],uc[1]-ua[1]
    det=du1*dv2-du2*dv1
    if abs(det)<=1e-30 or norm(cross(e1,e2))<=1e-14:raise ValueError('Cannot repair tangent on degenerate UV or geometry')
    dpdu=[(e1[k]*dv2-e2[k]*dv1)/det for k in range(3)];dpdv=[(e2[k]*du1-e1[k]*du2)/det for k in range(3)]
    for corner,index in enumerate(ids):
     if index not in bad:continue
     r=sub(p[ids[(corner+1)%3]],p[index]);s=sub(p[ids[(corner+2)%3]],p[index]);weight=math.atan2(norm(cross(r,s)),dot(r,s))
     normal=normals[index];length=norm(normal)
     if not .99<=length<=1.01:raise ValueError('Cannot repair with invalid normal')
     normal=[x/length for x in normal];entry=accum.setdefault(addresses[index],{'u':[0.,0.,0.],'v':[0.,0.,0.],'normal':normal,'signs':set()})
     if norm(sub(entry['normal'],normal))>1e-5:raise ValueError('Shared tangent storage has incompatible normals')
     orientation=dot(cross(normal,dpdu),dpdv)
     if abs(orientation)<1e-15:raise ValueError('Cannot derive incident handedness')
     entry['signs'].add(-1 if orientation<0 else 1)
     if len(entry['signs'])>1:raise ValueError('Shared singular tangent has conflicting handedness; vertex splitting is required')
     for k in range(3):entry['u'][k]+=dpdu[k]*weight;entry['v'][k]+=dpdv[k]*weight
 for address,entry in accum.items():
  n=entry['normal'];u=entry['u'];projection=dot(n,u);t=[u[k]-n[k]*projection for k in range(3)];length=norm(t)
  if not math.isfinite(length) or length<1e-12:raise ValueError('Cannot derive a stable tangent')
  t=[x/length for x in t];orientation=dot(cross(n,t),entry['v'])
  if not math.isfinite(orientation) or abs(orientation)<1e-15:raise ValueError('Cannot derive tangent handedness')
  struct.pack_into('<4f',data,address,*t,-1. if orientation<0 else 1.)
 if accum:Path(path).write_bytes(data)
 return {'repaired_vectors':len(accum),'method':'Double dP/dUV; robust atan2 corner-angle accumulation; normal-orthogonal projection; recomputed handedness','unchanged':'All positions, indices, UVs, normals and already-valid tangent bytes'}
