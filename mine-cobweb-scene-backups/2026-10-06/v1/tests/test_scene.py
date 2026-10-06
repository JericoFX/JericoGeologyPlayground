import pathlib, unittest, importlib.util, math
ROOT=pathlib.Path(__file__).resolve().parents[1]
SCRIPT=ROOT/'scripts/render_mine.py'
class SceneContract(unittest.TestCase):
 def test_renderer_exists(self): self.assertTrue(SCRIPT.is_file(), 'Combined mine scene renderer missing')
 @unittest.skipUnless(SCRIPT.is_file(), 'Renderer absent')
 def test_modules_are_partition_of_unity(self):
  spec=importlib.util.spec_from_file_location('mine',SCRIPT); m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
  for y in [i*.01-5 for i in range(1901)]:
   w=m.module_weights(y);self.assertAlmostEqual(sum(a[1] for a in w),1.,places=12)
   self.assertTrue(all(a[1]>=0 for a in w))
 @unittest.skipUnless(SCRIPT.is_file(), 'Renderer absent')
 def test_closed_surface_winding(self):
  spec=importlib.util.spec_from_file_location('mine',SCRIPT); m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
  v,f=m.closed_grid_mesh(4,7,lambda u,v:(u,v,.2),lambda u,v:(u,v,-.3))
  edges={}
  for face in f:
   for a,b in zip(face,face[1:]+face[:1]): edges[tuple(sorted((a,b)))]=edges.get(tuple(sorted((a,b))),0)+1
  self.assertTrue(all(n==2 for n in edges.values())); self.assertTrue(all(math.isfinite(x) for p in v for x in p))
if __name__=='__main__': unittest.main()
