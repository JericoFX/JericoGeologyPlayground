# JericoEngine — Mix Damage, Laces Generator, Fence/Gate/Railing/Wall, AgedFX and Procedural Building Generator V2

**Date:** 2026-09-20  
**Milestone ownership:** M8 research input only.  
**Operational status:** documentation/research; does **not** change `docs/work/ACTIVE_SLICE.md`, which is currently M9.  
**Product rule:** purchased Blender add-ons are research references only. Jerico Shipping must remain source-free from these packages and Blender-free.

## 1. Scope and inspection method

The five owner-supplied packages were inspected statically. ZIP/RAR contents, Python orchestration where present, Blender datablocks, Geometry Nodes / shader node-tree interfaces, node categories, references and asset-role naming were examined. A local read-only Blender-file DNA parser was used to inventory `.blend` structures without registering or executing the add-ons.

This analysis intentionally separates:

1. **observable capability / useful mathematics**;
2. **Blender UI and orchestration**, which is not a Jerico runtime requirement;
3. **purchased presets, meshes, textures and node graphs**, which are not copied into Jerico;
4. **Jerico-native abstractions** that can later be independently implemented and tested.

No visual-equivalence claim is made. Blender was not executed for this pass, graphs were not evaluated/rendered, and the bundled PBG2 PDF was not required for the technical conclusions below. Exact third-party licensing was not established from all packages, therefore the conservative boundary is reference-only.

## 2. Supplied archive identity

| Package | Bytes | SHA-256 |
| --- | ---: | --- |
| Mix Damage V1.2.zip | 25,891,969 | `efe8e1f4ba741cc05b7c3c0802f363d9bf1dfb61f3496c15a90f4ed747ced0d0` |
| Laces Generator 2.0.rar | 1,846,805 | `8b4f45ee70eb49654184385a2be11ea2e57a3a98eb834138e8cccff021ad7482` |
| Fence (Gate) Railing___Wall_Generator_v1.1.rar | 186,758,923 | `5468f3f791bcf66febb5bbeeeb3fe166c45aa22378b24cc37a82c20f292e8dec` |
| AgedFX_v1.22.zip | 9,916,747 | `f84f39696fa7cdb28cb7f54fdfb87150b76b4acb760f0645d5e8751b5487ba4b` |
| Procedural_Building_Generator_V2.zip | 234,828,124 | `4b0ada65491a15269b2cd5e40635dcfe4343a5c88f15ff3b274b4fa51321becb` |

Static `.blend` inventory:

| Source | Blender file version | Node trees | Objects | Materials | Collections |
| --- | ---: | ---: | ---: | ---: | ---: |
| LacesGenerator2.0.blend | 4.02 | 45 | 10 | 4 | 2 |
| AgedFX_1_2.blend | 4.01 | 8 | 30 | 8 | 10 |
| Fence, Gate, Railing & Wall Generator.blend | 4.05 | 59 | 252 | 225 | 1 |
| PBG2.blend | 3.06 | 68 | 18 | 7 | 3 |
| PBG2 historic_styles_01.blend | 3.06 | 12 | 151 | 19 | 5 |
| PBG2 base_styles.blend | 3.06 | 10 | 272 | 28 | 6 |
| Mix Damage assets/19.blend | 4.00 | 40 | 1 | 11 | 1 |
| Mix Damage assets/Material.blend | 4.00 | 16 | 18 | 16 | 1 |

## 3. Architectural conclusion

These references do **not** justify five new Jerico subsystems. They reinforce reusable services that already fit Jerico's architecture:

```text
Procedural Studio
  ├─ DamageOperator / DamageField
  ├─ Path Assembly / Profile Sweep / Curve Weave
  └─ Semantic Surface Assembly / Facade Tiler

JericoBSP / Terrain / authored meshes
  └─ provide structural host geometry and spatial queries

EPLS
  └─ owns environmental/surface state: dust, dirt, moss state, scratches, wear...

PWAS
  └─ owns deterministic placement/distribution of physical instances

JericoParticles / Volumetrics
  └─ airborne dust / fog presentation

Destruction + Jolt
  └─ runtime bonds, chunks, physical activation and gameplay destruction
```

