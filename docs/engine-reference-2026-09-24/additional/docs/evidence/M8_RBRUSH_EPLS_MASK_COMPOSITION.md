# M8 RBrush EPLS — deterministic final-mask composition

Date: 2026-08-13

State: **Task 1 PASS; RBrush Slice 25, C26 EPLS, M8 and JericoEngine 1.0 remain ACTIVE**

This checkpoint establishes the first real EPLS product boundary. It does not
claim an editor workspace, persisted recipe, RBrush stroke adapter, MeshBlend,
cook, runtime integration or Shipping proof.

## Observable contract

`JericoEplsAuthoring` is a Qt-free Tool module. For each admitted U16 page it
computes, with widened integer arithmetic:

```text
combined = min(65535, Procedural + ManualInclude)
Final    = max(0, combined - ManualExclude)
```

The implementation validates stable non-zero surface/page GUIDs, finite and
strictly increasing world bounds, bounded dimensions and exact sample counts
before allocating. It checks cooperative cancellation at least once per row
and publishes no candidate after cancellation.

A changed page emits exactly one bounded `DirtySpatialRegion` with:

```text
SurfacePages | GiBricks | WorldCellDependencies
```

An unchanged final mask emits no dirty work.

## RED retained

After registering the new focal test and updating the scheduling inventory,
the authentic product RED was:

```text
EplsMaskTests.cpp(1,10): fatal error C1083:
Cannot open include file 'Jerico/Epls/EplsMask.hpp': No such file or directory
```

The later first GREEN build exposed one test-only wiring omission:
`TestSupport/TestHarness.hpp` was not on the new target's include path. The
target now reuses the same `${PROJECT_SOURCE_DIR}/Tests` seam as neighboring
test modules; no product contract or assertion was weakened.

## Literal proof

The focal test uses literal inputs independent from production helpers:

```text
40000 + 30000 - 10000 -> 55535 after saturation
65535 + 65535 - 65535 -> 0 (full Exclude wins)
0 + 12345 - 0         -> 12345
1000 + 0 - 2000       -> 0 (subtraction clamps)
```

It also proves exact changed count, exact dirty dependencies, zero-change
behavior, cancellation and stable errors for identity, bounds, dimensions,
storage and resource limits.

## Verification

Fresh commands and results:

```powershell
cmake --preset windows-vulkan-editor
# PASS; configured and generated in 68.3 s

cmake --build --preset windows-vulkan-editor --target JericoEplsMaskTests
# PASS

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^epls\.mask$" --output-on-failure
# 1/1 PASS in 0.87 s

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(epls\.mask|repository\.test_scheduling_contract|architecture\.)" --output-on-failure
# 22/22 PASS in 1.38 s

cmake --build --preset windows-vulkan-editor --config Release --target JericoEplsMaskTests
# PASS in 5.5 s

ctest --test-dir build/windows-vulkan-editor -C Release -R "^epls\.mask$" --output-on-failure
# 1/1 PASS in 0.08 s

git diff --check
# PASS; only existing line-ending conversion warnings from user-owned files
```

Product commit:

```text
7d49c59 feat(epls): compose deterministic final masks
```

## Boundaries and remaining work

- No Qt, Diligent, PWAS or SurfacePaint dependency enters the EPLS module.
- RBrush, `.rbrush`, `.abr`, EPLS recipes and manual strokes remain editor-only.
- No runtime or Shipping format was introduced by this task.
- MeshBlend remains a future shared backend; it is not duplicated in EPLS or
  PWAS and is not demonstrated here.
- Next legal behavior is an EPLS recipe referencing the canonical
  `ProceduralMask` Box/SplineCorridor scope, with deterministic validation and
  hashing.
