#include <Jerico/ProceduralAuthoring/Geology/GeologyNoise.hpp>

#include "GeologyValueNoiseReference.hpp"

#include <algorithm>
#include <cmath>

namespace Jerico::Tools::ProceduralAuthoring
{
namespace
{
Vec3d multiply(const Vec3d& first, const Vec3d& second) noexcept
{
    return {
        first.x * second.x,
        first.y * second.y,
        first.z * second.z};
}

Vec3d boundedVector(Vec3d value) noexcept
{
    const auto length = std::sqrt(
        value.x * value.x + value.y * value.y + value.z * value.z);
    if (length > 1.0)
    {
        value.x /= length;
        value.y /= length;
        value.z /= length;
    }
    return value;
}

std::uint32_t foldSeed(const std::uint64_t seed) noexcept
{
    return static_cast<std::uint32_t>(seed)
        ^ static_cast<std::uint32_t>(seed >> 32U);
}

double configuredNoise(
    const Vec3d& position,
    const std::uint32_t seed,
    const GeologyNoisePayload& detail) noexcept
{
    switch (detail.fractal)
    {
    case GeologyFractalKind::None:
        return Detail::geologyValueNoise(position, seed);
    case GeologyFractalKind::Fbm:
        return Detail::configuredGeologyFbm(
            position,
            seed,
            detail.octaves,
            detail.lacunarity,
            detail.gain);
    case GeologyFractalKind::Ridged:
        return Detail::configuredGeologyRidged(
            position,
            seed,
            detail.octaves,
            detail.lacunarity,
            detail.gain);
    }
    return 0.0;
}

double remapNoise(
    const double value,
    const GeologyLinearRemap& remap) noexcept
{
    const auto normalized = std::clamp(
        (value - remap.inputMinimum)
            / (remap.inputMaximum - remap.inputMinimum),
        0.0,
        1.0);
    return remap.outputMinimum
        + (remap.outputMaximum - remap.outputMinimum) * normalized;
}

Vec3d domainWarpedPosition(
    const Vec3d& worldPosition,
    const GeologyNoisePayload& detail,
    const std::uint32_t seed) noexcept
{
    if (detail.domainWarpAmplitudeMeters == 0.0) return worldPosition;
    const auto coordinate = multiply(
        worldPosition, detail.domainWarpFrequencyPerMeter);
    auto warp = boundedVector({
        Detail::configuredGeologyFbm(
            coordinate,
            seed ^ 0x68bc21ebU,
            detail.octaves,
            detail.lacunarity,
            detail.gain),
        Detail::configuredGeologyFbm(
            coordinate,
            seed ^ 0x02e5be93U,
            detail.octaves,
            detail.lacunarity,
            detail.gain),
        Detail::configuredGeologyFbm(
            coordinate,
            seed ^ 0x967a889bU,
            detail.octaves,
            detail.lacunarity,
            detail.gain)});
    warp.x *= detail.domainWarpAmplitudeMeters;
    warp.y *= detail.domainWarpAmplitudeMeters;
    warp.z *= detail.domainWarpAmplitudeMeters;
    return {
        worldPosition.x + warp.x,
        worldPosition.y + warp.y,
        worldPosition.z + warp.z};
}
}

double evaluateGeologyFbmReference(
    const Vec3d& position,
    const std::uint32_t seed,
    const std::uint32_t octaves,
    const double lacunarity,
    const double gain) noexcept
{
    return Detail::configuredGeologyFbm(
        position, seed, octaves, lacunarity, gain);
}

GeologyNoiseReferenceSample evaluateGeologyNoiseReference(
    const GeologyScalarFieldReference& source,
    const Vec3d& worldPosition,
    const GeologyNoisePayload& detail,
    const double strength,
    const std::uint64_t seed,
    const double maskWeight)
{
    const auto foldedSeed = foldSeed(seed);
    const auto noisePosition = domainWarpedPosition(
        worldPosition, detail, foldedSeed);
    const auto noiseCoordinate = multiply(
        noisePosition, detail.frequencyPerMeter);
    const auto detailValue = remapNoise(
        configuredNoise(noiseCoordinate, foldedSeed, detail), detail.remap);
    const auto inputDistance = source(worldPosition);
    const auto surfaceWeight = std::clamp(
        1.0 - std::abs(inputDistance) / detail.surfaceBandMeters,
        0.0,
        1.0);
    const auto weight = std::clamp(strength, 0.0, 1.0)
        * std::clamp(maskWeight, 0.0, 1.0)
        * surfaceWeight;
    if (weight == 0.0 || detail.amplitudeMeters == 0.0)
    {
        return {
            .noisePositionWorld = noisePosition,
            .sourcePositionWorld = worldPosition,
            .detailValue = detailValue,
            .signedDistanceMeters = inputDistance};
    }

    if (detail.displacement == GeologyNoiseDisplacement::SurfaceNormal)
    {
        const auto displacement = std::clamp(
            detailValue * detail.amplitudeMeters * weight,
            -detail.clampMeters,
            detail.clampMeters);
        return {
            .noisePositionWorld = noisePosition,
            .sourcePositionWorld = worldPosition,
            .detailValue = detailValue,
            .signedDistanceMeters = inputDistance - displacement};
    }

    auto direction = boundedVector({
        detailValue,
        remapNoise(
            configuredNoise(
                noiseCoordinate, foldedSeed ^ 0x9e3779b9U, detail),
            detail.remap),
        remapNoise(
            configuredNoise(
                noiseCoordinate, foldedSeed ^ 0x243f6a88U, detail),
            detail.remap)});
    direction.x *= detail.amplitudeMeters * weight;
    direction.y *= detail.amplitudeMeters * weight;
    direction.z *= detail.amplitudeMeters * weight;
    const Vec3d sourcePosition{
        worldPosition.x + direction.x,
        worldPosition.y + direction.y,
        worldPosition.z + direction.z};
    const auto sourceDistance = source(sourcePosition);
    const auto boundedDifference = std::clamp(
        sourceDistance - inputDistance,
        -detail.clampMeters,
        detail.clampMeters);
    return {
        .noisePositionWorld = noisePosition,
        .sourcePositionWorld = sourcePosition,
        .detailValue = detailValue,
        .signedDistanceMeters = inputDistance + boundedDifference};
}
}
