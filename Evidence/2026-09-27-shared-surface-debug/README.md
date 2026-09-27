# Shared surface debug view — 2026-09-27

Geology and Procedural Assemblies now select curvature, convexity, concavity, cavity/contact, crease distance, boundary distance, exposure and thickness from the same `SurfaceFeatureFields` backend. The scalar channel is a renderer preview only; it does not rewrite the mesh or its damage recipe. Both UIs report feature cache hits, misses and total computation time.

Verification:

- Release `ALL_BUILD`: exit 0. CTest: 25/25 passed after the UI/core changes; the later assembly fixture passed its focused executable.
- `JericoGeologyPlayground.exe --shared-surface-debug-test Evidence/2026-09-27-shared-surface-debug`: exit 0 on RTX 3090, four original 1280×960 Vulkan captures from rock and wooden pier. The four PNGs repeated byte for byte.
- `shared-surface-debug-proof.json` records mesh hashes, scalar ranges, vertex counts, cache hits/misses and individual computation times. The CPU times are single runs and are not a frame pacing benchmark.
- Unit fixtures assert the signed curvature/cavity scalar values, finite crease distance for unreachable vertices, explicit thickness analysis, and vertex-domain mapping on the pier.

The normal Playground window exposes the selector in both workspaces, but this increment did not verify a full manual click sequence. This is a surface-feature debug slice: selected `DamageField`, fresh interior, protected masks, tier and semantic edge IDs still need a shared debug presentation. The global goal stays active.
