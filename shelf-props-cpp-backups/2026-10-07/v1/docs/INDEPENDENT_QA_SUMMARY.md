# Independent shelf accessory QA

Status: PASS on the final generated geometry and the 256-pixel integrated delivery variant.

## Geometry and contacts

- Three props, 30 independently closed, consistently oriented component surfaces; 24,108 triangles. No degenerate triangles after float32 conversion, including the actual exported GLB streams.
- Mug and bottle mouths are geometrically open. A downward ray at each center first hits its internal floor at approximately 5 mm and 10 mm, respectively. No lid or accidental mouth cap is present.
- All eight guard endpoints are embedded in actual lower/upper lantern support solids, proven with solid-angle containment.
- Exact triangle-feature distance in the complete clipped free span z=64–172 mm is at least 1.0141348 mm for guards versus glass and 10.9252125 mm for side rails versus glass. Intentional end-seat and metal-to-metal assembly overlaps are allowed.
- Grab groups cover every intended part exactly once. OBB axes are orthonormal and proper, and boxes contain every prop vertex.
- Lantern geometry/materials contain no emissive material or light extension. Physics and gameplay remain unimplemented metadata.

## Bookshelf preservation and placement

- All 519 original shelf/book/crate parts are structurally identical to the supplied source scene. Original item metadata, transforms, OBBs and grab groups are also unchanged.
- All three props rest on actual shelf triangles, checked at every float32 bottom vertex. There are no positive-volume OBB overlaps with other shelf items.
- Integrated bottom_plane_z metadata now follows the translated support plane. The directed-output regression is retained as evidence of the corrected defect.
- The integrated delivery variant has 549 meshes, 98,126 triangles, 20 material families and 120 embedded images. Its geometry, normals, UVs, material parameters, OBBs and placement are identical to the final 384-pixel variant; only texture resolution/path/provenance differ.

## Export and import

- Native GLB validation checks topology, geometry/source correspondence, finite attributes, normal/tangent frames, UV nondegeneracy, embedded PNG16 CRC/content, material parameters and image provenance.
- The GLB verifier rejects 6 deliberately corrupted fixtures.
- Native OBJ/MTL validation passes; the verifier rejects 21 corrupted fixtures. Source geometry, source maps and material channels were additionally checked through the existing validator API with scene_dir=None because the working scene uses workspace-relative bake paths. No scene or validator was changed to make it pass.
- Read-only Blender 4.3.2 reimport passes for props GLB, props OBJ, final integrated GLB and delivery integrated GLB. Positions are exact to source float32, triangle counts are unchanged, expected texture sizes are retained, and all asset meshes have zero modifiers.
- OBJ/MTL retains geometry, explicit normals, UVs, base color and supported roughness/metallic maps, but cannot preserve physical glass transmission or the full glTF material model. Blender's OBJ importer ignores the MTL norm field; normal PNGs remain in the package for other consumers. No importer repair is applied.
- Delivery ZIP CRCs and member paths verified. Each chosen archive is below 20,000,000 bytes, and the integrated GLB member matches the validated SHA-256.

## Reproduction

Run from the workspace parent so the native scene bake paths resolve, replacing the example directory names if needed:

    python JericoShelfPropsCppLab/tools/shelfprops_independent_qa.py --out JericoShelfPropsCppLab/output-final --shelf-source JericoBookshelfCppLab/output/final256/reference_four/scene.json --report JericoShelfPropsCppLab/verification/final-independent-qa.json
    python JericoShelfPropsCppLab/tools/validate_native_glb.py JericoShelfPropsCppLab/output-final/shelf-props.glb --scene JericoShelfPropsCppLab/output-final/scene.json --report JericoShelfPropsCppLab/verification/final-native-glb.json
    python JericoShelfPropsCppLab/tools/validate_native_obj.py JericoShelfPropsCppLab/output-final/shelf-props.obj --self-test --report JericoShelfPropsCppLab/verification/final-native-obj.json
    blender -b -t 2 --python JericoShelfPropsCppLab/tools/blender_import_qa.py -- --out JericoShelfPropsCppLab/output-final --report JericoShelfPropsCppLab/verification/final-blender-import-qa.json
    blender -b -t 2 --python JericoShelfPropsCppLab/tools/blender_import_qa.py -- --out JericoShelfPropsCppLab/output-integrated256 --files shelf-with-props.glb --report JericoShelfPropsCppLab/verification/delivery-integrated256-blender-import.json

The independent numeric QA uses NumPy only for read-only analysis; Blender is used solely to reimport and inspect. Neither tool produces or repairs geometry/materials.
