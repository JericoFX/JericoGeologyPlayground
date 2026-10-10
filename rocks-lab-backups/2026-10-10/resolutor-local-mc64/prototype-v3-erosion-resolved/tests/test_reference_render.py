import importlib.util
from pathlib import Path
from types import SimpleNamespace
import unittest
ROOT=Path(__file__).resolve().parents[1]
class ReferenceRender(unittest.TestCase):
 def test_render_wrapper_preserves_documented_ao_and_no_geometry_changes(self):
  path=ROOT/'tools'/'render_reference_studies.py'
  self.assertTrue(path.is_file(), 'new reference renderer is missing')
  spec=importlib.util.spec_from_file_location('rr',path);mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
  r=dict(size=2,relief=.55,backDepth=.3,units='m')
  s=mod.reference_settings(SimpleNamespace(size=800,samples=32),r,'main','neutral','4.3.2')
  self.assertEqual(s['source_geometry_changes'],[])
  self.assertIn('renderer_dependencies',s)
  self.assertEqual(set(s['renderer_dependencies']),{'render_surface_studies.py','render_alveolar_ao.py','render_reference_studies.py'})
  for name,digest in s['renderer_dependencies'].items():self.assertEqual(digest,mod.base.digest(ROOT/'tools'/name))
  self.assertEqual(s['ambient_occlusion']['strength'],.30)
  self.assertEqual(s['ambient_occlusion']['distance_meters'],.12)
if __name__=='__main__':unittest.main()
