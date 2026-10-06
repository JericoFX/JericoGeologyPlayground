import sys,unittest
from pathlib import Path
root=Path(__file__).resolve().parent;sys.path.insert(0,str(root))
suite=unittest.defaultTestLoader.discover(str(root/'tests'))
result=unittest.TextTestRunner(verbosity=2).run(suite)
if not result.wasSuccessful():raise RuntimeError('Blender tests failed')
