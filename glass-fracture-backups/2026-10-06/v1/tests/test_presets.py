import importlib.util
import unittest

class PresetTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(importlib.util.find_spec('presets'),'Reference-guided preset module must exist')
    def test_baseline_unchanged(self):
        import core,presets
        self.assertEqual(presets.build_preset({}),core.build_asset({}))
    def test_reinforced_adds_mass_and_preserves_interactions(self):
        import core,presets
        for kind in ['door','window']:
            base=core.build_asset({'kind':kind})
            data=presets.build_preset({'kind':kind,'preset':'reinforced_timber'})
            self.assertGreater(len(data['parts']),len(base['parts']))
            self.assertEqual(base['nodes'],data['nodes'])
            self.assertEqual(base['sockets'],data['sockets'])
            self.assertEqual(base['grips'],data['grips'])
            self.assertEqual(core.validate_scene(data),[])
            self.assertEqual(data['metadata']['preset'],'reinforced_timber')
            self.assertTrue(any(p['role']=='CornerReinforcement' for p in data['parts']))
            self.assertTrue(any(p['material']=='wood_rough' for p in data['parts']))
    def test_preset_reproducible_and_damage_preserves_missing_support(self):
        import core,presets
        for damage in ['aged','deteriorated','broken']:
            recipe={'preset':'reinforced_timber','damage':damage,'seed':89}
            data=presets.build_preset(recipe)
            self.assertEqual(core.asset_hash(data),core.asset_hash(presets.build_preset(recipe)))
            self.assertEqual(core.validate_scene(data),[])
            if damage=='broken':
                self.assertFalse(any(p['id'].startswith('door.handle.front.') for p in data['parts']))
    def test_hinge_rivets_stand_proud_of_thick_strap(self):
        import presets
        scene=presets.build_preset({'preset':'reinforced_timber'})
        py=scene['metadata']['dimensions']['pivot_front_offset']
        for part in scene['parts']:
            if part['id'].startswith('door.hinge.') and '.rivet.' in part['id']:
                self.assertLess(min(v[1] for v in part['vertices']),py-.016)

    def test_cli_selects_reinforced_recipe(self):
        import subprocess,sys,pathlib,tempfile,json
        with tempfile.TemporaryDirectory() as d:
            out=pathlib.Path(d)/'asset.json'
            result=subprocess.run([sys.executable,'generate.py','--preset','reinforced_timber','--out',str(out)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(json.loads(out.read_text())['metadata']['preset'],'reinforced_timber')

    def test_unknown_preset_rejected(self):
        import presets
        with self.assertRaises(ValueError):presets.build_preset({'preset':'bad'})

if __name__=='__main__':unittest.main()
