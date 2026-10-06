#!/usr/bin/env python3
"""Offline R2 oracle for native panel cuts and crack profiles. No runtime use."""
import argparse
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'fixtures/sources/JericoIndustrialPropsMaterialsR2Checkpoint_20261006_1927'
sys.dont_write_bytecode=True
sys.path.insert(0,str(SOURCE))
import core
import damage_math as dm
import crack_math as cm


def panel(bounds=(0,1,-.05,.05,0,2),material='painted_metal'):
    return core.Builder(core.normalize_recipe({})).box('panel','leaf',*bounds,material=material,role='Panel')


def requests():
    cases=[]
    def add(name,request): cases.append(dict(name=name,request=request))
    def sequence(name,cuts,part=None,queries=None):
        add(name,dict(mode='sequence',part=part or panel(),cuts=cuts,queries=queries or [[.431,0,.937],[.131,0,.937],[-1,0,1],[.5,.2,1],[.5,0,3]]))
    rect=[[.3,.7],[.7,.7],[.7,1.3],[.3,1.3]]
    rect2=[[.5,1.1],[.9,1.1],[.9,1.6],[.5,1.6]]
    for seed in [0,1,19,29,107,-17,2**40+17]:
        for vertices in [3,13,64]:
            add(f'irregular_{seed}_{vertices}',dict(mode='irregular',center=[.4,1],radii=[.2,.4],seed=seed,vertices=vertices,jaggedness=.45))
    for side,depth in [('front',None),('front',.02),('back',.03)]:
        for name,polygon in [('rectangle',rect),('edge',[[-.1,.7],[.2,.7],[.2,1.3],[-.1,1.3]]),('corner',[[-.1,-.1],[.2,-.1],[.2,.3],[-.1,.3]])]:
            sequence(f'{name}_{side}_{depth}',[dict(polygon=polygon,side=side,depth=depth)])
    sequence('concave',[dict(polygon=[[.2,.5],[.8,.5],[.8,1.5],[.6,1.5],[.6,.8],[.4,.8],[.4,1.5],[.2,1.5]])])
    sequence('overlap_through',[dict(polygon=rect),dict(polygon=rect2)])
    sequence('crossing_depths',[dict(polygon=rect,depth=.065),dict(polygon=rect2,side='back',depth=.065)])
    sequence('glass_material',[dict(polygon=rect)],panel((0,1,-.003,.003,0,2),'glass'))
    sparse=panel()
    for field in ['id','node','material']:sparse.pop(field)
    sequence('sparse_panel_fields',[dict(polygon=rect)],sparse)
    sequence('three_disjoint',[dict(polygon=dm.irregular_polygon(center,radii,18+i)) for i,(center,radii) in enumerate([([.3,.4],[.12,.17]),([.7,1.5],[.13,.2]),([.5,1],[.15,.21])])],panel((0,1,-.025,.025,0,2),'metal'))
    source_panel=panel((-.976,0,.028,.08,.012,2.088),'metal')
    chips=[(-.952,.274,.3484,.006,113),(-.949,.5795,.6632,.009,126),(-.956,.75,.812,.006,139),(-.94,.886,.9976,.01,152),(-.958,1.293,1.3488,.005,165),(-.953,1.8755,1.9468,.008,178)]
    chipcuts=[dict(polygon=dm.irregular_polygon([(-.985+xmax)/2,(zmin+zmax)/2],[(xmax+.985)/2,(zmax-zmin)/2],seed,vertices=9,jaggedness=.57),depth=depth) for xmax,zmin,zmax,depth,seed in chips]
    sequence('near_edge_sliver',[chipcuts[4]],source_panel)
    sequence('design_opening_plus_six_chips',[dict(polygon=[[-.75152,.273],[-.22448,.273],[-.22448,.7245],[-.75152,.7245]])]+chipcuts,source_panel)
    for seed in range(12):
        polygon=dm.irregular_polygon([.05 if seed%2 else .5,.1 if seed%3 else 1],[.3,.5],seed,vertices=13,jaggedness=.45)
        for side,depth in [('front',None),('front',.021),('back',.026)]:
            sequence(f'irregular_carve_{seed}_{side}_{depth}',[dict(polygon=polygon,side=side,depth=depth)])
    for seed in [5,7,41]:
        for width,length in [(.001,.004),(.001,.55),(.006,.024),(.012,.7)]:
            add(f'crack_{seed}_{width}_{length}',dict(mode='crack',center=[.5,1],length=length,width=width,depth=.025,seed=seed,segments=12,direction=[.34,-.86]))
    for width in [.001,.003,.006]:
        for seed in [7,41]:
            profiles=cm.crack_profiles([.5,1],.48,width,.025,seed)
            sequence(f'crack_cut_{width}_{seed}',[dict(polygon=p['polygon_xz'],depth=p['depth']) for p in profiles],panel((0,1,-.026,.026,0,2)))
    for seed,length,depth in [(5,.48,.025),(1,.12,.02),(3,.004,.02),(5,.12,.02)]:
        profiles=cm.crack_profiles([.5,1],length,.001,depth,seed,direction_xz=[math.sin(seed),math.cos(seed)])
        sequence(f'rotated_crack_cut_{seed}_{length}',[dict(polygon=p['polygon_xz'],depth=p['depth']) for p in profiles],panel((0,1,-.026,.026,0,2)))
    # Clear native rejection contracts. Nonfinite cases are tested in C++ directly.
    bad=[[],[[0,0],[1,1]],[[.1,.1],[.2,.2],[.3,.3]],[[.1,.1],[.8,.8],[.1,.8],[.8,.1]],[[.1,.1],[.8,.1],[.8,.1],[.1,.8]],[[2,2],[3,2],[3,3],[2,3]],[[-1,-1],[2,-1],[2,3],[-1,3]]]
    for index,polygon in enumerate(bad): sequence(f'invalid_polygon_{index}',[dict(polygon=polygon)])
    for depth in [0,-.1,.1,.2,True,'deep']: sequence('invalid_depth_'+str(depth),[dict(polygon=rect,depth=depth)])
    sequence('invalid_side',[dict(polygon=rect,side='left')])
    sequence('duplicate_cut',[dict(polygon=rect),dict(polygon=rect)])
    deform=panel();deform['vertices'][0][0]+=.01;sequence('deformed_panel',[dict(polygon=rect)],deform)
    for seed,offset in [(2**63-1,53813),(-2**63,53813),(-10,30),(-100,5),(2**63-1,2**64-1),(-2**63,2**64-1)]:
        add('offset_irregular_'+str(seed),dict(mode='irregular',center=[.4,1],radii=[.2,.4],seed=seed,positive_offset=offset))
        add('offset_crack_'+str(seed),dict(mode='crack',center=[.5,1],length=.48,width=.006,depth=.025,seed=seed,positive_offset=offset))
    for update in [dict(radii=[0,1]),dict(vertices=2),dict(vertices=65),dict(jaggedness=-.01),dict(jaggedness=.86)]:
        request=dict(mode='irregular',center=[0,0],radii=[1,1],seed=1);request.update(update)
        add('invalid_irregular_'+str(len(cases)),request)
    for update in [dict(length=.003,width=.001),dict(width=.00099),dict(width=-1),dict(depth=0),dict(depth=5e-324),dict(branches=-1),dict(branches=3),dict(segments=1),dict(segments=13),dict(direction=[0,0]),dict(center=[100000,0])]:
        request=dict(mode='crack',center=[.5,1],length=.48,width=.006,depth=.025,seed=41);request.update(update)
        add('invalid_crack_'+str(len(cases)),request)
    for count in [12,13]:
        original=panel();original['cuts']=[dict(polygon_xz=rect,side='front',depth=None) for _ in range(count)]
        sequence('cut_budget_'+str(count),[dict(polygon=rect2)],original)
    return cases


