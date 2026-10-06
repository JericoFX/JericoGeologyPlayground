#!/usr/bin/env python3
"""Offline independent native OBJ/MTL verifier; stdlib only, no exporter imports.

Usage: python tools/validate_native_obj.py asset.obj --scene scene.json
       python tools/validate_native_obj.py --self-test
"""
import argparse
from collections import defaultdict, Counter
import hashlib
import shlex
import json
import math
from pathlib import Path
import struct
import tempfile
import zlib


def require(condition, message):
    if not condition:
        raise ValueError(message)


def close(a, b, tolerance=2e-7):
    return len(a) == len(b) and all(abs(x-y) <= tolerance * max(1, abs(y)) for x,y in zip(a,b))


def sub(a,b):
    return tuple(x-y for x,y in zip(a,b))


def dot(a,b):
    return sum(x*y for x,y in zip(a,b))


def cross(a,b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def unit(a):
    length = math.sqrt(dot(a,a))
    require(length > 0, 'zero normal')
    return tuple(x/length for x in a)


def numbers(tokens, count, context):
    require(len(tokens) == count, context+': component count')
    result = tuple(float(x) for x in tokens)
    require(all(math.isfinite(x) for x in result), context+': nonfinite number')
    return result


def safe_reference(root, relative, context):
    # OBJ bundle references are portable POSIX relative paths. Reject traversal
    # even if a particular '..' happens to remain inside the output folder.
    require(relative and '\\' not in relative and ':' not in relative and not relative.startswith('/'), context+': path must be portable and relative')
    rel = Path(relative)
    require('..' not in rel.parts and not rel.is_absolute(), context+': path traversal')
    target = (root / rel).resolve()
    require(target.is_relative_to(root.resolve()), context+': symlink escapes bundle')
    require(target.is_file(), context+': referenced file missing: '+relative)
    return target


def png16(data):
    require(data[:8] == b'\x89PNG\r\n\x1a\n', 'PNG signature')
    offset, payload, dimensions = 8, bytearray(), None
    ended = False
    while offset < len(data):
        require(offset+12 <= len(data), 'truncated PNG chunk')
        size = struct.unpack_from('>I',data,offset)[0]
        require(offset+size+12 <= len(data), 'PNG chunk exceeds image')
        kind = data[offset+4:offset+8]; body = data[offset+8:offset+8+size]
        expected_crc = struct.unpack_from('>I',data,offset+8+size)[0]
        require(zlib.crc32(kind+body) & 0xffffffff == expected_crc, 'PNG CRC mismatch')
        if kind == b'IHDR':
            require(dimensions is None and offset == 8 and size == 13, 'PNG IHDR order')
            w,h,bits,color,compression,filtering,interlace = struct.unpack('>IIBBBBB',body)
            require(w > 0 and h > 0 and bits == 16 and color == 2, 'expected native RGB16 PNG')
            require((compression,filtering,interlace) == (0,0,0), 'unsupported PNG encoding')
            dimensions = w,h
        elif kind == b'IDAT':
            require(dimensions is not None, 'PNG IDAT before IHDR')
            payload.extend(body)
        elif kind == b'IEND':
            require(size == 0, 'PNG IEND size')
            ended = True; offset += size+12; break
        offset += size+12
    require(ended and offset == len(data) and dimensions is not None, 'PNG missing IEND or trailing bytes')
    stream = zlib.decompressobj()
    raw = stream.decompress(payload) + stream.flush()
    require(stream.eof and not stream.unused_data, 'invalid PNG compression stream')
    w,h = dimensions; stride=w*6
    require(len(raw) == h*(stride+1), 'PNG decompressed size')
    decoded=bytearray(); prev=bytearray(stride)
    for y in range(h):
        filt=raw[y*(stride+1)]; require(filt in range(5), 'PNG filter')
        row=bytearray(raw[y*(stride+1)+1:(y+1)*(stride+1)])
        if filt:
            for x in range(stride):
                left=row[x-6] if x >= 6 else 0; above=prev[x]; upper_left=prev[x-6] if x>=6 else 0
                if filt==1: prediction=left
                elif filt==2: prediction=above
                elif filt==3: prediction=(left+above)//2
                else:
                    p=left+above-upper_left
                    distances=(abs(p-left),abs(p-above),abs(p-upper_left))
                    prediction=(left,above,upper_left)[distances.index(min(distances))]
                row[x]=(row[x]+prediction)&255
        decoded.extend(row); prev=row
    return dimensions, decoded


def read_obj(path):
    positions,uvs,normals,faces,libraries=[],[],[],[],[]
    obj,groups,material,part_id,pending_part_id=None,[],None,None,None
    comments=[]
    for line_no,line in enumerate(path.read_text(encoding='utf8').splitlines(),1):
        context=f'{path.name}:{line_no}'
        if line.lstrip().startswith('#'):
            comment=line.lstrip()[1:].strip(); comments.append(comment)
            if comment.startswith('jerico_part_id '):
                pending_part_id=json.loads(comment[len('jerico_part_id '):])
                require(isinstance(pending_part_id,str),context+': part ID must be string')
            continue
        tokens=shlex.split(line,comments=True,posix=True)
        if not tokens: continue
        key,args=tokens[0],tokens[1:]
        if key=='v': positions.append(numbers(args,3,context))
        elif key=='vt': uvs.append(numbers(args,2,context))
        elif key=='vn': normals.append(numbers(args,3,context))
        elif key=='o':
            require(len(args)==1,context+': one object name required')
            obj=args[0]
            # The source ID comment precedes its object and applies only to it.
            part_id=pending_part_id or obj
            pending_part_id=None
        elif key=='g':
            require(len(args)>0,context+': group is empty'); groups=args
        elif key=='usemtl':
            require(len(args)==1,context+': one material required'); material=args[0]
        elif key=='mtllib':
            require(len(args)==1,context+': one portable material library required')
            libraries.append(safe_reference(path.parent,args[0],context))
        elif key=='f':
            require(len(args)==3,context+': face is not a triangle')
            require(obj and groups and material,context+': face missing object/group/material')
            refs=[]
            for value in args:
                indexes=value.split('/')
                require(len(indexes)==3 and all(indexes),context+': full v/vt/vn reference required')
                resolved=[]
                for i,(text,values) in enumerate(zip(indexes,(positions,uvs,normals))):
                    index=int(text)
                    require(index != 0,context+': zero OBJ index')
                    absolute=index-1 if index>0 else len(values)+index
                    require(0<=absolute<len(values),context+': index outside '+('positions','UVs','normals')[i])
                    resolved.append(absolute)
                refs.append(tuple(resolved))
            faces.append({'part':part_id or obj,'object':obj,'groups':tuple(groups),'material':material,'refs':tuple(refs),'line':line_no})
        elif key=='s':
            require(len(args)==1,context+': smoothing declaration')
        else:
            raise ValueError(context+': unsupported native OBJ statement '+key)
    require(positions and uvs and normals and faces, 'empty geometry')
    require(len(libraries)==1, 'expected one native MTL sidecar')
    require(libraries[0].name==path.stem+'.mtl','MTL sidecar stem mismatch')
    return {'positions':positions,'uvs':uvs,'normals':normals,'faces':faces,'mtl':libraries[0],'comments':comments}


def read_mtl(path):
    materials={}; current=None; comments=[]
    for line_no,line in enumerate(path.read_text(encoding='utf8').splitlines(),1):
        if line.lstrip().startswith('#'):
            comments.append(line.lstrip()[1:].strip()); continue
        tokens=shlex.split(line,comments=True,posix=True)
        if not tokens: continue
        context=f'{path.name}:{line_no}'; key,args=tokens[0],tokens[1:]
        if key=='newmtl':
            require(len(args)==1 and args[0] not in materials,context+': duplicate/invalid material')
            current={'maps':{}}; materials[args[0]]=current; continue
        require(current is not None,context+': property before newmtl')
        if key.startswith('map_') or key in ('norm','bump','disp','decal'):
            # The native writer does not transform texture coordinates or hide
            # normal handedness in MTL-specific options.
            if len(args)==3 and args[:2]==['-bm','1']: args=args[2:]
            require(len(args)==1,context+': unexpected map options')
            require(key not in current['maps'],context+': duplicate texture property')
            current['maps'][key]=safe_reference(path.parent,args[0],context)
        elif key in ('Ka','Kd','Ks','Ke','Tf'):
            require(key not in current,context+': duplicate material property')
            current[key]=numbers(args,3,context)
        elif key in ('Ni','Ns','d','Tr','Pr','Pm','Ps','Pc','Pcr','aniso','anisor','illum'):
            require(key not in current,context+': duplicate material property')
            current[key]=numbers(args,1,context)[0]
        else:
            raise ValueError(context+': unsupported native MTL statement '+key)
    require(materials,'empty MTL')
    return materials,comments


def transform_table(source):
    nodes={n['id']:n for n in source['nodes']}
    require(len(nodes)==len(source['nodes']),'duplicate source node ID')
    transforms={}; active=set()
    def visit(node_id):
        if node_id in transforms: return transforms[node_id]
        require(node_id in nodes and node_id not in active,'missing source node or hierarchy cycle')
        active.add(node_id); node=nodes[node_id]
        angle=float(node.get('rotation_z',0)); translation=tuple(node.get('translation',(0,0,0)))
        require(math.isfinite(angle) and len(translation)==3 and all(math.isfinite(x) for x in translation),'invalid source transform')
        if node.get('parent') is not None:
            parent_angle,parent_t=visit(node['parent']); c,s=math.cos(parent_angle),math.sin(parent_angle)
            x,y,z=translation
            translation=(c*x-s*y+parent_t[0],s*x+c*y+parent_t[1],z+parent_t[2]); angle+=parent_angle
        transforms[node_id]=(angle,translation); active.remove(node_id)
        return transforms[node_id]
    for node_id in nodes: visit(node_id)
    return transforms


def validate_source_part(part, faces, obj, transform):
    angle,translation=transform; c,s=math.cos(angle),math.sin(angle)
    def rotate(p): return (c*p[0]-s*p[1],s*p[0]+c*p[1],p[2])
    world=[tuple(x+t for x,t in zip(rotate(p),translation)) for p in part['vertices']]
    source_uv=part['texture_uvs']; source_n=part['normals']
    require(len(source_uv)==len(source_n)==len(part['faces']),part['id']+': source face attribute count')
    lookup=defaultdict(list)
    def key(uv): return tuple(round(x,7) for x in uv)
    for face_index,polygon in enumerate(part['faces']):
        require(len(polygon)>=3 and len(source_uv[face_index])==len(source_n[face_index])==len(polygon),part['id']+': source loop count')
        for loop_index,vertex in enumerate(polygon):
            lookup[key(source_uv[face_index][loop_index])].append((face_index,loop_index,world[vertex],unit(rotate(source_n[face_index][loop_index]))))
    assignments=defaultdict(list)
    for triangle in faces:
        matches=[]
        for vi,ti,ni in triangle['refs']:
            p,uv,n=obj['positions'][vi],obj['uvs'][ti],obj['normals'][ni]
            matching=[candidate for candidate in lookup[key(uv)] if close(p,candidate[2]) and close(n,candidate[3]) and close(uv,source_uv[candidate[0]][candidate[1]])]
            require(matching, f'{part["id"]}: line {triangle["line"]}: source world position/loop normal/original atlas UV mismatch')
            matches.append(matching)
        common=set(x[0] for x in matches[0]) & set(x[0] for x in matches[1]) & set(x[0] for x in matches[2])
        require(len(common)==1,part['id']+': triangle does not identify one source polygon')
        source_face=common.pop()
        loops=[next(x[1] for x in candidates if x[0]==source_face) for candidates in matches]
        require(len(set(loops))==3,part['id']+': triangle repeats a source loop')
        assignments[source_face].append(loops)
    require(set(assignments)==set(range(len(part['faces']))),part['id']+': source face omitted')
    # Count directed boundary edges: every source polygon boundary must occur
    # once in the correct direction; triangulation diagonals cancel in pairs.
    for face_index,polygon in enumerate(part['faces']):
        tris=assignments[face_index]
        require(len(tris)==len(polygon)-2,part['id']+': source polygon triangle count')
        directed=Counter((tri[i],tri[(i+1)%3]) for tri in tris for i in range(3))
        for i in range(len(polygon)):
            edge=(i,(i+1)%len(polygon)); require(directed[edge]==1,part['id']+': polygon boundary lost/reversed/duplicated')
            directed[edge]-=1
        for edge,count in directed.items():
            require(count==directed[(edge[1],edge[0])] and count<=1,part['id']+': invalid triangulation interior edges')


def validate(path, source=None, scene_dir=None):
    path=Path(path).resolve(); obj=read_obj(path); materials,mtl_comments=read_mtl(obj['mtl'])
    for i,n in enumerate(obj['normals']):
        require(abs(dot(n,n)-1)<2e-5,f'normal {i+1}: non-unit normal')
    for i,uv in enumerate(obj['uvs']):
        require(all(0<=x<=1 for x in uv),f'UV {i+1}: atlas UV outside [0,1]')
    part_faces=defaultdict(list); material_parts=defaultdict(set); hemisphere_min=1
    for face in obj['faces']:
        require(face['material'] in materials,f'line {face["line"]}: missing material')
        points=[obj['positions'][v] for v,t,n in face['refs']]
        area=cross(sub(points[1],points[0]),sub(points[2],points[0]))
        require(dot(area,area)>0,f'line {face["line"]}: degenerate triangle')
        ng=unit(area)
        align=[dot(obj['normals'][n],ng) for v,t,n in face['refs']]
        require(all(x>0 for x in align),f'line {face["line"]}: loop normal crosses geometric hemisphere {align}')
        hemisphere_min=min(hemisphere_min,*align)
        tex=[obj['uvs'][t] for v,t,n in face['refs']]
        a,b=sub(tex[1],tex[0]),sub(tex[2],tex[0])
        require(abs(a[0]*b[1]-a[1]*b[0])>0,f'line {face["line"]}: degenerate atlas UV triangle')
        part_faces[face['part']].append(face); material_parts[face['material']].add(face['part'])
    require(set(material_parts)==set(materials),'unused MTL material')
    require(all(len(parts)==1 for parts in material_parts.values()),'expected a separate atlas material per part')
    require(all(len({f['material'] for f in faces})==1 for faces in part_faces.values()),'part uses multiple materials')
    image_cache={}
    for name,mat in materials.items():
        require(mat.get('Kd')==(1,1,1),name+': Kd must be one')
        require(mat.get('Ni',0)>=1,name+': invalid or missing IOR')
        require(mat.get('d',1)==1 and mat.get('Tr',0)==0,name+': native physical glass must not be approximated as alpha transparency')
        require('map_Kd' in mat['maps'] and 'norm' in mat['maps'],name+': color and tangent normal maps required')
        require('Pr' in mat or 'map_Pr' in mat['maps'],name+': missing scalar or mapped roughness')
        require('Pm' in mat or 'map_Pm' in mat['maps'],name+': missing scalar or mapped metallic')
        for prop in ('Pr','Pm'):
            if 'map_'+prop in mat['maps']:
                require(mat.get(prop,1)==1,name+': mapped '+prop+' multiplier must be one')
        for key in ('Pr','Pm'):
            if key in mat: require(0<=mat[key]<=1,name+': '+key+' outside [0,1]')
        dims=set()
        for key,image_path in mat['maps'].items():
            if image_path not in image_cache:
                blob=image_path.read_bytes(); dimensions,pixels=png16(blob)
                image_cache[image_path]=(hashlib.sha256(blob).hexdigest(),dimensions)
                del pixels,blob
            require(image_path.is_relative_to(path.parent/(path.stem+'_textures')),name+': texture is outside the native sidecar directory')
            dims.add(image_cache[image_path][1])
        require(len(dims)==1,name+': texture dimensions differ')
    if source:
        source_parts={p['id']:p for p in source['parts']}
        require(len(source_parts)==len(source['parts']),'duplicate source part ID')
        require(set(source_parts)==set(part_faces),'source part IDs differ')
        require(len({faces[0]['object'] for faces in part_faces.values()})==len(part_faces),'source parts alias the same object ID')
        transforms=transform_table(source)
        for part_id,part in source_parts.items():
            faces=part_faces[part_id]
            require(len({f['object'] for f in faces})==1 and all(f['object'] in f['groups'] for f in faces),part_id+': inconsistent object/group ID')
            validate_source_part(part,faces,obj,transforms[part['node']])
            mat=materials[faces[0]['material']]
            require(abs(mat['Ni']-part['material_constants']['ior'])<1e-12,part_id+': IOR differs from source')
            def source_path(name):
                p=Path(part['textures'][name])
                if not p.is_absolute() and scene_dir is not None: p=Path(scene_dir)/p
                require(p.is_file(),part_id+': source texture unavailable: '+str(p))
                return p
            for native,mtl in (('base_color','map_Kd'),('normal','norm')):
                require(image_cache[mat['maps'][mtl]][0]==hashlib.sha256(source_path(native).read_bytes()).hexdigest(),part_id+': '+native+' differs from original native PNG16')
            dims,params=png16(source_path('material_params').read_bytes())
            require(dims==image_cache[mat['maps']['map_Kd']][1],part_id+': material parameter dimensions differ')
            if 'resolution' in part['textures']:
                require(dims==(part['textures']['resolution'],)*2,part_id+': source atlas resolution mismatch')
            for channel,prop in ((0,'Pr'),(1,'Pm')):
                if 'map_'+prop not in mat['maps']:
                    constant_name={'Pr':'roughness','Pm':'metallic'}[prop]
                    require(constant_name in part['material_constants'],part_id+': scalar '+prop+' lacks a certified source material constant; a map is required')
                    # Uncovered atlas texels can be zero even for a material
                    # with a constant parameter. Do not guess a coverage mask
                    # or discard zeros: compare the certified scalar exactly.
                    require(abs(mat[prop]-part['material_constants'][constant_name])<1e-12,part_id+': exact '+constant_name+' constant differs')
                else:
                    actual_dim,actual=png16(mat['maps']['map_'+prop].read_bytes())
                    require(actual_dim==dims,part_id+': '+prop+' map dimensions differ')
                    require(all(actual[k:k+6]==params[k+channel*2:k+channel*2+2]*3 for k in range(0,len(params),6)),part_id+': '+prop+' RGB16 differs from native parameter channel')
    return {'status':'PASS','obj':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'mtl':str(obj['mtl']),'parts':len(part_faces),'materials':len(materials),'positions':len(obj['positions']),'uvs':len(obj['uvs']),'normals':len(obj['normals']),'triangles':len(obj['faces']),'png16_images':len(image_cache),'source_geometry_checked':source is not None,'source_images_checked':source is not None,'minimum_shading_normal_hemisphere_dot':hemisphere_min,'limitations':['OBJ has no native node hierarchy or metric secondary UV layer.','MTL cannot reproduce physical transmission; original tangent normal and source material channels are verified when --scene is supplied.']}


def self_test():
    # A tilted triangle under two different translated/rotated nodes proves
    # source-basis preservation and hierarchy composition independently.
    with tempfile.TemporaryDirectory(prefix='native-obj-validator-test-') as d:
        root = Path(d)
        def png(rgb):
            raw = b'\0' + b''.join(struct.pack('>H', x) for x in rgb)
            def chunk(kind, data):
                return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
            return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 16, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
        texture_dir = root / 'sample_textures'
        texture_dir.mkdir()
        images = {'color': (1000, 2000, 3000), 'normal': (32768, 32768, 65535), 'params': (12000, 22000, 0), 'rough': (12000,)*3, 'metal': (22000,)*3}
        for name, rgb in images.items():
            (texture_dir / (name + '.png')).write_bytes(png(rgb))
        vertices = [(0, 0, 0), (1, 0, 0), (0, 0, 1)]
        total = .7 - .2
        tx, ty, tz = 3 + math.cos(.7) - 2 * math.sin(.7), -2 + math.sin(.7) + 2 * math.cos(.7), 1 + .5
        world = [(math.cos(total)*x-math.sin(total)*y+tx, math.sin(total)*x+math.cos(total)*y+ty, z+tz) for x,y,z in vertices]
        normal = (math.sin(total), -math.cos(total), 0)
        uvs = [(.1,.1), (.8,.1), (.1,.8)]
        text = 'mtllib sample.mtl\n# jerico_part_id \"piece/1\"\no piece_1\ng piece_1\nusemtl piece.1_material\n'
        text += ''.join('v ' + ' '.join(format(c, '.17g') for c in p) + '\n' for p in world)
        text += ''.join('vt ' + ' '.join(str(c) for c in uv) + '\n' for uv in uvs)
        text += 'vn ' + ' '.join(format(c, '.17g') for c in normal) + '\nf 1/1/1 2/2/1 3/3/1\n'
        mtl = 'newmtl piece.1_material\nKd 1 1 1\nNi 1.52\nd 1\nPr 1\nPm 1\nmap_Kd sample_textures/color.png\nnorm sample_textures/normal.png\nmap_Pr sample_textures/rough.png\nmap_Pm sample_textures/metal.png\n'
        target = root / 'sample.obj'; target.write_text(text)
        mtl_path = root / 'sample.mtl'; mtl_path.write_text(mtl)
        source = {'nodes': [{'id':'root','parent':None,'translation':[3,-2,1],'rotation_z':.7}, {'id':'child','parent':'root','translation':[1,2,.5],'rotation_z':-.2}], 'parts': [{'id':'piece/1','node':'child','vertices':vertices,'faces':[[0,1,2]],'normals':[[[0,-1,0]]*3], 'texture_uvs':[uvs], 'textures':{'base_color':str(texture_dir/'color.png'),'normal':str(texture_dir/'normal.png'),'material_params':str(texture_dir/'params.png'),'resolution':1}, 'material_constants':{'ior':1.52}}]}
        validate(target, source)
        mutations = [
            ('out-of-range index', text.replace('f 1/1/1', 'f 9/1/1'), mtl),
            ('missing world transforms', text.replace('v '+' '.join(format(c,'.17g') for c in world[0]), 'v 0 0 0'), mtl),
            ('wrong rotated unit normals', text.replace('vn '+' '.join(format(c,'.17g') for c in normal), 'vn 0 -1 0'), mtl),
            ('wrong texture multiplier', text, mtl.replace('Pr 1','Pr 0.5')),
            ('wrong scalar channel', text, mtl.replace('Pr 1','Pr 0.5').replace('map_Pr sample_textures/rough.png\n','')),
            ('zero index', text.replace('f 1/1/1', 'f 0/1/1'), mtl),
            ('nonfinite vertex', text.replace('v '+str(world[0][0]), 'v nan', 1), mtl),
            ('non-unit normal', text.replace('vn ', 'vn 9 ', 1), mtl),
            ('inward normals', text.replace('vn '+' '.join(format(c,'.17g') for c in normal), 'vn '+' '.join(format(-c,'.17g') for c in normal)), mtl),
            ('atlas bounds', text.replace('vt 0.1 0.1', 'vt -0.1 0.1'), mtl),
            ('non-triangle', text.replace('f 1/1/1 2/2/1 3/3/1', 'f 1/1/1 2/2/1 3/3/1 1/1/1'), mtl),
            ('degenerate triangle', text.replace('f 1/1/1 2/2/1 3/3/1', 'f 1/1/1 1/1/1 3/3/1'), mtl),
            ('missing material', text.replace('usemtl piece.1_material', 'usemtl missing'), mtl),
            ('path traversal', text, mtl.replace('sample_textures/color.png', '../escaped.png')),
            ('absolute path', text, mtl.replace('sample_textures/color.png', str(texture_dir/'color.png'))),
            ('wrong IOR', text, mtl.replace('Ni 1.52','Ni 1.51')),
            ('incorrect UV flip', text.replace('vt 0.1 0.1','vt 0.1 0.9'), mtl),
            ('incorrect source object ID', text.replace('piece/1','other'), mtl),
            ('roughness channel swapped', text, mtl.replace('rough.png','metal.png')),
        ]
        rejected = []
        for name, obj_text, mtl_text in mutations:
            target.write_text(obj_text); mtl_path.write_text(mtl_text)
            try:
                validate(target, source)
            except (ValueError, IndexError, KeyError, struct.error):
                rejected.append(name)
            else:
                raise AssertionError('verifier accepted '+name)
        target.write_text(text)
        scalar_source=json.loads(json.dumps(source))
        scalar_source['parts'][0]['material_constants']['roughness']=12000/65535
        scalar_mtl=mtl.replace('Pr 1','Pr '+repr(12000/65535)).replace('map_Pr sample_textures/rough.png\n','')
        mtl_path.write_text(scalar_mtl)
        validate(target,scalar_source)
        mtl_path.write_text(scalar_mtl.replace('Pr '+repr(12000/65535),'Pr 0.5'))
        try:
            validate(target,scalar_source)
        except ValueError:
            rejected.append('scalar roughness differs from certified source constant')
        else:
            raise AssertionError('verifier accepted corrupted certified roughness')
        target.write_text(text); mtl_path.write_text(mtl)
        normal_path = texture_dir / 'normal.png'
        damaged = bytearray(normal_path.read_bytes()); damaged[29] ^= 1; normal_path.write_bytes(damaged)
        try:
            validate(target)
        except ValueError:
            rejected.append('corrupted PNG CRC')
        else:
            raise AssertionError('verifier accepted corrupted PNG CRC')
        return {'status':'PASS', 'rejected_corruptions':rejected, 'negative_validation_tests':len(rejected)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('obj', type=Path, nargs='?')
    parser.add_argument('--scene', type=Path)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.obj is None and not args.self_test:
        parser.error('an OBJ path or --self-test is required')
    report = validate(args.obj, json.loads(args.scene.read_text()) if args.scene else None, args.scene.parent if args.scene else None) if args.obj else {}
    if args.self_test:
        report['validator_self_test'] = self_test()
    output = json.dumps(report, indent=2)
    if args.report:
        args.report.write_text(output+'\n')
    print(output)


if __name__ == '__main__':
    main()
