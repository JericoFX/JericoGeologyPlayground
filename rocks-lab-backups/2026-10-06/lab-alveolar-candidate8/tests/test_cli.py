import unittest,json,subprocess,tempfile,pathlib,math
ROOT=pathlib.Path(__file__).resolve().parents[1]
class CLI(unittest.TestCase):
 def run_cli(self,r,out):
  p=out.parent/'input.json';p.write_text(json.dumps(r));return subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(p),'--out',str(out)],capture_output=True,text=True)
 def test_exports_and_rejection(self):
  r=json.loads((ROOT/'recipes/kayenta-angular-v2.json').read_text())
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);a=p/'a';result=self.run_cli(r,a);self.assertEqual(result.returncode,0,result.stderr)
   self.assertEqual(json.loads((a/'recipe.json').read_text()),r);v=json.loads((a/'viewer.json').read_text())
   for name,m in v['stages'].items():
    self.assertEqual(len(m['positions']),len(m['normals']));self.assertEqual(len(m['resistance'])*3,len(m['positions']));self.assertTrue(all(math.isfinite(x) for x in m['positions']));self.assertIn('//',(a/(name+'.obj')).read_text())
   b=p/'b';self.assertEqual(self.run_cli(r,b).returncode,0);self.assertEqual((a/'viewer.json').read_bytes(),(b/'viewer.json').read_bytes())
   for key,value in [('schemaVersion',2),('seed',-1),('size',float('nan')),('grid',{'cells':65})]:
    bad={**r,key:value};o=p/('bad'+key);self.assertEqual(self.run_cli(bad,o).returncode,2);self.assertFalse((o/'viewer.json').exists())
 def test_heightfield_versioned_recipe(self):
  recipe=json.loads((ROOT/'recipes/heightfield-cliff-48-v10.json').read_text());recipe['grid']={'cells':32,'width':64,'height':64}
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);a=p/'a';result=self.run_cli(recipe,a);self.assertEqual(result.returncode,0,result.stderr)
   self.assertEqual(json.loads((a/'recipe.json').read_text()),recipe)
   m=json.loads((a/'metrics.json').read_text());self.assertEqual(m['heightMapping'],'fixed_uint16_0_65535');self.assertEqual(m['sourceKind'],'procedural_heightfield');self.assertEqual(m['clippedLow']+m['clippedHigh'],0)
   self.assertEqual(set(json.loads((a/'viewer.json').read_text())['stages']),{'heightfield'})
   self.assertEqual(self.run_cli(json.loads((a/'recipe.json').read_text()),p/'b').returncode,0)
   self.assertEqual((a/'heightfield.obj').read_bytes(),(p/'b/heightfield.obj').read_bytes())
   for key,value in [('schemaVersion',4),('schemaVersion',3.0),('generatorVersion','heightfield-cliff-v11'),('seed',-1),('seed',4294967296),('seed',1.5),('bedCount',3),('tiltDegrees',46),('irregularity',True),('fractures',-1),('thicknessVariation',2),('relief',float('nan')),('size',float('inf')),('grid',{'cells':32,'width':64,'height':64,'extra':0})]:
    result=self.run_cli({**recipe,key:value},p/'bad');self.assertEqual(result.returncode,2,(key,result.stderr))
   self.assertEqual(self.run_cli({**recipe,'unknown':1},p/'bad').returncode,2)
 def test_jointed_profiles(self):
  base=json.loads((ROOT/'recipes/heightfield-cliff-48-v10.json').read_text())
  for key in ['bedCount','thicknessVariation','tiltDegrees','fragmentedContacts']:del base[key]
  base.update(generatorVersion='heightfield-cliff-v11',profile='massive_oblique',directionDegrees=0,jointSpacing=.24,jointDepth=.65,blockScale=.28)
  base['grid']={'cells':32,'width':64,'height':64}
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d)
   for profile in ['massive_oblique','vertical_channels']:
    r={**base,'profile':profile};out=p/profile;result=self.run_cli(r,out);self.assertEqual(result.returncode,0,result.stderr)
    self.assertEqual(json.loads((out/'recipe.json').read_text()),r)
    m=json.loads((out/'metrics.json').read_text());self.assertEqual(m['profile'],profile);self.assertEqual(m['generatorVersion'],'heightfield-cliff-v11');self.assertEqual(m['heightMapping'],'fixed_uint16_0_65535')
    self.assertEqual(self.run_cli(r,p/'replay').returncode,0);self.assertEqual((out/'heightfield.obj').read_bytes(),(p/'replay/heightfield.obj').read_bytes())
    independent=subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(out/'recipe.json'),'--out',str(p/'independent')],cwd=p,capture_output=True,text=True);self.assertEqual(independent.returncode,0,independent.stderr);self.assertEqual((out/'heightfield.obj').read_bytes(),(p/'independent/heightfield.obj').read_bytes())
   for key,value in [('profile','sandstone'),('jointSpacing',0),('directionDegrees',91),('jointDepth',-1),('blockScale',True),('bedCount',12)]:self.assertEqual(self.run_cli({**base,key:value},p/'bad').returncode,2,key)
 def test_direct_polyhedral_corner_normals(self):
  r=json.loads((ROOT/'recipes/fractured_wall-v3.json').read_text())
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);out=p/'poly';result=self.run_cli(r,out);self.assertEqual(result.returncode,0,result.stderr)
   v=json.loads((out/'viewer.json').read_text());self.assertEqual(v['stages']['stratified'],v['stages']['eroded'])
   for name,m in v['stages'].items():
    self.assertEqual(len(m['cornerNormals']),len(m['indices'])*3)
    lines=(out/(name+'.obj')).read_text().splitlines();verts=sum(x.startswith('v ') for x in lines);normals=sum(x.startswith('vn ') for x in lines)
    self.assertEqual(normals,len(m['indices']))
    for line in lines:
     if line.startswith('f '):
      for pair in line.split()[1:]:
       vertex,normal=map(int,pair.split('//'));self.assertTrue(1<=vertex<=verts);self.assertTrue(1<=normal<=normals)
   self.assertEqual(self.run_cli({**r,'wear':.1},p/'invalid-curved-wear').returncode,2)
 def test_reference_heightmap_command(self):
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);(p/'samples.u16').write_bytes(bytes([0,0,255,255,0,0,255,255]));config={"source":"samples.u16","width":2,"height":2,"cells":16,"size":2,"relief":.5,"backDepth":.3,"sourceUrl":"test://analytic-ramp","license":"test fixture","description":"Analytic reference, not procedural output"};(p/'reference.json').write_text(json.dumps(config))
   result=subprocess.run([str(ROOT/'build/rocks_cli'),'heightmap-reference','--config',str(p/'reference.json'),'--out',str(p/'out')],capture_output=True,text=True)
   self.assertEqual(result.returncode,0,result.stderr);self.assertTrue((p/'out/reference.obj').exists());m=json.loads((p/'out/metrics.json').read_text());self.assertTrue(m['accepted']);self.assertEqual(m['sourceKind'],'reference_heightmap');self.assertAlmostEqual(m['signedVolume'],2.2)
   rerun=subprocess.run([str(ROOT/'build/rocks_cli'),'heightmap-reference','--config',str(p/'out/configuration.json'),'--out',str(p/'again')],capture_output=True,text=True);self.assertEqual(rerun.returncode,0,rerun.stderr);self.assertEqual((p/'out/reference.obj').read_bytes(),(p/'again/reference.obj').read_bytes())
 def test_procedural_heightfield_command(self):
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d);config={"seed":48,"width":64,"height":64,"cells":32,"size":2,"relief":.5,"backDepth":.3};(p/'generated.json').write_text(json.dumps(config))
   result=subprocess.run([str(ROOT/'build/rocks_cli'),'procedural-heightfield','--config',str(p/'generated.json'),'--out',str(p/'out')],capture_output=True,text=True)
   self.assertEqual(result.returncode,0,result.stderr);self.assertTrue((p/'out/procedural.obj').exists());m=json.loads((p/'out/metrics.json').read_text());self.assertEqual(m['sourceKind'],'procedural_heightfield');self.assertTrue(m['accepted'])
if __name__=='__main__':unittest.main()
