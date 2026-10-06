"""Blender 4.3 authoring adapter. The core contract remains Blender independent."""
import bpy
import math
from mathutils import Vector


def clear_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)


def _node(tree,kind,**attrs):
    n=tree.nodes.new(kind)
    for k,v in attrs.items(): setattr(n,k,v)
    return n


def _material(name,color,roughness=.8,metallic=0):
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes=True
    t=m.node_tree
    t.nodes.clear()
    out=_node(t,'ShaderNodeOutputMaterial')
    bs=_node(t,'ShaderNodeBsdfPrincipled'); bs.name='Principled BSDF'
    bs.inputs['Base Color'].default_value=(*color,1)
    bs.inputs['Roughness'].default_value=roughness
    bs.inputs['Metallic'].default_value=metallic
    t.links.new(bs.outputs['BSDF'],out.inputs['Surface'])
    m.diffuse_color=(*color,1)
    m.roughness=roughness; m.metallic=metallic
    return m,t,bs


def _noise(t,scale,detail=3,roughness=.65,axis_scale=(1,1,1)):
    tex=_node(t,'ShaderNodeTexCoord')
    mapping=_node(t,'ShaderNodeVectorMath',operation='MULTIPLY')
    mapping.inputs[1].default_value=axis_scale
    t.links.new(tex.outputs['Object'],mapping.inputs[0])
    n=_node(t,'ShaderNodeTexNoise')
    n.inputs['Scale'].default_value=scale; n.inputs['Detail'].default_value=detail
    n.inputs['Roughness'].default_value=roughness
    t.links.new(mapping.outputs['Vector'],n.inputs['Vector'])
    return n


def _ramp(t,source,stops):
    r=_node(t,'ShaderNodeValToRGB')
    for e in list(r.color_ramp.elements)[2:]: r.color_ramp.elements.remove(e)
    for i,(position,color) in enumerate(stops):
        e=r.color_ramp.elements[i] if i<2 else r.color_ramp.elements.new(position)
        e.position=position; e.color=(*color,1)
    t.links.new(source,r.inputs['Fac'])
    return r.outputs['Color']


def build_materials():
    mats={}
    for key,axis in [('wood',(95,95,2.8)),('wood_x',(2.8,95,95))]:
        m,t,bs=_material('Mine.'+key,(.13,.095,.065),.88)
        grain=_noise(t,1,4,.7,axis)
        c=_ramp(t,grain.outputs['Fac'],[(.25,(.006,.0048,.0037)),(.43,(.018,.012,.008)),(.57,(.038,.027,.017)),(.75,(.075,.056,.037))])
        # Damp staining follows the bottom of the asset rather than uniform mottling.
        coord=_node(t,'ShaderNodeTexCoord'); xyz=_node(t,'ShaderNodeSeparateXYZ')
        t.links.new(coord.outputs['Object'],xyz.inputs['Vector'])
        height=_node(t,'ShaderNodeMapRange'); height.clamp=True
        height.inputs['From Min'].default_value=0; height.inputs['From Max'].default_value=.34
        height.inputs['To Min'].default_value=.48; height.inputs['To Max'].default_value=1
        t.links.new(xyz.outputs['Z'],height.inputs['Value'])
        stained=_node(t,'ShaderNodeMixRGB',blend_type='MULTIPLY'); stained.inputs[0].default_value=1
        t.links.new(c,stained.inputs[1]); t.links.new(height.outputs['Result'],stained.inputs[2])
        t.links.new(stained.outputs['Color'],bs.inputs['Base Color'])
        bump=_node(t,'ShaderNodeBump'); bump.inputs['Strength'].default_value=.4; bump.inputs['Distance'].default_value=.0009
        t.links.new(grain.outputs['Fac'],bump.inputs['Height']); t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
        mats[key]=m
    for key,axis in [('wood_rough',(34,34,4.5)),('wood_rough_x',(4.5,34,34))]:
        m,t,bs=_material('Mine.'+key,(.025,.021,.016),.94)
        grain=_noise(t,1,5,.78,axis)
        micro=_noise(t,1,4,.76,(370,370,23) if key=='wood_rough' else (23,370,370))
        mixed=_node(t,'ShaderNodeMixRGB',blend_type='MULTIPLY'); mixed.inputs[0].default_value=.38
        t.links.new(grain.outputs['Fac'],mixed.inputs[1]); t.links.new(micro.outputs['Fac'],mixed.inputs[2])
        c=_ramp(t,mixed.outputs['Color'],[(.18,(.0015,.0014,.0012)),(.33,(.005,.0045,.0037)),(.46,(.013,.011,.0085)),(.64,(.032,.027,.018))])
        t.links.new(c,bs.inputs['Base Color'])
        bump=_node(t,'ShaderNodeBump'); bump.inputs['Strength'].default_value=.6; bump.inputs['Distance'].default_value=.0017
        t.links.new(micro.outputs['Fac'],bump.inputs['Height']); t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
        mats[key]=m
    for key in ['iron','rust']:
        m,t,bs=_material('Mine.'+key,(.042,.029,.02),.74,.58)
        n=_noise(t,42,4,.77)
        stops=[(.20,(.006,.007,.007)),(.52,(.015,.014,.013)),(.62,(.040,.020,.010)),(.75,(.11,.043,.013)),(.87,(.19,.075,.028))]
        if key=='rust': stops=[(.18,(.045,.022,.010)),(.5,(.12,.051,.019)),(.8,(.23,.11,.039))]
        t.links.new(_ramp(t,n.outputs['Fac'],stops),bs.inputs['Base Color'])
        bump=_node(t,'ShaderNodeBump'); bump.inputs['Distance'].default_value=.00065; bump.inputs['Strength'].default_value=.4
        micro=_noise(t,550,2,.7)
        t.links.new(micro.outputs['Fac'],bump.inputs['Height']); t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
        bs.inputs['Metallic'].default_value=.38 if key=='iron' else .13
        mats[key]=m
    m,t,bs=_material('Mine.glass',(.10,.13,.12),.16)
    bs.inputs['Transmission Weight'].default_value=.88
    bs.inputs['IOR'].default_value=1.52
    n=_noise(t,1,3,.6,(70,70,3))
    bump=_node(t,'ShaderNodeBump'); bump.inputs['Distance'].default_value=.00022; bump.inputs['Strength'].default_value=.3
    t.links.new(n.outputs['Fac'],bump.inputs['Height']); t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
    mats['glass']=m
    mats['grime']=_material('Mine.grime',(.025,.022,.017),.97)[0]
    mats['concrete']=_material('Stage.concrete',(.033,.035,.034),.92)[0]
    return mats


