"""Opt-in original millimetric coating breakup, evaluated by import and PBR bake.

Material only: no topology, displacement, physical fracture or add-on code.
All channels retain their original inputs wherever the selected front mask is 0.
"""


def shared_math(op):
    """Reuse identical expressions; named contour outputs retain their identity."""
    cache = {}

    def shared(kind, a, b=None, name=None):
        def identity(value):
            return ('socket', value.as_pointer()) if hasattr(value, 'as_pointer') else ('scalar', value)
        key = (kind, identity(a), identity(b))
        if name is None and key in cache:
            return cache[key]
        result = op(kind, a, b, name=name)
        if name is None:
            cache[key] = result
        return result
    return shared


def apply_fine_wear(part, side, source):
    """Return a private material copy only for explicitly enabled front configs."""
    selected = [config for config in part.get('edge_wear', [])
                if config.get('side') == 'front' and config.get('fine_breakup') is True]
    if side != 'front' or not selected:
        return source
    from blender_adapter import _node, _noise, _ramp
    from industrial_materials import _directed_wear

    material = source.copy()
    material.name = source.name + '.FineBreakup'
    tree = material.node_tree
    bs = next(n for n in tree.nodes if n.type == 'BSDF_PRINCIPLED')

    def op(kind, a, b=None, name=None):
        node = _node(tree, 'ShaderNodeMath', operation=kind)
        if name:
            node.name = name
        for index, value in enumerate((a, b)):
            if value is None:
                continue
            if hasattr(value, 'node'):
                tree.links.new(value, node.inputs[index])
            else:
                node.inputs[index].default_value = value
        return node.outputs[0]

    # Reuse the already-composed exact mask only when all front configs opt in.
    directed = [n for n in tree.nodes if n.name.startswith('Directed edge wear (front)')]
    all_front = [config for config in part.get('edge_wear', []) if config.get('side') == 'front']
    if directed and len(selected) == len(all_front):
        edge = directed[-1].outputs[0]
    else:
        coordinates = _node(tree, 'ShaderNodeTexCoord')
        xyz = _node(tree, 'ShaderNodeSeparateXYZ')
        tree.links.new(coordinates.outputs['Object'], xyz.inputs[0])
        edge = _directed_wear(tree, xyz, dict(part, edge_wear=selected), side, op)

    coarse = _noise(tree, 140, 2.5, .72, (1, 1, 1.25))
    fine = _noise(tree, 1250, 2, .73)
    modulation = op('ADD', op('MULTIPLY', coarse.outputs['Fac'], .76),
                    op('MULTIPLY', fine.outputs['Fac'], .24))
    broken = _ramp(tree, modulation, [(.40, (.06, .06, .06)), (.53, (1, 1, 1))])
    mask = op('MULTIPLY', edge, broken, name='Fine front coating mask')
    for channel, value in [('Base Color', (.135, .145, .132, 1)),
                           ('Roughness', (.48, .48, .48, 1)),
                           ('Metallic', (.70, .70, .70, 1))]:
        original = bs.inputs[channel]
        mix = _node(tree, 'ShaderNodeMixRGB', blend_type='MIX')
        mix.name = 'Fine front ' + channel
        tree.links.new(mask, mix.inputs[0])
        if original.is_linked:
            tree.links.new(original.links[0].from_socket, mix.inputs[1])
        else:
            default = original.default_value
            mix.inputs[1].default_value = tuple(default) if hasattr(default, '__len__') else (default, default, default, 1)
        mix.inputs[2].default_value = value
        tree.links.new(mix.outputs[0], original)
    material['fine_breakup'] = True
    material['fine_breakup_scope'] = 'Original 1–6 mm coating colour/roughness/metallic variation inside enabled front contour masks; no geometry change'
    return material
