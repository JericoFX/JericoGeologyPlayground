#!/usr/bin/env python3
"""Generate the original procedural scene contract without external dependencies."""
import argparse
import json
import pathlib
from core import asset_hash, validate_scene
from recipe_router import build_recipe as build_asset


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recipe',type=pathlib.Path)
    parser.add_argument('--out',type=pathlib.Path,required=True)
    for key in ['kind','damage','handle','preset','glazing']: parser.add_argument('--'+key)
    for key in ['width','height','thickness','angle']: parser.add_argument('--'+key,type=float)
    parser.add_argument('--seed',type=int)
    args=parser.parse_args()
    try:
        recipe=json.loads(args.recipe.read_text()) if args.recipe else {}
        for key in ['kind','damage','handle','preset','glazing','width','height','thickness','angle','seed']:
            value=getattr(args,key)
            if value is not None: recipe[key]=value
        scene=build_asset(recipe)
        errors=validate_scene(scene)
        if errors: raise ValueError('\n'.join(errors))
        args.out.parent.mkdir(parents=True,exist_ok=True)
        args.out.write_text(json.dumps(scene,indent=2,allow_nan=False)+'\n')
        print(json.dumps({'path':str(args.out),'sha256':asset_hash(scene),'parts':len(scene['parts']),'clearance':scene['metadata']['clearance']}))
    except (ValueError,TypeError,OSError) as exc: parser.error(str(exc))

if __name__=='__main__': main()
