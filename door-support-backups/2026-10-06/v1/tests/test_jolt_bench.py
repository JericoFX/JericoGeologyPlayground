import csv,json,pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class JoltBenchTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.binary=ROOT/'build/door_jolt_bench'
  cls.result=None;cls.csv=None
  if cls.binary.exists():
   with tempfile.TemporaryDirectory() as d:
    p=subprocess.run([str(cls.binary),d],capture_output=True,text=True)
    if p.returncode:raise AssertionError(p.stdout+p.stderr)
    cls.result=json.loads((pathlib.Path(d)/'jolt_report.json').read_text())
    with (pathlib.Path(d)/'jolt_poses.csv').open() as stream:cls.csv=list(csv.DictReader(stream))
 def setUp(self):self.assertTrue(self.binary.exists(),'Real Jolt benchmark executable is required')
 def test_real_engine_and_pinned_source(self):
  self.assertEqual(self.result['engine'],'Jolt 5.6.0')
  self.assertTrue(self.result['physics_executed'])
  self.assertEqual(self.result['source_asset_hash'],'3e0d55be93a08009d3d0a502b1444d328fc7f7b73294a4937475c3e2340a090c')
 def test_one_hinge_retains_door_without_sag_claim(self):
  self.assertLess(self.result['max_supported_drift_m'],.0015)
  self.assertEqual(self.result['supports_sequence'],[3,1,0])
  self.assertEqual(self.result['frame_shape_updates'],2)
 def test_one_support_is_solver_tested_while_awake(self):
  self.assertEqual(self.result.get('support_change_activation_calls'),1)
  self.assertTrue(all(r['active']=='1' for r in self.csv[120:150]))

 def test_zero_support_removes_constraint_and_gravity_causes_fall(self):
  self.assertEqual(self.result['constraint_removals'],1)
  self.assertEqual(self.result['activation_calls'],1)
  self.assertGreater(self.result['total_drop_m'],.8)
  self.assertLess(self.result['total_drop_m'],1.2)
  self.assertGreater(self.result['floor_contact_count'],0)
  self.assertGreater(self.result['free_fall_drop_0_25_s_m'],.28)
  self.assertLess(self.result['free_fall_drop_0_25_s_m'],.34)
 def test_poses_are_simulated_with_no_position_driving(self):
  self.assertEqual(self.result['position_writes_after_creation'],0)
  self.assertEqual(len(self.csv),601)
  self.assertTrue(all(r['supports']=='3' for r in self.csv[:120]))
  self.assertTrue(all(r['supports']=='1' for r in self.csv[120:240]))
  self.assertTrue(all(r['supports']=='0' for r in self.csv[240:]))
 def test_repeat_run_is_deterministic_in_this_build(self):
  with tempfile.TemporaryDirectory() as d:
   p=subprocess.run([str(self.binary),d],capture_output=True,text=True)
   self.assertEqual(p.returncode,0,p.stderr)
   b=json.loads((pathlib.Path(d)/'jolt_report.json').read_text())
   self.assertEqual(self.result,b)

if __name__=='__main__':unittest.main()