def import_asset(data,prefix='asset',offset=(0,0,0),debug_uv=False):
    mats=build_materials()
    objects={}
    for n in data['nodes']:
        obj=bpy.data.objects.new(prefix+'.'+n['id'],None)
        bpy.context.collection.objects.link(obj)
        obj.location=n['translation']; obj.rotation_euler.z=n.get('rotation_z',0)
        obj['stable_id']=n['id']; obj['role']=n.get('role','')
        objects[n['id']]=obj
    for n in data['nodes']:
        obj=objects[n['id']]
        if n.get('parent') is not None: obj.parent=objects[n['parent']]
        else: obj.location+=Vector(offset)
    checker=None
    if debug_uv:
        checker,t,bs=_material('Debug.metric_10cm',(.7,.7,.7),.7)
        uv=_node(t,'ShaderNodeTexCoord'); ck=_node(t,'ShaderNodeTexChecker')
        ck.inputs['Scale'].default_value=10
        ck.inputs['Color1'].default_value=(.018,.06,.12,1)
        ck.inputs['Color2'].default_value=(.8,.64,.1,1)
        t.links.new(uv.outputs['UV'],ck.inputs['Vector']); t.links.new(ck.outputs['Color'],bs.inputs['Base Color'])
    for p in data['parts']:
        mesh=bpy.data.meshes.new(prefix+'.'+p['id'])
        mesh.from_pydata(p['vertices'],[],p['faces']); mesh.update()
        if p['material'] in ('iron','rust') and len(mesh.polygons)>12:
            for poly in mesh.polygons: poly.use_smooth=(poly.loop_total<=4)
        obj=bpy.data.objects.new(prefix+'.'+p['id'],mesh)
        bpy.context.collection.objects.link(obj); obj.parent=objects[p['node']]
        obj['stable_id']=p['id']; obj['role']=p.get('role',''); obj['material_role']=p['material']
        uv=mesh.uv_layers.new(name='MetricUV')
        for poly,uvs in zip(mesh.polygons,p['uvs']):
            for li,point in zip(poly.loop_indices,uvs): uv.data[li].uv=point
        mk=p['material']
        if mk in ('wood','wood_rough') and p['vertices']:
            span=[max(v[i] for v in p['vertices'])-min(v[i] for v in p['vertices']) for i in range(3)]
            if span[0]>span[2]: mk=mk+'_x'
        if mk in ('painted_steel','dark_steel') and not checker:
            from industrial_materials import steel_material
            material=steel_material(p)
        else:
            material=checker or mats.get(mk,mats['iron'])
        mesh.materials.append(material)
        if p.get('face_materials') and not checker:
            from industrial_materials import cut_material
            slots={p['material']:0}
            for poly,key in zip(mesh.polygons,p['face_materials']):
                if key not in slots:
                    slots[key]=len(mesh.materials)
                    mesh.materials.append(cut_material(key) if key in ('cut_metal','glass_edge') else mats.get(key,material))
                poly.material_index=slots[key]
        if p.get('edge_wear') and mk in ('painted_steel','dark_steel','iron','rust') and not checker:
            from industrial_materials import steel_material
            wear_slots={}
            for side in ('front','back'):
                if any(c['side']==side for c in p['edge_wear']):
                    wear_slots[side]=len(mesh.materials)
                    if mk in ('painted_steel','dark_steel'):directed=steel_material(p,side)
                    else:
                        from industrial_materials import directed_hardware_material
                        directed=directed_hardware_material(p,side,material)
                    from fine_wear_adapter import apply_fine_wear
                    directed=apply_fine_wear(p,side,directed)
                    mesh.materials.append(directed)
            for poly in mesh.polygons:
                if poly.material_index!=0:continue
                side='front' if poly.normal.y<-.9999 else ('back' if poly.normal.y>.9999 else None)
                if side in wear_slots:poly.material_index=wear_slots[side]
        bevel=p.get('bevel',0)
        if p.get('cuts') and bevel>0:
            shortest=min(math.dist(p['vertices'][a],p['vertices'][b]) for f in p['faces'] for a,b in zip(f,f[1:]+f[:1]))
            if shortest<bevel:
                obj['bevel_omission']='Source feature edge smaller than requested decorative bevel radius'
                obj['minimum_feature_edge_m']=shortest;obj['requested_bevel_m']=bevel;bevel=0
        if bevel>0:
            mod=obj.modifiers.new('Real edge radius','BEVEL'); mod.width=bevel; mod.segments=3; mod.affect='EDGES'
            mod.limit_method='ANGLE'; mod.angle_limit=.35
            mod=obj.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL'); mod.keep_sharp=True; mod.weight=40
        objects[p['id']]=obj
    bpy.context.scene.unit_settings.system='METRIC'; bpy.context.scene.unit_settings.scale_length=1
    return objects


