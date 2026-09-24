# M8 P1 DirectedCrack swept-volume research mapping

## Scope and provenance

This note supports only
`M8-GEOLOGY-BLUEPRINT-P1-DIRECTED-CRACK-SWEPT-VOLUME`. It records the
primary-source research and the independent Jerico conclusions selected before
any production implementation or RED fixture is written.

Primary sources inspected on 2026-08-28:

- Brett Desbenoit, Eric Galin and Samir Akkouche, *Modeling Cracks and
  Fractures*, *The Visual Computer* 21 (2005), pages 717-726, DOI
  `10.1007/s00371-005-0317-z`, version-of-record metadata and abstract:
  <https://link.springer.com/article/10.1007/s00371-005-0317-z>.
- the full article copy uploaded under Eric Galin's author profile, inspected
  only to understand the published concepts and limitations:
  <https://www.researchgate.net/profile/Eric-Galin/publication/226590041_Modeling_cracks_and_fractures/links/0deec524e86dac2d35000000/Modeling-cracks-and-fractures.pdf>.
- Eric Galin's official LIRIS publication inventory, which lists both the 2005
  article and the 2004 precursor:
  <https://perso.liris.cnrs.fr/eric.galin/articles.html>.
- Eric Galin's official LIRIS code page, which states that the Arches research
  code is private, restricted to the LIRIS Arches team and not publicly
  available:
  <https://perso.liris.cnrs.fr/eric.galin/code.html>.
- Antoine Martinet, Eric Galin, Brett Desbenoit and Samir Akkouche,
  *Procedural Modeling of Cracks and Fractures*, SMI 2004, official LIRIS copy:
  <https://liris.cnrs.fr/Documents/Liris-1915.pdf>.

Springer's record identifies the article as original work published on
2005-08-31 and provides a rights-and-permissions route. The inspected full
article carries `Copyright Springer-Verlag 2005`. No open-source license was
found for the article, its example atlas, profiles, assets or historical
implementation. The official author code page explicitly says the relevant
research libraries are private.

The sources were studied read-only. No source, shader, class, graph structure,
equation text, pseudocode, profile, image, asset, tolerance, constant, example
scene or mesh from the authors is copied, translated or ported into Jerico.

## Published problem and conceptual flow

The 2005 work is phenomenological and artist-controlled. It explicitly
distinguishes itself from a physical fracture/stress simulation. Its published
flow is:

```text
editable 2D crack pattern and branching graph
-> map/march the graph over a triangle-mesh surface
-> orient profile curves in local surface frames
-> interpolate profiles along graph arcs
-> build a piecewise swept carving volume
-> Boolean-difference that volume from the source object
```

The pattern graph carries connectivity, distances and relative directions. Its
nodes carry profile curves; intermediate sections are interpolated along the
mapped skeleton. The resulting carve is true geometry, not a decal, material
line or indiscriminate surface noise.

The paper's mesh implementation is not a Jerico contract. Jerico already owns a
negative-inside resident scalar field, typed recipe, canonical spline inventory,
field CSG, classified Surface Nets, cook/collision and Diligent/Vulkan material
path. Reproducing the paper's mesh marcher or generalized-cylinder builder would
create a parallel geometry system and is forbidden.

## Published limitations that affect admission

The authors report that consecutive swept cross-sections can self-intersect when
surface curvature is large relative to section size or when the crack turns
sharply. Their mesh-volume construction responds by merging or splitting
skeleton samples according to section radii and segment length. Very rough or
noisy surfaces can still produce interlocking mapped segments; the paper then
uses a smoother carrier surface that embeds the original object to reduce
unwanted tunnels.

Those observations become Jerico test and admission requirements, not copied
algorithms:

- sampling density must derive from voxel size and authored crack width;
- undeclared crossings, collinear overlap and degenerate segments must fail
  before GPU packing;
- the admitted chart is deliberately bounded and cannot claim arbitrary
  distortion-free coverage of a whole noisy object;
- the implicit swept union must not create disconnected holes in the admitted
  product;
- no durability, stress, physical propagation or fragment ownership may be
  inferred from this static authoring operator.

The paper reports example geometry counts and authoring times, not portable
budgets or acceptance thresholds. Jerico does not inherit them.

## Existing Jerico ownership audit

Jerico already owns every seam required by the selected solution:

- `GeologyStackRecipe::splines` serializes canonical `ProceduralSpline` objects
  with stable spline/point GUIDs, width and roll controls.
- `sampleProceduralSpline` supplies validated deterministic arc-length samples,
  stable segment slots, frames, point identity and cancellation.
- `GeologyRecipe` owns cumulative typed layer schema, canonical text, masks,
  transforms, strength, seed and deterministic hashing.
- `GeologyGpuSession` owns the resident negative-inside ping-pong fields,
  Diligent/Vulkan dispatch, bounded resources, cancellation, inspections and
  dirty tracking.
- `GeologyGpuTopologyPlan` owns conservative occupancy bounds.
- classified Surface Nets, JRMS 1.1 `GEOL`, exact collision and the canonical
  world/triplanar PBR renderer own downstream output.

`CaveSweep` already proves that geology recipes may reference the canonical
spline inventory, but its payload is not admitted by the productive resident
GPU layer evaluator. It is not reused as a crack evaluator and is not promoted
into a second field or graph stack. `DirectedCrack` references the same spline
inventory while defining its own narrow static carving semantics.

## Independent Jerico conclusion

Jerico will use one bounded implicit chart rather than reproduce the paper's
triangle-mesh marching and swept mesh:

```text
canonical local-XY ProceduralSpline graph
-> deterministic physical-scale resampling
-> resident-field closest-surface estimate
-> local chart distance + explicit depth profile
-> implicit union of bounded segment volumes
-> existing negative-inside field subtraction
```

At a world sample `p`, the current resident field value `d` and its centered
gradient define an outward normal `n`. The closest-surface estimate is
`q = p - d*n`. Transforming `q` into the layer-local chart provides the 2D
coordinate used to measure distance to the authored spline graph. Crack depth
is `-d`, so the carve follows the existing implicit surface instead of an
unrelated plane or a second surface representation.

The graph remains an inventory of canonical open splines plus stable branch
records that reference spline, parent and attachment identities. The layer does
not generate new paths. Seeded branch probability activates already-authored
child branches only; propagation clips their cumulative authored length.

The cross-section is Jerico-owned: spline-point width interpolates along each
segment, branch depth supplies its physical inward extent and one explicit
piecewise-linear profile curve scales width over normalized depth. Segment
volumes are combined as an implicit union and subtracted through the existing
negative-inside CSG rule. This avoids generalized-cylinder mesh
self-intersection and preserves the only Surface Nets route.

## Deliberate limits and risks

- The first admitted projection mode is a local planar chart evaluated against
  the closest implicit surface estimate. It is not a global UV unwrap or an
  arbitrary curved carrier system.
- Positive uniform layer scale is required. Non-uniform scale is rejected so
  widths, depths, sampling and CPU/GPU parity remain physically unambiguous.
- Splines are open and coplanar in local `z = 0`. Only declared branch endpoint
  joins are allowed; other crossings and collinear overlaps are rejected.
- The current field is not necessarily an exact Euclidean SDF. Projection and
  depth are valid only inside the admitted surface/depth band; zero or unstable
  gradients produce exact identity.
- The profile is bounded and piecewise linear. Arbitrary non-convex cubic
  sections from the paper are intentionally not copied or admitted.
- This static operator creates carve geometry but no physical crack ownership,
  durability, runtime fracture graph, propagation animation or fragments.
- Fresh crack-surface semantic classification remains an explicit downstream
  integration requirement. Existing classification/material lineage passes
  through unchanged in this slice.

## NO-DONOR boundary

Jerico may use the abstract separation of graph, profile, surface mapping and
swept subtraction. It must not use the paper, precursor or private Arches code as
a donor. In particular, it will not import the authors' graph classes, atlas,
curve-morphing implementation, mesh marcher, generalized cylinders, split/merge
code, carrier-mesh implementation, animation data, symbols, example profiles or
assets. No Springer or ResearchGate content becomes a repository or runtime
asset.

## Explicit scope boundary

This research does not admit durability, rockfall, colluvium, imported-mesh
structural operators, later P1/P2/M9/Task23 work, OpenVDB, restored Geology UI,
named generators/presets or bespoke proof geometry. It does not start
`docs/JERICO_1_0_PRODUCTION_EXPANSION_GEOLOGY_PWAS_EPLS_ANIMATION_DESTRUCTION.md`.

Completing DirectedCrack will not complete P1, the Geology blueprint or
JericoEngine 1.0.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
