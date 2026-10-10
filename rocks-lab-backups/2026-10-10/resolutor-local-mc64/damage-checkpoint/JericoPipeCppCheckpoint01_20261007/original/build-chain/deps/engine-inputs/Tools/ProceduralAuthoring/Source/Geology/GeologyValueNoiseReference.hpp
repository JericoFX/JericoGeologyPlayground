#pragma once

#include <Jerico/ProceduralAuthoring/Geometry.hpp>

#include <array>
#include <cmath>
#include <cstdint>

namespace Jerico::Tools::ProceduralAuthoring::Detail
{
inline std::uint32_t mixGeologyValueNoiseHash(std::uint32_t value) noexcept
{
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    value *= 0x846ca68bU;
    value ^= value >> 16U;
    return value;
}

inline double geologyLatticeValue(
    const std::array<std::int32_t, 3U>& coordinate,
    const std::uint32_t seed) noexcept
{
    auto value = seed
        ^ static_cast<std::uint32_t>(coordinate[0U]) * 0x8da6b343U;
    value ^= static_cast<std::uint32_t>(coordinate[1U]) * 0xd8163841U;
    value ^= static_cast<std::uint32_t>(coordinate[2U]) * 0xcb1ab31fU;
    return static_cast<double>(mixGeologyValueNoiseHash(value) & 0x00ffffffU)
        * (2.0 / 16777215.0) - 1.0;
}

inline double geologyValueNoise(
    const Vec3d& position,
    const std::uint32_t seed) noexcept
{
    const std::array<std::int32_t, 3U> base{
        static_cast<std::int32_t>(std::floor(position.x)),
        static_cast<std::int32_t>(std::floor(position.y)),
        static_cast<std::int32_t>(std::floor(position.z))};
    std::array<double, 3U> weight{
        position.x - std::floor(position.x),
        position.y - std::floor(position.y),
        position.z - std::floor(position.z)};
    for (auto& value : weight)
        value = value * value * (3.0 - 2.0 * value);
    const auto sample = [&] (
        const std::int32_t x,
        const std::int32_t y,
        const std::int32_t z) {
        return geologyLatticeValue(
            {base[0U] + x, base[1U] + y, base[2U] + z}, seed);
    };
    const auto interpolate = [] (
        const double first,
        const double second,
        const double amount) {
        return first + (second - first) * amount;
    };
    const auto x00 = interpolate(sample(0, 0, 0), sample(1, 0, 0), weight[0U]);
    const auto x10 = interpolate(sample(0, 1, 0), sample(1, 1, 0), weight[0U]);
    const auto x01 = interpolate(sample(0, 0, 1), sample(1, 0, 1), weight[0U]);
    const auto x11 = interpolate(sample(0, 1, 1), sample(1, 1, 1), weight[0U]);
    return interpolate(
        interpolate(x00, x10, weight[1U]),
        interpolate(x01, x11, weight[1U]),
        weight[2U]);
}

inline double configuredGeologyFbm(
    Vec3d position,
    const std::uint32_t seed,
    const std::uint32_t octaveCount,
    const double lacunarity,
    const double gain) noexcept
{
    double value{};
    double amplitude{1.0};
    double amplitudeSum{};
    for (std::uint32_t octave{}; octave < octaveCount; ++octave)
    {
        value += geologyValueNoise(
            position, seed + octave * 0x9e3779b9U) * amplitude;
        amplitudeSum += amplitude;
        position = {
            position.x * lacunarity,
            position.y * lacunarity,
            position.z * lacunarity};
        amplitude *= gain;
    }
    return amplitudeSum > 0.0 ? value / amplitudeSum : 0.0;
}

inline double configuredGeologyRidged(
    Vec3d position,
    const std::uint32_t seed,
    const std::uint32_t octaveCount,
    const double lacunarity,
    const double gain) noexcept
{
    double value{};
    double amplitude{1.0};
    double amplitudeSum{};
    for (std::uint32_t octave{}; octave < octaveCount; ++octave)
    {
        auto ridge = 1.0 - std::abs(geologyValueNoise(
            position, seed + octave * 0x9e3779b9U));
        ridge *= ridge;
        value += ridge * amplitude;
        amplitudeSum += amplitude;
        position = {
            position.x * lacunarity + 11.7,
            position.y * lacunarity - 7.3,
            position.z * lacunarity + 5.1};
        amplitude *= gain;
    }
    return amplitudeSum > 0.0 ? value / amplitudeSum * 2.0 - 1.0 : 0.0;
}
}