`Procedural Studio` constructs reusable geometry/assemblies. `PWAS` distributes physical world objects. `EPLS` decides environmental surface state. BSP remains structural architecture. Destruction remains the physical/runtime authority. The add-ons must plug into those owners rather than create competing representations.

---

# 4. Mix Damage V1.2

## What the package actually contains

The Python layer is mostly Blender operator/UI/orchestration around an external Geometry Nodes asset. The primary graph is `DamagePro2`.

`DamagePro2` contains **591 nodes / 786 links** in the inspected main tree. Important node families include:

- Math / Vector Math / Map Range / Compare;
- Noise Texture and deterministic random values;
- Set Position / Extrude Mesh / Delete Geometry;
- Curve to Mesh / Mesh to Curve;
- Instance on Points / Realize Instances;
- **Mesh to Volume / Volume to Mesh**;
- **Mesh Boolean**;
- internal damage, cracks, steel/grid, brick/rock and mixed surface-pattern branches.

Exposed controls include two surface damage layers (`Texture`, `Scale`, `Power`, `Quality`, `Angle`, `Mix`) plus independent controls for internal damage, cracks, steel/grid and brick/rock families with scale/power/quality/count/offset/seed-style parameters.

## Useful abstraction for Jerico

Do not reproduce `DamagePro2`. Extract a generic authoring chain:

```text
DamageRegion / Selection
    ↓
DamageField = base shape + distance + noise + directional/angle terms
    ↓
Representation policy
    ├─ material/decal mask
    ├─ normal/displacement bake
    ├─ local SDF / boolean silhouette damage
    ├─ authored cavity / crack sweep
    └─ prefractured semantic variant / physical chunks
```

This should reuse the existing M8 direction from Damage Control and Infinite Fracture instead of adding another damage architecture.

## Jerico placement

- **Procedural Studio:** `DamageOperator`, reusable damage recipes, deterministic seed streams.
- **Destruction:** physical chunks, bonds, anchors, break states and Jolt activation.
- **EPLS:** cheap persistent surface wear/crack/damage masks where silhouette does not need to change.
- **RBrush/Decal/Material:** paint/local masks and visual-only cracks.
- **PWAS:** only debris/rebar/physical fragments that are actual placed objects.
- **Cook:** bake high-cost authoring results whenever dynamic recomputation is unnecessary.

## What not to port

- the 591-node topology;
- Blender modifier/operator lifecycle;
- every artistic socket as a 1:1 Jerico setting;
- full volume + boolean reconstruction every runtime frame.

The heavy volume/boolean path is an authoring/cook capability first. Runtime destruction should consume cooked semantic chunks, compact fields or bounded local operations.

## Acceptance targets later

- deterministic damage for seed + inputs;
- stable semantic IDs before/after damage;
- explicit fresh-interior mask;
- bounded geometric deviation outside the damage region;
- tiered equivalence test: material-only → displaced → silhouette/SDF → physical chunks;
- cook/collision/nav tests after topology-changing damage.

---

# 5. Laces Generator 2.0

## What the package actually teaches

Although presented as a shoelace generator, its useful core is a **general ordered-anchor curve weaving system**.

The main `LacesGenerator` tree is very large (**2,852 nodes / 3,707 links**). Helper graphs reveal clearer reusable operations:

- `FlatRibbon`: curve/profile sweep, thickness, seam orientation, UV and endpoint accessories;
- `RoundRibbon`: round cable/rope profile generation;
- `Twist`: vector rotation around the path tangent, modulated by path/border position;
- `Chaos`: local lateral/depth variation with masks and smoothing;
- eyelet sorting / left-right ordering;
- endpoint spawning;
- tied/untied guide deformation;
- UV capture/storage;
- seam, bevel, profile and loop helpers.

