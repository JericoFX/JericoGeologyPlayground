# Axial cave profile with high→low feature reduction — 2026-09-25

Status: **PASS for this opt-in geometry/bake route; PARTIAL for natural-cave Beauty**. The prior 336-triangle regenerated LOD2 lost the throat and produced 65,874 misses. This candidate keeps the single selected BSP host/tunnel, applies a deterministic positive axial cross-section map to the macro mesh, adds finer normal noise to the high, and reduces the *pre-detail macro high* natively for LOD0/1/2. The square tunnel remains the unchanged control. JericoEngine was read-only; Blender and OpenVDB were not used.

`--cave-axial-lods-bakes-test <directory>` retains source and region identities. The map combines a front flare, narrow throat, wider inner section and a small lateral bend; quality-gated Manifold simplification uses macro error budgets 0.01/0.05/0.10. A separate sampled high↔low check caps error at the existing 0.15 diagnostic limit before the bake. The [focused reduction test](2026-09-25-cave-axial-reduction-test.log) measures the right passage wall at `(y=-0.7, z=1.3/0.2/-0.7)`: high `1.473/1.182/1.589 m`; LOD2 `1.477/1.165/1.602 m`. All LODs remain closed and retain these sections within 0.15 m.

The [Vulkan bake proof](2026-09-25-cave-axial-candidate/cave-forms-proof.json) is 1280×960 with 1024² textures:

| Arch mesh | Triangles | Sampled high↔low error | Misses / backfaces | Bake CPU ms |
|---|---:|---:|---:|---:|
| High | 5,376 | — | — | — |
| LOD0 | 4,018 | 0.0960 | 0 / 0 | 1,376 |
| LOD1 | 2,724 | 0.1072 | 0 / 0 | 1,235 |
| LOD2 | 1,618 | 0.1081 | 0 / 0 | 1,270 |

The [package audit](2026-09-25-cave-axial-audit.json) passed all six arch/square high→low bakes: automatic UV interior covered, specialist AO/contact/cavity/convex-edge/crease/curvature/height/thickness/region-ID maps and their mips coherent, no fabricated region IDs. The [visual comparison](2026-09-25-cave-axial-visual.json) verifies square material controls identical. A second run of the same Release binary (`SHA-256 70108575463B3797B57976BEABED994018AC8DD44BD502BD0508B6499F1ED5D4`) matched [266 PNG/OBJ artifacts byte-for-byte](2026-09-25-cave-axial-determinism.json); the previous `--cave-geology-bakes-test` also replayed [266 identical artifacts](2026-09-25-cave-geology-regression-audit.json) after this change. Release build passed; full CTest passed 17/17 in [54.31 s](2026-09-25-cave-axial-final-ctest.log). The bake timings are single runs without a controlled performance baseline; LOD2 uses more geometry and takes longer than the prior experimental 336-triangle LOD2.

Original-resolution [front](2026-09-25-cave-axial-candidate/arch/high-material.png), [oblique high](2026-09-25-cave-axial-candidate/arch/high-oblique-material.png) and [oblique LOD2](2026-09-25-cave-axial-candidate/arch/lod-2/low-oblique-material.png) were inspected. They show good high/low agreement but the exterior still reads as a broad perforated block, with no deep chamber, branch, stratification or interior lighting. Thus this is an improved **bake/LOD mathematics** proof, not a natural cave or M8 integration acceptance. The pinned CC0 material source and SHA-256 remain in `Evidence/cc0-references/manifest.json`; no new external texture was introduced.
