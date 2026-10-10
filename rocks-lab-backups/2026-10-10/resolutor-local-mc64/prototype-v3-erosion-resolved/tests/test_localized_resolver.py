import json,pathlib,subprocess,unittest,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
class LocalizedResolver(unittest.TestCase):
 def test_305_and_999_are_resolved_by_normal_cli(self):
  for seed in (305,999):
   with self.subTest(seed=seed),tempfile.TemporaryDirectory() as d:
    r={'schemaVersion':2,'seed':seed,'units':'m','generatorVersion':'rocks-v3','extractorVersion':'paris-mc-v1','size':2,'layerThickness':.4,'contrast':.85,'wear':0,'detail':1,'grid':{'cells':64},'family':'irregular_beds'}
    recipe=pathlib.Path(d)/'r.json';recipe.write_text(json.dumps(r));out=pathlib.Path(d)/'out'
    p=subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(recipe),'--out',str(out)],capture_output=True,text=True)
    self.assertEqual(p.returncode,0,p.stderr)
    metrics=json.loads((out/'metrics.json').read_text());self.assertTrue(metrics['accepted'])
    stage=metrics['stages']['base' if seed==305 else 'stratified'];self.assertGreater(stage['ambiguousCells'],0);self.assertEqual(stage['resolvedAmbiguousCells'],stage['ambiguousCells']);self.assertEqual(stage['unresolvedAmbiguousCells'],0);self.assertEqual(stage['resolvedAmbiguousFaces'],1)
 def test_previously_unsupported_neighbor_seed4_is_now_atomic(self):
  # Seed4 is now in the supported single-face class. Negative downstream and
  # mixed-face cases are pinned by atomic_resolution_tests instead of keeping
  # this historical, repairable seed permanently rejected.
  with tempfile.TemporaryDirectory() as d:
   r={'schemaVersion':2,'seed':4,'units':'m','generatorVersion':'rocks-v3','extractorVersion':'paris-mc-v1','size':2,'layerThickness':.4,'contrast':.85,'wear':0,'detail':1,'grid':{'cells':64},'family':'irregular_beds'}
   recipe=pathlib.Path(d)/'r.json';recipe.write_text(json.dumps(r));out=pathlib.Path(d)/'out'
   p=subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(recipe),'--out',str(out)],capture_output=True,text=True)
   self.assertEqual(p.returncode,0,p.stderr)
   m=json.loads((out/'metrics.json').read_text())['stages']['stratified']
   self.assertEqual(m['ambiguousCells'],2);self.assertEqual(m['resolvedAmbiguousCells'],2)
   self.assertEqual(m['resolvedAmbiguousFaces'],1);self.assertEqual(m['unresolvedAmbiguousCells'],0)
   self.assertEqual(m['degenerateFaces'],0);self.assertEqual(m['badEdges'],0);self.assertEqual(m['badVertices'],0)
 def test_inconclusive_field_and_interior_cases_remain_rejected(self):
  for seed in (73,98,111):
   with self.subTest(seed=seed),tempfile.TemporaryDirectory() as d:
    r={'schemaVersion':2,'seed':seed,'units':'m','generatorVersion':'rocks-v3','extractorVersion':'paris-mc-v1','size':2,'layerThickness':.4,'contrast':.85,'wear':0,'detail':1,'grid':{'cells':64},'family':'irregular_beds'}
    recipe=pathlib.Path(d)/'r.json';recipe.write_text(json.dumps(r));out=pathlib.Path(d)/'out'
    p=subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(recipe),'--out',str(out)],capture_output=True,text=True)
    self.assertNotEqual(p.returncode,0);self.assertFalse(out.exists())
    m=json.loads(p.stderr.split(': ',1)[1]);self.assertGreater(m['ambiguousCells'],0)
    self.assertEqual(m['resolvedAmbiguousCells'],0);self.assertEqual(m['resolvedAmbiguousFaces'],0)
    self.assertEqual(m['unresolvedAmbiguousCells'],m['ambiguousCells'])
if __name__=='__main__':unittest.main()
