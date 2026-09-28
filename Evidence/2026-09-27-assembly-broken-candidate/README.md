# Broken state for remaining solid assembly roles

Status: **geometric cut verified for Coupling, RailJoint and CrossBeam; Beauty PARTIAL; global damage goal ACTIVE**.

The `Broken` state is now selectable for every assembly role. The roles that previously rejected it use the existing shared path cutter after generating their original closed Part. The cutter is scoped to that Part, gives the exposed faces a fresh region, and leaves the other Parts unchanged. Its projector basis is right handed: an inverted basis initially added volume instead of cutting it, which the volume regression now prevents.

The focused fixture covers generated Coupling, RailJoint and CrossBeam members. It checks a closed nonempty result, reduced volume, one fresh interior region, complete corner UVs, deterministic replay and byte-identical meshes for unaffected Parts. Release `ALL_BUILD` exited 0 (`../2026-09-27-assembly-broken-build-final.log`) and CTest passed **25/25** (`../2026-09-27-assembly-broken-ctest.log`). The Vulkan capture generated 34 original 1280×960 PNGs on the RTX 3090; a second run matched all 34 SHA-256 hashes, and the 31 previously retained cases stayed pixel-identical. The new captures are `broken-coupling.png`, `broken-rail-joint.png` and `broken-cross-beam.png`, with per-case mesh and state details in `procedural-assemblies-proof.json`.

The three cuts visibly separate material, but they remain straight, clean and mechanically regular. There is no physics activation, debris, material-specific splintering, or complete cross-system parity yet. These captures do **not** establish realistic fracture Beauty or completion of the shared-damage directive.
