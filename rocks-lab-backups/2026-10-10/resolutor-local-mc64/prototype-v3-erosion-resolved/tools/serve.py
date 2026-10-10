#!/usr/bin/env python3
"""Loopback-only invocation bridge. No geological calculations."""
import argparse,json,math,mimetypes,pathlib,re,shutil,subprocess,tempfile,threading,uuid
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
from urllib.parse import urlsplit,unquote
ROOT=pathlib.Path(__file__).resolve().parents[1]
KEYS={'schemaVersion','seed','units','generatorVersion','extractorVersion','size','layerThickness','contrast','wear','grid'}
HF_KEYS={'schemaVersion','generatorVersion','extractorVersion','family','units','seed','size','relief','backDepth','bedCount','thicknessVariation','tiltDegrees','fractures','irregularity','fragmentedContacts','grid'}
HF_JOINT_KEYS=(HF_KEYS-{'bedCount','thicknessVariation','tiltDegrees','fragmentedContacts'})|{'profile','directionDegrees','jointSpacing','jointDepth','blockScale'}
def validate_heightfield(r):
 jointed=r.get('generatorVersion')=='heightfield-cliff-v11'
 if set(r)!=(HF_JOINT_KEYS if jointed else HF_KEYS):raise ValueError('Heightfield requires exactly documented fields')
 if type(r['schemaVersion'])!=int or r['schemaVersion']!=3 or r['generatorVersion'] not in ('heightfield-cliff-v10','heightfield-cliff-v11') or r['extractorVersion']!='heightfield-solid-v2' or r['family']!='heightfield_cliff' or r['units']!='m':raise ValueError('Unsupported heightfield version')
 for key,low,high in ([('seed',0,4294967295)] if jointed else [('seed',0,4294967295),('bedCount',4,24)]):
  if type(r[key])!=int or not low<=r[key]<=high:raise ValueError(key+' outside integer range')
 if type(r['grid'])!=dict or set(r['grid'])!={'cells','width','height'}:raise ValueError('Heightfield grid requires cells, width, height')
 for key,high in [('cells',256),('width',4096),('height',4096)]:
  if type(r['grid'][key])!=int or not 2<=r['grid'][key]<=high:raise ValueError('Grid outside integer range')
 if type(r['size']) not in (int,float) or not .1<=r['size']<=10:raise ValueError('size outside finite range')
 ranges=[('size',.1,10),('relief',0,.5*r['size']),('backDepth',.02*r['size'],.5*r['size']),('fractures',0,1),('irregularity',0,1)]
 ranges+= [('directionDegrees',-90,90),('jointSpacing',.08,.6),('jointDepth',0,1),('blockScale',.12,.6)] if jointed else [('thicknessVariation',0,1),('tiltDegrees',-45,45)]
 for key,low,high in ranges:
  if type(r[key]) not in (int,float) or not -1e100<r[key]<1e100 or not math.isfinite(r[key]) or not low<=r[key]<=high:raise ValueError(key+' outside finite range')
 if jointed:
  if r['profile'] not in ('massive_oblique','vertical_channels'):raise ValueError('Unsupported cliff profile')
 elif type(r['fragmentedContacts'])!=bool:raise ValueError('fragmentedContacts must be boolean')
 return r
