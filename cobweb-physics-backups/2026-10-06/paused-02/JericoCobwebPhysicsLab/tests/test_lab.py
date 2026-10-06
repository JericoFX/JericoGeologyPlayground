import json, math, pathlib, struct, subprocess, tempfile, unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
BIN=ROOT/'build/cobweb_lab'
class LabTests(unittest.TestCase):
    def generate(self,preset='radial',**opts):
        self.assertTrue(BIN.is_file(),'C++ CLI generator must exist before assets can be generated')
        temp=tempfile.TemporaryDirectory(); self.addCleanup(temp.cleanup)
        out=pathlib.Path(temp.name)
        args=[str(BIN),'--preset',preset,'--out',str(out),'--atlas','256']
        for k,v in opts.items(): args += ['--'+k.replace('_','-'),str(v)]
        r=subprocess.run(args,capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stderr)
        return out,json.loads((out/'metadata.json').read_text())
    def pixels(self,path):
        b=path.read_bytes(); w,h=struct.unpack_from('<HH',b,12)
        self.assertEqual(b[2],2); self.assertEqual(b[16],32)
        return [b[i+2]/255 for i in range(18,len(b),4)],w,h
    def test_same_recipe_is_byte_deterministic(self):
        a,_=self.generate(seed=42); b,_=self.generate(seed=42)
        for f in ('mesh.obj','coverage.tga','orientation.tga','curves.json','metadata.json','recipe.json'):
            self.assertEqual((a/f).read_bytes(),(b/f).read_bytes(),f)
    def test_two_seeds_change_fibers_not_fixed_anchors(self):
        a,ma=self.generate(seed=42); b,mb=self.generate(seed=99)
        self.assertNotEqual((a/'coverage.tga').read_bytes(),(b/'coverage.tga').read_bytes())
        self.assertEqual(ma['anchors'],mb['anchors'])
    def test_controls_preserve_anchors_and_carrier_count(self):
        a,ma=self.generate('corridor',density=.2,sag=.1,tension=.9)
        b,mb=self.generate('corridor',density=.95,sag=.8,tension=.1)
        self.assertEqual(ma['anchors'],mb['anchors'])
        self.assertEqual(ma['stats']['triangles'],mb['stats']['triangles'])
        self.assertGreater(mb['stats']['fiber_curves'],ma['stats']['fiber_curves'])
    def test_all_presets_export_finite_valid_geometry(self):
        for preset in ('radial','corner','corridor'):
            with self.subTest(preset=preset):
                out,m=self.generate(preset)
                verts=[]; uv=[]; faces=[]; objects=[]
                for line in (out/'mesh.obj').read_text().splitlines():
                    t=line.split()
                    if not t: continue
                    if t[0]=='v': verts.append(tuple(map(float,t[1:])))
                    if t[0]=='vt': uv.append(tuple(map(float,t[1:])))
                    if t[0]=='f': faces.append([int(s.split('/')[0])-1 for s in t[1:]])
                    if t[0]=='o': objects.append(t[1])
                self.assertEqual(len(objects),1); self.assertGreater(len(faces),0)
                self.assertLess(len(faces),10000)
                for v in verts:
                    self.assertTrue(all(math.isfinite(x) for x in v))
                    self.assertTrue(all(m['aabb']['min'][i]-1e-7<=v[i]<=m['aabb']['max'][i]+1e-7 for i in range(3)))
                for p in uv: self.assertTrue(all(math.isfinite(x) and 0<=x<=1 for x in p))
                for f in faces:
                    self.assertEqual(len(f),3)
                    self.assertTrue(all(0<=i<len(verts) for i in f))
                    a,b,c=[verts[i] for i in f]
                    u=[b[i]-a[i] for i in range(3)]; v=[c[i]-a[i] for i in range(3)]
                    cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
                    self.assertGreater(sum(x*x for x in cross),1e-16)
                for anchor in m['anchors']:
                    self.assertTrue(any(sum((v[i]-anchor['position'][i])**2 for i in range(3))<1e-12 for v in verts))
    def test_corridor_entire_faces_have_separating_axis_from_void(self):
        out,m=self.generate('corridor',sag=1,tension=0,layers=3)
        v=[]
        for line in (out/'mesh.obj').read_text().splitlines():
            s=line.split()
            if s and s[0]=='v': v.append(list(map(float,s[1:])))
            if s and s[0]=='f':
                tri=[v[int(x.split('/')[0])-1] for x in s[1:]]
                separated=(max(p[0] for p in tri)<=-.72 or min(p[0] for p in tri)>=.72 or min(p[2] for p in tri)>=2.4)
                self.assertTrue(separated,'Whole triangles, not merely vertices, must avoid the central prism')
        self.assertEqual(m['stats']['clearance_intersections'],0)
        self.assertGreaterEqual(m['stats']['main_patches'],8)
        self.assertLessEqual(m['stats']['main_patches'],12)
    def test_coverage_is_finite_nonempty_and_not_solid(self):
        for preset in ('radial','corner','corridor'):
            p,m=self.generate(preset)
            pixels,w,h=self.pixels(p/'coverage.tga')
            self.assertEqual((w,h),(256,256))
            self.assertGreater(sum(pixels)/len(pixels),.001)
            self.assertLess(sum(pixels)/len(pixels),.55)
            self.assertEqual(m['stats']['runtime_web_objects'],1)
            self.assertEqual(m['stats']['fiber_tubes'],0)
    def test_density_adds_coverage_without_adding_triangles(self):
        a,ma=self.generate('corner',density=.1); b,mb=self.generate('corner',density=.9)
        self.assertGreater(sum(self.pixels(b/'coverage.tga')[0]),sum(self.pixels(a/'coverage.tga')[0]))
        self.assertEqual(ma['stats']['triangles'],mb['stats']['triangles'])
    def test_damage_only_removes_coverage(self):
        a,_=self.generate('corner',broken=0); b,_=self.generate('corner',broken=.8)
        x=self.pixels(a/'coverage.tga')[0]; y=self.pixels(b/'coverage.tga')[0]
        self.assertTrue(all(q<=p for p,q in zip(x,y)))
        self.assertLess(sum(y),sum(x))
    def test_subpixel_fiber_width_controls_integrated_coverage(self):
        a,_=self.generate('radial',width=.0001,broken=0)
        b,_=self.generate('radial',width=.001,broken=0)
        thin=sum(self.pixels(a/'coverage.tga')[0]); thick=sum(self.pixels(b/'coverage.tga')[0])
        self.assertGreater(thick,thin*3,'Subpixel coverage must shrink with physical fiber width')
    def test_atlas_unallocated_tiles_remain_empty_at_max_width(self):
        out,_=self.generate('corridor',density=0,width=.03,broken=0,atlas=2048)
        data,w,h=self.pixels(out/'coverage.tga')
        self.assertTrue(all(v==0 for v in data[(h*3//4)*w:]),'Fiber footprints must never bleed into an adjacent or unused tile')
    def test_existing_output_requires_explicit_overwrite(self):
        out,_=self.generate('radial',seed=42)
        before=(out/'coverage.tga').read_bytes()
        command=[str(BIN),'--preset','radial','--out',str(out),'--seed','99','--atlas','256']
        denied=subprocess.run(command,capture_output=True,text=True)
        self.assertNotEqual(denied.returncode,0)
        self.assertIn('exist',denied.stderr.lower())
        self.assertEqual(before,(out/'coverage.tga').read_bytes())
        approved=subprocess.run(command+['--overwrite'],capture_output=True,text=True)
        self.assertEqual(approved.returncode,0,approved.stderr)
        self.assertNotEqual(before,(out/'coverage.tga').read_bytes())
    def test_seed_requires_unsigned_decimal_digits(self):
        self.assertTrue(BIN.is_file())
        for value in (' -1','-1','\t-1','+1','1 ',''):
            with self.subTest(value=value), tempfile.TemporaryDirectory() as out:
                result=subprocess.run([str(BIN),'--seed',value,'--out',out,'--atlas','64'],capture_output=True,text=True)
                self.assertNotEqual(result.returncode,0,'Only unsigned decimal digits are valid seed input')
                self.assertIn('seed',result.stderr.lower())
                self.assertFalse((pathlib.Path(out)/'metadata.json').exists())
    def test_invalid_controls_fail_without_success_assets(self):
        self.assertTrue(BIN.is_file(),'C++ CLI generator must exist')
        for flag,value in (('--density','nan'),('--sag','-1'),('--layers','9'),('--atlas','0'),('--preset','unknown')):
            with tempfile.TemporaryDirectory() as d:
                r=subprocess.run([str(BIN),'--out',d,flag,value],capture_output=True,text=True)
                self.assertNotEqual(r.returncode,0); self.assertIn('error',r.stderr.lower())
                self.assertFalse((pathlib.Path(d)/'metadata.json').exists())
if __name__=='__main__': unittest.main()
