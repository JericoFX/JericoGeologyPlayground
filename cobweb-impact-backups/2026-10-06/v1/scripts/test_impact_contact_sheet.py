import importlib.util
import pathlib
import unittest

class ContactSheetTests(unittest.TestCase):
    def test_letterboxing_preserves_source_aspect(self):
        path=pathlib.Path(__file__).with_name('impact_contact_sheet.py')
        self.assertTrue(path.exists(), 'The impact contact-sheet helper must exist')
        spec=importlib.util.spec_from_file_location('impact_contact_sheet',path)
        mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
        for width,height in [(1100,900),(900,900),(300,600)]:
            w,h=mod.fit_cell(width,height)
            self.assertLessEqual(w,.76+1e-12)
            self.assertLessEqual(h,.76*900/1100+1e-12)
            self.assertAlmostEqual(w/h,width/height)

if __name__=='__main__':unittest.main()
