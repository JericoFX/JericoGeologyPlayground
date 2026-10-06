#!/usr/bin/env python3
"""Offline native fine-wear authoring and exact contour reduction oracle."""
import argparse
import copy
import hashlib
import importlib.util
import inspect
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'fixtures/sources/JericoIndustrialPropsFineWearCheckpoint_20261006_1921'
sys.dont_write_bytecode=True
sys.path.insert(0,str(SOURCE))
import core
import damage_math
import crack_math
import localized_damage as ld
import industrial
import fine_wear_math as fm
sys.path.insert(0,str(ROOT/'tools'))
from damage_oracle import compare


def capture():
    cases=[];seen=set();depth=0
    def wrap(module,name,mode):
        original=getattr(module,name);signature=inspect.signature(original)
        def wrapped(*args,**kwargs):
            nonlocal depth
            nested=depth>0;depth+=1
            try:
                result=original(*args,**kwargs);expected=dict(value=copy.deepcopy(result))
            except ValueError as error:
                expected=dict(error=str(error));raise
            finally:
                depth-=1
                if not nested:
                    binding=signature.bind(*args,**kwargs);binding.apply_defaults()
                    request=copy.deepcopy(dict(binding.arguments));request['mode']=mode
                    encoded=json.dumps(request,sort_keys=True,allow_nan=False);digest=hashlib.sha256(encoded.encode()).hexdigest()
                    if digest not in seen:seen.add(digest);cases.append(dict(name=f'{mode}_{len(cases):03}',request=request,expected=expected))
            return result
        setattr(module,name,wrapped)
    wrap(ld,'apply_operations','apply');wrap(fm,'contour_segments','contour')
    suite=unittest.TestSuite()
    for filename in ['test_fine_wear_math.py','test_fine_wear.py']:
        spec=importlib.util.spec_from_file_location('frozen_'+filename[:-3],SOURCE/'tests'/filename);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        suite.addTests(unittest.defaultTestLoader.loadTestsFromModule(module))
    cwd=Path.cwd()
    try:os.chdir(SOURCE);run=unittest.TextTestRunner(stream=io.StringIO(),verbosity=0).run(suite)
    finally:os.chdir(cwd)
    if not run.wasSuccessful():raise RuntimeError(f'frozen fine source suite failed: {run.failures!r} {run.errors!r}')
    # Exercise the named checkpoint recipes through the native wrapper too.
    for name in ['fine_wear_before','fine_wear_after','dense_wear_continuous']:
        recipe=json.loads((SOURCE/'recipes'/f'{name}.json').read_text())
        value=industrial.build_industrial(recipe)
        cases.append(dict(name=name,request=dict(mode='build',recipe=recipe),expected=dict(value=value)))
    return cases,dict(source_tests=run.testsRun,blender_shader_tests_skipped=len(run.skipped))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--write-reference',type=Path);args=p.parse_args()
    cases,source=capture()
    if args.write_reference:args.write_reference.write_text(json.dumps(cases,allow_nan=False))
    with tempfile.TemporaryDirectory(prefix='jerico-finewear-oracle-') as folder:
        request=Path(folder)/'requests.json';output=Path(folder)/'results.json';request.write_text(json.dumps([c['request'] for c in cases],allow_nan=False))
        run=subprocess.run([str(args.exe.resolve()),'--evaluate',str(request),str(output)],capture_output=True,text=True)
        if run.returncode:raise RuntimeError(f'native evaluator failed: {run.stderr}')
        actual=json.loads(output.read_text())
    if len(actual)!=len(cases):raise AssertionError('result case count mismatch')
    stats=dict(**source,cases=len(cases),passed=0,expected_rejections=0,numbers=0,max_abs_error=0.0,max_error_path='',failures=[])
    for c,result in zip(cases,actual):
        # The native wrapper records which original adapter owns the material.
        # Verify the exact native-only annotation, then compare source semantics.
        if c['request']['mode'] in ('apply','build') and 'value' in result:
            metadata=result['value'].get('metadata',{})
            if metadata.get('native_material_profile')!='finewear_1921':
                raise AssertionError(c['name']+': missing explicit FineWear profile')
            del metadata['native_material_profile']
        try:compare(c['expected'],result,c['name'],stats);stats['passed']+=1;stats['expected_rejections']+=int('error' in c['expected'])
        except AssertionError as e:stats['failures'].append(dict(name=c['name'],error=str(e)))
    print(json.dumps(stats,indent=2))
    if stats['failures']:raise SystemExit(1)
if __name__=='__main__':main()
