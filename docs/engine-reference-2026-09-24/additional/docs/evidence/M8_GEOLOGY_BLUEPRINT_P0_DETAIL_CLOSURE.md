# M8 Geology Blueprint P0 Detail Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-DETAIL`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Recipe contract:** existing schema-1 `Noise` payload; no schema bump

## Result

Jerico now evaluates bounded FBM, Ridged and small domain-warp Detail on the
real negative-inside geology scalar field. The productive Diligent/Vulkan path
and the admitted CPU reference share deterministic value noise, octave,
lacunarity, gain, seed, remap, surface-band, mask and physical-scale semantics.

The implementation:

- reuses the existing canonical `Noise` payload, recipe serialization/hash,
  value-noise primitive, scalar ping-pong fields, regional masks, trilinear
  sampling, dirty tracking, bounded inspection and Surface Nets extraction;
- admits normalized FBM and squared-ridge accumulation with one through eight
  octaves and the existing full-width seed identity;
- implements a bounded three-channel noise-domain warp that changes only the
  Detail coordinate;
- preserves the negative-inside value-displacement update for `SurfaceNormal`
  and makes `Vector` a real trilinear source-domain deformation;
- enforces explicit voxel-relative micro/meso wavelength, amplitude, clamp,
  warp and surface-band ceilings before dispatch;
- produces real scalar-field and extracted-topology changes while retaining
  source material GUIDs and unrelated Strata, AnisotropicVoronoi and Fracture
  structural channels;
- proves disabled and zero-strength Detail are no-dispatch identities whose
  field/hash, extracted counts, bounds and material attribution equal the
  inherited pre-detail result exactly.

No VDB/UI route, gradient-based P1 detail warping, persistent DurabilityField,
mineral chemistry, material transport, rockfall, colluvium, sediment, Step 14
proof recipes, P1/P2/M9/Task23 or later consumer was introduced. No external
implementation was copied and no dependency or licence record changed.

## RED to GREEN evidence

The slice began with mathematical RED contracts before production code. The
recipe target first failed because the CPU Detail reference did not exist.
Those tests froze finite deterministic FBM/Ridged output, physical frequency,
negative-inside update sign, bounded noise-domain and source-domain
deformation, distinct value-versus-domain behavior and exact zero controls.

The productive Vulkan RED then rejected FBM, Ridged and Vector Noise as
unsupported. Admitting the minimum existing payload exposed two integration
defects rather than hiding them:

- bounded structural inspection published zero AnisotropicVoronoi values after
  a later Noise layer because its persistence flag was missing even though the
  resident buffers already existed; the session now retains that live channel
  exactly like the existing Strata channel;
- an early identity assertion compared raw Surface Nets append order, which is
  not canonical because GPU atomic counters may emit equivalent vertices and
  indices in a different order. The contract was corrected to exact scalar
  field/hash plus equivalent counts, world bounds and material attribution.

The mask RED also caught replacement of the inherited C9 RBrush mask instead
of appending the localized Detail mask; preserving both masks restored the
intended composition. Final cancellation RED first stopped before the Detail
dispatch, then revealed Diligent's single-live-device rule. The final contract
uses one fresh session state and cancels at the first check after the bounded
Detail dispatch and before dense surface extraction.

Frozen conventions are recorded in:

`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_DETAIL_CONVENTIONS.md`

## Mathematical and Vulkan proof

Focused real Vulkan command:

```text
ctest --test-dir build/windows-vulkan-dev -C Debug --output-on-failure -R "^procedural\.geology_gpu_detail$"
```

Observed result:

```text
procedural.geology.p0_detail|backend=Diligent/Vulkan|device=NVIDIA GeForce RTX 3090|baseline_hash=15244624087024613006|fbm_hash=909893002338219789|ridged_hash=5065914307827062782|vector_hash=13141217158671420183|parity_max_m=3.45256e-08|ridged_dirty_voxels=4074|mask_dirty_voxels=12
```

FBM, Ridged, noise-domain warp and Vector source-domain deformation all change
the productive scalar field and extracted topology under the frozen two-voxel
amplitude/warp ceiling. Maximum selected-sample CPU/GPU deviation is
`3.45256e-08 m`. Identical reevaluation reports the same field hash and zero
dirty work; changed seed and parameters produce bounded non-zero dirty work.
The localized mask changes only 12 voxels.

Visible proof:

- `docs/evidence/images/m8-geology-blueprint-p0-detail-vulkan.png`;
- `docs/evidence/images/m8-geology-blueprint-p0-detail-vulkan.svg`;
- exact `1600x950` PNG plus matching vector source;
- panels show inherited Detail OFF fractures, FBM, Ridged and small domain warp
  as meshes extracted from evaluated GPU scalar fields;
- the same macro/meso fractured geological body remains recognizable in every
  panel; Detail does not substitute for its silhouette or structure;
- SVG SHA-256
  `62f5732b12a61c44d78aa557f88e144a7671dad8c0346a4910b9b75678655a14`;
- PNG SHA-256
  `650ed83604d31b5548af94d4b1cdca8eefc36b7ec935b19ebffe5bed5e4ef3f8`.

## Verification

Final Debug verification used the existing Windows Vulkan build products:

- direct focused build of `JericoGeologyRecipeTests`,
  `JericoGeologyGpuTopologyPlanTests` and `JericoGeologyGpuSessionTests`:
  `PASS`;
- `JericoGeologyRecipeTests.exe`: `PASS` for mathematical, deterministic,
  identity, bounds and rejection contracts;
- `JericoGeologyGpuTopologyPlanTests.exe`: `PASS`;
- updated CTest `procedural.geology_gpu_detail`: `1/1 PASS` in `73.72 s`,
  including post-dispatch cancellation;
- inherited full Vulkan session, BubbleWeathering, CurvatureWeathering and
  Detail matrix: `4/4 PASS` in `812.49 s` (`591.54 / 73.47 / 73.64 / 73.79 s`);
- focused CavityCarve Vulkan gate: `PASS`, parity `7.03335e-08 m`, inherited
  field/baseline hashes `2463036689238531135 / 4703855310837332601`;
- focused FractureCells Vulkan gate: `PASS`;
- `git diff --check`: clean for the owned publication paths.

Full CMake generation remains blocked by unrelated preserved foreign state:
missing `Qt6::Core` and `Jerico::EditorProjectAuthority` targets in other test
subtrees. The configured test project builds directly and its generated CTest
registry contains the focused Detail gate; this slice does not alter that
foreign configuration.

## Scope boundary

Completing Detail does not complete geology P0, the geology blueprint or
JericoEngine 1.0. Blueprint Step 14 (`Proof recipes`) remains the next P0 slice
and is not implemented here.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
