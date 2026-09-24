# M8 Ucupaint 2.4.9 — Technical Research Input

**Milestone:** M8  
**Category:** Smart Materials / layered painting / procedural masks / bake / UDIM / tangent-space vector displacement  
**Status:** owner-supplied source inspected; reference design ready; Jerico-native implementation not started by this document  
**Owner timing:** consume only when the relevant M8 material/painter capability is legal  
**Runtime dependency:** none intended  
**Source archive SHA-256:** `c4d87fd9d73b4855b5ed096a52e4374aadb099babeb924674a7674a9b50eb5f8`  
**Source license:** GPL-3.0-or-later  
**Repository policy:** source archive, `.blend` libraries, textures and presets stay outside JericoEngine; only clean-room findings/specifications belong here

---

## 1. Executive verdict

Ucupaint is **strongly useful as a research reference** for JericoEngine, but it should
not become a Jerico runtime dependency and it should not be mirrored as a Blender-node
implementation inside the engine.

The package exposes three especially valuable technical areas that fill real M8 gaps:

1. **surface-feature fields for Smart Materials**, especially signed curvature;
2. **direction fields derived from UV correspondence**, useful for grain, streaks,
   anisotropy and directional procedural masks;
3. **tangent-space vector displacement (VDM) semantics**, including basis conversion,
   range packing and bake/reconstruction concerns.

It also reinforces the already-planned `MaterialBakeIR` direction with a mature set of
specialist bake products: AO, curvature, thickness, flow, object-space normal,
multires normal/displacement, bevel masks, wireframe and selected-region masks.

The correct Jerico response is therefore:

```text
Ucupaint behavior/source
    -> inspect capabilities and invariants
    -> derive clean-room mathematical contracts
    -> reuse existing Jerico mesh/tangent/ray/bake authorities
    -> produce deterministic material fields / cooked textures
    -> no Ucupaint code or Blender node graph in Shipping
```

### 1.1 Practical Jerico role

In product terms, the most important Ucupaint contribution is not a replacement
paint application. It is a common **SurfaceFeatureFields** backend that computes
geometry-aware signals once and lets multiple Jerico systems consume them:

```text
Mesh / surface
    -> SurfaceFeatureFields
         Curvature
         Convexity
         Concavity
         Cavity / Contact
         Edge / feature strength
         Slope / normal direction
         Height / exposure
         Flow / directional field
    -> Smart Materials
    -> Aged Wood / Aged Metal
    -> Damage masks
    -> Geology weathering / sediment
    -> EPLS eligibility
    -> PWAS placement masks
    -> Remesh feature density
    -> MaterialBakeIR / cooked textures
```

Concrete authoring examples:

- convex edges can drive worn paint, polished metal and exposed stone;
- concave regions and contacts can drive dirt, dampness and sediment;
- slope/orientation can drive dust, snow, moss and water eligibility;
- flow direction can orient leaks, rust streaks, grain and anisotropy;
- DamageField can become a material mask before escalating to real geometry/fracture;
- the same fields can be baked offline when runtime evaluation is unnecessary.

This avoids separate edge/cavity/curvature implementations for every material,
damage, geology or scatter feature. The field backend is the shared authority;
consumers combine the fields according to their own recipes.

### 1.2 Adoption table

