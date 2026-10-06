#!/usr/bin/env python3
"""Generate actual C++ control meshes and numerical A/B evidence, no JS geology."""
import argparse, copy, hashlib, json, pathlib, subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--binary',type=pathlib.Path,default=ROOT/'build/rocks_cli');parser.add_argument('--out',type=pathlib.Path,default=ROOT/'out/heightfield-v10-controls');args=parser.parse_args()
 recipe=json.loads((ROOT/'recipes/heightfield-cliff-48-v10.json').read_text());variants={'default':recipe}
 for seed in [117,305]:variants[f'seed-{seed}']={**recipe,'seed':seed}
 for key,value in [('bedCount',6),('thicknessVariation',0),('tiltDegrees',20),('relief',.45),('fractures',0),('irregularity',0),('size',3),('backDepth',.5),('fragmentedContacts',True)]:variants[key]=copy.deepcopy(recipe);variants[key][key]=value
 variants['mesh-resolution']={**recipe,'grid':{**recipe['grid'],'cells':64}}
 variants['source-resolution']={**recipe,'grid':{**recipe['grid'],'width':1024,'height':1024}}
 report={};base=None;base_codes=None
 for name,r in variants.items():
  folder=args.out/name;folder.mkdir(parents=True,exist_ok=True);source=folder/'input.json';source.write_text(json.dumps(r,indent=2))
  result=subprocess.run([str(args.binary),'generate','--recipe',str(source),'--out',str(folder)],capture_output=True,text=True);assert result.returncode==0,(name,result.stderr)
  mesh=json.loads((folder/'viewer.json').read_text())['stages']['heightfield'];codes=(folder/'height.u16').read_bytes();positions=mesh['positions'];front=(r['grid']['cells']+1)**2;z=positions[2:front*3:3]
  if base is None:base=positions;base_codes=codes
  item={'recipe':r,'objSha256':hashlib.sha256((folder/'heightfield.obj').read_bytes()).hexdigest(),'samplesSha256':hashlib.sha256(codes).hexdigest(),'frontZMin':min(z),'frontZMax':max(z),'frontPeakToValleyMeters':max(z)-min(z),'metrics':json.loads((folder/'metrics.json').read_text())}
  if len(codes)==len(base_codes):item['changedSourceSamples']=sum(codes[i:i+2]!=base_codes[i:i+2] for i in range(0,len(codes),2))
  if len(positions)==len(base):item['changedVertices']=sum(positions[i:i+3]!=base[i:i+3] for i in range(0,len(positions),3));item['maxVertexComponentChange']=max(abs(a-b) for a,b in zip(positions,base))
  report[name]=item
 args.out.mkdir(parents=True,exist_ok=True);(args.out/'ablation-report.json').write_text(json.dumps(report,indent=2));print(args.out/'ablation-report.json')
if __name__=='__main__':main()
