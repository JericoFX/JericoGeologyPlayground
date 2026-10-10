import copy,json,pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class ReferenceStudies(unittest.TestCase):
 def run_cli(self,recipe,out):
  path=out.parent/'input.json';path.write_text(json.dumps(recipe))
  return subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(path),'--out',str(out)],cwd=out.parent,capture_output=True,text=True)
 def test_independent_replay_controls_and_metadata(self):
  for profile in ['lava_rope_lobes','angular_scree']:
   with self.subTest(profile=profile),tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d);r=json.loads((ROOT/f'recipes/{profile}-48-v15.json').read_text());r['grid']={'cells':128,'width':513,'height':513}
    run=self.run_cli(r,p/'a');self.assertEqual(run.returncode,0,run.stderr)
    self.assertEqual(self.run_cli(r,p/'b').returncode,0)
    self.assertEqual((p/'a/heightfield.obj').read_bytes(),(p/'b/heightfield.obj').read_bytes())
    m=json.loads((p/'a/metrics.json').read_text());self.assertTrue(m['accepted']);self.assertFalse(m['imagePixelsUsed']);self.assertFalse(m['geologicalSimulation']);self.assertEqual(m['baseGeometry'],'new_independent_analytic_evaluator');self.assertEqual(m['clippedLow']+m['clippedHigh'],0)
    control=copy.deepcopy(r);control['featureDepth']=0;self.assertEqual(self.run_cli(control,p/'control').returncode,0);self.assertNotEqual((p/'a/height.u16').read_bytes(),(p/'control/height.u16').read_bytes())
    for key,value in [('generatorVersion','surface-families-v14'),('referenceImage','forbidden.jpg'),('featureDepth',True),('irregularity',-1)]:
     bad=copy.deepcopy(r);bad[key]=value;self.assertEqual(self.run_cli(bad,p/'bad').returncode,2,(key,value))
    key='ridgeWidthMeters' if profile=='lava_rope_lobes' else 'fragmentScaleMeters'
    bad=copy.deepcopy(r);bad[key]=.001;self.assertEqual(self.run_cli(bad,p/'bad-small').returncode,2)
if __name__=='__main__':unittest.main()
