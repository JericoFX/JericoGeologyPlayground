# Jolt Static Mesh and Marching Cubes Source Record

Date: 2026-08-02

This record fixes the primary sources and boundaries used by the M7B Sparse
SDF house vertical. It is not evidence that M7B is implemented.

## Jolt static triangle collision

- Official repository: `https://github.com/jrouwe/JoltPhysics`
- Jerico pin: `e77f175595e64cb44218cc9d9d56fc365ad0e36a`
- Primary file:
  `Jolt/Physics/Collision/Shape/MeshShape.h`
- Pinned URL:
  `https://raw.githubusercontent.com/jrouwe/JoltPhysics/e77f175595e64cb44218cc9d9d56fc365ad0e36a/Jolt/Physics/Collision/Shape/MeshShape.h`
- Retrieved UTF-8 SHA-256:
  `41e74234697e00fdde1c059265eccfa869ee95d09612d93fa8a07ef482bb390c`
- Documentation lookup: Context7 library `/jrouwe/joltphysics` and the alternate
  indexed-document source were queried before planning the adapter change. The
  exact pinned official source above is the final API authority.

The pinned header establishes the required integration contract:

- `MeshShapeSettings(VertexList, IndexedTriangleList)` is the indexed input
  path and owns the passed containers.
- construction sanitizes duplicate and degenerate triangles;
- triangle winding is counter-clockwise and simulation triangles are
  single-sided;
- `MeshShapeSettings::Create()` returns a `ShapeResult`, which Jerico must
  inspect before creating a body;
- `MeshShape::MustBeStatic()` returns true;
- default `EBuildQuality` is `FavorRuntimePerformance`;
- active-edge discovery exists to avoid treating every internal triangle edge
  as a collision feature.

Jerico consequently admits cooked static triangle collision only for static,
unit-scale bodies. `.rcoll` remains Jerico-owned and never serializes Jolt
types or binary state.

## Marching Cubes lookup data

- Official repository: `https://github.com/Kitware/VTK`
- Exact source commit: `b12ffd10bb914e046b51cbb787b780a122d9faf7`
- Primary file:
  `Common/DataModel/vtkMarchingCellsContourCases.cxx`
- Pinned URL:
  `https://raw.githubusercontent.com/Kitware/VTK/b12ffd10bb914e046b51cbb787b780a122d9faf7/Common/DataModel/vtkMarchingCellsContourCases.cxx`
- Retrieved UTF-8 SHA-256:
  `17e125fa454fb6b29a27854808022d36dd5066d827b5567d7b96be079c5c2992`
- License file:
  `https://raw.githubusercontent.com/Kitware/VTK/b12ffd10bb914e046b51cbb787b780a122d9faf7/Copyright.txt`
- Retrieved UTF-8 license SHA-256:
  `11232448be82e0ea2c2c66219c2e36389f42249894070ee10c549ff182fc08b6`
- SPDX identifier in the source: `BSD-3-Clause`
- Retained notice: `ThirdParty/Licenses/VTK-BSD-3-Clause.txt`

Only the fixed 256 hexahedron contour cases and their 12 edge pairs may be
adapted into a private Jerico lookup header. Jerico does not fetch, build, link,
or expose VTK. Task 7 must add a repository contract that locks the source
commit, SPDX line, retained license, table hash, and absence of a VTK target
before the table can be committed.

## Boundary decision

Jolt is the existing private runtime physics dependency. The VTK table is
licensed source data used to avoid inventing a topology table; VTK itself is
not a Jerico dependency. Sparse fields, meshing, transitions, recipes, Qt,
toml++, and Manifold remain authoring-only. Shipping consumes only the cooked
`.rmesh` and `.rcoll` result.
