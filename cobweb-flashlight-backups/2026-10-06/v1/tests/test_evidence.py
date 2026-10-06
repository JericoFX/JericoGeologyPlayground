import json
from pathlib import Path
import sys
import unittest
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from contract import CLASSES,asset_identity

class GeneratedEvidenceTests(unittest.TestCase):
    def test_baked_mesh_and_anchors_match_across_classes(self):
        meshes=[];anchors=[]
        for c in CLASSES:
            root=ROOT/'assets'/c['id']
            self.assertTrue(root.exists())
            meshes.append(asset_identity(root)['mesh.obj'])
            anchors.append(json.loads((root/'metadata.json').read_text())['anchors'])
        self.assertEqual(len(set(meshes)),1)
        self.assertTrue(all(x==anchors[0] for x in anchors))
    def test_bake_asset_hashes_unchanged(self):
        known=json.loads((ROOT/'assets/identities.json').read_text())
        for c in CLASSES:self.assertEqual(asset_identity(ROOT/'assets'/c['id']),known[c['id']])
    def test_final_render_evidence(self):
        run=ROOT/'renders/final01'
        self.assertTrue((run/'measurement.json').exists(),'Run final renders and analyze_study before delivery')
        m=json.loads((run/'manifest.json').read_text());q=json.loads((run/'measurement.json').read_text())
        self.assertEqual(len(m['frames']),5)
        self.assertEqual(len({f['scene_identity'] for f in m['frames']}),1)
        self.assertEqual(len(set(m['contract']['mesh_hashes'].values())),1)
        self.assertEqual(q['coverage_AOV_max_absolute_difference'],0)
        arrays=[np.load(p) for p in run.glob('*_coverage_*.npy')]
        self.assertEqual(len(arrays),5)
        for a in arrays:self.assertTrue(np.array_equal(a,arrays[0]))
        for c in q['classes'].values():
            self.assertGreater(c['direct_to_off_mean_radiance_ratio'],5)
            self.assertLess(c['display_clipped_pixel_fraction'],.01)

if __name__=='__main__':unittest.main()
