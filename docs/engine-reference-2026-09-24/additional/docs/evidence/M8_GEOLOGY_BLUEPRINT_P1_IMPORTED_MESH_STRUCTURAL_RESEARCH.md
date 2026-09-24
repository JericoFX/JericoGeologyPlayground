# M8 Imported-Mesh Structural Operators Research

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P1-IMPORTED-MESH-STRUCTURAL-OPERATORS`
**Decision:** `DIRECT_MESH_OFFLINE_STATIC_PLANAR_DISPLACEMENT`

## Question

What is the smallest coherent Jerico-owned boundary that applies a useful
structural operation to real imported static geometry while preserving source,
identity, topology, materials, classification when present, collision lineage,
determinism, cancellation and bounded work?

## Current Jerico

The admitted path is:

```text
immutable glTF/GLB source
-> fastgltf-backed CanonicalModel
-> JericoGltfCook world-transform flattening
-> MeshWriteRequest
-> deterministic .rmesh + manifest
-> Asset Registry / static renderer
```

The root GUID remains the mesh GUID. Material and texture identities are derived
from that root and stable logical keys. Reimport preserves the root identity and
publishes through the existing preview/prepare/replace/undo-redo transaction.

The productive geology field path has no imported-mesh source. It plans topology
from `BspDocument`, materializes convex BSP brush triangles and planes, evaluates
the resident field, extracts Surface Nets and cooks classified `.rmesh` plus exact
static `.rcoll`. Adding an imported-mesh field source would therefore be a new
representation bridge, not reuse of an already-cheap bridge.

## Primary sources

1. The [Khronos glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html)
   defines a mesh as primitives containing vertex attributes, optional indices
   and an optional material. It requires triangle index cardinality and bounds,
   defines normals as unit vectors and defines the bitangent from normal,
   tangent and tangent handedness. Jerico can preserve primitive/material
   ownership and index topology while updating only positions and the affected
   direction basis.
2. The official [OpenVDB MeshToVolume reference](https://www.openvdb.org/documentation/doxygen/MeshToVolume_8h.html)
   shows that mesh-to-field conversion is a distinct conversion with explicit
   world/grid transform, interior/exterior band widths and interruption. Its
   signed level-set conversion requires a closed surface. This is useful design
   evidence, not permission to restore OpenVDB or copy its implementation.
3. Jacobson, Kavan and Sorkine-Hornung's primary
   [generalized winding-number publication](https://users.cs.utah.edu/~ladislav/jacobson13robust/jacobson13robust.html)
   demonstrates why inside/outside classification for imperfect polygon meshes
   is a separate robustness problem. A field bridge would need an explicit sign
   policy rather than an incidental ray test.
4. The official [CGAL Polygon Mesh Processing manual](https://doc.cgal.org/latest/Polygon_mesh_processing/)
   and its [corefinement/Boolean contract](https://doc.cgal.org/latest/Polygon_mesh_processing/group__PMP__corefinement__grp.html)
   show that topology-changing Boolean work requires triangulated inputs,
   self-intersection preconditions, manifold output constraints and robust
   predicates/constructions. A local floating-point cut is not an acceptable
   substitute.

## Boundary comparison

### Field conversion

Benefits:

- immediate reuse of existing geology layers, classification and GeologyCook;
- natural weathering, cracks and field-space masks.

Costs that do not exist in Jerico today:

- canonical closed/open source policy and inside/outside semantics;
- imported-mesh topology planning and field allocation;
- mesh-to-field source evaluation and GPU publication;
- material-to-geology classification transfer;
- error bounds for voxel resolution and later Surface Nets reconstruction.

The blueprint explicitly allows imported-mesh field conversion to remain
deferred when the bridge is not already cheap. This bridge is not cheap.

### Direct topology-changing cut/crack

A robust Boolean or remesh path would require a newly admitted geometry backend
or a substantially larger independently implemented exact-topology system.
Neither is authorized by this slice.

### Direct topology-preserving displacement

A bounded planar displacement can operate on the already flattened
`MeshWriteRequest` immediately before serialization. It can:

- leave source bytes and `CanonicalModel` untouched;
- preserve vertex/index cardinality, index order, submesh ranges, material GUIDs,
  UVs and optional per-vertex geology classification;
- update only positions, affected normals/tangents, bounds and visibility;
- reuse the existing `.rmesh`, `.rcoll`, manifest, reader and runtime consumers;
- derive cook identity from canonical operator parameters;
- support cancellation and a checked work ceiling without runtime mutation.

## Independent Jerico recommendation

Admit one offline/static operator:

```text
PlanarSurfaceDisplacement
```

It selects one flattened submesh by ordinal plus expected material GUID, computes
a deterministic smooth signed-plane weight for every selected vertex and moves
that vertex by `direction * amplitude * weight`. It rejects stale material
identity, shared selected/unselected vertex ownership, non-finite or degenerate
parameters, non-triangle topology, excessive work and cancellation.

After displacement it derives area-weighted normals and orthogonal finite
tangents for affected vertices, preserves tangent handedness, then regenerates
bounds and visibility through the existing helpers. It writes one exact static
triangle collision from the final vertex/index arrays through the existing
`CollisionAsset` writer. No source, importer, runtime format, renderer, material,
physics backend, graph, field or remesher is added.

The proof input is the Khronos BoomBox glTF fixture already admitted in Jerico.
The proof must demonstrate a non-zero structural result rather than a debug mask,
repeat byte identity, stable root/material/collision GUIDs, unchanged topology,
material and optional classification lineage, exact render/collision triangle
agreement, cancellation without publication and bounded work.

## Explicit deferrals

The following remain `INTEGRATION REQUIRED`:

- mesh-to-field conversion and geological material classification;
- robust cuts, cracks, remeshing, retopology and topology repair;
- runtime deformation/destruction, dirty remesh, collision replacement and
  save/load;
- editor controls beyond the existing generic cooker/process transaction;
- LOD/world-cell propagation and runtime result events for gameplay, Luau,
  navigation, particles and audio.

No source above defines Jerico's API, recipe identity or implementation. Jerico
uses the sources only to bound the problem and implements the selected operation
independently.
