#pragma once

#include <Jerico/Core/Guid.hpp>

#include <cstdint>
#include <vector>

namespace Jerico::Tools::ProceduralAuthoring
{
struct Vec3d
{
    double x{};
    double y{};
    double z{};

    bool operator==(const Vec3d&) const = default;
};

struct TriangleMesh
{
    std::vector<Vec3d> positions;
    std::vector<std::uint32_t> indices;
    std::vector<Guid> triangleRegions;
};
}
