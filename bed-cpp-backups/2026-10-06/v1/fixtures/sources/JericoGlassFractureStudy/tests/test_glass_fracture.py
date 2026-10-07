import unittest, math, collections, json
from industrial import build_industrial
from core import asset_hash

class GlassFractureTests(unittest.TestCase):
 def make(self,**kw):
  from glass_fracture import build_fracture
  r=dict(preset='industrial_window',kind='window',width=1.16,height=1.25,thickness=.052,damage='aged',finish='weathered_gray',glazing='broken',seed=29,angle=0);r.update(kw)
  return build_fracture(r)
 def test_feature_exists(self):
  import importlib.util
  self.assertIsNotNone(importlib.util.find_spec('glass_fracture'),'Original glass fracture core is required')
 def test_frame_stays_exact_and_three_states(self):
  for state in ['broken','intact','empty']:
   actual=self.make(glazing=state);base=build_industrial(actual['recipe']['base_recipe'])
   self.assertEqual([p for p in base['parts'] if p['role']!='GlassPane'],[p for p in actual['parts'] if p['role']!='GlassPane'])
   socket=next(s for s in actual['sockets'] if s['role']=='Glass')
   self.assertEqual(socket['enabled'],state!='empty')
   self.assertEqual(actual['sockets'],base['sockets'])
 def test_partition_area_and_support(self):
  for seed in [29,107]:
   scene=self.make(seed=seed);m=scene['metadata']['glass_fracture']
   self.assertAlmostEqual(m['source_partition_area_m2'],m['pane_area_m2'],places=10)
   self.assertGreater(m['removed_area_m2'],0);self.assertLess(m['retained_area_m2'],m['pane_area_m2'])
   self.assertAlmostEqual(m['retained_area_m2']+m['removed_area_m2'],m['pane_area_m2'],places=10)
   parts=[p for p in scene['parts'] if p['role']=='GlassPane'];self.assertGreater(len(parts),10)
   for p in parts:
    self.assertGreater(p['footprint_area_m2'],0)
    self.assertGreater(p['support_overlap_area_m2'],0)
    self.assertGreater(p['perimeter_bearing_length_m'],.001)
    edges=collections.Counter(tuple(sorted((a,b))) for f in p['faces'] for a,b in zip(f,f[1:]+f[:1]))
    self.assertTrue(all(n==2 for n in edges.values()),p['id'])
    self.assertTrue(all(math.isfinite(v) for uv in p['uvs'] for xy in uv for v in xy))
    for f in p['faces']:
     a,b,c=[p['vertices'][i] for i in f[:3]]
     u=[b[k]-a[k] for k in range(3)];v=[c[k]-a[k] for k in range(3)]
     self.assertGreater(sum((u[(i+1)%3]*v[(i+2)%3]-u[(i+2)%3]*v[(i+1)%3])**2 for i in range(3)),1e-24)
 def test_deterministic_seed_and_no_invented_frame(self):
  self.assertEqual(asset_hash(self.make()),asset_hash(self.make()))
  self.assertNotEqual(asset_hash(self.make()),asset_hash(self.make(seed=107)))
 def test_real_gap_and_hole_collision(self):
  from glass_fracture import point_in_glass
  s=self.make();m=s['metadata']['glass_fracture']
  self.assertFalse(point_in_glass(s,m['impact_xz']))
  self.assertGreater(m['branch_crack_count'],8)
  for sample in m['gap_samples_xz']:self.assertFalse(point_in_glass(s,sample))
  for sample in m['solid_samples_xz']:self.assertTrue(point_in_glass(s,sample))
 def test_reject_invalid_controls(self):
  for kw in [dict(energy=-1),dict(energy=float('nan')),dict(ray_count=3),dict(impact=[20,20])]:
   with self.assertRaises(ValueError):self.make(**kw)

class GlassGeometryOracleTests(GlassFractureTests):
 def test_positive_volume_coherent_winding_and_no_positive_area_overlap(self):
  def cross(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
  def intersect(subject,clip):
   out=list(subject)
   for a,b in zip(clip,clip[1:]+clip[:1]):
    src=out;out=[]
    if not src:break
    for p,q in zip(src,src[1:]+src[:1]):
     dp,dq=cross(a,b,p),cross(a,b,q)
     if dp>=-1e-13:out.append(p)
     if (dp>=0)!=(dq>=0):
      u=dp/(dp-dq);out.append([p[k]+u*(q[k]-p[k]) for k in range(2)])
   return out
  def ar(poly):return abs(sum(a[0]*b[1]-a[1]*b[0] for a,b in zip(poly,poly[1:]+poly[:1])))/2 if len(poly)>2 else 0
  for seed in (29,107):
   parts=[p for p in self.make(seed=seed)['parts'] if p['role']=='GlassPane'];caps=[]
   for p in parts:
    triangles=[];volume=0;edges=collections.Counter()
    for face in p['faces']:
     for a,b in zip(face,face[1:]+face[:1]):edges[(a,b)]+=1
     for j in range(1,len(face)-1):
      a,b,c=[p['vertices'][k] for k in (face[0],face[j],face[j+1])]
      volume+=sum(a[k]*(b[(k+1)%3]*c[(k+2)%3]-b[(k+2)%3]*c[(k+1)%3]) for k in range(3))/6
     if all(p['vertices'][k][1]<0 for k in face):triangles.append([[p['vertices'][k][0],p['vertices'][k][2]] for k in face])
    self.assertAlmostEqual(volume,p['footprint_area_m2']*.004,places=11)
    self.assertTrue(all(edges[(b,a)]==n for (a,b),n in edges.items()))
    caps.append(triangles)
   for i,one in enumerate(caps):
    for other in caps[i+1:]:
     for a in one:
      for b in other:self.assertLess(ar(intersect(a,b)),1e-10)

class GlassDefaultsTests(unittest.TestCase):
 def test_inherits_existing_industrial_defaults(self):
  from glass_fracture import build_fracture
  for state in ['broken','intact','empty']:
   s=build_fracture({'preset':'industrial_window','glazing':state})
   self.assertEqual(s['recipe']['width'],1.16)
   self.assertEqual(s['metadata']['glazing_state'],state)

class GlassSafetyTests(unittest.TestCase):
 def test_known_branch_overlap_seeds_emit_simple_profiles(self):
  import json
  from pathlib import Path
  from glass_fracture import build_fracture
  from damage_math import _profile
  r=json.loads(Path('recipes/industrial_window_broken.json').read_text())
  for seed in [5,15,35,49,53,62,69,81,87]:
   s=build_fracture(dict(r,seed=seed,energy=1 if seed in (15,35) else .66))
   for p in s['parts']:
    if p['role']=='GlassPane':_profile(p['footprint_xz'])
 def test_extreme_gap_configuration_has_no_crossing_shards(self):
  import json
  from pathlib import Path
  from glass_fracture import build_fracture
  r=json.loads(Path('recipes/industrial_window_broken.json').read_text())
  scene=build_fracture(dict(r,seed=29,width=.65,height=.8,energy=.1,ray_count=12,impact=[.3,.25]))
  from damage_math import _cross2
  polygons=[p['footprint_xz'] for p in scene['parts'] if p['role']=='GlassPane']
  for i,p in enumerate(polygons):
   for q in polygons[i+1:]:
    for a,b in zip(p,p[1:]+p[:1]):
     for c,d in zip(q,q[1:]+q[:1]):
      self.assertFalse(_cross2(a,b,c)*_cross2(a,b,d)<-1e-20 and _cross2(c,d,a)*_cross2(c,d,b)<-1e-20)
