# Playground backend contract for later adoption

The older sedimentary preset 5 no longer authors floating slabs: its six coherent strata share complete paired contact triangles, with zero spacing/drift and final first-contact settling. Coherent layers with nonzero lateral drift, rotation and scale now also interpolate between shared horizon frames; a RED fixture reproduced their previous mismatched contacts, and the final test verifies full cap pairing and closed geometry. The default seed gate checks 128 seeds at low resolution 16 and 32 at high resolution 32; a failed seed-7 cap led to radial attenuation of perimeter deformation inside the cap. Its native simplifier failed the sampled error gate (0.21–0.70 at a requested 0.08), so the standalone bake uses an explicitly labelled **recipe-regenerated low** at resolution 16. The seed-7 high 73,728 → low 18,432 triangle bake measures sampled bidirectional surface error 0.03934, 516,096 covered samples and zero misses at radius 0.20. `lowStrategy` and `sampledLowSurfaceError` accompany the legacy reduction-error field in exported metadata. This route preserves the high source and semantic region matching but does not imply production remeshing, UV/mips, physical gravity or M8 integration. Evidence: `Evidence/2026-09-24-geology-math-checkpoint.md`.

Current 2026-09-24 continuation: schema v9 retains the v8 separation of geological `direction` from optional `maskAxis` (zero inherits the operation axis for v1–v7 compatibility), and open-surface normal relief uses a stable reference direction across contiguous relief operations. The v7 shared angular contact contours, side-only relief, `LayerSettle` and signed high-to-low displacement bake remain. The bake now classifies misses by other-region fronts, nearby backfaces and distance without hidden fallback. `RBrushInterop` binds canonical Engine `RBrushTipIR`/`RBrushPresetIR` to the original Engine 2D mask SDF derivation and local-field contract, with imported Engine hash/validation libraries and tested physical projection. A CC0 cliff image is now imported and roundtripped through the official `.rbrush` container, then sampled in that local field. This is standalone math and official-format intake, not GPU parity, RBrush editor authoring or geology SDF stamping. Current evidence and limits are in `Evidence/2026-09-24-geology-math-checkpoint.md`.

The layered preset checks every pair of contact triangles and smooths matching exterior side normals without moving geometry; this removes misleading dark bands in the Vulkan preview and high-to-low normal bake. Formation now uses the source's local planar/vertical center, so a translated selection retains its shape. Preset 11 uses perimeter amplitude 0.16 after amplitudes 0.28 and 0.22 folded high-resolution contact caps in the expanded seed sweep; 64 seeds at both low and high resolution pass with this default. Its silhouette still resembles a stack of regular slabs. First-contact settling is not a gravity or fracture simulation, and production collision/material/cook integration remains open.

The preceding v8 preset 11 increased the shared perimeter amplitude to 0.28; at 27,648 high triangles its 0.02-error low had 15,406 triangles and the 0.20-radius same-region bake covered 554,616 samples without misses for the tested seed. A later 64-seed sweep found an inverted contact cap, so these are historical measurements, not accepted defaults. The looser 0.04-error low had 62 nearby-backface misses on that same high mesh.

Schema v9 adds `shapeOuterCaps` to coherent layers. When enabled, only the exposed top/base contours taper inward relative to their neighboring contact horizon; internal horizons and triangle-region identity remain untouched. The **current safe default** produces 27,648 high → 13,086 low triangles and 588,870 covered bake samples with zero misses at the same 0.02/0.20 settings. Old v8 recipes deserialize with this flag false and retain the anchored top/base geometry. This is an authoring shape option, not a Boolean union, dynamic gravity, or accepted material/Beauty result.

## Owner decisions for M8 — 2026-09-15

The owner selected the Playground direction as the intended M8 geology backend. The intended authoring flow is **select BSP -> geology stack -> preview/diff -> explicit apply**. This records the product decision, not completion of production integration.

The explicit M8 requirement for creating reusable recipes, plus the documented-only heightmap/RBrush requirements, is recorded in `docs/M8_GEOLOGY_BACKEND.md`. This is the standalone preparation document for later production adoption.

