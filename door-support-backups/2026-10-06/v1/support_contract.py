"""Original, fixture-bounded structural support queries. NO PHYSICS EXECUTION.

Wood = authored XZ polygon extruded along Y, minus body-local box masks.
Connectivity = analytical vertical decomposition into trapezoidal prisms.
Bearing area = disclosed 7x7 midpoint quadrature over each hardware footprint;
penetration = exact Y-interval subtraction, with a required front half-depth.
No mesh baking, material strengths, fracture, inertia, or Jolt calls occur.
"""
from bisect import bisect_right
from copy import deepcopy
import hashlib
import json
import math

PINNED_SCENE_HASH = '3e0d55be93a08009d3d0a502b1444d328fc7f7b73294a4937475c3e2340a090c'
EPS = 1e-10
AREA_SAMPLES = 7
DEPTH_FRACTION = 0.5
MIN_AREA_FRACTION = 0.5


def _canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False)


def _bounds(vertices):
    return [[min(p[i] for p in vertices) for i in range(3)],
            [max(p[i] for p in vertices) for i in range(3)]]


def _profile(part):
    low = min(v[1] for v in part['vertices'])
    return [(v[0], v[2]) for v in part['vertices'] if abs(v[1]-low) < EPS]


def _inside(polygon, x, z):
    inside = False
    for (ax, az), (bx, bz) in zip(polygon, polygon[1:] + polygon[:1]):
        cross = (x-ax)*(bz-az)-(z-az)*(bx-ax)
        if abs(cross) < EPS and min(ax,bx)-EPS <= x <= max(ax,bx)+EPS and min(az,bz)-EPS <= z <= max(az,bz)+EPS:
            return True
        if (az > z) != (bz > z) and x < (bx-ax)*(z-az)/(bz-az)+ax:
            inside = not inside
    return inside


def _at(line, x):
    return line[0]*x + line[1]


def _rotate(v, angle):
    c, s = math.cos(angle), math.sin(angle)
    return [c*v[0]-s*v[1], s*v[0]+c*v[1], v[2]]


def _vector(v, name):
    if not isinstance(v, (list, tuple)) or len(v) != 3 or any(isinstance(x,bool) or not isinstance(x,(int,float)) or not math.isfinite(x) for x in v):
        raise ValueError(name + ' must be a finite numeric three-vector')
    return list(v)


