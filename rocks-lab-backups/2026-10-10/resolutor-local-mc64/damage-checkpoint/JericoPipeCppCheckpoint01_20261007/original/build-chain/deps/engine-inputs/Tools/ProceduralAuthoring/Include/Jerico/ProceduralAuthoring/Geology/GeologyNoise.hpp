#pragma once

#include <Jerico/ProceduralAuthoring/Geology/GeologyDirectionalWarp.hpp>

#include <cstdint>

namespace Jerico::Tools::ProceduralAuthoring
{
struct GeologyNoiseReferenceSample
{
    Vec3d noisePositionWorld;
    Vec3d sourcePositionWorld;
    double detailValue{};
    double signedDistanceMeters{};

    bool operator==(const GeologyNoiseReferenceSample&) const = default;
};

[[nodiscard]] double evaluateGeologyFbmReference(
    const Vec3d& position,
    std::uint32_t seed,
    std::uint32_t octaves,
    double lacunarity,
    double gain) noexcept;

[[nodiscard]] GeologyNoiseReferenceSample evaluateGeologyNoiseReference(
    const GeologyScalarFieldReference& source,
    const Vec3d& worldPosition,
    const GeologyNoisePayload& detail,
    double strength,
    std::uint64_t seed,
    double maskWeight = 1.0);
}