def execute(r):
    mode=r['mode']
    if mode=='irregular':return dm.irregular_polygon(r['center'],r['radii'],r['seed']+r.get('positive_offset',0),r.get('vertices',13),r.get('jaggedness',.25))
    if mode=='crack':return cm.crack_profiles(r['center'],r['length'],r['width'],r['depth'],r['seed']+r.get('positive_offset',0),r.get('direction',[0,1]),r.get('branches',2),r.get('segments',6))
    if mode=='point':return dm.point_in_panel(r['part'],r['point'])
    part=r['part'];parts=[]
    for cut in r['cuts']:
        part=dm.carve_panel(part,cut['polygon'],cut.get('side','front'),cut.get('depth'));parts.append(part)
    return dict(parts=parts,occupancy=[dm.point_in_panel(part,p) for p in r.get('queries',[])])


def compare(a,b,path,stats):
    # The native generator has its own provenance; all semantic fields stay exact.
    if path.endswith('.metadata.generator'): return
    if isinstance(a,dict):
        if not isinstance(b,dict) or a.keys()!=b.keys():raise AssertionError(f'{path}: object keys differ')
        for k in a:compare(a[k],b[k],path+'.'+k,stats)
    elif isinstance(a,list):
        if not isinstance(b,list) or len(a)!=len(b):raise AssertionError(f'{path}: list size {len(a)} != {len(b) if isinstance(b,list) else type(b)}')
        for i,(x,y) in enumerate(zip(a,b)):compare(x,y,f'{path}[{i}]',stats)
    elif isinstance(a,bool) or a is None or isinstance(a,str):
        if type(a) is not type(b) or a!=b:raise AssertionError(f'{path}: {a!r} != {b!r}')
    else:
        if isinstance(b,bool) or not isinstance(b,(int,float)) or not math.isfinite(b):raise AssertionError(f'{path}: invalid number')
        delta=abs(a-b);stats['numbers']+=1
        if delta>stats['max_abs_error']:stats['max_abs_error']=delta;stats['max_error_path']=path
        if delta>2e-12 or isinstance(a,int) and isinstance(b,int) and a!=b:raise AssertionError(f'{path}: {a!r} != {b!r} (delta {delta})')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--filter',default='');p.add_argument('--write-reference',type=Path);args=p.parse_args()
    cases=[c for c in requests() if args.filter in c['name']]
    for c in cases:
        try:c['expected']=dict(value=execute(c['request']))
        except ValueError as e:c['expected']=dict(error=str(e))
    if args.write_reference:args.write_reference.write_text(json.dumps(cases,allow_nan=False))
    with tempfile.TemporaryDirectory(prefix='jerico-damage-oracle-') as folder:
        request=Path(folder)/'requests.json';output=Path(folder)/'results.json';request.write_text(json.dumps([c['request'] for c in cases],allow_nan=False))
        run=subprocess.run([str(args.exe.resolve()),'--evaluate',str(request),str(output)],capture_output=True,text=True)
        if run.returncode:raise RuntimeError(f'native evaluator failed: {run.stderr}')
        actual=json.loads(output.read_text())
    if len(actual)!=len(cases):raise AssertionError('result case count mismatch')
    stats=dict(cases=len(cases),passed=0,expected_rejections=0,numbers=0,max_abs_error=0.0,max_error_path='',failures=[])
    for c,result in zip(cases,actual):
        try:compare(c['expected'],result,c['name'],stats);stats['passed']+=1;stats['expected_rejections']+=int('error' in c['expected'])
        except AssertionError as e:stats['failures'].append(dict(name=c['name'],error=str(e)))
    print(json.dumps(stats,indent=2))
    if stats['failures']:raise SystemExit(1)
if __name__=='__main__':main()
