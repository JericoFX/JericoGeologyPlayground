# M8 Geology Blueprint P1 — Colluvium research

Date: 2026-08-28

Slice: `M8-GEOLOGY-BLUEPRINT-P1-COLLUVIUM`

Research status: `PARTIAL` for the internal algorithm in the cited paper/thesis; `PASS` for the bounded architectural facts used by this slice.

## Question

Determine what the admitted primary sources actually establish about colluvium,
then design an independent Jerico capability that follows the already published
Rockfall lineage without copying or porting a reference implementation.

## Primary-source record

| Source | Primary record | Facts available to this research | Access result |
| --- | --- | --- | --- |
| Jones et al., *Automatic Generation of Voxel-Based Procedural Terrains*, IEEE TVCG 2010, DOI `10.1109/TVCG.2009.39` | [BYU faculty publication record](https://scholarsarchive.byu.edu/facpub/110/), [PubMed record](https://pubmed.ncbi.nlm.nih.gov/19910663/), IEEE document `4815233` | The method operates on voxel terrain; mean curvature drives convex spheroidal and concave/cavernous weathering; durability is editable; rockfall and colluvium improve realism. | Abstract/metadata available. Full IEEE text was not accessible in this session. |
| Farley, *Voxel-Based Terrain Generation*, BYU thesis, 2011 | [Official BYU thesis record](https://scholarsarchive.byu.edu/etd/2746/) | The thesis describes a simple deposition algorithm that produces piles of colluvium from rock broken away by weathering, and investigates GPU/OpenCL execution. | Abstract/metadata available. The official download was blocked by Cloudflare in this session. |
| Jones, Beardall and Egbert, *Goblin XNA: A Platform for 3D AI Research* / procedural geology discussion | [Primary author-hosted PDF](https://faculty.cs.byu.edu/~jones/cgnp/papers/goblins.pdf) | The described geological sequence separates deposition/lithification, fracturing, weathering, and transport; purely local sampling is insufficient to model accumulation such as colluvium correctly. | PDF accessible. |

The access failures above are evidence limits. This document does not infer the
unavailable deposition recipe, constants, API, units, neighborhood, ordering,
conservation policy, masks, seeds, bounds, or runtime behavior.

## Findings admitted from the sources

1. Colluvium is transported/deposited material originating in rock that has
   broken away; it is not merely a recoloring of still-resident rock.
2. Accumulation needs non-local information. A point-local material or shader
   rule cannot represent transport and settling faithfully.
3. The published context is voxel terrain and static terrain generation. It
   does not establish a Jerico runtime destruction, remeshing, persistence, or
   gameplay-state contract.
4. The sources establish the problem and broad data relationship, not an
   implementation that Jerico may copy.

## Existing Jerico lineage inspected

The current Jerico implementation already provides the legal upstream and
downstream seams needed for a narrow capability:

```text
classified sparse voxel field
-> Rockfall six-face support and detached components
-> removed detached voxel volume plus fresh exposure
-> classified Surface Nets mesh
-> JRMS/material/collision/cook consumers
```

Relevant existing contracts:

- `GeologyRockfallReferenceResult` identifies deterministic detached
  components, detached voxel count, exact voxel volume, and canonical hashes.
- the productive Diligent session applies Rockfall to the classified field and
  preserves the field/classification buffers for the next static authoring
  stage;
- `GeologySurfaceClassificationSample::deposition` already occupies an admitted
  packed classification byte and survives classified meshing, JRMS, material,
  and cook lineage;
- the existing heightfield erosion sediment/deposition path is a distinct
  heightfield simulation and cannot substitute for volumetric Rockfall lineage
  without creating a parallel system.

## Facts not demonstrated and therefore not imported

- a specific cellular transport or repose-angle algorithm;
- a continuous granular/fluid solver;
- oblique-gravity voxel traversal semantics;
- runtime/Jolt ownership or mutable destruction;
- a generalized remesh/save-state framework;
- imported-mesh structural operators;
- a new geological material schema or a bespoke C++ generator/preset.

## Independent Jerico constraints derived for design

These are Jerico design requirements, not claims attributed to the references:

1. `offline/static` is the smallest coherent boundary: consume the immediately
   preceding Rockfall result during authoring/cook and bake the result through
   the existing classified-field pipeline.
2. Rockfall component identity and detached volume must remain the source of
   truth. Colluvium may place only already-detached material; it must not
   redetect fracture or support.
3. Conservation must be exact in integer voxel units:

   ```text
   detached source voxels = deposited voxels + explicitly unsettled voxels
   ```

4. Ordering, bounds, masking, cancellation, failure, identity, and discarded or
   unsettled remainder must be observable and deterministic.
5. Deposited cells must preserve source geological classification and mark the
   existing `deposition` channel; downstream geometry, material, collision, and
   cook paths remain the published Jerico paths.
6. No runtime mutable state, Jolt integration, generalized destruction/remesh,
   or alternate material/cook route is authorized by this slice.

## Research conclusion

The primary evidence is sufficient to justify a bounded, non-local deposition
stage that consumes Rockfall-detached voxel material and produces a conserved
static classified field. It is not sufficient to justify copying a reference
algorithm or claiming historical constants and mechanics. The implementation
design must therefore be independently specified, deterministic, bounded, and
explicit about any deliberately unsupported transport case.
