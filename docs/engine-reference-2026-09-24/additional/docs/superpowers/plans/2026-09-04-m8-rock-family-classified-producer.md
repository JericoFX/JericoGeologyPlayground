# M8 Rock Family Classified Producer Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans inline, with the repository's sole-writer rule. Steps use checkbox tracking.

**Goal:** Demonstrate that four explicit current Geology recipes produce independently identified, classified cooked mesh/collision candidates, replacing the failed legacy input at this boundary.

**Architecture:** A test-side composition invokes the existing Vulkan Geology session and GeologyCook. Consumer assertions inspect decoded cooked bytes, not producer acceptance flags. This is the first bounded implementation step of the approved repair, not Beauty or slice closure.

**Tech Stack:** C++20, existing CMake Windows Vulkan Editor tree, existing Diligent Vulkan and Jerico runtime formats; no dependencies added.

**Spec:** `docs/superpowers/specs/2026-09-01-m8-rock-family-final-shaded-visual-quality-repair-design.md`.

## Global Constraints

- Four explicit GeologyStackRecipe instances; no legacy generator restoration.
- Existing GeologyGpuSession, Surface Nets and GeologyCook only.
- Recipe values and product composition belong to the showcase fixture.
- No new runtime format, material database, evaluator or transaction system.
- Preserve all 185 foreign status entries and all other product acceptance rows.
- A classified-producer PASS does not grant Beauty, transaction, package or criterion-13 credit.

## Task 1: Authentic cooked-lineage RED

**Files:** Create `Tests/Editor/GeologyRockFamilyTests.cpp`; register its executable in `Tests/Editor/CMakeLists.txt`.

**Interface:** `JericoGeologyRockFamilyTests --inspect member.rmesh` returns 0 for the checked lineage contract, 1 for a semantic rejection, 2 for invalid input/usage. It calls the real `RuntimeFormats::readMesh` and inspects `CookedMeshAsset::geology`.

