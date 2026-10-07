# Asset size and delivery budget

These are high-fidelity authoring exports used to verify the native procedural port. They are **not optimized runtime assets** and do not meet a blanket 20 MB budget. No resolution, bit depth, mesh detail or visual content was reduced to obtain the measurements below.

## Measured lossless packaging

| Asset | GLB MB | ZIP MB | Meshes / triangles | PNG count | PNG share | Atlas edge range |
|---|---:|---:|---|---:|---:|---|
| Jerico-Cpp-Cajonera.glb | 95.36 | 82.59 | 184 / 56,168 | 1104 | 85.8% | 32–1024 |
| Jerico-Cpp-Puerta-madera.glb | 49.32 | 38.54 | 154 / 48,416 | 924 | 76.4% | 64–1024 |
| Jerico-Cpp-Puerta-metal.glb | 24.38 | 19.33 | 71 / 22,444 | 426 | 77.8% | 32–1024 |
| Jerico-Cpp-Ventana-humeda.glb | 26.57 | 23.01 | 54 / 13,524 | 324 | 86.8% | 146–1024 |
| Jerico-Cpp-Vidrio-aprobado.glb | 60.51 | 56.52 | 52 / 9,880 | 312 | 95.8% | 172–2048 |

Sizes use decimal MB. Each ZIP contains one unmodified GLB and was extracted and SHA-256 compared to the original. These archives are packaging only; they do not reduce runtime memory, texture counts or draw calls.

## Actual dominant costs

- The cabinet has 184 separate part atlases and 1,104 embedded PNG16 textures (six per material). PNG payload is 81.82 MB: color 42.10 MB, normal 23.27 MB, ORM 13.51 MB, and the remaining channels 2.94 MB. Mesh buffers plus JSON account for 13.55 MB.
- Its six texture sets contain 205.85 million pixel instances, equivalent to 34.31 million atlas texels per set. The raw PNG channel storage represents about 1.30 GB before compression, without mipmaps. This is an authoring-fidelity cost, not an engine-budget claim.
- The atlas packer retains per-face islands, guarded thin bevel strips and three-texel gutters. The reference-density setting is not a fixed maximum texture resolution: it expands atlases when necessary to keep valid islands. Per-part atlases and duplicated constant channels also add overhead.
- PNG was encoded with lossless zlib but initially always used filter None. A native three-image benchmark tested predictive filters with independent CRC/zlib/unfilter verification of every decoded 16-bit channel. Forcing the adaptive predictor increased the combined size by 2.05%; selecting the smaller of None or adaptive saved only 0.72%. This does not solve the budget, so production encoding remains unchanged. The benchmark is recorded in output/png-predictive-benchmark/report.json.
- Shared/global atlases, material pooling, channel-specific quantization, GPU texture compression and LOD are separate optimization work. They are not silently substituted for the appearance validated in this port.

Large assets are preserved in Library. The principal deliverable is the compact source archive and the matched images; raw GLBs above 20 MB are not suitable for that attachment limit. A split archive, if provided, must be fully reassembled before importing and is never described as independently importable fragments.

For the cabinet, covered face samples occupy 8063635 of 34,308,744 atlas texels (23.5%). Remaining atlas texels include required gutters and unused packing space. Blank space compresses well in PNG but still contributes to uncompressed runtime allocation.