def aim(obj,point):
    obj.rotation_euler=(Vector(point)-obj.location).to_track_quat('-Z','Y').to_euler()


def setup_stage(camera=(4,-7,3.3),target=(0,0,1.2),ortho=4.7,resolution=(1600,1100),samples=64):
    scene=bpy.context.scene
    scene.render.engine='CYCLES'; scene.cycles.samples=samples
    scene.cycles.use_denoising=False; scene.cycles.seed=17
    scene.render.threads_mode='FIXED'; scene.render.threads=4
    scene.render.resolution_x=resolution[0]; scene.render.resolution_y=resolution[1]; scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.world.color=(.045,.05,.06)
    scene.view_settings.view_transform='AgX'
    scene.view_settings.look='AgX - Medium High Contrast'
    mats=build_materials()
    bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.015))
    bpy.context.object.name='Stage.floor'; bpy.context.object.data.materials.append(mats['concrete'])
    for name,loc,power,color,size in [('key',(-3.5,-4.5,6),1550,(1,.82,.63),4),('fill',(3,-2,4),900,(.65,.77,1),3),('rim',(0,3,4.5),1500,(1,.9,.75),3)]:
        ld=bpy.data.lights.new('Stage.'+name,'AREA'); ld.energy=power; ld.shape='DISK'; ld.size=size; ld.color=color
        lo=bpy.data.objects.new('Stage.'+name,ld); bpy.context.collection.objects.link(lo); lo.location=loc; aim(lo,(0,0,1.2))
    cd=bpy.data.cameras.new('Stage.camera'); co=bpy.data.objects.new('Stage.camera',cd); bpy.context.collection.objects.link(co)
    co.location=camera; aim(co,target); cd.type='ORTHO'; cd.ortho_scale=ortho; cd.lens=50
    scene.camera=co
    return scene
