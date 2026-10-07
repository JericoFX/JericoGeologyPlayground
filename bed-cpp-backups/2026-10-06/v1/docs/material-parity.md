# Native procedural material port

## Production boundary

`src/materials.cpp` is compiled C++17. It creates all pixel values from the recipe, source part metadata, metric object-space position, and deterministic algorithms. It does not invoke Python or Blender, load reference textures, execute source shaders, or copy pre-baked input assets.

The immutable source snapshots remain under `fixtures/sources`. `tools/material_oracle.py` is an **offline test oracle only**: it constructs the original source graphs in Blender 4.3.2 and measures them through emission into linear 32-bit EXR. Production texture generation never calls it.

Implemented families:

- Approved fracture glass: metric rebate/bottom dust, fixed-seed 11 analytic scratch segments, clean-glass smoothness, separate glass edge material, transmission, IOR, coat, localized bump.
- R2 gray and dark steel: local weather bounds, seed offsets including Python-compatible negative modulo, bottom corrosion, edge wear, exact-side region/protection clipping, cut metal and legacy cut glass.
- Fine wear: front-only coating breakup, including hardware iron/rust, with color, roughness, metallic changes. Only scenes explicitly tagged `metadata.native_material_profile="finewear_1921"` select this adapter. R2 ignores free `fine_breakup` config metadata; see the adversarial regression in `material-coverage.md`.
- Source base wood, exposed rough wood, damp wood with age/wetness/local height, ordinary hardware.
- Furniture: axis-oriented grain, SHA-256 part-ID offset, endgrain, patch coating, world-height dampness, hardware and paper.

## Noise and ramps

The native noise is the original Cycles normalized 3D Perlin/fBM algorithm, using 32-bit floats, Jenkins Lookup3 hash, the source gradient function, 0.982 normalization, integer and fractional detail, lacunarity 2, and source large-coordinate wrapping. It is not a replacement noise library.

Source references (Blender v4.3.2):