Main controls expose profile type, lacing style, master size, width/thickness, subdivision/quality, eyelets/aglets, custom profile, seam/direction flips, under/over start, weave strength, twist, depth offset, chaos and accessory-specific offsets.

## Useful abstraction for Jerico

Generalize this to:

```text
AnchorSequence
  + canonical path / guide
  + PathFrame (tangent + stable normal/binormal)
  + WeavePolicy (left/right, parity, under/over, crossing depth)
  + ProfileSweep (round / ribbon / custom)
  + Twist / local noise
  + EndpointAccessory policy
  -> semantic curve assembly
```

A robust Jerico implementation should use the canonical spline/path representation and rotation-minimizing frames rather than a lace-specific path type.

## Jerico placement

- **Procedural Studio / Path Assembly:** primary owner.
- **Cable/rope/web authoring:** wires through rings, mine cables, lacing, straps, stitched seams, bindings, hoses and chained anchor runs.
- **PWAS:** world distribution of finished assemblies when many copies are needed; not the curve construction authority.
- **Physics:** optional downstream rope/cable simulation only when gameplay requires it; generation is not a physics solver.
- **Cook:** default path for static props.

## Data worth preserving

`PathU`, `anchorIndex`, left/right side, crossing index, under/over state, profile/material role, endpoint role and deterministic seed. Those semantics make later selection, damage, UV and gameplay attachment far more useful than a baked anonymous mesh.

## What not to port

- literal shoe-specific UI;
- all accessory presets;
- giant node topology;
- runtime deformation for objects that can be cooked once.

---

# 6. Fence / Gate / Railing / Wall Generator 1.1

## What the package actually teaches

This is a broad **path-driven architectural assembly** system, not merely a fence scatterer.

The inspected `.blend` has 59 node trees, 252 objects and 225 materials. The main generator exposes a very large interface (hundreds of interface records) covering:

- posts, spacing, width/height, bevels;
- bases, collars, caps and finials;
- rails / handrails;
- wooden and metal pickets;
- balusters;
- lattice / panels / glass / walls;
- gates and gate count/width/position/opening;
- hinges and supports;
- stepped/high-low layouts;
- slope-aware top/bottom cutting;
- laser-cut pattern masks;
- per-part materials.

A major helper tree uses `Curve to Mesh`, `Instance on Points`, `Resample Curve`, `Raycast`, `Set Position`, indexed sampling and edge-angle/attribute logic. A laser-cut helper uses a procedural scalar mask based on Voronoi/mapping operations.

## Useful abstraction for Jerico

This should **extend the same generic Path Assembly backend already motivated by Stair Builder**, not create a Fence subsystem.

```text
CanonicalPath
  ↓ arc-length table + stable frame
Span segmentation
  ├─ ordinary spans
  ├─ gate spans
  └─ endpoints/corners
Module distribution
  ├─ posts
  ├─ rails/handrails
  ├─ pickets/balusters/panels
  └─ caps/finials/accessories
Host adaptation
  ├─ terrain ray/sample
  ├─ slope trim
  └─ step/high-low policy
Semantic output
  └─ Post / Rail / Panel / Gate / Hinge / Wall / etc.
```

Reusable math includes arc-length distribution, fit/remainder policies, rotation-minimizing path frames, terrain/slope adaptation, semantic span splitting, profile sweep and deterministic variants.

## Jerico placement

- **Procedural Studio / Path Assembly:** generator authority.
- **JericoBSP / Scene Editor:** author paths around structural spaces and use walls/fences/railings as architectural assemblies.
- **Terrain:** height/normal query host for ground adaptation.
- **PWAS:** distribution of reusable preset assemblies and secondary props after construction.
- **Gameplay/Physics:** `Gate` must emerge as a distinct semantic entity/socket; hinge/door physics is downstream, not buried in geometry generation.
- **Navigation:** generated gate/opening semantics can feed nav portal/obstacle state.
- **Materials/EPLS:** semantic part materials first, environmental aging later.

