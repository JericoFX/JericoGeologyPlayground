# Original Nature Generator adapter — private control smoke

Status: `PASS_PRIVATE_CONTROL_SMOKE_ONLY`; full backend/native admission `PARTIAL`.
Date: 2026-09-08. User requested the purchased addon's quality and implementation
instead of accepting the inferior original Jerico v2 approximation.

## What now runs

`private_addon_preview.py` loads and evaluates the installed original
`NG_Fractured_Rock_Formation` object and its original Geometry Nodes. It does not
translate the graph, save a replacement BLEND, copy the purchased library into
the repository, register addon Python, or claim third-party authorship as Jerico's.
The adapter is Jerico glue; the executed geology implementation is Nature Generator.

The library remains at:
`F:/JericoEngine/_local_reference/blender_addons/The Nature Generator/The_Nature_Generator1.2/The_Nature_Generator/asset_library/Nature_Generator_Assets.blend`.
Its before/after SHA-256 is
`234724d0de9f10aeac4674a69801fc659899ef3c753af1409a9564811cc8da47`.

## Reproducible evidence

Private run root: `../_local/private-controls-20260908-1945/` relative to the backend
evidence directory. Directory labels are identifiers, not authoritative timestamps;
actual UTC start/PID/argv are recorded in each `captures/process.json`.

| Clean process case | Vertices | Actual evaluated geometry hash |
| --- | ---: | --- |
| Baseline original preset | 17,987 | `f4f9f3b85284b9f6cf1fa31d1597dc072107012e3d0a0a3200b6f458207f1af4` |
| Same input repeat | 17,987 | `f4f9f3b85284b9f6cf1fa31d1597dc072107012e3d0a0a3200b6f458207f1af4` |
| Mesh Randomisation Seed = 5 | 17,493 | `4e1e9432940bd4dec34faa7310fd42004c9dc312312b15c2f82ae1f17f26a7a4` |

`replay.json` verifies repeated-input identity and changed-seed geometry. Each case
retains original-resolution front/three-quarter PNGs, before/after control schemas,
request (where applicable), invocation, logs and manifest hashes. The two displayed
three-quarter images were visually inspected at original resolution: original
polygonal fractured layers survive and the changed seed changes the cell layout.
This is not proof of physical geology realism or all possible seeds.

Final verification: 139 CPU tests passed (the three Blender-only unittest modules
were excluded from that CPU count). Pillow decoded all six PNGs at 1024×1024;
baseline/repeat three-quarter pixels were identical and changed-seed pixels
differed. `git diff --check -- Tools/BlenderWorker` from the repository root was
clean. This test count is separate from the three-process Blender smoke.

The mesh was frozen from a `VIEWPORT` dependency graph and then rendered in clay.
It is not the addon's higher-resolution final-render branch and not a PBR/material
comparison. The original preset and previous Jerico ellipsoid are different source
inputs. No claim of equal-BSP comparison or final Beauty admission follows.

## Control and version contract

Blender pin: 5.2.1 LTS, build `9e2066aef7ef`, two threads, factory startup, background,
offline mode and disabled automatic script execution. Each smoke process is bounded
by the existing `transformer.library.run_process` 30-second deadline/log budget and
reaped on completion/failure. No second scheduler was introduced.

The actual installed 5.2.1 API is:

```python
socket_value = getattr(modifier.properties.inputs, socket_identifier)
current = socket_value.value
socket_value.value = requested_value
obj.update_tag()
bpy.context.view_layer.update()
```

The `type` property distinguishes `VALUE` from `ATTRIBUTE`; the private profile
rejects attribute-driven inputs rather than silently replacing them. The official
[5.2 NodesModifier reference](https://docs.blender.org/api/5.2/bpy.types.NodesModifier.html)
documents `properties`; live RNA probes established the nested `inputs` socket
groups and writable `value`. Older `modifier[identifier]`/`modifier.get(...)`
assumptions failed in this pinned binary.

Current editable private profile, all bound to original identifiers:

| Socket | Original meaning | Profile range | Effect verified here |
| --- | --- | --- | --- |
| `Socket_2` | Layer Amount | 1–6 | Binding present; geometry effect not yet tested |
| `Socket_6` | Mesh Randomisation Seed | -10000–10000 | Yes, seed 0 to 5 |
| `Socket_14` | Selection Seed | -10000–10000 | Binding present; effect not yet tested |
| `Socket_17` | Extrude Seed | -10000–10000 | Binding present; effect not yet tested |
| `Socket_31` | Noise Seed | -10000–10000 | Binding present; effect not yet tested |

Other inputs, native ranges, panels, descriptions and current values are retained
as `inspection-only`. These profile caps bound the private experiment; they are
not native Engine acceptance caps. A complete addon interface is not claimed.

Actual request example:

```json
{
  "schemaVersion": 1,
  "sourceSha256": "sha256:234724d0de9f10aeac4674a69801fc659899ef3c753af1409a9564811cc8da47",
  "object": "NG_Fractured_Rock_Formation",
  "overrides": [{
    "modifier": "Fractured Rock Formation Generator",
    "nodeGroup": "NG_Fractured_Rock_Formation_Generator",
    "values": {"Socket_6": 5}
  }]
}
```

Library hash, object, modifier, group and socket must all match. Unknown IDs,
unsupported/datablock values, invalid numeric types/ranges, duplicate modifier
assignments and attribute bindings fail before any requested value is applied.
The input request remains unchanged. Applied assignments are JSON-native arrays
so provenance serialization cannot fail on internal Python tuples.

## Retained REDs and corrections

1. `_local/private-controls-20260908-1938/baseline/stderr.log`: direct modifier
   ID-property lookup is unsupported by this runtime. Corrected using inspected
   `properties.inputs.<identifier>.value`, not by altering the original graph.
2. `_local/private-controls-20260908-1944/changed-seed/stderr.log`: actual geometry
   and PNGs were produced, but tuple assignments could not be serialized by the
   strict JSON protocol. A CPU regression reproduced this; assignments now use
   JSON-native lists. That failed run is not the accepted evidence run.
3. Initial CPU RED for the missing validator and subsequent six passing CPU tests
   cover identity, types/ranges, attribute/duplicate rejection and serialization.

Actual coordinated window began 19:39:17 UTC. All Blender processes were absent at
19:45:27 UTC and the window was explicitly released to Astra before its 19:58 limit.
No build, native runtime, shared authority, Engine/Editor edit or Git mutation ran.

## Existing downstream contracts still required

Every result here remains `publishable:false`. There is no BSP wall-patch mapping,
keep-clear/closedness proof, semantic multi-layer provenance, collision validation,
high-to-low bake, fidelity-qualified LOD, PWAS/EPLS integration, native cook or
source-free package acceptance for this adapter. Native ownership is unchanged.
JericoEditor remains the only planned front; M10.5 must consume reconciled adapter
controls through the established GeneratorSchema/job/result route. This private
inspection schema is evidence for that mapping, not a new admitted native ABI.

The user's closed-engine direction and per-component license/provenance audit are
in `PRIVATE_ADDON_REUSE_DIRECTIVE_20260908.md`. Shared authority reconciliation is
still required before a purchased dependency becomes a product dependency.