class _Components:
    """Analytical cells for finite simple XZ profiles and local box cutters.

    All polygon vertices, edge/edge crossings, cut faces and edge/cut crossings
    split X. Y is split at extrusion and cut boundaries. Line ordering cannot
    change inside a cell. A positive-area shared face is required for adjacency.
    EPS is the numerical tolerance, not a voxel or general fracture resolution.
    Only wood union is structural here; cosmetic iron is not a load bridge.
    """
    def __init__(self, owner, parts, cuts, seed):
        self.owner, self.cells, self.parent = owner, {}, []
        self.xs, self.ys = [], []
        if not parts:
            self.owner_root = None
            return
        edges, px = {}, set()
        ys = set()
        for part in parts:
            poly = part['_profile']
            es = []
            for (x,z),(xx,zz) in zip(poly, poly[1:]+poly[:1]):
                px.add(x)
                if abs(xx-x)>EPS:
                    a=(zz-z)/(xx-x)
                    es.append((min(x,xx),max(x,xx),(a,z-a*x)))
            edges[part['id']] = es
            ys.update([part['_bounds'][0][1],part['_bounds'][1][1]])
        xmin,xmax = min(px),max(px)
        ymin,ymax = min(ys),max(ys)
        cuts = [c for c in cuts if c['maximum'][0]>xmin and c['minimum'][0]<xmax and c['maximum'][1]>ymin and c['minimum'][1]<ymax]
        all_edges=[e for es in edges.values() for e in es]
        for cut in cuts:
            lo,hi=cut['minimum'],cut['maximum']
            px.update(x for x in (lo[0],hi[0]) if xmin<x<xmax)
            ys.update(y for y in (lo[1],hi[1]) if ymin<y<ymax)
            all_edges.extend((max(xmin,lo[0]),min(xmax,hi[0]),(0,z)) for z in (lo[2],hi[2]))
        for n,(a,b,f) in enumerate(all_edges):
            for c,d,g in all_edges[n+1:]:
                if abs(f[0]-g[0])>EPS:
                    x=(g[1]-f[1])/(f[0]-g[0])
                    if max(a,c)+EPS<x<min(b,d)-EPS:
                        px.add(x)
        self.xs=sorted(px)
        self.ys=sorted(ys)
        # Fail explicitly rather than create an unbounded arbitrary geometry engine.
        if len(self.xs)*len(self.ys)>100000:
            raise ValueError('fixture-bounded decomposition budget exceeded')
        for i,(xl,xr) in enumerate(zip(self.xs,self.xs[1:])):
            if xr-xl<EPS:
                continue
            xm=(xl+xr)/2
            for j,(yl,yr) in enumerate(zip(self.ys,self.ys[1:])):
                ym=(yl+yr)/2
                spans=[]
                for part in parts:
                    if not part['_bounds'][0][1]<ym<part['_bounds'][1][1]:
                        continue
                    lines=[f for a,b,f in edges[part['id']] if a<xm<b]
                    lines.sort(key=lambda f:_at(f,xm))
                    if len(lines)%2:
                        raise ValueError('profile decomposition has odd edge count')
                    spans.extend(zip(lines[::2],lines[1::2]))
                spans.sort(key=lambda span:_at(span[0],xm))
                merged=[]
                for lo,hi in spans:
                    if merged and _at(lo,xm)<=_at(merged[-1][1],xm)+EPS:
                        if _at(hi,xm)>_at(merged[-1][1],xm):
                            merged[-1]=(merged[-1][0],hi)
                    else:
                        merged.append((lo,hi))
                for cut in cuts:
                    a,b=cut['minimum'],cut['maximum']
                    if not a[0]<xm<b[0] or not a[1]<ym<b[1]:
                        continue
                    next_spans=[]
                    for lo,hi in merged:
                        l,h=_at(lo,xm),_at(hi,xm)
                        if b[2]<=l+EPS or a[2]>=h-EPS:
                            next_spans.append((lo,hi))
                        else:
                            if l<a[2]-EPS:
                                next_spans.append((lo,(0,a[2])))
                            if h>b[2]+EPS:
                                next_spans.append(((0,b[2]),hi))
                    merged=next_spans
                cells=[]
                for lo,hi in merged:
                    index=len(self.parent)
                    self.parent.append(index)
                    cells.append((lo,hi,index))
                self.cells[i,j]=cells
                for lo,hi,index in cells:
                    for a,b,k in self.cells.get((i-1,j),[]):
                        if min(_at(hi,xl),_at(b,xl))-max(_at(lo,xl),_at(a,xl))>EPS:
                            self._union(index,k)
                    for a,b,k in self.cells.get((i,j-1),[]):
                        if min(_at(hi,xm),_at(b,xm))-max(_at(lo,xm),_at(a,xm))>EPS:
                            self._union(index,k)
        self.owner_root=self.root_at(seed)

    def _root(self,x):
        while x!=self.parent[x]:
            self.parent[x]=self.parent[self.parent[x]]
            x=self.parent[x]
        return x

    def _union(self,a,b):
        a,b=self._root(a),self._root(b)
        self.parent[max(a,b)]=min(a,b)

    def root_at(self,p):
        x,y,z=p
        i=bisect_right(self.xs,x)-1
        j=bisect_right(self.ys,y)-1
        for lo,hi,k in self.cells.get((i,j),[]):
            if _at(lo,x)-EPS<=z<=_at(hi,x)+EPS:
                return self._root(k)
        return None

    def component_at(self,p):
        root=self.root_at(p)
        if root is None:
            return None
        return self.owner+'.owner' if root==self.owner_root else self.owner+'.fragment.'+str(root)


