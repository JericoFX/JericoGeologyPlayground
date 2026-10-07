#!/usr/bin/env python3
"""Offline GLB coverage harness. C++ performs every generation and bake.

Generation: --manual-driver build/variant_export_driver --bake 64
Re-export only: --mode reexport --exporter build/export_native_scene
Use --cases name1,name2 for a narrowly changed profile; other evidence survives.
"""
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', type=Path, default=ROOT/'output/variant-export-gate')
    p.add_argument('--mode', choices=['generate','reexport'], default='generate')
    p.add_argument('--exe', type=Path, default=ROOT/'build/jerico_procedural')
    p.add_argument('--manual-driver', type=Path, default=ROOT/'build/variant_export_driver')
    p.add_argument('--exporter', type=Path, default=ROOT/'build/export_native_scene')
    p.add_argument('--cases', default='')
    p.add_argument('--bake', type=int, choices=[32,64], default=64)
    p.add_argument('--jobs', type=int, choices=[1,2], default=2)
    args = p.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    catalog=json.loads((ROOT/'fixtures/variant-export/cases.json').read_text())
    selected=set(args.cases.split(',')) if args.cases else {c['name'] for c in catalog}
    if selected-{c['name'] for c in catalog}:p.error('Unknown requested case')
    cases=[c for c in catalog if c['name'] in selected and (args.mode!='reexport' or not c['reject'])]
    validator=ROOT/'tools/validate_native_glb.py'
    executables={args.exporter.resolve()} if args.mode=='reexport' else {args.manual_driver.resolve() if c['manual'] else args.exe.resolve() for c in cases}
    for executable in executables:
        if not executable.is_file():p.error('Build the required native executable first: '+str(executable))
    manifest_path=out/'manifest.json'
    manifest=json.loads(manifest_path.read_text()) if manifest_path.exists() else dict(scope='Native binary validity and functional metadata; no new renders or visual inference.',cases=[])
    previous={c['name']:c for c in manifest['cases']}
    phase=dict(mode=args.mode,requested_bake_resolution=args.bake if args.mode=='generate' else None,selected_cases=[c['name'] for c in cases],executables={str(exe):sha(exe) for exe in executables},validator_sha256=sha(validator),results=[])
    def run(case):
        directory=out/case['name'];directory.mkdir(exist_ok=True)
        if args.mode=='reexport':
            scene=directory/'scene.json'
            if not scene.is_file():return dict(name=case['name'],status='MISSING_NATIVE_SCENE')
            command=[str(args.exporter.resolve()),str(scene),str(directory/'asset.glb')]
            record=dict(previous.get(case['name'],case));record.update(exporter_sha256=sha(args.exporter),reexport_input_scene_sha256=sha(scene))
        else:
            recipe=directory/'recipe.json';recipe.write_text(json.dumps(case['recipe'],indent=2)+'\n')
            executable=args.manual_driver.resolve() if case['manual'] else args.exe.resolve()
            command=[str(executable),str(recipe),str(directory)] if case['manual'] else [str(executable),'--family',case['family'],'--recipe',str(recipe),'--out',str(directory/'scene.json'),'--evaluate','1','--bake',str(args.bake),'--glb',str(directory/'asset.glb')]
            record={k:v for k,v in case.items() if k!='recipe'};record.update(recipe_sha256=sha(recipe),generator_sha256=sha(executable))
        start=time.monotonic();process=subprocess.run(command,capture_output=True,text=True,cwd=ROOT)
        (directory/('reexport.log' if args.mode=='reexport' else 'generation.log')).write_text(process.stdout+process.stderr)
        record.update(command=command,exit_code=process.returncode,elapsed_seconds=time.monotonic()-start,generated_glb=(directory/'asset.glb').exists())
        if case['reject']:
            record['status']='EXPECTED_REJECTION' if process.returncode!=0 and case['reject'] in process.stderr and not record['generated_glb'] else 'UNEXPECTED_RESULT'
            record['reason']=process.stderr.strip();return record
        if process.returncode:
            record['status']='EXPORT_FAILED' if args.mode=='reexport' else 'GENERATION_FAILED';record['reason']=process.stderr.strip();return record
        validation=subprocess.run([sys.executable,str(validator),str(directory/'asset.glb'),'--scene',str(directory/'scene.json'),'--report',str(directory/'validation.json')],capture_output=True,text=True,cwd=ROOT)
        (directory/'validation.log').write_text(validation.stdout+validation.stderr);record['validation_exit_code']=validation.returncode
        if validation.returncode:
            record['status']='BINARY_VALIDATION_FAILED';record['reason']=validation.stderr.strip();return record
        scene=json.loads((directory/'scene.json').read_text());report=json.loads((directory/'validation.json').read_text());materials=json.loads((directory/'material-report.json').read_text())
        reports=scene['metadata'].get('damage_operations',[])
        record.update(status='VALID_ASSET',validation=report,operation_statuses={x:sum(r.get('status')==x for r in reports) for x in ['applied','protected','outside']},cut_records=sum(len(p.get('cuts',[])) for p in scene['parts']),fine_breakup_configs=sum(c.get('fine_breakup') is True for p in scene['parts'] for c in p.get('edge_wear',[])),disabled_sockets=[s['id'] for s in scene['sockets'] if not s.get('enabled',True)],disabled_grips=[g['id'] for g in scene['grips'] if not g.get('enabled',True)],actual_atlas_resolutions=sorted(set(p['resolution'] for p in materials['parts'])),empty_islands=sum(p.get('empty_islands',0) for p in materials['parts']),native_material_profile=scene['metadata'].get('native_material_profile'),source_snapshot_sha256=sha(ROOT/case['source']) if case['source'] else None)
        sockets={s['id']:s for s in scene['sockets']};grips={g['id']:g for g in scene['grips']};checks=[]
        for key,value in case['expect'].items():
            if key=='disabled_socket':checks.append(not sockets[value]['enabled'])
            elif key=='enabled_socket':checks.append(sockets[value]['enabled'])
            elif key=='disabled_grip':checks.append(not grips[value]['enabled'])
            elif key=='report_status':checks.append(reports[-1]['status']==value)
        if not all(checks):record['status']='FUNCTIONAL_EXPECTATION_FAILED'
        return record
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures={pool.submit(run,c):c for c in cases}
        for future in as_completed(futures):
            result=future.result();previous[result['name']]=result;phase['results'].append(dict(name=result['name'],status=result['status']));print(result['name']+': '+result['status'],flush=True)
    manifest['cases']=[previous[c['name']] for c in catalog if c['name'] in previous]
    phase['executables_unchanged']=all(sha(exe)==digest for exe,digest in phase['executables'].items());phase['validator_unchanged']=sha(validator)==phase['validator_sha256'];manifest.setdefault('phases',[]).append(phase)
    manifest['valid_assets']=sum(c['status']=='VALID_ASSET' for c in manifest['cases']);manifest['expected_rejections']=sum(c['status']=='EXPECTED_REJECTION' for c in manifest['cases']);manifest['unexpected_results']=[c['name'] for c in manifest['cases'] if c['status'] not in ['VALID_ASSET','EXPECTED_REJECTION']]
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({k:manifest[k] for k in ['valid_assets','expected_rejections','unexpected_results']},indent=2))
    if manifest['unexpected_results'] or not phase['executables_unchanged'] or not phase['validator_unchanged']:raise SystemExit(1)
if __name__=='__main__':main()
