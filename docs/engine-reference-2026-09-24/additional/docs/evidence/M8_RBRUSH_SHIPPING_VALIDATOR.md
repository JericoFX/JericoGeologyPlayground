# M8 RBrush Slice 37 — Shipping Validator

Status: `PASS` for Slice 37 only. M8 and the JericoEngine 1.0 master Goal remain
`ACTIVE / NOT COMPLETE`.

## Demonstrated boundary

The existing `JericoAssetDump --shipping-validate` path is the canonical
package inspector. It now reports a machine-readable RBrush Shipping summary
and rejects leaked brush authoring through package paths or decoded
Level/Prefab component payloads. No second package scanner, format, catalog or
runtime RBrush reader was introduced.

The integrated proof regenerates and inspects seven real packages from the
three previously accepted Shipping verticals:

- Surface Paint `base.epak`, `delta.epak` and `compact.epak`;
- Static Sparse SDF `base.epak`, `delta.epak` and `compact.epak`;
- Runtime Dynamic Paint `runtime-paint.epak`.

Those packages contain 28 entries in total. Their own Vulkan/Jolt/VFS runtime
proofs execute before the inventory gate, so an empty UI or synthetic package
cannot satisfy this slice.

## Product contracts

- ABR v6/v10 modern headers are recognized structurally through version,
  subversion, `8BIM` section framing and bounded big-endian payload size.
- ABR, RBrush, PNG and JPEG signatures in decoded component payloads fail
  closed with a specific reason; source images are recognized both directly
  and after a four-byte length prefix.
- `.abr`, `.rbrush`, brush-source image paths, `SourceAssets`, `EditorAssets`
  and `Import/Brushes` are forbidden in Shipping logical paths.
- Textual authoring references inside decoded Level/Prefab components are
  rejected.
- Opaque runtime bytes are not treated as source text. A valid
  `RuntimePaintBrush` whose R8 coverage literally contains
  `.abr.rbrush.EditorAssets/Import/Brushes/` is accepted.
- Successful inspection reports entry count plus
  `no_abr=true|no_brush_source_images=true|no_editor_rbrush=true|no_editorassets=true`.

## Authentic RED / GREEN evidence

The valid RuntimePaintBrush collision fixture first failed because the original
validator interpreted arbitrary binary coverage as ASCII and reported
`reason=rbrush_abr_source`. Restricting reference-token inspection to textual
payloads made that positive fixture pass while preserving signature checks.

Independent review then found that the first negative mutation used the PSD
signature `8BPS`, not an ABR header. The corrected fixture embeds the real
documented framing `00 06 00 02 8BIM samp 00000000`. The unchanged validator
accepted it and printed `no_abr=true`, providing the final authentic RED. The
bounded structural detector produced GREEN.

The final pre-commit review then moved the PNG signature behind a four-byte
length prefix. The validator accepted it and again printed
`no_brush_source_images=true`; checking signatures at offsets zero and four
closed that RED without changing package generation.

Negative mutations now prove these exact results:

- real ABR component -> `reason=rbrush_abr_source`;
- length-prefixed PNG brush-source component -> `reason=rbrush_source_image`;
- Editor RBrush textual dependency -> `reason=editor_rbrush_dependency`;
- `EditorAssets` logical path -> `reason=editor_only_path`.

## Fresh integrated evidence

Debug and Release both report:

```text
rbrush.shipping_validator.pass|packages=7|entries=28|no_abr=true|no_brush_source_images=true|no_editor_rbrush=true|no_editorassets=true
```

Package SHA-256 values are byte-identical across both configurations:

| Package | SHA-256 |
|---|---|
| Surface base | `85DA435897BDC81B0DB39410536F36187071156C5674E3C9E3049C35CFE3DDD4` |
| Surface delta | `44DE2B3C7AB688E8FB4A2F45A514F9395277606AA09BCEAA9324CB7F4D06D677` |
| Surface compact | `0866F0A3B12361832BC978BF0BC11D2E6653876043F522FB2B3BFAB6CDD20C85` |
| Sparse SDF base | `319323A8E19D7235E34C8DB638836647B8D55ED363ECB96D6EA05B74AC3D7FDD` |
| Sparse SDF delta | `703BB120CE1ADBDDB8BE04AE36446FDEE06812DF3B257C84359EE1FA6A044883` |
| Sparse SDF compact | `46D0AB169ADF625C8724223BA7F7DCD42B908C48A06CBC0C2525267211D3B2D8` |
| Runtime dynamic | `2EB61EE1D07D37722741A22C7F8351B8FC31D36C04FD1F9827BB13A64DB6B2AD` |

The prerequisite product proofs also remained GREEN:

- Surface Paint: one changed page, one delta Texture, zero Levels,
  base+delta rendered, compact equals clean, capture
  `58504B6B70AC59C9D8A9287E83C7B6EBBC698E8E7E7EDF8C9C1FFFC883BA8088`.
- Static Sparse SDF: two delta entries, zero Levels, two base+delta replays,
  compact equivalence, Jolt body and Vulkan draw; Debug/Release frame values
  `3.742/3.110 ms`.
- Runtime Dynamic Paint: 63 dabs, 4,150 changed texels, coverage hash
  `1358154781762946768`, byte-identical capture
  `F192184F510A0471237455F9F32E0716F941B58929F454192FA1E937D2043D68`
  and `source_free=true`.

## Verification

- `repository.shipping_source_exclusion`: final Debug `1/1 PASS` in `5.91 s`
  and final Release `1/1 PASS` in `2.61 s`.
- `JericoRBrushShippingValidatorProof`: Debug and Release regenerated all seven
  packages and prerequisite runtime proofs. After the final length-prefix
  hardening, the inventory script was repeated directly with both final
  AssetDump binaries and returned `7/7` packages, `28` entries and PASS.
- Independent Terra review: first found the binary false positive, then the
  invalid PSD-as-ABR mutation. Both were reproduced and corrected; final
  re-review is `PASS` with no P1/P2 in scope.

## Limits and next legal action

This closes RBrush Slice 37, not M8 and not JericoEngine 1.0. It proves the
absence of brush authoring sources from the seven representative packages; it
does not complete the global renderer, Virtualized World Runtime, JSVGI,
Procedural 3D Layout, Water, Vegetation or the BSP/SDF visual showcase.

The next legal M8 action is the Render Foundation/Sponza gate: complete the
required Diligent/DiligentFX/SIGGRAPH audit and create the first retained Sponza
PBR/material/light RED on the Virtualized World/JSVGI-compatible architecture.
M9 remains blocked.
