#!/usr/bin/env python3
"""Capture frozen R2 behavioral tests and replay their public calls in C++.

Python is only an offline oracle; requests contain no expected output, and the
native test executable never imports or invokes Python. Duplicate requests are
coalesced. Unserializable Python-only inputs are reported rather than hidden.
"""
import argparse
import copy
import hashlib
import importlib.util
import inspect
import io
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'fixtures/sources/JericoIndustrialPropsMaterialsR2Checkpoint_20261006_1927'
sys.dont_write_bytecode=True
sys.path.insert(0,str(SOURCE))
import localized_damage as ld
# Reuse only comparison; this module uses the same immutable R2 dependency set.
sys.path.insert(0,str(ROOT/'tools'))
from damage_oracle import compare


def capture():
    cases=[];seen=set();depth=0;skipped=[]
    def wrap(name,mode):
        original=getattr(ld,name);signature=inspect.signature(original)
        def wrapped(*args,**kwargs):
            nonlocal depth
            nested=depth>0;depth+=1
            try:
                result=original(*args,**kwargs)
                expected=dict(value=copy.deepcopy(result))
            except ValueError as error:
                expected=dict(error=str(error));raise
            finally:
                depth-=1
                if not nested:
                    binding=signature.bind(*args,**kwargs);binding.apply_defaults()
                    request=copy.deepcopy(dict(binding.arguments));request['mode']=mode
                    if mode=='edge_distance' and request.get('side') is None:request.pop('side')
                    try:
                        encoded=json.dumps(request,sort_keys=True,allow_nan=False)
                        # nlohmann JSON numbers are IEEE double or 64-bit integer.
                        def supported(value):
                            if isinstance(value,dict):return all(supported(v) for v in value.values())
                            if isinstance(value,list):return all(supported(v) for v in value)
                            return not (isinstance(value,int) and not isinstance(value,bool) and abs(value)>2**63-1)
                        if not supported(request):raise ValueError('outside native signed 64-bit JSON range')
                    except ValueError as error:skipped.append(str(error))
                    else:
                        digest=hashlib.sha256(encoded.encode()).hexdigest()
                        if digest not in seen:
                            seen.add(digest);cases.append(dict(name=f'{mode}_{len(cases):03}',request=request,expected=expected))
            return result
        setattr(ld,name,wrapped)
    for name,mode in [('apply_operations','apply'),('selection_weight','selection'),('edge_distance','edge_distance'),('edge_wear_weight','edge_weight')]:wrap(name,mode)
    spec=importlib.util.spec_from_file_location('frozen_localized_tests',SOURCE/'tests/test_localized_damage.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    suite=unittest.defaultTestLoader.loadTestsFromModule(module)
    result=unittest.TextTestRunner(stream=io.StringIO(),verbosity=0).run(suite)
    if not result.wasSuccessful():raise RuntimeError(f'frozen source tests failed: {result.failures!r} {result.errors!r}')
    # Regression: support-end iteration produces visible ordered reasons.
    scene=module.with_mounts()
    scene['sockets'][0]['support_ends']={name:dict(node='leaf',part_ids=['untouched'],samples=[[99,0,1]],required_depth_m=.004) for name in ['z_end','a_end']}
    ld.apply_operations(scene,[module.cut_operation(depth=.02,override_protection=True)])
    # Sparse but valid source scenes must not gain optional socket/grip arrays.
    sparse=module.panel_scene()
    for field in ['metadata','sockets','grips']:sparse.pop(field)
    ld.apply_operations(sparse,[module.cut_operation(depth=.02,override_protection=True)])
    return cases,dict(source_tests=result.testsRun,unserializable_python_only_calls=len(skipped))


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--write-reference',type=Path);parser.add_argument('--filter',default='');args=parser.parse_args()
    cases,source=capture();cases=[c for c in cases if args.filter in c['name']]
    if args.write_reference:args.write_reference.write_text(json.dumps(cases,allow_nan=False))
    with tempfile.TemporaryDirectory(prefix='jerico-localized-oracle-') as folder:
        request=Path(folder)/'requests.json';output=Path(folder)/'results.json';request.write_text(json.dumps([c['request'] for c in cases],allow_nan=False))
        run=subprocess.run([str(args.exe.resolve()),'--evaluate',str(request),str(output)],capture_output=True,text=True)
        if run.returncode:raise RuntimeError(f'native evaluator failed: {run.stderr}')
        actual=json.loads(output.read_text())
    if len(actual)!=len(cases):raise AssertionError('result case count mismatch')
    stats=dict(**source,cases=len(cases),passed=0,expected_rejections=0,numbers=0,max_abs_error=0.0,max_error_path='',failures=[])
    for c,result in zip(cases,actual):
        try:compare(c['expected'],result,c['name'],stats);stats['passed']+=1;stats['expected_rejections']+=int('error' in c['expected'])
        except AssertionError as e:stats['failures'].append(dict(name=c['name'],error=str(e)))
    print(json.dumps(stats,indent=2))
    if stats['failures']:raise SystemExit(1)
if __name__=='__main__':main()
