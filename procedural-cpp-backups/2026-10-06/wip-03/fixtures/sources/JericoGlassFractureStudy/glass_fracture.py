"""Independent planar annealed-glass fracture authoring; no addon code.

A kinked fan partitions the rectangle. Irregular central cells are removed;
physical hairline separations and partial circumferential branch cuts split the
retained fan. Every remaining solid bears directly behind perimeter glazing
beads. This is controlled visual geometry, not fracture physics.
"""
import math, random, copy
from core import Builder, _triangulate_profile
from industrial import build_industrial
from damage_math import _area, _contains, _profile, _cross2

def lerp(a,b,t):return [a[k]+(b[k]-a[k])*t for k in range(2)]
def area(p):return abs(_area(p))

def _clip_rect(poly,bounds):
 x0,x1,z0,z1=bounds
 result=poly
 for a,b in [([x0,z0],[x1,z0]),([x1,z0],[x1,z1]),([x1,z1],[x0,z1]),([x0,z1],[x0,z0])]:
  if not result:return []
  def sign(p):return (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
  clipped=[]
  for p,q in zip(result,result[1:]+result[:1]):
   sp,sq=sign(p),sign(q)
   if sp>=0:clipped.append(p)
   if (sp>=0)!=(sq>=0):clipped.append(lerp(p,q,sp/(sp-sq)))
  result=clipped
 return result

def _support(poly,w,h):
 strips=[[-w/2-.011,-w/2,-.011,h+.011],[w/2,w/2+.011,-.011,h+.011],[-w/2,w/2,-.011,0],[-w/2,w/2,h,h+.011]]
 return sum(area(p) if len(p)>=3 else 0 for p in [_clip_rect(poly,r) for r in strips])

def _ray_rect(cx,cz,a,bounds):
 x0,x1,z0,z1=bounds;dx,dz=math.cos(a),math.sin(a)
 length=min(((x1 if dx>0 else x0)-cx)/dx if abs(dx)>1e-12 else math.inf,((z1 if dz>0 else z0)-cz)/dz if abs(dz)>1e-12 else math.inf)
 return [cx+length*dx,cz+length*dz],length

def _controls(recipe):
 r=copy.deepcopy(recipe);energy=r.pop('energy',.66);count=r.pop('ray_count',18);impact=r.pop('impact',[.15,.56])
 if isinstance(energy,bool) or not isinstance(energy,(int,float)) or not math.isfinite(energy) or not .1<=energy<=1:raise ValueError('energy must be finite in 0.1..1')
 if isinstance(count,bool) or not isinstance(count,int) or not 12<=count<=40:raise ValueError('ray_count must be integer 12..40')
 if not isinstance(impact,(tuple,list)) or len(impact)!=2 or any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) for v in impact) or not -.3<=impact[0]<=.3 or not .25<=impact[1]<=.75:raise ValueError('impact is [normalized X -0.3..0.3, Z 0.25..0.75]')
 return r,float(energy),count,list(impact)

class UnsafeFractureGeometry(ValueError):pass

