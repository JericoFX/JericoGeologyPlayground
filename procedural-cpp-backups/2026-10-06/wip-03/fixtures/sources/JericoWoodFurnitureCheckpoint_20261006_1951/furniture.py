"""Reference-led procedural wooden furniture. Authoring geometry only; metres."""
from __future__ import annotations
import math, random, copy
from mesh_core import Builder, _uvs, _cross, _sub, _dot, asset_hash, validate_scene

DEFAULTS=dict(preset='cabinet',width=.76,depth=.52,height=1.32,drawer_count=4,drawer=True,shelf=True,seed=41,open_fraction=0.,wear=.7)

def normalize_recipe(recipe):
 if not isinstance(recipe,dict):raise TypeError('Recipe must be an object')
 r=dict(DEFAULTS)
 if recipe.get('preset')=='workbench':r.update(width=1.65,depth=.68,height=.91,drawer_count=1)
 if set(recipe)-set(r):raise ValueError('Unknown fields: '+str(set(recipe)-set(r)))
 r.update(recipe)
 if r['preset'] not in ('cabinet','workbench'):raise ValueError('Unknown preset')
 for k,lo,hi in [('width',.55,2.4),('depth',.4,.95),('height',.65,2.1),('open_fraction',0,1),('wear',0,1)]:
  if isinstance(r[k],bool) or not isinstance(r[k],(float,int)) or not math.isfinite(r[k]) or not lo<=r[k]<=hi:raise ValueError(k+' outside supported range')
  r[k]=float(r[k])
 for k in ['drawer_count','seed']:
  if isinstance(r[k],bool) or not isinstance(r[k],int):raise ValueError(k+' must be integer')
 if not 1<=r['drawer_count']<=8:raise ValueError('drawer_count must be 1–8')
 for k in ['drawer','shelf']:
  if not isinstance(r[k],bool):raise ValueError(k+' must be boolean')
 if r['preset']=='cabinet' and (r['height']-.22)/r['drawer_count']<.15:raise ValueError('Drawer bays must be at least 150 mm')
 return r

