#!/usr/bin/env python3
"""Emit deterministic support-query fixtures. Does not execute physics."""
import argparse
import json
from pathlib import Path

from support_contract import DoorSupportContract, PINNED_SCENE_HASH


def generate(output, fixture=None):
    root = Path(__file__).resolve().parent
    scene = json.loads(Path(fixture or root/'fixtures/approved_door.json').read_text())
    contract = DoorSupportContract(scene)
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    manifest = {'schema_version':1, 'source_scene_hash':PINNED_SCENE_HASH,
                'physics_executed':False, 'label':'QUERY-ONLY CSG. NO PHYSICS SIMULATION.',
                'cases':[], 'bridge':{
                    'cut_path':'damage.cuts', 'cut_coordinates':'authored body-local meters',
                    'alive_mount_ids_path':'articulation.support_mount_ids',
                    'desired_constraint_state_path':'articulation.desired_active',
                    'commands_path':'planned_commands',
                    'note':'Logical names are not Jolt BodyIDs. Future adapter owns shape rebuilds, bodies and command deduplication.'}}

    def save(name, result=None):
        state = contract.snapshot()
        if result is not None:
            state['last_damage_result'] = result
        (output/(name+'.json')).write_text(json.dumps(state, sort_keys=True, indent=2, allow_nan=False)+'\n')
        manifest['cases'].append({'name':name, 'file':name+'.json',
            'supports_alive':state['articulation']['supports_alive'],
            'support_mount_ids':state['articulation']['support_mount_ids'],
            'desired_active':state['articulation']['desired_active'],
            'accepted_cut_count':len(state['damage']['cuts'])})

    def cut(i, event, override):
        z = next(s['translation'][2] for s in scene['sockets'] if s['id']==f'door.hinge.{i}.socket')
        return contract.apply_box_cut(event,'frame',[-0.570,-0.08,z-0.027],[-0.505,0.08,z+0.027],
            expected_version=contract.version,override_protection=override)

    save('initial')
    save('protected_cut',cut(0,'protected-cut-0',False))
    cut(0,'override-cut-0',True)
    save('one_support',cut(1,'override-cut-1',True))
    save('zero_support',cut(2,'override-cut-2',True))
    (output/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    summary = '\n'.join([
        'QUERY-ONLY CSG SUPPORT CONTRACT. NO PHYSICS SIMULATION.',
        'Approved source mesh unchanged; render/collision cut geometry is not baked.',
        'Initial: 3 paired supports. Protected cut: rejected, 3 remain.',
        'Two accepted local frame slots: 1 support; one ideal hinge remains planned.',
        'Third accepted local frame slot: 0 supports; remove_constraint + activate_body planned once.',
        'Connectivity: analytical wood extrusion/box decomposition with fixed body owner seeds.',
        'Bearing: 7x7 footprint quadrature, exact contiguous half-thickness backing.',
        'Protection: conservative projected backing-volume overlap and owner-connectivity rejection.',
        'Grips: explicit removal of authored contact/backing source parts only.',
        'No sag, fracture strength, Jolt execution or motion is claimed.',
        f'Source scene SHA-256: {PINNED_SCENE_HASH}',
    ])+'\n'
    (output/'summary.txt').write_text(summary)
    return summary


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', default='out/contract_examples', help='Output directory for query-only examples')
    parser.add_argument('--fixture', default=None, help='Pinned fixture path (contents must match approved hash)')
    args=parser.parse_args()
    print(generate(args.output,args.fixture),end='')


if __name__=='__main__':
    main()
