# M8 Geology Blueprint P0 PlaneCut Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-FOUNDATION-PLANECUT`

**Blueprint:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

**Audit checkpoint:** `a77b23d1e57ebe038d6621e00ebd75a3ad2adc5b`

## Result

PlaneCut is a schema-7 Jerico geology layer with one CPU mathematical
reference and one productive Diligent/Vulkan implementation. It composes a
negative-inside source field with

```text
plane(p) = dot(p, normalize(normalWorld)) - offsetMeters + roughness(p)
output(p) = lerp(source(p), max(source(p), plane(p)), strength * mask(p))
```

The operator modifies only the scalar channel. The existing Surface Nets path
extracts its new face and preserves the source material identity.

## RED contracts recorded before implementation

1. Bounded intermediate inspection failed to compile because
   `GeologyGpuFieldInspectionRequest`, `fieldInspection` and
   `fieldInspections` did not exist.
2. The CPU mathematical test failed to compile because
   `GeologyPlaneCut.hpp` did not exist.
3. Canonical recipe tests failed to compile because schema 7 and its PlaneCut
   contract did not exist.
4. The real Vulkan test reached the productive stack but both evaluations
   returned `procedural.geology.gpu_layer_unsupported` for PlaneCut.

These failures demonstrated absent behavior rather than parser/build-only
coverage.

## Intermediate inspection

- Explicit opt-in request; no default readback.
- One to 64 finite world-space sample positions.
- Nearest represented voxel-center snap.
- Source and every enabled non-zero layer stage record recipe index, pass name,
  two-word field hash, selected scalar values and inside/outside classification.
- Only selected 32-bit field words and the two-word hash are copied; no hidden
  whole-field readback occurs.
- The focused source-only proof used 2 selected samples and read back fewer
  bytes than one source field.

## Mathematical and identity proof

- Negative scalar means inside and zero is the surface.
- The retained half-space is `dot(p, normalWorld) <= offsetMeters`.
- Full strength uses `max(source, plane)`; zero strength is exact identity.
- The normal is normalized on CPU and before upload to GPU.
- Zero roughness is analytic and seed-independent.
- Non-zero roughness uses the existing layer-shader lattice value-noise
  formula; CPU and GPU fold the low/high seed words with XOR.
- Canonical write/parse round-trip covers every PlaneCut payload field.
- Recipe hash changes with normal, offset, roughness, frequency or header seed.
- Repeated real GPU evaluation produced the same non-zero field hash and zero
  dirty voxels/leaves.

## Real Vulkan and Surface Nets result

The proof route ran on `NVIDIA GeForce RTX 3090` through
`Diligent/Vulkan` and returned:

```text
field_hash=14376630535236132161
repeat_hash=14376630535236132161
analytic_parity_max_m=1.07288e-08
rough_parity_max_m=1.72863e-08
planar_triangles=690
planar_max_deviation_m=2.68221e-09
offset_dirty_voxels=4697/73728
offset_dirty_leaves=28/144
```

Both parity measurements are below the frozen `1e-4 m` tolerance. The offset
change is non-empty and bounded rather than invalidating the whole field.

Visible proof:

- `images/m8-geology-blueprint-p0-planecut/planecut-surface-nets.svg`
- `images/m8-geology-blueprint-p0-planecut/planecut-surface-nets.png`
  - SHA-256:
    `5B7BD6450551C0907FAFD1BF65E100BE54835354180F7739CBF3BBBD34E04998`

The SVG is generated directly from the evaluated GPU Surface Nets mesh. Orange
triangles are selected only when all three vertex X coordinates are within
`1e-4 m` of the authored `0.12 m` plane. The PNG is a 1200x700 raster rendering
of that same SVG, not generated or retouched geometry.

## Verification

Build gate:

```text
cmake --build build/windows-vulkan-editor --config Debug --target JericoGeologyRecipeTests JericoGeologyGpuTopologyPlanTests JericoGeologyGpuSessionTests --parallel 8
```

Focused proof gate:

```text
build/windows-vulkan-editor/Tests/ProceduralAuthoring/Debug/JericoGeologyGpuSessionTests.exe --planecut-proof docs/evidence/images/m8-geology-blueprint-p0-planecut/planecut-surface-nets.svg
exit_code=0
```

Integration gate:

```text
ctest --test-dir build/windows-vulkan-editor -C Debug --output-on-failure -R "^(repository\.cancelled_vdb_geology_absent|repository\.gpu_geology_boundary|procedural\.geology_recipe|procedural\.geology_gpu_topology|procedural\.geology_gpu_session)$"
5/5 passed; 0 failed; 401.48 seconds
```

The integration gate includes the complete pre-existing Vulkan geology session,
not only the new PlaneCut route.

## Provenance and scope

- No external source or shader code was copied.
- No dependency, copyright or license inventory changed.
- No VDB/OpenVDB/NanoVDB/PNanoVDB path was restored.
- No field framework, editor UI, later P0 operator, P1 or P2 capability was
  introduced.
- Existing recipe, masks, Diligent/Vulkan buffers, 8x8x8 dirty leaves, hashing,
  material identity and Surface Nets contracts were reused.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
