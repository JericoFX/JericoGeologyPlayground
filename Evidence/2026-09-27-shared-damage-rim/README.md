# Shared cavity boundary and floor — 2026-09-27

**Technical PASS; geological Beauty PARTIAL; global shared-damage goal ACTIVE.** The v23 opt-in boundary uses the shared structured noise and bounded radius scale for both the damage mask and the Boolean cutter. A second bottom ring uses the same shared cavity-floor depth function as the analytic field, so an irregular cavity no longer ends in a flat disk. The bevel rim, frequency and irregularity are editable in the normal geology UI; assembly DamageAreas persist the optional boundary. The v22 zero-boundary mesh hashes remain unchanged.

Reproduce with `JericoGeologyPlayground.exe --damage-cavity-test F:\JericoGeologyPlayground\Evidence\2026-09-27-shared-damage-rim`. The command writes v23 recipes, 1280×960 Vulkan before/after images and `damage-cavity-proof.json`. Two independent captures produced identical PNG SHA-256 hashes. On the RTX 3090, the latest run measured rock 23,232 → 22,868 triangles and 190.15 ms after-generation CPU; panel 8,096 → 7,670 triangles and 82.31 ms. These are generation times, not frame times.

Release `ALL_BUILD` and CTest **25/25** passed (`build-surface/shared-rim-all-build.log`, `build-surface/shared-rim-all-ctest.log`). Focused checks cover manifold closure, removed volume, fresh-interior provenance, deterministic recipe replay, v22 compatibility including frequency 100, v23 save/load, source restoration when an area moves away, and a metal railing member selected by the same movable assembly area used for wood. In the normal window, moving the cavity center to X=5 restored the original 23,232-triangle rock; Undo restored the 22,868-triangle cut. Shift+drag is implemented but was not exercised by the available UI driver.

The retained rock capture has a less circular edge and a shaped floor, but exposed rock still lacks a distinct fresh-interior material and the outline retains an artificial cut appearance. The panel through-hole is open with a beveled irregular edge. Further geology Beauty work, other consumer integrations, shared debug UI and performance comparison remain under the active goal.

| Capture | SHA-256 |
| --- | --- |
| `rock-cavity-before.png` | `8190C2E37AECDFF38D3F1BC36A8B5524411F06EBD6D640EEEF97617324085919` |
| `rock-cavity-after.png` | `B6FAA07992E7A66C0F0F17E91591E1929FB4FB7BBF4F3D8A7FF4C936B9A4779B` |
| `panel-through-hole-before.png` | `DEA0E9781F62B4D13AE02C0C8A4BBD5AC379F9C1E017647E4E8C4357794ACFE0` |
| `panel-through-hole-after.png` | `CADB230C7E6F46D27316F9C2A814E967346669A158F2F19B03D4317A41804C31` |
