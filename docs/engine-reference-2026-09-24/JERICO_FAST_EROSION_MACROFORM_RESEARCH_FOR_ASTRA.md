# JericoEngine Research Note — Fast Erosion Filters, Macroform Authoring, and M8 Applicability

**Date:** 2026-09-06  
**Audience:** Astra / JericoEngine geology work  
**Status:** RESEARCH ONLY — DO NOT IMPLEMENT FROM THIS NOTE  
**Primary goal:** Extract useful mathematics and architecture lessons from fast procedural erosion and Blender rock/terrain authoring systems without opening a new M8 detour.

---

## 1. Executive summary

Two separate findings matter:

1. **Fast procedural erosion filters can create convincing branching gullies/ridges without simulating water transport.**  
   Rune Skovbo Johansen's *Advanced Terrain Erosion Filter* is effectively a directional, multi-octave erosion-looking noise/filter driven by the terrain gradient. It is point-evaluable, chunk-friendly, GPU-friendly, and can sit on top of an existing height function.

2. **Good rock/terrain authoring systems usually establish macroform before adding detail.**  
   Blender tools such as RockForm Designer and On The Rocks emphasize guide shapes, base-form families, directional placement, edge damage, fractures, and later displacement/detail. This is conceptually different from "start with a generic mass and add more noise."

Neither finding justifies a new Jerico subsystem now.

**Current M8 priority remains the real-member LOD RED.**  
The latest audit (`M8_F4_LOD_CAUSE_AUDIT.md`, commit `059da236`) confirms folded faces already exist in the dense sampled mesh before LOD. Any erosion or macroform experiment must remain deferred until valid local face geometry and angularly faithful coarse geometry are solved.

---

## 2. Source research

### 2.1 Rune Skovbo Johansen — Fast and Gorgeous Erosion Filter

Primary write-up:

- Rune Skovbo Johansen, **Fast and Gorgeous Erosion Filter**, 2026.
- The technique is explicitly intended to create the appearance of erosion without particle/water simulation.
- Each point can be evaluated independently.
- This makes the method suitable for chunked terrain and GPU evaluation.

Reference implementation reviewed:

- `lpmitchell/AdvancedTerrainErosion`
- CPU/Burst translation of the shader technique.
- Outputs:
  - eroded height;
  - approximate ridge map;
  - intermediate slope derivatives.

Licensing from the reviewed repository:

- `ErosionFilter` — MPL-2.0.
- `PhacelleNoise` — MPL-2.0.
- Other portions are MIT unless otherwise noted.

**Jerico rule:** treat the code as external reference. Do not copy MPL-covered implementation into Jerico without an explicit licensing decision. Prefer a Jerico-owned implementation derived from the published mathematical ideas and independently tested semantics.

---

## 3. What the erosion filter actually does

This is **not hydraulic erosion simulation**.

It behaves more like:

```text
base height + base slope/gradient
        |
        v
directional gully pattern aligned to downhill direction
        |
        v
update effective slope
        |
        v
next, smaller octave branches from the modified slope
        |
        v
mask / preserve previous ridges and creases
        |
        v
height delta + approximate slope delta + ridge map
```

The important properties are:

### 3.1 Direction from the terrain gradient

The input height function supplies a gradient.

The erosion pattern aligns elongated stripes/gullies with the slope direction. Smaller octaves use the slope modified by previous octaves, producing branching rather than unrelated layered noise.

This is the first strong lesson for Jerico:

> **Higher-frequency detail should inherit direction from lower-frequency structure.**

It should not merely be independent FBM stacked on top of FBM.

---

### 3.2 Phacelle Noise

Phacelle Noise blends cosine/sine stripe contributions from a local 4x4 cell neighborhood.

Conceptually:

- cells contain randomized local pivots;
- stripe phase is evaluated around each cell;
- neighboring contributions are smoothly blended;
- cosine provides height-like offset;
- sine plus the perpendicular direction supplies an analytical derivative.

This produces directional noise that can bend with a varying slope without using a global flow simulation.

Approximate computational shape:

```text
per sample:
    for each erosion octave:
        evaluate 4x4 local cell contributions
        update gully direction
        update masks / height / slope
```

