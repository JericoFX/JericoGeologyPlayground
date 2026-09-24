# M8 F4 pre-cut Strata relief

Date: 2026-09-05. Active slice remains
`M8-F4-GEOLOGY-STRUCTURAL-AUTHORING-AND-ROCK-FAMILY-BEAUTY`.
Baseline: `92e8435d70d112255e04bd6ed3b08947e9703678`.
Status: bounded dense-relief, regressions, deterministic export and native replay
PASS. Beauty remains RED; no full F4 acceptance.

## Reproduced missing behavior

The new `JericoGeologyRockFamilyTests --dense-strata-relief` starts with actual
member1, persisted density16 and all six existing cuts. A separately authored
stack moves the existing Strata before the first cut and sets shape displacement
to0.045m and groove depth to0.015m. The producer never reorders the recipe.
The original geometric witness and its zero-displacement export remain unchanged.

Build `f4-strata-relief-red-build.log`:exit0/13.051s. Runtime
`f4-strata-relief-red.log`:exit1/84.718s at
`procedural.geology.gpu_working_surface_region_operator_unsupported`.
The RED is an actual generation failure, not a missing symbol or shader compile.

## Changed boundary and admission

Reuse the existing GPU Strata calculation and retained pre-cut field. In the
explicit `sampledWorkingRelief` mode, Surface Nets edge positions interpolate that
float32 field and Hermite normals differentiate its trilinear cell polynomial.
The bounded QEF still solves cell constraints; it does not clamp a failed exact
native fit. The previous native/analytic Hermite gather is unchanged for ordinary
working stacks. Exactly the same six native cuts follow extraction, and final
vertex classifications are resampled from the complete authored GPU fields.

This is a sampled-field boundary contract, not an analytic Strata root solver.
One grid edge has one linear interpolated crossing; analytic/intermediate features
absent from the samples are not recovered. No claim is made that the old analytic
multiple-crossing guard executes in this mode. Its work is bounded by eight
corners and twelve edges per cell instead of an unbounded replacement BVH walk.
Zero/nonfinite gradients still reject, and the existing extraction capacities,
closed-volume validation, cancellation and native-cut budgets remain in force.
Resolution adequacy and natural form still require the F4 visual/quality gates.

Admission requires one full-strength, unmasked, unwarped shape Strata after the
ImplicitBlock and before the first exact cut. Shape plus groove amplitude must
stay below the surface band with a float32 margin. For fixed world position,
the existing operation is `F(d)=d+k*max(1-|d|/B,0)`. Its inner slopes are
`1+k/B` and `1-k/B`; `|k|<B` preserves strict ordering. Equality or reversal
is rejected. Other structural operators and source-contact relief remain open.

Region attribution does not rely on a uniquely nearest source face after
displacement. Every sampled prefix triangle must certify implicit-block dominance
over the source throughout its surface. At each proof patch center, the difference
between block scalar and signed source distance must exceed
`(2+abs(obliqueness))*patchRadius + 1e-9`. Source distance is1-Lipschitz;
the block bound is `1+abs(obliqueness)`, including its minimum-scale transform.
Strictly monotone Strata preserves this ordering. Uncertain patches subdivide
only for proof, at most four levels, using a fixed16-entry stack and at most262144
queries per projection. Exceeding the budget rejects visibly. All patches must
retain the active implicit-block region; final cut faces
receive their native cut GUIDs. Source-contact cases reject visibly as
`procedural.geology.gpu_strata_source_boundary_unsupported`. This bounded mode
does not claim general transport of displaced source-face identities.

## Tests and regression boundaries

- GPU known answers cover planar and mixed trilinear derivatives at interior and
  corner positions, plus degenerate and nonfinite rejection.
- Admission tests cover layer order, missing cuts, duplicate shape Strata,
  masks, strength, transforms, warp and the monotonicity bound. Existing
  post-cut Strata rejection and ordinary native region tests remain.
- Actual member generation checks closed positive-volume cook/collision, support,
  every original cut halfspace, surviving cut/block GUIDs and classified data.
