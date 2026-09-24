# M8 Geology Blueprint P0 PlaneCut — Step 1 Audit

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-FOUNDATION-PLANECUT`
**Baseline:** `77e9ca89f5fe07b6b6811dedad2e866a4996715b`
**State:** `PASS` for blueprint Step 1 only; no PlaneCut implementation exists yet

## Scope and authority

This is the required post-cleanup inventory from Step 1 of
`docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`.
It classifies the current VDB-free repository before any new operator code is
written. `OpenVDB`, `NanoVDB`, `PNanoVDB`, the removed Geology Playground and
all foreign work listed in `docs/work/HANDOFF.md` are outside this audit's
implementation path.

Classification:

- `Implemented and usable`: directly reusable by PlaneCut.
- `Implemented but hidden`: usable machinery without a current authoring surface.
- `Partial`: a relevant seam exists but the required PlaneCut behavior is absent.
- `Missing`: the slice must add the behavior.
- `Duplicate`: a parallel abstraction that must not be extended.

## Required A-J inventory

| Area | Current implementation | Classification | PlaneCut decision |
| --- | --- | --- | --- |
| A. Field/SDF architecture | `SparseSdfField` owns generic 8-cell bricks and signed samples in `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfField.hpp`; the productive geology route owns a separate linear sequence of 512-voxel leaves through `GeologyGpuSession`. GPU layer and Surface Nets code classify `distance < 0` as inside. | Implemented and usable | Extend the existing linear geology field. Do not add VDB, another sparse field, another voxel format or another mesher. PlaneCut is scalar implicit-field intersection under the existing negative-inside convention. |
| B. Noise math | `ProceduralFieldRecipe` plus `ProceduralFieldSampler` and the private FastNoise2 backend provide deterministic OpenSimplex2/Perlin/Cellular and None/FBM/Ridged CPU sampling. The existing layer shader already contains Jerico-owned deterministic lattice value-noise/FBM and seed mixing. | Partial | Reuse the existing GPU value-noise formula for optional plane roughness and add only its small CPU semantic reference required for parity. Do not add a noise framework or copy `proc-rock` GPL code. |
| C. Recipe and material channels | `GeologyStackRecipe`, `GeologyLayerHeader`, the tagged `GeologyLayerPayload`, validation, TOML read/write and canonical recipe hashing are current. Strata data already names material-class, hardness and erodibility ranges; heightfield erosion carries hardness/erodibility. The scalar GPU layer field does not carry a general multi-channel buffer, and generated triangles currently retain the source GUID as material identity. | Implemented and usable for ordered scalar operators; material propagation is partial | Add PlaneCut as a normal layer payload in the existing recipe and schema. PlaneCut changes only the scalar/density channel; it preserves the existing source material identity and does not invent a second material field. |
| D. GPU geology evaluator | `GeologyGpuSession::evaluate` uses Diligent/Vulkan, bounded workload admission, linear leaf buffers, an ordered layer shader, GPU duration queries, field hashing, dirty comparison and resident Surface Nets. Current admitted layers are Rebuild, CliffShelf, Noise, Strata and SurfaceWeathering. | Implemented and usable; PlaneCut missing | Add one opcode/payload packing branch and one HLSL semantic branch to the current layer pass. Do not add a shader VM or a second evaluator. |
| E. CPU reference math | CPU references exist for generic procedural noise, hydrology/erosion and the macro-heightfield Vulkan contract. There is no general CPU evaluator for the ordered scalar layers and no PlaneCut reference. | Partial; PlaneCut reference missing | Add a focused pure PlaneCut reference function that evaluates analytic plane, retained/removed half-spaces, deterministic roughness and composition at requested points. It is the semantic oracle, not a production field framework. |
| F. Surface Nets | The productive Vulkan session runs `geologySurfaceNetsShader` directly over the final linear scalar field and returns `GeologySurfaceMesh`; generic sparse SDF also has `meshSparseSdf`. Both use the negative-inside zero crossing. | Implemented and usable | Reuse the productive Vulkan Surface Nets result for the visible planar-face proof. Do not rewrite either mesher. |
| G. Editor property system | The generic Qt 6 editor shell and slider/property infrastructure remain, but the VDB-coupled Geology Playground and its panel were deliberately removed in the cleanup. | Implemented but hidden; geology UI missing by design | Do not revive the removed UI in this slice. PlaneCut remains data-driven through the recipe; diagnostics and the dedicated proof route are sufficient for this bounded vertical. |
| H. Diagnostics/inspection | `GeologyGpuDiagnostics` exposes backend/device, active voxels, bytes, GPU/CPU timings, pass names, physical/logical readbacks, total/dirty leaves and dirty voxels. `GeologyGpuEvaluationResult` exposes recipe/source/final-field hashes. It does not expose intermediate stage hashes or selected scalar samples/slices. | Partial | Add explicit opt-in field probes and stage hashes sufficient to inspect the source/PlaneCut result and zero-side classification. No hidden full-field readback. |
| I. Cancellation/debounce | `GeologyGpuEvaluationRequest::cancellationRequested` is checked before and between bounded GPU passes/dispatches. The deleted Playground owned the old UI debounce/session surface. | Cancellation implemented and usable; UI debounce absent by design | Preserve the callback contract. Do not rebuild an async editor session or UI debounce mechanism in this slice. |
| J. Quality profiles | `GeologyQuality` supplies InteractivePreview, HighPreview, FinalBake and Custom plus voxel size in meters. GPU topology/workload guards bound leaf, voxel, geometry, allocation and dispatch work. | Implemented and usable | PlaneCut uses recipe quality and the current guards unchanged. Roughness frequency remains cycles per meter and offset/roughness remain meters. |

## Reuse map

| Contract | Exact current seam |
| --- | --- |
| Ordered data-driven recipe | `GeologyStackRecipe`, `GeologyLayer`, `GeologyLayerPayload` |
| Deterministic identity | `writeGeologyStackRecipe`, `hashGeologyStackRecipe`, layer header seeds |
| Scalar/noise sampling | `ProceduralFieldSampler`; existing layer-shader lattice value-noise |
| GPU execution | `GeologyGpuSession::evaluate`, `geologyLayerFieldShader` |
| Linear 8x8x8 leaves | GPU topology plan/workload guard and 512-value field layout |
| Dirty accounting | `geologyDirtyLeafShader`, `dirtyLeafCount`, `dirtyVoxelCount` |
| Extraction | `geologySurfaceNetsShader`, `GeologySurfaceMesh` |
| Quality/workload | `GeologyQuality`, GPU topology/workload limits |
| Cancellation | `GeologyGpuEvaluationRequest::cancellationRequested` |
| Visible proof base | Real Vulkan GPU-session test executable and returned Surface Nets mesh |

## Missing PlaneCut work, in legal order

1. Freeze sign, units, coordinate/transform, seed, roughness, material-preservation,
   voxel-center and parity conventions.
2. Add opt-in intermediate inspection with stage hashes and bounded scalar probes.
3. Write and run mathematical RED tests before production code.
4. Add the recipe enum/payload/schema validation, deterministic serialization and
   hashing using the existing stack.
5. Add the CPU semantic reference.
6. Add the equivalent Diligent/Vulkan layer and parity proof.
7. Prove deterministic hashes and bounded dirty voxels/leaves.
8. Reuse Surface Nets for a real-Vulkan mesh whose cut region has a visible
   planar structural face when roughness is zero.

## Duplicate/forbidden findings

- `ProceduralFieldRecipe` is the admitted low-level noise input to
  `ProceduralFieldSampler`; it is not a second geology stack and will not be
  expanded into one.
- `SparseSdfField` is a generic downstream SDF capability; PlaneCut will not
  create another sparse-field owner around it.
- No indexed production symbol named `PlaneCut` exists at this baseline.
- No external implementation will be copied. R1 and R2 supply the conceptual
  plane-cut research basis only; R2 is GPL-3.0 and is architecture/math study,
  not donor code.

## Step 1 result

The reusable path is coherent and VDB-free. The first unproved behavior is the
analytic negative-inside PlaneCut contract plus opt-in intermediate inspection.
That is the only legal next implementation action.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
