import importlib.util
import json
import tempfile
import pathlib
import unittest

class EvidenceTests(unittest.TestCase):
    def setUp(self):self.assertIsNotNone(importlib.util.find_spec('evidence'),'Immutable evidence module required')
    def test_manifest_is_content_addressed_and_prior_manifest_survives(self):
        from evidence import freeze_manifest
        with tempfile.TemporaryDirectory() as d:
            a=freeze_manifest([{'view':'closed','file':'a.png'}],pathlib.Path(d))
            b=freeze_manifest([{'view':'open','file':'b.png'}],pathlib.Path(d))
            self.assertNotEqual(a,b)
            self.assertTrue(a.exists())
            self.assertEqual(json.loads(a.read_text())[0]['view'],'closed')
    def test_duplicate_bytes_reuse_immutable_file(self):
        from evidence import freeze_file
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'working.png';p.write_bytes(b'known data')
            a=freeze_file(p,'closed')
            p.write_bytes(b'known data');b=freeze_file(p,'closed')
            self.assertEqual(a,b);self.assertTrue(pathlib.Path(a['file']).exists())
    def test_existing_hash_path_with_wrong_bytes_is_not_silently_overwritten(self):
        from evidence import freeze_file
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'working.png';p.write_bytes(b'known data');a=freeze_file(p,'closed')
            pathlib.Path(a['file']).write_bytes(b'corrupt')
            p.write_bytes(b'known data')
            with self.assertRaises(RuntimeError):freeze_file(p,'closed')

if __name__=='__main__':unittest.main()