## Important new value beyond Stair Builder

- explicit **gate-span segmentation**;
- slope trim and stepped-span policies;
- richer repeated subassemblies;
- caps/finials/profile families;
- panel/laser-cut mask concept;
- semantic handoff to hinge/gameplay/nav.

## What not to port

Do not expose 800+ Blender-facing interface entries 1:1. Jerico needs a compact typed schema with expandable style recipes. Likewise, the supplied 60 pattern images and authored preset objects are reference assets, not repository content.

---

# 7. AgedFX 1.22

## What the package actually teaches

AgedFX is two things mixed together:

1. **surface/material aging fields**;
2. **physical/environmental decorators** such as moss, cobwebs, particles and fog.

Important shader groups:

### Dust

Uses noise, Layer Weight/view-angle information, geometry/texture coordinates, ramps and mapping. Controls include dust scale, top ratio and viewing-angle variation.

### Dirt

Uses mapped noise plus geometry/normal/bevel information to combine general dirt and edge dirt, with edge spread.

### Deformations

Uses Noise + Voronoi + ramps + Bump to synthesize scratches and bumps.

Important geometry groups:

### Moss Generator

Deterministic point distribution / instancing with density, viewport density, growth regularity/scale, seed, mask/vertex-group use, diversity, scale range, rotation and animation controls.

### Dust Particles

Uses time, random values, instancing, position offsets and noise for animated particulate presentation.

## Correct Jerico ownership

### EPLS — primary surface-state owner

- dust accumulation;
- general/edge dirt;
- scratches and bumps;
- moss likelihood/state;
- wet/aged environmental surface history when combined with existing EPLS fields.

Useful independent equations/fields:

```text
TopExposure     = saturate(dot(N, WorldUp) remap)
EdgeFeature     = curvature / convexity / edge-distance field
DirtMask        = f(EdgeFeature, cavity/exposure, noise, age)
DustMask        = f(TopExposure, occlusion/exposure, noise, age)
ScratchMask     = anisotropic/cellular field × authored mask
GrowthProbability = density × regularityField × hostMask × environmentState
```

Jerico should **not** store camera/view-angle dependence in the physical/environmental state. AgedFX's Layer Weight behavior is useful as shading reference, but accumulation itself should be world/environment based; view-dependent response belongs in the material shader.

### PWAS — physical decorators only

- physical moss clumps/strands;
- cobweb meshes;
- debris or other instantiated dressing requested by EPLS/world rules.

### JericoParticles / Volumetrics

- airborne dust particles;
- fog/volumetric presentation.

### Material Graph / Bake

Shader-only dust/dirt/scratch effects should remain procedural while authoring and be bakeable to compact masks when runtime evaluation does not justify its cost.

## What not to port

- Blender-wide “apply to every material” operator behavior;
- scene collection lifecycle;
- view-dependent data as environmental truth;
- permanent live geometry for effects that can be masks or baked textures.

---

# 8. Procedural Building Generator V2

## What the package actually teaches

PBG2 is the strongest reference here for a **semantic modular building assembler**.

The main `Simple Building` graph delegates most work to helper groups and exposes a compact high-level contract:

- input guide geometry;
- `Style` collection;
- deterministic seed;
- wall alignment;
- module scale and roof scale/threshold;
- floor/ceiling policies;
- beams/pillars;
- ground collision object;
- wall/roof greeble density/orientation;
- door placement and rarity;
- optional fixed wall/roof type indices;
- deterministic rotation variation;
- semantic material/color roles.

`Two-Part Building` adds a height threshold and allows style changes between lower/upper regions.

### Geometry preparation

`Prepare geometry` classifies and separates wall, roof, internal/corner beam, roof ridge, floor and flat-roof roles using normals, topology and stored attributes.

### Best-fit modular tiling

