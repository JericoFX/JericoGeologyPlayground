#!/usr/bin/env python3
"""Isolated scene assembly. Reuses frozen R3 silk and mathematical rock CLI OBJs.
Rock module boundaries are blended as geometry, not displaced from runtime images.
This is an artistic Blender scene, not an engine integration or physical simulation.
"""
import argparse, hashlib, importlib.util, json, math, random, sys, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT
SEEDS=(48,117,305,791)
CENTERS=(-4.0,.5,5.0,9.5,14.0)

def module_weights(y):
    weights=[]
    for i,c in enumerate(CENTERS):
        d=abs(y-c)
        if d<2.7:
            t=max(0.,min(1.,(2.7-d)/.9));w=t*t*(3-2*t)
            if w: weights.append((i,w))
    if not weights: return [(0 if y<CENTERS[0] else len(CENTERS)-1,1.)]
    total=sum(v for _,v in weights)
    return [(i,v/total) for i,v in weights]

def closed_grid_mesh(nx,ny,front,back):
    vertices=[];faces=[]
    for fn in (front,back):
        vertices.extend(fn(i/nx,j/ny) for j in range(ny+1) for i in range(nx+1))
    count=(nx+1)*(ny+1)
    for j in range(ny):
        for i in range(nx):
            a=j*(nx+1)+i;b=a+1;c=a+nx+2;d=a+nx+1
            faces.extend(((a,b,c),(a,c,d),(count+a,count+c,count+b),(count+a,count+d,count+c)))
    ring=list(range(nx+1))+[j*(nx+1)+nx for j in range(1,ny+1)]+[ny*(nx+1)+i for i in range(nx-1,-1,-1)]+[j*(nx+1) for j in range(ny-1,0,-1)]
    for a,b in zip(ring,ring[1:]+ring[:1]): faces.extend(((a,count+a,count+b),(a,count+b,b)))
    return vertices,faces

def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def source_grids(profile="mass"):
    grids=[]
    for seed in SEEDS:
        p=ROOT/f'sources/{profile}-{seed}/heightfield.obj'; v=[]
        for line in p.read_text().splitlines():
            if line.startswith('v '):v.append(tuple(map(float,line.split()[1:4])))
        grids.append([x[2] for x in v[:97*97]])
    return grids

def sample(g,u,v):
    x=max(0.,min(96.,(u+.5)*96));y=max(0.,min(96.,(v+.5)*96));i=min(95,int(x));j=min(95,int(y));a=x-i;b=y-j
    return (1-a)*(1-b)*g[j*97+i]+a*(1-b)*g[j*97+i+1]+a*b*g[(j+1)*97+i+1]+(1-a)*b*g[(j+1)*97+i]

def rock_depth(grids,y,z,offset=0):
    return sum(w*sample(grids[(i+offset)%4],(y-CENTERS[i])/5.4,z/4.9-.5) for i,w in module_weights(y))

def mesh_object(name,verts,faces,material):
    import bpy,bmesh
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update()
    bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(mesh);bm.free()
    obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj);mesh.materials.append(material)
    obj['scene_study_role']='mathematical_rock'
    for p in mesh.polygons:p.use_smooth=True
    return obj

