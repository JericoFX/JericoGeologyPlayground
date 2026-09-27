
# Spider Web Mesh-to-Texture / Card LOD Lab

Owner request: 2026-09-27
Status: IMPLEMENT / VERIFY / LEAVE INTERACTIVE
Repository: JericoFX/JericoGeologyPlayground

## Goal

Generate a spider web as a real strand graph / high-detail mesh for authoring, then automatically reduce it for runtime distance rendering:

LOD0 = real tube/strand geometry
LOD1 = reduced ribbons
LOD2 = one 2-triangle texture card when the web is sufficiently planar
LOD2 fallback = a small locally-planar card cluster when the web is not planar enough

The authored Web Graph remains authority. The texture card is a derived LOD.

Reuse the Playground's existing NormalBake, AutoUv, Renderer, explicit mip-chain, PNG export and evidence/capture infrastructure. Do not build a second unrelated image pipeline. Do not modify JericoEngine.

## 1. One card only when mathematically valid

Fit a best-fit plane to web vertices/samples.

Centroid:
C = sum(P_i) / N

Covariance:
M = sum((P_i-C)(P_i-C)^T)

The eigenvector associated with the smallest eigenvalue is the candidate plane normal n.

For each source point:
d_i = dot(P_i-C, n)

Measure:
maxPlaneError = max(abs(d_i))
rmsPlaneError = sqrt(mean(d_i^2))

A single card is admitted only when both errors satisfy configurable world/relative limits.

If not:
- split into a small number of locally-planar patches/cards; or
- keep the ribbon LOD.

Never flatten a corner/cave web into one quad just to save triangles.

## 2. Card projection

Choose orthonormal plane axes U,V.

Project:
uWorld = dot(P-C,U)
vWorld = dot(P-C,V)

Compute projected bounds plus padding, then:
u = (uWorld-uMin)/(uMax-uMin)
v = (vWorld-vMin)/(vMax-vMin)

For a valid planar web, the final render mesh is two triangles.

## 3. Rasterize the strand graph, not just a screenshot

For projected strand segment A->B and sample point X:

t = clamp(dot(X-A,B-A) / dot(B-A,B-A), 0, 1)
Q = A + t*(B-A)
d = length(X-Q)
r = lerp(radiusA, radiusB, t)

The sample is covered when d <= r.

Use bounded supersampling (for example 4x4 or 8x8 in evidence mode) and resolve fractional coverage:

alpha = coveredSamples / totalSamples

This preserves thin silk better than binary one-sample rasterization.

## 4. Maps to bake

Minimum:
- coverage / opacity
- card-space normal

Evaluate:
- thickness
- height/depth
- roughness/material variation
- distance/centreline field
- stable Edge-ID contributor metadata

Do not pack integer Edge IDs into ordinary filtered RGBA material channels.

## 5. Thin strands: coverage + distance field experiment

Ordinary alpha mips can erase thin strands.

For each texel X find nearest projected strand:
dCenter(X) = min distance(X, strand)
edge(X) = nearest strand
r(X) = local radius of nearest strand

Define:
phi(X) = dCenter(X) - r(X)

Inside strand: phi <= 0.

Store phi using a declared world-space spread, or store centreline distance and radius separately.

At runtime:
aa = max(epsilon, k * fwidth(phi))
alpha = 1 - smoothstep(-aa, +aa, phi)

Compare this against ordinary coverage.

Important: an undersized traditional SDF can also lose very thin features. The baker must measure minimum representable strand width and either raise resolution, use a stronger centreline/radius representation, or reject/fallback.

## 6. Mips

Do not rely only on naive RGBA averaging.

Compare:

A. Source-rerasterized coverage mips:
At every mip resolution rasterize the original strand graph again and compute fractional coverage.

B. Coverage-preserving alpha mips:
Downsample, then rescale/remap alpha to preserve target alpha-test coverage.

C. Distance/centreline mips:
Regenerate the field from the source graph at each mip resolution instead of averaging distance values blindly.

Measure per mip:
- visible strand coverage
- holes/open area
- apparent thickness
- temporal stability
- minimum represented strand width

Do not keep subpixel strands artificially opaque forever; at extreme distance preserve integrated coverage rather than turning silk into one-pixel ropes.

## 7. Card-space normal

For card basis U,V,Nc and source normal Nh:

normalCard = (dot(Nh,U), dot(Nh,V), dot(Nh,Nc))

Normalize and encode to RGB.

Dilation may extend valid normal/color texels into gutters, but must not expand the actual coverage mask.

Front/back rendering policy must be explicit for a two-sided web card.

## 8. Stable Edge IDs and tearing

The cheap LOD must not resurrect a broken strand.

A single nearest-ID texture is insufficient at crossings because one texel can contain more than one strand.

