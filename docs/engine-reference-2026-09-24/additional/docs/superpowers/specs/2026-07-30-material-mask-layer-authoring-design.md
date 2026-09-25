# Material Mask and Layer Authoring Design

Date: 2026-07-30

Status: accepted direction. The reusable-preset option was recommended and the
user's standing one-minute rule selected it after no objection was received.
This is a future material-authoring slice; it does not widen M6F-a Task 3.

## 1. Intent

JericoEditor will provide one generic Material Mask Lab for building reusable
surface masks and physically meaningful material blends. Rust at exposed edges
is one preset, not a dedicated system. The same machinery must author paint
wear, dirt, dust, mud, snow, wetness, moss, frost, burns and future effects.

The designer can:

- combine procedural, geometric, painted and imported mask sources;
- mix materials horizontally by a mask;
- perform height-aware blending between material surfaces;
- stack an admitted physical layer such as coat or fuzz where the master shader
  supports it;
- paint both masks and scalar height data directly in JericoEditor;
- expose friendly controls such as Strength, Amount, Contrast, Bias,
  Transition, Falloff and Invert;
- preview each source, intermediate result and final material before Apply;
- reuse one preset across meshes, JericoBSP, terrain and PWAS/EPLS targets; and
- cook static results to existing Jerico runtime assets.

OpenPBR is used as a vocabulary for predictable material behavior. It is not
adopted wholesale as a second renderer, a second file format or an unbounded
runtime shader graph.

## 2. Alternatives

### 2.1 Embed a private graph inside every material

Rejected. It duplicates logic, makes fixes inconsistent and prevents a solo
developer from improving one preset for all users.

### 2.2 Reusable Mask Graph plus per-instance overrides

Selected. A preset owns the graph, stable seed and exposed parameters. A
material, terrain layer or EPLS rule references it and may override only those
published parameters. The same preset can therefore produce related but not
identical results across many assets.

### 2.3 General Substance-like shader authoring or custom-code nodes

Rejected for the initial system. It would create another shader language,
unbounded variants and a hostile-code boundary. Jerico's graph produces typed
masks and inputs for already-admitted material masters.

## 3. Domain model

```text
MaterialMaskPreset
  identity + version
  deterministic seed
  bounded typed graph
  exposed parameters
  one or more named scalar outputs

MaterialMaskInstance
  preset reference
  target binding
  parameter overrides
  painted-layer references

MaterialBlendStack
  bounded material inputs
  mask output per blend
  WeightBlend | HeightBlend | PhysicalLayer
  channel-composition policy
```

The preset is reusable authoring data. The instance records only target-specific
bindings and overrides. A graph change marks dependents stale and offers a
preview/diff; it never silently rewrites locked or published results.

This design does not yet assign a new public extension. The implementation
slice must either use the established authoring-recipe envelope or approve a new
format through the normal RuntimeFormats/ADR gate. Shipping receives no graph.

## 4. Mask sources

The graph admits bounded, typed sources grouped by purpose.

### 4.1 Geometry-derived

- curvature and convex/concave edge selection;
- cavity and ambient-occlusion fields;
- world or object slope/orientation;
- position and altitude;
- thickness when valid source data exists;
- contact/proximity/distance fields;
- vertex color and material ID; and
- stable object, region and surface tags.

Geometry maps are generated from reader-validated cooked geometry or a trusted
editor document. They are never inferred by executing content from an imported
file.

### 4.2 Authored and spatial

- imported grayscale texture or heightmap;
- painted mask layer;
- painted scalar height layer;
- spline, decal and bounded volume mask;
- terrain weightmap;
- PWAS region/exclusion data; and
- EPLS wetness, snow, damage or other admitted environmental fields.

### 4.3 Procedural

- deterministic noise and fractal combinations;
- grunge breakup;
- directional streak/flow;
- edge breakup;
- cell/crack-like fields where a preset needs them; and
- seeded variation that is stable across Preview, Apply, reimport and cook.

Initial nodes output normalized scalar fields. Vector/color outputs remain a
later need-driven extension.

## 5. Mask operations and exposed controls

The initial operation vocabulary is deliberately small:

- Add, Subtract, Multiply, Minimum and Maximum;
- Remap/Levels, Clamp and Invert;
- Contrast and Bias;
- Blur, Dilate and Erode with explicit bounded radius;
- directional falloff; and
- deterministic noise warp.

Every reusable preset may publish a friendly subset of these controls:

| Control | Meaning |
| --- | --- |
| `Amount` or `Opacity` | Final contribution of the effect |
| `Strength` | Amplifies or attenuates the selected signal |
| `Contrast` | Narrows or widens the mask response |
| `Bias` | Moves the selection threshold |
| `Transition` | Controls the soft boundary between regions |
| `Falloff` | Controls distance or directional decay |
| `Invert` | Exchanges affected and unaffected areas |
| `Seed` | Selects a deterministic variation |

The Inspector shows exact numeric values next to sliders. Presets can rename
published controls for their domain, such as `RustAmount`, `SnowCoverage` or
`WetCavityStrength`, without changing the underlying typed operation.

## 6. Three different blend meanings

The UI and serialized recipe must not call all combinations simply `Blend`.

### 6.1 Weight Blend

This is horizontal mixing: two surface materials occupy different parts of the
same visible surface. The normalized mask controls coverage. It is appropriate
for iron next to rust, rock next to mud or exposed metal next to paint chips.

### 6.2 Height Blend

Height Blend combines authored coverage with each input material's micro-height.
At zero height strength it must reduce exactly to Weight Blend. Increasing
`HeightStrength` lets the relatively higher material dominate within the
transition region; `Transition` controls seam width and each input may have a
height scale and bias. The weights are normalized after comparison so the
result does not brighten, darken or leave holes accidentally.

Height Blend supports three distinct authoring inputs:

1. a height texture imported with the material;
2. a procedural or geometry-derived scalar height; and
3. a height field painted directly in JericoEditor.

This is how snow settles between rock detail, mud fills lower regions, rust
pitting breaks paint and terrain layers meet without a plain linear fade.

### 6.3 Physical Layer

This is vertical layering: light encounters an upper interface before the lower
material. Coat, thin film and fuzz/dust are examples. A painted dielectric coat
over metal is not represented by merely lowering metalness.

Only layers explicitly supported by the selected Jerico master shader are
available. The graph cannot invent a new runtime lobe or arbitrary stack.

## 7. Channel-correct composition

The blend resolver owns material semantics rather than applying the same color
lerp to every channel.

- base color is combined in the engine's declared linear working space;
- metalness represents spatial mixing of valid metal/dielectric endpoints;
- roughness uses the selected master shader's declared convention;
- normals use a stable normal-composition method such as reoriented normal
  mapping, not component-wise linear interpolation;
- AO remains an occlusion term rather than a color tint;
- emissive, opacity, coat and transmission are accepted only when the selected
  master exposes compatible channels; and
- displacement or silhouette-changing height is separate from Height Blend and
  requires an explicitly admitted geometry/tessellation policy.

The Material Mask Lab diagnoses an incompatible material pair before Apply.

## 8. Painting in JericoEditor

Painting operates on a separate non-destructive layer owned by the editor
document. It never modifies the imported source texture or mesh.

The brush exposes:

- Radius;
- Strength;
- Flow;
- Hardness;
- Spacing;
- optional deterministic jitter; and
- Add, Subtract, Replace, Smooth and Flatten modes where valid.

The designer chooses `Mask` or `Height` as the target channel. Projection can be
UV, triplanar/world or target-specific terrain/BSP projection. Unsupported or
overlapping UVs are shown as diagnostics rather than silently accepted.

One stroke is one undoable transaction. Preview displays the stroke immediately,
but Apply, batch bake and regeneration remain explicit. Freeze/lock protects
manual corrections from procedural reevaluation.

## 9. JericoEditor workspace

The Material Mask Lab stays inside the unified Asset Workbench.

```text
Assets / Presets       3D Preview                    Inspector
Mask presets           classic material sphere      exposed parameters
Materials              selected mesh/BSP/terrain    source settings
Paint layers           lighting/HDRI controls       blend mode
                                                    brush controls

Graph | 2D Mask | Height | Channels | Diff | Diagnostics | Bake
```

The classic round material sphere is the default. A selected production mesh,
JericoBSP face or terrain tile can replace it without opening another tool.
Preview modes include source-only, node-only, final mask, height, affected
channels, material A, material B and final shaded result.

`Preview` is zero-write. `Apply` creates one document transaction. `Bake/Cook`
shows the exact output inventory and cost estimate before publication.

## 10. PWAS and EPLS integration

PWAS and EPLS consume the same preset/instance contract rather than duplicating
material math.

- PWAS can assign a reusable preset to generated BSP, terrain, spline or scatter
  selections and can bake per-selection masks.
