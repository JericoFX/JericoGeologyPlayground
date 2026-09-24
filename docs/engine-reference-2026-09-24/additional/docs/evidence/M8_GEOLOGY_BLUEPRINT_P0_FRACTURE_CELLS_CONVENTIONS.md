# M8 Geology Blueprint P0 FractureCells — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-FRACTURE-CELLS`
**Authority:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

These conventions bind the Step 9 CPU reference, canonical recipe, productive
Diligent/Vulkan pass, bounded inspection, mathematical tests and visible proof.
They consume the published `AnisotropicVoronoiCells` structure and do not define
another cell evaluator, field, lattice, noise or structural-channel framework.

## Input ownership and units

- An enabled `FractureCells` layer consumes the most recent enabled,
  non-zero-strength `AnisotropicVoronoiCells` output earlier in recipe order.
  A missing producer or a second consumer without a newly published producer is
  ambiguous and rejected.
- The input remains the published 16-byte structural value: stable nearest cell
  ID plus nearest, second and boundary metrics, with its separately composed
  structural mask.
- `boundaryMetricMetersSquared = second - nearest` is finite, non-negative and
  measured in squared metres. `FractureCells` converts it once to the monotonic
  metric proxy:

  ```text
  boundaryProxyMeters = sqrt(boundaryMetricMetersSquared)
  ```

  This proxy has metre units but is not claimed to be exact Euclidean distance
  to an anisotropic bisector.
- Cell ID `0`, zero upstream structural mask or a non-finite input is explicit
  background and produces no fracture effect.

## Width, softness and mask endpoints

- `widthMeters` and `softnessMeters` are finite positive metre values preserved
  in the productive Vulkan `float` domain.
- The uncomposed cell-boundary response is exactly:

  ```text
  cellCrack = 1 - smoothstep(
      widthMeters,
      widthMeters + softnessMeters,
      boundaryProxyMeters)
  ```

- Endpoint semantics are inclusive: values at or below `widthMeters` produce
  `cellCrack = 1`; values at or above `widthMeters + softnessMeters` produce
  `cellCrack = 0`; the interval between them is finite and monotonic.
- The productive fracture mask is:

  ```text
  fractureMask = cellCrack
               * clamp(anisotropicStructuralMask, 0, 1)
               * clamp(fractureLayerStrength, 0, 1)
               * clamp(fractureRegionalMask, 0, 1)
  ```

  Regional masking changes the fracture effect, never cell ownership or the
  published anisotropic metrics.

## Modes and scalar sign

Jerico keeps the existing negative-inside scalar convention and admits exactly
two modes:

- `Carve`: subtract the boundary crack from the solid with
  `crackDistanceMeters = widthMeters - boundaryProxyMeters`, then blend
  `max(sourceSignedDistanceMeters, crackDistanceMeters)` by `fractureMask`.
  Positive values therefore create real void; this is not color or random bump
  displacement.
- `Weakening`: preserve the scalar field exactly while still publishing the
  deterministic fracture mask and owner ID as reusable structural data. It does
  not implement a durability or weathering consumer.

Both enum values and every semantic payload input participate in canonical
serialization and recipe hashing. `FractureCells` first appears in schema 9;
schemas 1–8 reject it. Non-identity transforms and unknown modes reject.

## Structural outputs and normal approximation

- The existing 16-byte structural value retains the source cell ID and
  nearest/second/boundary metrics. The existing separate structural mask becomes
  `fractureMask`; no new field or channel is introduced.
- `fractureId` is the stable nearest-cell owner ID when `fractureMask > 0` and
  `0` otherwise. It is intentionally owner-sided; this slice does not add block
  separation or pair-topology identity.
- `fractureNormalApprox` is the normalized centred finite-difference gradient of
  `boundaryProxyMeters` on the admitted voxel lattice. It is orientation-only;
  exact cusps, background and gradients below the finite epsilon publish
  `(0,0,0)` rather than inventing an axis. The retained boundary metric remains
  the reusable source for later consumers; no normal channel is materialized.

## Determinism, dirty work and material behavior

- No seed, clock, pointer, random device or hidden noise participates in
  `FractureCells`. Fixed structural input and recipe state are bit-stable in the
  productive representation.
- Identical reevaluation must preserve scalar and structural hashes and report
  zero dirty voxels/leaves. A bounded width, softness or mode change must produce
  non-empty bounded dirty work when it changes the admitted output.
- `Weakening` preserves the scalar field. `Carve` changes only the scalar and the
  reused fracture structural mask; source material GUIDs remain unchanged.
- Bounded inspection may read selected values and stage hashes only. The Vulkan
  layer chain retains GPU-to-GPU scalar/structural flow and performs no hidden
  whole-field intermediate readback.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