class FurnitureBuilder(Builder):
 def __init__(self,r):
  super().__init__(r)
  self.scene['metadata'].update(generator='JericoWoodFurnitureLab/furniture.py',leaf_envelopes=[],frame_envelopes=[],drawers=[],static_envelopes=[],references=['cajonera4cajones','bancocajon+balda'],clearance_scope='Conservative complete drawer shell AABB swept through full -Y travel against fixed structural envelopes. Hardware is constrained within its front envelope. Mortise and fastener overlaps are intentional.')
  self.node('frame',[0,0,0],'Furniture',None)
 def board(self,id,node,lo,hi,role='Plank',face='front',groove=False):
  """Solid with chamfered outline and a shallow, grain-aligned carved valley.

  Local U follows the longest in-plane dimension. Only selected surfaces carry
  one recessed groove. Bevel is evaluated by Blender; geometry is never noise-
  displaced uniformly. Exposed end-grain is assigned to perimeter faces.
  """
  if face=='front': origin=[lo[0],lo[1],lo[2]];U=[1,0,0];V=[0,0,1];N=[0,-1,0];L=hi[0]-lo[0];W=hi[2]-lo[2];T=hi[1]-lo[1]
  elif face=='side':origin=[hi[0],lo[1],lo[2]];U=[0,1,0];V=[0,0,1];N=[1,0,0];L=hi[1]-lo[1];W=hi[2]-lo[2];T=hi[0]-lo[0]
  else:origin=[lo[0],lo[1],hi[2]];U=[1,0,0];V=[0,1,0];N=[0,0,1];L=hi[0]-lo[0];W=hi[1]-lo[1];T=hi[2]-lo[2]
  if W>L:
   origin=[origin[i]+L*U[i] for i in range(3)];U,V=V,[-x for x in U];L,W=W,L
  wear=self.r['wear'];c=min(.006*wear,L*.06,W*.09);c=max(.0002,c)
  a=self.rng.uniform(.24,.41);end=self.rng.uniform(.64,.83)
  uk=sorted(set([0,c/L,.1,a,(a+end)/2,end,.91,1-c/L,1]))
  center=self.rng.uniform(.37,.63);half=min(.009/W,.045)
  vk=[0,.035,center-half,center,center+half,.965,1]
  d=min(T*.15,.0035)*wear if groove else 0
  verts=[];rows=len(vk);cols=len(uk)
  for side in [0,1]:
   for u in uk:
    edge=c*max(0,1-u*L/c,1-(1-u)*L/c)
    for j,v in enumerate(vk):
     notch=min(.003,W*.035)*wear*max(0,1-abs(u-end)/.075) if groove else 0
     s=edge+notch+(W-2*edge-notch)*v
     # Small wandering along the single selected crack, entirely in-plane.
     if j in [2,3,4]:s+=math.sin(u*18+1)*min(.0015,W*.01)
     k=max(0,math.sin(math.pi*(u-a)/(end-a))) if a<u<end else 0
     depth=-T if side else (-d*k if j==3 else 0)
     verts.append([origin[i]+L*u*U[i]+s*V[i]+depth*N[i] for i in range(3)])
  faces=[];mats=[]
  for side in [0,1]:
   off=side*rows*cols
   for i in range(cols-1):
    for j in range(rows-1):
     f=[off+i*rows+j,off+(i+1)*rows+j,off+(i+1)*rows+j+1,off+i*rows+j+1]
     if side:f.reverse()
     # Triangles enforce planar faces on the carved, non-planar surface.
     faces.extend([f[:3],[f[0],f[2],f[3]]]);mats.extend(['wood']*2)
  ring=[i*rows for i in range(cols)]+[(cols-1)*rows+j for j in range(1,rows)]+[i*rows+rows-1 for i in range(cols-2,-1,-1)]+list(range(rows-2,0,-1))
  for p,q in zip(ring,ring[1:]+ring[:1]):faces.append([q,p,p+rows*cols,q+rows*cols]);mats.append('wood_edge')
  part=self.mesh(id,node,verts,faces,'wood',min(.0025,T*.08,W*.03),role)
  part.update(face_materials=mats,grain_axis=U,groove_depth_m=d,outline_chamfer_m=c,bevel_material='wood_edge',bounds=[lo,hi])
  return part
 def fixed_board(self,id,lo,hi,role='Structure',**kw):
  p=self.board(id,'frame',lo,hi,role,**kw)
  self.scene['metadata']['static_envelopes'].append(dict(id=id,lo=lo,hi=hi))
  return p
 def bolt(self,id,node,x,y,z,r=.005):
  self.cylinder(id,node,[x,y,z],[x,y-.0035,z],r,'iron',12,.0006,'Bolt')
  # Recessed slot represented by a dark cut insert; not a functional screw.
  self.box(id+'.slot',node,x-r*.6,x+r*.6,y-.0038,y-.0035,z-.0007,z+.0007,'grime',0,'ScrewSlot')
 def cup(self,id,node,x,y,z):
  rx=.064;rz=.029;dep=.028;th=.0025;nt=16;nr=5
  verts=[];faces=[]
  # Front pole plus five arch rings; inner shell is inset and displaced toward wood.
  for inner in [False,True]:
   R=rx-th if inner else rx;Z=rz-th if inner else rz;D=dep-th if inner else dep
   verts.append([x,y-D,z])
   for j in range(1,nr+1):
    phi=j/nr*math.pi/2
    for i in range(nt+1):
     theta=i/nt*math.pi
     verts.append([x+R*math.cos(theta)*math.sin(phi),y-D*math.cos(phi),z+Z*math.sin(theta)*math.sin(phi)])
  ns=1+nr*(nt+1)
  for inner in [0,1]:
   off=inner*ns
   for i in range(nt):
    f=[off,off+1+i,off+2+i]
    if inner:f.reverse()
    faces.append(f)
   for j in range(nr-1):
    for i in range(nt):
     f=[off+1+j*(nt+1)+i,off+1+(j+1)*(nt+1)+i,off+2+(j+1)*(nt+1)+i,off+2+j*(nt+1)+i]
     if inner:f.reverse()
     faces.append(f)
  # Surface winding of this parametrization points inward; reverse once.
  faces=[list(reversed(f)) for f in faces]
  ring=[0]+[1+j*(nt+1) for j in range(nr)]+[1+(nr-1)*(nt+1)+i for i in range(1,nt+1)]+[1+j*(nt+1)+nt for j in range(nr-2,-1,-1)]
  for a,b in zip(ring,ring[1:]+ring[:1]):faces.append([a,b,b+ns,a+ns])
  faces=[f[::-1] for f in faces]
  p=self.mesh(id,node,verts,faces,'iron',.0005,'CupHandle')
  for sx in [-1,1]:
   self.box(id+'.ear.'+str(sx),node,x+sx*.072-.011,x+sx*.072+.011,y-.003,y+.001,z-.006,z+.015,'iron',.003,'HandleMount')
   self.bolt(id+'.bolt.'+str(sx),node,x+sx*.073,y-.003,z+.004,.005)
  s=self.socket(id+'.socket',node,'HandleFront','mount.cup_handle.v1',[x,y,z],{'support_part_ids':[node+'.front'],'footprint':[.18,.055],'max_projection':dep})
  # Exact surface vertex gives a verifiable contact point.
  self.scene['grips'].append(dict(id=id+'.grip',node=node,socket_id=s['id'],position=verts[0],approach=[0,1,0],basis={'x':[1,0,0],'y':[0,0,1],'z':[0,-1,0]},reach_radius=.15,enabled=True))
  self.scene['metadata']['grip_supports'][id+'.grip']=id
 def label(self,id,node,y,z):
  for suffix,lo,hi in [('top',[-.066,y-.003,z+.016],[.066,y,z+.022]),('bottom',[-.066,y-.003,z-.022],[.066,y,z-.016]),('left',[-.072,y-.003,z-.022],[-.066,y,z+.022]),('right',[.066,y-.003,z-.022],[.072,y,z+.022])]:
   self.box(id+'.'+suffix,node,*[v for pair in zip(lo,hi) for v in pair],'iron',.001,'LabelFrame')
  self.box(id+'.paper',node,-.064,.064,y-.0007,y,z-.014,z+.014,'paper',.0003,'LabelInsert')
  for x in [-.069,.069]:self.bolt(id+'.bolt.'+str(x),node,x,y-.003,z,.0028)

 def drawer_box(self,id,cx,front_y,z,width,height,depth,labels=True,open_it=False):
  travel=depth*.67;position=[cx,front_y-travel*self.r['open_fraction'] if open_it else front_y,z]
  self.node(id,position,'Drawer')
  t=.017;front=.03;gap=.008;sideh=height-.032
  self.board(id+'.front',id,[-width/2,-front,0],[width/2,0,height],'DrawerFront',groove=True)
  # Inset sides and bottom form a complete closed-bottom box, open at the top.
  boxw=width-.018;x0=-boxw/2;x1=boxw/2
  self.board(id+'.left',id,[x0,0,.008],[x0+t,depth,sideh],'DrawerLeft',face='side')
  self.board(id+'.right',id,[x1-t,0,.008],[x1,depth,sideh],'DrawerRight',face='side',groove=True)
  self.board(id+'.back',id,[x0+t,depth-t,.008],[x1-t,depth,sideh],'DrawerBack')
  self.board(id+'.bottom',id,[x0+t,0,.008],[x1-t,depth-t,.021],'DrawerBottom',face='top',groove=True)
  # Small end-grain plugs suggest pegged joinery, without intersecting the cavity.
  for side in [-1,1]:
   for j in range(3):self.box(id+'.peg.'+str(side)+'.'+str(j),id,side*(width/2-.011)-.004,side*(width/2-.011)+.004,-front-.0005,-front+.001,height*(.12+j*.30),height*(.12+j*.30)+min(.007,height*.05),'wood_edge',.0003,'JoineryPlug')
  hz=height*.40;self.cup(id+'.cup',id,0,-front,hz)
  if labels:
   self.label(id+'.label',id,-front,height*.75)
   # Key escutcheon beside the cup, with geometric dark keyhole insert.
   kx=width*.35
   self.box(id+'.keyplate',id,kx-.011,kx+.011,-front-.004,-front+.001,hz-.019,hz+.033,'iron',.003,'KeyEscutcheon')
   self.cylinder(id+'.keyhole.round',id,[kx,-front-.0041,hz+.012],[kx,-front-.0045,hz+.012],.0045,'grime',12,0,'Keyhole')
   self.prism(id+'.keyhole.slot',id,[(kx-.004,hz-.008),(kx+.004,hz-.008),(kx+.002,hz+.008),(kx-.002,hz+.008)],-front-.0045,-front-.0041,'grime',0,'Keyhole')
  guide=self.socket(id+'.guide',id,'DrawerGuide','drawer.linear.v1',[0,depth/2,.025],{'support_part_ids':[id+'.left',id+'.right'],'travel_m':travel,'lateral_gap_m':gap,'vertical_gap_m':.008,'vertical_gap_scope':'minimum nominal front-to-bay clearance','runner_gap_m':.003})
  guide.update(axis=[0,-1,0],limits_m=[0,travel],closed_translation=[cx,front_y,z])
  self.socket(id+'.mount','frame','DrawerMount','drawer.bay.v1',[cx,front_y,z],{'support_part_ids':[],'drawer_node':id,'opening_m':[width+.016,height+.016,depth+.025]})
  d=dict(node=id,closed_origin=[cx,front_y,z],travel_m=travel,outer_lo=[-width/2,-front-.04,0],outer_hi=[width/2,depth,height],interior_lo=[x0+t,0,.021],interior_hi=[x1-t,depth-t,sideh],clearance_m=.008)
  self.scene['metadata']['drawers'].append(d)
  return d

