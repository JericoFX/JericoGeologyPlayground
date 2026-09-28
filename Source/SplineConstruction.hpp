#pragma once

#include "Model.hpp"
#include "DamageField.hpp"
#include <Jerico/ProceduralAuthoring/ProceduralSpline.hpp>

#include <cstddef>
#include <array>
#include <string>
#include <vector>

namespace Playground::Construction
{
struct PathQuality
{
    double maximumDeviationMeters{0.01};
    double maximumTangentAngleRadians{0.18};
    double maximumStepMeters{0.25};
    std::size_t maximumSamples{4096};
};
struct PathFrame
{
    Vec3d position{}, tangent{}, normal{}, binormal{};
    double distanceMeters{}, widthMeters{1}, rollRadians{};
    std::size_t segment{};
};
struct PathResult
{
    std::vector<PathFrame> frames;
    double lengthMeters{}, maximumMeasuredDeviationMeters{};
};
struct RepeatSettings
{
    double spacingMeters{1}, offsetMeters{}, startPaddingMeters{}, endPaddingMeters{};
    std::size_t maximumParts{2048};
};
struct RepeatPlacement
{
    PathFrame frame;
};
enum class ProfileKind { Circle, Ring, Rectangle, RoundedRectangle, Channel, IBeam };
enum class UvMode { WorldMetric, Stretch01, RepeatCount };
struct PathDamageRule
{
    std::string id;
    DamagePathCutter cutter;
};
struct ConstructionSettings
{
    PathQuality pathQuality;
    ProfileKind profile{ProfileKind::Circle};
    UvMode uvMode{UvMode::WorldMetric};
    double radiusMeters{0.04}, widthMeters{0.12}, heightMeters{0.12};
    double wallThicknessMeters{0.008}, cornerRadiusMeters{0.012};
    double seamAngleRadians{}, profileRotationRadians{}, profileScale{1};
    double profileChordErrorMeters{0.002}, profileMaximumAngleRadians{0.25};
    double metresPerTileU{1}, metresPerTileV{0.25}, repeatCount{4};
    double taperLengthMeters{}, tipScale{1};
    double ribAmplitude{}, ribSpacingMeters{0.2}, twistTurns{};
    bool breakEnabled{};
    double breakFraction{0.5}, breakGapMeters{0.15}, breakRoughness{0.3};
    std::uint32_t breakSeed{123};
    bool capStart{true}, capEnd{true}, repeatEnabled{};
    RepeatSettings repeat;
    std::vector<PathDamageRule> damagePaths;
    std::size_t maximumVertices{100000};
};
struct ConstructionResult
{
    PathResult path;
    MeshResult mesh;
    std::vector<std::array<float, 2>> vertexUV, cornerUV;
    std::vector<std::array<float, 2>> bakeCornerUV;
    std::vector<RepeatPlacement> repeats;
    unsigned bakeIslandCount{};
    std::size_t profileSegments{}, bodyVertexCount{}, bodyTriangleCount{};
    double generationCpuMilliseconds{};
};

PathResult samplePath(const PA::ProceduralSpline &spline, const PathQuality &quality);
PathFrame frameAtDistance(const PathResult &path, double distanceMeters);
std::vector<RepeatPlacement> repeatAlongPath(const PathResult &path, const RepeatSettings &settings);
ConstructionResult buildConstruction(const PA::ProceduralSpline &spline,
                                     const ConstructionSettings &settings);
ConstructionResult withFrameAxes(const ConstructionResult &construction);
}
