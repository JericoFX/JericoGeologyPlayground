"""Blender 4.3 authoring adapter. The core contract remains Blender independent."""
import bpy
import math
import hashlib
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
    for axis in range(3):
        key='wood_'+str(axis);m,t,bs=_material('Furniture.'+key,(.075,.055,.034),.78)
        scale=[115,115,115];scale[axis]=2.8
        grain=_noise(t,1,3,.72,scale)
        timber=_ramp(t,grain.outputs['Fac'],[(.25,(.010,.007,.004)),(.43,(.026,.019,.011)),(.57,(.055,.039,.022)),(.74,(.095,.069,.040))])
        abrasion=[120,120,120];abrasion[axis]=9
        patch=_noise(t,1,4,.7,abrasion)
        mask=_ramp(t,patch.outputs['Fac'],[(.36,(.025,.025,.025)),(.53,(.055,.055,.055)),(.62,(.40,.40,.40)),(.73,(.88,.88,.88))])
        mix=_node(t,'ShaderNodeMixRGB',blend_type='MIX');mix.inputs[1].default_value=(.009,.010,.008,1)
        t.links.new(mask,mix.inputs[0]);t.links.new(timber,mix.inputs[2]);t.links.new(mix.outputs['Color'],bs.inputs['Base Color'])
        geometry=_node(t,'ShaderNodeNewGeometry');xyz=_node(t,'ShaderNodeSeparateXYZ');t.links.new(geometry.outputs['Position'],xyz.inputs['Vector'])
        damp=_node(t,'ShaderNodeMapRange');damp.clamp=True;damp.inputs['From Max'].default_value=.3;damp.inputs['To Min'].default_value=.46;damp.inputs['To Max'].default_value=1;t.links.new(xyz.outputs['Z'],damp.inputs['Value'])
        stain=_node(t,'ShaderNodeMixRGB',blend_type='MULTIPLY');stain.inputs[0].default_value=1;t.links.new(mix.outputs['Color'],stain.inputs[1]);t.links.new(damp.outputs['Result'],stain.inputs[2]);t.links.new(stain.outputs['Color'],bs.inputs['Base Color'])
        bump=_node(t,'ShaderNodeBump');bump.inputs['Strength'].default_value=.28;bump.inputs['Distance'].default_value=.0005
        t.links.new(grain.outputs['Fac'],bump.inputs['Height']);t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
        # Damp finish responds locally to the remaining dark coating.
        rough=_node(t,'ShaderNodeMapRange');rough.inputs['To Min'].default_value=.82;rough.inputs['To Max'].default_value=.94
        t.links.new(mask,rough.inputs['Value']);t.links.new(rough.outputs['Result'],bs.inputs['Roughness'])
        bs.inputs['Specular IOR Level'].default_value=.10
        mats[key]=m
    mats['wood']=mats['wood_2']
    m,t,bs=_material('Furniture.endgrain',(.10,.073,.044),.87)
    n=_noise(t,63,3,.65,(1,1,1));t.links.new(_ramp(t,n.outputs['Fac'],[(.24,(.025,.017,.010)),(.65,(.052,.036,.019)),(.84,(.088,.060,.033))]),bs.inputs['Base Color']);mats['wood_edge']=m
    for key in ['iron','rust']:
        m,t,bs=_material('Furniture.'+key,(.040,.028,.02),.63,.56)
        n=_noise(t,74,3,.73)
        t.links.new(_ramp(t,n.outputs['Fac'],[(.23,(.013,.014,.013)),(.47,(.042,.039,.030)),(.62,(.095,.041,.014)),(.82,(.22,.11,.042))]),bs.inputs['Base Color'])
        bump=_node(t,'ShaderNodeBump');bump.inputs['Distance'].default_value=.0005;bump.inputs['Strength'].default_value=.33;t.links.new(n.outputs['Fac'],bump.inputs['Height']);t.links.new(bump.outputs['Normal'],bs.inputs['Normal']);mats[key]=m
    mats['paper']=_material('Furniture.label',(.075,.063,.041),.94)[0]
    mats['grime']=_material('Furniture.recess',(.006,.005,.004),.98)[0]
    mats['concrete']=_material('Stage.concrete',(.016,.019,.022),.92)[0]
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
        if p.get('grain_axis'):
            mk='wood_'+str(max(range(3),key=lambda i:abs(p['grain_axis'][i])))
        elif mk in ('wood','wood_rough') and p['vertices']:
            span=[max(v[i] for v in p['vertices'])-min(v[i] for v in p['vertices']) for i in range(3)]
            if span[0]>span[2]: mk=mk+'_x'
        if mk in ('painted_steel','dark_steel') and not checker:
            from industrial_materials import steel_material
            material=steel_material(p)
        else:
            material=checker or mats.get(mk,mats['iron'])
        if p.get('grain_axis') and not checker:
            material=material.copy();material.name='Furniture.'+p['id']
            seed=int(hashlib.sha256(p['id'].encode()).hexdigest()[:8],16)
            shift=((seed%173)*.137,((seed//173)%113)*.153,((seed//193)%149)*.127)
            for coordinate in [n for n in material.node_tree.nodes if n.type=='TEX_COORD']:
                old=list(coordinate.outputs['Object'].links);add=_node(material.node_tree,'ShaderNodeVectorMath',operation='ADD');add.inputs[1].default_value=shift
                material.node_tree.links.new(coordinate.outputs['Object'],add.inputs[0])
                for link in old:material.node_tree.links.new(add.outputs['Vector'],link.to_socket)
        mesh.materials.append(material)
        if p.get('face_materials') and not checker:
            slots={p['material']:0}
            for poly,key in zip(mesh.polygons,p['face_materials']):
                if key not in slots:
                    slots[key]=len(mesh.materials)
                    mesh.materials.append(mats.get(key,material))
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
            if p.get('bevel_material') and p['bevel_material'] in slots:mod.material=slots[p['bevel_material']]
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
