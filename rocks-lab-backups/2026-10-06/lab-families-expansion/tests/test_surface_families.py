import unittest,json,subprocess,tempfile,pathlib,copy
ROOT=pathlib.Path(__file__).resolve().parents[1]
def recipe(profile):
 r={'schemaVersion':3,'generatorVersion':'surface-families-v13','extractorVersion':'heightfield-solid-v2','family':'heightfield_cliff','units':'m','profile':profile,'seed':48,'size':.64,'relief':.12,'backDepth':.08,'directionDegrees':0,'featureDepth':.7,'fractures':.65,'irregularity':.55,'grid':{'cells':128,'width':513,'height':513}}
 if profile=='limestone_dissolution': r.update(channelSpacingMeters=.13,channelWidthMeters=.024,pocketRadiusMeters=.045,pocketCount=7)
 else:r.update(layerSpacingMeters=.05,plateLengthMeters=.18,thicknessVariation=.65)
 return r
class SurfaceFamilies(unittest.TestCase):
 def run_cli(self,r,path):
  path.parent.mkdir(parents=True,exist_ok=True);p=path.parent/'input.json';p.write_text(json.dumps(r));return subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(p),'--out',str(path)],text=True,capture_output=True)
 def test_new_profiles_roundtrip_and_honest_metadata(self):
  for profile in ['shale_laminated','slate_foliated','limestone_dissolution']:
   with self.subTest(profile=profile),tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d);r=recipe(profile);a=p/'a';result=self.run_cli(r,a);self.assertEqual(result.returncode,0,result.stderr)
    self.assertEqual(json.loads((a/'recipe.json').read_text()),r)
    m=json.loads((a/'metrics.json').read_text());self.assertEqual(m['profile'],profile);self.assertEqual(m['generatorVersion'],'surface-families-v13');self.assertEqual(m['imagePixelsUsed'],False);self.assertEqual(m['geologicalSimulation'],False);self.assertEqual(m['surfaceModel'],'single_valued_closed_backed_patch');self.assertGreaterEqual(m['minimumBackingThicknessMeters'],r['backDepth']);self.assertAlmostEqual(m['meshCellSpacingMeters'],r['size']/r['grid']['cells'])
    self.assertEqual(self.run_cli(r,p/'b').returncode,0);self.assertEqual((a/'heightfield.obj').read_bytes(),(p/'b/heightfield.obj').read_bytes())
    for key,val in [('featureDepth',-1),('featureDepth',True),('directionDegrees',91),('generatorVersion','heightfield-cliff-v11'),('referenceImage','pixels.jpg')]:
     self.assertEqual(self.run_cli({**r,key:val},p/'bad').returncode,2,key)
 def test_family_specific_controls_change_real_samples(self):
  cases={'shale_laminated':[('layerSpacingMeters',.06),('plateLengthMeters',.3),('thicknessVariation',0),('fractures',0)],'slate_foliated':[('layerSpacingMeters',.06),('plateLengthMeters',.3),('thicknessVariation',0),('fractures',0)],'limestone_dissolution':[('channelSpacingMeters',.19),('channelWidthMeters',.03),('pocketRadiusMeters',.07),('pocketCount',0),('fractures',0)]}
  for profile,changes in cases.items():
   with self.subTest(profile=profile),tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d);r=recipe(profile);self.assertEqual(self.run_cli(r,p/'a').returncode,0);base=(p/'a/height.u16').read_bytes()
    for k,v in changes:
     self.assertEqual(self.run_cli({**r,k:v},p/k).returncode,0,k);self.assertTrue((p/k/'height.u16').read_bytes()!=base,k)
 def test_reject_aliased_source_and_realized_support(self):
  for profile in ['shale_laminated','slate_foliated','limestone_dissolution']:
   with self.subTest(profile=profile),tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d);r=recipe(profile);r['grid']={'cells':128,'width':129,'height':129}
    self.assertEqual(self.run_cli(r,p/'bad-source').returncode,2)
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);r=recipe('limestone_dissolution');r['channelWidthMeters']=.0125
   self.assertEqual(self.run_cli(r,p/'bad-width').returncode,2)
 def test_slate_split_support_guard(self):
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);r=recipe('slate_foliated');r['thicknessVariation']=1;r['layerSpacingMeters']=.041667
   self.assertEqual(self.run_cli(r,p/'bad-slate-split').returncode,2)
 def test_reject_underresolved_or_invalid_features(self):
  for profile,key in [('shale_laminated','layerSpacingMeters'),('slate_foliated','layerSpacingMeters'),('limestone_dissolution','channelWidthMeters')]:
   with tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d);r=recipe(profile)
    for v in [0,.00001,False,float('nan')]:self.assertEqual(self.run_cli({**r,key:v},p/'bad').returncode,2,(profile,v))
    bad=copy.deepcopy(r);bad['grid']['cells']=2;self.assertEqual(self.run_cli(bad,p/'coarse').returncode,2)
if __name__=='__main__':unittest.main()
