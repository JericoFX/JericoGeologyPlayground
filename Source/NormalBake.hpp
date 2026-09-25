#pragma once
#include "Model.hpp"
#include <optional>
#include <string>
#include <vector>
namespace Playground
{
enum class NormalProjection
{
    NormalRay,
    ClosestSurface
};
struct NormalBakeChart
{
    unsigned x{}, y{}, size{};
};
struct SpecialistMipChains
{
    std::vector<std::vector<std::uint8_t>> ambient, contact, curvature, cavity, edge, wire;
    std::vector<std::vector<std::uint8_t>> thickness, region, height;
};
struct NormalBakeSettings
{
    unsigned resolution{1024};
    double projectionDistance{0.15};
    unsigned padding{4};
    bool matchRegions{true};
    NormalProjection projection{NormalProjection::NormalRay};
    bool specialistMaps{false};
    double featureRadius{1};
    std::optional<double> contactRadius, ambientRadius;
    double cavityRadius{0.05};
    double edgeRadius{0.02};
    double wireRadius{0.1};
    unsigned featureSamples{16};
    double curvatureScale{0.25};
    double curvatureRelaxation{0};
    double thicknessRange{2};
    double heightRange{0.25};
    std::vector<NormalBakeChart> chartLayout;
    bool automaticUv{false};
    unsigned automaticUvMipCount{4};
    bool normalMomentMips{false};
};
struct NormalBake
{
    std::string lowStrategy{"provided-mesh"};
    MeshResult low;
    std::vector<std::array<float, 2>> cornerUV;
    std::vector<std::uint32_t> uvTriangleIsland;
    unsigned automaticUvIslandCount{}, automaticUvMarginPixels{};
    std::size_t automaticUvSubpixelTriangles{};
    double automaticUvActiveAreaFraction{};
    std::vector<Vec3d> cornerTangents, cornerBitangents;
    std::vector<std::uint8_t> rgba;
    std::vector<std::vector<std::uint8_t>> normalMips;
    std::vector<std::vector<std::uint8_t>> normalMomentMips;
    std::vector<std::uint8_t> ambientRgba, contactRgba, curvatureRgba, cavityRgba;
    std::vector<std::uint8_t> edgeRgba, wireRgba, thicknessRgba, regionRgba, heightRgba;
    SpecialistMipChains specialistMips;
    unsigned resolution{};
    NormalBakeSettings settings;
    std::size_t samples{}, misses{}, backfaceHits{};
    std::size_t missesOtherRegionFront{}, missesNearbyBackface{}, missesBeyondRadius{};
    std::size_t rejectedBackfaceCandidates{};
    double maximumDistance{}, meanDistance{}, meanEncodedAngleDegrees{}, milliseconds{};
    double minimumHeight{}, maximumHeight{};
    std::size_t clippedHeightSamples{};
    std::uint64_t highHash{}, lowHash{};
};
NormalBake bakeNormals(const MeshResult &high, const MeshResult &low, const NormalBakeSettings &settings = {},
                       const Jerico::CancellationToken *cancel = nullptr);
std::vector<std::vector<std::uint8_t>> buildNormalMomentMips(
    const std::vector<std::uint8_t> &rgba, unsigned resolution, unsigned mipCount);
void exportNormalBake(const NormalBake &bake, const std::filesystem::path &directory);
} // namespace Playground