**Production authoring remains M8 work:** heightmap import and applying RBrush. The earlier Playground stop instruction was superseded by the owner's 2026-09-24 request to continue standalone mathematics and RBrush compatibility checks. Both production workflows must be non-destructive and interactive: preserve the original BSP/mesh and imported source, retain editable operations and asset GUID/hash references, provide preview/cancel and undo/redo, respect locked/non-selected geometry, and apply through the editor's transaction/bridge. RBrush must consume its existing M8 asset/mask/projection contract rather than a parallel brush format. Heightmap format/bit-depth, projection, resampling and scale must be decided and tested at that future implementation step.

Heightmap/RBrush authoring are **INTEGRATION REQUIRED — future M8 authoring**, owner Editor/Asset Pipeline/Geometry, milestone of implementation to be assigned. The standalone signed displacement bake is not heightmap import, and canonical RBrush mask derivation is not editor stamping.

## Boundary and consumer

`PlaygroundModel` is a C++20 library with no renderer or ImGui dependency. It uses original engine sources and the existing Manifold library as read-only inputs. `Tests/BackendTests.cpp` is a runnable consumer. `Source/Model.hpp` is the public experimental interface; `SolidGeometry` and `LayerFormation` are implementation helpers.

```cpp
#include "Model.hpp"

auto recipe = Playground::load("rock.json");
Jerico::CancellationSource cancellation;
auto token = cancellation.token();
auto result = Playground::evaluate(recipe, recipe.finalResolution, -1, -1, &token);
// Another thread may call cancellation.requestCancellation().
// Submit result.mesh, result.cornerNormals and result.field to the consumer.
```

`stopAfter` is the inclusive operation index, or -1 for the full stack. `fieldLayer` selects a diagnostic scalar without changing geometry. Both indices refer to the original recipe, including disabled entries. All positions, distances and frequencies use current object-space units. No engine world scale is assumed. The recipe does not store camera, material or runtime entities.

Failures throw `std::runtime_error`; the UI retains the previous valid mesh. The native adapter's error code and cause are preserved. Cancellation uses the actual Jerico token throughout. Native Manifold calls are bounded synchronous work: cancellation is checked at adapter fences and local loops, not promised to interrupt Manifold at an arbitrary instruction. Inputs are immutable snapshots and no mutable global generation state is used.

## Five form operations

1. **Shared Macro Warp:** `p' = p + amplitude * outputScale * vectorFBM(p + domainWarp) * mask`. FBM uses `coordinateScale * frequency`. It moves geometry. Domain Warp changes later sampling coordinates only. Common low-frequency fields and a small vertical coordinate frequency let several layers bend together.
2. **Variable layer thickness:** in an orthonormal basis around layer direction, `T_k(u,v) = clamp(base * (1 + jitter * (rho * sharedFBM + (1-rho) * localFBM_k)), minimum, maximum)`. Cumulative positive thickness plus spacing defines the centered stack. It is artistic correlated thickness, not a geological deposition solver. The option is explicit; v1 remains on the legacy formation path.
3. **Correlated drift:** each offset/rotation/scale channel starts from seeded noise and advances as `state_k = rho * state_(k-1) + (1-rho) * innovation_k`. At rho=0 the layers are independent; at rho=1 they share the state. Drift bias is projected into the layer plane and varies linearly over stack height. Scale is bounded below at 0.15. This is a bounded smoother; its variance intentionally decreases as correlation rises, rather than an equal-variance stochastic process.
4. **Macroform envelope:** successive closed plane intersections. Ellipsoid support is `sqrt((a*nx)^2+(b*ny)^2+(c*nz)^2)`; box support is `a*abs(nx)+b*abs(ny)+c*abs(nz)`; rounded box uses a box reduced by radius plus spherical support; cylinder uses radial ellipse support plus height. Curves are finite faceted approximations, not exact quadrics/SDF surfaces. Scale is half extent; rotation is Euler XYZ degrees, then translation. An enclosing envelope preserves geometry ordering; an empty or zero-volume result reports an error. Optional native linear subdivision preserves the cut surface and supplies vertices for later deformation. It does not improve triangle aspect ratios.
5. **Layer edge breakup:** builds a triangle-distance acceleration structure for each stratum's lateral faces (`abs(dot(faceNormal, layerAxis)) < 0.65`). Weight is `1-smoothstep(distance/width)`. Noise is restricted to this band, with amplitude capped to 20% of authored local thickness. This orientation-based boundary classification is explicit; align its axis with the strata. It is not exact curvature or crack detection. The cap does not guarantee against intersections after arbitrary later deformations.

