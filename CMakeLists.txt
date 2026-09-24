cmake_minimum_required(VERSION 3.25)
project(JericoGeologyPlayground LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(JERICO_SOURCE "F:/JericoEngine" CACHE PATH "Read-only Jerico engine checkout")
set(JERICO_DEPS "${JERICO_SOURCE}/build/windows-vulkan-editor" CACHE PATH "Read-only engine dependency build")
if(CMAKE_SOURCE_DIR MATCHES "^${JERICO_SOURCE}(/|$)" OR CMAKE_BINARY_DIR MATCHES "^${JERICO_SOURCE}(/|$)")
    message(FATAL_ERROR "Playground source and build must be outside the engine checkout")
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
add_library(PlaygroundModel Source/Model.cpp Source/SolidGeometry.cpp Source/LayerFormation.cpp Source/SurfaceGeometry.cpp Source/MeshReduction.cpp Source/SurfaceFeatures.cpp Source/NormalBake.cpp Source/AutoUv.cpp Source/RockAddonMath.cpp Source/RBrushInterop.cpp
    "${JERICO_SOURCE}/Tools/ProceduralAuthoring/Source/Geology/GeologyNoise.cpp"
    "${JERICO_SOURCE}/Tools/ProceduralAuthoring/Source/MaskSignedDistanceField2D.cpp"
    "${JERICO_SOURCE}/Tools/ProceduralAuthoring/Source/SolidEvaluatorManifold.cpp"
    "${JERICO_SOURCE}/Tools/ProceduralAuthoring/Source/BspDocument.cpp"
    "${JERICO_SOURCE}/Engine/Core/Source/Guid.cpp")
target_include_directories(PlaygroundModel PUBLIC Source
    "${JERICO_SOURCE}/Engine/Core/Include"
    "${JERICO_SOURCE}/Engine/Jobs/Include"
    "${JERICO_SOURCE}/Tools/ProceduralAuthoring/Include"
    "${JERICO_SOURCE}/Tools/AssetPipeline/Include"
    "${JERICO_SOURCE}/Engine/VirtualizedWorld/Include"
    "${JERICO_DEPS}/DiligentTools/ThirdParty/json/single_include")
target_include_directories(PlaygroundModel PRIVATE
    "${JERICO_DEPS}/DiligentCore/ThirdParty/stb"
    "${JERICO_SOURCE}/Tools/ProceduralAuthoring/Source"
    "${JERICO_DEPS}/_deps/manifold-src/include"
    "${JERICO_DEPS}/_deps/manifold-build/include")
target_link_libraries(PlaygroundModel PRIVATE "${JERICO_DEPS}/_deps/manifold-build/lib/Release/manifold.lib"
    "${JERICO_DEPS}/Tools/AssetPipeline/$<CONFIG>/JericoAssetPipeline.lib"
    "${JERICO_DEPS}/Tools/ProceduralAuthoring/$<CONFIG>/JericoProceduralAuthoring.lib"
    "${JERICO_DEPS}/Engine/VirtualizedWorld/$<CONFIG>/JericoVirtualizedWorldContracts.lib"
    "${JERICO_DEPS}/Engine/Core/$<CONFIG>/JericoCore.lib")
target_compile_definitions(PlaygroundModel PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_definitions(PlaygroundModel PRIVATE MANIFOLD_PAR=-1)
enable_testing()
add_executable(PlaygroundNormalBakeTests Tests/NormalBakeTests.cpp)
target_link_libraries(PlaygroundNormalBakeTests PRIVATE PlaygroundModel)
add_test(NAME Playground.NormalBake COMMAND PlaygroundNormalBakeTests)
add_executable(PlaygroundAutoUvTests Tests/AutoUvTests.cpp)
target_link_libraries(PlaygroundAutoUvTests PRIVATE PlaygroundModel)
add_test(NAME Playground.AutoUv COMMAND PlaygroundAutoUvTests)
add_executable(PlaygroundRockAddonMathTests Tests/RockAddonMathTests.cpp)
target_link_libraries(PlaygroundRockAddonMathTests PRIVATE PlaygroundModel)
add_test(NAME Playground.RockAddonMath COMMAND PlaygroundRockAddonMathTests)
add_executable(PlaygroundRBrushInteropTests Tests/RBrushInteropTests.cpp)
target_link_libraries(PlaygroundRBrushInteropTests PRIVATE PlaygroundModel)
add_test(NAME Playground.RBrushInterop COMMAND PlaygroundRBrushInteropTests)
add_executable(PlaygroundRBrushImageRoundtripTests Tests/RBrushImageRoundtripTests.cpp)
target_link_libraries(PlaygroundRBrushImageRoundtripTests PRIVATE PlaygroundModel)
target_link_libraries(PlaygroundRBrushImageRoundtripTests PRIVATE
    "${JERICO_DEPS}/ThirdParty/$<CONFIG>/JericoLodePng.lib"
    "${JERICO_DEPS}/ThirdParty/$<CONFIG>/JericoTinyExr.lib"
    "${JERICO_DEPS}/_deps/libtiff-build/libtiff/$<CONFIG>/$<IF:$<CONFIG:Debug>,tiffd,tiff>.lib"
    "${JERICO_DEPS}/_deps/zstd-build/lib/$<CONFIG>/zstd_static.lib")
add_test(NAME Playground.RBrushImageRoundtrip COMMAND PlaygroundRBrushImageRoundtripTests
    "${CMAKE_SOURCE_DIR}/Evidence/cc0-references/cliff_side-disp-1k.png")
add_executable(PlaygroundSurfaceFeatureTests Tests/SurfaceFeatureTests.cpp)
target_link_libraries(PlaygroundSurfaceFeatureTests PRIVATE PlaygroundModel)
add_test(NAME Playground.SurfaceFeatures COMMAND PlaygroundSurfaceFeatureTests)
add_executable(PlaygroundTests Tests/ModelTests.cpp)
target_link_libraries(PlaygroundTests PRIVATE PlaygroundModel)
add_test(NAME Playground.Model COMMAND PlaygroundTests)
add_executable(PlaygroundSolidTests Tests/SolidGeometryTests.cpp)
target_link_libraries(PlaygroundSolidTests PRIVATE PlaygroundModel)
add_test(NAME Playground.NativeSolid COMMAND PlaygroundSolidTests)
add_executable(PlaygroundSharedWarpTests Tests/SharedWarpTests.cpp)
target_link_libraries(PlaygroundSharedWarpTests PRIVATE PlaygroundModel)
add_test(NAME Playground.SharedWarp COMMAND PlaygroundSharedWarpTests)
add_executable(PlaygroundThicknessTests Tests/ThicknessTests.cpp)
target_link_libraries(PlaygroundThicknessTests PRIVATE PlaygroundModel)
add_test(NAME Playground.Thickness COMMAND PlaygroundThicknessTests)
add_executable(PlaygroundDriftTests Tests/DriftTests.cpp)
target_link_libraries(PlaygroundDriftTests PRIVATE PlaygroundModel)
add_test(NAME Playground.Drift COMMAND PlaygroundDriftTests)
add_executable(PlaygroundEnvelopeTests Tests/EnvelopeTests.cpp)
target_link_libraries(PlaygroundEnvelopeTests PRIVATE PlaygroundModel)
add_test(NAME Playground.Envelope COMMAND PlaygroundEnvelopeTests)
add_executable(PlaygroundEdgeTests Tests/EdgeTests.cpp)
target_link_libraries(PlaygroundEdgeTests PRIVATE PlaygroundModel)
add_test(NAME Playground.Edge COMMAND PlaygroundEdgeTests)
add_executable(PlaygroundBackendTests Tests/BackendTests.cpp)
target_link_libraries(PlaygroundBackendTests PRIVATE PlaygroundModel)
add_test(NAME Playground.Backend COMMAND PlaygroundBackendTests "${CMAKE_SOURCE_DIR}/Evidence/baseline-v1")
add_executable(PlaygroundSurfaceTests Tests/SurfaceTests.cpp)
target_link_libraries(PlaygroundSurfaceTests PRIVATE PlaygroundModel)
add_test(NAME Playground.Surface COMMAND PlaygroundSurfaceTests)
add_executable(PlaygroundBspSourceTests Tests/BspSourceTests.cpp)
target_link_libraries(PlaygroundBspSourceTests PRIVATE PlaygroundModel)
add_test(NAME Playground.BspSource COMMAND PlaygroundBspSourceTests)
add_executable(PlaygroundReductionTests Tests/ReductionTests.cpp)
target_link_libraries(PlaygroundReductionTests PRIVATE PlaygroundModel)
add_test(NAME Playground.Reduction COMMAND PlaygroundReductionTests)

set(DC "${JERICO_DEPS}/DiligentCore")
set(DT "${JERICO_DEPS}/DiligentTools")
set(DB "${JERICO_DEPS}/_deps/diligentcore-build")
add_executable(JericoGeologyPlayground Source/Main.cpp Source/Renderer.cpp
    "${JERICO_SOURCE}/Engine/Platform/Source/WindowWin32.cpp"
    "${DT}/ThirdParty/imgui/backends/imgui_impl_win32.cpp")
target_include_directories(JericoGeologyPlayground PRIVATE
    "${JERICO_SOURCE}/Engine/Platform/Include"
    "${DC}/Primitives/interface" "${DC}/Platforms/interface" "${DC}/Platforms/Win32/interface"
    "${DC}/Common/interface" "${DC}/Graphics/GraphicsEngine/interface"
    "${DC}/Graphics/GraphicsEngineVulkan/interface" "${DC}/Graphics/GraphicsAccessories/interface"
    "${DC}/Graphics/GraphicsTools/interface" "${DC}/ThirdParty/stb"
    "${DT}/Imgui/interface" "${DT}/ThirdParty/imgui")
target_compile_definitions(JericoGeologyPlayground PRIVATE
    PLAYGROUND_ROOT="${CMAKE_SOURCE_DIR}"
    PLATFORM_WIN32=1 DILIGENT_VK_SHARED=1
    IMGUI_USER_CONFIG="ImGuiDiligentConfig.h" IMGUI_DEFINE_MATH_OPERATORS)
target_link_libraries(JericoGeologyPlayground PRIVATE PlaygroundModel
    "${JERICO_DEPS}/ThirdParty/RelWithDebInfo/JericoDiligentImgui.lib"
    "${DB}/Graphics/GraphicsTools/RelWithDebInfo/Diligent-GraphicsTools.lib"
    "${DB}/Graphics/GraphicsAccessories/RelWithDebInfo/Diligent-GraphicsAccessories.lib"
    "${DB}/Common/RelWithDebInfo/Diligent-Common.lib"
    "${DB}/Primitives/RelWithDebInfo/Diligent-Primitives.lib"
    "${DB}/Platforms/Win32/RelWithDebInfo/Diligent-Win32Platform.lib"
    "${DB}/Platforms/Basic/RelWithDebInfo/Diligent-BasicPlatform.lib"
    user32 shell32 ole32 comdlg32 dwmapi imm32 d3dcompiler)
add_custom_command(TARGET JericoGeologyPlayground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${JERICO_DEPS}/Tools/GeologyPreview/Release/GraphicsEngineVk_64r.dll"
        "${JERICO_DEPS}/Tools/GeologyPreview/Release/spv_dxcompiler.dll"
        "$<TARGET_FILE_DIR:JericoGeologyPlayground>")
