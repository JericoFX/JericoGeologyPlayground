# Stopped shader-cache experiment, 2026-09-05

Status: WIP / NOT ACCEPTED. Baseline df090e2c is published. Owner requests an
honest feasibility answer instead of continued token spending without results;
iterations stopped. Persistent project Goal remains incomplete, not achieved.

Plan: ../superpowers/plans/2026-09-05-f4-geology-shader-cache.md.
Native Diligent IRenderStateCache was connected for the12 shaders; existing PSO
creation/readiness retained. This is compiled shader caching, not VkPipelineCache.

Owned uncommitted implementation: Tools/ProceduralAuthoring/CMakeLists.txt,
GeologyGpuSessionDiligent.cpp, new private GeologyShaderCache.hpp. Test:
Tests/ProceduralAuthoring/VerifyGeologyShaderCache.cmake, explicitly run with cmake
-P; it is not registered in CTest. Do not stage foreign procedural CMake hunks.

Logs under build/windows-vulkan-editor:
- f4-shader-cache-red01.log: expected RED, existing process never populates cache.
- f4-shader-cache-build01.log: GPU test and native preview build exit0.
- f4-shader-cache-green01/cold.log: attempted repair fails; hits0/misses12,
  serialize_failed, then serialization allocator lifetime assertion; process exit3.
  Last pipeline ready92824.7ms. No fresh-process warm improvement was demonstrated.
- f4-shader-cache-build02.log: lifetime moved into session Impl, outliving shader
  members; archive serialization/size telemetry added. GPU test rebuild exit0.
  This revision has NOT been executed. Native preview still contains the earlier
  failing revision: rebuild a verified revision before any future Generate launch.

READ_ONLY review additionally identified unguarded shader-cooking-OFF/absent-DXC
configuration, missing patched-toolchain/API compatibility discrimination, and
unregistered CTest coverage. Adapter/driver keys must be assessed specifically:
this experiment stores SPIR-V, not native driver PSO binaries. The original64MiB
bound rejected serialization for an as-yet unmeasured reason/size; do not claim
the reason without the new telemetry. No cap was raised to pass a test.

All owned builds/tests are terminal; no native preview remains open. Existing189
preservation entries were verified byte-exact as a status inventory after df090e2c;
the new owned WIP is additional. No cache, runtime, geometry or Beauty PASS.