So cost is roughly:

```text
O(sample_count * octave_count)
```

with a relatively heavy ALU kernel but no global flow state.

There is no exact published Jerico-equivalent performance number in the reviewed material, so no performance claim should be accepted without benchmarking on Jerico hardware and target quality profiles.

---

### 3.3 Fade approach

Near peaks and valleys, slope approaches zero and its direction becomes unstable.

Instead of changing stripe frequency to infinity, the newer approach fades the gully contribution toward a target value.

The fade target can preserve:

- peaks;
- valleys;
- V-shaped valleys;
- overall macro height.

A smooth slope-shaped mask avoids abrupt folds around near-zero gradients.

**Jerico relevance:** this resembles a general design rule already useful in our geology stack:

> Structural detail should fade according to a stable geometric measure instead of changing frequency or orientation discontinuously near singular regions.

---

### 3.4 Stacked fading

Each octave not only modifies height; it also modifies the mask used by later octaves.

Result:

- large ridges and gullies survive;
- smaller detail does not overwrite them everywhere;
- higher-frequency effects are increasingly restricted to valid regions.

This is especially relevant to Jerico.

It is the same broad principle we want for geology:

```text
macro structure
    remains authoritative
        while
micro detail
    decorates only permitted regions
```

This is potentially more important than the specific noise function.

---

### 3.5 Partial normalization

Straight normalization of blended gully waves creates loop/spike artifacts where contributions cancel.

The reference technique only normalizes contributions above a magnitude threshold.

Result:

- crisper ridges/creases;
- fewer spikes/holes;
- controlled discontinuity rather than unstable full normalization.

**Jerico relevance:** avoid "normalize everything because mathematically cleaner."  
Conditioning and fallback must be feature-aware.

---

### 3.6 Straight gullies / assumed slope

A key discovery in the reference technique is that the physically exact analytical output slope was not always the best directional guide.

The filter keeps a separate internal `gullySlope` used to direct subsequent octaves.

It may deliberately use an assumed/faked slope magnitude to produce cleaner branching.

This is a useful architecture lesson:

```text
output geometric derivative != control derivative used to author the next scale
```

Jerico should keep this distinction in mind for procedural authoring. A control frame may be intentionally stabilized without pretending it is the final physical normal.

---

### 3.7 Ridge map

The algorithm separately exposes an approximate ridge/crease signal.

Potential Jerico uses, if independently implemented later:

- erosion/detail mask;
- drainage coloration;
- material transitions;
- vegetation/scatter masks;
- snow/debris accumulation masks;
- LOD feature-importance hints.

Do **not** wire such a signal into LOD until the current LOD producer is repaired and feature preservation is proven.

---

## 4. Comparison with Jerico's existing erosion systems

Jerico already has a much heavier explicit heightfield erosion path.

`GeologyHeightfieldErosionPayload` includes:

- stream-power iterations;
- depression filling;
- drainage concentration;
- rainfall;
- flow rate;
- hydraulic depth;
- sediment capacity;
- erosion;
- deposition;
- evaporation;
- thermal transfer;
- talus angle.

The CPU implementation keeps water, pipe flux, sediment, deposition and thermal state through iterations.

That system and the Rune-style filter solve different problems.

### Jerico existing heightfield erosion

Best when we need:

- explicit hydrology-like state;
- sediment/deposition behavior;
- stream-power controls;
- deterministic physical-ish process evidence.

Cost characteristics:

- iterative;
- neighbor-dependent;
- more memory/state;
- harder to evaluate purely pointwise.

### Fast procedural erosion filter

Best when we need:

- fast believable mountain gullies/ridges;
- chunk-safe evaluation;
- direct point sampling;
- cheap visual authoring previews;
- scalable macro terrain generation.

Cost characteristics:

- no water/sediment state;
- fixed local neighborhood work per octave;
- ALU-heavy;
- naturally parallel;
- visual model, not physical simulation.

**Recommendation:** do not replace Jerico heightfield erosion with the fast filter.

If admitted later, treat it as a separate **visual/macro erosion mode** or macro-heightfield filter, not as evidence-equivalent hydraulic erosion.

---

## 5. Best fit inside Jerico

