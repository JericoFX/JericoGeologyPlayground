# M8 Geology Blueprint P0 CavityCarve — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-CAVITY-CARVE`
**Authority:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

These conventions bind the Step 10 CPU reference, canonical recipe, productive
Diligent/Vulkan pass, bounded inspection, mathematical tests and visible proof.
They reuse Jerico's existing primitive, negative-inside field-CSG, regional-mask,
sampling, dirty-leaf, structural-channel and Surface Nets routes.

## Admitted geometry, transform and units

- Schema 10 admits exactly one explicit `Sphere` cavity. `radiusMeters` and
  `smoothnessMeters` are finite metre-domain values admitted to and evaluated in
  the productive Vulkan `float` domain; ordinary IEEE-754 rounding applies.
- The sphere centre is `layer.header.transform.translation` in world metres.
- Sphere rotation has no observable meaning, and non-uniform scale would make
  the payload ambiguous. Therefore rotation must be identity and scale must be
  `(1,1,1)`; radius is owned only by `radiusMeters`.
- `radiusMeters` is finite and positive. `smoothnessMeters` is finite and lies
  in `[0, radiusMeters]`. Translation must be finite and float-preserved.
- The primitive is deterministic and consumes no random input, so the canonical
  layer seed must be zero. A nonzero unused seed rejects instead of creating a
  different identity for identical geometry.
- Splines, swept capsules, boxes and arbitrary cross-sections are not admitted
  by this slice. They can be added later through the existing primitive/field-CSG
  architecture or the separate `CaveSweep` route without changing this sphere
  contract.

## Signed distance and subtraction

Jerico retains the existing negative-inside scalar convention. For sample
position `p`, centre `c`, radius `r`, source field `S` and smoothing width `k`:

```text
sphereSdf(p) = length(p - c) - r
complement(p) = -sphereSdf(p)
```

Hard subtraction (`k = 0`) is the existing field-CSG difference:

```text
carved = max(S, complement)
```

Positive values inside the primitive therefore become real void. The analytic
sphere boundary is exactly `length(p - c) = r`; exterior samples for which the
complement does not dominate remain unchanged.

Smooth subtraction (`k > 0`) reuses Jerico's polynomial smooth maximum:

```text
h = max(k - abs(S - complement), 0) / k
carved = max(S, complement) + h*h*k/4
```

The hard endpoint avoids division by zero. The smooth term is finite, bounded
to the stated metre width and becomes exactly the hard maximum outside its
blend interval. Productive scalar storage clamps only to the existing finite
background band; it does not invent a cavity value outside that representation.

## Strength, regional mask and channels

The final field sample is:

```text
weight = clamp(layerStrength, 0, 1)
       * clamp(existingRegionalMask(p), 0, 1)
output = lerp(S, carved, weight)
```

Zero strength is the explicit unchanged control. Regional masking uses the
existing compiled mask program and GPU mask buffers; this slice adds no mask
representation or evaluator. The Vulkan regression exercises the admitted
localized `RBrushLocalField` mask and proves non-empty bounded masked work plus
exact restoration after the mask is removed.

`CavityCarve` changes only the scalar field. It preserves source material GUIDs
and every unrelated structural scalar, mask, cell ID and metric exactly. No
durability, weathering or material-variation channel is introduced.

## Identity, dirty work and inspection

- Schema 10 serializes the explicit `Sphere` tag, translation, radius,
  smoothness, strength and mask reference through the existing canonical recipe
  writer/hash. Schemas 1–9 reject `CavityCarve`; unknown shapes and invalid,
  non-finite, degenerate or ambiguous state reject.
- Fixed source and recipe state produce stable field identity. Identical
  reevaluation reports zero dirty voxels and leaves.
- Translation, radius, smoothness or mask changes that alter the output produce
  non-empty bounded dirty work in the existing `8x8x8` leaves.
- Bounded inspection reads only selected samples and stage hashes. Scalar and
  mesh data remain GPU-to-GPU between stages; the visible proof is derived from
  the final evaluated meshes without whole-field intermediate readback.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
