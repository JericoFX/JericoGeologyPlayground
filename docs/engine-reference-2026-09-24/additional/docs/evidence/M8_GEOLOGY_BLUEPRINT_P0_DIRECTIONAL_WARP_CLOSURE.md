# M8 Geology Blueprint P0 DirectionalWarp Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-DIRECTIONAL-WARP`

**Blueprint:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

**PlaneCut checkpoint:** `6b581c1514fdbd596e02c0a48c5f04b0f7c1eeae`

## Result

DirectionalWarp is a schema-8 Jerico geology layer with a small CPU reference
and one productive Diligent/Vulkan implementation. It performs domain sampling:

```text
q = p + normalize(directionWorld)
        * amplitudeMeters * strength * mask(p)
        * fbm(p * frequencyPerMeter, seed)
result(p) = source(q)
```

It does not add noise to the scalar value. The operator samples the preceding
linear scalar field at `q`, preserves source material identity/channels and
continues through the existing Surface Nets path.

## Frozen conventions

The full contract is recorded in
`M8_GEOLOGY_BLUEPRINT_P0_DIRECTIONAL_WARP_CONVENTIONS.md` before production
implementation. In summary:

- direction and sampling positions are world-space;
- amplitude is meters and anisotropic frequency is cycles per meter;
- the authored direction is normalized before evaluation/upload;
- seed low/high words are folded by XOR and each octave advances by the
  established Jerico lattice increment;
- previous-field sampling is trilinear in voxel-center coordinates;
- each out-of-bounds interpolation corner returns the established positive
  background `4 * voxelSize` (`0.48 m` in this proof);
- strength, amplitude or mask weight zero is exact identity;
- bounded stage inspection remains opt-in and reads selected scalar words plus
  stage hashes, not whole intermediate fields.

## RED contracts recorded before implementation

1. The mathematical tests failed to compile because
   `GeologyDirectionalWarp.hpp` did not exist.
2. Schema-8 round-trip and validation tests reached the recipe code and failed
   because DirectionalWarp was not admitted, serialized or parsed.
3. The real Vulkan test created Diligent/Vulkan on the target GPU, then both
   evaluations returned no result because DirectionalWarp was unsupported.

These failures demonstrated absent math, identity and productive GPU behavior.

## Mathematical and identity proof

- Zero strength and zero mask weight return the source value exactly.
- Scaling the authored direction does not change the result.
- Repeated fixed-seed evaluation is exact; changing the seed changes the
  analytic result.
- An analytic source proves only the authored `+X` sample coordinate moves.
- Canonical write/parse round-trip covers direction, amplitude, anisotropic
  frequency, octave count, lacunarity and gain.
- Recipe hash changes for every payload field and the full-width header seed.
- Schema 7, zero direction, negative amplitude, non-positive frequency,
  excessive octaves and non-identity transform are rejected.

## Real Vulkan and Surface Nets result

The proof route ran on `NVIDIA GeForce RTX 3090` through `Diligent/Vulkan`:

```text
source_hash=4703855310837332601
field_hash=16314243348879344241
repeat_hash=16314243348879344241
parity_max_m=2.44873e-07
directional_bounds_change_m=0.307974
amplitude_dirty_voxels=12702/73728
amplitude_dirty_leaves=86/144
background_sample_m=0.48
```

The CPU/GPU deviation is over 400 times below the frozen `1e-4 m` tolerance.
Identical reevaluation reports zero dirty voxels and leaves. A bounded amplitude
change produces non-empty, bounded dirty work. Zero amplitude reproduces the
source field hash exactly. Every extracted triangle retains the source material
GUID.

Visible proof:

- `images/m8-geology-blueprint-p0-directional-warp/directional-warp-surface-nets.svg`
  - SHA-256:
    `214B4A3219844BCC90D956A9FCD2460C5055DCC707F35772C42EE71DEAEAC467`
- `images/m8-geology-blueprint-p0-directional-warp/directional-warp-surface-nets.png`
  - SHA-256:
    `925CDFBD0271BB9C83712F2C92C9C44FCF8B419A21DD7CF9E31107AD8AF05799`

The SVG compares the source and DirectionalWarp Surface Nets meshes produced by
the evaluated GPU fields. The PNG is a 1200x700 raster rendering of that SVG;
neither image contains synthetic or retouched geometry.

## Verification

Build gate:

```text
cmake --build build/windows-vulkan-editor --config Debug --target JericoGeologyRecipeTests JericoGeologyGpuTopologyPlanTests JericoGeologyGpuSessionTests
exit_code=0
```

Focused mathematical/recipe gate:

```text
build/windows-vulkan-editor/Tests/ProceduralAuthoring/Debug/JericoGeologyRecipeTests.exe
exit_code=0
```

Focused real Vulkan proof gate:

```text
build/windows-vulkan-editor/Tests/ProceduralAuthoring/Debug/JericoGeologyGpuSessionTests.exe --directional-warp-proof docs/evidence/images/m8-geology-blueprint-p0-directional-warp/directional-warp-surface-nets.svg
exit_code=0
```

Integration gate:

```text
ctest --test-dir build/windows-vulkan-editor -C Debug --output-on-failure -R "^(repository\.cancelled_vdb_geology_absent|repository\.gpu_geology_boundary|procedural\.geology_recipe|procedural\.geology_gpu_topology|procedural\.geology_gpu_session)$"
5/5 passed; 0 failed; 466.33 seconds
```

The integration gate includes the complete inherited Vulkan geology session,
not only DirectionalWarp.

The separate `windows-vulkan-dev` tree attempted an automatic regeneration and
was blocked by its pre-existing editor dependency configuration (`Qt6::Core`
and `Jerico::EditorProjectAuthority` unavailable there). No source change was
made for that unrelated configuration; all binding gates ran in the inherited
`windows-vulkan-editor` tree.

## Provenance and scope

- No external source or shader code was copied.
- No dependency, copyright or license inventory changed.
- No VDB/OpenVDB/NanoVDB/PNanoVDB path or replacement field framework was
  introduced.
- No removed geology UI, StrataField, later P0 operator, P1 or P2 capability
  was introduced.
- Existing recipe, lattice noise, masks, Diligent/Vulkan linear fields,
  bounded inspection, field hashing, 8x8x8 dirty leaves, material identity and
  Surface Nets contracts were reused.

DirectionalWarp closes only blueprint P0 Step 5. It does not complete all P0
operators, the blueprint or JericoEngine 1.0.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