def validate_recipe(r):
 if type(r)==dict and r.get("schemaVersion")==3:return validate_heightfield(r)
 study=type(r)==dict and r.get('generatorVersion')=='rocks-v12'
 extra=({'studySpacing','columnCount','columnLean','jointDepth'} if r.get('family')=='columnar_basalt' else {'studySpacing','roundness','chipping'}) if study else set()
 if type(r)!=dict or set(r)!=(extra|KEYS|({'family'}|({'detail'} if 'detail' in r else set()) if r.get('schemaVersion')==2 else set())):raise ValueError('Recipe requires exactly documented fields')
 for key in ['schemaVersion','seed']:
  if type(r[key])!=int or not 0<=r[key]<=4294967295:raise ValueError(key+' must be uint32')
 supported=(r['schemaVersion']==1 and r['generatorVersion']=='rocks-v2') or (r['schemaVersion']==2 and ((r['generatorVersion']=='rocks-v3' and r.get('family') in ('fractured_wall','irregular_beds','overhang')) or (r['generatorVersion'] in ('rocks-v4','rocks-v5','rocks-v6') and r.get('family') in ('common_rock','cliff','strata')) or (r['generatorVersion']=='rocks-v7' and r.get('family')=='weathered_boulder') or (r['generatorVersion']=='rocks-v8' and r.get('family')=='cliff_plateau') or (study and r.get('family') in ('columnar_basalt','jointed_granite'))))
 if study:
  if r['wear']!=0 or r.get('detail',0)!=0 or r['extractorVersion']!=('poly-csg-v1' if r['family']=='columnar_basalt' else 'paris-mc-v1'):raise ValueError('v12 requires matching extractor and wear/detail0')
  ranges=[('studySpacing',.3,.6)]+([('columnLean',-.15,.15),('jointDepth',0,1)] if r['family']=='columnar_basalt' else [('roundness',.06,.2),('chipping',0,1)])
  for key,lo,hi in ranges:
   if type(r[key]) not in (int,float) or not math.isfinite(r[key]) or not lo<=r[key]<=hi:raise ValueError(key+' outside finite range')
  if r['family']=='columnar_basalt' and (type(r['columnCount'])!=int or not 3<=r['columnCount']<=7):raise ValueError('columnCount must be integer in [3,7]')
 if not supported or r['units']!='m' or r['extractorVersion'] not in ('paris-mc-v1','poly-csg-v1'):raise ValueError('Unsupported version or units')
 if r['generatorVersion']=='rocks-v7' and (r['extractorVersion']!='paris-mc-v1' or r['wear']!=0):raise ValueError('Weathered family requires MC and wear0')
 if r['generatorVersion'] in ('rocks-v6','rocks-v8') and r['extractorVersion']!='poly-csg-v1':raise ValueError('This family requires direct polyhedral extraction')
 if r['generatorVersion']=='rocks-v8' and r.get('detail',0)!=0:raise ValueError('Plateau study currently requires detail0')
 if type(r['grid'])!=dict or set(r['grid'])!={'cells'} or type(r['grid']['cells'])!=int or r['grid']['cells'] not in (64,128):raise ValueError('grid.cells must be 64 or 128')
 if r['extractorVersion']=='poly-csg-v1' and (r['schemaVersion']!=2 or r['wear']!=0):raise ValueError('Direct polyhedral route requires schema2 and wear0')
 if 'detail' in r and (type(r['detail']) not in (int,float) or not 0<=r['detail']<=1):raise ValueError('detail must be finite in [0,1]')
 for k in ['size','layerThickness','contrast','wear']:
  if type(r[k]) not in (int,float) or not (-1e100<r[k]<1e100) or not math.isfinite(r[k]):raise ValueError(k+' must be finite number')
 if not .1<=r['size']<=10 or not (r['layerThickness']>0 if study else (.02*r['size'] if r['extractorVersion']=='poly-csg-v1' else 4*1.5*r['size']/r['grid']['cells'])<=r['layerThickness']<=.7*r['size']) or not 0<=r['contrast']<=1 or not 0<=r['wear']<=1:raise ValueError('Recipe parameters outside allowed ranges')
 return r