def _cabinet(b):
 w,d,h=[b.r[k] for k in ['width','depth','height']];s=.054;front=-d/2;back=d/2;leg=.10;low=.15;top=h-.057;n=b.r['drawer_count']
 for sx in [-1,1]:
  for sy in [-1,1]:
   x0=sx*(w/2-s/2)-s/2;y0=sy*(d/2-s/2)-s/2
   b.fixed_board('post.%s.%s'%(sx,sy),[x0,y0,0],[x0+s,y0+s,h-.045],role='Post',groove=(sx==1))
  # Side panels: vertical joined boards between the two posts.
  length=d-2*s;count=3
  for j in range(count):
   y0=front+s+j*length/count+.001;x0=-w/2+.010 if sx<0 else w/2-.029
   b.fixed_board('side.%s.%s'%(sx,j),[x0,y0,leg],[x0+.019,y0+length/count-.002,h-.05],role='SidePlank',face='side',groove=(j==1))
 for j in range(5):
  x0=-w/2+s+j*(w-2*s)/5+.001
  b.fixed_board('back.'+str(j),[x0,back-.025,leg],[x0+(w-2*s)/5-.002,back-.01,h-.05],role='BackPlank',groove=j==3)
 for z in [leg+.08,h*.54,h-.13]:
  b.fixed_board('backbrace.'+str(z),[-w/2+s,back-.008,z],[w/2-s,back+.018,z+.047],role='BackBrace')
 # Four planks make the projecting cap, retaining genuine seams.
 for j in range(4):
  y0=front-.018+j*(d+.036)/4+.0008
  b.fixed_board('cap.'+str(j),[-w/2-.02,y0,h-.052],[w/2+.02,y0+(d+.036)/4-.0016,h],role='TopPlank',face='top',groove=j in (0,2))
 bay=(top-low)/n;dw=w-2*s-.016;dd=d-.105
 for i in range(n+1):
  z=low+i*bay-.014
  b.fixed_board('rail.front.'+str(i),[-w/2+s,front,z],[w/2-s,front+.034,z+.019],role='FrontRail')
 for i in range(n):
  z=low+i*bay+.013;dh=bay-.039
  # Runners end below shell, with a measured 3 mm gap.
  for sx in [-1,1]:
   x=sx*(dw/2-.015)
   b.fixed_board('runner.%s.%s'%(i,sx),[x-.016,front+.031,z-.012],[x+.016,back-.044,z+.005],role='DrawerRunner')
  b.drawer_box('drawer.'+str(i),0,front+.004,z,dw,dh,dd,True,open_it=(i==max(0,n-2)))

