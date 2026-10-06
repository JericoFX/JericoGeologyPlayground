import unittest
try: import bpy
except ImportError: bpy=None
@unittest.skipIf(bpy is None,'Blender required')
class IndustrialAdapterTests(unittest.TestCase):
 def test_painted_steel_uses_procedural_local_weather(self):
  from industrial import build_industrial
  from blender_adapter import clear_scene,import_asset
  clear_scene(); data=build_industrial({}); obj=import_asset(data)['door.sheet']
  mat=obj.data.materials[0]
  self.assertEqual(mat.get('provenance'),'Original industrial shader, no add-on assets')
  self.assertTrue(any(n.name=='Analytic edge distance' for n in mat.node_tree.nodes))
  self.assertTrue(any(n.name=='Localized lower corrosion' for n in mat.node_tree.nodes))
 def test_material_role_survives_in_mesh(self):
  from industrial import build_industrial
  from blender_adapter import clear_scene,import_asset
  clear_scene(); obj=import_asset(build_industrial({}))['door.sheet']
  self.assertEqual(obj['material_role'],'painted_steel')
 def test_surface_wear_side_uses_distinct_material_without_changing_back(self):
  from industrial import build_industrial
  from blender_adapter import clear_scene,import_asset
  import copy
  clear_scene(); data=build_industrial({});part=next(p for p in data['parts'] if p['id']=='door.sheet')
  x0,x1,y0,y1,z0,z1=part['panel_bounds']
  part['edge_wear']=[{'side':'front','region':{'shape':'box','min':[x0-.01,y0-.001,z0],'max':[x0+.09,y0+.005,z1]},'width_m':.05,'amount':1.,'feather_m':.005,'seed':29,'override_protection':True,'protected_regions':[],'boundary_segments_xz':[[[x0,z0],[x0,z1]]]}]
  obj=import_asset(data)['door.sheet']
  front=[obj.data.materials[p.material_index] for p in obj.data.polygons if p.normal.y<-.9]
  back=[obj.data.materials[p.material_index] for p in obj.data.polygons if p.normal.y>.9]
  self.assertTrue(front and back)
  self.assertTrue(all(m.get('directed_wear_side')=='front' for m in front))
  self.assertTrue(all(m.get('directed_wear_side') is None for m in back))
  self.assertNotEqual(front[0],back[0])
 def test_seed_changes_baked_weather_without_changing_mesh(self):
  from industrial import build_industrial
  from blender_adapter import clear_scene,import_asset
  from export_assets import _prepare_atlas,_bake_maps
  baked=[];parts=[]
  for seed in [29,30]:
   clear_scene();scene=build_industrial({'seed':seed});part=next(p for p in scene['parts'] if p['id']=='door.sheet');parts.append(part['vertices'])
   obj=import_asset(scene)['door.sheet'];_prepare_atlas([obj],64);images=_bake_maps([obj],64,1);baked.append(list(images['basecolor'].pixels))
  self.assertEqual(parts[0],parts[1])
  self.assertGreater(sum(abs(a-b) for a,b in zip(*baked))/len(baked[0]),.002)
 def test_iron_hinge_edge_wear_reaches_adapter_and_keeps_back(self):
  from industrial import build_industrial
  from localized_damage import apply_operations
  from blender_adapter import clear_scene,import_asset
  s=build_industrial({});p=next(p for p in s['parts'] if p['id']=='door.hinge.0.leaf_plate');b=p['panel_bounds']
  op={'type':'edge_wear','part_id':p['id'],'side':'front','region':{'shape':'box','min':[b[0]-.001,b[2]-.001,b[4]-.001],'max':[b[1]+.001,b[3]+.001,b[5]+.001]},'width_m':.01}
  damaged=apply_operations(s,[op]);clear_scene();ob=import_asset(damaged)[p['id']]
  fronts=[ob.data.materials[poly.material_index] for poly in ob.data.polygons if poly.normal.y<-.999]
  backs=[ob.data.materials[poly.material_index] for poly in ob.data.polygons if poly.normal.y>.999]
  self.assertTrue(all(m.get('directed_wear_side')=='front' for m in fronts))
  self.assertTrue(all(m.name=='Mine.iron' for m in backs))
