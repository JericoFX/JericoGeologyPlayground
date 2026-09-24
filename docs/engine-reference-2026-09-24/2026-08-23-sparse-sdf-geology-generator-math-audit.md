# Sparse SDF geology generator: mathematical audit

Date: 2026-08-23
Scope: GeoGen first; bounded comparison with official Gaea, World Machine, and
SideFX HeightField/VDB documentation. This is a design audit, not a claim about
vendor internals and not a proposal to add a dependency.

**Current-status note (2026-08-27):** this is archival mathematical research,
not current architecture. Its VDB references do not admit OpenVDB, NanoVDB or
PNanoVDB; Jerico geology uses the direct scalar/voxel path defined by
`docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`.

## Executive finding

GeoGen's public material confirms a non-destructive, node-based terrain/planet
workflow with GPU editing, grid and particle simulations, weathering, spline
masks, OpenVDB-related workflow material, and heightmap/mesh/mask exports. Its
official documentation page still says the detailed documentation is work in
progress, so no public source found here establishes that GeoGen internally uses
an SDF, a sparse volume, a particular erosion solver, or a specific noise
formula. The official roadmap explicitly places rock/cliff-face models and
cliffs/overhangs in delayed/future work. Treat GeoGen as a workflow and
parameter vocabulary reference, not as an implementation specification.

The portable mathematical lesson for Jerico is a staged field pipeline:

```text
macro signed field / heightfield
  -> domain-warped low-frequency shape
  -> scale-separated hydraulic + thermal passes
  -> stratified material/hardness masks
  -> talus/deposition mass balance
  -> 2D-to-SDF lift where applicable
  -> sparse 3D displacement, overhangs, and caves
  -> SDF re-distance / narrow-band validation
  -> Jerico cook (mesh + collision + masks/material data)
```

The first five stages are naturally heightfield-friendly. The last three are
where Jerico's Sparse SDF route earns capability that a 2.5D heightfield cannot
provide.

## Primary sources

### GeoGen / JangaFX

