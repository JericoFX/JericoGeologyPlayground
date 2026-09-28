# Shared fracture on rails and bridge beams

Status: **Rail and Beam geometric damage verified; Beauty PARTIAL; global goal ACTIVE**.

The railway Rail and wooden-pier Beam previously implemented `Broken` as a fixed gap between two independently generated tube segments. They now draw the original closed member and apply the same shared path cutter used by Coupling, RailJoint and CrossBeam. The selected member gets a fresh interior region and two separately addressable closed fragments while the other Parts retain their meshes.

The focused RED fixture failed on the missing shared-cut interior (`../2026-09-27-assembly-structural-break-red-test.log`). The GREEN fixture checks volume loss, closed topology, fresh region, corner UVs, deterministic replay, unchanged other Parts, two individually extractable closed fragments, triangle picking and stable physical sides at LOD 1 for both roles. Release `ALL_BUILD` exited 0 (`../2026-09-27-assembly-structural-break-build-final.log`); CTest passed **25/25** (`../2026-09-27-assembly-structural-break-ctest.log`). Vulkan generated 36 original 1280×960 captures on the RTX 3090; a second run repeated all 36 PNG hashes exactly. Of the 34 earlier cases, only `broken-railway.png` changed as expected. The new isolated captures are `broken-rail-member.png` and `broken-beam-member.png`; the proof JSON records two fragments for each.

The cut is more local and uses the shared math, but the exposed face and gap still look mechanically clean, especially without distinct rail/wood materials or physics. Other beam-like assembly roles still use their old fixed-gap recipe, and the full cross-system damage directive remains unfulfilled.