`Smart rectangle fill` is a large graph dedicated to best-fit arrays, grid sizing, removing tiny/invisible faces, placement adjustment and wide/low-wall cases. `Smart triangle fill` and triangle subdivision cover non-quad regions.

### Style libraries

The included style files are organized as collections such as Half-timbered, Pueblo, Swedish cottage, Wooden shed, Minka, Stone, Timber and Viking. More important than their art content is their **role vocabulary**: door, half wall, pillar, ridge cap, roof greeble, square/triangle floor/roof/wall, thick/thin beam, wall greeble, wide wall, etc.

That is evidence for a role-based `StyleSet` / `VariantFamily`, not a reason to ship the purchased meshes.

## Useful abstraction for Jerico

```text
Structural host (BSP / authored shell / imported mesh)
    ↓
Surface + edge semantic classification
    ├─ Wall
    ├─ Roof
    ├─ Floor
    ├─ Ridge
    ├─ Convex/concave vertical edge
    └─ Opening / Door candidate
    ↓
FaceLocalFrame + measurable dimensions
    ↓
BestFitTiler
    ├─ rectangle modules
    ├─ triangle modules
    └─ remainder / scaling policy
    ↓
StyleSet / VariantFamily resolver
    ↓
semantic instances + deterministic IDs
    ↓
optional greebles / doors / beams / pillars
```

Shared data should include:

- `SurfaceRole`;
- `EdgeRole`;
- `ModuleRole`;
- `FaceLocalFrame`;
- `StyleSet`;
- `VariantFamily`;
- `ProceduralElementId`;
- module-space UV / local coordinates;
- source face/region lineage.

## Jerico placement

- **JericoBSP:** remains structural authority for playable architectural space.
- **Procedural Studio:** consumes BSP/guide surfaces and builds modular facade/roof/floor assemblies.
- **PWAS:** places secondary physical props/greebles or distributes completed building assemblies; does not become the building grammar.
- **Material system:** resolves semantic material slots/palettes.
- **EPLS:** applies environmental aging after the semantic building exists.
- **Doors:** generate actual door/opening semantic entities for gameplay, save/load and navigation rather than baking them into anonymous geometry.
- **Physics/Nav:** cook collision and nav from accepted final geometry/semantic openings.

## Best-fit math direction

Jerico does not need to copy the node network. An independent tiler can use measurable face-local dimensions and a policy such as:

```text
count = max(1, choose_count(faceExtent / targetModuleExtent, policy))
cellExtent = faceExtent / count
scale = cellExtent / sourceModuleExtent
```

with explicit remainder, minimum-size, triangle and boundary policies. Orientation comes from a stable face-local basis derived from normal + chosen longitudinal edge/axis. Variant selection is seeded from stable element identity, not global RNG order.

## What not to port

- the purchased historical/base style meshes or textures;
- style names as hard-coded engine behavior;
- Blender collection semantics as runtime architecture;
- full generator evaluation at runtime for static buildings.

---

# 9. Shared services revealed by all five references

Before implementing any addon-derived capability, audit existing Jerico services and converge on shared primitives:

```text
CanonicalPath + arc-length table
RotationMinimizingFrame / stable PathFrame
ProfileSweep / CurveToMesh equivalent
AnchorSequence + ordinal/parity policies
Deterministic staged hashing / ProceduralElementId
VariantFamily / StyleSet resolver
FaceLocalFrame
SurfaceRole / EdgeRole / ModuleRole
BestFitRectTiler + TriangleTiler
MeshSpatialQuery / raycast / closest projection
SurfaceFeatureField (curvature, convexity, cavity, edge distance, exposure)
DamageField
Semantic masks / lineage streams
Deterministic surface/volume candidate generation
PWAS placement adapter
EPLS state adapter
MaterialBakeIR / mask bake
Cook provenance and rebuild recipe
```

The strongest cross-reference opportunities are:

