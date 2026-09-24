#include "SurfaceGeometry.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <set>
#include <stdexcept>
namespace Playground
{
namespace
{
void check(const Jerico::CancellationToken *cancel)
{
    if (cancel && cancel->isCancellationRequested())
        throw std::runtime_error("Cancelled");
}
Vec3d shifted(Vec3d p, Vec3d d, double t)
{
    return {p.x + d.x * t, p.y + d.y * t, p.z + d.z * t};
}
} // namespace
MeshResult planeSource(std::array<float, 2> size, int n)
{
    if (n < 1 || n > 64 || !std::isfinite(size[0]) || !std::isfinite(size[1]) || size[0] <= 0 ||
        size[1] <= 0 || size[0] > 100 || size[1] > 100)
        throw std::runtime_error("Plane needs positive finite size up to 100 and resolution 1..64");
    MeshResult result;
    result.closedSolid = false;
    auto id = Solid::region(0, 7);
    for (int y = 0; y <= n; ++y)
        for (int x = 0; x <= n; ++x)
            result.mesh.positions.push_back(
                {size[0] * (double(x) / n - 0.5), size[1] * (double(y) / n - 0.5), 0});
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
        {
            auto a = std::uint32_t(y * (n + 1) + x), b = a + 1, c = a + n + 1, d = c + 1;
            result.mesh.indices.insert(result.mesh.indices.end(), {a, b, d, a, d, c});
            result.mesh.triangleRegions.insert(result.mesh.triangleRegions.end(), {id, id});
        }
    result.field.resize(result.mesh.positions.size());
    result.regions.push_back({id, {}, SurfaceRole::Source, -1, -1});
    normals(result);
    return result;
}
MeshResult bspSource(const PA::BspDocument &document, std::span<const Jerico::Guid> selected,
                     int subdivisions, const Jerico::CancellationToken *cancel)
{
    check(cancel);
    if (selected.empty() || subdivisions < 1 || subdivisions > 64)
        throw std::runtime_error("BSP source needs selection and subdivisions 1..64");
    std::set<Jerico::Guid> requested(selected.begin(), selected.end());
    if (requested.size() != selected.size())
        throw std::runtime_error("Duplicate BSP selection GUID");
    PA::BspDocument subset{document.id, document.revision, {}};
    for (auto &brush : document.brushes)
        if (requested.contains(brush.id))
            subset.brushes.push_back(brush);
    if (subset.brushes.size() != requested.size())
        throw std::runtime_error("BSP selection GUID is missing");
    auto solid = PA::evaluateSolid(subset);
    if (!solid.hasValue())
        throw std::runtime_error(solid.error().code + ": " + solid.error().cause);
    check(cancel);
    Jerico::CancellationSource fallback;
    auto dense = PA::subdivideClosedMesh(solid.value().mesh, subdivisions, 240000,
                                         cancel ? *cancel : fallback.token());
    if (!dense.hasValue())
        throw std::runtime_error(dense.error().code + ": " + dense.error().cause);
    MeshResult result;
    result.mesh = std::move(dense.value());
    result.sourceRevision = document.revision;
    result.sourceSelection.assign(requested.begin(), requested.end());
    result.field.resize(result.mesh.positions.size());
    std::set<Jerico::Guid> ids(result.mesh.triangleRegions.begin(), result.mesh.triangleRegions.end());
    for (auto id : ids)
        result.regions.push_back({id, {}, SurfaceRole::Source, -1, -1});
    normals(result);
    return result;
}
MeshResult bspWallSource(int subdivisions, const Jerico::CancellationToken *cancel)
{
    auto id = Solid::region(0, 0x60000);
    PA::BspDocument document{Solid::region(0, 0x60001), 1, {}};
    PA::BrushNode wall{id,
                       PA::BrushBoolean::Add,
                       PA::BoxBrush{{3, 2, 0.5}},
                       {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
    document.brushes.push_back(wall);
    return bspSource(document, std::span<const Jerico::Guid>(&id, 1), subdivisions, cancel);
}
void cliffCutLoop(MeshResult &surface, const Layer &layer, int operation, bool showField,
                  const Jerico::CancellationToken *cancel)
{
    check(cancel);
    if (surface.closedSolid || surface.mesh.positions.empty() ||
        surface.field.size() != surface.mesh.positions.size())
        throw std::runtime_error("Cliff cut loop requires an open plane surface");
    const auto side = std::size_t(std::llround(std::sqrt(double(surface.mesh.positions.size()))));
    if (side < 5 || side > 65 || side * side != surface.mesh.positions.size() ||
        surface.mesh.indices.size() != 6 * (side - 1) * (side - 1) ||
        surface.mesh.triangleRegions.size() != 2 * (side - 1) * (side - 1))
        throw std::runtime_error("Cliff cut loop requires the original plane grid topology");
    const auto n = side - 1;
    const double rotation = layer.projectorRotation * std::numbers::pi / 180.0;
    const double cs = std::cos(rotation), sn = std::sin(rotation);
    auto coordinates = [&](Vec3d point)
    {
        const double x = point.x - layer.translation[0], y = point.y - layer.translation[1];
        return std::pair{(cs * x + sn * y) / layer.projectorRadius[0],
                         (-sn * x + cs * y) / layer.projectorRadius[1]};
    };
    auto contour = [&](double u, double v)
    {
        const double angle = std::atan2(v, u), seed = double(layer.seed % 1000) * 0.019;
        return 1.0 + layer.distortion *
                         (0.55 * std::sin(5 * angle + seed) + 0.30 * std::sin(9 * angle - seed));
    };
    std::vector<unsigned char> selected(n * n);
    std::vector<unsigned> incident(surface.mesh.positions.size()), inside(surface.mesh.positions.size());
    std::size_t selectedCount = 0;
    Jerico::Guid parent{};
    for (std::size_t y = 0; y < n; ++y)
        for (std::size_t x = 0; x < n; ++x)
        {
            const auto q = y * n + x, offset = 6 * q;
            if ((q & 255) == 0)
                check(cancel);
            const auto a = std::uint32_t(y * side + x), b = a + 1,
                       c = a + std::uint32_t(side), d = c + 1;
            const auto &indices = surface.mesh.indices;
            if (indices[offset] != a || indices[offset + 1] != b || indices[offset + 2] != d ||
                indices[offset + 3] != a || indices[offset + 4] != d || indices[offset + 5] != c ||
                surface.mesh.triangleRegions[2 * q] != surface.mesh.triangleRegions[2 * q + 1])
                throw std::runtime_error("Cliff cut loop cannot use a modified plane grid");
            auto p = surface.mesh.positions[a];
            for (auto vertex : {b, c, d})
            {
                const auto v = surface.mesh.positions[vertex];
                p.x += v.x;
                p.y += v.y;
            }
            auto [u, v] = coordinates({p.x * 0.25, p.y * 0.25, 0});
            const bool hit = std::hypot(u, v) < contour(u, v);
            selected[q] = hit;
            selectedCount += hit;
            if (hit)
            {
                const auto id = surface.mesh.triangleRegions[2 * q];
                if (parent == Jerico::Guid{})
                    parent = id;
                else if (parent != id)
                    throw std::runtime_error("Cliff cut loop crosses multiple source regions");
            }
            for (auto vertex : {a, b, c, d})
            {
                ++incident[vertex];
                inside[vertex] += hit;
            }
        }
    if (selectedCount < 4 || selectedCount == n * n)
        throw std::runtime_error("Cliff cut loop must select a local interior patch");
    const auto id = Solid::region(layer.seed, 0x434c494646000000ULL ^ std::uint64_t(operation));
    for (std::size_t q = 0; q < selected.size(); ++q)
        if (selected[q])
            surface.mesh.triangleRegions[2 * q] = surface.mesh.triangleRegions[2 * q + 1] = id;
    surface.regions.push_back({id, parent, SurfaceRole::CutLoop, operation, -1});
    for (std::size_t i = 0; i < surface.mesh.positions.size(); ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        if (!inside[i] || inside[i] != incident[i])
        {
            if (showField)
                surface.field[i] = 0;
            continue;
        }
        auto &p = surface.mesh.positions[i];
        auto [u, v] = coordinates(p);
        const double depth = std::clamp((contour(u, v) - std::hypot(u, v)) / 0.55, 0.0, 1.0);
        const double blend = depth * depth * (3 - 2 * depth);
        p.z += blend * (-layer.amplitude - 0.65 * p.z + layer.lateral * v);
        if (showField)
            surface.field[i] = float(blend);
    }
    surface.hardRegions = true;
    normals(surface);
}
MeshResult solidifySurface(const MeshResult &source, const Layer &layer,
                           const Jerico::CancellationToken *cancel)
{
    check(cancel);
    if (source.closedSolid)
        throw std::runtime_error("Solidify expects an open surface");
    const auto count = source.mesh.positions.size();
    if (count == 0 || count > 120000)
        throw std::runtime_error("Solidify exceeds vertex budget");
    if (source.mesh.indices.empty() || source.mesh.indices.size() % 3 ||
        source.mesh.indices.size() > 720000 ||
        source.mesh.triangleRegions.size() != source.mesh.indices.size() / 3 ||
        source.normals.size() != count || source.field.size() != count)
        throw std::runtime_error("Invalid solidify input arrays");
    for (auto index : source.mesh.indices)
        if (index >= count)
            throw std::runtime_error("Solidify source index out of range");
    for (auto &values : {&source.mesh.positions, &source.normals})
        for (auto p : *values)
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
                throw std::runtime_error("Non-finite solidify input");
    auto validAttribute = [&](const auto &values)
    {
        if (!values.empty() && values.size() != count)
            throw std::runtime_error("Invalid solidify metadata length");
        for (auto value : values)
            if (!std::isfinite(double(value)))
                throw std::runtime_error("Non-finite solidify metadata");
    };
    validAttribute(source.field);
    validAttribute(source.layerIndices);
    validAttribute(source.layerThickness);
    validAttribute(source.layerEdgeDistance);
    Recipe parameters;
    parameters.layers = {layer};
    validate(parameters);
    struct Edge
    {
        std::uint32_t a{}, b{};
        int count{};
    };
    std::map<std::pair<std::uint32_t, std::uint32_t>, Edge> edges;
    for (std::size_t t = 0; t < source.mesh.indices.size(); t += 3)
    {
        if ((t % 768) == 0)
            check(cancel);
        for (int k = 0; k < 3; ++k)
        {
            auto a = source.mesh.indices[t + k], b = source.mesh.indices[t + (k + 1) % 3];
            auto &edge = edges[std::minmax(a, b)];
            if (edge.count == 0)
            {
                edge.a = a;
                edge.b = b;
            }
            else if (edge.count != 1 || edge.a != b || edge.b != a)
                throw std::runtime_error("Surface has inconsistent winding or non-manifold edge");
            ++edge.count;
        }
    }
    auto result = source;
    result.cornerNormals.clear();
    result.stages.clear();
    result.closedSolid = true;
    result.hardRegions = true;
    result.mesh.positions.resize(count * 2);
    result.field.resize(count * 2);
    auto duplicate = [&](auto &values)
    {
        if (values.size() == count)
        {
            auto copy = values;
            values.insert(values.end(), copy.begin(), copy.end());
        }
    };
    duplicate(result.layerIndices);
    duplicate(result.layerThickness);
    duplicate(result.layerEdgeDistance);
    Vec3d axis{layer.direction[0], layer.direction[1], layer.direction[2]};
    double len = std::hypot(axis.x, axis.y, axis.z);
    if (!layer.solidAlongNormals && len < 1e-9)
        throw std::runtime_error("Solidify direction is zero");
    if (len > 1e-9)
        axis = {axis.x / len, axis.y / len, axis.z / len};
    double front = (layer.solidOffset + 1) * 0.5 * layer.thickness, back = front - layer.thickness;
    for (std::size_t i = 0; i < count; ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        auto direction = layer.solidAlongNormals ? source.normals[i] : axis;
        result.mesh.positions[i] = shifted(source.mesh.positions[i], direction, front);
        result.mesh.positions[count + i] = shifted(source.mesh.positions[i], direction, back);
        result.field[count + i] = source.field[i];
    }
    std::map<Jerico::Guid, Jerico::Guid> backs;
    for (std::size_t t = 0; t < source.mesh.indices.size(); t += 3)
    {
        auto id = source.mesh.triangleRegions[t / 3];
        if (!backs.contains(id))
        {
            auto backId = Solid::region(layer.seed, Jerico::stableGuidHash(id) ^ 0x51000);
            backs[id] = backId;
            result.regions.push_back({backId, id, SurfaceRole::SolidBack, -1, -1});
        }
        for (int k : {2, 1, 0})
            result.mesh.indices.push_back(std::uint32_t(count + source.mesh.indices[t + k]));
        result.mesh.triangleRegions.push_back(backs.at(id));
    }
    auto rim = Solid::region(layer.seed, PA::hashTriangleMesh(source.mesh) ^ 0x52000);
    result.regions.push_back({rim, {}, SurfaceRole::SolidRim, -1, -1});
    std::size_t boundaries = 0;
    for (auto &[key, edge] : edges)
        if (edge.count == 1)
        {
            if ((boundaries & 255) == 0)
                check(cancel);
            ++boundaries;
            auto a = edge.a, b = edge.b, c = std::uint32_t(count + a), d = std::uint32_t(count + b);
            result.mesh.indices.insert(result.mesh.indices.end(), {a, c, d, a, d, b});
            result.mesh.triangleRegions.insert(result.mesh.triangleRegions.end(), {rim, rim});
        }
    if (boundaries == 0)
        throw std::runtime_error("Solidify surface has no boundary");
    if (result.mesh.indices.size() / 3 > 240000)
        throw std::runtime_error("Solidify exceeds triangle budget");
    check(cancel);
    Solid::validate(result.mesh);
    check(cancel);
    normals(result);
    return result;
}
} // namespace Playground
