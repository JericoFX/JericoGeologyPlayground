# Shared geometric damage cavity — 2026-09-27

**Status: technical PASS, Beauty PARTIAL.** The shared projector creates a bounded geometric cavity on a closed rock and a real through-hole in a solidified panel. It uses the common projector/decal dimensions, subtracts a bounded elliptical cutter through Manifold, preserves source region identities, and marks new cutter faces with a stable fresh-interior region. The source recipe and mesh are unchanged; moving the operation regenerates from the source.

## Reproduction

`JericoGeologyPlayground.exe --damage-cavity-test F:\JericoGeologyPlayground\Evidence\2026-09-27-shared-damage-cavity`

The capture command saved both v22 recipes, original-resolution 1280×960 Vulkan before/after images and `damage-cavity-proof.json`. On the NVIDIA GeForce RTX 3090, rock generation took 10.19 ms before and 190.61 ms after (23,232 → 22,742 triangles); the panel took 21.86 ms before and 80.93 ms after (8,096 → 7,680 triangles). These are generation CPU timings, not frame times. Each result replayed from its saved recipe and has one fresh-interior region.

Release `ALL_BUILD` passed and CTest passed **25/25** (`build-surface/shared-cavity-final-ctest.log`). The focused test covers closedness, actual removed volume, shallow floor, penetrating hole, deterministic replay, source immutability, moving the operation away, recipe save/load, cancellation, invalid inputs, mesh budget, and fresh-region ID collision. Luna's read-only review found no high-severity defect in this slice.

In the normal Playground window, the geometric cavity is selectable in the operation list. Its center, orientation, radii, rotation and depth are editable; moving the center off the rock restored the original 23,232-triangle source, and Undo restored the 22,742-triangle cut. The UI also offers Shift+drag for the selected area. The manual check used the position sliders and Undo; Shift+drag itself remains to be exercised interactively.

## Visual limit

The retained rock image shows a conspicuously circular, sharp cavity. It proves the geometric cut but does **not** meet a natural rock Beauty target or the owner's soft decal request. The panel hole is visibly open. The cutter samples the analytic elliptical cavity as a 64-sided Boolean mesh; exact local SDF extraction, a softer/noisier rim, material treatment of exposed interiors, shared damage controls across every procedural family and final cost comparison remain pending under the active goal.

| Capture | SHA-256 |
| --- | --- |
| `rock-cavity-before.png` | `8190C2E37AECDFF38D3F1BC36A8B5524411F06EBD6D640EEEF97617324085919` |
| `rock-cavity-after.png` | `993E0545AAF57D2CCC109388042C8DB683B2D6845A6A92764B1E6791265E4022` |
| `panel-through-hole-before.png` | `DEA0E9781F62B4D13AE02C0C8A4BBD5AC379F9C1E017647E4E8C4357794ACFE0` |
| `panel-through-hole-after.png` | `BABF18398020BED8F9BE609B965271E30672E6A0DE384211EF964CF3D074C061` |
