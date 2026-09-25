# F4 stepped source with authored Strata: concrete integration RED

2026-09-06. Parent `ee507eb0eba2f4c1eb0330936216659967484bd0`.
Status: **reproduced implementation limit; full-layer Beauty not generated**.
Persistent Goal ACTIVE. No new geological algorithm was implemented.

## Bounded comparison

The preceding stepped witness disabled Strata relief/grooves to isolate source
structure. This comparison restores the EXACT authored values from
`Fixtures/F4WorkingWeatheringControl.geology.toml`: shape displacement0.045 m
and groove depth0.015 m. Parsed A/B recipes differ ONLY in those two fields,
both before and after weathering. Seed, source52/64 occupancy, three merged
boxes, six cuts, phase warp, material, durability, weathering, working density16,
voxel0.06 m, QEF, LOD budget3000 and angular gate35 degrees are unchanged.

The existing native test harness now exposes `--stepped-layered-beauty`; this
only avoids its diagnostic Strata reset. Existing commands/defaults are unchanged.
The output names remain stepped-structural and stepped-weathered; in this new
mode BOTH include the authored Strata relief, and only the latter includes wear.

Reproduction, from the authoritative checkout:

```powershell
$env:JERICO_GEOLOGY_SHADER_CACHE_DIR='F:/JericoEngine/build/windows-vulkan-editor/f4-lod-cause-cache'
& build/windows-vulkan-editor/Tests/ProceduralAuthoring/Release/JericoGeologyGpuSessionTests.exe --stepped-layered-beauty Tests/ProceduralAuthoring/Fixtures/F4WorkingWeatheringControl.geology.toml build/windows-vulkan-editor/f4-strata-warp-material-01/member-1.rmesh build/windows-vulkan-editor/f4-stepped-layered-boundary-01
```

The output directory must be new. The material reference is the unchanged prior
native cooked witness; the earlier evidence retains its identity. Existing cache
WIP is not admitted by this reproduction. First run `f4-stepped-layered-beauty-01`
and the diagnostic replay above both exit1, with no cooked mesh/collision or
Beauty image published for B.

## What actually rejects

Both branches report `procedural.geology.gpu_strata_source_boundary_unsupported`
from `projectWorkingSourceRegions`, in its `sampledWorkingRelief` branch.
That branch only assigns the implicit block's region after proving block
dominance over the whole output patch. It deliberately rejects source-owned or
uncertifiable patches; transport of native source regions is not implemented there.

The error now includes triangle, subdivision depth, signed dominance gap and
patch radius. This is diagnostic text only; the test and production predicates
are unchanged. The native replay records:

| Candidate | Triangle | Depth | Dominance gap, m | Patch radius, m |
| --- | ---: | ---: | ---: | ---: |
| With Strata, before wear | 634 | 0 | -0.082088 | 0.044997 |
| With Strata and wear | 634 | 4 | +0.003904 | 0.002395 |

The first case is an actual source-dominant sample, not just exhaustion of the
subdivision budget. Raising depth cannot make that negative dominance gap pass.
The second case is distinct: its positive gap is insufficient to certify the
complete small patch within the current depth bound. It does not prove that
every point on that patch belongs to the source. No cap or tolerance was raised.

## Concrete conclusion and next repair

1. **Confirmed:** the current implementation cannot evaluate this fixed authored
   Strata/source combination. This is an explicit region-transport limitation,
   not evidence that GPU fields, QEF or the desired rock appearance are
   mathematically impossible. Overall Beauty quality remains unassessed for B.
2. It explains why the layered stepped source fails before native cuts/weathered
   finalization/LOD can complete. It does not explain every aesthetic defect of
   the earlier brown, rounded control and does not reopen the PlaneCut diagnosis.
3. The required repair is source-region correspondence through the existing
   Strata stage, using the existing source regions and canonical mesh/cook
   consumers. Simply removing this guard, assigning the block GUID, turning off
   Strata, enlarging the source, or changing seed/resolution would evade the case.
   An exact minimal algorithm has NOT been demonstrated by this diagnostic;
   generic provenance, permanent buffers/schema changes and future architecture
   are not authorized by it.
4. The next concrete blocker is this source/Strata integration RED. Start from
   the two retained candidate recipes and the existing source geometry/region
   data; demonstrate bounded correct correspondence before accepting a repair.
   Then rerun the SAME full stack, regional LOD/cooks and matched native Beauty.
   Do not resume parameter search or call the control four-rock acceptance.

This bounded comparison is closed as evidence. No further visual generation of
the rejected combination is useful until its identified integration is repaired.

## Regression, integration audit and retained evidence

Both Release builds exit0. Original `--stepped-source-beauty` exits0 and ALL11
recipe/mesh/collision/dump files match the prior accepted technical witness byte
for byte. `procedural.geology_stepped_working_lod` passes10.54 s with the existing
19.752209-degree result and unchanged checks. No renderer, shader, cook, physics,
format, LOD algorithm or material implementation changed. The user's preview
PID33112 remained open and responsive; its draft was not altered. No concurrent
performance conclusion is drawn.

Lineage audit: source regions exist upstream, but the sampled Strata branch only
supports the implicit-block-owned subset. Source-region transport is therefore
**INTEGRATION REQUIRED** for M8 procedural authoring before this combination can
reach mesh, regional LOD, cook, collision and renderer. Input: original source
geometry/region GUIDs and the actual displaced surface. Expected interface: the
existing working-surface region projection/finalization, with certified ownership
and explicit rejection where unsupported. Owner: ProceduralAuthoring geology;
dependency: source/Strata correspondence proof; milestone: current M8 F4.
No new runtime semantic channel is asserted or introduced. Other consumers and
full M8 acceptance remain governed by the active slice.

Archive `data/m8-f4-stepped-layered-boundary-01/proof.zip`:12 rehashed entries,
26,296 bytes; SHA256
`62b65f91cee50c9d8579ce4ae70f3d8f6c14a6ecdcf3e6d32f238d2989736e08`.
It retains both A/B recipe pairs, parsed difference, both native RED logs,
build logs, passing control/regression and exact11-file comparison. The adjacent
manifest contains every entry hash. No new successful Beauty capture exists.
