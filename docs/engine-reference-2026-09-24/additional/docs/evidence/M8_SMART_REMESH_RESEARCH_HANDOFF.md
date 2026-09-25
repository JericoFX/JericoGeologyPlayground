# M8 — Smart Remesh / Feature-Preserving Retopology Research Handoff

**Date:** 2026-09-17  
**Milestone:** M8  
**Status:** RESEARCH / DEFERRED WITH M8 — NOT IMPLEMENTED, NOT PASS  

## Purpose

Keep the Smart Remesh 3 analysis explicitly attached to M8 so it is not lost when M8 resumes.

Canonical research document:

- `docs/JERICO_EXPERIMENTAL_SMART_REMESH_RESEARCH.md`

This handoff does not duplicate that document. It only records M8 ownership and the intended integration point.

## M8 integration

When M8 resumes, use the research as input to the existing Generate Game Mesh / remesh work rather than creating an unrelated subsystem.

Relevant M8 work:

```text
B1 — dense working mesh / Generate Game Mesh
    source
      -> sufficiently subdivided working mesh
      -> structural operations
      -> final remesh
      -> preserve regions/materials/relevant edges
      -> measure deviation / triangles / time / memory

B4 — general remesh / asset-processing integration where required
```

The research proposes a Jerico-native feature-preserving path based on:

```text
feature detection
    -> 4-RoSy orientation field
    -> position field
    -> lattice / quad-dominant extraction
    -> source-surface reprojection
    -> feature-constrained relaxation
    -> normal/tangent reconstruction
    -> attribute transfer
```

Hard-surface rules to retain during evaluation:

```text
smooth surface vertex -> tangential relaxation + reprojection to source surface
feature-edge vertex   -> 1D slide + reprojection to feature curve
hard corner           -> pinned
```

The hard-surface panel path should also evaluate flat-region simplification / n-gon panels with geometry concentrated around bevels, creases and silhouette-critical regions.

## Architectural boundary

Do not replace or fork the admitted geology extraction path:

```text
Sparse SDF / geology
    -> Surface Nets + Hermite/QEF
    -> generated mesh
    -> optional downstream AdaptiveSurfaceRemesh
```

Imported/general asset processing remains conceptually separate:

```text
Imported / authored mesh
    -> AssetRetopoRemesh
    -> cooked Jerico mesh
```

Shared low-level mesh analysis, feature classification, projection and normal/tangent utilities are allowed where their ownership is clean.

## M8 acceptance reminder

This research is useful only if a future M8 slice proves it with fixtures and measurable evidence. At minimum validate:

1. planar hard edges retain position and angle within declared tolerance;
2. hard corners remain pinned;
3. smooth curved surfaces improve element distribution without drifting beyond source-surface error bounds;
4. adaptive density transitions do not produce uncontrolled propagation or runaway topology;
5. thin surfaces do not collapse across opposite sides;
6. UV/material/region lineage survives or has an explicit transfer policy;
7. deterministic input + settings + seed produce deterministic output;
8. triangle/quad count, Hausdorff-style geometric deviation, time and memory are reported;
9. cooked output remains compatible with the canonical Jerico mesh format;
10. no Blender or purchased-addon runtime dependency is introduced.

Do not mark any of these PASS based on this handoff or the research document alone.
