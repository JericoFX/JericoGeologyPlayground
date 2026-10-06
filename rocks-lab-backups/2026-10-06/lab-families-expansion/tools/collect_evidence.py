#!/usr/bin/env python3
"""Orchestration only: geometry comparison and generation remain in C++."""
import argparse,json,pathlib,platform,subprocess,time,statistics,shutil
p=argparse.ArgumentParser();p.add_argument('--binary',default='build/rocks_cli');p.add_argument('--recipe',default='recipes/kayenta-angular-v2.json');p.add_argument('--out',default='out/evidence');args=p.parse_args();binary=pathlib.Path(args.binary).resolve();out=pathlib.Path(args.out).resolve();out.mkdir(parents=True,exist_ok=True);recipe=json.loads(pathlib.Path(args.recipe).read_text())
def command(cmd):return subprocess.run(cmd,text=True,capture_output=True,check=True)
report={'hardware':{'platform':platform.platform(),'cpu':pathlib.Path('/proc/cpuinfo').read_text()[:3000] if pathlib.Path('/proc/cpuinfo').exists() else 'unavailable','compiler':command(['g++','--version']).stdout,'flags':'CMake Release -O3 -DNDEBUG; C++20; no fast-math','crossCompiler':'not tested; second compiler unavailable unless separately reported'},'repetitions':5,'warmups':1,'resolutions':{},'browser':'NOT VERIFIED: cloud browser rejected localhost with ERR_BLOCKED_BY_CLIENT; no supported preview route exposed'}
for cells in [64,128]:
 r={**recipe,'grid':{'cells':cells}};rp=out/f'recipe-{cells}.json';rp.write_text(json.dumps(r));dest=out/str(cells);runs=[]
 for repeat in range(6):
  cmd=[str(binary),'generate','--recipe',str(rp),'--out',str(dest)];rss=out/f'rss-{cells}-{repeat}.txt';timed=['/usr/bin/time','-f','%M','-o',str(rss),*cmd] if pathlib.Path('/usr/bin/time').exists() else cmd
  start=time.perf_counter();result=command(timed);wall=(time.perf_counter()-start)*1000;metrics=json.loads((dest/'metrics.json').read_text());entry={'wallMs':wall,'peakRssKiB':int(rss.read_text()) if rss.exists() else None,'metrics':metrics,'command':cmd};(out/f'measurement-{cells}-{repeat}.json').write_text(json.dumps(entry,indent=2));
  if repeat:runs.append(entry)
  print(f'{cells} repeat {repeat}: {wall:.1f} ms, accepted',flush=True)
 def summary(v):return {'median':statistics.median(v),'min':min(v),'max':max(v)}
 report['resolutions'][str(cells)]={'wallMs':summary([x['wallMs'] for x in runs]),'peakRssKiB':summary([x['peakRssKiB'] for x in runs]) if runs[0]['peakRssKiB'] else 'unavailable','stages':{}}
 for stage in ['base','stratified','eroded']:
  latest=runs[-1]['metrics']['stages'][stage];report['resolutions'][str(cells)]['stages'][stage]={**latest,'timings':{k:summary([x['metrics']['stages'][stage][k] for x in runs]) for k in ['fieldMs','extractNormalsMs','validationMs']}}
command([str(binary),'compare','--a',str(out/'64/viewer.json'),'--b',str(out/'128/viewer.json'),'--out',str(out/'comparison.json')]);report['comparison']=json.loads((out/'comparison.json').read_text());report['wearSeries']=[]
for wear in [0,.25,.5,.75,1]:
 r={**recipe,'wear':wear};rp=out/'wear-input.json';rp.write_text(json.dumps(r));dest=out/f'wear-{wear}';result=subprocess.run([str(binary),'generate','--recipe',str(rp),'--out',str(dest)],capture_output=True,text=True);entry={'wear':wear,'returnCode':result.returncode}
 if result.returncode==0:entry['erodedVolume']=json.loads((dest/'metrics.json').read_text())['stages']['eroded']['signedVolume']
 else:entry['error']=result.stderr
 report['wearSeries'].append(entry)
(out/'summary.json').write_text(json.dumps(report,indent=2));print('Evidence saved to '+str(out/'summary.json'),flush=True)
