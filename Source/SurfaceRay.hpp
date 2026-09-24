#pragma once
#include "Model.hpp"
#include <TriangleBvh.hpp>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace Playground::Projection
{
namespace D = PA::Detail;
inline Vec3d cross(Vec3d a, Vec3d b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double length(Vec3d p)
{
    return std::sqrt(D::dot(p, p));
}
inline bool rayBounds(const PA::Bounds3d &bounds, Vec3d origin, Vec3d direction, double limit)
{
    double lo = 0, hi = limit;
    const double p[3] = {origin.x, origin.y, origin.z}, d[3] = {direction.x, direction.y, direction.z};
    const double b0[3] = {bounds.minimum.x, bounds.minimum.y, bounds.minimum.z};
    const double b1[3] = {bounds.maximum.x, bounds.maximum.y, bounds.maximum.z};
    for (int k = 0; k < 3; ++k)
    {
        if (std::abs(d[k]) < 1e-14)
        {
            if (p[k] < b0[k] || p[k] > b1[k])
                return false;
            continue;
        }
        auto a = (b0[k] - p[k]) / d[k], b = (b1[k] - p[k]) / d[k];
        if (a > b)
            std::swap(a, b);
        lo = std::max(lo, a);
        hi = std::min(hi, b);
        if (lo > hi)
            return false;
    }
    return true;
}
struct Hit
{
    double distance{std::numeric_limits<double>::infinity()}, u{}, v{};
    std::uint32_t triangle{std::numeric_limits<std::uint32_t>::max()};
    bool backface{};
    std::size_t rejectedBackfaces{};
};
inline Hit cast(const D::TriangleBvh &bvh, const TriangleMesh &mesh, Vec3d origin, Vec3d direction,
                double radius, std::uint32_t ignoreVertex = std::numeric_limits<std::uint32_t>::max(),
                const Jerico::Guid *region = nullptr, double preferredDistance = -1, bool frontOnly = false)
{
    Hit result;
    double nearest = radius;
    double bestScore = std::numeric_limits<double>::infinity();
    std::array<std::uint32_t, 128> stack{};
    std::size_t count = 1;
    while (count)
    {
        const auto &node = bvh.nodes()[stack[--count]];
        if (!rayBounds(node.bounds, origin, direction, preferredDistance < 0 ? nearest : radius))
            continue;
        if (node.count == 0)
        {
            if (count + 2 > stack.size())
                throw std::runtime_error("Surface ray BVH stack limit");
            stack[count++] = node.left;
            stack[count++] = node.right;
            continue;
        }
        for (std::uint32_t j = 0; j < node.count; ++j)
        {
            const auto &tri = bvh.triangles()[bvh.order()[node.begin + j]];
            auto t = tri.canonicalIndex * 3;
            if (mesh.indices[t] == ignoreVertex || mesh.indices[t + 1] == ignoreVertex ||
                mesh.indices[t + 2] == ignoreVertex)
                continue;
            if (region && mesh.triangleRegions[tri.canonicalIndex] != *region)
                continue;
            auto e1 = D::subtract(tri.second, tri.first), e2 = D::subtract(tri.third, tri.first);
            auto h = cross(direction, e2);
            auto det = D::dot(e1, h);
            if (std::abs(det) <= 1e-12 * length(e1) * length(e2))
                continue;
            auto s = D::subtract(origin, tri.first);
            auto u = D::dot(s, h) / det;
            if (u < -1e-10 || u > 1 + 1e-10)
                continue;
            auto q = cross(s, e1);
            auto v = D::dot(direction, q) / det;
            if (v < -1e-10 || u + v > 1 + 1e-10)
                continue;
            auto distance = D::dot(e2, q) / det;
            if (distance > 0 && distance <= radius)
            {
                if (frontOnly && det < 0)
                {
                    ++result.rejectedBackfaces;
                    continue;
                }
                auto score = preferredDistance < 0 ? distance : std::abs(distance - preferredDistance);
                if (score < bestScore || (score == bestScore && tri.canonicalIndex < result.triangle))
                {
                    bestScore = score;
                    nearest = distance;
                    result.distance = distance;
                    result.u = u;
                    result.v = v;
                    result.triangle = tri.canonicalIndex;
                    result.backface = det < 0;
                }
            }
        }
    }
    return result;
}
inline Hit closestFront(const D::TriangleBvh &bvh, Vec3d point, Vec3d normal, double radius)
{
    Hit result;
    double nearest = radius;
    std::array<std::uint32_t, 128> stack{};
    std::size_t count = 1;
    while (count)
    {
        const auto &node = bvh.nodes()[stack[--count]];
        if (D::distanceSquared(node.bounds, point) > nearest * nearest)
            continue;
        if (node.count == 0)
        {
            if (count + 2 > stack.size())
                throw std::runtime_error("Surface projection BVH stack limit");
            stack[count++] = node.left;
            stack[count++] = node.right;
            continue;
        }
        for (std::uint32_t i = 0; i < node.count; ++i)
        {
            const auto &tri = bvh.triangles()[bvh.order()[node.begin + i]];
            auto e = D::subtract(tri.second, tri.first), f = D::subtract(tri.third, tri.first),
                 face = cross(e, f);
            if (D::dot(face, normal) <= 0)
            {
                ++result.rejectedBackfaces;
                continue;
            }
            auto closest = D::closestPointOnTriangle(point, tri.first, tri.second, tri.third);
            double distance = length(D::subtract(closest, point));
            if (distance > nearest || (distance == nearest && tri.canonicalIndex >= result.triangle))
                continue;
            auto q = D::subtract(closest, tri.first);
            double denominator = D::dot(face, face);
            nearest = distance;
            result.distance = distance;
            result.triangle = tri.canonicalIndex;
            result.u = D::dot(cross(q, f), face) / denominator;
            result.v = D::dot(cross(e, q), face) / denominator;
        }
    }
    return result;
}
} // namespace Playground::Projection
