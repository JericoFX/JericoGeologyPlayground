"""Run all native evaluation families. Inputs are frozen oracle recipes only."""
import json,subprocess,tempfile,argparse
from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--exe',default=str(r/'build/jerico_procedural'));p.add_argument('--report',default=str(r/'output/evaluated-family-gate.json'));o=p.parse_args();cases=[]
for f in sorted((r/'fixtures/sources/JericoIndustrialPropsMaterialsR2Checkpoint_20261006_1927/recipes').glob('industrial_*.json')):
 j=json.loads(f.read_text())
 if not j.get('operations'):cases.append(('industrial',f.name,j))
for kind in ['wood','furniture','damp']:
 for f in sorted((r/'fixtures'/kind).glob('*.json')):
  if f.name=='manifest.json':continue
  s=json.loads(f.read_text());j=s.get('recipe') if isinstance(s,dict) else None
  if j:cases.append(('singlepane' if j.get('preset')=='single_pane_fixed' else kind,f.name,j))
report=[]
for family,name,j in cases:
 with tempfile.TemporaryDirectory() as d:
  inp=Path(d)/'r.json';out=Path(d)/'s.json';inp.write_text(json.dumps(j));c=subprocess.run([o.exe,'--family',family,'--recipe',str(inp),'--out',str(out),'--evaluate','1'],capture_output=True,text=True)
  report.append(dict(family=family,fixture=name,passed=c.returncode==0,error=c.stderr.strip()))
  print(family,name,'PASS' if c.returncode==0 else c.stderr[:260],flush=True)
Path(o.report).write_text(json.dumps(report,indent=2));print(sum(x['passed'] for x in report),'/',len(report),'passed')
raise SystemExit(not all(x['passed'] for x in report))
