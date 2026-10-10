import unittest,sys,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
class Collection(unittest.TestCase):
 def test_reports_matched_vertex_geometric_error(self):
  self.assertTrue((ROOT/'tools/collect_alveolar.py').exists(),'Dedicated collector not implemented')
  from collect_alveolar import geometric_error
  self.assertEqual(geometric_error([0,0,0,1,0,0],[0,0,.003,1,0,.004]),{'maxMeters':.004,'rmsMeters':(.000025/2)**.5,'vertices':2})
 def test_rejects_changed_xy_or_topology_size(self):
  self.assertTrue((ROOT/'tools/collect_alveolar.py').exists(),'Dedicated collector not implemented')
  from collect_alveolar import geometric_error
  for a,b in [([0,0,0],[.1,0,0]),([0,0,0],[0,0,0,1,1,1])]:
   with self.assertRaises(ValueError):geometric_error(a,b)
if __name__=='__main__':unittest.main()
