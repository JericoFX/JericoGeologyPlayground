import json,math,unittest,pathlib
import test_lab
class DenseVariantTests(unittest.TestCase):
    generate=test_lab.LabTests.generate
    pixels=test_lab.LabTests.pixels
    def test_corridor_declares_three_asymmetric_density_foci(self):
        _,meta=self.generate('corridor',density=.9)
        self.assertIn('density_foci',meta)
        foci=meta['density_foci']
        self.assertEqual(len(foci),3)
        self.assertEqual([f['patch'] for f in foci],[0,4,8])
        self.assertEqual(len({tuple(f['uv']) for f in foci}),3)
        self.assertTrue(all(f['radius'][0]!=f['radius'][1] for f in foci))
    def test_corridor_supports_match_v2_checkpoint(self):
        _,meta=self.generate('corridor',density=.4,sag=.8,tension=.1)
        expected=json.loads((pathlib.Path(__file__).parent/'fixtures/v2_corridor_anchors.json').read_text())
        self.assertEqual(meta['anchors'],expected)
    def test_three_foci_concentrate_coverage_over_their_surroundings(self):
        out,meta=self.generate('corridor',density=.9,broken=0,width=.0045,atlas=1024)
        self.assertIn('density_foci',meta)
        pixels,w,h=self.pixels(out/'coverage.tga');grid=4;tile=w//grid
        for f in meta['density_foci']:
            inside=[];outside=[];pid=f['patch'];cx,cy=f['uv'];rx,ry=f['radius']
            for y in range(8,tile-8):
                for x in range(8,tile-8):
                    u=(x-3)/(tile-6);v=(y-3)/(tile-6)
                    ell=((u-cx)/rx)**2+((v-cy)/ry)**2
                    value=pixels[(pid//grid*tile+y)*w+pid%grid*tile+x]
                    if ell<.5:inside.append(value)
                    elif ell>2:outside.append(value)
            self.assertGreater(sum(inside)/len(inside),1.3*sum(outside)/len(outside),f'patch {pid} needs a real localized mass')
if __name__=='__main__':unittest.main()
