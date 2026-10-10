#!/usr/bin/env python3
"""Render new analytic reference studies using the unchanged validated OBJ pipeline."""
import argparse
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import render_surface_studies as base
import render_alveolar_ao as ao
_original_settings=base.settings_for
_original_setup=base.setup_scene

def reference_settings(args,recipe,view,mode,version):
 settings=ao.settings_with_ao(_original_settings(args,recipe,view,mode,version),.30,.12,32)
 # Cache identity includes every renderer dependency before base.render_one can skip.
 settings['renderer_dependencies']={p:base.digest(ROOT/'tools'/p) for p in ['render_surface_studies.py','render_alveolar_ao.py','render_reference_studies.py']}
 return settings
def reference_setup(settings):
 scene,material=_original_setup(settings)
 ao.apply_ao(material,settings)
 return scene,material

def main():
 parser=argparse.ArgumentParser()
 parser.add_argument('--profile',required=True,choices=['lava_rope_lobes','angular_scree'])
 parser.add_argument('--seed',type=int,default=48)
 parser.add_argument('--size',type=int,default=900)
 parser.add_argument('--samples',type=int,default=32)
 parser.add_argument('--view',default='main',choices=['main','alternate','detail'])
 parser.add_argument('--force',action='store_true')
 parser.add_argument('--variant',default='new',choices=['new','control'])
 parser.add_argument('--output-dir',type=Path,default=ROOT/'renders/reference-v15')
 args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else None)
 args.output_dir=args.output_dir.resolve()
 (args.output_dir/'provenance').mkdir(parents=True,exist_ok=True)
 directory=ROOT/'out'/f'{"" if args.variant=="new" else "control-"}{args.profile}-{args.seed}'
 entry=base.preflight_directory(directory)
 base.settings_for=reference_settings
 base.setup_scene=reference_setup
 base.render_one(args,entry,args.profile,args.seed,args.variant,args.view,'neutral')
 record_path=args.output_dir/'provenance'/f'{args.profile}-{args.seed}-{args.variant}-{args.view}-neutral.json'
 import json
 record=json.loads(record_path.read_text())
 record['renderer_dependencies']=record['settings']['renderer_dependencies']
 base.save_json(record_path,record)
if __name__=='__main__':main()
