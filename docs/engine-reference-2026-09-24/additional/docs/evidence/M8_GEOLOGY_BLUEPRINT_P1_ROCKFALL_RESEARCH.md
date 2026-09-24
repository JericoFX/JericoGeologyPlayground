# M8 Geology Blueprint P1 Rockfall Research

Date: 2026-08-28

Slice: `M8-GEOLOGY-BLUEPRINT-P1-ROCKFALL`

Status: research complete; implementation not yet admitted

## Research question

What can Jerico legitimately take from the cited weathering and rocky-scenery
research, and what support, connectivity, units and execution-boundary decisions
must Jerico define independently for the first bounded rockfall slice?

## Primary-source provenance

| Source | Stable locator | Facts used here | Limit |
| --- | --- | --- | --- |
| Jones, Farley, Butler, Beardall, *Directable Weathering of Concave Rock Using Curvature Estimation* (2010) | [DOI 10.1109/TVCG.2009.39](https://doi.org/10.1109/TVCG.2009.39), [BYU institutional record](https://scholarsarchive.byu.edu/facpub/110/), [PubMed record](https://pubmed.ncbi.nlm.nih.gov/19910663/) | Voxel curvature drives spheroidal/cavernous weathering; material durability is artist-editable; rockfall and colluvium are separate realism stages. | The accessible abstract/records do not specify the rockfall trigger, support graph, connectivity algorithm, units, thresholds, detached-mass representation or runtime integration. The full IEEE text was not accessible in this environment. |
| Ito, Fujimoto, Chiba, *Modeling Rocky Scenery Taking into Account Joints* (2003) | [DOI 10.1109/CGI.2003.1214475](https://doi.org/10.1109/CGI.2003.1214475), [author-uploaded copy](https://www.researchgate.net/publication/4023724_Modeling_rocky_scenery_taking_into_account_joints) | Six axis-adjacent voxel bonds; joints cut bonds; connected blocks are identified from the remaining bonds; gravity/support and empty space can cause blocks to fall. | This is a separate primary paper and is not evidence of Jones et al.'s exact implementation. Its random candidate order, limiting-equilibrium/friction model, voxel-relative units and rigid translations are not copied into Jerico. |
| Farley, *Fast Spheroidal Weathering with Colluvium Deposition* (2011) | [BYU thesis record](https://scholarsarchive.byu.edu/etd/2746/) | Weathering and colluvium are iterative voxel processes; colluvium deposition is a distinct material/process. | Colluvium mechanics, angle of repose and per-voxel deposition are explicitly outside this slice and must not be folded into intact-rock detachment. |

Accessed 2026-08-28. The cited records and DOI metadata are provenance, not
code donors.

## Direct findings versus Jerico inference

### Directly supported

- Jones et al. support a voxel weathering representation, editable durability
  and the existence of rockfall/colluvium stages.
- Ito et al. support representing intact rock as face-adjacent voxel bonds and
  treating joints as bond cuts before connected-block analysis.
- Neither source establishes a Jerico-ready SI mass model or a live-runtime
  contract.
- Colluvium is not intact detached rock and must remain a later, separate slice.

### Not sourced and therefore owned by Jerico

The following are `[JERICO SYNTHESIS]`:

- the explicit world-space gravity vector and support plane;
- the exact six-neighbour traversal and deterministic tie-breaking;
- the fracture cutoff used to decide whether a face bond transmits support;
- the way layer `strength` and the regional mask delimit the active topology;
- fixed cells outside that active topology acting as retained support;
- the detached-set/component identities and diagnostics;
- the conversion from detached occupied voxels to cubic metres;
- the authoring-only CPU topology pass between resident Vulkan field evaluation
  and resident Vulkan Surface Nets extraction;
- cancellation points, checked work/readback limits and failure identifiers.

These decisions must not be described as the Jones or Ito algorithm.

## Structural and durability interpretation

The first bounded rockfall operator keeps three concerns separate:

1. **Weathering geometry:** the current negative-inside scalar field is the
   authoritative remaining rock. Earlier weathering may already have removed a
   weak support neck.
2. **Durability:** the resident durability field controls earlier weathering in
   recipe order. Rockfall does not reinterpret normalized durability as force,
   density, friction or tensile strength.
3. **Fracture structure:** the existing normalized fracture channel may cut
   face bonds at an explicitly authored threshold. This is structural
   connectivity, not a physical Jolt constraint.

The resulting lineage is:

```text
BSP source + recipe
-> Strata / FractureCells structural fields
-> DurabilityField
-> existing Bubble/Curvature weathering
-> current scalar occupancy + fracture bonds
-> deterministic support/connectivity pass
-> retained scalar + freshly exposed classification
-> existing Vulkan Surface Nets
-> existing exact mesh collision / JRMS GEOL / package / static runtime
```

## Units and mass ceiling

- Positions, voxel size and support-plane offset use metres.
- Gravity is a normalized direction only; no acceleration magnitude is applied.
- Fracture and activation cutoffs are normalized unitless values in `[0, 1]`.
- Detached geometric volume is exactly
  `detachedOccupiedVoxelCount * voxelSizeMeters^3` and is reported in `m^3`.
- The result is not kilograms, impulse, kinetic energy or a Jolt body. No density
  is invented.

## Selected execution boundary

`OFFLINE_STATIC_FIELD_ROCKFALL` is selected.

The global connected-topology pass is an authoring/cook operation. It consumes a
bounded readback of the already-evaluated sparse Vulkan field, performs one
deterministic CPU topology analysis, uploads the retained scalar/classification
state and then resumes the existing Vulkan Surface Nets path. It does not create
another evaluator, mesh operator, cooker or runtime format.

The rejected runtime boundary would require mutable collision, Jolt bodies,
runtime remeshing, persistence and deterministic replay. Those are broader
destruction/save-state systems prohibited by the active slice.

## Explicit exclusions and future integrations

`INTEGRATION REQUIRED`, not implemented here:

- runtime rockfall/destruction/remeshing and save/load;
- Jolt density, physical mass, rigid-body motion and impact events;
- colluvium/deposition/runout/angle of repose;
- imported-mesh structural operators;
- navigation, gameplay/Luau, particles and audio reactions;
- editor UI/painting/visualization beyond existing bounded field inspection;
- LOD/streaming-aware mutable topology.

## Research conclusion

The smallest coherent Jerico boundary is a deterministic offline/static voxel
topology operator. It can remove unsupported intact-rock components while
preserving the established recipe, Vulkan field, classified Surface Nets,
collision, cook and static runtime lineage. Any stronger physical or runtime
claim would exceed both the available source evidence and the active slice.
