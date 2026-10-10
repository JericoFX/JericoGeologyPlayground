#pragma once

#include <Jerico/ProceduralAuthoring/Geology/GeologyLayer.hpp>

#include <cstdint>
#include <functional>

namespace Jerico::Tools::ProceduralAuthoring
{
using GeologyScalarFieldReference = std::function<double(const Vec3d&)>;

[[nodiscard]] Vec3d evaluateGeologyDirectionalWarpSamplePositionReference(
    const Vec3d& worldPosition,
    const GeologyDirectionalWarpPayload& warp,
    double strength,
    std::uint64_t seed,
    double maskWeight = 1.0) noexcept;

[[nodiscard]] double evaluateGeologyDirectionalWarpReference(
    const GeologyScalarFieldReference& source,
    const Vec3d& worldPosition,
    const GeologyDirectionalWarpPayload& warp,
    double strength,
    std::uint64_t seed,
    double maskWeight = 1.0);
}
