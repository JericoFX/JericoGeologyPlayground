# M8 Geology Blueprint P0 FaultDisplace Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-FAULT-DISPLACE`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Schema:** 8 (unchanged)

## Result

Jerico now evaluates one deterministic world-space `FaultDisplace` by
deforming where the existing scalar field and reusable structural StrataField
are sampled. The positive side of an explicit fault plane samples along an
explicit normalized slip direction; the negative side remains unchanged and a
finite cubic-Hermite transition connects them. This is coordinate deformation,
not random bump and not an additive SDF term.

The admitted implementation:

- normalizes the authored fault normal and slip direction robustly;
- uses meters for plane offset, displacement, transition width and boundary
  roughness;
- makes zero displacement, strength or regional mask exact identity before
  noise or resampling;
- folds the full-width seed into the existing three-octave Jerico FBM path;
- trilinearly samples the previous scalar field with the published positive
  background;
- trilinearly samples an existing StrataField at the same deformed coordinate,
  with structural `(0,0)` background;
- reuses the existing structural channel through bounded ping-pong storage;
- composes the admitted regional mask without moving the plane or roughness;
- preserves source material GUIDs;
- exposes only bounded selected samples and stage hashes;
- tracks changes through the existing exact dirty-voxel and `8x8x8` leaf path.

No VDB/OpenVDB/NanoVDB/PNanoVDB route, replacement field/noise/channel
framework, removed UI, AnisotropicVoronoi, FractureCells, CavityCarve or later
consumer was introduced. The implementation is Jerico code and copies no
external source.

## TDD evidence

The slice began with mathematical RED contracts. Recorded failures were:

1. the CPU recipe-test target failed because
   `GeologyFaultDisplace.hpp` did not exist;
2. the recipe test then failed to compile because
   `GeologyLayerKind::FaultDisplace` did not exist;
3. the topology RED observed identical leaf and voxel capacities before and
   after an enabled fault displacement;
4. the productive Vulkan RED returned
   `procedural.geology.gpu_layer_unsupported` for FaultDisplace.

A final read-only review found that otherwise-valid positive values below the
Vulkan `float` range could collapse to zero during constant packing. Six added
RED cases reproduced acceptance of underflowing plane offset, displacement,
transition width, roughness and frequency, plus overflowing displacement.
Schema validation now rejects before conversion any authored non-zero
dimensional scalar outside the normal productive float domain, while plane/slip
axes are still normalized robustly before packing.

The resulting GREEN contracts cover robust axis normalization, exact identity,
finite transition and both sides, expected source sampling, strength/mask
composition, deterministic bounded roughness, schema-8 canonical round-trip and
hash identity, schemas 1-7 rejection, conservative topology expansion,
trilinear scalar/structural parity and background, material preservation,
repeat identity, seed sensitivity and bounded dirty work.

## Mathematical and runtime proof

The frozen formula and units are recorded in
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_FAULT_DISPLACE_CONVENTIONS.md`.

Focused real Vulkan command:

```text
JericoGeologyGpuSessionTests.exe --fault-displace-proof docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_FAULT_DISPLACE_VULKAN.svg
```

Observed result:

```text
fault_displace|backend=Diligent/Vulkan|device=NVIDIA GeForce RTX 3090|field_hash=2045260120188572808|structural_hash=5254059935579691644|scalar_parity_max_m=1.78814e-08|structural_parity_max=1.63913e-07|background_m=0.48|dirty_voxels=16200|dirty_leaves=88|total_leaves=180
```

Both parity maxima are below the frozen `1e-4` tolerances. Repeating identical
input preserved scalar and structural hashes and reported zero dirty work. Zero
displacement produced scalar and structural hashes identical to the preceding
Strata stage. Changing displacement from `0.24 m` to `0.26 m` changed both
identities with non-empty bounded work: `16200` voxels and `88/180` leaves.
The CPU mathematical contracts independently prove that changing the folded
full-width seed changes the coordinate deformation and that strength and the
admitted regional mask compose multiplicatively without moving the fault.

The final in-bounds edge probe displaced beyond the lattice and returned the
published scalar background `0.48 m` and structural `(0,0)`. All extracted
triangles retained the source document material GUID.

Visible proof:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_FAULT_DISPLACE_VULKAN.svg`;
- two real bounded Vulkan structural-mask inspections over the same `8x8`
  world-space slice, before and after FaultDisplace;
- 64 selected samples per evaluation, never a whole-field intermediate
  readback.

## Verification

Focused coherent MSVC targets were built in dependency order:

```text
JericoProceduralAuthoring
JericoGpuGeologyDiligent
JericoGeologyRecipeTests
JericoGeologyGpuTopologyPlanTests
JericoGeologyGpuSessionTests
```

Focused executables:

```text
JericoGeologyRecipeTests.exe                         PASS
JericoGeologyGpuTopologyPlanTests.exe                PASS
JericoGeologyGpuSessionTests.exe --fault-displace-proof ...  PASS
JericoGeologyGpuSessionTests.exe --strata-field      PASS
JericoGeologyGpuSessionTests.exe --directional-warp  PASS
```

The inherited StrataField regression retained structural hash
`3141405104561149360` and parity `1.5731e-07`. The inherited DirectionalWarp
regression retained field hash `16314243348879344241`, exact repeat identity and
parity `2.44873e-07 m`.

The complete five-gate integration result is recorded in the published commit
and handoff checkpoint. The final matrix passed without changing its inherited
timeout:

```text
repository.cancelled_vdb_geology_absent  PASS
repository.gpu_geology_boundary          PASS
procedural.geology_recipe                PASS
procedural.geology_gpu_topology          PASS
procedural.geology_gpu_session           PASS (537.56 s)
100% tests passed, 0 tests failed out of 5
Total Test time (real) = 538.08 sec
```

The monolithic gate reuses one Vulkan session for the consecutive StrataField
and FaultDisplace contracts, then destroys it before later independent Vulkan
tests. The focused StrataField and FaultDisplace modes still create and own
independent sessions.

## Scope boundary

Completing FaultDisplace does not complete geology P0, the geology blueprint or
JericoEngine 1.0. Blueprint Step 8 (`AnisotropicVoronoi`) remains the next P0
slice.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
