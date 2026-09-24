# M8 P1 durability-graph research mapping

## Scope and provenance

This note supports only
`M8-GEOLOGY-BLUEPRINT-P1-DURABILITY-GRAPH`. It records the primary-source
research and the independent Jerico conclusions selected before production
implementation or a RED fixture is written.

Primary sources inspected on 2026-08-28:

- Michael D. Jones, McKay Farley, Joseph Butler and Matthew Beardall,
  *Directable Weathering of Concave Rock Using Curvature Estimation*, IEEE
  Transactions on Visualization and Computer Graphics 16(1), 2010, pages
  81-94, DOI `10.1109/TVCG.2009.39`; publisher DOI metadata and the authors'
  institutional BYU ScholarsArchive record:
  <https://doi.org/10.1109/TVCG.2009.39> and
  <https://scholarsarchive.byu.edu/facpub/110/>.
- Matthew Beardall et al., *Goblins by Spheroidal Weathering*, Eurographics
  Workshop on Natural Phenomena, 2007, official BYU Computer Generated Natural
  Phenomena Laboratory copy:
  <https://faculty.cs.byu.edu/~jones/cgnp/papers/goblins.pdf>.
- the official BYU Computer Generated Natural Phenomena Laboratory project page,
  which identifies differential resistance between strata as the core control
  problem for the laboratory's geology work:
  <https://faculty.cs.byu.edu/~jones/cgnp/>.

The IEEE article is copyrighted by IEEE. Its institutional record exposes the
publisher metadata and abstract, but the full PDF download returned HTTP 403 in
this environment. The conclusions below therefore use only claims visible in
the primary institutional record, the official Goblins paper, and Jerico's own
published blueprint. No inaccessible equation, code or implementation detail is
inferred.

The sources were studied read-only. No source, shader, class, data structure,
equation text, pseudocode, image, asset, graph representation, constant or
tolerance from the authors is copied, translated or ported into Jerico.

## Published problem and useful concepts

The Directable Weathering record establishes a small set of relevant facts:

- curvature-driven spheroidal and cavernous weathering is influenced by rock
  durability;
- durability is artist editable before and during a weathering simulation;
- the edited durability graph is intended to control the final weathered
  profile;
- rockfall and colluvium are separate realism stages, not part of the durability
  control itself.

The Goblins paper independently describes voxel resistance that varies between
and within bedding-plane layers. Its weathering amount depends on local exposure
and resistance. Runtime cost depends on active voxels and neighborhood radius,
which reinforces Jerico's existing bounded active-voxel and pass-count guards.

These sources justify a first-class, spatially varying resistance field. They do
not prescribe Jerico's recipe schema, graph representation, composition algebra,
GPU buffers, serialization, hashing, masks or cook boundary.

## Existing Jerico ownership audit

Jerico already owns the required seams:

- `GeologyStrataPayload` already serializes normalized hardness ranges and the
  resident Strata stage publishes a periodic scalar plus composed structural
  mask;
- `FractureCells` already publishes a bounded fracture mask derived from the
  admitted Anisotropic Voronoi structural field;
- every typed layer already owns enabled state, strength, seed, regional mask
  and canonical hashing through `GeologyLayerHeader`;
- `GeologyGpuSession` already owns resident field allocation, bounded Vulkan
  dispatch, field-stage inspection, cancellation, dirty tracking and Surface
  Nets extraction;
- Bubble and Curvature `SurfaceWeathering` already consume a uniform durability
  scalar using Jerico's `1 - D` convention;
- classified Surface Nets, JRMS 1.1 `GEOL`, exact collision and world/triplanar
  PBR already form the static cook/runtime path.

The current gap is precise: Strata hardness values are validated, serialized and
hashed but are not evaluated into a durability field; weathering consumes only a
uniform payload constant. Building another recipe, graph, field evaluator,
mesher, material path or runtime component would duplicate existing ownership.

## Independent Jerico conclusion

Jerico will add one cumulative typed `DurabilityField` layer inside the existing
geology stack and resident Vulkan session:

