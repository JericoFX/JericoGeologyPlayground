# Smart Remesh 3 Research Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Preserve and verify the uploaded Smart Remesh 3 mathematical techniques in an isolated C++ research target without wiring GPL-derived code into JericoEngine production.

**Architecture:** The production repository remains untouched outside documentation and a standalone `Research/SmartRemesh3GPL` directory. Pure C++20 kernels expose value-based APIs with no Blender/NumPy/engine dependency; a small test executable verifies deterministic mathematical behavior.

**Tech Stack:** C++20, CMake, standard library only.

**Spec:** `docs/research/SMART_REMESH_3_CPP_PORT_DESIGN.md`

## Global Constraints

- Do not modify root `CMakeLists.txt`.
- Do not link the research target into any Jerico production target.
- Keep source-derived C++ under `Research/SmartRemesh3GPL/` only.
- Mark the research source GPL-3.0-or-later and document provenance.
- Keep the branch independent from the active M9 slice.
- Tests must run without Blender, Python, NumPy, Instant Meshes or GPU dependencies.

---

### Task 1: Mathematical behavior tests

**Files:**
- Create: `Research/SmartRemesh3GPL/tests/SmartRemeshMathTests.cpp`

**Interfaces:**
- Consumes: declarations in `SmartRemeshMath.hpp`.
- Produces: deterministic RED/GREEN coverage for every research kernel.

- [ ] **Step 1: Write tests for 4-RoSy representative selection, smoothing and combing.**
- [ ] **Step 2: Compile before implementation and confirm missing symbols/header fail the build.**
- [ ] **Step 3: Add tests for signed volume, crease threshold and Otsu.**
- [ ] **Step 4: Add tests for graph relaxation, face-budget fit and centroid-bounded collapse.**
- [ ] **Step 5: Add tests for Douglas-Peucker, Newell area and topology metrics.**

### Task 2: Standalone math library

**Files:**
- Create: `Research/SmartRemesh3GPL/SmartRemeshMath.hpp`
- Create: `Research/SmartRemesh3GPL/SmartRemeshMath.cpp`

**Interfaces:**
- Produces: `Vec3`, `Edge`, `Triangle`, `Face`, `BestRosy`, `SmoothRosyField`, `CombRosyField`, `SignedVolume`, `DetectCreaseThreshold`, `OtsuThreshold`, `RelaxMin`, `EstimateFaces`, `FitSizingToTarget`, `CollapseLattice`, `DouglasPeucker`, `NewellArea`, `EvaluateTopology`.

- [ ] **Step 1: Implement vector primitives and 4-RoSy kernels minimally until their tests pass.**
- [ ] **Step 2: Implement scalar statistics/detection kernels until their tests pass.**
- [ ] **Step 3: Implement sizing and lattice-collapse kernels until their tests pass.**
- [ ] **Step 4: Implement polyline/polygon and topology metrics until their tests pass.**
- [ ] **Step 5: Run the full test executable with warnings enabled.**

### Task 3: Standalone build and provenance

**Files:**
- Create: `Research/SmartRemesh3GPL/CMakeLists.txt`
- Create: `Research/SmartRemesh3GPL/README.md`

**Interfaces:**
- Produces: an intentionally unreferenced standalone research build.

- [ ] **Step 1: Add a local CMake project that builds the library and test executable only when configured from this directory.**
- [ ] **Step 2: Document GPL isolation, verified scope and production non-integration.**
- [ ] **Step 3: Configure, build and run tests from a fresh local build directory.**

### Task 4: Technical dossier

**Files:**
- Create: `docs/research/SMART_REMESH_3_TECHNICAL_ANALYSIS.md`
- Create: `docs/research/SMART_REMESH_3_CPP_PORT_DESIGN.md`

**Interfaces:**
- Produces: a durable algorithm inventory and Jerico-native clean-room roadmap.

- [ ] **Step 1: Document every source module and distinguish custom logic from Instant Meshes.**
- [ ] **Step 2: Record the key equations and topology algorithms without copying implementation text.**
- [ ] **Step 3: Rank CPU/GPU suitability and propose Jerico subsystem boundaries.**
- [ ] **Step 4: Record licensing/provenance boundary and why production integration is deferred.**

### Task 5: Isolated GitHub publication

**Files:**
- Publish only the files listed above on a dedicated research branch.

**Interfaces:**
- Produces: a reviewable GitHub branch/PR with no active-slice mutation.

- [ ] **Step 1: Create the branch from the current `main` commit.**
- [ ] **Step 2: Upload verified research files.**
- [ ] **Step 3: Open a PR explicitly marked research-only and GPL-isolated.**
- [ ] **Step 4: Re-fetch the branch files and verify the published content exists.**
