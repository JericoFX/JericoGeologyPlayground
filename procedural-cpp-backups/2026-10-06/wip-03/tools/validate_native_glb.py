#!/usr/bin/env python3
"""Independent, offline GLB/PNG16 verifier. No exporter or Blender imports.

Usage: python3 tools/validate_native_glb.py asset.glb --scene scene.json
The optional source scene proves basis, per-loop normals, both UV layers,
node pivots, material image provenance and preserved sockets/grips.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import tempfile
import zlib


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def unit(a):
    length = math.sqrt(dot(a, a))
    require(length > 0, 'zero-length vector')
    return tuple(x / length for x in a)


def convert(a):
    return (a[0], a[2], -a[1])


def close(a, b, tolerance=2e-6):
    return len(a) == len(b) and all(abs(x - y) <= tolerance * max(1, abs(y)) for x, y in zip(a, b))


def paeth(a, b, c):
    p = a + b - c
    distances = (abs(p - a), abs(p - b), abs(p - c))
    return (a, b, c)[distances.index(min(distances))]


def png16(data):
    require(data[:8] == b'\x89PNG\r\n\x1a\n', 'PNG signature')
    offset, payload, dimensions = 8, bytearray(), None
    ended = False
    while offset < len(data):
        require(offset + 12 <= len(data), 'truncated PNG chunk')
        size = struct.unpack_from('>I', data, offset)[0]
        kind = data[offset + 4:offset + 8]
        body = data[offset + 8:offset + 8 + size]
        require(offset + size + 12 <= len(data), 'PNG chunk exceeds image')
        crc = struct.unpack_from('>I', data, offset + 8 + size)[0]
        require(zlib.crc32(kind + body) & 0xffffffff == crc, 'PNG CRC mismatch')
        if kind == b'IHDR':
            require(dimensions is None and offset == 8 and size == 13, 'PNG IHDR order')
            width, height, bits, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', body)
            require(width > 0 and height > 0 and bits == 16 and color in (2, 6), 'expected native RGB/RGBA16 PNG')
            require((compression, filtering, interlace) == (0, 0, 0), 'unsupported native PNG encoding')
            dimensions = width, height
        elif kind == b'IDAT':
            payload.extend(body)
        elif kind == b'IEND':
            require(size == 0, 'PNG IEND length')
            ended = True
            offset += size + 12
            break
        offset += size + 12
    require(ended and offset == len(data) and dimensions is not None, 'PNG missing IEND or trailing data')
    width, height = dimensions
    raw = zlib.decompress(payload)
    pixel_size = 6 if color == 2 else 8
    stride = width * pixel_size
    require(len(raw) == height * (stride + 1), 'PNG decompressed size')
    rows, previous = bytearray(), bytearray(stride)
    for y in range(height):
        filt = raw[y * (stride + 1)]
        require(filt in range(5), 'PNG filter')
        row = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        if filt != 0:
            for x in range(stride):
                a, b, c = row[x - pixel_size] if x >= pixel_size else 0, previous[x], previous[x - pixel_size] if x >= pixel_size else 0
                predictor = (0, a, b, (a + b) // 2, paeth(a, b, c))[filt]
                row[x] = (row[x] + predictor) & 255
        rows.extend(row)
        previous = row
    return dimensions, rows


class Glb:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        require(len(self.data) >= 28, 'GLB is truncated')
        magic, version, length = struct.unpack_from('<III', self.data)
        require((magic, version, length) == (0x46546c67, 2, len(self.data)), 'GLB header')
        n, kind = struct.unpack_from('<II', self.data, 12)
        require(kind == 0x4e4f534a and n % 4 == 0 and n + 28 <= length, 'GLB JSON chunk')
        self.doc = json.loads(self.data[20:20 + n])
        size, kind = struct.unpack_from('<II', self.data, 20 + n)
        require(kind == 0x004e4942 and size % 4 == 0 and size + n + 28 == length, 'GLB BIN chunk')
        self.bin = memoryview(self.data)[28 + n:]
        d = self.doc
        require(d['asset']['version'] == '2.0', 'glTF asset version')
        require(len(d['buffers']) == 1 and 'uri' not in d['buffers'][0], 'GLB must be self-contained')
        declared = d['buffers'][0]['byteLength']
        require(0 <= len(self.bin) - declared <= 3, 'BIN buffer padding')
        for view in d['bufferViews']:
            require(view['buffer'] == 0 and view.get('byteOffset', 0) % 4 == 0, 'buffer view alignment')
            require(view['byteLength'] > 0 and view.get('byteOffset', 0) + view['byteLength'] <= declared, 'buffer view bounds')

    def view(self, index):
        view = self.doc['bufferViews'][index]
        offset = view.get('byteOffset', 0)
        return self.bin[offset:offset + view['byteLength']]

    def accessor(self, index):
        a = self.doc['accessors'][index]
        components = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
        formats = {5126: ('f', 4), 5125: ('I', 4), 5123: ('H', 2), 5121: ('B', 1)}
        code, size = formats[a['componentType']]
        view = self.doc['bufferViews'][a['bufferView']]
        stride = view.get('byteStride', components * size)
        offset = a.get('byteOffset', 0)
        require(offset % size == 0 and a['count'] > 0, 'accessor alignment/count')
        require(offset + (a['count'] - 1) * stride + components * size <= view['byteLength'], 'accessor exceeds view')
        data = self.view(a['bufferView'])
        values = [struct.unpack_from('<' + code * components, data, offset + i * stride) for i in range(a['count'])]
        require(all(math.isfinite(x) for value in values for x in value), 'nonfinite accessor')
        if 'min' in a:
            require(close([min(v[c] for v in values) for c in range(components)], a['min'], 1e-7), 'accessor min mismatch')
            require(close([max(v[c] for v in values) for c in range(components)], a['max'], 1e-7), 'accessor max mismatch')
        return values

    def image(self, texture):
        image = self.doc['images'][self.doc['textures'][texture]['source']]
        require(image['mimeType'] == 'image/png' and 'uri' not in image, 'image is not embedded PNG')
        view = self.doc['bufferViews'][image['bufferView']]
        require('target' not in view and 'byteStride' not in view, 'image view has geometry properties')
        return bytes(self.view(image['bufferView']))


def validate(path, source=None):
    glb = Glb(path)
    d = glb.doc
    report = {'glb': str(path), 'sha256': hashlib.sha256(glb.data).hexdigest(), 'bytes': len(glb.data),
              'meshes': len(d['meshes']), 'materials': len(d['materials']), 'images': len(d['images']),
              'vertices': 0, 'triangles': 0, 'source_geometry_checked': source is not None}
    # Validate every accessor, including any accidentally unused one.
    for i in range(len(d['accessors'])):
        glb.accessor(i)
    source_parts = {p['id']: p for p in source['parts']} if source else {}
    all_images = set()
    for mesh in d['meshes']:
        require(len(mesh['primitives']) == 1, 'expected one native atlas material per part')
        primitive = mesh['primitives'][0]
        require(primitive.get('mode', 4) == 4, 'nontriangle primitive')
        attrs = primitive['attributes']
        require(set(('POSITION', 'NORMAL', 'TANGENT', 'TEXCOORD_0', 'TEXCOORD_1')) <= attrs.keys(), 'missing vertex attributes')
        p, n, t, uv, metric = [glb.accessor(attrs[k]) for k in ('POSITION', 'NORMAL', 'TANGENT', 'TEXCOORD_0', 'TEXCOORD_1')]
        require('min' in d['accessors'][attrs['POSITION']], 'POSITION bounds missing')
        require(len(p) == len(n) == len(t) == len(uv) == len(metric), 'attribute count mismatch')
        indices = [row[0] for row in glb.accessor(primitive['indices'])]
        require(len(indices) % 3 == 0 and all(isinstance(i, int) and 0 <= i < len(p) for i in indices), 'triangle indices')
        for normal, tangent, texcoord in zip(n, t, uv):
            require(abs(dot(normal, normal) - 1) < 2e-5, 'non-unit normal')
            require(abs(dot(tangent[:3], tangent[:3]) - 1) < 2e-5, 'non-unit tangent')
            require(abs(dot(normal, tangent[:3])) < 2e-5, 'nonorthogonal tangent')
            require(tangent[3] in (-1, 1), 'tangent handedness must be +/-1')
            require(all(0 <= c <= 1 for c in texcoord), 'atlas UV bounds')
        for offset in range(0, len(indices), 3):
            a, b, c = indices[offset:offset + 3]
            e1, e2 = sub(p[b], p[a]), sub(p[c], p[a])
            area = cross(e1, e2)
            require(dot(area, area) > 0, 'degenerate triangle')
            geometric_normal = unit(area)
            alignments = [dot(n[i], geometric_normal) for i in (a, b, c)]
            source_loops = primitive.get('extras', {}).get('source_loops', [])
            origin = [source_loops[i] for i in (a, b, c)] if source_loops else []
            context = f'{mesh["name"]}: triangle {offset // 3}, source loops {origin}, dot(Ns,Ng)={alignments}'
            require(all(value > 0 for value in alignments), 'loop normal crosses geometric hemisphere: ' + context)
            du1, dv1 = sub(uv[b], uv[a])
            du2, dv2 = sub(uv[c], uv[a])
            determinant = du1 * dv2 - du2 * dv1
            require(abs(determinant) > 0, 'degenerate UV triangle after float32 quantization')
            tangent = tuple((e1[k] * dv2 - e2[k] * dv1) / determinant for k in range(3))
            bitangent = tuple((e2[k] * du1 - e1[k] * du2) / determinant for k in range(3))
            require(t[a][3] == t[b][3] == t[c][3], 'mixed handedness within one triangle: ' + context)
            for i in (a, b, c):
                sign = dot(cross(n[i], t[i][:3]), bitangent)
                # The native export keeps the original OpenGL +Y normal PNG.
                # Convert derivative handedness for top-left glTF UVs; see
                # docs/glb-tangent-convention.md. This is not a generic rule
                # for every possible explicitly supplied glTF tangent frame.
                require(sign * t[i][3] < 0, 'tangent handedness disagrees with the canonical glTF normal-map frame')
                projected = unit(tuple(tangent[k] - n[i][k] * dot(n[i], tangent) for k in range(3)))
                require(dot(projected, t[i][:3]) > 1 - 2e-6, 'tangent direction disagrees with projected UV derivative')
        mat = d['materials'][primitive['material']]
        mr = mat['pbrMetallicRoughness']
        require(mr['roughnessFactor'] == mr['metallicFactor'] == 1, 'PBR texture factor')
        ex = mat['extensions']
        require(ex['KHR_materials_transmission']['transmissionFactor'] == 1, 'transmission factor')
        require(ex['KHR_materials_ior']['ior'] >= 1, 'IOR range')
        require(ex['KHR_materials_clearcoat']['clearcoatFactor'] == 1, 'clearcoat factor')
        require(set(ex) <= set(d['extensionsUsed']), 'undeclared glTF material extension')
        texture_ids = {'base_color': mr['baseColorTexture']['index'], 'normal': mat['normalTexture']['index'],
                       'orm': mr['metallicRoughnessTexture']['index'],
                       'transmission': ex['KHR_materials_transmission']['transmissionTexture']['index'],
                       'surface': ex['KHR_materials_clearcoat']['clearcoatTexture']['index'],
                       'specular_weight': ex['KHR_materials_specular']['specularTexture']['index']}
        require(mat['occlusionTexture']['index'] == texture_ids['orm'], 'occlusion must use ORM')
        require(ex['KHR_materials_specular']['specularFactor'] == 1, 'specular texture multiplier')
        images = {}
        part = source_parts.get(mesh['name'])
        for key, tid in texture_ids.items():
            image = glb.image(tid)
            images[key] = png16(image)
            all_images.add(d['textures'][tid]['source'])
            if part:
                require(image == Path(part['textures'][key]).read_bytes(), f'{mesh["name"]}: {key} differs from native bake')
        dims = images['orm'][0]
        require(all(im[0] == dims for im in images.values()), 'atlas dimensions mismatch')
        require(len(images['specular_weight'][1]) == dims[0] * dims[1] * 8, 'specular weight must be RGBA16')
        orm = images['orm'][1]
        require(all(orm[k] == orm[k + 1] == 255 for k in range(0, len(orm), 6)), 'ORM R occlusion is not one')
        if part:
            loops = primitive['extras']['source_loops']
            require(len(loops) == len(p), 'source loop provenance count')
            for i, (face, loop) in enumerate(loops):
                vertex = part['faces'][face][loop]
                require(close(p[i], convert(part['vertices'][vertex])), 'source position mismatch')
                require(close(n[i], unit(convert(part['normals'][face][loop]))), 'source loop normal mismatch')
                u, v = part['texture_uvs'][face][loop]
                require(close(uv[i], (u, 1 - v)), 'native atlas UV mismatch')
                require(close(metric[i], part['uvs'][face][loop]), 'metricUV mismatch')
            require(abs(ex['KHR_materials_ior']['ior'] - part['material_constants']['ior']) < 1e-12, 'IOR lost precision')
            params_dim, params = png16(Path(part['textures']['material_params']).read_bytes())
            require(params_dim == dims, 'params dimensions')
            transmission = images['transmission'][1]
            for i in range(0, len(params), 6):
                require(orm[i + 2:i + 4] == params[i:i + 2] and orm[i + 4:i + 6] == params[i + 2:i + 4], 'ORM rough/metal packing')
                require(transmission[i:i + 2] == params[i + 4:i + 6], 'transmission R packing')
            # Byte-for-byte identity of the original native normal PNG was
            # checked above. Green must not be transformed on this export path.
        report['vertices'] += len(p)
        report['triangles'] += len(indices) // 3
    require(len(all_images) == len(d['images']), 'unvalidated embedded image')
    parents = {}
    for parent, node in enumerate(d['nodes']):
        for child in node.get('children', []):
            require(0 <= child < len(d['nodes']) and child not in parents, 'invalid or multiply-parented node')
            parents[child] = parent
        if 'rotation' in node:
            require(abs(dot(node['rotation'], node['rotation']) - 1) < 1e-10, 'node quaternion is not unit')
    roots = d['scenes'][d['scene']]['nodes']
    require(set(roots) == set(range(len(d['nodes']))) - set(parents), 'root nodes mismatch')
    for index in range(len(d['nodes'])):
        visited = set()
        while index in parents:
            require(index not in visited, 'node cycle')
            visited.add(index)
            index = parents[index]
    if source:
        require(d['extras']['sockets'] == source.get('sockets', []) and d['extras']['grips'] == source.get('grips', []), 'socket/grip metadata altered')
        native_nodes = {node['extras']['id']: (i, node) for i, node in enumerate(d['nodes']) if 'mesh' not in node}
        for expected in source['nodes']:
            i, actual = native_nodes[expected['id']]
            require(actual['extras'] == expected, 'node ID/pivot metadata altered')
            require(close(actual['translation'], convert(expected['translation']), 1e-12), 'node pivot basis conversion')
            a = expected['rotation_z'] / 2
            require(close(actual['rotation'], (0, math.sin(a), 0, math.cos(a)), 1e-12), 'node rotation basis conversion')
            require((i not in parents) if expected['parent'] is None else parents[i] == native_nodes[expected['parent']][0], 'source node hierarchy mismatch')
        require(set(source_parts) == {mesh['name'] for mesh in d['meshes']}, 'source part IDs differ')
    report['status'] = 'PASS'
    return report


def self_test(path):
    """Prove the verifier rejects broken geometry, tangent frames and image data."""
    glb = Glb(path)
    start = len(glb.data) - len(glb.bin)
    primitive = glb.doc['meshes'][0]['primitives'][0]

    def attribute_offset(index):
        accessor = glb.doc['accessors'][index]
        view = glb.doc['bufferViews'][accessor['bufferView']]
        return start + view.get('byteOffset', 0) + accessor.get('byteOffset', 0)

    cases = []
    broken = bytearray(glb.data)
    struct.pack_into('<I', broken, attribute_offset(primitive['indices']), 0xffffffff)
    cases.append(('index outside geometry', broken))
    broken = bytearray(glb.data)
    struct.pack_into('<f', broken, attribute_offset(primitive['attributes']['POSITION']), float('nan'))
    cases.append(('nonfinite geometry', broken))
    broken = bytearray(glb.data)
    for mesh in glb.doc['meshes']:
        attrs = mesh['primitives'][0]['attributes']
        normals, tangents = glb.accessor(attrs['NORMAL']), glb.accessor(attrs['TANGENT'])
        offset = attribute_offset(attrs['TANGENT'])
        for i, tangent in enumerate(tangents):
            bitangent = cross(normals[i], tangent[:3])
            rotated = tuple((tangent[c] + bitangent[c]) / math.sqrt(2) for c in range(3)) + (tangent[3],)
            struct.pack_into('<4f', broken, offset + 16 * i, *rotated)
    cases.append(('orthogonal but wrongly rotated tangent', broken))
    broken = bytearray(glb.data)
    for mesh in glb.doc['meshes']:
        attrs = mesh['primitives'][0]['attributes']
        for semantic, count in (('NORMAL', 3), ('TANGENT', 4)):
            offset = attribute_offset(attrs[semantic])
            for i, value in enumerate(glb.accessor(attrs[semantic])):
                inverted = tuple(-x for x in value) if semantic == 'NORMAL' else value[:3] + (-value[3],)
                struct.pack_into('<' + 'f' * count, broken, offset + 4 * count * i, *inverted)
    cases.append(('inward shading normals with matching inverted tangent handedness', broken))
    broken = bytearray(glb.data)
    for mesh in glb.doc['meshes']:
        index = mesh['primitives'][0]['attributes']['TANGENT']
        offset = attribute_offset(index)
        for i, value in enumerate(glb.accessor(index)):
            struct.pack_into('<f', broken, offset + 16 * i + 12, -value[3])
    cases.append(('raw UV handedness paired with unmodified normal green', broken))
    broken = bytearray(glb.data)
    image = glb.doc['images'][0]
    view = glb.doc['bufferViews'][image['bufferView']]
    broken[start + view.get('byteOffset', 0) + 29] ^= 1
    cases.append(('corrupted PNG CRC', broken))
    with tempfile.TemporaryDirectory(prefix='jerico-glb-negative-') as directory:
        target = Path(directory) / 'invalid.glb'
        for name, data in cases:
            target.write_bytes(data)
            try:
                validate(target)
            except (ValueError, IndexError, KeyError, struct.error):
                continue
            raise ValueError('validator accepted ' + name)
    return len(cases)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('glb', type=Path)
    parser.add_argument('--scene', type=Path)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--self-test', action='store_true', help='also reject six deliberately corrupted GLBs')
    args = parser.parse_args()
    scene = json.loads(args.scene.read_text()) if args.scene else None
    report = validate(args.glb, scene)
    if args.self_test:
        report['negative_validation_tests'] = self_test(args.glb)
    text = json.dumps(report, indent=2)
    if args.report:
        args.report.write_text(text + '\n')
    print(text)


if __name__ == '__main__':
    main()