class State:
 def __init__(self,binary,output_root,timeout):self.binary=pathlib.Path(binary).resolve();self.output=pathlib.Path(output_root).resolve();self.output.mkdir(parents=True,exist_ok=True);self.timeout=timeout;self.lock=threading.Lock();self.current=None
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def respond(self,status,obj):
  data=json.dumps(obj,allow_nan=False).encode();self.send_response(status);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)));self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff');self.end_headers();self.wfile.write(data)
 def trusted(self):
  port=self.server.server_address[1];hosts={f'127.0.0.1:{port}',f'localhost:{port}'}
  if self.headers.get('Host') not in hosts:return False
  origin=self.headers.get('Origin');return origin is None or origin in {'http://'+h for h in hosts}
 def do_GET(self):
  if not self.trusted():return self.respond(403,{'error':'Host/Origin forbidden'})
  path=unquote(urlsplit(self.path).path)
  if path=='/api/current':return self.respond(200,self.server.state.current) if self.server.state.current else self.respond(404,{'error':'No generation yet'})
  allowed={'/':ROOT/'viewer/index.html','/index.html':ROOT/'viewer/index.html','/style.css':ROOT/'viewer/style.css','/app.js':ROOT/'viewer/app.js','/renderer.js':ROOT/'viewer/renderer.js','/tests/viewer_tests.html':ROOT/'tests/viewer_tests.html','/tests/viewer_tests.js':ROOT/'tests/viewer_tests.js','/default-recipe.json':ROOT/'recipes/heightfield-cliff-48-v10.json'}
  file=allowed.get(path)
  if re.fullmatch(r'/generations/[a-f0-9]{32}/(viewer.json|metrics.json|recipe.json|base.obj|stratified.obj|eroded.obj|heightfield.obj|height.u16)',path):file=self.server.state.output/path.removeprefix('/generations/')
  if path.startswith('/assets/textures/polyhaven/') and '..' not in pathlib.PurePosixPath(path).parts:
   candidate=(ROOT/path.lstrip('/')).resolve();base=(ROOT/'assets/textures/polyhaven').resolve()
   if candidate.is_relative_to(base) and candidate.suffix in ('.json','.jpg','.png'):file=candidate
  if file is None or not file.is_file():return self.respond(404,{'error':'Not found'})
  data=file.read_bytes();self.send_response(200);self.send_header('Content-Type',mimetypes.guess_type(file)[0] or 'application/octet-stream');self.send_header('Content-Length',str(len(data)));self.send_header('X-Content-Type-Options','nosniff');self.send_header('Cache-Control','no-cache');self.end_headers();self.wfile.write(data)
 def do_POST(self):
  if not self.trusted():return self.respond(403,{'error':'Host/Origin forbidden'})
  if self.path!='/api/generate':return self.respond(404,{'error':'Not found'})
  if self.headers.get('Content-Type','').split(';')[0]!='application/json':return self.respond(400,{'error':'JSON required'})
  try:
   raw=self.headers.get('Content-Length');length=int(raw) if raw is not None else -1
   if length<0:raise ValueError('Content-Length required')
   if length>65536:return self.respond(413,{'error':'Body exceeds 64 KiB'})
   data=json.loads(self.rfile.read(length),parse_constant=lambda _: (_ for _ in ()).throw(ValueError('Nonfinite JSON')))
   if type(data)!=dict or set(data)!={'requestId','recipe'}:raise ValueError('Only requestId and recipe allowed')
   request=data['requestId']
   if type(request)!=int or not 0<=request<=9007199254740991:raise ValueError('requestId must be nonnegative safe integer')
   recipe=validate_recipe(data['recipe'])
  except (ValueError,TypeError,KeyError) as e:return self.respond(400,{'error':str(e)})
  state=self.server.state
  if not state.lock.acquire(False):return self.respond(409,{'requestId':request,'error':'A generation is already running'})
  def generate():
   try:
    with tempfile.TemporaryDirectory(prefix='.pending-',dir=state.output) as temp:
     path=pathlib.Path(temp);source=path/'input.json';source.write_text(json.dumps(recipe,allow_nan=False));
     try:result=subprocess.run([str(state.binary),'generate','--recipe',str(source),'--out',str(path)],shell=False,timeout=state.timeout,capture_output=True,text=True)
     except subprocess.TimeoutExpired:return 504,{'requestId':request,'error':'Generation timeout; previous result retained'}
     if result.returncode:return 422,{'requestId':request,'error':result.stderr[-4000:] or 'CLI rejected generation'}
     names=['heightfield'] if recipe['schemaVersion']==3 else ['base','stratified','eroded']
     files=['viewer.json','metrics.json','recipe.json']+[n+'.obj' for n in names]
     if any(not (path/f).is_file() for f in files):return 422,{'requestId':request,'error':'Incomplete generation'}
     source.unlink();gid=uuid.uuid4().hex;destination=state.output/gid;path.rename(destination)
     prefix='/generations/'+gid+'/';response={'requestId':request,'generationId':gid,'viewerUrl':prefix+'viewer.json','metricsUrl':prefix+'metrics.json','exports':{n:prefix+n+'.obj' for n in names}};response['exports']['recipe']=prefix+'recipe.json';state.current=response
     return 200,response
   except (OSError,ValueError) as e:return 500,{'requestId':request,'error':str(e)}
  try:status,payload=generate()
  finally:state.lock.release()
  return self.respond(status,payload)

def make_server(port,binary=ROOT/'build/rocks_cli',output_root=ROOT/'out/generations',timeout=120):
 server=ThreadingHTTPServer(('127.0.0.1',port),Handler);server.daemon_threads=True;server.state=State(binary,output_root,timeout);return server
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--port',type=int,default=8000);args=parser.parse_args();server=make_server(args.port);print(f'Rocks Lab http://127.0.0.1:{args.port}',flush=True);server.serve_forever()