def mineral_material(name='Cool fractured slate'):
    import bpy
    m=bpy.data.materials.new(name);m.use_nodes=True;t=m.node_tree;n=t.nodes.new;l=t.links.new;b=t.nodes.get('Principled BSDF')
    b.inputs['Roughness'].default_value=.89
    tex=n('ShaderNodeTexCoord');noise=n('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=3.8;noise.inputs['Detail'].default_value=4.;noise.inputs['Roughness'].default_value=.73;l(tex.outputs['Object'],noise.inputs['Vector'])
    ramp=n('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.16;ramp.color_ramp.elements[0].color=(.035,.043,.049,1);ramp.color_ramp.elements[1].position=.84;ramp.color_ramp.elements[1].color=(.135,.146,.15,1);l(noise.outputs['Fac'],ramp.inputs['Fac']);l(ramp.outputs['Color'],b.inputs['Base Color'])
    micro=n('ShaderNodeTexNoise');micro.inputs['Scale'].default_value=58.;micro.inputs['Detail'].default_value=3.;l(tex.outputs['Object'],micro.inputs['Vector']);bump=n('ShaderNodeBump');bump.inputs['Strength'].default_value=.4;bump.inputs['Distance'].default_value=.045;l(micro.outputs['Fac'],bump.inputs['Height']);l(bump.outputs['Normal'],b.inputs['Normal'])
    return m

def timber_material():
    import bpy
    m=bpy.data.materials.new('Weathered timber procedural grain');m.use_nodes=True;t=m.node_tree;n=t.nodes.new;l=t.links.new;b=t.nodes.get('Principled BSDF');b.inputs['Roughness'].default_value=.85
    tex=n('ShaderNodeTexCoord');scale=n('ShaderNodeVectorMath');scale.operation='MULTIPLY';scale.inputs[1].default_value=(13,16,1);l(tex.outputs['Generated'],scale.inputs[0]);noise=n('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=4.;noise.inputs['Detail'].default_value=3;l(scale.outputs['Vector'],noise.inputs['Vector'])
    ramp=n('ShaderNodeValToRGB');ramp.color_ramp.elements[0].color=(.021,.011,.006,1);ramp.color_ramp.elements[1].color=(.075,.047,.026,1);l(noise.outputs['Fac'],ramp.inputs['Fac']);l(ramp.outputs['Color'],b.inputs['Base Color']);bump=n('ShaderNodeBump');bump.inputs['Strength'].default_value=.36;bump.inputs['Distance'].default_value=.017;l(noise.outputs['Fac'],bump.inputs['Height']);l(bump.outputs['Normal'],b.inputs['Normal']);return m

def add_lantern(lab):
    import bpy
    metal=lab.plain_material('Lantern black iron',(.035,.028,.018),.45,.75)
    glow=lab.plain_material('Lantern golden glowing core',(1.,.43,.10),.3,emission=7.)
    x,y,z=-1.86,-.21,2.61
    for zz,r,dep in ((z-.20,.13,.05),(z+.20,.16,.06)):
        bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=r,depth=dep,location=(x,y,zz));bpy.context.object.data.materials.append(metal)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=8,radius=.10,location=(x,y,z));obj=bpy.context.object;obj.scale=(.68,.68,1.65);obj.data.materials.append(glow)
    for a in range(0,360,90):
        dx=.1*math.cos(math.radians(a));dy=.1*math.sin(math.radians(a));lab.segment('Lantern iron cage',(x+dx,y+dy,z-.19),(x+dx,y+dy,z+.18),.014,metal)
    lab.segment('Lantern bracket',(-2.07,0,z+.36),(x,y,z+.36),.025,metal);lab.segment('Lantern hanger',(x,y,z+.21),(x,y,z+.36),.018,metal)
    ld=bpy.data.lights.new('Warm lantern emission','POINT');ld.energy=75;ld.color=(1.,.54,.22);ld.shadow_soft_size=.15;ob=bpy.data.objects.new('Warm lantern emission',ld);bpy.context.collection.objects.link(ob);ob.location=(x+.15,y-.15,z)

def validate_scene(assets,rocks):
    import bpy
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    trees=[];invalid=degenerate=badnormals=nonmanifold=0;triangles=vertices=0
    import bmesh
    for ob in rocks:
        mesh=ob.data;vertices+=len(mesh.vertices);triangles+=len(mesh.polygons)
        invalid+=sum(not all(math.isfinite(c) for c in v.co) for v in mesh.vertices)
        degenerate+=sum(p.area<1e-12 for p in mesh.polygons)
        badnormals+=sum(not all(math.isfinite(c) for c in p.normal) or p.normal.length<.9 for p in mesh.polygons)
        bm=bmesh.new();bm.from_mesh(mesh);nonmanifold+=sum(not e.is_manifold for e in bm.edges);bm.free()
        trees.append(BVHTree.FromPolygons([v.co for v in mesh.vertices],[p.vertices[:] for p in mesh.polygons]))
    min_anchor=min(t.find_nearest(Vector(a['position']))[3] for a in assets['metadata']['anchors'] for t in trees)
    min_web=min(t.find_nearest(Vector(v))[3] for v in assets['vertices'] for t in trees)
    clearance_violation=sum(abs(v.co.x)<.74 and -.03<v.co.z<2.45 and 0<=v.co.y<=9 for ob in rocks for v in ob.data.vertices)
    bounds={ob.name:[[min(v.co[i] for v in ob.data.vertices) for i in range(3)],[max(v.co[i] for v in ob.data.vertices) for i in range(3)]] for ob in rocks}
    assert not(invalid or degenerate or badnormals or nonmanifold or clearance_violation),(invalid,degenerate,badnormals,nonmanifold,clearance_violation)
    assert min_web>.06 and min_anchor>.06,(min_web,min_anchor)
    # Every rock side is separated by an entire vertical half-space from silk.
    # Roof by horizontal half-space: convex faces cannot cross either gap.
    leftmax=bounds['ROCK left continuous modular wall'][1][0];rightmin=bounds['ROCK right continuous modular wall'][0][0];roofmin=bounds['ROCK continuous modular ceiling'][0][2]
    assert leftmax<min(v[0] for v in assets['vertices']) and rightmin>max(v[0] for v in assets['vertices']) and roofmin>max(v[2] for v in assets['vertices'])
    return dict(rock_vertices=vertices,rock_triangles=triangles,nonfinite_vertices=invalid,degenerate_faces=degenerate,invalid_normals=badnormals,nonmanifold_edges=nonmanifold,clearance_intrusions=clearance_violation,minimum_anchor_to_rock_m=min_anchor,minimum_silk_vertex_to_rock_m=min_web,separating_halfspace_no_silk_rock_intersection=True,rock_bounds=bounds)

def main():
    import bpy
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);parser.add_argument('--view',choices=('front','oblique'),default='front');parser.add_argument('--width',type=int,default=1200);parser.add_argument('--samples',type=int,default=32);parser.add_argument('--save-blend',action='store_true');parser.add_argument('--validate-only',action='store_true');a=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    if a.output.exists() or a.output.with_suffix('.json').exists():raise FileExistsError('Immutable output exists')
    spec=importlib.util.spec_from_file_location('base_render',BASE/'scripts/render_lab.py');lab=importlib.util.module_from_spec(spec);spec.loader.exec_module(lab)
    args=lab.parse_args(['--input',str(ROOT/'assets/silk'),'--output',str(a.output),'--width',str(a.width),'--height',str(round(a.width*2/3)),'--samples',str(min(a.samples,32))])
    result=lab.build_scene(args);scene=bpy.context.scene;scene.cycles.samples=a.samples;result['report']['samples']=a.samples;start=time.monotonic()
    for ob in list(scene.objects):
        if ob.type=='LIGHT' or ob.name in ('Corridor far darkness','Corridor floor'):bpy.data.objects.remove(ob,do_unlink=True)
    rock=mineral_material();timber=timber_material()
    for ob in scene.objects:
        if ob.type=='MESH' and ('Timber' in ob.name or 'sleeper' in ob.name or 'stand-off' in ob.name):ob.data.materials.clear();ob.data.materials.append(timber)
    grids=source_grids();beds=source_grids("layered");rocks=[]
    for side,offset,label in ((-1,0,'left'),(1,2,'right')):
        def front(u,v,side=side,offset=offset):
            y=-5.8+u*22.6;z=-.32+v*5.15;d=.34*rock_depth(grids,y,z+.32,offset)+.66*rock_depth(beds,y,z+.32,offset)
            return (side*(2.99-.96*d+.065*math.sin(y*.52+offset)+.028*math.sin(y*.91+z*2)),y,z)
        def back(u,v,side=side):return(side*3.7,-5.8+u*22.6,-.32+v*5.15)
        verts,faces=closed_grid_mesh(384,96,front,back);rocks.append(mesh_object('ROCK '+label+' continuous modular wall',verts,faces,rock))
    def roof(u,v):
        x=-3.5+v*7.;y=-5.8+u*22.6;d=rock_depth(grids,y,(x+3.5)*.7,1)
        return (x,y,4.49-.42*d+.065*math.sin(y*.51))
    verts,faces=closed_grid_mesh(384,96,roof,lambda u,v:(-3.5+v*7,-5.8+u*22.6,5.4));rocks.append(mesh_object('ROCK continuous modular ceiling',verts,faces,rock))
    ground=mineral_material('Rock dust and gravel ground');lab.box('Extended mine floor',(0,5.3,-.17),(7.4,23,.3),ground)
    for y in (12,15):
        for x in (-2.08,2.08):lab.box('Distant timber post',(x,y,1.95),(.19,.27,3.9),timber,bevel=.012)
        lab.box('Distant timber lintel',(0,y,3.83),(4.35,.30,.21),timber,bevel=.012)
    rail=bpy.data.materials.get('Dark rail metal')
    for x in (-.61,.61):lab.box('Distant guide rail',(x,13.55,.04),(.046,6.7,.055),rail,bevel=.007)
    for y in [10.4+i*.6 for i in range(11)]:lab.box('Distant rail sleeper',(0,y,-.005),(1.6,.14,.055),timber)
    # Small mathematical low-poly chips at the edges. None crosses either rail.
    rng=random.Random(31415)
    for i in range(170):
        x=rng.choice((-1,1))*rng.uniform(.93,2.4);y=rng.uniform(-2.,15.7);r=rng.uniform(.025,.17)
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(x,y,r*.15-.013));ob=bpy.context.object;ob.name='Loose angular floor chip';ob.scale=(r,r*rng.uniform(.55,1.55),r*rng.uniform(.25,.6));ob.rotation_euler=(rng.uniform(-.4,.4),rng.uniform(-.4,.4),rng.uniform(0,6.28));ob.data.materials.append(rock)
    add_lantern(lab)
    bg=scene.world.node_tree.nodes['Background'];bg.inputs['Color'].default_value=(.04,.065,.09,1);bg.inputs['Strength'].default_value=.11
    lab.add_area('Warm front bounced lantern light',(-.8,-2.0,2.9),(0,3.7,1.8),210,2.3,(1.,.71,.46))
    lab.add_area('Soft neutral camera fill',(.7,-3.0,2.2),(0,3.5,2.2),85,3.,(.67,.78,1.))
    lab.add_area('Blue distant tunnel light',(0,11.2,3.2),(0,4.,2.1),230,2.9,(.29,.54,1.))
    lab.add_area('Blue far wall glow',(0,15.8,2.8),(0,10.,1.8),100,2.2,(.24,.46,1.))
    cam=scene.camera
    target=(0,6.,1.92)
    if a.view=='front':cam.location=(0,-4.5,1.98);cam.data.lens=25.0
    else:cam.location=(1.25,-4.6,2.15);target=(-.20,5.0,1.85);cam.data.lens=24.5
    lab.point_at(cam,target);cam.data.clip_start=.05
    scene.view_settings.look='AgX - Medium High Contrast';scene.view_settings.exposure=-.1;scene.cycles.transparent_max_bounces=64;scene.cycles.max_bounces=7
    report=result['report'];report.update(study='mine-rocks-and-frozen-R3-cobwebs-v1',view=a.view,camera_position=list(cam.location),camera_target=list(target),camera_lens_mm=cam.data.lens,exposure=scene.view_settings.exposure,renderer_sha256=digest(__file__),source_silk_hashes={n:digest(ROOT/'assets/silk'/n) for n in ('mesh.obj','coverage.tga','orientation.tga','metadata.json')},source_rocks=[{'seed':s,'obj_sha256':digest(ROOT/f'sources/mass-{s}/heightfield.obj'),'recipe_sha256':digest(ROOT/f'sources/mass-{s}/recipe.json'),'generator':'heightfield-cliff-v11','profile':'massive_oblique'} for s in SEEDS],rock_assembly='Five overlapping seeded modules per surface; geometry-only partition-of-unity blend; closed backed sidewalls and roof; no input image pixels or runtime rock heightmap',layered_rock_sources=[{'seed':s,'obj_sha256':digest(ROOT/f'sources/layered-{s}/heightfield.obj'),'recipe_sha256':digest(ROOT/f'sources/layered-{s}/recipe.json'),'generator':'heightfield-cliff-v10','bedCount':9,'wall_mixture_weight':.66} for s in SEEDS],engine_integration=False,physical_simulation=False,**validate_scene(result['assets'],rocks))
    a.output.parent.mkdir(parents=True,exist_ok=True)
    if a.save_blend:bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(a.output.with_suffix('.blend').resolve()))
    if not a.validate_only:bpy.ops.render.render(write_still=True);report.update(png_sha256=digest(a.output),png_bytes=a.output.stat().st_size)
    report['rendered']=not a.validate_only;report['elapsed_seconds']=round(time.monotonic()-start,3);a.output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n');print('MINE_SCENE_VERIFIED '+str(a.output))
if __name__=='__main__':main()