def _build_fracture(recipe,straight_rays=False):
 base,energy,count,impact=_controls(recipe);scene=build_industrial(base);normalized=scene['recipe']
 if scene['metadata']['preset']!='industrial_window':raise ValueError('Fracture study requires industrial_window')
 scene['recipe']=dict(scene['recipe'],base_recipe=copy.deepcopy(base),energy=energy,ray_count=count,impact=impact)
 scene['metadata']['generator']='JericoGlassFractureStudy/glass_fracture.py'
 for p in scene['parts']:
  if p['role']=='GlassPane':p.update(glass_study=True,glass_bounds=[-normalized['width']/2-.011,normalized['width']/2+.011,-.011,normalized['height']+.011])
 if normalized.get('glazing','intact')!='broken':return scene
 w,h=normalized['width'],normalized['height'];bounds=[-w/2-.011,w/2+.011,-.011,h+.011];x0,x1,z0,z1=bounds
 cx,cz=impact[0]*w,impact[1]*h;rng=random.Random(normalized['seed']+53813)
 # Unequal angular intervals, with mandatory rectangle corners for exact cover.
 weights=[rng.uniform(.40,1.65) for i in range(count)];s=sum(weights);a=rng.uniform(0,.25);angles=[]
 for weight in weights:angles.append(a%math.tau);a+=weight/s*math.tau
 for x,z in [(x0,z0),(x1,z0),(x1,z1),(x0,z1)]:
  corner=math.atan2(z-cz,x-cx)%math.tau
  angles=[a for a in angles if abs((a-corner+math.pi)%math.tau-math.pi)>.085];angles.append(corner)
 angles.sort();n=len(angles);rays=[]
 for i,a in enumerate(angles):
  outer,dist=_ray_rect(cx,cz,a,bounds)
  prev=(a-angles[i-1])%math.tau;nxt=(angles[(i+1)%n]-a)%math.tau;space=min(prev,nxt)
  # A broad asymmetrical missing region, without alternating star radii.
  rx,rz=w*(.26+.14*energy),h*(.26+.14*energy)
  impact_radius=1/math.sqrt((math.cos(a)/rx)**2+(math.sin(a)/rz)**2)
  irregular=1+.18*math.sin(2*a+.7)+.12*math.cos(3*a+1.4)+rng.uniform(-.19,.19)
  hole=min(.84,impact_radius*irregular/dist)
  levels=[hole,hole+(1-hole)*.24,hole+(1-hole)*.53,hole+(1-hole)*.77,1]
  ray=[]
  for j,t in enumerate(levels):
   perturb=rng.uniform(-.16,.16)*space if j<4 else 0
   aa=a+(0 if straight_rays else perturb)
   ray.append([cx+math.cos(aa)*dist*t,cz+math.sin(aa)*dist*t] if j<4 else outer)
  rays.append(ray)
 # Source cells tile rectangle minus central hole before separation and chips.
 holepoly=[r[0] for r in rays];source_area=area(holepoly);parts=[];gap_samples=[];solid_samples=[];branches=0
 b=Builder(scene['recipe']);nominal_gap=.0012
 for i in range(n):
  left,right=rays[i],rays[(i+1)%n]
  source=left+list(reversed(right));source_area+=area(source)
  margins=[min(.08,nominal_gap*.5/math.dist(a,c)) for a,c in zip(left,right)]
  L=[lerp(a,c,e) for a,c,e in zip(left,right,margins)];R=[lerp(a,c,1-e) for a,c,e in zip(left,right,margins)]
  gap_samples.append(lerp(left[2],left[3],.5))
  outline=[L[0]];branch_tips=[]
  # Partial concentric branch: tapered slit, not painted line or floating ring.
  k=1 if rng.random()<.60 else 2
  for j in range(1,5):
   if j==k:
    branch_tips.append((len(outline)+1,L[j]))
    outline.extend([L[j],lerp(L[j],R[j],rng.uniform(.38,.79)),lerp(L[j],L[j+1],rng.uniform(.010,.019))]);branches+=1
   else:outline.append(L[j])
  outline.append(R[4])
  for j in range(3,-1,-1):
   if j==3 and rng.random()<.66:
    branch_tips.append((len(outline)+1,R[j]))
    outline.extend([R[j],lerp(R[j],L[j],rng.uniform(.28,.67)),lerp(R[j],R[j-1],rng.uniform(.011,.024))]);branches+=1
   else:outline.append(R[j])
  # Chipped hole lip, with small non-periodic bites distinct from radial cracks.
  p,q=R[0],L[0];mid=lerp(p,q,.54);away=[mid[0]-cx,mid[1]-cz];length=math.hypot(*away)
  chip=min(rng.uniform(.002,.014)*(1 if i%5 else 2.4),.22*min(math.dist(L[0],L[1]),math.dist(R[0],R[1])))
  outline.extend([lerp(p,q,.38),[mid[0]+away[0]/length*chip,mid[1]+away[1]/length*chip],lerp(p,q,.70)])
  # A long partial slit can cross a bent adjacent boundary. Validate the
  # actual outline and deterministically shorten only offending configurations.
  original=copy.deepcopy(outline)
  for safety_scale in (1,.60,.30,.10):
   candidate=copy.deepcopy(original)
   for tip,anchor in branch_tips:candidate[tip]=lerp(anchor,original[tip],safety_scale)
   try:_profile(candidate)
   except ValueError:continue
   outline=candidate;break
  else:
   omitted={index for tip,_ in branch_tips for index in (tip,tip+1)}
   outline=[p for k,p in enumerate(original) if k not in omitted]
   branches-=len(branch_tips)
   try:_profile(outline)
   except ValueError:
    # If even the tiny lip chip does not fit, preserve the unchipped fragment.
    outline=L+list(reversed(R));_profile(outline)
  if _area(outline)<0:outline.reverse()
  part=b.prism('window.glass.shard.%02d'%i,'frame',outline,-.002,.002,'glass',0,'GlassPane')
  capcount=(len(outline)-2)*2
  part['face_materials']=['glass']*capcount+['glass_edge']*len(outline)
  part.update(glass_study=True,glass_bounds=bounds)
  part['footprint_xz']=outline;part['footprint_area_m2']=area(outline)
  part['support_overlap_area_m2']=_support(outline,w,h)
  part['perimeter_bearing_length_m']=math.dist(L[-1],R[-1])
  part['fracture_provenance']='Original kinked fan with geometric partial branch slits and chipped lip'
  # An interior sample from a genuine cap triangle, avoiding notches.
  tri=part['faces'][0];solid_samples.append([sum(part['vertices'][v][axis] for v in tri)/3 for axis in (0,2)])
  parts.append(part)
 # Reject rare unsupported combinations rather than emitting overlapping
 # shards. The calibrated examples are unchanged; this is a fail-closed guard,
 # not a claim that all parameter combinations have a successful construction.
 for i,p in enumerate(parts):
  first=p['footprint_xz']
  for q in parts[i+1:]:
   second=q['footprint_xz'];crossing=False
   for a,b in zip(first,first[1:]+first[:1]):
    for c,d in zip(second,second[1:]+second[:1]):
     if _cross2(a,b,c)*_cross2(a,b,d)<-1e-20 and _cross2(c,d,a)*_cross2(c,d,b)<-1e-20:crossing=True;break
    if crossing:break
   if crossing or _contains(first,second[0],False) or _contains(second,first[0],False):
    raise UnsafeFractureGeometry('Unsupported fracture configuration: fragment overlap')
 scene['parts']=[p for p in scene['parts'] if p['role']!='GlassPane']+parts
 pane=(x1-x0)*(z1-z0);retained=sum(p['footprint_area_m2'] for p in parts)
 scene['metadata']['retained_glass_fraction']=retained/pane
 scene['metadata']['glass_hole']={'center_xz':[cx,cz],'polygon_xz':holepoly,'area_m2':area(holepoly),'construction':'Physical through-hole plus open radial and partial branch slits; all retained shards bear directly in rebate'}
 scene['metadata']['glass_fracture']={'impact_xz':[cx,cz],'energy_proxy':energy,'ray_count':n,'branch_crack_count':branches,'pane_area_m2':pane,'source_partition_area_m2':source_area,'retained_area_m2':retained,'removed_area_m2':pane-retained,'gap_samples_xz':gap_samples,'solid_samples_xz':solid_samples,'nominal_open_fracture_gap_m':nominal_gap,'sharp_unbeveled_glass_edges':True,'minimum_thickness_m':.004,'support_policy':'Every closed fragment intersects the actual 11 mm glazing bead footprint; no loose suspended islands','simulation':False}
 if straight_rays:scene['metadata']['glass_fracture']['safety_fallback']='Straight radial boundaries preserve non-overlap for this off-center or narrow configuration; irregular hole and partial branch cracks remain'
 return scene

def build_fracture(recipe):
 try:return _build_fracture(recipe)
 except UnsafeFractureGeometry:return _build_fracture(recipe,straight_rays=True)

def point_in_glass(scene,point_xz):
 return any(_contains(p['footprint_xz'],point_xz) for p in scene['parts'] if p['role']=='GlassPane' and 'footprint_xz' in p)