- The amended amplitude comparison uses two positive amplitudes in the same
  sampled mode, so a changed mesh cannot be attributed merely to switching
  between analytic and sampled reconstruction.
- An intentionally narrowed source tests source-boundary rejection, followed by
  deterministic regeneration of the original candidate after that failure.
- `--export-dense-strata` reuses the ordinary four-member package exporter and
  fresh-session byte comparisons. Only member1 is the dense Strata witness;
  the other three remain canonical controls. The material is still diagnostic.

Initial implementation build `f4-strata-relief-build-02.log`:exit0/53.470s;
Hermite `f4-strata-relief-hermite.log`:exit0/25.478s; topology
`f4-strata-relief-topology.log`:exit0,1/1,2.442s; first actual member
`f4-strata-relief-green.log`:exit0/105.532s,3072 working ->2840 final triangles.
These precede the strengthened source-clearance/control tests and are not the
final verification. The first whole-triangle clearance guard rejected the actual
member (`f4-strata-relief-final.log`,exit1/86.334s); its global maximum-amplitude
bound was replaced by adaptive dominance certification without changing the rock
or its cuts. Build `f4-strata-relief-region-proof-build.log`: exit0/25.409s;
strengthened `f4-strata-relief-region-proof.log`: exit0/108.678s. This includes
positive-amplitude comparison, adverse source rejection and deterministic recovery.
Read-only review found no false acceptance in the dominance inequality; mixed
source/block surfaces and positive LOD remain outside this demonstrated result.
A mistaken standalone Hermite target and a test member-name
compile error are retained in the build logs; neither is a feature RED.

Final ordinary dense control `f4-strata-relief-control.log`: exit0/93.755s,
3072 working ->1926 final triangles. Canonical-source regression
`f4-strata-relief-canonical.log`: exit0/91.278s. Fresh four-member export
`f4-strata-relief-witness-01-export.log`: exit0/181.939s,12 entries, byte-identical
mesh/collision regeneration in a fresh session; `accepted=false` remains deliberate.
All logs are under `build/windows-vulkan-editor/`.

## Package and original native images

Package: `build/windows-vulkan-editor/f4-strata-relief-witness-01/candidates.epak`,
19813722 bytes, SHA256
`102E420328A92E10324D1B9C3ECCAD1F4D2AF9F3986C7CA8D4695C161140F523`.
Member1:1760 render vertices/2840 triangles; source8071145322725865162,
recipe17835284073156893995, surface16504915257722493140.
Native finalization CPU3531475900ns, classification GPU10592ns,
retained pre-cut field1048576 bytes. These are distinct diagnostic timings,
not a cross-hardware budget or complete generation/frame measurement.

Native replay uses only the package in
`build/windows-vulkan-editor/f4-strata-relief-replay-01/Packages/`.
`front-capture.log` and `side-capture.log` both exit0, Vulkan,
`source_free=true`, `fallbacks=0`,1280x720. The unchanged cooked proof executable
loads member GUID `B8130000000000000000000000000106`; scale1, worldY-1.4,
cameraZ5, pitch-15, verticalFOV40 and yaw25/135 match the previous witness.
The RTX3090 capture has no valid GPU timing samples and proves no lower-tier
performance acceptance.

Original images, inspected at native resolution:

- [Front](images/m8-f4-strata-relief-witness-01/member-1-front.bmp), SHA256
  `35B352536ACFD133AC00D71EDFEAF2888162D14D2365663EF5FD4C2A8C5F4768`.
- [Side](images/m8-f4-strata-relief-witness-01/member-1-side.bmp), SHA256
  `BBDC72F3C7DCA7F9EED8C4ABAF7AAD5380E55F2A16B885ABE6BE94C26A567C80`.

Visual result: relief visibly changes the silhouette between retained flat cut
faces, with continuous bands across the side. Bands remain too regular and the
large facets/material too artificial. This is useful structural evidence, not an
accepted natural rock or the requested CryEngine lighting target. The other three
members are unchanged canonical controls; no four-dense-member claim follows.