### 5.1 Strongest fit: macro-heightfields / hills / mountains

Jerico already has `GeologyGpuMacroHeightfieldSource` with:

- macro fractal;
- ridge fractal;
- hybrid amplitude;
- domain warp;
- boundary irregularity;
- macroform kinds such as Cliff, RockFamily, Cave, IceArch and TerrainTransition.

A fast directional erosion pass fits naturally **after macro height generation and before conversion into the final geology field**.

Conceptually:

```text
macro height
    ->
macro/ridge/hybrid/domain warp
    ->
FAST DIRECTIONAL EROSION FILTER
    ->
height + optional ridge mask
    ->
existing SDF/geology integration
```

This is the cleanest candidate because the technique is natively heightfield/2D-gradient based.

---

### 5.2 Possible fit: local cliff-face authoring

A future version could evaluate the filter in a stable local 2D chart over selected cliff surfaces.

Possible mapping:

```text
classified cliff region
    ->
stable local tangent/planar chart
    ->
directional erosion filter
    ->
bounded displacement
```

Risks:

- chart seams;
- chart distortion;
- overhang transitions;
- multi-face continuity;
- region ownership;
- detail preservation through remesh/LOD.

This is research only.

---

### 5.3 Poor direct fit: arbitrary 3D SDF weathering

Do not directly interpret a 2D terrain erosion filter as a general 3D volumetric weathering operator.

It has no native concept of:

- arbitrary cave walls;
- multiple sheets;
- interior cavities;
- full 3D drainage;
- overhang topology;
- closed-volume erosion.

For arbitrary SDF geology, Jerico's existing volumetric/surface-aware operators remain the proper architecture.

---

## 6. Performance-first assessment

Jerico's rule is performance before feature admission.

A future spike must measure:

- evaluations per output sample;
- octave count;
- CPU vs GPU time;
- memory delta;
- shader register pressure;
- occupancy;
- cancellation behavior;
- dirty-region behavior;
- chunk seam determinism;
- lower-end hardware profile.

### Expected qualitative profile

The reference Phacelle kernel evaluates a fixed 4x4 cell neighborhood per octave.

At five octaves that means many trigonometric/hash/weight operations per output point, but:

- no water grid;
- no sediment grid;
- no iterative neighbor exchange between terrain cells;
- no global convergence;
- no mandatory readback.

Therefore it is a plausible **preview/final-quality scalable visual erosion option**, but only benchmark data can decide whether it beats existing Jerico paths at equivalent visible quality.

Possible quality profiles later:

```text
InteractivePreview: 2-3 octaves
HighPreview:        3-5 octaves
FinalBake:          5+ octaves if evidence justifies it
```

These are research ranges, not frozen API values.

---

## 7. Macroform authoring lesson from Blender rock systems

The strongest cross-tool pattern is not the exact erosion mathematics.

It is the order of authoring:

```text
guide / intended silhouette
    ->
macroform family
    ->
structural breakup
    ->
edge/fracture operations
    ->
surface erosion
    ->
micro detail
    ->
remesh / LOD
```

RockForm Designer and similar systems let the artist establish:

- path/guide shape;
- formation direction;
- density;
- scale;
- rotation;
- spread;
- recognizable large silhouette;

before fine displacement.

On The Rocks similarly separates:

- rock/formation creation;
- edge damage;
- surface erosion;
- fractures;
- landscapes.

This suggests a Jerico audit question:

> Does Jerico currently start procedural geology from a sufficiently intentional macroform, or are we asking later layers to rescue a source mass whose silhouette was generic from the beginning?

Jerico already has substantial pieces:

- BSP sources;
- BlockGrammar;
- AperiodicTiling;
- ImplicitBlock archetypes;
- macro-heightfield kinds;
- PlaneCut;
- JointSet;
- FaultDisplace;
- FoldDomain;
- fractures;
- weathering.

So this is **not evidence that a new generator is required**.

The likely question is whether the authoring layer exposes enough macroform authority and guide-driven composition before detail layers begin.

---

## 8. Relationship to the current LOD blocker

The latest M8 LOD audit is more urgent than this research.

Confirmed facts from `M8_F4_LOD_CAUSE_AUDIT.md`:

