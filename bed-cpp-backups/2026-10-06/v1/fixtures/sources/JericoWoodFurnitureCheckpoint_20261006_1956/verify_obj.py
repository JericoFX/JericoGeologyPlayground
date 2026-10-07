"""Independent OBJ/MTL checks for the deliverable. Does not modify exports."""
import json,math
from pathlib import Path

def verify(path,expected_triangles=None):
 path=Path(path);positions=[];uv=[];normals=[];faces=[];objects=[];mtl=None;errors=[]
 for line in path.read_text().splitlines():
  fields=line.split()
  if not fields:continue
  if fields[0]=='v':positions.append(tuple(map(float,fields[1:4])))
  elif fields[0]=='vt':uv.append(tuple(map(float,fields[1:3])))
  elif fields[0]=='vn':normals.append(tuple(map(float,fields[1:4])))
  elif fields[0]=='f':faces.append([tuple(int(v)-1 for v in x.split('/')) for x in fields[1:]])
  elif fields[0]=='o':objects.append(fields[1])
  elif fields[0]=='mtllib':mtl=path.parent/fields[1]
 for rows in [positions,uv,normals]:
  if any(not math.isfinite(x) for row in rows for x in row):errors.append('nonfinite data')
 for n in normals:
  if abs(sum(x*x for x in n)-1)>.02:errors.append('nonunit normal');break
 for f in faces:
  if len(f)!=3:errors.append('nontriangle');continue
  for ix in f:
   if not (len(ix)==3 and 0<=ix[0]<len(positions) and 0<=ix[1]<len(uv) and 0<=ix[2]<len(normals)):errors.append('invalid index')
  a,b,c=[positions[ix[0]] for ix in f];u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
  cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
  if sum(x*x for x in cross)<=1e-28:errors.append('zero area triangle')
  a,b,c=[uv[ix[1]] for ix in f]
  if (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])==0:errors.append('zero area UV')
 if expected_triangles is not None and expected_triangles!=len(faces):errors.append('triangle count differs from GLB')
 if not mtl or not mtl.is_file():errors.append('missing material file')
 else:
  for line in mtl.read_text().splitlines():
   fields=line.split()
   if fields and (fields[0].startswith('map_') or fields[0]=='norm') and not (mtl.parent/fields[-1]).is_file():errors.append('missing texture '+fields[-1])
 return dict(passed=not errors,errors=errors,vertices=len(positions),triangles=len(faces),objects=len(objects),uvs=len(uv),normals=len(normals),checks=['finite coordinates','triangle/index validity','nonzero geometry/UV area','unit normals','GLB triangle count agreement','relative MTL/texture links'])

if __name__=='__main__':
 import sys
 print(json.dumps(verify(sys.argv[1]),indent=2))
