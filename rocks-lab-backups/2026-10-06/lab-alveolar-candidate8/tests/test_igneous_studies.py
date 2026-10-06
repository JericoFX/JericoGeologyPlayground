import unittest,json,tempfile,pathlib,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from serve import validate_recipe

def recipe(family):
 r=dict(schemaVersion=2,generatorVersion='rocks-v12',extractorVersion='poly-csg-v1' if family=='columnar_basalt' else 'paris-mc-v1',family=family,units='m',seed=48,size=2,layerThickness=.4,contrast=.85,wear=0,detail=0,grid={'cells':64},studySpacing=.45)
 r.update(dict(columnCount=5,columnLean=.08,jointDepth=.4) if family=='columnar_basalt' else dict(roundness=.12,chipping=.35))
 return r
class IgneousStudies(unittest.TestCase):
 def test_service_validation(self):
  for family in ['columnar_basalt','jointed_granite']:
   r=recipe(family);self.assertEqual(validate_recipe(r),r);self.assertEqual(validate_recipe({**r,'layerThickness':.04})['layerThickness'],.04)
   for key,value in [('studySpacing',.29),('studySpacing',True),('detail',1),('wear',1),('extractorVersion','bad')]:
    with self.assertRaises(ValueError):validate_recipe({**r,key:value})
 def test_ambiguous_extreme_is_rejected(self):
  r=recipe('jointed_granite');r.update(seed=4294967295,size=.1,layerThickness=.02,studySpacing=.3,roundness=.06,chipping=0)
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);source=p/'ambiguous.json';source.write_text(json.dumps(r));out=p/'out'
   result=subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(source),'--out',str(out)],capture_output=True,text=True)
   self.assertEqual(result.returncode,2,result.stderr);self.assertIn('"ambiguousCells":2',result.stderr);self.assertFalse((out/'viewer.json').exists())
 def test_cli_replay(self):
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d)
   for family in ['columnar_basalt','jointed_granite']:
    r=recipe(family);path=p/'input.json';path.write_text(json.dumps(r));out=p/family
    cmd=[str(ROOT/'build/rocks_cli'),'generate','--recipe',str(path),'--out',str(out)]
    result=subprocess.run(cmd,capture_output=True,text=True);self.assertEqual(result.returncode,0,result.stderr)
    self.assertEqual(json.loads((out/'recipe.json').read_text()),r);self.assertIn('joint-controlled 3D',json.loads((out/'metrics.json').read_text())['stageSemantics'])
    cmd[-1]=str(p/'replay');cmd[-3]=str(out/'recipe.json');result=subprocess.run(cmd,cwd=p,capture_output=True,text=True);self.assertEqual(result.returncode,0,result.stderr)
    self.assertEqual((out/'stratified.obj').read_bytes(),(p/'replay/stratified.obj').read_bytes())
if __name__=='__main__':unittest.main()