Prototype a bounded K-contributor representation, initially K=2:

ID0 + coverage0
ID1 + coverage1

IDs use integer/non-filtered textures. Coverage remains filtered.

Runtime visibility:
a0 = coverage0 * visible[ID0]
a1 = coverage1 * visible[ID1]
alpha = 1 - (1-a0)*(1-a1)

At hubs where more than K significant contributors overlap, count contributor overflow.

If overflow exceeds a configured limit:
- retain that hub/region as ribbon geometry;
- split into another card;
- increase K only if measured worthwhile;
- or reject dynamic per-edge Card LOD and use a prepared damaged variant.

Never silently keep ghost strands.

## 9. Non-planar webs

Partition a corner/cave web into locally-planar connected patches using bounded region growing:
- seed an unassigned connected edge;
- fit a local plane;
- accept neighboring edges while point-to-plane and angle errors stay within limits;
- refit;
- finish patch;
- repeat.

Typical expectation:
- planar orb web: 1 card
- corner cobweb: 2-4 cards
- highly spatial web: ribbon LOD

Measure card count and overdraw.

## 10. LOD chain

LOD0: tubes / real strands
LOD1: ribbons / reduced strand geometry
LOD2: card / card cluster
LOD3: optional fade/cull under visibility threshold

Use dither/crossfade if needed to avoid a hard pop.

The stable source graph and Edge IDs own every representation.

## 11. Performance metrics

Record:
- source edges
- source vertices/triangles
- ribbon vertices/triangles
- card count
- final card triangles
- texture resolution
- mip count
- total texture bytes
- bake CPU ms
- occupied card coverage ratio
- contributor overflow texels
- minimum strand radius
- minimum represented pixel width

A giant mostly-transparent card is not automatically a win; measure overdraw / coveredPixels-to-cardPixels ratio.

## 12. Interactive Playground UI

Add a Web LOD / Bake panel.

Controls:
- source web preset
- seed
- complexity
- strand radius
- planarity threshold
- card padding
- bake resolution
- supersample count
- mip strategy
- Coverage vs Distance/Centreline field
- minimum strand pixel policy

Preview modes:
- Mesh
- Ribbon
- Card
- Coverage
- Normal
- Distance field
- Edge IDs / contributor overflow
- Card bounds

Damage controls:
- select Edge ID
- break/hide edge
- reset damage

The owner must be able to break an edge, switch to Card, and verify that the accepted cheap LOD does not show a ghost strand.

## 13. Required tests

Planarity:
- exact planar -> one card
- mild warp within tolerance -> one card
- corner web -> cluster or single-card rejection
- deterministic plane fit

Coverage:
- thin strand has non-zero coverage
- holes stay open
- crossing coverage correct
- outside bounds stays empty

Mips:
- declared minimum test strand survives until the configured visibility threshold
- coverage error is measured
- mip bytes deterministic

Normals:
- finite normalized decode
- planar fixture has expected normal
- empty texels follow explicit neutral policy

Edge contributors:
- isolated edge gets correct ID
- K=2 crossing stores both contributors
- breaking one crossing edge leaves the other
- overflow counted and diagnosed
- ID textures never linearly filter

Damage:
- same broken Edge ID disappears in LOD0, LOD1 and accepted LOD2
- no ghost strand in accepted dynamic-card fixture

Budgets:
- reject/fallback when resolution cannot represent strand width
- reject/fallback when contributor overflow is too high
- reject/fallback when planarity is too poor
- reject/fallback when texture/bake budget is exceeded

## 14. Evidence command

Add an automated evidence command such as:

JericoGeologyPlayground.exe --web-card-bake-test Evidence/web-card-bake

Retain:
- source mesh web
- ribbon LOD
- coverage card
- normal-lit card
- distance-field debug
- mip comparison
- intact crossing
- broken crossing
- non-planar corner card cluster
- metrics JSON
- recipe
- deterministic hashes

## 15. PASS gate

Not PASS because a PNG exists.

PASS requires:
- real source web geometry/graph
- automatic planarity decision
- automatic card/card-cluster mesh
- actual graph/mesh-to-texture bake
- thin-strand-aware mip path
- coverage + normal at minimum
- interactive Mesh/Ribbon/Card switch
- accepted planar case reaches a two-triangle card
- accepted broken edge does not reappear as a ghost
- measured geometry/bytes/bake cost
- Release build and CTest green
- evidence retained
- JericoEngine untouched

Owner-facing success:
Create a detailed web, generate the cheap version, switch to Card and see essentially the same web rendered from a tiny mesh plus textures. Break one strand and confirm the cheap LOD does not resurrect it.

HACELO CHOTO: reduce the rendering cost, not a new tubular algorithm.
