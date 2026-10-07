"""Conservative shader-only contour reduction; source geometry stays untouched."""
import copy
import importlib.util
import math
import unittest

class FineContourTests(unittest.TestCase):
    def reducer(self):
        self.assertIsNotNone(importlib.util.find_spec('fine_wear_math'), 'Fine contour optimizer missing')
        from fine_wear_math import contour_segments
        return contour_segments

    def test_merges_only_straight_degree_two_vertices_and_clips_conservatively(self):
        reduce = self.reducer()
        config = dict(width_m=.025, region=dict(shape='box',min=[-.01,-.1,.02],max=[.03,.1,.08]),
                      boundary_segments_xz=[[[0,0],[0,.05]],[[0,.05],[0,.1]],[[0,.1],[.1,.1]],[[.1,.1],[.1,0]],[[.1,0],[0,0]]])
        original=copy.deepcopy(config)
        reduced=reduce(config)
        self.assertLess(len(reduced),len(config['boundary_segments_xz']))
        self.assertEqual(config,original)
        self.assertNotIn([[.1,.1],[.1,0]],reduced)
        branches=dict(config,boundary_segments_xz=[[[0,0],[0,.05]],[[0,.05],[0,.1]],[[0,.05],[.02,.06]]])
        self.assertEqual(len(reduce(branches)),3)

    def test_box_and_sphere_masks_match_original_on_dense_grid(self):
        reduce = self.reducer()
        from localized_damage import _segment_distance, smootherstep
        segments=[[[0,0],[0,.03]],[[0,.03],[0,.07]],[[0,.07],[0,.1]],[[0,.1],[.1,.1]],[[.1,.1],[.1,0]],[[.1,0],[0,0]]]
        for region in (dict(shape='box',min=[-.01,-.1,.015],max=[.03,.1,.085]),dict(shape='sphere',center=[.012,0,.05],radius=.025)):
            config=dict(width_m=.012,boundary_segments_xz=segments,region=region)
            optimized=reduce(config)
            for ix in range(101):
                for iz in range(101):
                    x,z=ix/1000,iz/1000
                    inside=(region['min'][0]<=x<=region['max'][0] and region['min'][2]<=z<=region['max'][2]) if region['shape']=='box' else math.hypot(x-region['center'][0],z-region['center'][2])<=region['radius']
                    if not inside:continue
                    weights=[1-smootherstep(min((_segment_distance([x,z],a,b) for a,b in ss),default=math.inf)/config['width_m']) if ss else 0 for ss in (segments,optimized)]
                    self.assertAlmostEqual(*weights,delta=1e-10)

    def test_real_fine_recipe_masks_match_and_reduce_segment_budget(self):
        reduce = self.reducer()
        from pathlib import Path
        import json
        from industrial import build_industrial
        from localized_damage import _segment_distance,smootherstep
        scene=build_industrial(json.loads(Path('recipes/fine_wear_after.json').read_text()))
        before=after=0
        for part in scene['parts']:
            for config in part.get('edge_wear',[]):
                original=config['boundary_segments_xz'];optimized=reduce(config)
                before+=len(original);after+=len(optimized)
                lo,hi=config['region']['min'],config['region']['max']
                for ix in range(31):
                    for iz in range(31):
                        point=[lo[0]+(hi[0]-lo[0])*ix/30,lo[2]+(hi[2]-lo[2])*iz/30]
                        vals=[1-smootherstep(min((_segment_distance(point,a,b) for a,b in segments),default=math.inf)/config['width_m']) if segments else 0 for segments in (original,optimized)]
                        self.assertAlmostEqual(*vals,delta=1e-10)
        self.assertLess(after,before/2,(before,after))

if __name__=='__main__':unittest.main()
