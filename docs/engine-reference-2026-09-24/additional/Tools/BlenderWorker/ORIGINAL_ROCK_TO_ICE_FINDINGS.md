# From original fractured rock to ice caves

Date: 2026-09-08. Scope: private addon inspection and next-family design.
This document does not claim an implemented ice-cave generator.

## What was actually learned from the purchased graph

The original `NG_Fractured_Rock_Formation_Generator` was inspected in the pinned
Blender binary after successful private control evaluation. The geometry-link
inspection ran read-only, PID 29512; no node group or BLEND was saved/exported.
The graph has 71 nodes. Its observed main geometry path is:

```text
input faces → duplicate faces → position/face scale → merge
→ subdivision → triangulation → dual mesh → position/capture
→ edge extrusion → position → edge split
→ subdivision/triangulation → second dual mesh → merge
→ face extrusion → subdivision → displacement
→ smooth/edge shading → original material
```

This is original addon structure, not a Jerico-authored algorithm. The first
extrusion is explicitly `EDGES`, the second `FACES`; duplication and scaling use
the `FACE` domain. Layer count feeds the duplication branch. Separate random
seeds drive base randomization, detail selection and extrusion. Noise has its
own seed/scale/strength/detail/roughness/lacunarity/distortion controls. Viewport
and render resolution feed separate branches of a switch.

Consequences supported by the observed graph:

- Macro structure is created by topological operations before fine displacement.
  The earlier rounded body with straight cuts was not equivalent construction.
- Polygonal regions are generated at two detail scales. More surface noise does
  not replace this layered topology.
- Face-based input and edge extrusion mean that replacing the preset source with
  an arbitrary closed BSP solid is not an established adapter. A correct source
  patch/frame convention and validation must precede that integration.
- The original modifier is reusable as-is for the demonstrated private preset.
  A new biome does not become correct merely by changing the output material.

## Ice is a distinct geometric problem

Rock caves containing ice and cavities inside glacier ice are distinct cases.
Glacier walls can have smooth melt forms and bands; rock caves can carry ice
coatings, floor accumulation, attached icicles and pillars. These distinctions
are documented by [NPS ice caves](https://www.nps.gov/subjects/caves/ice-caves.htm)
and [NPS exploring ice caves](https://www.nps.gov/articles/000/exploring-ice-caves.htm).

Selected next design, preserving the user's quality-first reuse preference:

1. **BSP-constrained host wall:** consume the existing closed solid, selected wall
   region, material/physical IDs and protected voids. Keep the playable cave layout
   unchanged. Do not use the free-standing rock preset as the cave layout authority.
2. **Continuous glacier morphology:** construct directional melt/scallop concavity
   in a flow frame at macro and meso scales. Keep the solid continuous; original
   rock cell splitting is useful for crevasses, not as a mandatory global ice pattern.
3. **Bands and deposits:** generate warped bands with metric spacing and depth,
   distinct from coloring. Local deposition follows authored accumulation masks;
   icicles require ceiling attachment and gravity direction, pillars need contact.
4. **Detail and shading:** only after silhouette and concavity are correct, add
   small roughness/frost relief and preview absorption/transmission. Keep texture
   licensing separate and do not treat a blue rock as an ice result.
5. **Masks and fidelity:** preserve source/layer provenance, ice band, melt channel,
   frost potential and physical-material identity. Evaluate all LODs against the
   high mesh; bake detail that low topology cannot retain, without erasing cavities
   or allowing collision/LOD geometry into protected space.

This is a design inference based on inspected construction and geological
references, not a claim that the purchased graph already creates ice caves.
Use its suitable original stages with attribution if adapted; implement the
missing melt/deposition stages where needed. No source-ownership relabeling.

## Next observable gates

- First integrate the original rock graph with an explicit source-patch contract:
  same authored input, repeatable seed, topology/mask provenance, protected-space
  checks and actual evaluated mesh captures. The current private preset test does
  not close this gate.
- For glacier ice, prove directional wall concavity in clay before material work.
  A thin coat of blue material, noise-only displacement or detached shards fails.
- Prove ceiling/floor attachment for accumulated ice forms and keep-clear for
  every returned representation.
- Review original-resolution high/low images and geometric error with the native
  owner's quality profile; no invented caps, alternate cooker or parallel broker.

`glacial_ice_wall` remains metadata-only and non-executable until its own geometry
implementation and these bounded tests exist. User-visible controls must reflect
that status in the future Jerico front rather than presenting a successful no-op.