| Ucupaint area | Jerico decision | Reason |
| --- | --- | --- |
| Layer + mask architecture | **ADOPT CONCEPT** | Directly useful for Smart Materials, but represented as Jerico IR rather than Blender nodes. |
| Signed curvature bake | **PORT FIRST, CLEAN-ROOM** | High-value generic field for edge wear, cavity, dust, damage masks, remesh density and geology/material coupling. |
| Contact concavity/AO | **REUSE/EXTEND** | Keep separate from curvature and use Jerico ray/spatial queries; do not bake the two semantics together. |
| UV flow map | **PORT FIRST, IMPROVE MATH** | Replace edge accumulation heuristic with a triangle Jacobian mapping and confidence field. |
| Tangent-space VDM | **PORT WHEN CONSUMER EXISTS** | Valuable, but must reuse Jerico canonical tangent handedness/UV convention and explicit range metadata. |
| PBR layer blending | **ADOPT THROUGH IR** | Keep common channel blend semantics; do not reproduce every Blender/UI-specific node. |
| Height stack / transition bounds | **ADOPT AS INTERVAL PROPAGATION** | Useful for POM/displacement bounds; remove Blender-specific magic constants. |
| Specialist bake inventory | **ADOPT INTO `MaterialBakeIR`** | Strong extension of the SimpleBake research already in M8. |
| UDIM / image atlas management | **REFERENCE / REUSE EXISTING COOK** | Jerico should not create a second texture ownership/packing authority. |
| POM node construction | **DO NOT PORT** | Jerico already owns height/POM research and runtime rendering. Ucupaint's graph unrolling is a Blender workaround. |
| Cavity/dust/paint-base node groups | **RECONSTRUCT FROM SEMANTIC FIELDS** | Exact formulas are serialized in Ucupaint `.blend` libraries; semantics matter more than graph reproduction. |
| Blender UI / Shader Editor wiring | **REJECT AS ENGINE ARCHITECTURE** | Authoring UX reference only. |

---

## 2. Inspected package inventory

The supplied archive is Ucupaint `2.4.9`, Blender `4.2+`, GPL-3.0-or-later.

Static archive facts from the supplied ZIP:

```text
341 files
34 Python files
~11.80 MiB uncompressed total
~2.59 MiB Python source
```

Largest inspected implementation modules include:

```text
ui.py
Layer.py
common.py
bake_common.py
node_connections.py
Root.py
Bake.py
subtree.py
Mask.py
vector_displacement.py
BakeToLayer.py
UDIM.py
ImageAtlas.py
vector_displacement_lib.py
```

The package also contains serialized Blender node libraries. Those are not copied into
this document or into JericoEngine. This pass extracts only behavior, dimensions,
semantics and independently stated mathematics.

### 2.1 Source anchors used by this audit

These anchors refer to the owner-supplied 2.4.9 snapshot and are intentionally kept
as source-path + line references instead of redistributing GPL source:

```text
ucupaint/bake_common.py:168-260      signed discrete curvature field
ucupaint/common.py:1055-1065         sRGB conversion helpers
ucupaint/common.py:1098-1125         straight-alpha over helper
ucupaint/common.py:396-421           bake-type inventory
ucupaint/common.py:4347-4440         layer/transition height range logic
ucupaint/common.py:6659-6724         UV flow-map construction
ucupaint/common.py:4140-4152         POM graph subdivision helpers
ucupaint/vector_displacement_lib.py  tangent/bitangent, object<->tangent and VDM packing
ucupaint/lib.py:82-206               named node-group capability inventory
```

---

## 3. What Ucupaint actually contributes

Ucupaint is not merely a paint UI. Its architecture reveals a useful decomposition for
Jerico Smart Materials:

```text
source field / image / procedural input
        -> mapping / projection
        -> masks
        -> per-channel modifiers
        -> channel blend
        -> height/normal composition
        -> preview
        -> optional bake / packing / export
```

That decomposition is the useful asset. Jerico should encode it as data and compile it
to its own CPU/GPU/bake paths.

### 3.1 Layer channels

The inspected package supports conventional color/value blending plus distinct height
and normal semantics. The important Jerico lesson is that **normal/height composition
is not equivalent to RGB blending**.

A Smart Material layer therefore needs typed writes, for example:

```text
BaseColor
Roughness
Metalness
NormalTangent
Height
AO
Emission
Opacity
CustomMask
```

and typed composition operators for geometric channels.

### 3.2 Masks are first-class

Ucupaint allows masks to modify a layer independently of the source itself. Jerico
already needs this separation for:

- Smart Materials;
- RBrush/manual paint;
- edge wear;
- cavity dirt;
- wetness/snow/frost eligibility;
- damage masks;
- procedural geology region masks;
- EPLS-driven environmental state.

Do not represent those as ad-hoc booleans in individual shaders. They should compile
through the common mask/field IR.

---

## 4. Strong finding: signed geometric curvature

`bake_common.py:168-260` constructs a genuine mesh-derived curvature signal rather
than blurring a texture.

Observed behavior:

