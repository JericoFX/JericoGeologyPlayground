# M8 Sparse SDF Geology Typed Controls Design

Date: 2026-08-23

State: recommended option approved in chat; pending written review

Slice: `M8-SPARSE-SDF-CAVE-TUNNEL-SHOWCASE`

**Superseded:** archival design only. The deleted Sparse-SDF/VDB-era authorities
below no longer govern implementation. Current authority is
`docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`.

## 1. Outcome

Jerico exposes every behavior-affecting Sparse SDF geology parameter through a
typed, versioned recipe and a Basic/Advanced inspector. Rocks, cliffs and cave
host masses use the same controls; caves add an independent negative-space
stack. Presets only assign visible recipe values and never hide a second
hardcoded geology path.

This design does not expose implementation invariants such as hash primes,
Marching Cubes tables or floating-point safety epsilons. Those do not represent
authoring choices. Every value that changes shape, geology, extraction quality,
collision, LOD, cook output or a debug view is inspectable and editable.

## 2. Authority and scope

This historical spec was formerly paired with two now-deleted authorities. Any
reusable mathematical statement is subordinate to the current blueprint and
`docs/SDF_GEOLOGY_EROSION_FRACTURE_MATH.md`.

The binding flow remains:

```text
BaseField
  -> GeologicalModifierStack
  -> OptionalNegativeSpaceStack
  -> Final Sparse SDF
  -> SurfaceClassification
  -> Mesh / Collision / LOD / Cook
```

The controls work on arbitrary 3D solids. They must not reduce the core system
to a heightfield or introduce OpenVDB, a general node-graph editor, a new
renderer, a second world system or a new terrain slice.

## 3. Approaches

### 3.1 Chosen: typed complete controls

Use strongly typed parameter groups owned by `SparseSdfGeologyRecipe`. Basic
mode shows semantic geological controls; Advanced mode reveals the complete
numeric model. Both edit the same data. This preserves validation,
deterministic hashing, migration and discoverability.

### 3.2 Rejected: expose every implementation constant

Numerical tolerances, lookup tables and hashing constants are algorithm
invariants. Exposing them would make identical recipes dependent on unstable
implementation details without adding geological control.

### 3.3 Deferred: unrestricted field graph

A typed field graph may be valuable later, but it is a separate subsystem with
serialization, evaluation, cycle handling and UI contracts. It is not required
to prove the current shared geology layer.

## 4. Canonical recipe ownership

The public authoring model is composed from explicit groups rather than one
flat bag of numbers:

```text
SparseSdfGeologyRecipe
  IdentityAndVersion
  GeometryType
  BaseFieldSettings
  SamplingAndDistanceQualitySettings
  ModifierStack
    AnisotropySettings
    DomainWarpSettings
    MacroNoiseSettings
    MesoBreakupSettings
    StrataSettings
    GeologyMaterialSettings
    FractureSettings[]
    ErosionSettings
    ContactSettings
  NegativeSpaceSettings[]
  SurfaceClassificationSettings
  ExtractionAndTopologySettings
  LodSettings
  CollisionAndCookSettings
  DebugSettings
```

Every group has an explicit enabled state where disabling is meaningful.
Modifier order is canonical and visible. Reordering is not allowed until the
engine supports and versions arbitrary operator ordering.

## 5. Complete control surface

### 5.1 Identity, base and sampling

- geometry type, base-field kind and stable identity;
- transform, center, dimensions/radii, orientation and roundness/exponent;
- composite/BSP source identity and source revision where applicable;
- global seed, algorithm version and per-stage seed offsets;
- bounds, voxel size, narrow-band width and resource budgets;
- distance-quality mode, automatic repair threshold, re-distance mode,
  iteration/sweep budget and maximum accepted gradient error.

`Show SDF Bounds` only controls bounds visualization. Generated Mesh visibility
is independent and remains on when bounds are hidden.

### 5.2 Anisotropy and domain warp

- per-axis anisotropy and asymmetry;
- warp amplitude, frequency, channel offsets and world/local-space choice;
- octave count, lacunarity, gain and normalization;
- maximum coordinate displacement and topology guardrail;
- seed offset and enabled state.

Warp channels remain independently decorrelated. The UI never offers a mode
that broadcasts one scalar sample to all three axes.

### 5.3 Macro and meso

Each band owns its amplitude, base frequency, octave count, lacunarity, gain,
normalization, seed offset, remap/profile and maximum displacement. Meso also
selects its breakup profile, such as signed FBM, ridged or terraced response.

Macro and meso values are expressed in world-space units and validated against
local mass thickness. They may deform a mass but cannot silently enable cave
subtraction.

### 5.4 Strata and geology material fields

Strata controls include:

- arbitrary normalized direction/orientation;
- physical spacing, thickness/duty cycle and phase;
- profile, displacement amplitude and groove strength;
- coordinate-warp amplitude/frequency/noise controls;
- seed offset and regional mask;
- a typed list of geological layers/material responses.

Each geological layer exposes hardness, erodibility, porosity, permeability,
fracture resistance, sediment affinity and material ID. Geometry displacement,
material classification and erosion response consume separate outputs; one
strata scalar is not silently reused for every role.

### 5.5 Fracture families

The recipe owns an editable list of fracture families. Every family exposes:

- stable identity, enabled state, kind (`Voronoi3D` or directional plane
  joints), seed offset and priority;
- world-space spacing, width, maximum depth and blend width;
- orientation, anisotropic scale, jitter and density;
- persistence/probability, regional mask and surface-proximity mask;
- minimum surviving wall thickness and through-cut policy;
- influence on erodibility and exposed-interior material;
- debug color/view selection.

