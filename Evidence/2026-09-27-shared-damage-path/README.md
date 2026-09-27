# Shared path damage and local plank fracture — 2026-09-27

The same shared `DamagePathCutter` cuts a rock, a panel, a wooden deck plank and a beam. The cut is an editable recipe operation with stable identity, protected-part exclusion and fresh-interior face provenance. The plank's prepared broken variant now has a narrow tessellated strip around its fracture; the intact end does not acquire a dense mesh.

Verification on the Release build:

- `cmake --build build-surface --config Release --target ALL_BUILD --parallel 4`: exit 0.
- `ctest --test-dir build-surface -C Release --output-on-failure`: 25/25 passed.
- `JericoGeologyPlayground.exe --damage-path-test Evidence/2026-09-27-shared-damage-path`: exit 0, 1280×960 Vulkan on RTX 3090. Six PNG captures repeated byte for byte on a second run.
- Rock: 3072 → 3276 triangles, 492 fresh-material corners, 27.87 ms after generation. Panel: 1152 → 1270 triangles, 333 fresh-material corners, 16.26 ms after generation. Values are from `damage-path-proof.json`; they are single captures, not performance distributions.
- The locally tessellated broken plank validates as a closed solid. Its test bounds added geometry to the break band and fewer than 500 triangles in the selected part.

Visual review: both paths are clear geometric recesses, with exposed interior shading. Their Boolean capsule outline is still regular, especially on the rock, and is **not accepted as final natural crack Beauty**. The interactive path controls and dragging were compiled and exercised by automated geometry/serialization tests, but the full manual UI sequence was not verified in this increment. The shared global goal remains active.
