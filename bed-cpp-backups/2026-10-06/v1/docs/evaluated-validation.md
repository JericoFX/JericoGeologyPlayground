# Source and evaluated-mesh validity

Source meshes retain the historical source oracle's absolute cross-product-squared guard `1e-20`. Native bevels can validly introduce smaller triangles, so evaluated meshes use a scale-independent degeneracy condition: reject zero-length edges or `cross_squared <= 1e-20 * length_squared(ab) * length_squared(ac)` (sine of angle at most 1e-10). All finite-value, index, manifold, orientation, positive-volume, per-corner UV and unit-normal checks remain active.

This distinction was required by an actual paired oracle, not by removing an inconvenient failure. Damp-wood age0.350001/frame.left has valid native triangles down to4.685e-11 square metres; the original Blender4.3.2 bevel also creates triangles below the source guard, down to7.309e-12. The native case has minimum edge12.46micrometres and minimum angle sine2.948e-6. Casting tofloat32 preserves all triangles: minimum edge12.42micrometres, minimum area4.664e-11 square metres, no zero triangles. Evidence is captured by the evaluator test reports.

Final GLB float32 geometry, UV tangents and handedness are independently checked. Passing binary64 native evaluation alone does not establish portable export validity.

## Shading-normal hemisphere regression

Valid topology and unit-length normals are not sufficient: each exported corner normal must point into the positive hemisphere of its triangle geometric normal. The independent exporter/GLB validator now rejects `dot(Ns, Ng) <= 0`; it does not flip tangent signs to mask that failure. The regression suite checks actual source fixtures in double precision and after float32 conversion.

The original adapters keep timber/painted surfaces flat and smooth selected iron/rust hardware. The native evaluator reproduces the flat policy and the fully smooth FACE_AREA Weighted Normal settings (weight 40, threshold 0.01). Blender's packed custom-normal quantization is not emulated. Mixed flat/smooth hardware keeps the calibrated earlier native construction and is explicitly outside the claim of an exact modifier port.

A cup-handle failure was geometric: two beveled edges at a smooth chain junction produced separately projected intermediate profiles, creating an artificial thin ring. A shared two-region profile removes that ring without recipe or part-ID special cases. Only the four cup handles changed geometry in the selected cabinet; its other 180 parts retained positions and face indices exactly. The corrected cup has 718 faces rather than 1030. Independent Blender smoothing and weighted-normal evaluation of that native mesh both have zero negative corner-normal hemispheres.

## Authoring contract guards

The source validator also rejects unsupported schema versions, non-metre units,
nonfinite node transforms and leaf/frame envelope sweep collisions. Four negative
regressions were observed failing before those omitted guards were restored. The
CLI invokes the furniture-specific continuous full-travel drawer clearance and
interior-volume check before native mesh evaluation; source socket/grip support
checks likewise run before bevel evaluation, where surfaces may move slightly.
The standalone re-export utility validates its loaded native scene before writing
GLB. These changes tighten validation without changing generated valid geometry.