def _bench(b):
 w,d,h=[b.r[k] for k in ['width','depth','height']];s=.086;front=-d/2;back=d/2;top=h-.065;legx=w/2-.095;legy=d/2-.072;shelfz=.22
 for sx in [-1,1]:
  for sy in [-1,1]:
   x=sx*legx;y=sy*legy
   b.fixed_board('leg.%s.%s'%(sx,sy),[x-s/2,y-s/2,0],[x+s/2,y+s/2,top],role='Leg',groove=sx==1)
   # Foot sleeves and external corner brackets contact the timber.
   for zz,hh in [(0,.048),(top-.125,.105)]:
    b.box('strap.%s.%s.%s'%(sx,sy,zz),'frame',x-s/2-.002,x+s/2+.002,y-s/2-.004,y-s/2,zz,zz+hh,'iron',.002,'CornerStrap')
    for z in [zz+.015,zz+hh-.015]:b.bolt('strapbolt.%s.%s.%s'%(sx,sy,z),'frame',x,y-s/2-.004,z,.006)
 for j in range(5):
  y0=front-.018+j*(d+.036)/5+.001
  b.fixed_board('top.'+str(j),[-w/2,y0,top],[w/2,y0+(d+.036)/5-.002,h],role='TopPlank',face='top',groove=j in (0,3))
 # Back apron and side aprons, supported at each end.
 b.fixed_board('apron.back',[-legx,back-.086,top-.15],[legx,back-.05,top],role='Apron')
 for sx in [-1,1]:
  x=sx*legx
  b.fixed_board('apron.side.'+str(sx),[x-.023,-legy,top-.15],[x+.023,legy,top],role='Apron',face='side')
 drawerw=min(.46,w*.31);cx=legx-.071-drawerw/2;dh=.135;z=top-.166;dd=d-.18
 if b.r['drawer']:
  # Split front apron deliberately leaves the right drawer bay unobstructed.
  b.fixed_board('apron.front.left',[-legx,front+.015,top-.151],[cx-drawerw/2-.012,front+.051,top],role='Apron',groove=True)
  b.fixed_board('apron.front.header',[cx-drawerw/2-.008,front+.015,top-.016],[legx,front+.051,top],role='Apron')
  for x in [cx-drawerw/2-.025,cx+drawerw/2+.025]:
   b.fixed_board('drawer.support.'+str(x),[x-.01,front+.022,z-.024],[x+.01,front+dd+.01,top-.019],role='DrawerSupport')
   rx=cx+(-1 if x<cx else 1)*drawerw/2
   b.fixed_board('drawer.runner.'+str(x),[rx-.028,front+.024,z-.012],[rx+.028,front+dd+.01,z+.005],role='DrawerRunner')
  b.drawer_box('drawer.0',cx,front+.012,z,drawerw,dh,dd,False,open_it=True)
 else:b.fixed_board('apron.front',[-legx,front+.015,top-.151],[legx,front+.051,top],role='Apron')
 for sy in [-1,1]:
  y=sy*legy
  b.fixed_board('shelf.rail.'+str(sy),[-legx,y-.023,shelfz-.052],[legx,y+.023,shelfz-.019],role='ShelfSupport')
 for sx in [-1,1]:
  x=sx*(legx-.030)
  b.fixed_board('shelf.cross.'+str(sx),[x-.026,-legy,shelfz-.052],[x+.026,legy,shelfz-.019],role='ShelfSupport')
 if b.r['shelf']:
  count=4;dep=2*legy-.03
  for j in range(count):
   y0=-dep/2+j*dep/count+.001
   b.fixed_board('shelf.'+str(j),[-legx+.043,y0,shelfz-.019],[legx-.043,y0+dep/count-.002,shelfz+.011],role='ShelfPlank',face='top',groove=j==0)
 # Side-plane iron diagonal straps never enter the drawer's central volume.
 for sx in [-1,1]:
  x=sx*(legx+.045)
  a=[x,-legy+.043,shelfz+.05];bb=[x,legy-.043,top-.18]
  length=math.dist(a,bb);direction=[(bb[i]-a[i])/length for i in range(3)];perp=[0,-direction[2]*.013,direction[1]*.013]
  vs=[]
  for dx in [-.0025,.0025]:
   for p,sg in [(a,-1),(bb,-1),(bb,1),(a,1)]:vs.append([p[0]+dx,p[1]+sg*perp[1],p[2]+sg*perp[2]])
  # A thin solid oriented on the side-plane.
  fs=[[3,2,1,0],[4,5,6,7],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7]]
  # Ensure positive winding in either sign direction.
  vol=sum(_dot(vs[f[0]],_cross(vs[f[1]],vs[f[2]]))/6 for f in fs)
  if vol<0:fs=[f[::-1] for f in fs]
  b.mesh('diagonal.'+str(sx),'frame',vs,fs,'iron',.001,'DiagonalBrace')

