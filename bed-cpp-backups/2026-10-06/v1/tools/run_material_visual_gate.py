"""Offline proof harness. Native generation is performed by the C++ CLI only.

Frozen reference images are never regenerated or changed by this tool.
"""
import argparse, concurrent.futures, hashlib, json, subprocess, sys, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def run(command,log):
    start=time.monotonic()
    with log.open('w') as stream:
        stream.write(json.dumps(command)+'\n');stream.flush()
        result=subprocess.run(command,stdout=stream,stderr=subprocess.STDOUT)
    if result.returncode:raise RuntimeError(f'{log}: exit {result.returncode}')
    return time.monotonic()-start
p=argparse.ArgumentParser();p.add_argument('--out',required=True);p.add_argument('--jobs',type=int,default=2);p.add_argument('--bake',type=int,default=512);p.add_argument('--render-reference',action='store_true');p.add_argument('--reference-dir',default=str(ROOT/'output/family-visual'));a=p.parse_args()
out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True)
labels={'industrial_louvered','wood_reinforced','cabinet_open','damp_old_broken'}
cases=json.loads((ROOT/'fixtures/visual/cases.json').read_text())
for c in cases:c['recipe']=str(ROOT/'fixtures/visual'/c['recipe'])
files=list((ROOT/'src').glob('*.cpp'))+list((ROOT/'include/jerico').glob('*.hpp'))+[ROOT/'tools/render_family_compare.py',ROOT/'build/jerico_procedural']
provenance={str(f.relative_to(ROOT)):digest(f) for f in files}
(out/'input-provenance.json').write_text(json.dumps({'files':provenance,'bake':a.bake,'native_generator':'C++17','preview':'Blender only','cases':cases},indent=2))
def one(c):
    d=out/c['label'];d.mkdir(exist_ok=True);scene=d/'scene.json';glb=d/'asset.glb';r={'label':c['label'],'passed':False}
    try:
        r['generation_seconds']=run([str(ROOT/'build/jerico_procedural'),'--family',c['family'],'--recipe',c['recipe'],'--out',str(scene),'--evaluate','1','--bake',str(a.bake),'--glb',str(glb)],d/'generation.log')
        r['validation_seconds']=run([sys.executable,str(ROOT/'tools/validate_native_glb.py'),str(glb),'--scene',str(scene),'--report',str(d/'glb-validation.json')],d/'glb-validation.log')
        r['render_seconds']=run(['blender','-b','--threads','4','--python-exit-code','1','--python',str(ROOT/'tools/render_family_compare.py'),'--','--mode','native','--family',c['family'],'--recipe',c['recipe'],'--scene',str(scene),'--out',str(d/'native.png'),'--samples','64','--scale','.5'],d/'render.log')
        ref=Path(a.reference_dir)/c['label']/'reference-material.png'
        if a.render_reference:
            ref=d/'reference.png'
            r['reference_seconds']=run(['blender','-b','--threads','4','--python-exit-code','1','--python',str(ROOT/'tools/render_family_compare.py'),'--','--mode','reference','--family',c['family'],'--recipe',c['recipe'],'--out',str(ref),'--samples','64','--scale','.5'],d/'reference.log')
        r['passed']=True;r['scene_sha256']=digest(scene);r['glb_sha256']=digest(glb);r['native_image_sha256']=digest(d/'native.png');r['reference']=str(ref);r['reference_sha256']=digest(ref)
    except Exception as e:r['error']=str(e)
    (d/'status.json').write_text(json.dumps(r,indent=2));print(json.dumps(r),flush=True);return r
with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:results=list(pool.map(one,cases))
(out/'status.json').write_text(json.dumps(results,indent=2))
changed=[f for f,h in provenance.items() if digest(ROOT/f)!=h]
if changed:(out/'source-mutated.json').write_text(json.dumps(changed,indent=2))
raise SystemExit(0 if all(r['passed'] for r in results) and not changed else 1)
