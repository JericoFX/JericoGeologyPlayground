# Parity matrix — WIP, no blanket identity claim

| Family | Baseline | Native source data | Evaluated mesh and export | Visual gate |
|---|---|---|---|---|
| Industrial window + approved broken glass | User-approved glass reference | 144 fracture cases, including matching rejection and signed64 extreme-seed cases | Native bevel, normals, texture bake and GLB; independent binary validation | Frozen Final native PNG16/1024 regression: RGB MAE 0.000218; earlier comparison is preserved |
| Wood doors/casements | Previously shown timber; consistent R2/SinglePane source | Included in 25 complete wood/single-pane fixtures, max numeric delta 7.33e-15 | Geometry valid; original flat shading reproduced; material-filter and float32 tangent regressions covered | Revised clay MAE 0.000375; final integrated color metrics below |
| Single-pane wood windows | Previously shown basic window | Included in 25 complete fixtures; same supports and sockets | Source and evaluated mesh validity pass; no claim from a source-only JSON comparison | Dedicated final color comparison not yet recorded |
| Industrial props and local damage | Corrected R2 multi-material baseline | 142 Boolean/crack and 406 localized-operation oracle cases, including matching rejections | Native cuts, cracks, edge wear, material groups, bevel and GLB | Four-family color gate includes louvered industrial door |
| Fine directed metal wear | Previously shown 19:21 checkpoint | 20 complete oracle cases, including matching rejections; opt-in rules preserved | Shares verified industrial evaluator and native procedural masks | Dedicated final color comparison not yet recorded |
| Damp wood windows | Development source 19:56; not final user aesthetic signoff | 12 complete fixtures, 193,201 numbers, max delta 8.63e-15; support samples protected | Native evaluated timber and approved glass; no reproduction of the original broken tangent export | Four-family color gate includes old wet timber/broken glass |
| Cabinet/workbench | Development source 19:56; not final user aesthetic signoff | 16 complete fixtures, 2.22M numbers, max delta 6.66e-16; continuous drawer clearance | Cup-handle profile junction defect corrected; source flat and all-smooth weighted normals covered | Revised cabinet clay MAE 0.000432; final color metrics below include open cabinet |

Source comparisons use 2e-12 metric tolerance, with IDs, face indices, material groups and semantic metadata exact. Generator provenance is intentionally different. Source topology equality does not imply identical evaluated bevel topology. Base seeds are signed64, not arbitrary Python integers; derived offsets are computed without signed overflow.

The integrated gate currently passes 26/26 test suites, 60/60 evaluated family recipes and 544 representative evaluated pieces with manifold, volume, UV and normal-hemisphere checks before and after float32 conversion. There are 151 scalar graph samples (maximum error 1.3113e-6) and 12 Bump-normal samples (maximum vector error 1.18141e-6). These are separate numerical gates, not all-family aesthetic approval.

Difficult native bevels retain measured differences from Blender's clamp/miter construction. Selected sampled surface maxima range from 0.0468 mm on glass to 0.514 mm on a cabinet cap. Mixed smooth/flat hardware still uses the earlier native normal construction rather than claiming an exact port of Blender's complete modifier behavior.

The renderer is a frozen-camera Blender viewer of native output. No Jerico/Playground/Diligent runtime, physics, collision or interactive engine integration has been tested here.

## Integrated material previews, 2026-10-06

The final four representative color scenes use 512-reference-density native PNG16 atlases, frozen cameras/lights and 64 Cycles samples. These are measured visual comparisons, not bitwise identity or additional aesthetic approval for the development-reference families. Each corresponding native GLB passed the independent binary validator before rendering.

| Representative | Whole-image RGB MAE (0–1) | Asset bounding-box MAE |
|---|---:|---:|
| Louvered industrial door | 0.00152239 | 0.00297324 |
| Reinforced timber door | 0.00158958 | 0.00248301 |
| Open cabinet | 0.00061826 | 0.00181345 |
| Old damp timber with broken glass | 0.00112817 | 0.00167435 |

Whole-image values include background; the second column of metrics restricts the image to the projected asset bounds. Final timber close-up RGB MAE is 0.00279270 with the same texture bake, compared with the frozen original at that closer camera. No camera parameter is supplied to production material generation.

The read-only GLB reimport images are an additional interoperability gate. Their native-image difference is recorded separately rather than substituting source-preview metrics for portable output.

## Secondary OBJ/MTL transport

The native OBJ/MTL writer passes independent reading for the open cabinet (184 parts, 56,168 triangles) and approved glass window (52 parts, 9,880 triangles), plus 21 corrupted-file rejections. World-space positions, atlas UVs, explicit corner normals, part/material IDs and native image channels are checked. Read-only Blender import retains both meshes and custom vertex normals; its OBJ importer ignores the `norm` material extension. OBJ also lacks physical glass transmission and node hierarchy. These format limitations are explicit and are not repaired by the preview.