- EPLS can drive bounded fields such as wetness, frost, snow, rust, moss, dirt
  and damage through declared runtime channels.
- A newly added road, cave spline or exclusion volume invalidates only affected
  generated regions, shows a diff and preserves frozen manual paint.
- Static environmental results bake. Dynamic gameplay fields use only the
  explicitly bounded layer-mask contract already budgeted by the renderer.

The system suggests; the designer decides.

## 11. Cook and runtime boundary

Static authoring resolves through the existing material pipeline:

```text
validated sources + preset + overrides + painted layers
-> deterministic evaluation
-> preview and diff
-> Apply transaction
-> baked mask/material textures (.rtext)
-> flattened material (.rmat)
```

Shipping loads the resulting `.rtext` and `.rmat`. It does not parse the graph,
run arbitrary noise networks, load MaterialX/OpenPBR authoring files or compile
shader code. MaterialX interoperability may be evaluated later as an offline
import/export adapter with its own security and dependency gate.

GPU evaluation may accelerate interactive preview. Authoritative baked outputs
must still satisfy deterministic repeatability and cross-path conformance before
GPU bake can replace a deterministic reference path.

## 12. Security and limits

- no custom code, shell, process, plugin or script node;
- strict schema, node-count, edge-count, resolution, iteration, radius, memory
  and evaluation-time bounds;
- acyclic graphs only in the initial system;
- imported images and geometry pass their normal hostile-input readers first;
- normalized project-scoped paths and stable typed asset identities;
- cancellation between bounded jobs;
- dry-run, diff, transaction and Undo/Redo for every mutation;
- no source overwrite; and
- diagnostics never embed arbitrary imported bytes or execute metadata.

## 13. Performance policy

- compile one typed evaluation plan per preset revision, not per frame;
- cache geometry-derived maps by mesh identity and settings;
- evaluate dirty tiles/regions only;
- pack compatible runtime masks into the established bounded layer channels;
- flatten static graphs during cook;
- cap active terrain/material layers per draw;
- keep preview resolution separate from final bake resolution; and
- expose estimated texture memory, bake work and shader variant impact before
  Apply/Cook.

No virtual texturing, arbitrary runtime graph interpreter or new shader variant
family is added without a measured scene that requires it.

## 14. First implementation slice

After the currently accepted M6F-a material-instance vertical and its planned
texture-workbench expansion are green, the smallest useful Mask Lab proof is:

1. one reusable preset with Noise, Curvature, Painted Mask, Remap and Combine;
2. two compatible opaque PBR material inputs;
3. Weight Blend and Height Blend;
4. Amount, Strength, Contrast, Bias, Transition, Invert and Seed controls;
5. mask/height painting with Radius, Strength, Flow and Undo/Redo;
6. classic sphere plus one selected production mesh preview;
7. Preview with zero writes, typed diff and explicit Apply;
8. deterministic `.rtext` mask plus flattened `.rmat` cook; and
9. retained screenshots proving an edge-wear/rust preset and a second non-rust
   preset use the same graph machinery.

This slice proves generality without attempting the complete node catalog.

## 15. Acceptance evidence

- repeated evaluation/cook from identical inputs is byte-identical;
- changing only Strength changes the retained mask and shaded preview;
- HeightStrength zero matches Weight Blend exactly;
- mask and height strokes Undo/Redo exactly one transaction each;
- preset changes do not mutate locked instances without approval;
- invalid graphs, cycles, oversized maps and incompatible channels reject with
  stable diagnostics;
- source assets remain byte-identical;
- runtime package contains no graph, source image, custom code or MaterialX file;
- the cooked result reloads through the production readers and renders in the
  real Vulkan viewport; and
- a readable JericoEditor screenshot shows graph, brush controls, blend mode,
  2D mask/height and final 3D result.

## 16. References

- [Adobe Substance 3D: OpenPBR overview](https://experienceleague.adobe.com/en/docs/substance-3d/general-knowledge/openpbr/openpbr-overview)
- [Academy Software Foundation: OpenPBR Surface specification](https://academysoftwarefoundation.github.io/OpenPBR/)
- `CODEX_JERICOENGINE_SHADER_MASTER.md`, especially Material Graph, PWAS/EPLS,
  Terrain and HorrorRendering layer contracts
- `docs/PROCEDURAL_PWAS_EPLS_MASTER.md`, especially Smart Materials, Terrain and
  EPLS mask/layer sources

