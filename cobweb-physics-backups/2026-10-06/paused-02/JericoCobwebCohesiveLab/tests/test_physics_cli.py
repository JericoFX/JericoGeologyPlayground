import json,pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
BIN=ROOT/'build/cobweb_sim'
class PhysicsCliTests(unittest.TestCase):
 def run_sim(self,out,*args):
  return subprocess.run([str(BIN),'--out',str(out),'--ticks','8','--atlas','64',*args],capture_output=True,text=True)
 def test_valid_run_exports_actual_states(self):
  with tempfile.TemporaryDirectory() as d:
   out=pathlib.Path(d)/'run';r=self.run_sim(out);self.assertEqual(r.returncode,0,r.stderr);self.assertTrue((out/'final.checkpoint').is_file());self.assertTrue((out/'01_start/mesh.obj').is_file());self.assertTrue((out/'05_final/frame.json').is_file())
 def test_signed_and_oversized_seeds_rejected(self):
  with tempfile.TemporaryDirectory() as d:
   for seed in ['-1','+42','18446744073709551616','2x']:
    r=self.run_sim(pathlib.Path(d)/seed,'--seed',seed);self.assertNotEqual(r.returncode,0);self.assertIn('seed',r.stderr.lower())
 def test_finite_input_bounds(self):
  with tempfile.TemporaryDirectory() as d:
   for flag,v in [('--width','nan'),('--strength','inf'),('--density','2'),('--ticks','-1'),('--atlas','4096')]:
    self.assertNotEqual(self.run_sim(pathlib.Path(d)/(flag+v),flag,v).returncode,0)
 def test_deterministic_and_checkpoint_resume(self):
  with tempfile.TemporaryDirectory() as d:
   a,b,c=[pathlib.Path(d)/x for x in 'abc'];self.assertEqual(self.run_sim(a,'--ticks','12').returncode,0);self.assertEqual(self.run_sim(b,'--ticks','6').returncode,0);self.assertEqual(self.run_sim(c,'--ticks','12','--resume',str(b/'final.checkpoint')).returncode,0);self.assertEqual((a/'final.checkpoint').read_bytes(),(c/'final.checkpoint').read_bytes())
 def test_resume_rejects_mismatched_asset_and_script(self):
  with tempfile.TemporaryDirectory() as d:
   a=pathlib.Path(d)/"a";self.assertEqual(self.run_sim(a).returncode,0)
   for flag,value in [("--seed","99"),("--width","2"),("--mass",".02")]:
    r=self.run_sim(pathlib.Path(d)/(flag+value),flag,value,"--resume",str(a/"final.checkpoint"));self.assertNotEqual(r.returncode,0);self.assertIn("checkpoint",r.stderr.lower())
   script=pathlib.Path(d)/"script.txt";script.write_text("# changed script\n")
   r=self.run_sim(pathlib.Path(d)/"changed_script","--script",str(script),"--resume",str(a/"final.checkpoint"));self.assertNotEqual(r.returncode,0);self.assertIn("checkpoint",r.stderr.lower())
 def test_short_no_break_run_is_labeled_truthfully(self):
  with tempfile.TemporaryDirectory() as d:
   script=pathlib.Path(d)/"none.txt";script.write_text("# no interactions\n");out=pathlib.Path(d)/"run"
   r=self.run_sim(out,"--gravity","0","--script",str(script));self.assertEqual(r.returncode,0,r.stderr)
   summary=json.loads((out/"summary.json").read_text());self.assertFalse(summary["tear_observed"]);self.assertFalse(summary["final_fixture_release_is_scripted"])
   self.assertIn("no break observed",(out/"stages.json").read_text())
 def test_restored_frame_checkpoint_matches_exported_state(self):
  with tempfile.TemporaryDirectory() as d:
   a,b,c=[pathlib.Path(d)/x for x in "abc"];self.assertEqual(self.run_sim(a,"--ticks","4").returncode,0)
   self.assertEqual(self.run_sim(b,"--ticks","8","--resume",str(a/"final.checkpoint"),"--snapshot-every","2").returncode,0)
   self.assertEqual(json.loads((b/"01_start/frame.json").read_text())["tick"],4)
   self.assertEqual(self.run_sim(c,"--ticks","8","--resume",str(b/"01_start/replay.checkpoint")).returncode,0)
   self.assertEqual((b/"final.checkpoint").read_bytes(),(c/"final.checkpoint").read_bytes())
   self.assertTrue((b/"frame_000006/replay.checkpoint").is_file())
 def test_pull_ramps_force_in_physical_time(self):
  with tempfile.TemporaryDirectory() as d:
   states=[]
   for name,command in [("force","force 0 1 459 0.01 0 0\n"),("pull","pull 0 100 459 1 0 0\n")]:
    script=pathlib.Path(d)/(name+".txt");script.write_text(command);out=pathlib.Path(d)/name
    r=self.run_sim(out,"--ticks","1","--script",str(script));self.assertEqual(r.returncode,0,r.stderr);states.append(json.loads((out/"05_final/frame.json").read_text())["particles"])
   self.assertEqual(states[0],states[1],"first ramp tick must apply 1/100 of the target force")
 def test_recipe_resolves_its_own_script_relative_to_recipe(self):
  with tempfile.TemporaryDirectory() as d:
   path=pathlib.Path(d);(path/"none.txt").write_text("# no interactions\n");recipe=path/"portable.recipe";recipe.write_text("seed=42\nscript=none.txt\ngravity=0\n")
   out=path/"run";r=self.run_sim(out,"--ticks","1","--recipe",str(recipe));self.assertEqual(r.returncode,0,r.stderr)
   self.assertIn("script=interactions.txt",(out/"recipe.txt").read_text())
 def test_output_is_immutable(self):
  with tempfile.TemporaryDirectory() as d:
   out=pathlib.Path(d)/'same';self.assertEqual(self.run_sim(out).returncode,0);before=(out/'final.checkpoint').read_bytes();self.assertNotEqual(self.run_sim(out).returncode,0);self.assertEqual((out/'final.checkpoint').read_bytes(),before)
if __name__=='__main__':unittest.main()