Structural planes are separate seeded support cuts; Surface/field fracture remains displacement. Neither should be confused with a cutter Difference operation, which is not implemented here.

## Output and semantic lineage

`TriangleMesh` is the actual engine representation. Positions are welded, indices reference positions, and `triangleRegions` has one GUID per triangle. Vertex normals remain continuous for geometric displacement so region borders cannot separate into cracks. `cornerNormals`, when present, has one entry per triangle corner for rendering with hard region boundaries. The renderer expands those corners and counts the actual expanded buffer bytes.

| Output | Meaning and propagation |
|---|---|
| `regions` | Catalogue of source, stratum, plane-cut and envelope-cut regions. Includes ancestor entries; only IDs referenced by triangles are active surfaces. Stratum entries retain parent GUID, generating operation index and stratum index. Cut entries identify their generating operation; a single parent surface is not invented for a new interior face. |
| `layerIndices` | Latest formation's per-vertex stratum index, 0..11. Empty before formation. Legacy layers retain their original face regions, so this array distinguishes their strata. |
| `layerThickness` | Authored thickness at the formation stage. It is not a measurement of final physical thickness after later transforms, cuts or deformation. |
| `layerEdgeDistance` | Distance evaluated immediately before the latest Edge Breakup operation. It is not recomputed by later operations. |
| `field` | Selected diagnostic signal/mask. Coherent Layers shows thickness, legacy Layers normalized index, structural operations mark vertices touching new cut faces, Edge shows band weight, Shared Warp shows vector displacement magnitude. |
| `stages` | Timings and vertex/triangle counts for each enabled operation. Includes that operation's normal recomputation, native work and sample transfer. Excludes primitive construction and final corner-normal preparation. Total `milliseconds` includes those extra costs; it excludes GPU upload/render. |

Cuts and subdivision may reorder and add vertices. The engine triangle BVH projects new samples to the closest source triangle: continuous fields, thickness and domain offsets use barycentric interpolation; categorical stratum index uses the dominant barycentric corner. New caps therefore carry **projected source samples**, not exact volumetric geology. Region metadata records the cap explicitly. A future volume-field pipeline must replace this projection when a consumer needs true interior semantics.

`meshHash` is a **geometry-only** comparison of positions and indices, retained for v1 evidence. Use `PA::hashTriangleMesh` for geometry plus region GUIDs. Neither hash includes the entire RegionInfo catalogue or all scalar attributes; do not use either alone as a production asset/cache identity. A future cache key must include recipe, evaluator version, resolution, dependencies and relevant output schema.

## Limits and versioning

### Surface, BSP and reduction additions (v3)

`Base::Plane` constructs an XY grid with a +Z front normal. `planeSize` is width/height in object units. `MeshResult::closedSolid` distinguishes an open surface from a solid; it is not a certificate against every self-intersection. `StrataRelief` and `RidgedNoise` displace the surface. `Solidify` creates offset front/back copies and stitches oriented boundary edges; back/rim have derived GUIDs and roles. The solid-cliff preset extrudes along a fixed Z direction. Offset along varying normals remains an explicit option and can fold on sharp relief.

`SurfaceGeometry.hpp::bspSource(document, selectedGUIDs, subdivisions, token)` filters the actual engine `BspDocument`, evaluates it with the original engine solid adapter, and subdivides its mesh. `evaluateSource(recipe, source, resolution, ...)` evaluates the stack on that mesh instead of creating a primitive. The source document and unselected brushes are not mutated. Selection GUIDs and observed document revision survive the evaluation; missing/duplicate GUIDs and malformed arrays are rejected. Original CSG order is retained, so an isolated subtract brush is not silently turned into an additive volume. A production bridge must still fence against a changed document revision before Apply; the standalone function cannot observe future edits to another document. The resolution argument does not resample a supplied mesh: source tessellation is explicit in `bspSource`.

