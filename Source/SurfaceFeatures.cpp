#include "SurfaceFeatures.hpp"
#include "SurfaceRay.hpp"
#include <TriangleBvh.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <map>
#include <numbers>
#include <queue>
#include <stdexcept>
namespace Playground
{
namespace D = PA::Detail;
namespace
{
void check(const Jerico::CancellationToken *cancel)
{
    if (cancel && cancel->isCancellationRequested())
        throw std::runtime_error("Cancelled");
}
Vec3d cross(Vec3d a, Vec3d b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double length(Vec3d p)
{
    return std::sqrt(D::dot(p, p));
}
Vec3d unit(Vec3d p)
{
    auto size = length(p);
    if (!(size > 0))
        throw std::runtime_error("Surface feature normal has zero support");
    return D::multiply(p, 1 / size);
}
} // namespace
const SurfaceFeatureFields &SurfaceFeatureCache::get(const TriangleMesh &mesh,
    const SurfaceFeatureSettings &settings,const Jerico::CancellationToken *cancel)
{
    check(cancel);
    const auto hash=meshHash(mesh);
    const auto &old=settings_;
    const bool sameSettings=old.contact==settings.contact &&
        old.ambient==settings.ambient && old.thickness==settings.thickness &&
        old.radius==settings.radius && old.creaseAngleDegrees==settings.creaseAngleDegrees &&
        old.contactSamples==settings.contactSamples &&
        old.ambientRadius==settings.ambientRadius && old.ambientSamples==settings.ambientSamples &&
        old.thicknessRadius==settings.thicknessRadius &&
        old.exposureDirection.x==settings.exposureDirection.x &&
        old.exposureDirection.y==settings.exposureDirection.y &&
        old.exposureDirection.z==settings.exposureDirection.z;
    if (valid_ && hash==meshHash_ && sameSettings)
    {++hits_;return fields_;}
    const auto start=std::chrono::steady_clock::now();
    auto computed=surfaceFeatures(mesh,settings,cancel);
    computeMilliseconds_+=std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now()-start).count();
    fields_=std::move(computed);
    settings_=settings;meshHash_=hash;valid_=true;++misses_;
    return fields_;
}
std::vector<double> relaxSignedCurvature(const TriangleMesh &mesh, const std::vector<double> &raw,
                                         double lambda, const Jerico::CancellationToken *cancel)
{
    check(cancel);
    if (!std::isfinite(lambda) || lambda < 0 || lambda > 1 || raw.size() != mesh.positions.size() ||
        mesh.indices.empty() || mesh.indices.size() % 3 ||
        (!mesh.triangleRegions.empty() && mesh.triangleRegions.size() != mesh.indices.size() / 3))
        throw std::runtime_error("Curvature relaxation input or weight invalid");
    for (auto value : raw)
        if (!std::isfinite(value))
            throw std::runtime_error("Curvature relaxation source non-finite");
    if (lambda == 0)
        return raw;

    struct Edge
    {
        std::uint32_t firstFace{}, secondFace{};
        unsigned count{};
    };
    std::map<std::pair<std::uint32_t, std::uint32_t>, Edge> edges;
    std::vector<Jerico::Guid> vertexRegion(mesh.positions.size());
    std::vector<bool> assigned(mesh.positions.size()), mixed(mesh.positions.size());
    for (std::size_t face = 0; face < mesh.indices.size() / 3; ++face)
    {
        if ((face & 255) == 0)
            check(cancel);
        for (int corner = 0; corner < 3; ++corner)
        {
            auto a = mesh.indices[3 * face + corner];
            auto b = mesh.indices[3 * face + (corner + 1) % 3];
            if (a >= mesh.positions.size() || b >= mesh.positions.size())
                throw std::runtime_error("Curvature relaxation index out of range");
            if (!mesh.triangleRegions.empty())
            {
                if (assigned[a] && vertexRegion[a] != mesh.triangleRegions[face])
                    mixed[a] = true;
                else if (!assigned[a])
                    vertexRegion[a] = mesh.triangleRegions[face];
                assigned[a] = true;
            }
            auto key = std::minmax(a, b);
            auto &edge = edges[{key.first, key.second}];
            if (edge.count++ == 0)
                edge.firstFace = std::uint32_t(face);
            else if (edge.count == 2)
                edge.secondFace = std::uint32_t(face);
            else
                throw std::runtime_error("Curvature relaxation non-manifold edge");
        }
    }
    std::vector<double> weighted(mesh.positions.size()), total(mesh.positions.size());
    for (const auto &[key, edge] : edges)
    {
        if (edge.count != 2 || mixed[key.first] || mixed[key.second] ||
            (!mesh.triangleRegions.empty() &&
             mesh.triangleRegions[edge.firstFace] != mesh.triangleRegions[edge.secondFace]))
            continue;
        if (!((raw[key.first] > 0 && raw[key.second] > 0) ||
              (raw[key.first] < 0 && raw[key.second] < 0)))
            continue;
        auto distance = length(D::subtract(mesh.positions[key.first], mesh.positions[key.second]));
        if (!(distance > 0) || !std::isfinite(distance))
            throw std::runtime_error("Curvature relaxation edge length invalid");
        auto weight = 1 / distance;
        weighted[key.first] += weight * raw[key.second];
        weighted[key.second] += weight * raw[key.first];
        total[key.first] += weight;
        total[key.second] += weight;
    }
    auto relaxed = raw;
    for (std::size_t i = 0; i < relaxed.size(); ++i)
        if (total[i] > 0)
        {
            relaxed[i] = (1 - lambda) * raw[i] + lambda * weighted[i] / total[i];
            if (!std::isfinite(relaxed[i]))
                throw std::runtime_error("Curvature relaxation result non-finite");
        }
    return relaxed;
}
SurfaceFeatureFields surfaceFeatures(const TriangleMesh &mesh, const SurfaceFeatureSettings &settings,
                                     const Jerico::CancellationToken *cancel)
{
    check(cancel);
    if (!std::isfinite(settings.radius) || settings.radius < 0.001 || settings.radius > 10 ||
        !std::isfinite(settings.creaseAngleDegrees) || settings.creaseAngleDegrees < 1 ||
        settings.creaseAngleDegrees > 179 || settings.contactSamples < 4 || settings.contactSamples > 64 ||
        !std::isfinite(settings.ambientRadius) || settings.ambientRadius < 0.001 ||
        settings.ambientRadius > 100 || settings.ambientSamples < 4 || settings.ambientSamples > 128 ||
        !std::isfinite(settings.thicknessRadius) || settings.thicknessRadius < 0.001 ||
        settings.thicknessRadius > 1000 || !std::isfinite(settings.exposureDirection.x) ||
        !std::isfinite(settings.exposureDirection.y) || !std::isfinite(settings.exposureDirection.z) ||
        !(D::lengthSquared(settings.exposureDirection) > 1e-18))
        throw std::runtime_error("Surface feature settings outside limits");
    const auto size = mesh.positions.size();
    if (size == 0 || size > 240000 || mesh.indices.empty() || mesh.indices.size() % 3 ||
        mesh.indices.size() > 720000)
        throw std::runtime_error("Surface feature input shape or budget invalid");
    for (auto p : mesh.positions)
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
            throw std::runtime_error("Surface feature position is non-finite");
    SurfaceFeatureFields result;
    result.signedDihedralCurvature.resize(size);
    result.convexity.resize(size);
    result.concavity.resize(size);
    result.absoluteCurvature.resize(size);
    result.edgeDistance.assign(size, std::numeric_limits<double>::infinity());
    result.boundaryDistance.assign(size, std::numeric_limits<double>::infinity());
    result.exposure.resize(size);
    result.minimumEdgeLength.assign(size, std::numeric_limits<double>::infinity());
    result.minimumTriangleAltitude.assign(size, std::numeric_limits<double>::infinity());
    result.boundary.resize(size);
    std::vector<double> area(size);
    std::vector<Vec3d> vertexNormals(size), faceNormals;
    std::vector<D::Triangle> triangles;
    faceNormals.reserve(mesh.indices.size() / 3);
    triangles.reserve(mesh.indices.size() / 3);
    struct Edge
    {
        std::uint32_t from{}, to{}, face{};
        int count{};
    };
    std::map<std::pair<std::uint32_t, std::uint32_t>, Edge> edges;
    std::vector<std::vector<std::pair<std::uint32_t, double>>> neighbors(size);
    const auto angleLimit = settings.creaseAngleDegrees * std::numbers::pi / 180;
    for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
    {
        if (t % 768 == 0)
            check(cancel);
        auto a = mesh.indices[t], b = mesh.indices[t + 1], c = mesh.indices[t + 2];
        if (a >= size || b >= size || c >= size)
            throw std::runtime_error("Surface feature index out of range");
        auto p = mesh.positions[a], q = mesh.positions[b], r = mesh.positions[c];
        auto ab = D::subtract(q, p), ac = D::subtract(r, p), n = cross(ab, ac);
        double area2 = length(n), scale = std::max(D::lengthSquared(ab), D::lengthSquared(ac));
        if (!(area2 > 1e-12 * scale) || !std::isfinite(area2))
            throw std::runtime_error("Surface feature degenerate triangle");
        faceNormals.push_back(D::multiply(n, 1 / area2));
        auto altitude = area2 / std::max({length(ab), length(ac), length(D::subtract(r, q))});
        for (auto i : {a, b, c})
        {
            area[i] += area2 / 6;
            vertexNormals[i] = D::add(vertexNormals[i], n);
            result.minimumTriangleAltitude[i] = std::min(result.minimumTriangleAltitude[i], altitude);
        }
        auto bounds = D::emptyBounds();
        D::include(bounds, p);
        D::include(bounds, q);
        D::include(bounds, r);
        triangles.push_back(
            {p, q, r, D::multiply(D::add(D::add(p, q), r), 1.0 / 3), bounds, 0, std::uint32_t(t / 3)});
        for (int k = 0; k < 3; ++k)
        {
            auto from = mesh.indices[t + k], to = mesh.indices[t + (k + 1) % 3];
            auto key = std::minmax(from, to);
            auto &edge = edges[{key.first, key.second}];
            if (++edge.count == 1)
            {
                edge.from = from;
                edge.to = to;
                edge.face = std::uint32_t(t / 3);
            }
            else
            {
                if (edge.count != 2 || edge.from != to || edge.to != from)
                    throw std::runtime_error("Surface feature non-manifold or inconsistent winding at edge " +
                                             std::to_string(key.first) + "," +
                                             std::to_string(key.second) + " count=" +
                                             std::to_string(edge.count) + " face=" +
                                             std::to_string(t / 3));
                auto tangent = unit(D::subtract(mesh.positions[edge.to], mesh.positions[edge.from]));
                auto nf = faceNormals[edge.face], ng = faceNormals[t / 3];
                auto theta =
                    std::atan2(D::dot(tangent, cross(nf, ng)), std::clamp(D::dot(nf, ng), -1.0, 1.0));
                auto integral = length(D::subtract(mesh.positions[from], mesh.positions[to])) * theta;
                result.signedDihedralCurvature[from] += integral;
                result.signedDihedralCurvature[to] += integral;
                if (std::abs(theta) >= angleLimit)
                    result.edgeDistance[from] = result.edgeDistance[to] = 0;
            }
        }
    }
    for (const auto &[key, edge] : edges)
    {
        double distance = length(D::subtract(mesh.positions[key.first], mesh.positions[key.second]));
        for (auto i : {key.first, key.second})
        {
            result.minimumEdgeLength[i] = std::min(result.minimumEdgeLength[i], distance);
            if (edge.count == 1)
            {
                result.boundary[i] = true;
                result.boundaryDistance[i] = 0;
            }
        }
        neighbors[key.first].push_back({key.second, distance});
        neighbors[key.second].push_back({key.first, distance});
    }
    auto exposureDirection = unit(settings.exposureDirection);
    for (std::uint32_t i = 0; i < size; ++i)
    {
        if (!(area[i] > 0))
            throw std::runtime_error("Surface feature isolated vertex");
        result.signedDihedralCurvature[i] /= 4 * area[i];
        if (!std::isfinite(result.signedDihedralCurvature[i]))
            throw std::runtime_error("Surface feature curvature overflow");
        result.convexity[i] = std::max(0.0, result.signedDihedralCurvature[i]);
        result.concavity[i] = std::max(0.0, -result.signedDihedralCurvature[i]);
        result.absoluteCurvature[i] = std::abs(result.signedDihedralCurvature[i]);
        result.exposure[i] = std::max(0.0, D::dot(unit(vertexNormals[i]), exposureDirection));
    }
    auto propagateDistance = [&](std::vector<double> &distance)
    {
        using Entry = std::pair<double, std::uint32_t>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
        for (std::uint32_t i = 0; i < size; ++i)
            if (distance[i] == 0)
                queue.push({0, i});
        std::size_t visits = 0;
        while (!queue.empty())
        {
            if ((visits++ & 255) == 0)
                check(cancel);
            auto [current, i] = queue.top();
            queue.pop();
            if (current > distance[i])
                continue;
            for (auto [j, step] : neighbors[i])
                if (current + step < distance[j])
                {
                    distance[j] = current + step;
                    queue.push({current + step, j});
                }
        }
    };
    propagateDistance(result.edgeDistance);
    propagateDistance(result.boundaryDistance);
    if (settings.contact || settings.ambient || settings.thickness)
    {
        check(cancel);
        D::TriangleBvh bvh(std::move(triangles));
        if (settings.contact)
            result.contactOcclusion.resize(size);
        if (settings.ambient)
            result.ambientOcclusion.resize(size);
        if (settings.thickness)
        {
            result.thickness.resize(size);
            result.thicknessConfidence.resize(size);
        }
        for (std::uint32_t i = 0; i < size; ++i)
        {
            if ((i & 63) == 0)
                check(cancel);
            auto n = unit(vertexNormals[i]);
            auto tangent = unit(cross(std::abs(n.z) < 0.9 ? Vec3d{0, 0, 1} : Vec3d{0, 1, 0}, n));
            auto bitangent = cross(n, tangent);
            auto occlusion = [&](double radius, int samples)
            {
                const double bias = radius * 1e-5;
                auto origin = D::add(mesh.positions[i], D::multiply(n, bias));
                double sum = 0;
                for (int j = 0; j < samples; ++j)
                {
                    // Fixed cosine-weighted golden-angle disk sequence. No seed-dependent AO shimmer.
                    double u = (j + 0.5) / samples, phi = j * std::numbers::pi * (3 - std::sqrt(5.0));
                    auto direction =
                        D::add(D::multiply(n, std::sqrt(1 - u)),
                               D::add(D::multiply(tangent, std::sqrt(u) * std::cos(phi)),
                                      D::multiply(bitangent, std::sqrt(u) * std::sin(phi))));
                    auto distance = Projection::cast(bvh, mesh, origin, direction, radius - bias, i).distance +
                                    bias;
                    if (distance < radius)
                        sum += 1 - distance / radius;
                }
                return sum / samples;
            };
            if (settings.contact)
                result.contactOcclusion[i] = occlusion(settings.radius, settings.contactSamples);
            if (settings.ambient)
                result.ambientOcclusion[i] = occlusion(settings.ambientRadius, settings.ambientSamples);
            if (settings.thickness)
            {
                const double bias = settings.thicknessRadius * 1e-7;
                auto origin = D::subtract(mesh.positions[i], D::multiply(n, bias));
                auto hit = Projection::cast(bvh, mesh, origin, D::multiply(n, -1),
                                            settings.thicknessRadius - bias, i);
                if (std::isfinite(hit.distance))
                {
                    result.thickness[i] = hit.distance + bias;
                    result.thicknessConfidence[i] = 1;
                }
            }
        }
    }
    result.cavityAccumulation.resize(size);
    for (std::size_t i=0;i<size;++i)
    {
        const auto concavity=std::clamp(result.concavity[i]*settings.radius,0.0,1.0);
        const auto contact=result.contactOcclusion.empty()?0.0:result.contactOcclusion[i];
        result.cavityAccumulation[i]=std::clamp(.5*concavity+.5*contact,0.0,1.0);
    }
    check(cancel);
    return result;
}
} // namespace Playground