```text
latest active Strata hardness range + resident Strata scalar
-> bounded canonical piecewise-linear response graph
-> optional resident FractureCells weakening
-> layer strength + existing regional mask composition
-> resident scalar durability field D(p) in [0,1]
-> existing SurfaceWeathering durability input when downstream in the stack
```

The graph maps normalized Strata hardness to normalized durability. The Strata
hardness sample is independently defined from already canonical data:

```text
H(p) = lerp(hardnessMinimum, hardnessMaximum, strataScalar(p))
```

The graph is an ordered piecewise-linear response with two to eight keys over
the full normalized domain. It is Jerico-owned and intentionally simpler than
an arbitrary node graph. A fracture weakening scalar then lowers resistance:

```text
Dcandidate(p) = graph(H(p)) * (1 - fractureMask(p) * fractureWeakening)
Dout(p) = lerp(Dprevious(p), Dcandidate(p), strength * regionalMask(p))
```

All terms are clamped only after canonical validation proves finite `[0,1]`
inputs. Validation rejects invalid authoring state instead of silently repairing
it. Disabled, zero-strength and zero-mask layers preserve the previous field
exactly.

When a durability field exists before an existing Bubble or Curvature
`SurfaceWeathering` layer, that field is the authoritative durability input at
each voxel. When no field exists, the existing uniform weathering-payload value
is retained exactly. This provides a real admitted consumer without adding a new
weathering operator or changing weathering iteration, exposure, curvature,
ping-pong or removal semantics.

## Why durability remains authoring/intermediate data here

The current eight-byte `GEOL` surface record is fully occupied by visible static
classification: strata class/strength, fracture, erosion, deposition, cavity,
curvature, exposure and fresh state. Widening or reinterpreting it would change
JRMS, renderer vertex fetch, materials, collision/query ownership and future
physical/destruction semantics.

This slice therefore keeps durability as a resident authoring/intermediate field.
The existing weathering evaluator is its admitted consumer. Static Surface Nets,
JRMS/collision and rendering remain byte-compatible, and a durability-only layer
must not change signed distance or packed classification. Persisting physical
durability for runtime queries or destruction remains a separately authorized
consumer slice.

## Deliberate limits and risks

- The first graph input is the latest active upstream Strata hardness field.
  Constant, height, noise, material-ID, painted-field and arbitrary graph-node
  inputs remain outside this slice.
- A Durability layer requires upstream Strata. Upstream FractureCells is optional;
  absent fracture input is exactly zero weakening.
- The graph contains at most eight ordered keys and evaluates in one bounded
  voxel pass. It does not iterate or modify geometry.
- Multiple durability layers compose through the existing layer strength/mask
  rule. They do not create branches, dependencies or a second scheduler.
- Existing weathering consumes the latest durability field only when the field
  is present earlier in recipe order. The legacy uniform value remains the exact
  fallback for older recipes.
- No claim is made that normalized authoring durability equals a Jolt material,
  engineering strength, fracture toughness or save-state quantity.
- Runtime physical properties, destruction/remesh, gameplay queries, particles,
  audio, navigation, LOD/streaming and editor semantic views remain
  `INTEGRATION REQUIRED`.

## NO-DONOR boundary

Jerico may use the published concept of artist-editable spatial resistance. It
must not use the papers as an implementation donor. In particular, it will not
import the authors' graph classes, voxel layout, weathering scheduler, rockfall,
colluvium, UI, code, assets, constants or example profiles.

## Explicit scope boundary

This research does not admit new weathering behavior, rockfall, colluvium,
imported-mesh structural operators, runtime destruction/remeshing/save-state,
later P1/P2/M9/Task23 work, OpenVDB, restored Geology UI, named generators or
presets. It does not start
`docs/JERICO_1_0_PRODUCTION_EXPANSION_GEOLOGY_PWAS_EPLS_ANIMATION_DESTRUCTION.md`.

Completing Durability Graph will not complete P1, the Geology blueprint or
JericoEngine 1.0.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
