#include "SolidGeometry.hpp"
#include <Jerico/Jobs/Cancellation.hpp>
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <TriangleBvh.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace Playground::Solid
{
namespace D = PA::Detail;
namespace
{
void check(const Jerico::CancellationToken *cancel)
{
    if (cancel && cancel->isCancellationRequested())
        throw std::runtime_error("Cancelled");
}
double random(std::uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return double(value & 0xffffffU) / 16777215.0;
}
TriangleMesh tessellate(TriangleMesh mesh, const Layer &layer, const Jerico::CancellationToken *cancel)
{
    check(cancel);
    if (layer.cutSubdivisions == 1)
        return mesh;
    Jerico::CancellationSource cancellation;
    auto refined =
        PA::subdivideClosedMesh(mesh, layer.cutSubdivisions, 240000, cancel ? *cancel : cancellation.token());
    if (!refined.hasValue())
        throw std::runtime_error(refined.error().code + ": " + refined.error().cause);
    check(cancel);
    return std::move(refined.value());
}
} // namespace
Jerico::Guid region(std::uint32_t seed, std::uint64_t id)
{
    Jerico::Guid root;
    root.bytes[0] = std::byte{0x47};
    root.bytes[1] = std::byte{0x50};
    for (int k = 0; k < 4; ++k)
        root.bytes[4 + k] = std::byte((seed >> (8 * k)) & 255);
    return PA::deriveFaceRegionGuid(root, id);
}
TriangleMesh validate(const TriangleMesh &mesh)
{
    auto result = PA::validateClosedMesh(mesh);
    if (!result.hasValue())
        throw std::runtime_error(result.error().code + ": " + result.error().cause);
    return std::move(result.value());
}
TriangleMesh planes(const TriangleMesh &mesh, const Layer &layer, const Jerico::CancellationToken *cancel)
{
    check(cancel);
    auto result = validate(mesh);
    auto bounds = D::emptyBounds();
    for (auto p : mesh.positions)
        D::include(bounds, p);
    auto center = D::multiply(D::add(bounds.minimum, bounds.maximum), 0.5);
    Jerico::CancellationSource cancellation;
    for (int k = 0; k < layer.count; ++k)
    {
        check(cancel);
        double y = 1.0 - 2.0 * (k + 0.5) / layer.count;
        double phi = k * std::numbers::pi * (3 - std::sqrt(5.0)) + random(layer.seed) * 6.28318530718;
        double radius = std::sqrt(1 - y * y);
        Vec3d normal{radius * std::cos(phi), y, radius * std::sin(phi)};
        double support = 0;
        for (auto p : result.positions)
            support = std::max(support, D::dot(D::subtract(p, center), normal));
        double depth =
            std::clamp(double(layer.amplitude) * (0.75 + 0.5 * random(layer.seed + k * 73)), 0.01, 0.45);
        PA::MeshPlaneCut cut{normal, D::dot(center, normal) + support * (1 - depth),
                             region(layer.seed, PA::hashTriangleMesh(mesh) ^ (0x10000 + k)), 240000};
        auto split = PA::splitClosedMeshByPlane(result, cut, cancel ? *cancel : cancellation.token());
        if (!split.hasValue())
            throw std::runtime_error(split.error().code + ": " + split.error().cause);
        result = std::move(split.value().negative);
        check(cancel);
    }
    return tessellate(std::move(result), layer, cancel);
}
TriangleMesh envelope(const TriangleMesh &mesh, const Layer &layer, const Jerico::CancellationToken *cancel)
{
    check(cancel);
    validate(mesh);
    // Keep source ordering when all support planes contain it.
    auto result = mesh;
    std::vector<Vec3d> directions;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z)
            {
                int nonzero = (x != 0) + (y != 0) + (z != 0);
                if (nonzero == 0 || (layer.envelopeShape == 1 && nonzero != 1))
                    continue;
                if (layer.envelopeShape == 3 && y != 0 && (x != 0 || z != 0))
                    continue;
                double length = std::sqrt(double(x * x + y * y + z * z));
                directions.push_back({x / length, y / length, z / length});
            }
    Vec3d extent{layer.scale[0], layer.scale[1], layer.scale[2]};
    Vec3d center{layer.translation[0], layer.translation[1], layer.translation[2]};
    double rounding = layer.envelopeRoundness * std::min({extent.x, extent.y, extent.z});
    auto rotate = [&](Vec3d p)
    {
        double x = layer.rotation[0] * std::numbers::pi / 180, y = layer.rotation[1] * std::numbers::pi / 180,
               z = layer.rotation[2] * std::numbers::pi / 180;
        p = {p.x, p.y * std::cos(x) - p.z * std::sin(x), p.y * std::sin(x) + p.z * std::cos(x)};
        p = {p.x * std::cos(y) + p.z * std::sin(y), p.y, -p.x * std::sin(y) + p.z * std::cos(y)};
        return Vec3d{p.x * std::cos(z) - p.y * std::sin(z), p.x * std::sin(z) + p.y * std::cos(z), p.z};
    };
    Jerico::CancellationSource cancellation;
    auto identity = PA::hashTriangleMesh(mesh);
    bool clipped = false;
    for (std::size_t k = 0; k < directions.size(); ++k)
    {
        check(cancel);
        auto n = directions[k];
        double support = std::sqrt(std::pow(extent.x * n.x, 2) + std::pow(extent.y * n.y, 2) +
                                   std::pow(extent.z * n.z, 2));
        if (layer.envelopeShape == 1)
            support = extent.x * std::abs(n.x) + extent.y * std::abs(n.y) + extent.z * std::abs(n.z);
        if (layer.envelopeShape == 2)
            support = (extent.x - rounding) * std::abs(n.x) + (extent.y - rounding) * std::abs(n.y) +
                      (extent.z - rounding) * std::abs(n.z) + rounding;
        if (layer.envelopeShape == 3)
            support = std::hypot(extent.x * n.x, extent.z * n.z) + extent.y * std::abs(n.y);
        n = rotate(n);
        double offset = D::dot(center, n) + support, lo = 1e20, hi = -1e20;
        for (auto p : result.positions)
        {
            double h = D::dot(p, n);
            lo = std::min(lo, h);
            hi = std::max(hi, h);
        }
        if (hi <= offset + 1e-9)
            continue;
        if (lo >= offset - 1e-9)
            throw std::runtime_error("Envelope has empty or zero-volume intersection");
        PA::MeshPlaneCut cut{n, offset, region(layer.seed, identity ^ (0x30000 + k)), 240000};
        auto split = PA::splitClosedMeshByPlane(result, cut, cancel ? *cancel : cancellation.token());
        if (!split.hasValue())
            throw std::runtime_error(split.error().code + ": " + split.error().cause);
        result = std::move(split.value().negative);
        clipped = true;
    }
    return clipped ? tessellate(std::move(result), layer, cancel) : result;
}
void transferSamples(const MeshResult &source, const std::vector<Vec3d> &sourceWarp, MeshResult &destination,
                     std::vector<Vec3d> &destinationWarp, const Jerico::CancellationToken *cancel)
{
    std::vector<D::Triangle> triangles;
    for (std::size_t t = 0; t < source.mesh.indices.size(); t += 3)
    {
        if ((t % 768) == 0)
            check(cancel);
        auto a = source.mesh.positions[source.mesh.indices[t]],
             b = source.mesh.positions[source.mesh.indices[t + 1]],
             c = source.mesh.positions[source.mesh.indices[t + 2]];
        auto bounds = D::emptyBounds();
        D::include(bounds, a);
        D::include(bounds, b);
        D::include(bounds, c);
        triangles.push_back({a, b, c, D::multiply(D::add(D::add(a, b), c), 1.0 / 3), bounds,
                             std::uint32_t(t / 3), std::uint32_t(t / 3)});
    }
    D::TriangleBvh bvh(std::move(triangles));
    destination.field.clear();
    destinationWarp.clear();
    for (auto p : destination.mesh.positions)
    {
        if ((destination.field.size() & 255) == 0)
            check(cancel);
        auto closest = bvh.closest(p, 1e-12);
        auto t = closest.owner * 3;
        auto ai = source.mesh.indices[t], bi = source.mesh.indices[t + 1], ci = source.mesh.indices[t + 2];
        auto a = source.mesh.positions[ai], b = source.mesh.positions[bi], c = source.mesh.positions[ci];
        auto q = D::closestPointOnTriangle(p, a, b, c), u = D::subtract(b, a), v = D::subtract(c, a),
             w = D::subtract(q, a);
        double uu = D::dot(u, u), uv = D::dot(u, v), vv = D::dot(v, v), wu = D::dot(w, u), wv = D::dot(w, v);
        double denominator = uu * vv - uv * uv;
        double beta = denominator > 1e-24 ? (wu * vv - wv * uv) / denominator : 0;
        double gamma = denominator > 1e-24 ? (wv * uu - wu * uv) / denominator : 0;
        double alpha = 1 - beta - gamma;
        destination.field.push_back(
            float(alpha * source.field[ai] + beta * source.field[bi] + gamma * source.field[ci]));
        if (source.layerIndices.size() == source.mesh.positions.size())
            destination.layerIndices.push_back(source.layerIndices[alpha >= beta && alpha >= gamma ? ai
                                                                   : beta >= gamma                 ? bi
                                                                                                   : ci]);
        if (source.layerThickness.size() == source.mesh.positions.size())
            destination.layerThickness.push_back(float(alpha * source.layerThickness[ai] +
                                                       beta * source.layerThickness[bi] +
                                                       gamma * source.layerThickness[ci]));
        if (source.layerEdgeDistance.size() == source.mesh.positions.size())
            destination.layerEdgeDistance.push_back(float(alpha * source.layerEdgeDistance[ai] +
                                                          beta * source.layerEdgeDistance[bi] +
                                                          gamma * source.layerEdgeDistance[ci]));
        destinationWarp.push_back(
            D::add(D::multiply(sourceWarp[ai], alpha),
                   D::add(D::multiply(sourceWarp[bi], beta), D::multiply(sourceWarp[ci], gamma))));
    }
}
} // namespace Playground::Solid