`Base::BspWall` is a real-engine BSP fixture for the standalone UI. Arbitrary editor selection binding is not implemented by that dropdown. Recipe JSON stores operations and the fixture choice, not a live engine document or selection. Production recipe binding must remain distinct, as specified in `docs/M8_GEOLOGY_BACKEND.md`.

`StrataRelief`: `s=dot(q,unit(axis))*frequency+phase+distortion*FBM(0.3*q)`, `u=s-floor(s)`. Profiles are sine, a smooth ledge, or triangle; clamp and raise to `profileSharpness`, then displace by `normal*amplitude*(profile-0.5)*mask`. `RidgedNoise` samples the actual engine FBM with one octave at each frequency and combines normalized weighted powers of `max(0,1-abs(noise))`. These are artistic fields, not physical stratigraphy; relief bands do not create categorical stratum metadata.

`Reduce` consumes closed meshes via the existing Manifold 3.5.2 `Simplify` API. GUID runs preserve region boundaries. It attempts up to eight decreasing native tolerances, independently measuring bidirectional distances at vertices, centroids and edge midpoints using the engine triangle BVH. The user limit is in object units and is never raised automatically. A quality candidate that does not reduce triangle count, or exhausted quality attempts, returns the input geometry. Native topology/provenance failures remain visible errors; they are not silently converted into success. Non-interruptible native calls are fenced by cancellation checks.

`sampledReductionError` is **not a certified continuous Hausdorff bound**. `trianglesBeforeReduction` reports the input count; subsequent operations can change the final geometry, so the error only applies at the Reduce stage. Continuous samples are transferred by the existing closest-triangle projection and categorical strata by dominant corner. There is no isotropic/voxel remesh implementation. Dense planar cube reduction and two explicit cliff budgets are recorded in `Evidence/SURFACE_V3_VERIFICATION.md`; a 0.01-unit quality budget currently yields no cliff reduction. This limitation remains open rather than redefining the stricter experiment as a pass.

- Historical v5 recipe behavior remains covered. Current schema v9 accepts v1–v8 and re-saves as v9; v6 cannot encode independent stratum perimeter shaping, v7 cannot encode a nonzero independent mask axis, and v8 cannot encode `shapeOuterCaps=true`. Existing legacy operation behavior remains under regression tests.
- At most 24 operations; base resolution 4..64; at most 240,000 vertices for layer replication; native solid operation triangle budget 240,000. Requested subdivision predicts `triangles * segments^2` and rejects overflow. No silent resolution downgrade.
- The sedimentary v2 preset declares final resolution 32. More layers, larger cuts or more tessellation can exceed budgets and require an explicit recipe change.
- Closed topology and finite geometry are checked in tests and native operations. Arbitrary field deformations are not certified free of self-intersections or suitable for collision. Separate stacked shells are not implicitly Boolean-unioned.
- No GPU stack evaluation: the GPU probe only compares scalar FBM against the compiled engine CPU reference. GPU dispatch-only and CPU reference-plus-check have different scopes and are not a speedup measurement.
- Manifold headers identify 3.5.2. Original engine files and prebuilt binaries have separate hashes in provenance. The engine build's private header/link layout is a portability constraint, not a promised stable public engine API.

## End-to-end completeness audit

Lineage exercised now: **recipe -> CPU fields / formation / native cut -> mesh and region catalogue -> standalone Vulkan renderer / diagnostic field**. Tests also exercise the model directly without a renderer. This proves source-code reuse and a usable experimental seam; it does not prove production asset compatibility or superiority to M8.