1. face normals, centers and areas are read;
2. coincident vertex positions are welded for the analysis;
3. two faces incident to each welded edge are paired;
4. a dihedral angle is computed from the face normals;
5. the angle receives a convex/concave sign;
6. it is weighted by edge length;
7. contributions are accumulated at edge endpoints;
8. the result is normalized by local vertex area, giving inverse-length units;
9. the signal is scaled by a user distance and saturated;
10. one neighbor relaxation pass reduces coarse-mesh noise.

The source also uses a short-range occlusion contribution during curvature baking to
capture concavity between disconnected pieces.

### 4.1 Jerico correction

Jerico should **not** collapse curvature and contact occlusion into a single primitive.
They are different fields:

```text
SignedCurvature       -> local differential/dihedral shape
ContactOcclusion      -> nearby geometry visibility/proximity
```

A Smart Material can combine them, but keeping them separate makes the same data
reusable for:

```text
positive curvature -> edge polish / edge damage / frost exposure
negative curvature -> grime / cavity darkening / crack preference
contact AO          -> dirt accumulation / contact dampness
abs(curvature)      -> remesh density / feature preservation
curvature + strata  -> geological weathering masks
```

The clean-room formula and tests are specified in
`M8_UCUPAINT_2_4_9_MATHEMATICAL_RECONSTRUCTION.md`.

---

## 5. Strong finding: UV flow direction

`common.py:6659-6724` derives a per-corner direction field by comparing a target UV
layout with a second, deliberately straight reference UV layout.

Observed intent:

```text
known direction in straight UV space
    -> infer corresponding direction in arbitrary target UV space
    -> normalize
    -> encode into two texture/color channels
```

The implementation accumulates target UV edge vectors weighted by alignment of the
corresponding reference-UV edge with a fixed reference direction.

### 5.1 Why Jerico should improve it

The same intent has a cleaner affine formulation. For each triangle:

```text
reference UV triangle S
current/target UV triangle U
J = dU/dS
flow_U = normalize(J * flow_S)
```

This uses the exact 2x2 triangle Jacobian, handles arbitrary directions and provides a
natural degeneracy/confidence measure.

Potential Jerico consumers:

- wood grain and brushed direction;
- water/leak/rust streaks;
- cloth/fiber direction;
- anisotropic material orientation;
- directional dirt and weathering;
- procedural brush direction;
- path-derived textures that later get repacked into arbitrary UV charts.

The result should be a **generic direction field**, not a Ucupaint-specific vertex
color named after flow.

---

## 6. Strong finding: tangent-space vector displacement

`vector_displacement_lib.py` makes the relevant basis conversions explicit.

Observed behavior includes:

- compute/obtain a UV tangent;
- derive bitangent from normal, tangent and a handedness sign;
- project an object-space offset into the `T/B/N` basis using dot products;
- reconstruct object-space offset from tangent-space components;
- optionally pack a signed vector into `[0,1]` around midpoint `0.5`;
- bake/load vector displacement through texture images;
- carry an axis-swap option for differing authoring conventions.

### 6.1 Jerico rule

Jerico already has a canonical tangent path in import/cook and normal mapping. VDM
must consume that authority; it must **not** define another tangent convention.

A cooked VDM descriptor needs explicit metadata:

```text
semantic = TangentVectorDisplacement
uvSet
basisConvention = JericoCanonicalTangent
unit = metres
rangeMeters
encoding = SignedFloat | MidpointUNorm
axisTransform = explicit enum/matrix
colorSpace = LinearData
```

No hidden `flip Y` or `swap YZ` flag should leak into the renderer without provenance.

### 6.2 Runtime versus offline use

Jerico may support two consumers without changing the texture semantic:

```text
OfflineBakeConsumer
    VDM -> displaced high-detail mesh -> remesh/LOD -> normal bake

RuntimeVisualConsumer
    VDM -> supported displacement/microdisplacement path
```

The semantic texture is the same; consumer capability and cost policy differ.

---

## 7. Height-stack range propagation

`common.py:4347-4440` tracks maximum height through layers, groups, transition bump,
flip, crease and intensity. This is important because POM/displacement needs a
conservative range before rendering/baking.

The source uses Blender-specific branch logic and some historical heuristics. Jerico
should retain the **range-propagation requirement** but express it as interval math.

For every scalar height expression `h`, carry:

```text
Range(h) = [h_min, h_max]
```

