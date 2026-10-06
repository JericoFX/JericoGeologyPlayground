"""Save a separate relative-path scene for sub-20 MB recovery archives; never edit final01."""
import json
import os
from pathlib import Path
import sys
import bpy
ROOT=Path(__file__).resolve().parents[1]
source=ROOT/'renders/final01/study.blend'
out=ROOT/'persistence/recovery_project_v2';out.mkdir(exist_ok=True)
target=out/'study.blend'
assert not target.exists()
bpy.ops.wm.open_mainfile(filepath=str(source))
assets=[]
for image in bpy.data.images:
    if image.type!='IMAGE' or not image.filepath:continue
    sourcepath=Path(bpy.path.abspath(image.filepath))
    if sourcepath.name not in ('coverage.tga','orientation.tga'):continue
    c=sourcepath.parent.name
    if image.packed_file:image.unpack(method='REMOVE')
    image.filepath='//assets/'+c+'/'+sourcepath.name
    assets.append(image.filepath)
bpy.ops.wm.save_as_mainfile(filepath=str(target),compress=True,relative_remap=False)
# Link the already generated assets only in the local validation staging directory.
# The delivered archives contain regular assets files and do not rely on this symlink.
link=out/'assets'
if not link.exists():link.symlink_to(ROOT/'assets',target_is_directory=True)
bpy.ops.wm.open_mainfile(filepath=str(target))
reloaded=[i for i in bpy.data.images if i.type=='IMAGE' and i.filepath]
assert len(reloaded)==6
for image in reloaded:
    assert image.filepath.startswith('//assets/'), image.filepath
    assert Path(bpy.path.abspath(image.filepath)).is_file(), image.filepath
    assert not image.packed_file
    image.reload()
assert len(assets)==6
report={'relative_asset_references':assets,'image_files_verified':6,'all_relative':True,'original_packed_scene_unchanged':True,
 'recovery':'Extract scene archive and three class assets archives into the same directory; open study.blend'}
(out/'recovery_validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('RECOVERY_SCENE_OK',target)
