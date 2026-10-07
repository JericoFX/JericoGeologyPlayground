import importlib.util,pathlib,unittest
class PortableValidationTests(unittest.TestCase):
 def test_portable_mesh_attribute_validator_exists(self):
  self.assertIsNotNone(importlib.util.find_spec('portable_validation'),'Missing binary GLB geometry validator')
 def test_previous_export_detects_real_zero_tangents(self):
  self.assertIsNotNone(importlib.util.find_spec('portable_validation'))
  from portable_validation import validate_glb_geometry
  p=pathlib.Path('output/exports/industrial_solid/industrial_solid_')
  files=list(p.parent.glob('*.glb'))
  if not files:self.skipTest('Historic first-pass export not present')
  report=validate_glb_geometry(files[0]);self.assertFalse(report['passed'])
  self.assertGreater(report['zero_tangent_count'],0)

class TangentRepairTests(unittest.TestCase):
 def fixture(self,path,mirrored=False,degenerate_uv=False,conflict=False,acute=False):
  import json,struct
  streams=[([0,0,0,1,0,0,0,1,0],'VEC3'),([0,0,1]*3,'VEC3'),([0,0,1,0,0,-1 if mirrored else 1] if not degenerate_uv else [0,0]*3,'VEC2'),([0,0,0,1]*3,'VEC4')]
  if acute:streams[0]=([0,0,0,.00001,1,0,0,1,0],'VEC3')
  if conflict:streams=[([0,0,0,1,0,0,0,1,0,-1,0,0,0,-1,0],'VEC3'),([0,0,1]*5,'VEC3'),([0,0,1,0,0,1,1,0,0,-1],'VEC2'),([0,0,0,1]*5,'VEC4'),([0,1,2,0,3,4],'SCALAR')]
  binary=bytearray();views=[];accessors=[]
  for values,kind in streams:
   start=len(binary);raw=struct.pack('<'+('I' if kind=='SCALAR' else 'f')*len(values),*values);binary.extend(raw);views.append({'buffer':0,'byteOffset':start,'byteLength':len(raw)});accessors.append({'bufferView':len(views)-1,'componentType':5125 if kind=='SCALAR' else 5126,'count':len(values)//{'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[kind],'type':kind})
  d={'asset':{'version':'2.0'},'buffers':[{'byteLength':len(binary)}],'bufferViews':views,'accessors':accessors,'meshes':[{'primitives':[{'attributes':{'POSITION':0,'NORMAL':1,'TEXCOORD_0':2,'TANGENT':3}}]}]}
  if conflict:d['meshes'][0]['primitives'][0]['indices']=4
  j=json.dumps(d).encode();j+=b' '*((-len(j))%4);length=12+8+len(j)+8+len(binary)
  path.write_bytes(struct.pack('<4sII',b'glTF',2,length)+struct.pack('<II',len(j),0x4e4f534a)+j+struct.pack('<II',len(binary),0x004e4942)+binary)
 def test_repair_only_singular_tangent_bytes_and_recompute_handedness(self):
  import portable_validation as p,tempfile,pathlib,struct,json
  self.assertTrue(hasattr(p,'repair_singular_tangents'),'Missing robust double derivative tangent repair')
  for mirror in [False,True]:
   with tempfile.TemporaryDirectory() as d:
    path=pathlib.Path(d)/'fixture.glb';self.fixture(path,mirrored=mirror);before=path.read_bytes();r=p.repair_singular_tangents(path);after=path.read_bytes()
    self.assertEqual(r['repaired_vectors'],3);self.assertTrue(p.validate_glb_geometry(path)['passed'])
    self.assertEqual(before[:-48],after[:-48])
    for i in range(3):self.assertEqual(struct.unpack_from('<4f',after,len(after)-48+16*i),(1.,0.,0.,-1. if mirror else 1.))
 def test_repair_rejects_degenerate_uv_without_partial_write(self):
  import portable_validation as p,tempfile,pathlib
  self.assertTrue(hasattr(p,'repair_singular_tangents'))
  with tempfile.TemporaryDirectory() as d:
   path=pathlib.Path(d)/'fixture.glb';self.fixture(path,degenerate_uv=True);before=path.read_bytes()
   with self.assertRaises(ValueError):p.repair_singular_tangents(path)
   self.assertEqual(before,path.read_bytes())

 def test_repair_rejects_shared_vertex_with_conflicting_handedness(self):
  import portable_validation as p,tempfile,pathlib
  with tempfile.TemporaryDirectory() as d:
   path=pathlib.Path(d)/'fixture.glb';self.fixture(path,conflict=True);before=path.read_bytes()
   with self.assertRaisesRegex(ValueError,'handedness'):p.repair_singular_tangents(path)
   self.assertEqual(before,path.read_bytes())
 def test_acute_triangle_uses_robust_angle_and_preserves_valid_frames(self):
  import portable_validation as p,tempfile,pathlib,struct
  with tempfile.TemporaryDirectory() as d:
   path=pathlib.Path(d)/'fixture.glb';self.fixture(path,acute=True);p.repair_singular_tangents(path)
   self.assertTrue(p.validate_glb_geometry(path)['passed']);before=path.read_bytes()
   self.assertEqual(p.repair_singular_tangents(path)['repaired_vectors'],0);self.assertEqual(before,path.read_bytes())