Examples:

```text
Range(a + b)          = [amin+bmin, amax+bmax]
Range(s * a), s >= 0  = [s*amin, s*amax]
Range(max(a,b))       = [max(amin,bmin), max(amax,bmax)]
Range(mix(a,b,t))     = conservative hull when t in [0,1]
```

The material compiler then derives:

```text
maxPositiveDisplacement
maxNegativeDisplacement
visualBoundsExpansion
POM depth scale
bake cage safety requirement
```

Do not copy arbitrary fine-bump multipliers from a Blender graph. Use world-metre or
texel-aware derivatives.

---

## 8. Specialist bake inventory

The inspected Ucupaint bake type list includes the following relevant products:

```text
AO
Pointiness
Cavity
Dust
Paint Base
Bevel Normal
Bevel Mask
Multires Normal
Multires Displacement
Other-object Normal / Color / Channels
Selected Vertices / Edges / Faces
Flow
Object-space Normal
Thickness
Wireframe
Curvature
```

Jerico should not make `Dust` or `Paint Base` primitive renderer concepts. Instead,
distinguish **primitive fields** from **material recipes**.

Recommended primitive bake fields:

```text
AmbientOcclusion
ContactOcclusion
SignedCurvature
Thickness
BevelFeature
ObjectNormal
TangentNormal
FlowDirection
BoundaryDistance / WireDistance
RegionId / SelectionMask
MaterialId
```

Recommended recipes can then derive:

```text
Dust
PaintWear
CavityDirt
EdgePolish
WetContact
FrostExposure
RustStreakEligibility
```

This aligns the bake system with Smart Materials and avoids one-off bake code per
visual effect.

---

## 9. `MaterialLayerIR` extension

`M8_SIMPLEBAKE_2_9_11_TECHNICAL_INPUT.md` already proposes `MaterialBakeIR`. Ucupaint
adds enough evidence to define a layer-side IR that feeds it rather than creating a
second material system.

Recommended conceptual contract:

```text
MaterialLayerIR {
    LayerGuid stableId;
    SourceField source;
    MappingIR mapping;
    MaskExpr mask;
    ModifierIR modifiers[];
    ChannelWrite writes[];
    BlendPolicy colorValueBlend;
    HeightBlendPolicy heightBlend;
    NormalBlendPolicy normalBlend;
    float opacity;
    uint64 deterministicSeed;
}
```

`ChannelWrite` is typed:

```text
ChannelWrite {
    MaterialSemantic semantic;
    ValueExpression value;
    optional ValueRange conservativeRange;
}
```

Compilation:

```text
MaterialLayerIR[]
  -> validate semantics/units/color spaces
  -> resolve geometry/world fields
  -> evaluate preview graph
  -> MaterialBakeIR when flattening is requested
  -> canonical .rmat/.rtext cook
```

### 9.1 Authority boundary

The IR is authoring/compiler data. Shipping should normally receive canonical cooked
materials/textures and only the runtime-dynamic fields intentionally retained by a
material feature.

---

## 10. Blend and color-space rules

The source contains straight-alpha composition helpers and sRGB conversion helpers.
These are useful as a reminder of two requirements, not as code to copy:

1. blend color in the **declared linear working space** unless the operation explicitly
   defines a different space;
2. choose one alpha convention and make it explicit.

Jerico should prefer canonical float math:

```text
premultiplied source-over:
Cout = Cs + Cd * (1 - As)
Aout = As + Ad * (1 - As)
```

or the algebraically equivalent straight-alpha form at API boundaries.

Do not preserve Ucupaint's byte-oriented rounding helper as an engine requirement.
Do not import a second sRGB transfer threshold when Jerico already owns color-space
conversion in the texture pipeline.

---

## 11. POM: useful confirmation, not a new implementation

Ucupaint contains POM preparation/process node groups and Python helpers that split a
requested iteration count into nested groups. This primarily exists because large
iterative shader logic is represented through Blender node graphs.

Jerico already has dedicated height/POM/displacement work. Therefore:

```text
Ucupaint POM graph topology -> research confirmation only
Jerico POM implementation   -> existing renderer authority
```

What should transfer is the requirement that a layer stack publishes a **known height
range** and tangent/UV convention. The POM algorithm itself should not be forked from
Ucupaint.

