# M8 Smart Remesh — native reimplementation research input

**Milestone:** M8
**Status:** research input ready; product implementation not started
**Timing:** read and act on this only when M8 becomes the legal `ACTIVE_SLICE`.

This document is a discoverability pointer for Astra. It does not replace `ACTIVE_SLICE.md`, `HANDOFF.md`, the M8 closure plan, or the M8 Blender/geology authority.

When Astra resumes M8 and reaches addon/capability selection, remeshing, mesh cleanup, Blender-result post-processing, LOD or topology work, read this input before designing a new solution.

## Source material already in `main`

1. `docs/research/SMART_REMESH_3_TECHNICAL_ANALYSIS.md`
2. `docs/research/SMART_REMESH_3_CPP_PORT_DESIGN.md`
3. `Research/SmartRemesh3GPL/README.md`
4. `Research/SmartRemesh3GPL/SmartRemeshMath.hpp`
5. `Research/SmartRemesh3GPL/SmartRemeshMath.cpp`
6. `Research/SmartRemesh3GPL/tests/SmartRemeshMathTests.cpp`

The research was merged through PR #2. The isolated C++ project was verified standalone and is intentionally disconnected from Jerico production CMake.

## Product decision

JericoEngine must not require Smart Remesh, another paid Blender addon, or its Python source to provide remeshing/retopology functionality.

The intended flow is:

```text
reference addon
  -> behavior/math analysis
  -> equations + invariants + tests
  -> independent Jerico implementation
  -> cooked/runtime result with no addon dependency
```

Jerico's future open-source tooling should expose the useful capability through Jerico-owned contracts. The paid addon remains research provenance only.

## License boundary

Smart Remesh declares `GPL-3.0-or-later`. Therefore `Research/SmartRemesh3GPL/` is an isolated research reference and must not be mechanically copied or linked into production Jerico merely because Jerico itself will be open source.

Prefer an independent implementation from the documented mathematics, observable behavior, invariants and tests. This keeps the final engine free to choose its own open-source license strategy.

The Instant Meshes component observed in the package is separately BSD-3-Clause; if it is ever evaluated, inspect the upstream project and admit it through Jerico's normal dependency process rather than depending on the addon bundle.

## Algorithms Astra should evaluate for M8

- principal-curvature estimation and curvature-aligned fields;
- dihedral/crease detection and automatic thresholding;
- multi-scale cavity/convexity signals;
- adaptive sizing from curvature, creases, thickness and source density;
- Lipschitz-style density relaxation;
- face-budget calibration;
- tangent 4-RoSy orientation fields and crease constraints;
- coarse-to-fine/Jacobi field solving and field combing;
- local lattice/position-field integration;
- centroid-bounded clustering/collapse;
- polygon/quad extraction from local topology order;
- hard-surface planar-region and persistent-bend detection;
- panel/n-gon reconstruction and chamfer preservation;
- contour simplification where justified;
- edge-loop cleanup and pole/valence relaxation;
- UV transfer/healing after topology changes;
- deterministic quality metrics/candidate scoring.

Do not assume all of them belong in one subsystem. Audit existing Jerico mesh processing, import/cook, ProceduralAuthoring, geology, LOD and BlenderWorker contracts first and place each capability at the narrowest reusable boundary.

## Likely M8 consumers

- `Tools/ProceduralAuthoring/` geology/generated geometry;
- general asset import/cook preprocessing;
- deterministic cleanup of Blender headless output;
- hard-surface authoring/destruction preparation where sharp regions must survive;
- future LOD/remesh tooling.

CPU/GPU placement must be measured. Per-vertex field relaxation, sizing/curvature and large independent reductions may benefit from compute; topology mutation/extraction can remain CPU until profiling demonstrates otherwise.

## Native implementation acceptance direction

At minimum test:

- smooth organic mesh;
- planar hard-surface mesh with intentional creases;
- mixed smooth/hard regions;
- thin shells/narrow features;
- boundary and non-manifold diagnostics;
- deterministic repeated cook;
- target-density/face-budget error;
- feature preservation and normal deviation;
- UV behavior when enabled;
- source-free/cooked execution with no paid-addon process or source required.

Compare quality against the original high-density/source surface, not merely against whether a mesh was emitted.

## Relation to M8 authority

`docs/JERICOENGINE_M8_BLENDER_HEADLESS_GEOLOGY_AUTHORITY_DIRECTIVE.md` already requires, when M8 resumes, reviewing/selecting addons and capabilities before building the Jerico adapter/front path. This file is the Smart Remesh-specific research input for that stage.

It does not authorize M8 work while another slice owns execution and does not change any current PASS/PARTIAL status.
