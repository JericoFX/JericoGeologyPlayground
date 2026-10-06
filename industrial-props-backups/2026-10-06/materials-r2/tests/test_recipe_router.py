import importlib.util,unittest,json,pathlib,subprocess,sys,tempfile

class RouterTests(unittest.TestCase):
 def test_router_exists(self):self.assertIsNotNone(importlib.util.find_spec('recipe_router'))
 def test_old_door_recipe_stays_identical(self):
  if importlib.util.find_spec('recipe_router') is None:self.fail('recipe router missing')
  from recipe_router import build_recipe
  from core import build_asset
  self.assertEqual(build_recipe({'kind':'door'}),build_asset({'kind':'door'}))
 def test_approved_reinforced_door_hash_is_unchanged(self):
  from presets import build_preset
  from core import asset_hash
  r=json.loads(pathlib.Path('recipes/door_reinforced.json').read_text())
  self.assertEqual(asset_hash(build_preset(r)),'3e0d55be93a08009d3d0a502b1444d328fc7f7b73294a4937475c3e2340a090c')
 def test_glazing_recipes_route_to_fixed_frame(self):
  if importlib.util.find_spec('recipe_router') is None:self.fail('recipe router missing')
  from recipe_router import build_recipe
  for state in ['intact','broken','empty']:
   s=build_recipe({'kind':'window','glazing':state})
   self.assertEqual(s['metadata']['glazing_state'],state)
   self.assertFalse(any(n['role']=='Leaf' for n in s['nodes']))
 def test_cli_can_select_glazing_state(self):
  with tempfile.TemporaryDirectory() as d:
   out=pathlib.Path(d)/'out.json'
   p=subprocess.run([sys.executable,'generate.py','--kind','window','--glazing','empty','--out',str(out)],capture_output=True,text=True)
   self.assertEqual(p.returncode,0,p.stderr)
   self.assertEqual(json.loads(out.read_text())['metadata']['glazing_state'],'empty')

if __name__=='__main__':unittest.main()