- [x] Inspect retained Shipping Beauty at original 1280x720: coarse prisms, occluded fourth member, uniform response.
- [x] Decode retained product using current AssetDump: 15624 vertices, `geology=false`, `geologyVertices=0`.
- [x] Compile the test target and execute the consumer assertion against that actual retained product:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoGeologyRockFamilyTests --parallel 4
build/windows-vulkan-editor/Tests/Editor/Debug/JericoGeologyRockFamilyTests.exe --inspect build/windows-vulkan-editor/Tests/Editor/Debug/SparseSdfRockFamilyShowcase/Artifacts/RockFamily.rmesh
```

Expected: exit 1 at `mesh.geology.has_value()`, not a missing file or parser failure. Keep the rejected input and prior `0.0411372` structural evidence untouched.

## Task 2: Current four-recipe producer and round-trip contract

**Files:** Extend only `Tests/Editor/GeologyRockFamilyTests.cpp` and its CMake registration.

**Interfaces:** Existing `GeologyGpuSession::evaluate(const GeologyGpuEvaluationRequest&)` and `cookGeologySurface(const GeologyCookRequest&)`. Default test invocation runs the four-member contract; `--inspect` stays available for independent raw-product checks.

- [x] Extend the authentic cooked-lineage RED with assertions for exactly four successful candidates; unique recipe hashes/source GUIDs/surface hashes; decoded classification count and full position/classification multiset continuity; nonzero lineage; collision indices/positions match the final cooked mesh; repeated evaluation and cook preserve bytes. Input-field hashes may be shared by distinct authored sources.
- [x] Build four explicit bounded source/structural recipes using admitted `BlockGrammar`, `ImplicitBlock` and `FoldDomain`, with distinct identities/seeds/forms. No Detail owns silhouette. Existing Strata, DurabilityField and Rebuild provide the explicit GSTR input required by current World preparation.
- [x] Use canonical recipe serialization and parse it again before evaluation. Generate on Vulkan and cook through the existing classified path.
- [x] Run `JericoGeologyRockFamilyTests` directly or through the explicit `JericoGeologyRockFamilyProof` target, exclusively on the GPU. The new proof passes actual cook/decoded-consumer checks. Existing implicit-block/fold, legacy rock-family and scheduling regressions pass (4/4). No hashes or aggregate rows are promoted.
- [x] Inspect the diff and preserve the foreign inventory. Publish only the verified test-side producer checkpoint; Git is the publication authority.

## Task 3: Independent topology and diagnostic consumer export

**Files:** Extend the same owned test fixture; keep production code and foreign files unchanged.

- [x] Demonstrate that the old edge-count/absolute-volume oracle wrongly accepts an inward tetrahedron, one reversed face, and two disconnected outward tetrahedra (three expected RED assertions).
- [x] Require opposing directed edge uses, positive signed volume and one edge-connected face component. Cover open/invalid/empty inputs. All four real Vulkan candidates pass, including fresh-session deterministic cook.
- [x] Export the four independent mesh/collision products and canonical recipes into a new directory only. Package diagnostics through existing RuntimeFormats; round-trip all package payloads exactly.
- [x] Inspect each member at original 1280x720 through the existing cooked PBR Vulkan consumer. Flat calibrated material inputs are diagnostic, not final palette or Beauty acceptance. All four current forms remain visually rejected; isolated one-package replay reproduces each image exactly.
- [x] Retain honest visual findings, review the code, rerun relevant checks and publish the checkpoint without promoting criterion 13. Final export uses the existing exclusive directory publisher; topology/export safety contracts and the two focused CTest regressions pass.

The existing CookedAssetProof renders one selected mesh per invocation; its optional
level check does not draw a four-body scene. Individual diagnostic captures must
not be represented as the family scene proof. Stable ground/contact, three-view
silhouette/anti-clone witnesses, final materials and the existing multi-entity
editor/renderer transaction remain required.

## Task 4: Repair current macro-form RED and authored support

**Files:** Same owned rock-family fixture and evidence for the recipe step; no
evaluator changes. The separately bounded renderer repair below addresses the
normal-mapping defect subsequently found by these diagnostics.

- [x] Use the retained current 1280x720 captures as authentic macro-form RED: soft slab/block, deformed box and bent extrusion.
- [x] Add an independent cooked-surface support witness and demonstrate RED on the prior member: no contact at the authored plane, below-plane vertices and unsupported uniform-density geometric center.
- [x] Replace large smooth FoldDomain deformation with bounded admitted DirectionalWarp, smaller edge smoothing, rough oblique PlaneCut faces and an explicit support cut before Rebuild. Preserve current recipe/cook/classification/structural-field APIs and voxel budgets. An initial MacroWarp trial was rejected by the productive GPU allowlist; use the blueprint's published DirectionalWarp without widening that allowlist. These inputs remain diagnostic candidates, not accepted natural forms.
- [x] Verify connected outward volume, exact classification/collision, fresh-session determinism and support. The diagnostic support witness uses a 2 cm tolerance (one quarter voxel), projected contact area at least 15% of the XZ bounds footprint, and projected uniform-density geometric center over actual contact triangles. The gate inspects the final decoded cooked mesh, after Rebuild; no vertex may penetrate the plane beyond tolerance. Positive, floating, tipping and penetrating fixtures are covered, including a verified mutation RED for the penetration check. This is not Jolt/material-density stability acceptance.
- [ ] Inspect fresh native-size diagnostic views against retained RED, then establish three-view anti-clone/silhouette witnesses and ground/family scene through existing consumers. Reject partial or artificial-looking forms without weakening gates.

The first repaired export added geometric Strata displacement/grooves and produced
regular terracing. A controlled ablation removed only that geometric Strata input;
material classification and authored durability remain. Bands disappeared, but
dark pinched edges and box-like forms persisted. A subsequent single-variable
member-0 top-plane normal correction tests the thin wedge; the three other members
stay unchanged as controls. Keep these ablations separate from Beauty acceptance.

Do not reuse the legacy shape inspector as the new anti-clone gate: its pair-distance
histogram selects at most 96 vertices by index and is therefore sensitive to vertex
reordering during canonical cook. The old accepted distance/threshold are not new
candidate evidence. Independent order-invariant shape and three-view witnesses
remain required; no legacy inspector or acceptance row was changed.

## Completeness audit and required continuation

### Order-invariant shape diagnostic checkpoint

- [x] Introduce the bounded independent cooked-position pair-distance witness;
  demonstrate missing implementation RED and transform/order/duplicate invariance.
- [x] Apply >0.04 total-variation to all six real pairs. Pair 2/3 fails at 0.0285522;
  change member 3's authored structural profile, not the gate. Final minimum is
  0.0805746. Members 0/2 remain exact controls.
- [x] Reproduce orphan-vertex sensitivity, consume indexed positions only, and
  cover fractional bins, invalid/non-finite/collapsed/bounded inputs.
- [ ] Complete three-view silhouettes, natural forms and the full family vertical.

On explicit owner request this work is checkpointed PARTIAL before F2's whole-engine
architecture/performance audit. See the current producer evidence. The witness is
not tessellation-independent or a Beauty gate; no M8 product acceptance is claimed.

### Bounded renderer repair discovered by the rock diagnostic

The cooked normal/face probe found no inward normal corner or degenerate triangle
on the current four candidates. The shared geological PBR normal projection,
however, changes the macro normal even with normalScale zero: a neutral sample
blends sharpened signed axis weights instead of preserving the macro normal.
This violates neutral normal-map behavior independently of the authored shape.

Allowed bounded extension: `Engine/Render/Shaders/Surface/PBR_Opaque.psh` geological
normal function and the existing `Tests/Render/StaticMeshRenderAssetTests.cpp`
geology Vulkan contract. No ordinary-PBR, material schema, mesh, recipe, lighting
or acceptance-threshold changes. Existing HDR/geology permutations are affected
and must be recooked; ordinary variants are regression controls.

- [x] Establish a real ordinary/geological neutral-normal Vulkan comparison.
  The first ambient-dominated probe was insensitive. A direct-lit probe with an
  explicit non-dark control produces two expected RGB parity failures (delta 3
  and 6; tolerance 1), `rock-family-neutral-normal-red2.log`, exit 1.
- [x] Preserve the macro normal in each triplanar projection before blending,
  leaving texture-weighting and non-geological paths unchanged. Normalize after
  XY strength scaling and reconstruct positive Z for BC5 as ordinary PBR does.
- [x] Verify neutral normal parity, non-neutral response, relevant regressions,
  and newly cooked package-only native rock captures with unchanged mesh bytes.
  All four meshes/collisions remain byte-identical. Two isolated diagnostic
  replays reproduce all native frames exactly; old sharp black pinches are absent
  in the inspected views, but soft block-like geometry remains visual RED.
- [x] Review the shader/test delta. Final read-only review reports no outstanding
  important findings. The default renderer test now includes neutral RGBA8/BC5
  strength 0/1 and non-neutral response, passing in 77.72 s under the unchanged
  120 s cap; the three focused CTest regressions pass. Native candidate replay
  evidence is retained in the producer checkpoint; no Beauty promotion follows.

The approved spec's complete pre-implementation integration/data-lineage audit remains binding. This step implements source recipe -> GPU -> classified surface -> cooked mesh/collision and test diagnostics. The following are **INTEGRATION REQUIRED**, not implied by this test:

| Subsystem / owner | Required input and interface | Dependency / milestone |
|---|---|---|
| Editor / ProjectTransactionCoordinator | Four candidate mesh/collision pairs and one World/catalog transaction; exact Cancel/Apply/Undo/Redo | Current single-body GeologyWorldPreparation must be extended or composed through the existing coordinator; same M8 slice |
| Renderer / showcase | Four classified meshes, admitted palette, ground/contact, family + landmark cameras | Geometry structure/contact/non-clone witnesses first; same M8 slice |
| Physics / package | Exact accepted collisions and one source-free package | Current Jolt admission and package replay; same M8 slice |
| Acceptance / aggregates | Frozen new identities, real performance sample kinds, original-size Beauty review | Criteria 6-12 recertification and criterion 13 1/5 -> 2/5 only after full evidence; same M8 slice |
| Test scheduling / repository contracts | Register the final vertical in the fixed CTest inventory and recertify all supported inventory variants | The candidate checkpoint is an explicit proof target, not an automatic CTest gate; same M8 slice |

Further structural, transactional and visual steps will be specified against the observed candidates and existing consumer APIs; none is reported implemented by this producer checkpoint.

## Safe improvement findings from repository recovery

- Remote documentation was 30 commits ahead; fast-forwarded to `c7c4f6fc`. All 13 content-modified tracked files retained their exact SHA-256 values.
- Preserve the incoming capability-map anti-regression directive and reconciliation audit. A ledger/freshness check is useful future work, not permission to regenerate the map from a stale snapshot.
- Audio/navigation/input/save/gameplay/QA authorities are approved designs; they do not prove runtime implementation and do not replace current M8 work.
- Keep the producer test separate from the enormous GPU regression source and untouched legacy fixtures. This reduces change coupling without refactoring the engine.
- CTest's scheduled and full inventories are frozen across configuration variants. An attempted new registration was rejected during configure; retain the explicit standalone proof target for this candidate checkpoint, then update that matrix as part of final vertical integration. No inventory validation is disabled or weakened.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
