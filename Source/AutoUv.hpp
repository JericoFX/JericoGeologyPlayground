#pragma once
#include "Model.hpp"

namespace Playground
{
struct AutoUvSettings
{
    unsigned resolution{2048};
    unsigned mipCount{4};
    unsigned bakeDilationPixels{4};
    unsigned minimumMipSafetyPixels{1};
    unsigned compressionBlockSafetyPixels{4};
    double seamAngleDegrees{35};
};
struct AutoUvPlan
{
    std::vector<std::array<float, 2>> cornerUv;
    std::vector<std::uint32_t> triangleIsland;
    unsigned islandCount{};
    unsigned marginPixels{};
    std::size_t subpixelTriangles{};
    double pixelsPerMeter{};
    double activeAreaFraction{};
};
unsigned requiredUvMarginPixels(const AutoUvSettings &settings);
AutoUvPlan planAutomaticUv(const TriangleMesh &mesh, const AutoUvSettings &settings = {},
                           const Jerico::CancellationToken *cancel = nullptr);
} // namespace Playground
