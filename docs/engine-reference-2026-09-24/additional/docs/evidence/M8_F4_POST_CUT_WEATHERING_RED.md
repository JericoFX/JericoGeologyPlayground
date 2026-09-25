# F4 actual-member post-cut weathering RED

Date: 2026-09-05. Baseline `b88b1d1ada0639948dd178991a5f47bf8b0e3b39`.
Status: **PARTIAL — integration RED reproduced; no production repair claimed**.
Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.

The preceding native ImGui increment is published and verified. This increment
returns to the actual rock's missing form-producing behavior. The blueprint's
sections1914-2036 relate positive curvature/exposure to convex wear. This can
recede corners; it does not create missing macro structure or establish Beauty.

## Reproducible actual product

`Tests/ProceduralAuthoring/Fixtures/F4WorkingWeatheringControl.geology.toml` is an
unchanged copy of member1's previously generated actual Strata-warp recipe from
`build/windows-vulkan-editor/f4-strata-warp-material-01`. SHA256:
`6E897825979DA3EE452AB4E51D4689D22185FC946C42ADFD49B81BACD3103893`.
It retains density16, voxel0.06m, implicit block, Strata shape0.045m/groove0.015m,
phase warp0.15m/frequencyXYZ0.1, all six authored cuts and final Rebuild.
The immutable fixture makes the RED independent of an ephemeral recipe path.

The new explicit test mode reads that fixture and inserts one full-strength,
unmasked identity Convex CurvatureWeathering layer after the cuts/before Rebuild:
sample spacing0.06m, response length0.12m, maximum wear0.06m/iteration,
durability0, iterations4. It never changes the input file or weakens admission.
The serialized candidate is saved before evaluation into a new output directory.

```powershell
cmake --build --preset windows-vulkan-editor --target JericoGeologyGpuSessionTests -- /m:2
build/windows-vulkan-editor/Tests/ProceduralAuthoring/Debug/JericoGeologyGpuSessionTests.exe `
  --actual-working-weathering `
  Tests/ProceduralAuthoring/Fixtures/F4WorkingWeatheringControl.geology.toml `
  build/windows-vulkan-editor/f4-working-weathering-next
