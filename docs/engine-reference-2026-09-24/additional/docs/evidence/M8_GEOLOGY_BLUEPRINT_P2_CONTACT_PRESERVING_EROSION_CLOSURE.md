# M8 Geology Blueprint P2 Contact-Preserving Erosion Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-CONTACT-PRESERVING-EROSION`

**State:** `PASS`

**Selected boundary:** `BOUNDED_EXPLICIT_CONTACT_DAMPED_EROSION`

**Persistent Goal:** `ACTIVE until full JericoEngine 1.0 closure`

## Result

Jerico now admits one schema-23 cumulative geology layer that consumes one to
eight explicit, unique and canonically ordered world-space contact witnesses.
It erodes only the admitted inside-surface band in one finite pointwise pass,
returns the input field exactly inside every preservation radius, blends over a
finite transition width and publishes useful seed-sensitive wear through the
existing Vulkan field, erosion classification, Surface Nets, mesh, collision,
cook, package and static runtime consumers.

This is not contact discovery, a stability solver, generic neighborhood
erosion, mutable runtime destruction or a second geology pipeline. The designer
supplies the contacts and explicitly accepts a recipe after immutable
preview/diff/cancel.

## Research-first provenance and independent design

The research record is
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_CONTACT_PRESERVING_EROSION_RESEARCH.md`.
It studies the contact-retention problem in Peytavie et al., *Arches: a
Framework for Modeling Complex Terrains* (2009), including the reason a complete
contact set matters and the compactly supported attenuation idea. Jerico does
not copy its Voronoi construction, contact search, neighborhood erosion,
weathering system or implementation.

The frozen independent contract is
`docs/superpowers/specs/2026-08-29-m8-contact-preserving-erosion-design.md`.
It selects explicit authoring witnesses and a single analytic damped-removal
pass because they are the smallest coherent inputs and termination boundary
that deepen the already admitted field vertical without reopening closed P2.

## Observable RED chronology

The first production RED added a real `Source -> Rebuild ->
ContactPreservingErosion` Vulkan mode plus CPU recipe/invariant tests before any
production implementation. The direct pre-generated Release build failed as
expected because schema 23, layer kind 25, the typed payload, analytic evaluator
and GPU integration did not exist. The tests already required exact protected
samples, useful outside wear, CPU/GPU parity, identity sensitivity,
classification, cancellation, hard-budget rejection, deterministic cook and a
consumed runtime product. Only then was the minimum production path added.

## Frozen contract

- input: `1..8` finite, exactly float-preserved, strictly lexicographic unique
  world-space contacts;
- preservation: for any point within any authored preservation radius,
  attenuation is exactly zero and the layer returns the input SDF bit-exactly;
- erosion domain: input `d <= 0` within one finite inside-surface band;
- operation: one deterministic pointwise removal using existing layer
  strength, mask and full-width seed;
- finite termination: one pass over the already admitted active voxels;
- identity: canonical schema-23 TOML and existing recipe hash cover ordered
  contacts, all five scalars, seed, header, strength and mask;
- ceilings: 8 contacts, 67,108,864 contact-distance evaluations and 128 retained
  contact bytes, all checked before Vulkan allocation/dispatch;
- authoring: pure typed diff plus existing immutable preview, cancellation and
  explicit designer acceptance; no automatic apply.

## Implementation and integration

- schema 23 adds only `GeologyLayerKind::ContactPreservingErosion = 25` and one
  typed payload to the existing recipe variant, parser, canonical writer,
  validator and hash path;
- a shared CPU reference owns the preservation equation, pure diff and checked
  work admission;
- the existing per-layer Vulkan constant buffer carries a fixed 128-byte contact
  block. This avoids adding a storage-buffer descriptor and preserves the
  backend's 16-dynamic-storage-buffer limit;
- the existing layer shader performs the same finite analytic operation and
  writes the established erosion classification;
- diagnostics expose exact contact count, work items and retained bytes;
- existing classified Surface Nets, JRMS/JRCL cook, package VFS and static PBR
  runtime consume the result unchanged.

An early GPU implementation used one additional dynamic storage buffer. Diligent
correctly rejected it because the resulting 17 descriptors exceeded Vulkan's
limit of 16. Moving the maximum-eight contacts into the existing layer constant
buffer removed that descriptor and is the final bounded solution; no global
resource-limit or framework change was made.

## Release verification

Direct Release builds used the existing generated projects with project
references disabled so unrelated foreign CMake changes could not regenerate or
enter this slice. Both focused executables returned zero.

The Vulkan field witness was:

```text
recipe_hash=6736422756677667907
field_hash=3144543574638455090
protected=-0.08 -> -0.08
outside_1=-0.08 -> 0.0330959
outside_2=-0.08 -> 0.0414783
```

The deterministic cook reported:

```text
source_hash=18089867140996499955
recipe_hash=6736422756677667907
final_field_hash=3144543574638455090
surface_hash=3144543574638455090
vertices=3744
indices=22440
mesh_bytes=300202
collision_bytes=134946
contact_count=1
work_items=147456
retained_contact_bytes=16
protected_before=-0.08
protected_after=-0.08
outside_delta=0.121478
```

The six-entry source-free package is 2,302,226 bytes with SHA-256
`6934F15AB14AC0A0CE96A1278BB55A0676CA4C38C6E59342ED6B310A54E28EA0`.
Shipping validation reports no ABR, brush source images, editor RBrush or
EditorAssets.

Two package-only 1280x720 Release Vulkan replays on RTX 3090 each reported one
draw, one material, three native textures, zero fallback and
`source_free=true`. Their original-resolution frames are byte-identical with
SHA-256
`F75C9D1118D5A7B1D749F777FA51A259318B009002EBF6DF8C9FD1FDC759FE7A`.
The oblique Beauty view visibly separates the retained circular contact plateau
on the bright right face from the eroded surrounding surface:

![Contact-preserving erosion Vulkan Beauty](images/m8-geology-blueprint-p2-contact-preserving-erosion/contact-preserving-erosion-vulkan.bmp)

Machine-readable metrics are retained in
`docs/evidence/data/m8-geology-blueprint-p2-contact-preserving-erosion-release.txt`.

## End-to-end system completeness and data lineage

```text
schema-23 explicit contacts + five scalars + seed/strength/mask
-> canonical recipe serialization and identity
-> checked topology/contact/work/byte admission
-> 128-byte contact block in existing per-layer constants
-> resident Vulkan SDF ping-pong + erosion classification
-> classified Surface Nets
-> JRMS 1.1 GEOL + exact JRCL triangles
-> six-entry source-free package
-> static Vulkan PBR visual result and static collision consumer
```

The useful upstream information that survives is: ordered contact identity in
the authoring recipe; erosion classification on the cooked mesh; exact final
triangles for rendering/collision; and recipe/source/final-field/surface hashes
plus work/byte diagnostics for reproducibility. Authored contact coordinates do
not enter Shipping because the static runtime consumes the accepted cooked
result, not source/editor state.

Applicable and proven categories are source/input, recipe serialization,
identity, CPU reference/diff/admission, GPU field processing, renderer/material,
static collision, cook/package, failure handling, cancellation, debugging,
budget diagnostics, tests and documentation. Animation, Sequencer and runtime
simulation do not apply to this static authoring slice.

### INTEGRATION REQUIRED

| Subsystem | Required data/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Dedicated editor transaction/UI | Typed payload, pure diff, existing preview/cancel/apply transaction | JericoEditor geology authoring; editor vertical | Later Geology editor integration |
| Physical contact authoring/properties | Stable explicit contact producer and geology-to-Jolt material/property contract | BSP/placement tools plus Jolt integration | Production Geology/physics |
| Classified LOD/world-cell streaming | Erosion classification preservation across simplification and cell cook | Geometry LOD and World Cell owners | Production streaming |
| Mutable runtime geology/destruction/save | Stable modification identity, remesh/collision replacement and save schema | Future destruction/runtime modification owner | Post-blueprint production expansion |
| Navigation/gameplay/Luau/events/particles/audio | Read-only cooked erosion/contact-derived semantic query contract | Respective downstream subsystem owners | Feature-driven production slices |

No deferred integration is silently claimed complete. Dorsey narrow-band
mineral weathering, moisture transport, mineral dissolution and multi-scale
microstructure synthesis remain the four ordered P2/research features. The
complete Geology blueprint must close before the PWAS/EPLS production expansion
begins.

Completing this slice does not complete P2, the Geology blueprint, the later
production expansion or JericoEngine 1.0.

Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`