---

## 12. UDIM and atlas management

`UDIM.py` and `ImageAtlas.py` contain authoring machinery for tiles, segments, reuse,
copy/swap/removal and packing.

The reusable requirement is:

```text
bake target can span multiple logical tiles
    -> stable logical tile identity
    -> deterministic bake outputs
    -> canonical texture cook/packing
```

Do not let Ucupaint inspire a second Jerico texture registry. Logical UDIM/tile
identity should be authoring metadata that resolves through the existing VFS/asset
cook and whichever atlas/virtual-texture policy Jerico ultimately owns.

---

## 13. Node-library capabilities that should remain semantic

The serialized node-library inventory includes capabilities such as:

```text
straight-over blending
height mix/add/compare
smooth height processing
transition bump / crease
bump-to-normal
edge detection
cavity
paint-base
fake hemi lighting
vector mixing
normal processing
POM
normal bake
sRGB conversion
axis flips
combined VDM
```

Jerico should not create one native shader function per named Ucupaint node group.
Instead map them onto a smaller set of durable primitives:

```text
scalar/vector/color expressions
mask expressions
surface feature fields
height interval propagation
normal/TBN transforms
blend operators
ray/spatial queries
bake targets
```

This keeps the engine independent from the historical shape of one Blender add-on.

---

## 14. Integration with existing Jerico systems

### 14.1 Smart Materials

This is the primary beneficiary. Ucupaint reinforces the existing requirement that a
Smart Material is a recipe responding to mesh/world properties, not a static shader.

Recommended field inputs:

```text
SignedCurvature
ContactOcclusion
SurfaceNormal / Upness
Slope
Height / WorldPosition
BoundaryDistance
Thickness
FlowDirection
DamageField
RBrush/manual masks
EPLS wetness/snow/frost fields
PWAS semantic regions where applicable
```

### 14.2 Aged Wood / Aged Metal / HorrorRendering

Existing wear/dirt/frost/wetness masks can consume the same surface-feature products.
Do not add private curvature implementations to individual materials.

### 14.3 Damage / decals

Positive high curvature and explicit boundary-distance fields are useful for edge
wear/chipping eligibility. They do not replace the canonical `DamageField` or physical
fracture authority.

### 14.4 Smart Remesh / segmentation

`abs(SignedCurvature)` is also a feature-density signal. Reuse the same mesh-analysis
backend rather than recomputing curvature separately in remesh, UV, Smart Materials
and damage tools.

### 14.5 Geology

Geology can publish or consume curvature as a classified surface field, but the
material system must not become a second geometry producer. Under the current
Blender-headless geology authority, Blender may generate candidates; Jerico still owns
admission, semantic validation, cook and runtime products.

---

## 15. Proposed shared services

When the capability is legal, audit existing code before adding anything. The desired
shape is shared infrastructure, not `UcupaintPort`.

Potential service boundaries:

```text
MeshAdjacency / HalfEdgeView
SurfaceFeatureFieldBuilder
  - SignedCurvature
  - BoundaryDistance
  - optional BevelFeature
MeshSpatialQuery / ray query
ContactOcclusionBaker
UvCorrespondenceFieldBuilder
TangentFrameAuthority (existing canonical importer/cook path)
MaterialLayerIR compiler
MaterialBakeIR scheduler
BakeTarget / tile abstraction
TextureCook / channel pack (existing authority)
```

No second mesh registry, tangent registry, texture registry, bake registry or material
runtime is authorized by this research.

---

## 16. CPU/GPU placement

### CPU is appropriate for

- topology construction/weld validation;
- deterministic adjacency;
- recipe validation;
- interval/range propagation;
- job hashing and lineage;
- small/medium mesh feature precomputation;
- reference/oracle implementations.

### GPU/compute is appropriate for

- large mesh field evaluation once topology/adjacency is packed;
- raster/ray bake passes;
- AO/contact sampling;
- texture-space field composition;
- large-resolution Smart Material preview/bake;
- mip/dilation/postprocess where existing texture tools allow it.

The implementation decision must be benchmark-driven. A GPU version is not a goal by
itself; deterministic output, bounded memory and reuse matter more.

---