| System | Current result / remaining integration | Owner and milestone |
|---|---|---|
| Source/input, CPU processing, serialization | Implemented: versioned recipe, deterministic seeded evaluation, old recipe regression tests. Production source assets/cook are INTEGRATION REQUIRED. | Playground now; Asset Pipeline at a future adoption milestone, not scheduled. |
| Editor/authoring, tooling/UI, undo/redo | Standalone stack, snapshots, cancellation, undo/redo, save/load and preview. Engine bridge, transaction and selection semantics are INTEGRATION REQUIRED. | Playground now; Editor adoption milestone to be assigned. |
| GPU, renderer, shaders, materials | Vulkan preview and scalar compute probe. Production upload/layout/material classification and interior-cap materials are INTEGRATION REQUIRED; inputs are mesh, corner normals, GUID catalogue and fields. | Renderer / Materials, future adoption milestone. |
| Runtime representation, ECS, streaming, LOD | INTEGRATION REQUIRED: cooked format/schema, component ownership, stable IDs, simplification with semantic transfer and residency budgets. | Geometry / Assets / Runtime, future adoption milestone. |
| Physics/collision, destruction/runtime modification, navigation/gameplay queries | INTEGRATION REQUIRED: validated closed solids and material/physical classifications; authoring fields are insufficient alone. | Physics / Navigation / Gameplay, future adoption milestone. |
| Luau, events, Sequencer, save/load | INTEGRATION REQUIRED only for product authoring/runtime exposure: explicit commands, events, typed queries and serialization. JSON Playground recipes are not engine savegames. | Editor / Runtime / Scripting, future adoption milestone. |
| Animation | No animated deformation capability in this scope. If adopted for animation, topology/identity policy must be defined before use. | Animation, milestone unassigned. |
| Particles/audio | No consumer now. INTEGRATION REQUIRED if fracture/impact effects use geology: surface identity, event and physical metadata would need to survive cook. | Effects / Audio, milestone unassigned. |
| Debugging/visualization, profiling/performance budgets | Implemented standalone fields, up-to-operation views, mesh metrics, stage timings, explicit budget errors and original-size captures. Production profilers/debug overlays are INTEGRATION REQUIRED. | Playground now; Tools / Renderer on adoption. |
| Testing, documentation, failure handling | RED/green unit tests, seed matrix, native contracts, Vulkan smoke, A/B recipes, provenance and this contract. Production cook/package/replay/physical tests remain INTEGRATION REQUIRED. | Playground now; adopting subsystem owners later. |

These entries deliberately keep the experiment separate from engine production. There is still more than frontend wiring to verify before an M8 production claim.

## v4: campos de superficie y normal high -> low

Implementado y probado en `Source/SurfaceFeatures.*` y `Source/NormalBake.*`, dentro de `PlaygroundModel`. Son APIs CPU cancelables, sin dependencia de la UI, cámara o renderer. `Main.cpp` sólo coordina el ejemplo y `Renderer.cpp` muestra el resultado.

- `surfaceFeatures(mesh, settings, cancel)`: curvatura firmada `sum(edgeLength * signedDihedral)/(4 * vertexBarycentricArea)`, distancia a aristas por caminos del grafo y contacto mediante rayos hemisféricos de radio finito. Convexo positivo presupone winding exterior. Bordes abiertos quedan marcados; no se suelda topología silenciosamente. Contacto y curvatura son magnitudes diferentes.
- `SurfaceDamage`: desgaste entrante por FBM y máscara; profundidad limitada por 20 % de la altura mínima de los triángulos incidentes y, cuando existe, del espesor local. Rechaza inversión de caras; no certifica ausencia global de auto-intersecciones. Conserva índices y regiones. Curvature, EdgeProximity y Contact se calculan sobre la geometría al llegar a la operación consumidora, no sobre una primitiva anterior.
- `bakeNormals(high, low, settings, cancel)`: proyecta normales del high al marco tangente del low sin cambiar sus posiciones ni índices. Devuelve UV/T/B por esquina, normales partidas y textura lineal RGBA. `exportNormalBake` escribe OBJ, PNG y métricas JSON. El consumidor debe conservar UV, normales partidas y handedness del marco tangente; PNG tiene filas descendentes y OBJ V ascendente.
- Proyecciones explícitas: rayos normales de cage o superficie compatible más cercana. Ambas respetan el radio y, por defecto, GUID de región; rechazan caras opuestas. Un fallo produce normal neutral y alfa cero, contabilizado en `misses`; no se oculta con otra estrategia. `backfaceHits` cuenta impactos aceptados (cero por contrato), no candidatos descartados.

El ejemplo genera primero el high completo, aplica la reducción nativa con presupuesto de error y hornea sobre el low. El error de reducción es muestreado, no una cota Hausdorff certificada. El atlas usa un chart por triángulo y padding local: no es un unwrap productivo ni contiene mipmaps. El normal no conserva silueta. El high, la receta y sus regiones siguen siendo la fuente.

