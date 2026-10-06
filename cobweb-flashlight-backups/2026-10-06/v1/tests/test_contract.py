import importlib.util
import json
import math
from pathlib import Path
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
try:
    import contract
except ImportError:
    contract=None

class ContractTests(unittest.TestCase):
    def require_contract(self):
        self.assertIsNotNone(contract,'Missing light-independent experiment contract')
        return contract
    def test_three_distinct_visual_classes(self):
        c=self.require_contract()
        self.assertEqual([x['id'] for x in c.CLASSES],['tenue','normal','monstruosa'])
        self.assertEqual(len(set(x['density'] for x in c.CLASSES)),3)
        self.assertEqual(len(set(x['width'] for x in c.CLASSES)),3)
        self.assertEqual(len(set(x['roughness'] for x in c.CLASSES)),3)
    def test_states_only_change_light(self):
        c=self.require_contract()
        self.assertEqual(set(c.STATES),{'off','left','center','right','backlight'})
        allowed={'position','target','energy','spot_size','spot_blend'}
        for state in c.STATES.values(): self.assertLessEqual(set(state),allowed)
        self.assertEqual(c.STATES['off']['energy'],0)
        self.assertEqual(c.STATES['left']['energy'],c.STATES['right']['energy'])
    def test_fixed_camera_exposure_ambient(self):
        c=self.require_contract()
        self.assertEqual(c.FIXED['exposure'],0)
        self.assertGreater(c.FIXED['ambient_strength'],0)
        self.assertLess(c.FIXED['ambient_strength'],.1)
        self.assertEqual(c.FIXED['seed'],17)
    def test_double_angle_decode_is_unoriented(self):
        c=self.require_contract()
        for theta in [-math.pi/2,-.75,0,.45,math.pi/2]:
            r=.5+.3*math.cos(2*theta);g=.5+.3*math.sin(2*theta)
            decoded=c.decode_angle(r,g)
            self.assertAlmostEqual(math.cos(2*decoded),math.cos(2*theta),places=7)
            self.assertAlmostEqual(math.sin(2*decoded),math.sin(2*theta),places=7)
    def test_existing_output_rejected(self):
        c=self.require_contract()
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'out.png';path.write_bytes(b'original')
            with self.assertRaises(FileExistsError): c.require_new([path])
            self.assertEqual(path.read_bytes(),b'original')
    def test_asset_identity_does_not_depend_on_light(self):
        c=self.require_contract()
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            for name in ['mesh.obj','coverage.tga','orientation.tga']: (root/name).write_bytes(name.encode())
            identities=[c.asset_identity(root) for _ in c.STATES]
            self.assertTrue(all(x==identities[0] for x in identities))
            before=identities[0];(root/'coverage.tga').write_bytes(b'changed')
            self.assertNotEqual(c.asset_identity(root)['coverage.tga'],before['coverage.tga'])

if __name__=='__main__': unittest.main()
