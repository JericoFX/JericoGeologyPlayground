# Broken plank geometry candidate — 2026-09-27

The former broken plank was a split with a regular gap and one whole half translated downward. This candidate varies the two fracture tips independently using the shared `fractureProfileOffset` and `structuredDamageNoise01` primitives. It keeps the far end anchored and lowers the broken tip. Extra triangles are concentrated around the cut rather than added across the full plank. Fresh side walls receive a selectable `DamageInterior` region and two-dimensional UVs.

The 1280×960 [isolated plank](plank-fracture-isolated.png) exposes both halves; [pier closeup](pier-break-closeup.png) shows the same break in context. The captured assembly proof includes stable semantic part IDs, mesh hashes and one-run CPU measurements. Release `ALL_BUILD` and 25/25 CTest passed. The fixture verifies a closed plank, local triangle budget, droop, fresh region and UV extent. Two Vulkan capture rounds produced identical PNG hashes, recorded in `png-sha256-round1.txt` and `png-sha256-round2.txt`.

**Visual acceptance remains partial.** The preview has no wood texture or fine fibers and the isolated gap still looks mechanical. This is a prepared geometric break, not physical motion. The generic shared damage cutters are not yet available on every generated object. The global goal remains ACTIVE.