## 17. Clean-room / licensing boundary

Both the source license and the future public direction of JericoEngine make this
boundary important.

Allowed research outputs:

- observed behavior;
- independently stated equations;
- dimensional analysis;
- architecture;
- test fixtures and acceptance criteria derived from behavior;
- source path/line anchors for audit provenance;
- independent Jerico C++/HLSL implementation.

Not admitted by this document:

- copying Python implementation into Jerico;
- translating GPL functions line-by-line to C++;
- committing Ucupaint `.blend` libraries;
- shipping Ucupaint as a hidden engine dependency;
- serializing/copying its node graphs as Jerico's material format.

The implementation document deliberately improves/re-derives the useful mathematics
instead of transliterating source.

---

## 18. Acceptance gates for a future native implementation

### 18.1 Signed curvature

Must pass at least:

- plane -> approximately zero field;
- convex cube edge -> correct convex polarity;
- concave L-corner -> opposite polarity;
- cylinder -> stable sign and approximately uniform magnitude away from caps;
- sphere -> approximately uniform positive curvature;
- mesh scale by `s` -> curvature scales approximately `1/s`;
- UV seams do not alter geometric curvature;
- intentionally disconnected contact pieces do not fake curvature; contact AO is a
  separate output;
- deterministic result independent of thread scheduling.

### 18.2 UV flow

Must pass:

- identity UV map;
- pure rotation;
- anisotropic scale;
- shear;
- mirrored island;
- seam duplication;
- degenerate reference triangle -> explicit invalid/confidence zero;
- deterministic raster/bake encoding.

### 18.3 VDM

Must pass:

- canonical tangent test asset;
- mirrored UV handedness;
- V-flipped imported asset;
- pure tangent/bitangent/normal unit displacement vectors;
- encode/decode round trip within declared format tolerance;
- displacement bound validation against `rangeMeters`;
- color management cannot reinterpret data as sRGB;
- offline and runtime consumers agree on basis convention.

### 18.4 Material layers

Must pass:

- deterministic reorder/rebuild;
- stable layer GUIDs;
- group range propagation;
- mask composition;
- blend type validation;
- missing field diagnostics;
- preview/bake parity within declared tolerance;
- cancel/failure publishes nothing;
- source-free cooked runtime does not need Blender or Ucupaint.

---

## 19. Scheduling / scope rule

This research **does not change `ACTIVE_SLICE.md`, `GOAL.md` or the current milestone**.

When the relevant M8 Smart Materials/painter work becomes legal:

1. audit existing mesh-adjacency, tangent, bake and material code;
2. read this document plus the mathematical reconstruction;
3. implement the smallest shared primitive first;
4. start with RED analytic fixtures;
5. add the CPU/reference path;
6. only add GPU acceleration when measured cost justifies it;
7. integrate through `MaterialLayerIR` / `MaterialBakeIR` and existing cook;
8. produce visual and numeric evidence before claiming the capability complete.

---

## 20. Recommended implementation order

```text
P0  SurfaceFeatureFieldBuilder::SignedCurvature
P0  ContactOcclusion stays a separate field/consumer
P0  MaterialLayerIR field/mask semantic contracts
P1  UV correspondence -> FlowDirection Jacobian field
P1  specialist MaterialBakeIR fields/packing
P1  Smart Material recipes consuming curvature/flow/contact
P2  tangent-space VDM semantic + bake/import descriptor
P2  UDIM/tile workflow integration through existing texture authority
```

Do **not** schedule Ucupaint POM, Blender UI mirroring or node-graph reproduction.

---

## 21. Final decision

Ucupaint 2.4.9 is one of the more useful add-on references supplied for the material
side of JericoEngine because it exposes not only UI workflow but reusable geometric
and bake semantics.

The most valuable extracted capability is not “layer painting” as a monolith. It is a
small set of reusable foundations:

```text
signed surface curvature
contact occlusion kept separate
UV-to-UV direction transport
typed material layers/masks
conservative height range propagation
canonical tangent-space vector displacement
specialist semantic bake fields
```

Those foundations should converge with Jerico's existing Smart Materials,
`MaterialBakeIR`, mesh analysis, texture cook and tangent authority. They must not
create a second material system or make Ucupaint/Blender required at runtime.