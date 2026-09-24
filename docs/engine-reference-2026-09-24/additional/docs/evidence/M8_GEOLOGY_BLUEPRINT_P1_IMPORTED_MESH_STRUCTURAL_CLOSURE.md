# M8 Geology Blueprint P1 Imported-Mesh Structural Operators Closure

Date: 2026-08-29

Slice: `M8-GEOLOGY-BLUEPRINT-P1-IMPORTED-MESH-STRUCTURAL-OPERATORS`

Status: `PASS`

## Result

JericoGltfCook now owns one bounded imported-mesh structural operation:
deterministic planar displacement of one accepted canonical submesh during an
offline/static cook. The selected boundary is exactly:

```text
DIRECT_MESH_OFFLINE_STATIC_PLANAR_DISPLACEMENT
```

It deepens the existing glTF canonical import and cook path. It does not add a
second importer, mesh representation, geology graph, field conversion, Boolean,
remesher, retopology system, runtime destruction path or editor UI.

The operation:

- identifies one flattened submesh by ordinal and accepted canonical material
  name/GUID;
- applies a smooth planar falloff and one finite displacement vector;
- preserves source files, vertex/index cardinality, triangle order, submesh and
  material identity, UVs, optional `GEOL` classification and unselected data;
- rejects shared vertex ownership across material regions rather than silently
  changing another submesh;
- reconstructs selected normals/tangents and whole-mesh bounds;
- preserves whether the imported asset participates in the optional visibility
  record system, recomputing records only when that contract already existed;
- cooks an exact static triangle `.rcoll` from the final `.rmesh` positions and
  indices;
- includes every structural parameter in recipe/manifest identity and republishes
  through the existing atomic directory path.

Research provenance and the independent design precede implementation in:

- `M8_GEOLOGY_BLUEPRINT_P1_IMPORTED_MESH_STRUCTURAL_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-29-m8-imported-mesh-structural-operators-design.md`;
- `docs/superpowers/plans/2026-08-29-m8-imported-mesh-structural-operators.md`.

## RED chronology

1. The operator contract first failed to link with unresolved
   `applyImportedMeshPlanarDisplacement` (`LNK2019`/`LNK1120`).
2. The existing cooker rejected the structural CLI with
   `gltf_cook.invalid_arguments` and exit 2.
3. The directory verifier test failed to compile because collision inventory
   was absent from its receipt.
4. A later real Vulkan package run exposed a blank frame. A focused RED then
   demonstrated that the operator invented visibility participation for an
   imported mesh whose canonical source had none. That contract now remains
   absent; already-present records are still recomputed.
5. The first real visual fixture used a 0.20 m displacement on a source only
   about 0.02 m across. The source-free runtime correctly drew it outside the
   useful camera composition. The proof recipe was corrected to a scale-coherent
   0.002 m displacement with a 0.01 m transition; no production limit was
   invented to forbid deliberate larger edits.

These were observable failures. A compiler-only, parser-only, package-only or
blank-frame result was not accepted as closure.

## Tests and observed proof

### Operator and negative contracts

Debug and Release focused executables pass:

- immutable input and deterministic repeat;
- exact no-op at zero amplitude;
- selected structural effect and untouched unselected geometry;
- unchanged topology/material/UV/optional geology lineage;
- valid bounds, unit normals and orthogonal tangents;
- visibility participation preservation and existing-record recomputation;
- stale material identity, shared vertex ownership, invalid direction,
  degenerate triangle, work ceiling and cancellation failures.

### Real import, reimport and exact collision

Fresh Release processes against the Khronos BoomBox fixture report:

```text
gltf_cook.structural_proof
source_immutable=true
deterministic=true
collision_exact=true
reimport_identity=true
vertices=3575
indices=18108
```

The gate cooks two identical source processes and a third process after a
bounded in-range source-position mutation. It proves:

- original `.gltf` and `.bin` immutability;
- byte-identical repeat outputs;
- stable root, material and collision GUIDs across reimport;
- changed source/content identity after the source mutation;
- unchanged topology cardinality;
- exact final render/collision positions and indices;
- complete directory verification including the optional collision leaf.

Original fixture hashes remain:

```text
BoomBox.gltf B1D2E0E56864C71A4B65FC560BFC1572B1EB32CFB9C8E86D06E6F9ACE2803A43
BoomBox.bin  F6C61A95AF3E0E2462A9AC01D1E681706149452E0191930E0591A81D50B5B944
```

Final scale-coherent Release products:

```text
model.rmesh  EA212F159BF028FF52C3A9AFCF8AE8E9DFB194D18C12BA3CD10F901B0340B230
model.rcoll  64983361B98C48BFF0E24260C94D810A3BAA2C3D5D898645912F95CD316F6419
manifest     FD05D8EC9321EDAA67AEA4203CA6A7A832FCBED812C3DDA2CB5CA3CB5C46246A
```

### Source-free package and Vulkan consumer

The existing PackageTool produced one seven-entry package:

