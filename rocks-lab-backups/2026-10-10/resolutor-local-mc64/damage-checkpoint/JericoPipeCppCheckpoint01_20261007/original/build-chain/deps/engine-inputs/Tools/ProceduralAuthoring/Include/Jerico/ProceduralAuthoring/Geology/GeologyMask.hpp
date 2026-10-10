#pragma once

#include <Jerico/Core/Guid.hpp>
#include <Jerico/Core/Result.hpp>
#include <Jerico/ProceduralAuthoring/Geology/GeologyTypes.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace Jerico::Tools::ProceduralAuthoring
{
struct GeologyMaskRef
{
    Guid maskGuid;
    bool inverted{};

    bool operator==(const GeologyMaskRef&) const = default;
};

enum class GeologyMaskSpace : std::uint8_t
{
    World = 1U,
    Local = 2U
};

enum class GeologyMaskNodeKind : std::uint8_t
{
    Constant = 1U,
    Bounds = 2U,
    HeightRange = 3U,
    NormalFacing = 4U,
    Slope = 5U,
    SurfaceProximity = 6U,
    ProceduralNoise = 7U,
    BspRegion = 8U,
    Multiply = 9U,
    Min = 10U,
    Max = 11U,
    Subtract = 12U,
    Invert = 13U,
    SmoothFalloff = 14U,
    Remap = 15U,
    RBrushLocalField = 16U
};

struct GeologyConstantMaskSource
{
    double value{1.0};

    bool operator==(const GeologyConstantMaskSource&) const = default;
};

struct GeologyBoundsMaskSource
{
    GeologyMaskSpace space{GeologyMaskSpace::World};
    Vec3d minimum;
    Vec3d maximum;
    GeologyTransform transform;
    double falloffMeters{};

    bool operator==(const GeologyBoundsMaskSource&) const = default;
};

struct GeologyHeightRangeMaskSource
{
    GeologyMaskSpace space{GeologyMaskSpace::World};
    double minimumHeight{};
    double maximumHeight{1.0};
    GeologyTransform transform;
    double falloffMeters{};

    bool operator==(const GeologyHeightRangeMaskSource&) const = default;
};

struct GeologyNormalFacingMaskSource
{
    Vec3d directionWorld{0.0, 0.0, 1.0};
    double minimumDot{};
    double maximumDot{1.0};

    bool operator==(const GeologyNormalFacingMaskSource&) const = default;
};

struct GeologySlopeMaskSource
{
    Vec3d upWorld{0.0, 1.0, 0.0};
    double minimumDegrees{};
    double maximumDegrees{90.0};

    bool operator==(const GeologySlopeMaskSource&) const = default;
};

struct GeologySurfaceProximityMaskSource
{
    double innerDistanceMeters{};
    double outerDistanceMeters{1.0};

    bool operator==(const GeologySurfaceProximityMaskSource&) const = default;
};

struct GeologyProceduralNoiseMaskSource
{
    GeologyMaskSpace space{GeologyMaskSpace::World};
    GeologyTransform transform;
    GeologyNoiseKind noise{GeologyNoiseKind::OpenSimplex2};
    GeologyFractalKind fractal{GeologyFractalKind::None};
    Vec3d frequencyPerMeter{1.0, 1.0, 1.0};
    std::uint32_t octaves{1U};
    double lacunarity{2.0};
    double gain{0.5};
    std::uint64_t seed{};
    GeologyLinearRemap remap{-1.0, 1.0, 0.0, 1.0};

    bool operator==(const GeologyProceduralNoiseMaskSource&) const = default;
};

struct GeologyBspRegionMaskSource
{
    GeologySourceKind kind{GeologySourceKind::BspGroup};
    Guid documentGuid;
    Guid sourceGuid;
    std::uint64_t sourceRevision{};
    std::uint64_t sourceContentHash{};
    double falloffMeters{};

    bool operator==(const GeologyBspRegionMaskSource&) const = default;
};

struct GeologyRBrushLocalFieldMaskSource
{
    Guid assetGuid;
    Guid presetGuid;
    Guid tipGuid;
    std::array<std::byte, 32U> canonicalTipHash{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<std::byte> coverageR8;
    GeologyTransform transform;
    double widthMeters{1.0};
    double heightMeters{1.0};
    double projectionDepthMeters{1.0};
    double falloffMeters{};

    bool operator==(const GeologyRBrushLocalFieldMaskSource&) const = default;
};

struct GeologyMaskNoParameters
{
    bool operator==(const GeologyMaskNoParameters&) const = default;
};

struct GeologySmoothFalloffMaskOperation
{
    double edgeMinimum{};
    double edgeMaximum{1.0};

    bool operator==(const GeologySmoothFalloffMaskOperation&) const = default;
};

struct GeologyRemapMaskOperation
{
    GeologyLinearRemap mapping{0.0, 1.0, 0.0, 1.0};

    bool operator==(const GeologyRemapMaskOperation&) const = default;
};

using GeologyMaskNodePayload = std::variant<
    GeologyConstantMaskSource,
    GeologyBoundsMaskSource,
    GeologyHeightRangeMaskSource,
    GeologyNormalFacingMaskSource,
    GeologySlopeMaskSource,
    GeologySurfaceProximityMaskSource,
    GeologyProceduralNoiseMaskSource,
    GeologyBspRegionMaskSource,
    GeologyRBrushLocalFieldMaskSource,
    GeologyMaskNoParameters,
    GeologySmoothFalloffMaskOperation,
    GeologyRemapMaskOperation>;

struct GeologyMaskNode
{
    GeologyMaskNodeKind kind{GeologyMaskNodeKind::Constant};
    GeologyMaskNodePayload payload{GeologyConstantMaskSource{}};

    bool operator==(const GeologyMaskNode&) const = default;
};

struct GeologyMask
{
    Guid id;
    std::string name;
    std::vector<GeologyMaskNode> nodes;

    bool operator==(const GeologyMask&) const = default;
};

[[nodiscard]] Result<void> validateGeologyMask(const GeologyMask& mask);
}