### Reutilización en M8

La directiva del Engine `docs/JERICOENGINE_M8_BLENDER_HEADLESS_GEOLOGY_AUTHORITY_DIRECTIVE.md`, sección 0T, requiere adoptar el backend probado del Playground. Incorporar estas implementaciones y sus tests junto al backend existente; no rehacer los algoritmos desde capturas ni introducir Blender como dependencia runtime. `SurfaceRay.hpp` es soporte interno compartido; `CMakeLists.txt` identifica los inputs nativos ya reutilizados. No se instaló ni incorporó código de addons.

Linaje: receta v4 -> geometría high con regiones -> campos sobre esa etapa -> desgaste -> low reducido -> UV/TBN y normal -> material de preview. Los campos crudos son snapshots de su malla de entrada; deben recalcularse si cambia la geometría. El campo de diagnóstico transferido por operaciones topológicas no constituye una clasificación geológica final. Para una identidad de cook, incluir receta, dependencias, versión, settings de reducción/proyección/atlas y esquema completo; los hashes geométricos solos no bastan.

INTEGRATION REQUIRED para aceptación productiva M8: asset/cook y serialización de low+UV/TBN+textura; material lineal y convención tangente; bridge/transacciones/reimport; LOD/streaming y unwrap/mips; validación source-free y Beauty representativa. Sus owners son Assets, Geometry, Materials y Editor en la adopción M8. Collision, destrucción, navegación, Luau/events y demás consumidores conservan el estado del audit anterior: este normal sólo modifica sombreado y no les añade detalle geométrico. Esta entrega no cierra M8 ni autoriza omitir esos gates.

Resultado medido y limitaciones: `Evidence/surface-features-v4/VERIFICATION.md`.

## v5: native reference mathematics and specialist maps

`RockAddonMath.hpp` exposes independent C++ projector coordinates, explicit blend operators, elliptical damage profile/lip, sphere and segment-crack fields, reinforcement/mortar lattices, deterministic logical IDs and artistic aging fields. These functions do not themselves construct Boolean interiors, reinforcement meshes or production materials. Aging inputs must already be normalized to [0,1]; raw inverse-length curvature requires caller remapping. Element IDs are deterministic hashes, not collision-free GUIDs.

`DamageDecal` integrates the projector into the recipe stack. Translation is projector center; direction is its outward normal; projectorRotation is degrees in its tangent plane; projectorRadius supplies ellipse radii. featureRadius bounds axial reach and only front-facing vertices move. It preserves indices/regions and rejects face inversion. It creates an indentation with a lip, not a through-hole. Preset 12 places it on the visible -Z face. Schema v5 adds these settings and rejects their operation/mask IDs in older schemas.

`SurfaceFeatureFields` additionally exposes positive/negative/absolute curvature, edge-graph distance to an open boundary, independent-radius ambient occlusion, positive normal alignment with exposureDirection, inward first-hit thickness and hit confidence. These are vertex-domain snapshots. Contact and AO use separate settings; thickness is not a volume solver.

`NormalBakeSettings::specialistMaps` projects AO, signed curvature, thickness and region ID using the same accepted high-surface hit as the normal map. The preview enables it. Export writes ambient-occlusion.png, signed-curvature.png, thickness.png and region-id.png alongside normal.png. Curvature maps through 0.5+0.5*tanh(curvature*scale); thickness is divided by thicknessRange and clamped. bake.json records these scales. Region colors are a diagnostic hash of GUIDs, not lossless semantic IDs. Preserve the mesh GUID catalogue for consumers. The atlas, padding, no-mips and projection-miss limitations remain.

Coverage remains PARTIAL for the entire research corpus: full Smart Remesh/Smart Segmentation, cutter interiors, MaterialLayerIR, UV flow, height intervals, VDM, production channel packing/UDIM and downstream Engine adoption are not implemented by these kernels. The v5 end-to-end path exercised here is recipe -> native crater/fields -> high mesh -> reduced low -> projected maps -> export and Vulkan normal preview. Specialist maps are exported/tested; they are not yet a complete PBR material stack.