def build_furniture(recipe):
 r=normalize_recipe(recipe);b=FurnitureBuilder(r)
 (_cabinet if r['preset']=='cabinet' else _bench)(b)
 b.scene['metadata']['engine_ownership']='Engine owns mechanisms, constraints, colliders, mass, damage response and interaction runtime.'
 return b.scene

def sweep_clearance(scene):
 collisions=[];pairs=0;minimum_runner_gap=float('inf')
 def overlap(lo,hi,fixed):return all(min(hi[k],fixed['hi'][k])-max(lo[k],fixed['lo'][k])>1e-7 for k in range(3))
 for dr in scene['metadata']['drawers']:
  o=dr['closed_origin']
  moving=[p for p in scene['parts'] if p['node']==dr['node']]
  for part in moving:
   lo=[o[k]+min(v[k] for v in part['vertices']) for k in range(3)];hi=[o[k]+max(v[k] for v in part['vertices']) for k in range(3)]
   lo[1]-=dr['travel_m']
   for fixed in scene['metadata']['static_envelopes']:
    pairs+=1
    if overlap(lo,hi,fixed):collisions.append(dict(drawer=dr['node'],moving=part['id'],part=fixed['id'],interval_m=[0,dr['travel_m']]))
    if 'runner' in fixed['id'] and part['role'] in ['DrawerLeft','DrawerRight','DrawerBottom']:
     if all(min(hi[k],fixed['hi'][k])-max(lo[k],fixed['lo'][k])>1e-7 for k in [0,1]):
      gap=lo[2]-fixed['hi'][2]
      if gap>=0:minimum_runner_gap=min(minimum_runner_gap,gap)
 return dict(passed=not collisions,collisions=collisions,samples=pairs,method='Continuous full-travel swept AABB of every rigid drawer part against every fixed structural envelope; conservative for curved parts; includes runners',runner_gap_m=None if minimum_runner_gap==float('inf') else minimum_runner_gap)

def validate_furniture(scene):
 errors=validate_scene(scene)
 for p in scene['parts']:
  if p.get('face_materials') and len(p['face_materials'])!=len(p['faces']):errors.append(p['id']+': material assignment count')
 for dr in scene['metadata']['drawers']:
  if any(a>=b for a,b in zip(dr['interior_lo'],dr['interior_hi'])):errors.append(dr['node']+': invalid interior')
 if not sweep_clearance(scene)['passed']:errors.append('Drawer swept clearance collision: '+str(sweep_clearance(scene)['collisions'][:4]))
 return errors