```text
structural-release.epak
bytes=24600110
SHA-256=14A1A28279F04D20AABF29AC711525AA41F83F68F033A15CA310F1D40483B434
```

Shipping validation reports seven admitted entries and no authoring/editor
fallback markers. Package-only Vulkan replay on NVIDIA GeForce RTX 3090 reports:

```text
backend=Vulkan
resolution=1280x720
vertices=3575
indices=18108
draws=1
materials=1
textures=4
fallbacks=0
source_free=true
```

The inspected original-resolution frame is non-empty and visibly contains the
structurally displaced BoomBox. Its SHA-256 is:

```text
BA6227FAEFD1101FA20B46975D2927750305D9AB27979F0987FD7C00AF123C59
```

Stable image:

`docs/evidence/images/m8-geology-blueprint-p1-imported-mesh-structural/structural-vulkan-release.bmp`

### Focused regression selection

The affected regression set passed without running the unrelated global matrix:

- ordinary JericoGltfCook source-immutable/deterministic integration;
- CookedDirectoryVerifier positive/negative suite;
- `runtime_formats.rmesh`, `runtime_formats.rcoll` and `physics.world_jolt`;
- repository cancelled-VDB absence and GPU geology boundary;
- `git diff --check` for owned implementation/test paths.

The global configure remains blocked by pre-existing unrelated editor scheduling,
Terrain/Diligent and `tomlplusplus` target issues. Those failures were not changed
or represented as evidence for this slice.

## Data lineage

```text
immutable glTF + buffers + canonical material template
-> existing CanonicalModel import
-> existing flattened MeshWriteRequest copy
-> selected planar displacement + reconstructed directions/bounds
-> preserved topology/material/UV/optional GEOL lineage
-> JRMS static mesh + exact static triangle collision
-> existing manifest/atomic publish/package
-> existing static renderer and Jolt/runtime-format consumers
-> source-free Vulkan visual result
```

The `.rcoll` intentionally carries geometry identity rather than a dependency on
the `.rmesh`: collision format ownership already stores the complete exact static
triangle payload and its stable collision GUID. No new runtime coupling was
invented.

## End-to-end system completeness audit

| Category | Result |
|---|---|
| Source/input data | `PASS`: immutable glTF/buffers and bounded in-range reimport mutation. |
| Asset pipeline | `PASS`: existing canonical importer/cooker, atomic publish and package. |
| Editor/authoring workflow | CLI/data contract proved; preview/diff/undo UI remains `INTEGRATION REQUIRED`. |
| ECS/runtime representation | `PASS`: existing static mesh/collision assets; no geology-specific runtime component. |
| CPU processing | `PASS`: deterministic bounded direct-mesh operation and stable errors. |
| GPU processing | `NOT APPLICABLE`: the selected operator is deliberately offline CPU cook. |
| Renderer/shaders/materials | `PASS`: existing Vulkan static PBR path, four textures, zero fallback. |
| Physics/collision | Exact triangle identity `PASS`; physical geology properties remain deferred. |
| Animation/Sequencer | `NOT APPLICABLE` to this static cook. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for a later admitted runtime query/change system. |
| Serialization | `PASS`: existing JRMS/JRCL/package formats, no new schema. |
| Save/load | Static cooked load `PASS`; mutable geology state remains deferred. |
| Streaming/LOD | Base static package `PASS`; classified LOD/world-cell ownership remains deferred. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; explicitly outside this direct-mesh static boundary. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` under later bounded consumers. |
| Debugging/visualization | Stable failures plus inspected non-empty package-only Vulkan frame `PASS`. |
| Profiling/performance budgets | Explicit 64M product ceiling and per-request work accounting `PASS`; no broad performance claim. |
| Tooling/UI | Existing cooker CLI `PASS`; dedicated semantic editor UI intentionally absent. |
| Testing/failure handling | Unit, negative, real reimport, collision, package and runtime gates `PASS`. |
| Documentation | Research, design, plan, closure and repeated audit match implementation. |

## Explicit limits and `INTEGRATION REQUIRED`

This slice closes the P1 operator list, not the complete Geology blueprint.
Remaining work retains its existing owners and must not be reconstructed from the
static mesh:

1. field conversion/classification synthesis for imported sources when a later
   slice demonstrates a real consumer;
2. topology-changing Boolean/remesh/retopology and robust fracture ownership;
3. runtime destruction, mutable collision generations and save/load identity;
4. physical density/mass/resistance/break properties for Jolt;
5. classified LOD/world-cell streaming and semantic editor visualization;
6. navigation, gameplay/Luau, result events, particles and audio seams;
7. all ordered P2/research features beginning with the SpeedRock-like block
   grammar source;
8. the later joint Geology/PWAS/EPLS/animation/destruction production expansion,
   which remains forbidden until the full Geology blueprint is published and
   closed.

No Rockfall or Colluvium implementation was reopened. No VDB/UI restoration,
bespoke generator/preset, parallel system or reference implementation was added.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
