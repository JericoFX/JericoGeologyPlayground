# M8 Geology Blueprint P0 Proof Recipes — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-PROOF-RECIPES`
**Recipe contract:** existing cumulative schema 12; no schema bump
**Backend:** productive Diligent/Vulkan plus existing CPU references

## Purpose and boundary

Proof Recipes demonstrate that admitted P0 geology operators compose into
recognizable authored results. A recipe is canonical data, not a bespoke C++
generator. The proof reuses recipe serialization/hash, the productive GPU
session, bounded inspection, Surface Nets extraction, mesh statistics,
materials and existing evidence helpers.

This slice does not implement `FoldDomain`, `ImplicitBlock`, columnar jointing,
P1/P2, VDB, a geology editor UI, new operators, mineral simulation, M9 or
Task23. The three feasible P0 recipes are A, B and E from blueprint Appendix C;
the folded block and columnar recipes remain gated by P1 operators.

## Canonical common source

All three recipes start from the same single `BoxBrush` document:

- half extents: `(2.4, 1.7, 1.25) m`;
- document/source hash: `17187918773788934726`;
- quality voxel size: `0.12 m`;
- negative inside, positive outside;
- extracted triangle material: the canonical source document GUID.

Changing recipe order, any semantic parameter or a full-width seed changes the
canonical recipe hash. Serialization, parse and reserialization reproduce the
exact same bytes.

## Recipe A — Sedimentary cliff

Canonical artifact:
`M8_GEOLOGY_BLUEPRINT_P0_PROOF_RECIPE_A.geology.toml`

Ordered stages:

1. `Source Box` — `Source`.
2. `Primary Cliff Plane` — `PlaneCut`, seed `18a09e667f3bcc91`.
3. `Sedimentary Strata` — `Strata`, seed `27d4eb2f165667c5`,
   spacing `0.44 m`.
4. `Secondary Cliff Plane` — `PlaneCut`, seed `94d049bb133111eb`.
5. `Bedding Fault` — `FaultDisplace`, seed `d2b74407b1ce6e93`,
   displacement `0.28 m`.
6. `Cliff Cavity` — `CavityCarve`, radius `0.42 m`.
7. `Cliff Bubble Weathering` — `SurfaceWeathering/Bubble`, two
   iterations.
8. `Cliff Ridged Detail` — `Noise/Ridged`, seed `d1b54a32d192ed8b`,
   amplitude `0.07 m`.
9. `Cliff FBM Detail` — `Noise/FBM`, seed `d1b54a32d192ed8a`,
   amplitude `0.045 m`.

## Recipe B — Fractured boulder

Canonical artifact:
`M8_GEOLOGY_BLUEPRINT_P0_PROOF_RECIPE_B.geology.toml`

Ordered stages:

1. `Source Box` — `Source`.
2. `Boulder Directional Warp` — `DirectionalWarp`, seed
   `9e3779b97f4a7c15`, amplitude `0.36 m`.
3. `Boulder Plane A` — `PlaneCut`, seed `243f6a8885a308d3`.
4. `Boulder Plane B` — `PlaneCut`, seed `13198a2e03707344`.
5. `Boulder Structural Cells` — `AnisotropicVoronoiCells`, seed
   `a4093822299f31d0`, six authored anisotropic cells.
6. `Boulder Cell Fractures` — `FractureCells/Carve`, width `0.20 m`,
   softness `0.08 m`.
7. `Boulder Cavity` — `CavityCarve`, radius `0.38 m`.
8. `Boulder Bubble Weathering` — `SurfaceWeathering/Bubble`, two
   iterations.
9. `Boulder Ridged Detail` — `Noise/Ridged`, seed `d1b54a32d192edaa`,
   amplitude `0.065 m`.
10. `Boulder FBM Detail` — `Noise/FBM`, seed `d1b54a32d192eda9`,
    amplitude `0.04 m`.

## Recipe E — Weathered rounded boulder

Canonical artifact:
`M8_GEOLOGY_BLUEPRINT_P0_PROOF_RECIPE_E.geology.toml`

Ordered stages:

1. `Source Box` — `Source`.
2. `Rounded Boulder Directional Warp` — `DirectionalWarp`, seed
   `082efa98ec4e6c89`, amplitude `0.52 m`.
3. `Rounded Boulder Bubble Weathering` — `SurfaceWeathering/Bubble`,
   four iterations.
4. `Rounded Boulder Curvature Weathering` —
   `SurfaceWeathering/Curvature/Convex`, two iterations.
5. `Rounded Boulder Sparse Pit` — masked `CavityCarve`, radius `0.30 m`;
   its admitted `RBrushLocalField` mask is `3.8 x 3.0 x 2.6 m`.
6. `Rounded Boulder FBM Detail` — `Noise/FBM`, seed
   `d1b54a32d192edc9`, amplitude `0.045 m`.

Schema 12 is cumulative: a `SurfaceWeathering` layer uses its existing `model`
discriminant and may encode Bubble or Curvature. Schema 11 remains
Bubble-only; schemas 1 through 10 retain the legacy model. This composition
uses no new field format and does not reinterpret existing canonical bytes.

## Detail OFF and structural proof

`Detail OFF` changes only every `Noise` layer's `enabled` flag to false. It
does not remove or alter Source, PlaneCut, Strata, FaultDisplace,
DirectionalWarp, AnisotropicVoronoiCells, FractureCells, CavityCarve,
SurfaceWeathering or masks.

The productive proof checks the last macro/meso stage in both executions at a
bounded `8 x 8` world-space sample grid on `z = 0`. All 64 signed-distance and
structural-mask samples are exactly equal before Detail. The full and
Detail-OFF final recipe, field and mesh hashes differ as expected, while A, B
and E remain pairwise distinct with Detail disabled.

Field hashes cover the evaluated dense plan and therefore are recorded rather
than treated as interchangeable across recipes with different enabled-layer
plans. Selected world-space samples freeze the exact pre-Detail scalar
contract.

## Surface and evidence contract

- One productive Vulkan session evaluates all variants.
- Repeating identical A reports the same final hash and zero dirty leaves and
  voxels.
- B publishes non-zero AnisotropicVoronoi and Fracture structural hashes and
  positive fracture-mask samples.
- Surface Nets must produce finite positions/normals, valid indices, more than
  100 vertices and triangles, non-empty bounds, document material attribution
  and bounded device/readback accounting.
- The evidence SVG is generated directly from the extracted vertex/index
  arrays. No custom geometry generator participates.

JericoEditor currently has no admitted `GeologySurfaceMesh` or
`.geology.toml` viewport route. Its capture contract accepts a cooked scene,
physical prefab, material preview or JericoBSP. Producing an editor screenshot
would require restoring or adding geology UI, which is deliberately outside
this slice; the Vulkan mesh plate is the honest visual proof.
