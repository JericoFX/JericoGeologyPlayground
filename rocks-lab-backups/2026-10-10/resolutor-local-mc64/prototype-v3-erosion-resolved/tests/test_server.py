import unittest,sys,pathlib,tempfile,threading,json,http.client,time
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import serve
class Service(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.d=pathlib.Path(self.tmp.name);self.fake=self.d/'fake';self.fake.write_text('#!/usr/bin/env python3\nimport sys,json,pathlib,time\np=pathlib.Path(sys.argv[-1]);r=json.loads(pathlib.Path(sys.argv[-3]).read_text());time.sleep(.1)\nif r["seed"]==99: sys.exit(2)\nif r["seed"]==98: time.sleep(5)\nfor f in ["viewer.json","metrics.json","recipe.json","base.obj","stratified.obj","eroded.obj"]: (p/f).write_text("{}")\n');self.fake.chmod(0o755)
  self.server=serve.make_server(0,binary=self.fake,output_root=self.d/'out',timeout=2);self.port=self.server.server_address[1];threading.Thread(target=self.server.serve_forever,daemon=True).start();self.recipe=json.loads((ROOT/'recipes/kayenta-angular-v2.json').read_text())
 def tearDown(self):self.server.shutdown();self.server.server_close();self.tmp.cleanup()
 def req(self,path='/api/generate',data=None,method='POST',headers=None):
  c=http.client.HTTPConnection('127.0.0.1',self.port);b=json.dumps(data or {'requestId':1,'recipe':self.recipe});h={'Content-Type':'application/json'};h.update(headers or {});c.request(method,path,b if method=='POST' else None,h);r=c.getresponse();v=r.read();c.close();return r.status,v
 def test_security_and_preservation(self):
  self.assertEqual(self.req(data={'requestId':1,'recipe':{**self.recipe,'schemaVersion':2,'generatorVersion':'rocks-v3','family':'fractured_wall'}})[0],200)
  self.assertEqual(self.req()[0],200);before=self.req('/api/current',method='GET')[1]
  for headers in [{'Host':'evil.test'},{'Origin':'https://evil.test'}]:self.assertEqual(self.req(headers=headers)[0],403)
  self.assertEqual(self.req(data={'requestId':1,'recipe':self.recipe,'binary':'evil'})[0],400)
  self.assertEqual(self.req(data={'requestId':1,'recipe':{**self.recipe,'size':float('nan')}})[0],400)
  self.assertEqual(self.req(data={'requestId':1,'recipe':{**self.recipe,'size':10**400}})[0],400)
  self.assertEqual(self.req(data={'x':'a'*70000})[0],413)
  self.assertEqual(self.req('/../cli/main.cpp',method='GET')[0],404)
  for seed,code in [(99,422),(98,504)]:self.assertEqual(self.req(data={'requestId':2,'recipe':{**self.recipe,'seed':seed}})[0],code)
  self.assertEqual(self.req('/api/current',method='GET')[1],before)
 def test_default_family(self):
  status,data=self.req("/default-recipe.json",method="GET")
  self.assertEqual(status,200)
  recipe=json.loads(data);self.assertEqual(recipe["generatorVersion"],"heightfield-cliff-v10");self.assertEqual(recipe["family"],"heightfield_cliff")
 def test_new_family_recipes(self):
  for family in ("common_rock","cliff","strata"):
   recipe=json.loads((ROOT/f"recipes/{family}-48-v4.json").read_text())
   self.assertEqual(self.req(data={"requestId":1,"recipe":recipe})[0],200)
   recipe["generatorVersion"]="rocks-v5"
   self.assertEqual(self.req(data={"requestId":1,"recipe":recipe})[0],200)
 def test_weathered_and_plateau_versions(self):
  for name in ("weathered_boulder-48-64-v7","cliff_plateau-48-v8"):
   recipe=json.loads((ROOT/f"recipes/{name}.json").read_text())
   self.assertEqual(self.req(data={"requestId":1,"recipe":recipe})[0],200)
   recipe["extractorVersion"]="poly-csg-v1" if name.startswith("weathered") else "paris-mc-v1"
   self.assertEqual(self.req(data={"requestId":1,"recipe":recipe})[0],400)
 def test_heightfield_recipe_validation(self):
  recipe=json.loads((ROOT/'recipes/heightfield-cliff-48-v10.json').read_text())
  self.assertEqual(serve.validate_recipe(recipe),recipe)
  for key,value in [('schemaVersion',3.0),('generatorVersion','bad'),('bedCount',25),('fractures',True),('tiltDegrees',float('nan')),('grid',{'cells':0,'width':64,'height':64})]:
   with self.assertRaises(ValueError):serve.validate_recipe({**recipe,key:value})
 def test_heightfield_real_cli_bundle(self):
  self.server.state.binary=ROOT/'build/rocks_cli'
  recipe=json.loads((ROOT/'recipes/heightfield-cliff-48-v10.json').read_text());recipe['grid']={'cells':32,'width':64,'height':64}
  status,data=self.req(data={'requestId':17,'recipe':recipe});self.assertEqual(status,200,data)
  result=json.loads(data);self.assertEqual(set(result['exports']),{'heightfield','recipe'})
  status,data=self.req(result['exports']['recipe'],method='GET');self.assertEqual(status,200);self.assertEqual(json.loads(data),recipe)
  status,data=self.req(result['exports']['heightfield'],method='GET');self.assertEqual(status,200);self.assertIn(b'v ',data)
  status,data=self.req(result['viewerUrl'],method='GET');self.assertEqual(status,200);self.assertEqual(set(json.loads(data)['stages']),{'heightfield'})
 def test_jointed_profiles_real_cli(self):
  self.server.state.binary=ROOT/'build/rocks_cli'
  for profile in ['massive-oblique','vertical-channels']:
   recipe=json.loads((ROOT/f'recipes/heightfield-{profile}-48-v11.json').read_text());recipe['grid']={'cells':24,'width':48,'height':48}
   status,data=self.req(data={'requestId':18,'recipe':recipe});self.assertEqual(status,200,data)
   result=json.loads(data);status,data=self.req(result['exports']['recipe'],method='GET');self.assertEqual(json.loads(data),recipe)
   status,data=self.req(result['metricsUrl'],method='GET');self.assertEqual(json.loads(data)['profile'],recipe['profile'])
   for key,value in [('profile','unknown'),('jointSpacing',False),('directionDegrees',91),('jointDepth',-1),('blockScale',1),('bedCount',12),('size','2'),('size',None),('size',10**400)]:
    with self.assertRaises(ValueError):serve.validate_recipe({**recipe,key:value})
 def test_response_releases_generation_lock(self):
  from unittest.mock import patch
  observed=[];original=serve.Handler.respond
  def watched(handler,status,data):
   if handler.command=='POST' and status in (200,422,504):observed.append(handler.server.state.lock.locked())
   return original(handler,status,data)
  with patch.object(serve.Handler,'respond',watched):
   for seed,status in [(48,200),(99,422),(98,504)]:
    self.assertEqual(self.req(data={'requestId':1,'recipe':{**self.recipe,'seed':seed}})[0],status)
  self.assertEqual(observed,[False,False,False],'A terminal response must not advertise completion while generation lock is held')
 def test_single_active(self):
  result=[];t=threading.Thread(target=lambda:result.append(self.req(data={'requestId':1,'recipe':{**self.recipe,'seed':98}})[0]));t.start();time.sleep(.1);self.assertEqual(self.req()[0],409);t.join();self.assertEqual(result,[504])
if __name__=='__main__':unittest.main()
