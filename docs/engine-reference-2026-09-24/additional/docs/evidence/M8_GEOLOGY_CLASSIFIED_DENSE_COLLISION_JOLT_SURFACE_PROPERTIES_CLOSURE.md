# M8 Geology Classified Dense Collision Jolt Surface Properties Closure

Date: 2026-08-29

Slice: `M8-GEOLOGY-CLASSIFIED-DENSE-COLLISION-JOLT-SURFACE-PROPERTIES`

Status: `PASS`

## Result

The accepted dense geology classification now survives as a bounded
Jerico-owned physical palette plus one material index per dense collision
triangle. GeologyCook writes the mapping deterministically as opt-in JRCL 1.4;
the private Jolt adapter consumes it for exact surface queries and real contact
friction/restitution. Legacy JRCL 1.0-1.3 and unclassified body-wide behavior
remain unchanged.

This slice did not add collision LOD, streaming, world cells, destruction,
editor authoring, density/mass, gameplay, Luau, events, VFX or audio.

## Retained RED chronology

1. An independently assembled JRCL 1.4 fixture, derived from the committed 1.3
   golden but carrying a new `PHYS` chunk, failed at
   `CollisionAssetTests.cpp:315` while the reader returned the required
   `runtime.rcoll.version_unsupported`.
2. GeologyCook then failed four assertions because candidate and reread
   collision assets contained neither a physical palette nor triangle indices.
3. PhysicsWorld then failed four observable contracts: both classified ray hits
   lacked a surface material, low/high restitution produced the same response,
   and low/high friction produced the same travel.
4. The first package-only command was also unavailable before the focused proof
   path was implemented and returned the existing invalid-arguments contract.

No parser-only, debug-only or body-wide-property result was accepted as GREEN.

## JRCL 1.4 contract

- Opt-in only for static triangle meshes with physical data.
- `SHAP` remains the exact version-4 static-mesh payload used by JRCL 1.3.
- Required `PHYS` version 1 stores:
  `u32 materialCount`, `u32 triangleCount`, two zero reserved words,
  one `GUID + float friction + float restitution` record per material, then one
  packed `u8` index per triangle.
- Palette size is 1-4; identities are stable, non-zero and unique; coefficients
  are finite `[0,1]`; counts and every index are validated before allocation or
  Jolt shape creation.
- Omitting the palette writes exact legacy JRCL bytes. All committed collision
  goldens remain byte-identical.

The independent two-material/four-triangle fixture equals writer output exactly,
round-trips semantically and is repeatable byte for byte. Corruption gates reject
oversized/duplicate/zero palettes, invalid coefficients, mismatched triangle
counts, non-zero reserved words, truncated payloads and out-of-range indices.

## Geology cook lineage

```text
dense GeologySurfaceMesh classification
+ MeshGeologyDescriptor material palette and response routing
+ explicit ordered physical records
-> existing classified material selection for each triangle's canonical vertex
-> one u8 physical material index per dense triangle
-> JRCL 1.4
```

The physical identities must match the active geology material palette in exact
order. Collision vertices and indices are copied only from the dense master.
Adding classified LOD1 changes neither dense collision geometry nor triangle
cardinality; no LOD1 triangle enters collision.

## Private Jolt integration

The public boundary exposes only
`RuntimeFormats::CollisionPhysicalMaterial` through the optional
`PhysicsRayCastHit::surfaceMaterial`. Jolt IDs, material pointers, subshape types
and serialized layouts remain private.

The adapter builds Jolt `IndexedTriangle` records with the cooked material index
and a private material object per bounded palette entry. Contact combination
retains Jolt's admitted semantics:

- friction: geometric mean of both selected/body values;
- restitution: maximum of both selected/body values.

An unclassified side uses its existing RigidBody coefficient. Existing box,
capsule and static-mesh tests prove that the legacy path remains valid.

## Deterministic fixed-step witnesses

Two separated classified regions use identities beginning `A0` and `C0`.
Raycasts return the exact authored records:

| Region | Friction | Restitution |
|---|---:|---:|
| Low | 0.04 | 0.10 |
| High | 0.81 | 0.90 |

At the fixed restitution checkpoint, the low/high Y positions are
`0.48963` / `2.38315`. On equivalent classified slopes with zero restitution,
the low/high friction Z positions are `6.27544` / `-2.81753`. Repeating each
world produces exactly equal query records and body transforms.

## Source-free package witness

Path at verification time:

`build/windows-vulkan-dev/Tests/Physics/Debug/ClassifiedSurfacePackageProof-20260829-02/ClassifiedSurfaces.epak`

The proof directory contains exactly one file and no loose/source input. The
package contains three Collision entries: classified floor, classified slope
and dynamic box.

```text
physics.world.classified_package_succeeded|entries=3|materials=2|triangles=8|low_y=0.48963|high_y=2.38315|low_z=6.27544|high_z=-2.81753|replay=repeatable|source_free=true
asset_dump.shipping_valid|entries=3|no_abr=true|no_brush_source_images=true|no_editor_rbrush=true|no_editorassets=true
forbidden_imports=0|dependencies=MSVCP140D.dll,VCRUNTIME140D.dll,VCRUNTIME140_1D.dll,ucrtbased.dll,KERNEL32.dll
files=1
bytes=1735
sha256=e0c961ddfc67f55476841964d9e4b14f7ca04b5209462a70ae1a2c2ddf760957
```

## Gates

```text
physics.world_jolt        Passed
runtime_formats.rcoll     Passed
procedural.sdf_cook       Passed
physics.room_source_free             Passed
physics.character_room_source_free   Passed
physics.constraint_room_source_free  Passed
```

The final three focal tests completed in 1.44 seconds. The three Vulkan source-free
proofs completed in 94.75 seconds with their existing zero-tolerance repeat
capture comparisons and Shipping/import checks.

## Completeness result and required expansion

The append-only audit in `M8_GEOLOGY_SYSTEM_COMPLETENESS_AUDIT.md` records the
full applicability matrix and deferred consumers. The user-required general
material direction is now explicit:

> Every Jerico material must be able to declare explicit physical properties
> and make cooked collision/runtime contacts behave according to the selected
> surface, through one authoring -> cook -> package -> physics route.

That general material/model authoring and propagation is
`INTEGRATION REQUIRED`; this slice proves the architecture only for classified
dense geology collision. A later dedicated vertical must cover generic material
and import settings, editor validation/preview/batch/reimport, supported
collider/model cooks, package preservation, query exposure and measured runtime
behavior. It must not be represented as already implemented by this closure.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