## End-to-end audit

### Editor control increment (bounded PASS)

The existing Geology World panel exposes Strata spacing, shape displacement and
groove depth in metres only when a Strata layer is selected. Each edit updates
only that payload field and uses the existing candidate validation, generation
invalidation and transaction flow. Loading values blocks signals; editing a
payload or seed does not quantize an unrelated precise strength value.
Generation/pending-publication/stale/recovery locks remain shared with the panel.
No recipe schema, cooker, runtime material or geometry algorithm changes here.

RED: `f4-strata-controls-red-build.log`, exit0/62.390s;
`f4-strata-controls-red.log`, exit1/11.500s, missing the three widgets in the real
Qt panel. The controls-only test additionally covers layer changes, invalid zero
spacing, exact Cancel and unrelated floating-point precision. The full workspace
test generates an edited canonical candidate, previews/cancels, applies the
persisted payload, checks its cooked recipe identity and restores/reapplies exact
project snapshots. This editor fixture exercises canonical Strata; the bounded
dense rock acceptance above remains separate evidence, not a claim that the
editor fixture is the final rock-family Beauty product.

Build `f4-strata-controls-green-build.log`: exit0/71.031s; focused controls
`f4-strata-controls-green.log`: exit0/18.170s. Read-only diff review found no
concrete control/locking/precision blocker. The first full workspace attempt
reported `input.good()` because a new assertion read `SourceAssets/Geology/Source.rmesh`;
the full generation fixture intentionally does not create that snapshot. The
assertion now compares against the retained applied-project snapshot. Production
code is unchanged by this verifier repair. The failed full run is retained as
`f4-strata-controls-workspace.log`, exit1/299.300s. Corrected build
`f4-strata-controls-final-build.log`: exit0/48.380s; full workspace
`f4-strata-controls-workspace-02.log`: exit0/299.999s. The original120s per-generation
wait remains unchanged. Markers confirm Strata payload publication,
Preview/Cancel/restore/reapply, density4 publication, native reprepare and ordinary
fracture bake/restore/reapply. All processes retired. This is a functional Qt/Vulkan
integration result, not a new Beauty image or a hardware performance acceptance.

Next visual seam, inspected read-only: `GeologyGenerationJob.cpp` passes the
selected material GUID to `cookGeologySurface`, with `materialCount=1`. The
rock-family diagnostic exporter instead writes constant 1x1 textures and a new
brown PBR payload, retaining only the supplied material's pipeline/archive
metadata. Therefore the next actual-rock capture should use an authored opaque
material and its cooked texture dependencies through the existing selection and
package contracts. Multi-material strata/fracture response authoring is still
INTEGRATION REQUIRED in F4; exposing three Strata shape controls does not add
palette selection or response routes. No new renderer family is justified by
the diagnostic export's deliberately flat appearance.

Source identity/subdivision, recipe serialization and ordered layers are retained.
GPU scalar generation and CPU native finalization share the existing pipeline;
the changed conversion is sampled Hermite placement and guarded region handling.
Cooked mesh/classification, structural fields, exact collision and source-free
replay retain existing formats. No new renderer/shader family or dependency.
The new diagnostic flag is authoring telemetry, not runtime serialized data.

INTEGRATION REQUIRED in F4: transport of displaced native source regions and
source seams (geology authoring owner; input is source topology/region provenance
plus deformation), remaining structural payload controls beyond the three Strata
fields proved above, actual dense rock-family publication and Beauty acceptance,
localized RBrush painting, actual-member LOD acceptance, full four-member Beauty,
and generation/UI/hardware budgets. Existing navigation, destruction and streaming
consume the same cooked geometry/collision/structural contracts; their broader
F4 gates remain. This change introduces no ECS, gameplay, scripting, events,
Sequencer, animation, audio or particle interface. A local generation PASS must
not close those required integrations or the persistent Goal.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