- 112.7° is not mainly a normal accumulation error.
- Dense sampled geometry already contains folded faces.
- A mass-point control removes opposed faces and improves the angular error to ~46.5°, but still fails the 35° gate.
- Same-region closest-surface correspondence still remains ~46.8°.
- Meshlets/clusters do not fix a defect already present before LOD.
- No production repair was demonstrated; the experiment was withdrawn.

Therefore:

**Do not begin erosion-filter integration while this RED is active.**

The current producer must first guarantee:

1. valid local face geometry during sampled extraction;
2. angular preservation in the coarse LOD;
3. region/collision correctness.

Fast erosion can improve macro terrain quality later, but adding better hills to invalid folded geometry would only decorate the defect.

---

## 9. Recommended future spike — only after the current M8 LOD gate is green

### Goal

Determine whether a Jerico-owned directional erosion filter can improve hills/cliffs at lower cost than explicit hydraulic simulation while preserving deterministic chunk-local generation.

### Test

Use one fixed macro-heightfield source and compare:

- **A:** base macro-heightfield, no erosion.
- **B:** current Jerico heightfield erosion at bounded comparable quality.
- **C:** independent Jerico implementation of the published directional-filter concepts.

Hold constant:

- seed;
- dimensions;
- sample spacing;
- materials;
- camera;
- output mesh path;
- LOD settings.

Measure:

- CPU/GPU generation time;
- memory;
- chunk seam identity;
- slope/ridge statistics;
- resulting triangle/LOD behavior;
- native visual captures.

### Falsation

Reject the fast filter if:

- visible quality is not meaningfully better than existing cheap macro noise;
- cost is not materially lower than current erosion at comparable quality;
- seams appear across chunks/charts;
- it worsens LOD or creates high-frequency features below representable scale;
- it requires a second incompatible terrain framework.

---

## 10. Integration preference if the future spike succeeds

Do **not** create a new `GeologyLayerKind` first.

Preferred order:

1. Audit whether it belongs inside existing macro-heightfield generation.
2. If it needs an authored geology-layer surface, audit `GeologySurfaceWeatheringPayload` as an alternate model.
3. Only introduce a new layer kind if neither existing contract can express its semantics cleanly.

Jerico already has many geology layer kinds. Avoid expanding the enum merely because a research technique has a name.

---

## 11. License boundary

The reviewed `AdvancedTerrainErosion` repository explicitly marks:

- `ErosionFilter` as **MPL-2.0**;
- `PhacelleNoise` as **MPL-2.0**.

Other referenced noise components may be MIT.

For Jerico:

- mathematics and architectural ideas may be studied;
- do not paste/copy the covered implementation into an existing Jerico source file;
- if direct code reuse is ever desired, perform an explicit MPL-2.0 compatibility/source-disclosure review first;
- safest current path: independent Jerico implementation from the published description, with original tests and naming.

This research note is not a licensing determination.

---

## 12. Astra directive

Astra should treat this note as **future research evidence**, not a next action.

### Now

Continue the active M8 LOD geometry RED:

```text
valid dense faces
    ->
angularly faithful coarse LOD
    ->
Beauty
    ->
four real rock members
    ->
remaining M8 gates
```

### Later, after that RED is closed

Audit two questions:

1. Can the directional erosion concept improve Jerico macro-heightfields cheaply and deterministically?
2. Does Jerico's authoring workflow provide enough guide-driven macroform authority before structural/detail operators, or are later operators being asked to rescue overly generic initial silhouettes?

Do not implement either answer without a bounded A/B showing visible benefit and acceptable performance.

---

## 13. Bottom line

The fast erosion research is promising for **mountains/hills and possibly charted cliff surfaces** because it creates structured branching detail without global simulation.

The bigger authoring lesson is:

> **Macroform first. Structure second. Surface detail third. LOD only after the form is valid.**

Jerico already contains most of the structural mathematics.

The next gains are more likely to come from:

- preserving valid geometry;
- controlling macroform intentionally;
- applying detail according to inherited structure;
- and reducing geometry without destroying those structures;

rather than adding another generic noise layer.

**M8 remains the priority.**
