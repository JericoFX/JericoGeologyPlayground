# M8 F4 heightmap reference comparison

Date: 2026-09-05. Scope: bounded READ_ONLY research requested by the owner,
inside the existing F4 slice. Jerico baseline: `6de05d9164907af55f7b524715360e2f4a67c72b`.
This is a diagnosis and reference comparison, not implementation or Beauty acceptance.

## What can be verified upstream

[GeoGen's official page](https://jangafx.com/software/geogen) describes GPU terrain
editing, grid/particle simulations and separate mesh, mask, normal and heightmap
exports. Its [documentation landing page](https://docs.jangafx.com/geogen/)
still says the documentation is in progress. These sources establish the workflow,
not the equations or remesher implementation. No public GeoGen implementation was
found in this bounded review; do not infer its geometry library from suite-wide
dependency notices or product screenshots.

[HighMap](https://github.com/ottolink-dev/HighMap) is the C++ heightmap solver behind
Hesiod. Its README declares GPLv3. Inspected upstream revision:
`0ab56de683b1a9ddaacbaba569e6309b2ff9f0b7` (GitHub API main resolution).
It remains a reference, not an admitted dependency; no upstream code was copied,
translated, compiled or installed into Jerico.

### Strata change shape before texturing

HighMap's [strata kernel](https://github.com/ottolink-dev/HighMap/blob/0ab56de683b1a9ddaacbaba569e6309b2ff9f0b7/HighMap/src/gpu_opencl/kernels/strata.cl)
remaps height within successive bands, with an oriented shift, adjustable profile,
multiple scales and optional masks. Its
[terrace variant](https://github.com/ottolink-dev/HighMap/blob/0ab56de683b1a9ddaacbaba569e6309b2ff9f0b7/HighMap/src/gpu_opencl/kernels/strata_terrace.cl)
varies the profile by band and corrects its frequency for the configured slope.
These write the height field itself. The useful concept is coherent, directional
relief driven by authored strata; the output is not merely a striped color map.
This observation does not validate geological realism or numerical robustness of
every upstream parameter combination.

### Work at higher resolution before reducing the result

HighMap's [upscale_amplification](https://github.com/ottolink-dev/HighMap/blob/0ab56de683b1a9ddaacbaba569e6309b2ff9f0b7/HighMap/src/multiscale/upscale_amplification.cpp)
successively resamples to power-of-two larger grids using bicubic interpolation,
applies an operation with a persistence-scaled strength, then returns to the
original dimensions using bilinear interpolation. The
[hydraulic wrapper](https://github.com/ottolink-dev/HighMap/blob/0ab56de683b1a9ddaacbaba569e6309b2ff9f0b7/HighMap/src/erosion/hydraulic_stream_upscale_amplification.cpp)
uses erosion at each scale. Its masked overload blends the processed result with
the original afterwards; it does not constrain each intermediate erosion pass.
The wrapper does not request bedrock/moisture or auxiliary erosion-map outputs.
This supports studying a distinct working-resolution stage before final reduction,
but does not establish a mesh subdivision algorithm, closed-solid topology,
categorical-field interpolation or a measured performance improvement in Jerico.

### Erosion and deposition maps need explicit meaning

The [erosion_maps helper](https://github.com/ottolink-dev/HighMap/blob/0ab56de683b1a9ddaacbaba569e6309b2ff9f0b7/HighMap/src/erosion/erosion_maps.cpp)
derives two nonnegative maps from before/after height differences and a tolerance.
These are net surface changes; they cannot recover the intervening transport
history. Jerico should retain its existing process fields and classifications
through extraction rather than replace them with a final-shape difference.

## Comparison with the actual Jerico stone

The existing GPU Strata operator already computes a shared directional field and
can apply shape displacement and groove depth (`GeologyGpuShaders.hpp:2778-2817`).
The current dense member intentionally has both displacements zero. More crucially,
`validateWorkingRegionOperators` admits only nondisplacing Strata: the dense
Hermite boundaries and region reference describe source triangles, one analytic
block and exact cuts, not the displaced surface. Enabling the existing shape
parameters alone would violate that correspondence. The existing test
`preservesNewCutRegionsInTheWorkingReference` explicitly checks this rejection
(`Tests/ProceduralAuthoring/GeologyGpuTopologyPlanTests.cpp:1138-1142`).

The beige witness is a separate authored-input limitation. The export deliberately
uses three constant 1x1 textures and one calibrated diagnostic PBR material
(`Tests/Editor/GeologyRockFamilyTests.cpp:742-777`), one palette slot, no response
routes and zero exposure/curvature controls. Its native log loads the geological
shader permutation. There is no evidence here of a disabled geological shader.
Geometry classifications survive cook, but these diagnostic inputs cannot
demonstrate the required authored geological palette.

The local thesis `C:/Users/Jerico/Desktop/FULLTEXT01.pdf`, PDF pages25/36/37,
likewise distinguishes geometry needed for contour from fine material detail.
It reports limitations of sphere inflation and incomplete corner chipping;
neither is evidence for replacing the current engine with a rounded base generator.

## Bounded consequence for F4

An ordinary heightmap assigns one elevation to each planar coordinate. A closed
rock can have multiple surfaces along that same vertical line. Therefore these
heightmap kernels are useful references for fields and authored controls, not a
drop-in solution to the closed-solid crossing/provenance problem.

The next structural RED must place a nonzero admitted Strata shape operation
before an exact cut and require changed prefix geometry, closure, every retained
source/cut region, shared Strata/Durability data and deterministic regeneration.
The current operator-rejection test remains valid until the missing boundary
correspondence is implemented. Do not disable that rejection, silently reorder
layers, reduce authored cuts or add fine noise to conceal the unsupported step.
Reusing a generated pre-cut boundary and the existing native finalizer is a seam
to evaluate, not an implemented or accepted design in this research increment.

INTEGRATION REQUIRED, F4 owner: dense geological surface generation must carry the
shape-modified boundary and provenance into final remesh; cook/material/collision
must consume the same result. Existing editor controls/publication, localized
RBrush painting, actual-member LOD and hardware budgets retain their active F4
gates. This research adds no runtime API, dependency, serialization or shader.

## Verification

Built the existing `JericoGeologyGpuTopologyPlanTests` target in
`build/windows-vulkan-editor`, Debug, `/m:1 /nr:false`: exit0,14.281s.
CTest `^procedural.geology_gpu_topology$`: exit0,1/1 passed,2.451s.
Logs: `f4-heightmap-research-topology-build.log` and
`f4-heightmap-research-topology-test.log` in that build tree.
This verifies the existing safeguards, including rejection of unsupported Strata
displacement; it does not prove that displacement is implemented in the dense path.
No source changes or new native images belong to this research increment.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