class DoorSupportContract:
    """A deterministic action-plan compiler for the pinned approved fixture.

    Logical body strings are not Jolt BodyIDs. Do not execute this snapshot as a
    physics adapter: it does not provide collision rebuilds or actual rigid bodies.
    """
    def __init__(self, scene):
        if hashlib.sha256(_canonical(scene).encode()).hexdigest()!=PINNED_SCENE_HASH:
            raise ValueError('Only the pinned approved fixture is supported')
        self.scene=deepcopy(scene)
        self.parts={p['id']:deepcopy(p) for p in scene['parts']}
        for p in self.parts.values():
            p['_bounds']=_bounds(p['vertices'])
            p['_profile']=_profile(p)
        self.nodes={n['id']:n for n in scene['nodes']}
        self.sockets={s['id']:s for s in scene['sockets']}
        self.version=0
        self.cuts=[]
        self.removed=set()
        self.events={}
        self.commands=[]
        self._detached=False
        self.seeds={}
        for body,pid in [('frame','frame.head'),('door.leaf','door.rail.1')]:
            a,b=self.parts[pid]['_bounds']
            self.seeds[body]={'source_part_id':pid,'point':[(x+y)/2 for x,y in zip(a,b)]}
        self.mount_specs=self._mount_specs()
        self._state=self._evaluate()

    def _node_world(self,node):
        n=self.nodes[node]
        if n['parent'] is None:
            return list(n['translation']),n['rotation_z']
        pt,pa=self._node_world(n['parent'])
        r=_rotate(n['translation'],pa)
        return [x+y for x,y in zip(pt,r)],pa+n['rotation_z']

    def _transform(self,node,point,basis):
        t,a=self._node_world(node)
        r=_rotate(point,a)
        return {'translation':[x+y for x,y in zip(t,r)],'basis':{k:_rotate(v,a) for k,v in basis.items()}}

    def _mount_specs(self):
        mounts=[]
        for i in range(3):
            sid=f'door.hinge.{i}.socket'
            s=self.sockets[sid]
            world=self._transform(s['node'],s['translation'],s['basis'])
            ft,fa=self._node_world('frame')
            flocal=_rotate([x-y for x,y in zip(world['translation'],ft)],-fa)
            fbasis={k:_rotate(v,-fa) for k,v in world['basis'].items()}
            pairs=[]
            for body,point,basis,hardware in [
                ('frame',flocal,fbasis,[f'door.hinge.{i}.frame_plate']),
                ('door.leaf',s['translation'],s['basis'],[f'door.hinge.{i}.rivet.{k}' for k in range(3)]+[f'reinforced.band.{i}.bolt.{k}' for k in range(5)])]:
                wood=[p for p in self.parts.values() if p['node']==body and p['material']=='wood_rough' and (body=='frame' and p['id']=='frame.left' or body=='door.leaf' and p['role']=='Plank')]
                samples=[]
                regions=[]
                for hid in hardware:
                    h=self.parts[hid]
                    a,b=h['_bounds']
                    weight=(b[0]-a[0])*(b[2]-a[2])/(AREA_SAMPLES**2)
                    for ix in range(AREA_SAMPLES):
                        for iz in range(AREA_SAMPLES):
                            x=a[0]+(ix+0.5)*(b[0]-a[0])/AREA_SAMPLES
                            z=a[2]+(iz+0.5)*(b[2]-a[2])/AREA_SAMPLES
                            if not _inside(h['_profile'],x,z):
                                continue
                            for p in wood:
                                if _inside(p['_profile'],x,z):
                                    lo,hi=p['_bounds'][0][1],p['_bounds'][1][1]
                                    samples.append({'x':x,'z':z,'y':lo,'depth':(hi-lo)*DEPTH_FRACTION,'area':weight,'part':p['id'],'hardware':hid})
                                    break
                    regions.append({'footprint_source_part_id':hid,'projected_xz_bounds':[[a[0],a[2]],[b[0],b[2]]],'sample_pitch_m':[(b[0]-a[0])/AREA_SAMPLES,(b[2]-a[2])/AREA_SAMPLES]})
                pairs.append({'id':sid+('.frame' if body=='frame' else '.leaf'),
                    'logical_body':body,'authored_source_node':body,
                    'source_socket_id':sid if body=='door.leaf' else None,
                    'pose_derivation':'authored_leaf_socket' if body=='door.leaf' else 'matching_pin_pose_via_node_transform',
                    'local_transform':{'translation':list(point),'basis':deepcopy(basis)},
                    'world_transform':self._transform(body,point,basis),
                    'hardware_part_ids':hardware,
                    'required_hardware_part_id':f'door.hinge.{i}.frame_plate' if body=='frame' else f'door.hinge.{i}.strap',
                    'required_owner_component_id':body+'.owner',
                    'protected_regions':regions,'_samples':samples})
            mounts.append({'id':sid,'endpoints':pairs})
        return mounts

    def material_at(self,part_id,point):
        if part_id not in self.parts:
            raise ValueError('Unknown source part ID')
        x,y,z=_vector(point,'point')
        p=self.parts[part_id]
        if p['material']!='wood_rough':
            raise ValueError('Analytical point occupancy is limited to authored wood extrusions')
        a,b=p['_bounds']
        if part_id in self.removed or not a[1]-EPS<=y<=b[1]+EPS or not _inside(p['_profile'],x,z):
            return False
        return not any(c['body']==p['node'] and all(lo-EPS<=v<=hi+EPS for v,lo,hi in zip(point,c['minimum'],c['maximum'])) for c in self.cuts)

    def _hardware_retained(self,part_id,x,z):
        """Same local box mask, applied to the authored hardware extrusion."""
        if part_id in self.removed:
            return False
        part=self.parts[part_id]
        if not _inside(part['_profile'],x,z):
            return False
        lo,hi=part['_bounds'][0][1],part['_bounds'][1][1]
        intervals=[(lo,hi)]
        for cut in self.cuts:
            a,b=cut['minimum'],cut['maximum']
            if cut['body']!=part['node'] or not a[0]-EPS<=x<=b[0]+EPS or not a[2]-EPS<=z<=b[2]+EPS:
                continue
            remaining=[]
            for l,h in intervals:
                if b[1]<=l+EPS or a[1]>=h-EPS:
                    remaining.append((l,h))
                else:
                    if l<a[1]-EPS:
                        remaining.append((l,a[1]))
                    if h>b[1]+EPS:
                        remaining.append((b[1],h))
            intervals=remaining
        return max((h-l for l,h in intervals),default=0)+EPS>=(hi-lo)*DEPTH_FRACTION

    def _remaining_depth(self,sample,body):
        if sample['part'] in self.removed or sample['hardware'] in self.removed:
            return 0
        start=sample['y']
        end=start+sample['depth']
        # Required contiguous material is measured from the authored near face.
        for c in self.cuts:
            a,b=c['minimum'],c['maximum']
            if c['body']==body and a[0]-EPS<=sample['x']<=b[0]+EPS and a[2]-EPS<=sample['z']<=b[2]+EPS and b[1]>start+EPS and a[1]<end-EPS:
                end=min(end,max(start,a[1]))
        return max(0,end-start)

    def _evaluate(self):
        graphs={}
        for body in self.nodes:
            wood=[p for p in self.parts.values() if p['node']==body and p['material']=='wood_rough' and p['id'] not in self.removed]
            graphs[body]=_Components(body,wood,[c for c in self.cuts if c['body']==body],self.seeds[body]['point'])
        mounts=[]
        for spec in self.mount_specs:
            mount={'id':spec['id'],'endpoints':[]}
            for ep in spec['endpoints']:
                out={k:deepcopy(v) for k,v in ep.items() if not k.startswith('_')}
                total=retained=connected=0.0
                ids=set()
                graph=graphs[ep['logical_body']]
                required_part=ep['required_hardware_part_id']
                for s in ep['_samples']:
                    total+=s['area']
                    if not self._hardware_retained(required_part,s['x'],s['z']) or not self._hardware_retained(s['hardware'],s['x'],s['z']):
                        continue
                    if self._remaining_depth(s,ep['logical_body'])+EPS<s['depth']:
                        continue
                    retained+=s['area']
                    component=graph.component_at([s['x'],s['y']+s['depth']/2,s['z']])
                    if component:
                        ids.add(component)
                    if component==ep['required_owner_component_id']:
                        connected+=s['area']
                fraction=connected/total if total else 0
                out['bearing']={'method':'7x7 midpoint footprint quadrature; exact near-face Y-depth subtraction',
                    'source_wood_part_ids':sorted({s['part'] for s in ep['_samples']}),
                    'original_area_m2':total,'retained_area_m2':retained,'connected_area_m2':connected,
                    'required_depth_m':max((s['depth'] for s in ep['_samples']),default=0),
                    'required_depth_fraction':DEPTH_FRACTION,'minimum_connected_area_fraction':MIN_AREA_FRACTION,
                    'retained_connected_fraction':fraction,'sample_count':len(ep['_samples'])}
                out['support_component_ids']=sorted(ids)
                out['connected_to_owner']=connected>EPS
                out['supported']=fraction+EPS>=MIN_AREA_FRACTION and total>EPS
                mount['endpoints'].append(out)
            mount['supported']=all(e['supported'] for e in mount['endpoints'])
            mounts.append(mount)
        grips=[]
        for grip in self.scene['grips']:
            g=deepcopy(grip)
            contact=self.scene['metadata']['grip_supports'][g['id']]
            backing=self.sockets[g['socket_id']]['coverage']['support_part_ids']
            required=[contact]+backing
            g['source_contact_part_id']=contact
            g['source_socket_support_part_ids']=list(backing)
            g['enabled']=g['enabled'] and all(p not in self.removed for p in required)
            g['query_method']='explicit source-part removal only; grasp-volume CSG not evaluated'
            g['disabled_reason']=None if g['enabled'] else 'authored contact or socket-support part removed'
            grips.append(g)
        return {'mounts':mounts,'grips':grips}

    def snapshot(self):
        alive=sum(m['supported'] for m in self._state['mounts'])
        return {'schema_version':1,'source_scene_hash':PINNED_SCENE_HASH,
            'source_basis':deepcopy(self.scene['basis']),'source_units':self.scene['units'],
            'physics_executed':False,'render_collision_geometry_baked':False,
            'label':'QUERY-ONLY CSG SUPPORT CONTRACT. NO PHYSICS SIMULATION.',
            'logical_bodies':[{'name':n,'authored_source_node':n,'jolt_body_id':None,'owner_seed':deepcopy(self.seeds[n])} for n in self.nodes],
            'support_query':{'material':'authored wood XZ polygon extrusion minus local axis-aligned box volumes',
                'connectivity':'analytical vertical decomposition with positive-area shared faces and explicit owner seeds',
                'geometry_tolerance_m':EPS,'bearing_samples_per_footprint_axis':AREA_SAMPLES,
                'scope':'pinned fixture only; wood continuity; hardware projected bearing intent, not modeled fastener shafts or strength',
                'hardware_policy':'At each bearing sample, a contiguous half-depth of source strap/plate and fastener hardware must remain.',
                'component_identity_scope':'Owner IDs are stable. Fragment IDs are scoped to the current damage version.',
                'limitations':['No Jolt integration, simulation, sag or collision/render rebuild.',
                    'Bearing quadrature can miss sub-sample local loss; protection also rejects any box intersecting a protected projected backing volume or hardware bounds.',
                    'Decorative iron does not bridge disconnected wood. Fixed owner seeds define required retained body components.',
                    'Grip CSG contact-volume damage is not evaluated; explicit source-part removal is supported.']},
            'damage':{'version':self.version,'protection_default':True,'representation':'query-only subtraction mask; source geometry unchanged',
                'cuts':deepcopy(self.cuts),'removed_source_part_ids':sorted(self.removed)},
            'source_part_bounds':{k:deepcopy(p['_bounds']) for k,p in self.parts.items()},
            'mounts':deepcopy(self._state['mounts']),'grips':deepcopy(self._state['grips']),
            'articulation':{'id':'door.articulation.hinge','kind':'single_ideal_hinge','supports_alive':alive,
                'state':'detached_planned' if self._detached else 'ideal_hinge_planned',
                'desired_active':alive>0,'support_mount_ids':[m['id'] for m in self._state['mounts'] if m['supported']],
                'policy':'At least one complete paired mount retains one ideal hinge; zero removes it. No sag model.'},
            'planned_commands':deepcopy(self.commands)}

    def canonical_json(self):
        return _canonical(self.snapshot())

    def _protected_overlap(self,cut):
        a,b=cut['minimum'],cut['maximum']
        for mount in self.mount_specs:
            for ep in mount['endpoints']:
                if ep['logical_body']!=cut['body']:
                    continue
                for pid in set(ep['hardware_part_ids']+[ep['required_hardware_part_id']]):
                    lo,hi=self.parts[pid]['_bounds']
                    if all(min(b[k],hi[k])-max(a[k],lo[k])>EPS for k in range(3)):
                        return True
                # Conservative region-box guard intentionally catches cuts smaller
                # than the quadrature spacing. False-positive rejection is safe.
                for region in ep['protected_regions']:
                    lo,hi=region['projected_xz_bounds']
                    for pid in {s['part'] for s in ep['_samples']}:
                        p=self.parts[pid]
                        pa,pb=p['_bounds']
                        if min(hi[0],pb[0],b[0])-max(lo[0],pa[0],a[0])>EPS and min(hi[1],pb[2],b[2])-max(lo[1],pa[2],a[2])>EPS and min(pa[1]+(pb[1]-pa[1])*DEPTH_FRACTION,b[1])-max(pa[1],a[1])>EPS:
                            return True
        return False

    def _apply(self,event_id,payload,expected_version,override_protection):
        if not isinstance(event_id,str) or not event_id.strip():
            raise ValueError('event_id must be nonempty')
        if type(expected_version) is not int or expected_version<0:
            raise ValueError('expected_version must be a nonnegative integer')
        if type(override_protection) is not bool:
            raise ValueError('override_protection must be boolean')
        signature=_canonical({'payload':payload,'override_protection':override_protection})
        if event_id in self.events:
            if self.events[event_id]['signature']!=signature:
                raise ValueError('event ID reused with different damage')
            return {'event_id':event_id,'status':'duplicate','version':self.version,'commands':[],'original_status':self.events[event_id]['status']}
        if expected_version!=self.version:
            raise ValueError('stale damage version')
        previous_cuts=deepcopy(self.cuts)
        previous_removed=set(self.removed)
        if payload['kind']=='box':
            self.cuts.append({'event_id':event_id,**payload})
        else:
            self.removed.add(payload['part_id'])
        try:
            candidate=self._evaluate()
        except Exception:
            self.cuts,self.removed=previous_cuts,previous_removed
            raise
        blocked=False
        if not override_protection:
            blocked=payload['kind']=='box' and self._protected_overlap(payload)
            for old,new in zip(self._state['mounts'],candidate['mounts']):
                for a,b in zip(old['endpoints'],new['endpoints']):
                    blocked |= b['bearing']['connected_area_m2']+EPS<a['bearing']['connected_area_m2']
            blocked |= any(a['enabled'] and not b['enabled'] for a,b in zip(self._state['grips'],candidate['grips']))
        commands=[]
        if blocked:
            self.cuts,self.removed=previous_cuts,previous_removed
            status='rejected_protected_support'
        else:
            self.version+=1
            self._state=candidate
            status='applied'
            if not self._detached and not any(m['supported'] for m in candidate['mounts']):
                self._detached=True
                commands=[{'id':'door.articulation.hinge.remove.v1','command':'remove_constraint','constraint_id':'door.articulation.hinge','adapter_method':'PhysicsSystem.RemoveConstraint','executed':False},
                    {'id':'door.leaf.activate.v1','command':'activate_body','logical_body':'door.leaf','adapter_method':'BodyInterface.ActivateBody','executed':False}]
                self.commands.extend(commands)
        self.events[event_id]={'signature':signature,'status':status}
        return {'event_id':event_id,'status':status,'version':self.version,'commands':deepcopy(commands)}

    def apply_box_cut(self,event_id,body,minimum,maximum,*,expected_version,override_protection=False):
        if body not in self.nodes:
            raise ValueError('Unknown logical body')
        lo,hi=_vector(minimum,'minimum'),_vector(maximum,'maximum')
        if any(b-a<=EPS for a,b in zip(lo,hi)):
            raise ValueError('Box must have positive dimensions larger than tolerance')
        return self._apply(event_id,{'kind':'box','body':body,'minimum':lo,'maximum':hi},expected_version,override_protection)

    def remove_part(self,event_id,part_id,*,expected_version,override_protection=False):
        if part_id not in self.parts:
            raise ValueError('Unknown authored source part ID')
        return self._apply(event_id,{'kind':'part_removed','part_id':part_id},expected_version,override_protection)


def compile_contract(scene):
    return DoorSupportContract(scene).snapshot()