- [GeoGen product page](https://jangafx.com/software/geogen) — official claims:
  node-based sourcing/modifiers/filters/masks; real-time GPU terrain editing;
  grid and particle simulations; advanced weathering; spline-based masks; mesh,
  texture-mask, color-map, water-plane, and heightmap exports for terrains;
  octahedral/equirectangular height maps for planets. It does not specify a
  GeoGen SDF schema or erosion equations.
- [GeoGen documentation index](https://docs.jangafx.com/geogen/index.html) —
  explicitly warns that detailed GeoGen documentation is still work in
  progress.
- [GeoGen roadmap](https://jangafx.com/roadmap/geogen) — JangaFX says GeoGen
  development is frozen except for MacOS at that point in the roadmap, and
  describes rock/cliff-face models plus cliffs/overhangs as delayed/future
  capabilities. This is evidence of a public product boundary, not a claim
  that the current build has no private implementation for such effects.
- [GeoGen download/release notes](https://jangafx.com/software/geogen/download)
  — official release history and current beta status; useful for dating the
  public capability surface, not for inferring internal math.

### Gaea / QuadSpinner

- [Dimensions and Scale](https://docs.gaea.app/using/advanced-topics/technical-information/dimensions-and-scale.html)
  — terrain is a normalized heightfield whose values map to a world-space
  width/height ratio.
- [Crafting the Surface](https://docs.gaea.app/using/using-gaea/crafting-the-surface/index.html)
  — official staged workflow: establish broad form, add erosion, then surface
  structure such as strata/outcrops/roughness/local detail.
- [Erosion](https://docs.gaea.app/reference/nodes/simulate/erosion) and
  [Erosion_2](https://docs.gaea.app/using/using-gaea/understanding-erosion/erosion_2/)
  — hydraulic erosion controls include feature scale, rock softness, strength,
  downcutting, inhibition, base level, flow/debris, selective masks, outputs for
  wear/deposits/flow, deterministic mode, and scale-aware processing. Erosion_2
  documents chained coarse-to-fine passes and a thermal component.
- [Stratify](https://docs.gaea.app/reference/nodes/surface/stratify.html) —
  broken strata driven by spacing, octaves, intensity, shape, seed, tilt, and
  direction.
- [Importing Meshes](https://docs.gaea.app/using/getting-started/basics/importing-meshes.html)
  — Gaea is heightfield-first; imported 3D meshes are projected/rasterized to a
  2.5D heightfield.

### World Machine

- [Hurricane Ridge release](https://help.world-machine.com/topic/version-hurricane-ridge/)
  — official new erosion model with feature-size control, soil, spatial
  parameters, repeatable output, plus thermal weathering and talus deposition.
- [Dragontail Peak release](https://help.world-machine.com/topic/build-4059-dragontail-peak/)
  — official VDM path: 3D displacement/noise, VDM combine/compare/detail tools,
  repair, and conversion back to heightfield plus unrepresentable cliff meshes.
  It also states that VDM simulation behavior is not finalized.
- [Legacy Erosion](https://help.world-machine.com/topic/device-erosion/) and
  [Thermal Erosion](https://help.world-machine.com/topic/device-thermalerosion/)
  — hardness masks, flow/wear/deposition outputs, and talus production/removal
  masks/depth.
- [Spatial Parameters](https://help.world-machine.com/topic/spatial-parameters/)
  — a heightfield can spatially vary eligible floating-point parameters rather
  than only provide a binary include/exclude mask.

### SideFX Houdini

- [HeightField](https://www.sidefx.com/docs/houdini/nodes/sop/heightfield.html)
  — `height` and `mask` are 2D volume primitives; height stores elevation and
  masks scale where an operation applies.
- [Heightfields and terrains](https://www.sidefx.com/docs/houdini/heightfields/index.html)
  — the official limitation is explicit: a heightfield is a flat 2D grid and
  cannot represent vertical terrain areas/overhangs.
- [HeightField Erode 3.0](https://www.sidefx.com/docs/houdini/nodes/sop/heightfield_erode.html)
  — thermal and hydraulic erosion, rainfall, erodability, sediment/debris/flow
  layers, feature size in meters, spread iterations, random seed, and chained
  multi-scale passes.
- [Erosion guide](https://www.sidefx.com/docs/houdini/heightfields/erosion.html)
  and [masking guide](https://www.sidefx.com/docs/houdini/heightfields/masking.html)
  — rock hardness/erodability as masks, flow-force/spread tradeoffs, and masks
  as named scalar layers that drive both geometry and parameters.
- [VDB from Polygons](https://www.sidefx.com/docs/houdini/nodes/sop/vdbfrompolygons.html)
  — signed/unsigned distance fields, voxel-size and band controls, negative
  interior convention, and sparse narrow-band storage around a surface.
- [Volume Noise SDF](https://www.sidefx.com/docs/houdini/nodes/sop/volumenoisesdf.html)
  — SDF displacement along the surface normal/closest point/vector, fractal
  noise, gradient warp, band activation, and re-normalization.
- [VDB Smooth SDF](https://www.sidefx.com/docs/houdini/nodes/sop/vdbsmoothsdf.html)
  — curvature/Laplacian/mean smoothing while preserving a valid SDF.

## Representation and ordering matrix

The last column is Jerico design synthesis. It is deliberately phrased as
operations on fields, not as copied vendor implementation.

| Geological concern | Evidence-backed representation/operation | Jerico Sparse SDF math |
|---|---|---|
| World scale | Gaea stores normalized heightfield values and maps them to world width/height; Houdini erosion feature size is in meters. | Keep all amplitudes, feature radii, repose angles, and voxel widths in world units. Store an explicit `domain_scale` and do not let resolution silently change geology. |
| Macro mountain | Gaea/World Machine/Houdini workflows begin with broad primitives/noise, then process erosion. | Start with a broad scalar field `h0(x,y)` or a 3D primitive field `phi0(x)`. Compose with smooth union/intersection and a low-frequency ridged field before detail. |
| Domain warp | GeoGen advertises node/modulator control but publishes no formula; Houdini documents gradient-warped SDF noise. | Use bounded warp `x' = x + a_w W(x)` before noise evaluation; cap `a_w` relative to the current feature scale so topology does not jump unpredictably. |
| Macro/meso/micro | Gaea Erosion_2 and Houdini Erode document feature scale and chained coarse-to-fine passes. | Run deterministic passes from large to small: `F_{i+1}=E(F_i, s_i)` with descending `s_i`; reserve micro displacement for the SDF band/normal stage. |
| Fractal noise | Gaea Stratify documents octaves; Houdini Volume Noise SDF documents fractal types, terrain/hybrid variants, and accumulated gradient warp. | Use normalized fBm/ridged noise `N=Σ a_i n(2^i x')`, with explicit amplitude/frequency normalization and seed. Apply it as field displacement, not arbitrary world-space vertex noise. |
| Strata | Gaea Stratify exposes spacing, octaves, tilt, direction, shape, intensity, and seed. | Build a layer phase `p(x)=dot(n,x)+w_s(x)`; use a periodic band function (triangle/saw/soft threshold) and multiply by a strata mask. For SDF, displace along the normal: `phi' = phi - A m_s B(p)`; retain the scalar layer mask for material assignment. |
| Hardness/material | Gaea and Houdini expose rock softness/erodability masks; World Machine exposes hardness and spatial parameters. | Maintain bounded fields `k_h(x)` (hardness), `k_m(x)` (material weights), and `m_*` (wear/deposit/flow). Map hardness continuously into erosion rate; do not encode material identity in geometry alone. |
| Hydraulic erosion | Gaea and Houdini document rainfall, transport, flow, sediment/deposition, downcutting, base level, and deterministic controls. | In a 2D stage maintain water `w`, sediment `q`, and flow `v`; each iteration conserves mass: rainfall adds `w`, capacity `C=f(|v|, slope, w)`, erosion/deposition changes `h` and `q`, and advection moves `q`. Emit `wear`, `deposit`, `flow`, and `flowdir` layers. |
| Thermal erosion | Gaea Erosion_2, World Machine, and Houdini document thermal weathering and repose/cut angles. | If `|grad h| > tan(theta_repose)`, transport excess height downslope with a conservative relaxation; use `k_h` and a spatial mask to vary production. In 3D, defer true gravity-driven volumetric transport until a solver contract exists. |
| Talus/deposition | World Machine documents talus production/removal masks and talus depth; Gaea documents coarse sediment, scree, debris, and alluvial fans. | Keep bedrock and deposits separate: `h_surface=h_bedrock+d_talus+d_sediment`; derive deposit depth from transported mass and preserve removal sinks. In SDF, deposit as a positive surface offset only where the deposition field is active. |
| Cliffs/ledges | Heightfield systems can create steep slopes/terraces but SideFX explicitly says vertical areas are not representable. Gaea mesh import is projected to 2.5D. | Use heightfield for the primary silhouette; mark a cliff witness mask from slope/curvature. For true undercuts, lift to `phi_hf(x,y,z)=z-h(x,y)` and apply 3D operations in the sparse field. |
| Overhangs | JangaFX roadmap treats cliffs/overhangs as future GeoGen capability; World Machine VDM advertises true 3D displacement and cliff extraction. | Use 3D offset/displacement along `n=-normalize(grad phi)` or explicit primitives; never pretend a single `h(x,y)` sample can encode two vertical surfaces. Validate overhang clearance in the focal acceptance test. |
| Caves/tunnels | No public GeoGen SDF/cave contract was found; heightfield limitation rules out hidden passages. | Subtract closed cave/tunnel fields with smooth boolean difference, e.g. `phi = max(phi, -phi_cave)` under a smooth-max variant; re-distance after composition and retain a topological/voxel witness. |
| SDF conversion | SideFX documents signed distance, negative interior, voxel-size/band controls, and zero-isosurface extraction. | Convert only at the boundary between 2D geology and 3D geometry, or author natively in 3D. Enforce sign, finite band, voxel size, active-set bounds, and re-normalization after displacement. |
| Sparse storage | SideFX VDB docs describe narrow-band active voxels; this is the relevant memory model, not a vendor dependency recommendation. | Keep `phi` sparse around the zero set plus a bounded interior/exterior band. Expand the active set before displacement; trim only after re-distance and validation. |
| Determinism | Gaea, World Machine, and Houdini expose seeds/repeatable or frozen simulation controls; Gaea warns parallel processing can become non-deterministic. | Every generator input includes seed, world scale, voxel size, pass order, iteration count, and solver version. Determinism is part of the cooked result contract, not a UI convenience. |

## 2.5D versus 3D boundary

`h(x,y)` is ideal for broad terrain, drainage, masks, and mass-conserving
erosion, but it has one height per horizontal coordinate. It cannot represent a
cave roof, a tunnel, a true undercut, or two opposing vertical surfaces. A
heightfield-derived SDF is therefore an extrusion/initial condition, not proof
that the source process was volumetric:

```text
phi_hf(x,y,z) = z - h(x,y)       # negative below the terrain surface
```

That field must be re-distanced after large displacement; otherwise its values
are only a signed-ish scalar and not reliable distances. Use sparse 3D fields
for local cliffs, overhangs, caves, and rock assets. Keep large-world erosion in
the 2D stage unless a future volumetric erosion contract specifies transport,
mass conservation, boundary conditions, and acceptance metrics.

## Recommendations for Jerico

### Applicable now

1. Add a geology recipe as a deterministic sequence of pure field stages:
   macro shape -> bounded warp/noise -> coarse/fine erosion -> strata and
   hardness masks -> talus/deposition -> heightfield-to-SDF lift.
2. Represent every auxiliary result as a named scalar layer (`wear`, `deposit`,
   `flow`, `flowdir`, `hardness`, `strata`, `material`) instead of baking all
   semantics into the final distance field.
3. Make feature scale, world scale, seed, voxel size, band width, pass order,
   and iteration count explicit recipe inputs. Capture byte-stable output in
   deterministic tests.
4. Apply normal-directed SDF displacement only after the broad form is stable;
   use band expansion before displacement and re-distance/trim afterward.
5. Add a RED acceptance that rejects heightfield-representable/camera-hidden
   geometry and verifies a real undercut or cave witness, while retaining the
   existing source-free cook/runtime gates.

### Future, requiring a separate contract

1. Native 3D hydraulic/thermal erosion over Sparse SDFs: define volumetric
   material transport, gravity/boundaries, mass balance, and stable narrow-band
   updates before implementation.
2. Procedural overhang/cliff authoring beyond a heightfield witness, with
   explicit topology, self-intersection, active-set, meshing, collision, and
   source-free replay tests.
3. Cave/tunnel boolean authoring with connectivity and watertightness checks;
   do not ship a visual-only subtraction.
4. Multi-material geology where hardness affects erosion and cook output while
   preserving independent material masks and reproducible reimport.

No vendor code, shader, file format, or third-party dependency is proposed by
this audit. GeoGen's undocumented internals remain intentionally out of scope.

## Source status and limits

- Sources were checked on 2026-08-23 and are first-party product/documentation
  pages. The vendor pages may change; the links are the durable source record.
- GeoGen's own docs state that detailed documentation is not yet published, so
  claims about GeoGen's internal SDF/volume representation, exact noise family,
  and erosion equations would be speculation and are excluded.
- World Machine's VDM path is explicitly described by its official release note
  as having behavior still subject to change; it is a comparative design signal,
  not a Jerico contract.