- **Fence + Stair Builder + Laces:** one path/frame/distribution/sweep backend.
- **PBG2 + Fence:** semantic architectural assemblies with explicit parts/openings.
- **Mix Damage + existing Infinite Fracture/Damage Control:** one `DamageField` and tiered representation policy.
- **AgedFX + Damage Control + Smart Segmentation:** one surface-feature backend for grime, wear, decals, damage masks and segmentation hints.
- **PBG2 + EPLS:** semantic building regions make aging materially smarter (roof exposure, wall orientation, lower-wall damp/dirt, metal/wood roles).
- **Fence/PBG2 + navigation:** openings and gates remain semantic rather than inferred from final triangles.

---

# 10. Recommended implementation order when M8 becomes legal

This is a dependency order, **not authorization to start while M9 is active**.

1. **Shared semantic/path infrastructure** already demanded by existing M8 research.
2. **Fence/Path Assembly extensions:** gate span, slope adaptation, stepped spans, caps/finials.
3. **PBG2 Semantic Surface Assembly:** face/edge classification + best-fit modular tiler + role-based style resolver.
4. **Mix Damage integration:** reuse canonical DamageField and select cheapest representation tier.
5. **AgedFX EPLS field library:** surface aging masks first; PWAS/Particles adapters second.
6. **Laces CurveWeave policy:** build on the finished path/sweep infrastructure; broad cable/rope/anchor applications, not shoe-specific code.

Editor UI belongs later to the existing JericoEditor/M10.5 plan. Backends, schemas and tests should exist first.

---

# 11. Runtime cost policy

The references make one policy especially clear: visual value does not justify evaluating authoring graphs every frame.

| Capability | Preferred Shipping representation |
| --- | --- |
| static fence/building/laces assembly | cooked mesh + instances + semantic metadata |
| dust/dirt/scratches | material masks / packed textures / cheap shader fields |
| moss/cobweb dressing | PWAS instances, optionally baked/merged by distance |
| fog/airborne dust | native volumetric/particle system |
| cosmetic crack | decal/material/bake |
| silhouette damage | bounded local geometry/SDF or cooked variant |
| gameplay destruction | dormant semantic chunks + Jolt activation |
| procedural authoring | recipe retained for rebuild, not mandatory runtime dependency |

Use the cheapest representation that preserves the requested visual/gameplay result.

---


## Mathematical reconstruction companion

The implementation-level equations, field definitions and independent Jerico formulations are in:

- `M8_MIX_DAMAGE_LACES_FENCE_AGEDFX_PBG2_MATHEMATICAL_RECONSTRUCTION.md`

That companion includes the explicit **DamageDecal3D -> indentation -> cavity -> through-hole** path, fresh-interior provenance, local SDF boolean math, crack/steel/brick formulations, CurveWeave/RMF equations, Fence Path Assembly math, AgedFX/EPLS fields and PBG2 best-fit tiling equations. This document remains the architectural summary; implementers must read the mathematical companion for the actual kernels and acceptance tests.

# 12. Source and repository boundary

The repository should contain this research and future Jerico-owned schemas/tests, **not** the purchased add-on archives, `.blend` files, preset meshes, textures, pattern images or serialized node graphs.

When implementation is legal:

1. specify Jerico behavior from this research;
2. write independent RED tests/fixtures;
3. implement native C++/HLSL/engine code without reading purchased graphs during production coding where a clean-room boundary is required;
4. compare observable behavior only through controlled fixtures/oracles;
5. preserve deterministic rebuild provenance;
6. prove cook/runtime without Blender or the purchased source tree.

## Final product interpretation

The useful outcome from these five purchases is not “Jerico gets five Blender add-ons.” It is:

- stronger **DamageField + destruction authoring**;
- a more complete **Path Assembly** system;
- a new **CurveWeave** policy on that same path foundation;
- a **Semantic Building/Facade Assembler** connected to BSP;
- richer **EPLS environmental aging**;
- clean handoffs to **PWAS, materials, particles, physics and navigation**.

That preserves JericoEngine's existing architecture while extracting the genuinely reusable mathematics and data-flow from the references.