- [Cycles Perlin noise](https://github.com/blender/blender/blob/v4.3.2/intern/cycles/kernel/svm/noise.h)
- [Cycles fractal noise](https://github.com/blender/blender/blob/v4.3.2/intern/cycles/kernel/svm/fractal_noise.h)
- [Cycles hash](https://github.com/blender/blender/blob/v4.3.2/intern/cycles/util/hash.h)
- [ColorRamp import](https://github.com/blender/blender/blob/v4.3.2/intern/cycles/blender/util.h#L137-L151)
- [Cycles ramp lookup](https://github.com/blender/blender/blob/v4.3.2/intern/cycles/kernel/svm/ramp.h)

ColorRamp reproduction includes the source's 257-entry lookup table, not merely direct piecewise interpolation. This distinction was detected by a real window-sill sample near the corrosion threshold: ignoring the table produced a roughness error of approximately 0.00096. The native implementation now includes table interpolation, and the threshold sample is retained in the oracle set.

Installed source hashes used during implementation:

- noise.h: `12419f99f4f80963ddd401d3744380e691dab4d9046223c7ab6b680c3cf9bdcc`
- fractal_noise.h: `87ed644943e9d1db5c4a949f851875602150252822aae15409352183b416f875`
- util/hash.h: `b9def855aa97d3a8ab7ce7591592bf1ffd69d292fcdf464b01575fa48e738d12`

## API and coordinate contract

`sample_material(part, material, object_position)` returns linear base color, roughness, metallic, transmission, IOR, coat, raw bump height, bump strength, bump distance, and specular IOR level.

Positions are in the original mesh object's local metres. Node translations/rotations do not affect Object coordinates. Furniture's world-height staining additionally accepts `_world_position`; the baker obtains it from the node hierarchy. Internal selection metadata `_surface_side` selects an exact front/back wear variant; `_material_family="furniture"` disambiguates hardware shaders from the window family. `_material_family="fine"` selects the FineWear adapter deliberately; the baker derives it only from the explicit scene profile, never from a config flag.

`bake_material_textures(scene, directory, resolution)` preserves `part.uvs` and writes `part.texture_uvs` as independent packed per-face texture coordinates. It preserves face material groups and selects `face_material_sides` when supplied, otherwise the geometric front/back normal. This lets evaluated bevel faces inherit the original source material side.

The resolution argument specifies the reference density. Images begin area-adapted and grow when chart gutters would otherwise collapse the density. Therefore an unusually fragmented part can exceed the requested reference image size. The report records the actual resolution, minimum texels/metre, coverage and islands. Each chart rounds its two dimensions up to texels, so even submillimetre bevel strips receive texel centres. Metric UVs remain unchanged. Three-texel gutters are extruded without crossing into other islands.

Texture keys and encodings:

| Key | Encoding | Meaning |
|---|---|---|
| `base_color` | RGB16 PNG, sRGB | Linear sampler values converted with the sRGB transfer function |
| `material_params` | RGB16 PNG, Non-Color | R roughness, G metallic, B transmission |
| `surface` | RGB16 PNG, Non-Color | R coat, G specular IOR level, B (IOR − 1) / 2 |
| `normal` | RGB16 PNG, Non-Color | OpenGL +Y tangent normal |
| `normal_gltf` | RGB16 PNG, Non-Color | Legacy green-inverted diagnostic; not embedded by the current GLB exporter |
| `orm` | RGB16 PNG, Non-Color | R=1, G roughness, B metallic |
| `transmission` | RGB16 PNG, Non-Color | Transmission in R (replicated RGB) |
| `specular_weight` | RGBA16 PNG, Non-Color | RGB white, A=2 × source Specular IOR Level |
| `height` | RGB float32 PFM | R height × bump distance (metres), G bump strength, B raw height |
| `base_color_linear` | RGB float32 PFM | Unquantized linear verification data |

The canonical GLB embeds `normal` unchanged and pairs it with the documented glTF tangent sign conversion (`TANGENT.w = −rawUVsign`, UV0=(u,1−v)). The former opposite-sign/green-inverted pair is physically equivalent for explicit tangents, but not suitable for Blender import that regenerates tangents. See `glb-tangent-convention.md`.

Native buffers and PFM rows use a bottom-left origin. PNG files are written top-down according to PNG conventions. Optional TGA diagnostics retain bottom-left rows (`metadata.emit_tga_diagnostics=true`). Texture paths in the JSON are absolute paths to the generated files. A packaging step should make them package-relative or embed the images without replacing their generated contents. `part.material_constants` records IOR, specular, coat, roughness, metallic and transmission only when that channel is confirmed uniform over all covered samples. The 151-sample shader gate, including extreme seeds, passed with maximum absolute error 1.3113e-6. Seed offsets take modulo before subtraction to avoid INT64_MIN overflow. The standalone `src/png.cpp` writer emits 16-bit RGB or RGBA PNG using checked PNG chunks and a lossless native zlib DEFLATE stream (system ZLIB dependency; no Blender or Python runtime). Dedicated tests verify signature, dimensions, 16-bit values, row orientation, alpha and invalid input; Python zlib/CRC and Pillow independently decoded the result.

Normal generation evaluates the original Cycles **forward** height stencil in C++. Its geometric-normal basis is constructed in world space using the source `make_orthonormals` rule, then transformed back to object space to sample the native material. The physical footprint is the mean of the two metric UV texel sizes; it never depends on a camera. Source Bump Distance/Strength and interpolated shading normals are then applied and encoded in the atlas tangent basis with handedness. This replaces the earlier 10-micrometre central derivative, which made wood visibly too rough.

A separate offline oracle (`tools/material_normal_oracle.py`) evaluates the original Bump node on rotated planes and captures each actual Object coordinate through an AOV. Twelve normal samples cover wood, steel, damp wood and glass at node rotations 0, +0.57 and −1.13 radians. Native normal vector error was at most 1.18141e-6 at the same specified physical footprint. A known-camera footprint was used only as a diagnostic control, never in production baking.

Texture charts are rotated to their longest edge and their texel grid is aligned to a true interior triangle centroid. No fallback color is invented. Baking rejects any remaining uncovered face. Original metric UVs and per-face material assignments are unchanged. Whole-scene regressions removed 32 empty charts from industrial, 338 from reinforced wood and 366 from the cabinet. Across 98,972 faces, float32 conversion preserved every UV edge and orientation, introduced no chart overlap, and changed no metric UV or face material.


## Verification and boundaries

The checked fixture `fixtures/material-samples.json` contains 151 original-graph samples: multiple glass dimensions and explicit scratch points, weather seeds 29/1/37/1007 plus 2^32+1, INT64_MAX, and INT64_MIN, front/back fine wear and hardware, damp wood age/wetness/axis combinations, furniture 1956 axes and stable seeded offsets, six explicit legacy-1951 samples, the ColorRamp threshold regression, and 16 R2 cases carrying true/string FineWear metadata that the R2 source correctly ignores. Family selection in the oracle is explicit. Furniture defaults to the frozen 1956 shader; `_material_revision="1951"` is an explicit legacy mode. The 1956 source changes anisotropic abrasion, coating ramp, bump, roughness and specular; these are not silently mixed with 1951.

A separate full evaluated-glass scene run sampled 100 real object positions from the native atlas: all scalar/color channels matched original graphs within **1.2517e-6 maximum absolute error** after adding the ramp table. The approved tolerance is 3e-5; it is not a visual-image metric.

The earlier TGA-stage test coverage verified that metric UVs are unchanged, native files are emitted, float-map pixels agree with original graph values, packed parameters stay within half an 8-bit code step, and a 0.1 mm bevel strip does not disappear from the atlas. Current mirrored/rotated UV tests with tilted per-corner normals reconstruct the physical normal using the actual packed atlas basis and verify unit length within PNG16 quantization. The then-current 8-test CTest suite passed after those historical checks; that count is not the current integrated suite. A second 104-sample comparison additionally read the actual native PFM/TGA atlas pixels and compared them against the original graphs; all passed (maximum unquantized channel error 1.19209e-6). The first narrow-strip test failed with zero covered pixels before the packing correction. A 52-part evaluated-glass bake at reference resolution 256 then had zero entirely empty parts and a minimum density above 181 texels/metre; its largest atlas was 512 square.

Reproduce the original sampler oracle:

```
blender -b -t 2 --python tools/material_oracle.py -- --output fixtures/material-samples.json
build/material_tests fixtures/material-samples.json
```

Reproduce atlas-pixel comparison from a native bake report:

```
blender -b -t 2 --python tools/material_oracle.py -- --bake-report output/your-bake/report.json --output output/your-bake/oracle-samples.json
build/material_tests output/your-bake/oracle-samples.json
```

For very large scenes, select representative report probes before building the offline oracle; the original graphs are duplicated per sample/channel and can consume substantial memory.

**Still distinct from render parity:** finite atlas resolution, bilinear/mipmap filtering, 16-bit quantization (8-bit only in earlier diagnostics), bump finite-difference footprint, normal interpolation, and tessellation/bevel differences can alter a rendered image. The 0.24 mm glass scratch width is below a texel at typical preview resolutions. The move to PNG16 specifically removes the large 8-bit neutral-normal bias: 0.5 quantized to 128/255 tilted a clean surface by approximately 0.32 degrees; 16-bit neutral quantization is below 0.002 degrees. Sample parity or a successful build does not establish identical final renders. Normal/tangent image equivalence must be measured with the same camera, source geometry, shading normals, texture filtering and sampling settings; no blanket visual-parity claim is made here.

## Third-party notices

The compact native Perlin implementation adapts the equations/hash/gradient code from Blender Cycles. Material-specific graphs and mask formulas are translations of the original Jerico source snapshots. No add-on assets are used.

### Cycles noise.h: BSD-3-Clause

Copyright 2009–2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved.
Copyright 2011–2022 Blender Foundation.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of Sony Pictures Imageworks nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### Cycles hash / fractal noise / ramp: Apache-2.0

Copyright 2011–2022 Blender Foundation. Adapted for standalone C++17 arrays and reduced to the 3D fBM and ramp operations used by these assets. The original Blender source files have not been changed.

Licensed under the Apache License, Version 2.0. The complete license follows.


                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "[]"
      replaced with your own identifying information. (Don't include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

   Copyright [yyyy] [name of copyright owner]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


## Native lossless PNG recompression

`tools/recompress_native_png.cpp` rewrites only IDAT data using native C++ zlib and preserves every non-IDAT PNG chunk. It verifies exact equality of decoded scanline bytes before writing. `output/png16-lossless-recompression.json` records the test conversion with an independent Python zlib decode and SHA-256 comparison. This is optional packaging work on already-native PNG16 outputs; it is not a material generator or an 8-bit conversion.


## Controlled wood normal comparison

The temporary candidate and central-derivative baseline used identical geometry, packed UVs and native color/parameter images. Forward-texel filtering reduced full-view RGB MAE from 0.003884 to 0.001627; the header crop fell from 0.009112 to 0.002696. On an independent closer view of the same object, MAE fell from 0.006892 to 0.002823 and the header from 0.014534 to 0.003977. Source and candidate used the same lights and 64 Cycles samples. These are measured view-specific differences, not a claim of identity across all cameras.

The approved glass PNG16 comparison, before this final normal-filter promotion, measured global MAE 0.000416 and asset-bounds MAE 0.000702 against the original at 640×640, 64 samples and seed 17. Its scene/PNG/source hashes are preserved in output/visual-png16-1024/visual-evidence.json. Diagnostic PFM files are excluded from delivery packages.


## Coverage audit and final representative views

`material-coverage.md` inventories all seven generator routes and each original shader/bump/atlas/PBR/export stage. It records the R2/FineWear profile isolation regression, four final representative PNG16 material comparisons, and remaining limits. The native JSON→GLB asset path has no Python/Blender generation dependency. OBJ/MTL is now exported natively from the evaluated Scene and native PNGs, with exact copied color/normal and lossless roughness/metallic channel extraction. Its physical-transmission, clearcoat and importer-specific normal-map limits are explicit; Blender 4.3.2 ignores the `norm` extension. Immutable/ZIP/20 MB legacy export wrappers remain a disclosed capability gap, not an implicit fallback. The independent material/PNG-filter review is recorded in `material-coverage.md`.