Voronoi evaluation retains stable nearest/second-nearest cell IDs and the
`F2-F1` proximity field. Independent feature-point offsets are used for X/Y/Z.
Fractures are finite volumetric cutter fields, not painted mesh lines. A
shallow-fracture family must remain a local cut and cannot create a through-cave
unless the explicit policy permits it.

### 5.6 Volumetric erosion and weathering

Erosion is an offset of the 3D implicit field. Controls include:

- maximum world-space wear depth and enabled state;
- base wear/noise scale, octave controls and seed offset;
- hardness/erodibility response;
- fracture-proximity response;
- curvature, exposure, slope/gravity and strata-material weights;
- author mask strength, deposit-cover response and output clamp;
- deterministic step/substep count when an iterative mode is admitted.

The first implementation may use the bounded deterministic approximation
authorized by the math directive. Controls that require a future physical
transport solver do not appear as fake functioning options.

### 5.7 Negative space

Negative-space features are a separate editable list. Every feature exposes
kind, stable identity/source BSP brush, transform/endpoints, radii/cross-section,
roundness, smoothness, radius variation, local warp, seed, enabled state and
influence bounds. Tunnel, chamber, branch, arch, undercut, cavity, cut-through
and BSP opening remain explicit semantics.

`NegativeSpace=None` means no cave/tunnel/chamber subtraction. Shallow fracture
and erosion settings do not change that declaration.

### 5.8 Classification, extraction, LOD, collision and cook

Controls include:

- Floor/Wall/Ceiling/Exterior/Strata/Fractured thresholds and material mapping;
- Generated Mesh visibility, normals/tangents debug and stage selection;
- minimum/maximum edge length, approximation error, feature preservation,
  adaptive subdivision/remesh mode and triangle budget;
- LOD count, per-level error/ratio and transition policy;
- collision source/quality and navigation invalidation policy;
- cook profile, deterministic algorithm version and runtime budgets.

Only controls backed by a working implementation may be enabled. Missing future
capabilities remain explicitly unavailable rather than pretending to modify
the result.

## 6. Basic, Advanced and presets

Basic mode exposes geometry type, base dimensions, the strength/scale of each
geological stage, negative-space features, quality preset and debug toggles.
Advanced mode exposes every field listed above. Switching modes never changes
the recipe.

Lithology and quality presets apply a visible transaction that changes ordinary
recipe values. The inspector shows the resulting values immediately, supports
diff and undo/redo, and labels a modified preset as customized. Presets do not
invoke hidden code paths.

## 7. Validation and guardrails

Editing uses two validation levels:

1. **Preview validity:** finite values, legal enums, positive physical scales,
   bounded resource use and resolvable identities. Invalid edits do not replace
   the last valid preview.
2. **Publish validity:** closed/dominant required mass, minimum wall thickness,
   resolved feature width, bounded displacement, collision/LOD/cook success and
   no accidental cave topology.

Extreme but finite values may be previewed when resource-safe. The UI reports
why a result is not publishable instead of silently clamping it into a different
recipe. Hard safety/resource limits fail explicitly.

## 8. Serialization, hashing and migration

Every behavior-affecting value participates in deterministic hashing and is
serialized with an algorithm/schema version. Same recipe, seed, source
revisions and algorithm version reproduce equivalent field/cook output.

Legacy sphere Add/Subtract operations retain their existing compatibility path.
The shared geology recipe must not silently reinterpret old serialized values.
Backend or default changes that alter output require an explicit version bump or
migration.

## 9. Editor transaction and preview

The inspector edits a draft recipe. Apply starts the existing asynchronous,
cancellable Sparse SDF generation path. The editor retains:

- last accepted recipe and preview;
- draft diff;
- generation progress/cancellation;
- validation diagnostics;
- undo/redo transaction;
- deterministic seed and version;
- bake/cook result only after explicit acceptance.

Batch and preset application use the same recipe transaction. Assets and BSP
sources remain immutable. The system suggests; the designer decides.

## 10. Debug evidence

The editor provides independent controls for:

```text
Bounds
Base Field
Warp Magnitude
Macro
Meso
Strata Coordinate / Material
Fracture Proximity / Cell ID
Erosion / Wear
Negative Space
Final SDF Sign / Gradient Error
Generated Mesh
Surface Classification
Collision / LOD
```

The current slice must retain RAW iso-surface evidence for stages A–H, then
show Rock, Cliff and Cave results with Generated Mesh on and Bounds off.

## 11. TDD and acceptance

Implementation proceeds through focused RED/GREEN verticals:

1. Recipe hashing/validation changes when any exposed control changes.
2. Base through Strata controls change their own field stage while preserving a
   closed dominant rock.
3. A 3D fracture family produces stable cell IDs and a finite-width local cut,
   remains visible on top/wall/underside samples and does not make a through-cave.
4. Hardness/erodibility and fracture proximity change volumetric wear on the
   same arbitrary 3D solid.
5. Distance-quality tracking requests repair only when required; re-distance
   restores the admitted gradient/error bound without changing intended sign.
6. Inspector Basic/Advanced edits the same typed recipe, supports preview,
   cancellation, diff and undo/redo.
7. Generated Mesh remains visible with Bounds hidden.
8. Rock, Cliff no-negative-space and Host Mass plus explicit Tunnel/Chamber
   pass real Vulkan, collision, navigation, LOD, cook and source-free runtime
   evidence.

No stage is `PASS` from hashes or unit tests alone; RAW and final visual evidence
remain mandatory.

## 12. Exclusions

This design does not admit OpenVDB/EmberGen, a general node graph, heightfield
terrain erosion, a physical sediment transport solver, Dual Contouring,
Procedural 3D Layout, PWAS 2.0, particles/vegetation, another renderer/GI,
VT/HLOD, ray tracing or a virtual-shadow rewrite. Those require their own legal
slices.
