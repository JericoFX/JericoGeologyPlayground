"""Parameters for a bounded light-response demonstration, not a physical silk model."""
import hashlib
import math
from pathlib import Path
CLASSES=[
 {'id':'tenue','label':'TENUE','density':.06,'width':.0008,'roughness':.20,'base':.12,'anisotropy':.82,'translucency':.30,'x':-3.5},
 {'id':'normal','label':'NORMAL','density':.45,'width':.0032,'roughness':.36,'base':.22,'anisotropy':.68,'translucency':.28,'x':0},
 {'id':'monstruosa','label':'MONSTRUOSA','density':.90,'width':.006,'roughness':.53,'base':.32,'anisotropy':.48,'translucency':.24,'x':3.5},
]
FIXED={'exposure':0.,'ambient_strength':.075,'ambient_color':[.65,.75,1.,1.], 'seed':17,
 'camera_position':[0,-16,2.15], 'camera_target':[0,0,2.0], 'ortho_scale':11.3,
 'resolution':[1500,620], 'samples':192, 'source_patch':0,'atlas_size':4096, 'background_color':[.035,.045,.060,1.]}
STATES={
 'off':{'position':[0,-5,2.4],'target':[-3.5,0,2.],'energy':0,'spot_size':.55,'spot_blend':.55},
 'left':{'position':[-3.5,-5,2.4],'target':[-3.5,0,2.],'energy':1100,'spot_size':.70,'spot_blend':.60},
 'center':{'position':[0,-5,2.4],'target':[0,0,2.],'energy':1100,'spot_size':.70,'spot_blend':.60},
 'right':{'position':[3.5,-5,2.4],'target':[3.5,0,2.],'energy':1100,'spot_size':.70,'spot_blend':.60},
 'backlight':{'position':[0,2.8,2.8],'target':[0,0,2.],'energy':1100,'spot_size':2.35,'spot_blend':.50},
}
def decode_angle(r,g): return .5*math.atan2(2*g-1,2*r-1)
def require_new(paths):
    for path in paths:
        if Path(path).exists(): raise FileExistsError(f'Immutable output exists: {path}; choose a new run directory')
def sha256(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def asset_identity(root):
    return {name:sha256(Path(root)/name) for name in ['mesh.obj','coverage.tga','orientation.tga']}
