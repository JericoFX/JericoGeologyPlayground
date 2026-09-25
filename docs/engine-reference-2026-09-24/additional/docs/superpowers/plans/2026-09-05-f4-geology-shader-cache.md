# F4 repeated shader startup repair

User objective: restarting the engine must not repeat minute-long geology DXC
compilation. Existing native session readiness is90.4418s. This bounded startup
repair belongs to the existing F4, with final UI still M10.5.

1. RED: fresh-process cold/warm gate using the existing classified LOD cook proof;
   require all12 shader hits on warm use and byte-exact mesh/collision.
2. Reuse Diligent IRenderStateCache compiled archives, not a new shader compiler
   or format. Keep existing compute pipeline creation/readiness and session
   lifetime. Source/create-info hashing remains Diligent's responsibility.
3. Persist only after all pipelines are ready, with bounded reads, integrity
   checking and atomic replacement. Cache misses/corruption/unwritable storage
   remain visible diagnostics; generation must still work. Use native app-data
   location by default and an explicit test cache directory override.
4. Include configured Diligent/compiler identity and build mode in compatibility;
   verify source change, corrupt/truncated data and warm fresh-process replay.
5. Measure warm native Generate, including responsive cancel/restore and actual
   weathering output. Preserve recipe, fields, geometry and semantic lineage.

Audit: this Tool-only accelerator touches session initialization, disk cache,
profiling and tests. It must not change source assets, authored data, evaluate,
cook, collision, runtime assets, LOD gates, shaders' equations, or Shipping.
Prepared tool-distribution archives and cross-hardware startup budgets remain
INTEGRATION REQUIRED until separately demonstrated. A shader archive avoids DXC;
driver pipeline creation may still take time and must be measured honestly.
