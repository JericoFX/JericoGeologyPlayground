# Cave passage profile experiments — rejected, 2026-09-25

The retained `--cave-geology-bakes-test` implementation remains the previous opt-in BSP/vector-FBM arch and square control. This experiment was **not promoted**. JericoEngine was read-only; Blender and OpenVDB were not run.

The source of the slab appearance is concrete: `caveSource` duplicates one arch contour at the front and back of a subtractive BSP brush. A first experiment replaced that cutter with overlapping convex BSP sections for a flared mouth, throat, chamber and exit. The closed-mesh check passed, but the subdivision-2 result contained a near-zero-area triangle at approximately `(-1.39086,-0.10016,-0.64347)` where cutter surfaces nearly coincided. The [failed focused test](2026-09-25-cave-rings-ctest.log) retained the geometry failure. This is a Boolean seam failure, not a reason to relax the area criterion.

A second experiment retained the single BSP subtraction and smoothly mapped each axial cross-section in XYZ before the vector-noise layer. At `y=-0.7`, measured right-wall distances at front `z=1.3`, throat `z=0.2` and chamber `z=-0.7` were `1.411`, `1.243` and `1.566` m. The focused geometry test passed; the resulting opening was visibly different in both 1280×960 Vulkan views. But the [high→low proof](2026-09-25-cave-profile-candidate/cave-forms-proof.json) invalidated the candidate:

| Arch LOD | Triangles | Sampled high↔low error | Projection misses |
|---|---:|---:|---:|
| 0 | 3,024 | 0.178 | 0 |
| 1 | 1,344 | 0.522 | 2,896 |
| 2 | 336 | 0.908 | 65,874 |

The [bake audit](2026-09-25-cave-profile-audit.log) failed its existing `≤0.15` error gate. The 2-subdivision LOD cannot represent the narrow throat and inner expansion present in the high mesh; increasing the bake cage would not restore absent low-poly geometry. The [same-camera visual comparison](2026-09-25-cave-profile-visual.json) changed 139,878 frontal and 119,061 oblique silhouette pixels; the square control was byte-identical. The [oblique high capture](2026-09-25-cave-profile-candidate/arch/high-oblique-material.png) still reads as a broad perforated mass, so the large silhouette change is not a Beauty improvement.

After rejecting the experiment, source and tests were restored exactly to the retained baseline; the [Release rebuild](2026-09-25-cave-profile-restored-build.log) and [focused BSP test](2026-09-25-cave-profile-restored-ctest.log) passed. The next geometry experiment needs axial tunnel rings retained at every LOD, or an equivalent feature-preserving reduction, before adding a chamber. Gate it on connected negative space, non-degenerate closed geometry, original-resolution Beauty, and the existing zero-miss high→low/UV/mip audit. Do not promote another cave variant merely because its high mesh looks wider.
