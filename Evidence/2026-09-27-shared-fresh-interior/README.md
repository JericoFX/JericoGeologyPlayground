# Fresh interior material provenance — 2026-09-27

**Technical PASS; geological Beauty PARTIAL; shared-damage goal ACTIVE.** `DamageInterior` region IDs now produce a per-corner material mask in the Playground renderer. The source triangles keep their existing material, including at vertices shared with newly exposed faces. Fresh faces retain triplanar texture detail but receive a lighter, rougher appearance without accumulated cavity/contact dirt. Geometry and semantic IDs are unchanged. Meshes with no fresh role take an empty-mask fast path.

Reproduce with `JericoGeologyPlayground.exe --damage-cavity-test F:\JericoGeologyPlayground\Evidence\2026-09-27-shared-fresh-interior`. The two saved v23 recipes replay the same rock and solidified-panel cuts as the earlier rim fixture. For each mesh, the command captures the same geometry with and without the fresh role; only the role changes between `after-material-unmarked` and `after-material`. Vulkan captures are 1280×960 on an RTX 3090. The rock has 1,614 fresh material corners and the panel has 783. The image comparison found 29,398 changed pixels in rock bounds (533,375)–(744,583), and 9,311 in panel bounds (587,409)–(745,576); pixels outside those bounds match. The images were also inspected at original resolution and the rock interior was checked in the normal interactive window.

The focused RED contract failed to link before the region-to-corner mapping existed (`build-surface/fresh-material-red.log`). Release `ALL_BUILD` and the final CTest **25/25** passed (`build-surface/fresh-material-final-ctest.log`). The Vulkan web-surface smoke also exited successfully after the renderer change. The material captures repeated with identical SHA-256 hashes after the empty-mask optimization. This is a shared visual treatment of fresh faces, not distinct authored wood/metal/rock interior materials; those remain for consumer integration. The geology cavity silhouette and overall Beauty remain partial.

| Capture | SHA-256 |
| --- | --- |
| `rock-cavity-after-material-unmarked.png` | `06A57B9C93926779B4FCFAD666EE21D4AAAC94CE69519F9AC5832E950B845548` |
| `rock-cavity-after-material.png` | `56DFE39FE6AB6C039EB1410CC6F1223CCFE99D109F6B067784E3ADE940F28B57` |
| `panel-through-hole-after-material-unmarked.png` | `C2E9977028F0114795D3369309F76B1F4C5CB7878CBC34E5943C85628BCF578B` |
| `panel-through-hole-after-material.png` | `C48A1EB675284A48EE19E232D46A39182999683CF674DF72312FCCABA0442177` |
