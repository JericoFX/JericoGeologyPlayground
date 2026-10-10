import unittest,json,subprocess,tempfile,pathlib,copy
ROOT=pathlib.Path(__file__).resolve().parents[1]
def recipe():
 return {'schemaVersion':3,'generatorVersion':'surface-families-v14','extractorVersion':'heightfield-solid-v2','family':'heightfield_cliff','units':'m','profile':'alveolar_blocks','seed':48,'size':2,'relief':.55,'backDepth':.3,'directionDegrees':0,'featureDepth':.85,'fractures':.7,'irregularity':.65,'patchCount':3,'patchRadiusMeters':.22,'alveolusRadiusMeters':.065,'grid':{'cells':256,'width':1025,'height':1025}}
class AlveolarCLI(unittest.TestCase):
 def run_cli(self,r,path):
  p=path.parent/'input.json';p.write_text(json.dumps(r));return subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(p),'--out',str(path)],text=True,capture_output=True)
 def test_profile_roundtrip_and_diagnostic_events(self):
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d)/'out';r=recipe();result=self.run_cli(r,p);self.assertEqual(result.returncode,0,result.stderr)
   self.assertEqual(json.loads((p/'recipe.json').read_text()),r);m=json.loads((p/'metrics.json').read_text());self.assertTrue(m['accepted']);self.assertFalse(m['geologicalSimulation']);self.assertEqual(m['baseControls']['blockScale'],.288)
   a=json.loads((p/'diagnostics.json').read_text());self.assertGreater(a['patchesAccepted'],0);self.assertLessEqual(a['patchesAccepted'],3);self.assertEqual(len(a['events']['patches']),a['patchesAccepted']);self.assertEqual(a['guideSide'],513);self.assertEqual(a['protectedBandCodeLeakage'],0);self.assertIn('including vertices outside',a['protectionDomain'])
 def test_invalid_controls_and_version_rejected(self):
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d)/'bad';r=recipe()
   for k,v in [('patchCount',9),('patchCount',-1),('patchCount',True),('patchRadiusMeters',.19),('alveolusRadiusMeters',.04),('patchRadiusMeters',float('nan')),('featureDepth',False),('generatorVersion','surface-families-v13'),('pocketCount',3),('profile','limestone_dissolution')]:
    self.assertEqual(self.run_cli({**r,k:v},p).returncode,2,(k,v))
   r['grid']['width']=1024;self.assertEqual(self.run_cli(r,p).returncode,2)
if __name__=='__main__':unittest.main()