```

Use an unused output directory. **Expected current exit is1**. This is an
explicit unfinished acceptance test; it is not disguised as an expected-failure
PASS and does not replace the existing default test mode.

Final build `f4-working-weathering-final-build.log`: exit0. First runtime RED
`f4-working-weathering-red.log`: exit1/86.2415473s. Strengthened final replay from
the durable fixture `f4-working-weathering-final-red.log`: exit1/84.870705s.
All log names are relative to `build/windows-vulkan-editor`.

Verified control and disabled branches:

- Original control succeeds: recipe hash4289218905036011389,
  final field hash7166596416233778912,3512 triangles.
- The native closed-volume validator succeeds despite render-region seams.
- All six derived authored-cut region IDs are present.
- With the added layer disabled, field hash, positions, indices and triangle
  regions equal the original control exactly.

The active branch fails only at `worn.hasValue()`:

```text
procedural.geology.gpu_working_surface_region_operator_unsupported
The working surface region reference could not be prepared.
Working regions require exact cuts and one implicit block; shape Strata must be
one full-strength unmasked pointwise-monotone operation between that block and
the first cut.
```

Candidate outputs are `f4-working-weathering-red-01/candidate.geology.toml` and
`f4-working-weathering-red-02/candidate.geology.toml`; SHA256
`1D93583060B21B4751DBB9D6AD098946175FC26A85ABAC47D6200DECBCE39350`.
Future positive assertions remain pending: changed field and real mesh geometry,
closed mesh, all six cut IDs, one classification per vertex and positive packed
curvature. These unreachable assertions are requirements, not current evidence.
Curvature's existing packed output is curvature/exposure; this test does not
invent an erosion-byte guarantee that the shader currently does not provide.

## Operator versus integration

The existing independent `--curvature-weathering` suite also ran unchanged:
`f4-working-weathering-reference.log`, **exit0/76.0528666s**, Diligent/Vulkan,
RTX3090. It verifies real geometry/field changes, convex/concave response,
CPU parity, durability, iteration changes, masks, zero/disabled controls,
structural-field retention and cancellation. Reported convex curvature11.7581,
concave-10.6201, response1 for each, dirty voxels1249/1561 and localized-mask54.
This proves the existing operator works in its admitted reference path, not in
the actual dense product or every possible CPU/GPU edge case.

## Confirmed consumer gap

1. `GeologyWorkingRegionReference.cpp::validateWorkingRegionOperators` rejects
   active post-cut weathering before GPU dispatch. Keep that protection intact
   until the downstream geometry/region contract exists.
2. `GeologyGpuSessionDiligent.cpp` saves `nativeCutBasis` immediately before the
   first cut (around5243), then binds it for Surface Nets whenever
   `nativeCutRemesh` is true (around7933). Merely admitting the new layer would
   let final field/hash advance while extraction still used the earlier field.
3. `finalizeWorkingSurfaceCuts` applies the exact authored cuts to that extracted
   prefix and reconstructs region-separated surface frames/classifications.
   Moving wear before the cuts changes authored semantics and leaves the final
   cut faces unweathered; it is not a repair of this RED.
4. `workingRegionGeometry` is **not** the actual block/Strata/cut result.
   `buildWorkingRegionReference` starts from the source and skips ImplicitBlock
   and Strata while applying cuts. The current prefix proof separately certifies
   block dominance over the source; exact final cuts then establish their IDs.
   Post-wear projection must not silently treat the auxiliary source/cut mesh as
   the fully generated geometric reference.
5. Existing ordinary nearest-centroid projection accepts only a unique source
   region and rejects ties. The sampled Strata path instead requires its bounded
   whole-triangle block/source dominance proof. Neither is permission to discard
   ambiguity, replace region IDs or weaken the existing proof for this product.

## Next implementation boundary and completeness audit

The next repair must preserve the full fixture, including sampled Strata relief.
First establish a post-cut geometric reference from the actual canonical
prefix extraction/native finalization, then make post-weathering Surface Nets
consume the final resident field. Region transport from that real reference,
source/block dominance, ambiguous boundaries and cut coverage need an explicit
tested contract before admission opens. Do not adopt a no-relief-only mode as a
substitute for the actual-member requirement. Do not move or reapply cuts in a
way that erases the authored late weathering operation.

INTEGRATION REQUIRED, F4, ProceduralAuthoring GPU/geometry owner: final-field
extraction, actual-reference region transport and corresponding LOD path. Inputs
are the canonical prefix geometry/regions and final resident field; output must
be closed classified geometry with stable provenance. Preserve existing work,
memory, cancellation, ambiguity and topology limits; charge any additional
reference/extraction buffers and passes explicitly.

INTEGRATION REQUIRED, F4, GeologyCook/Render/physics owners: carry the resulting
geometry, geological classification and structural fields through cooked mesh,
exact collision and matched native material views. Recipe/source/product hashes,
repeat/fresh-session determinism and source-free package replay remain required.
No new post-wear Beauty image or physical/Shipping result exists yet.

INTEGRATION REQUIRED, F4, authoring/RBrush owner: stable selectable regions and
localized material response on the resulting surface. Existing localized-paint
UV/projection RED remains separate. Final editor UI stays M10.5; the native ImGui
diagnostic host is available without resuming the withdrawn Qt controls.

Runtime formats, ECS, gameplay/Luau/events, Sequencer, animation, audio, particles,
navigation, streaming and destruction receive no API/schema change in this
diagnostic increment. Their existing F4 downstream gates remain open. The
CPU/GPU sign-crossing veto difference is still a separate topology-contract
question; no safeguard was removed and this run does not prove it causes the
actual member's form defect.

Only the explicit test, immutable fixture and durable checkpoint/evidence change.
Production behavior and the189 preexisting dirty entries remain unchanged.
