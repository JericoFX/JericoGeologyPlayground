#include "AutoUv.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <map>
#include <numbers>
#include <queue>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace Playground
{
namespace
{
struct Face
{
    Vec3d normal;
    unsigned axis{};
};
struct Island
{
    unsigned axis{};
    std::vector<std::size_t> faces;
    double minU{std::numeric_limits<double>::infinity()};
    double minV{std::numeric_limits<double>::infinity()};
    double maxU{-std::numeric_limits<double>::infinity()};
    double maxV{-std::numeric_limits<double>::infinity()};
    unsigned x{}, y{}, width{}, height{};
};
using Point2 = std::array<double, 2>;
struct ProjectedFace
{
    std::array<Point2, 3> points;
    double minU{}, minV{}, maxU{}, maxV{}, area{};
};
void check(const Jerico::CancellationToken *cancel)
{
    if (cancel && cancel->isCancellationRequested())
        throw std::runtime_error("Automatic UV planning cancelled");
}
Vec3d subtract(Vec3d a, Vec3d b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3d cross(Vec3d a, Vec3d b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double dot(Vec3d a, Vec3d b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
unsigned dominantAxis(Vec3d n)
{
    if (std::abs(n.x) >= std::abs(n.y) && std::abs(n.x) >= std::abs(n.z))
        return n.x >= 0 ? 0 : 1;
    if (std::abs(n.y) >= std::abs(n.z))
        return n.y >= 0 ? 2 : 3;
    return n.z >= 0 ? 4 : 5;
}
std::array<double, 2> project(Vec3d p, unsigned axis)
{
    switch (axis)
    {
    case 0: return {p.y, p.z};
    case 1: return {-p.y, p.z};
    case 2: return {-p.x, p.z};
    case 3: return {p.x, p.z};
    case 4: return {p.x, p.y};
    default: return {-p.x, p.y};
    }
}
double orient(Point2 a, Point2 b, Point2 c)
{
    return (b[0] - a[0]) * (c[1] - a[1]) -
           (b[1] - a[1]) * (c[0] - a[0]);
}
bool validProjectedTriangle(const std::array<Point2, 3> &points)
{
    auto squaredLength = [](Point2 a, Point2 b)
    {
        auto x = b[0] - a[0], y = b[1] - a[1];
        return x * x + y * y;
    };
    auto scale = std::max(squaredLength(points[0], points[1]),
                          squaredLength(points[0], points[2]));
    auto area2 = std::abs(orient(points[0], points[1], points[2]));
    return std::isfinite(area2) && area2 > 1e-12 * scale;
}
bool overlapsInterior(const ProjectedFace &a, const ProjectedFace &b)
{
    if (a.maxU <= b.minU || b.maxU <= a.minU ||
        a.maxV <= b.minV || b.maxV <= a.minV)
        return false;
    std::array<Point2, 8> polygon{}, clipped{};
    std::copy(a.points.begin(), a.points.end(), polygon.begin());
    unsigned count = 3;
    double sign = orient(b.points[0], b.points[1], b.points[2]) > 0 ? 1 : -1;
    for (unsigned edge = 0; edge < 3 && count; ++edge)
    {
        auto start = b.points[edge], end = b.points[(edge + 1) % 3];
        unsigned nextCount = 0;
        for (unsigned i = 0; i < count; ++i)
        {
            auto p = polygon[i], q = polygon[(i + 1) % count];
            auto dp = sign * orient(start, end, p);
            auto dq = sign * orient(start, end, q);
            if (dp >= 0)
                clipped[nextCount++] = p;
            if ((dp < 0 && dq > 0) || (dp > 0 && dq < 0))
            {
                auto t = dp / (dp - dq);
                clipped[nextCount++] = {p[0] + t * (q[0] - p[0]),
                                        p[1] + t * (q[1] - p[1])};
            }
        }
        polygon = clipped;
        count = nextCount;
    }
    double area2 = 0;
    for (unsigned i = 1; i + 1 < count; ++i)
        area2 += orient(polygon[0], polygon[i], polygon[i + 1]);
    return std::abs(area2) * 0.5 > std::min(a.area, b.area) * 1e-9;
}
bool validateIslandOverlap(const TriangleMesh &mesh, const Island &island,
                           const Jerico::CancellationToken *cancel,
                           const std::vector<std::array<float, 2>> *cornerUv = nullptr)
{
    auto n = island.faces.size();
    auto grid = std::min<unsigned>(256, std::max<unsigned>(1, unsigned(std::ceil(std::sqrt(n / 4.0)))));
    std::vector<std::vector<std::size_t>> bins(grid * grid);
    std::vector<ProjectedFace> projected(n);
    std::vector<std::size_t> seen(n, std::numeric_limits<std::size_t>::max());
    double minimumU = std::numeric_limits<double>::infinity();
    double minimumV = std::numeric_limits<double>::infinity();
    double maximumU = -std::numeric_limits<double>::infinity();
    double maximumV = -std::numeric_limits<double>::infinity();
    auto cell = [grid](double value, double minimum, double span)
    {
        return unsigned(std::clamp((value - minimum) / span * grid, 0.0, double(grid - 1)));
    };
    for (std::size_t i = 0; i < n; ++i)
    {
        if ((i & 1023) == 0)
            check(cancel);
        auto &face = projected[i];
        face.minU = face.minV = std::numeric_limits<double>::infinity();
        face.maxU = face.maxV = -std::numeric_limits<double>::infinity();
        for (int k = 0; k < 3; ++k)
        {
            auto p = cornerUv
                ? Point2{(*cornerUv)[3 * island.faces[i] + k][0],
                         (*cornerUv)[3 * island.faces[i] + k][1]}
                : project(mesh.positions[mesh.indices[3 * island.faces[i] + k]], island.axis);
            face.points[k] = p;
            face.minU = std::min(face.minU, p[0]);
            face.minV = std::min(face.minV, p[1]);
            face.maxU = std::max(face.maxU, p[0]);
            face.maxV = std::max(face.maxV, p[1]);
        }
        face.area = std::abs(orient(face.points[0], face.points[1], face.points[2])) * 0.5;
        if (!validProjectedTriangle(face.points))
        {
            std::ostringstream detail;
            detail << std::setprecision(17) << "Automatic UV collapsed "
                   << (cornerUv ? "packed" : "world") << " island triangle "
                   << island.faces[i] << " on axis " << island.axis
                   << " area2=" << 2 * face.area;
            if (cornerUv)
            {
                std::array<Point2, 3> world;
                for (int corner = 0; corner < 3; ++corner)
                    world[corner] = project(
                        mesh.positions[mesh.indices[3 * island.faces[i] + corner]], island.axis);
                detail << " worldArea2=" << std::abs(orient(world[0], world[1], world[2]));
            }
            throw std::runtime_error(detail.str());
        }
        minimumU = std::min(minimumU, face.minU);
        minimumV = std::min(minimumV, face.minV);
        maximumU = std::max(maximumU, face.maxU);
        maximumV = std::max(maximumV, face.maxV);
    }
    const double spanU = maximumU - minimumU, spanV = maximumV - minimumV;
    std::size_t comparisons = 0, binEntries = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        if ((i & 1023) == 0)
            check(cancel);
        auto &face = projected[i];
        auto x0 = cell(face.minU, minimumU, spanU), x1 = cell(face.maxU, minimumU, spanU);
        auto y0 = cell(face.minV, minimumV, spanV), y1 = cell(face.maxV, minimumV, spanV);
        for (auto y = y0; y <= y1; ++y)
            for (auto x = x0; x <= x1; ++x)
            {
                auto &bin = bins[y * grid + x];
                for (auto previous : bin)
                {
                    if (seen[previous] == i)
                        continue;
                    seen[previous] = i;
                    if (++comparisons > 20000000)
                        throw std::runtime_error("Automatic UV overlap check budget exceeded");
                    if (overlapsInterior(face, projected[previous]))
                        return false;
                }
                if (++binEntries > 4000000)
                    throw std::runtime_error("Automatic UV overlap grid budget exceeded");
                bin.push_back(i);
            }
    }
    return true;
}
bool pack(std::vector<Island> &islands, unsigned resolution, unsigned margin, double density)
{
    std::vector<std::size_t> order(islands.size());
    for (std::size_t i = 0; i < islands.size(); ++i)
    {
        order[i] = i;
        auto &island = islands[i];
        island.width = std::max(2 * margin + 4,
            unsigned(std::ceil((island.maxU - island.minU) * density)) + 2 * margin + 2);
        island.height = std::max(2 * margin + 4,
            unsigned(std::ceil((island.maxV - island.minV) * density)) + 2 * margin + 2);
        if (island.width > resolution || island.height > resolution)
            return false;
    }
    std::sort(order.begin(), order.end(), [&](auto a, auto b)
    {
        if (islands[a].height != islands[b].height)
            return islands[a].height > islands[b].height;
        if (islands[a].width != islands[b].width)
            return islands[a].width > islands[b].width;
        return a < b;
    });
    unsigned x = 0, y = 0, rowHeight = 0;
    for (auto index : order)
    {
        auto &island = islands[index];
        if (x + island.width > resolution)
        {
            y += rowHeight;
            x = rowHeight = 0;
        }
        if (y + island.height > resolution)
            return false;
        island.x = x;
        island.y = y;
        x += island.width;
        rowHeight = std::max(rowHeight, island.height);
    }
    return true;
}
} // namespace

unsigned requiredUvMarginPixels(const AutoUvSettings &settings)
{
    if (settings.resolution < 64 || settings.resolution > 4096 || settings.mipCount < 1 ||
        settings.mipCount > 12 || settings.bakeDilationPixels > 64 ||
        settings.minimumMipSafetyPixels < 1 || settings.minimumMipSafetyPixels > 16 ||
        settings.compressionBlockSafetyPixels < 4 || settings.compressionBlockSafetyPixels > 64 ||
        !std::isfinite(settings.seamAngleDegrees) || settings.seamAngleDegrees < 1 ||
        settings.seamAngleDegrees > 89)
        throw std::runtime_error("Automatic UV settings outside limits");
    auto margin = std::max({settings.bakeDilationPixels,
                            (1U << (settings.mipCount - 1)) * settings.minimumMipSafetyPixels,
                            settings.compressionBlockSafetyPixels});
    if (2 * margin + 4 > settings.resolution)
        throw std::runtime_error("Automatic UV margin leaves no usable atlas area");
    return margin;
}

AutoUvPlan planAutomaticUv(const TriangleMesh &mesh, const AutoUvSettings &settings,
                           const Jerico::CancellationToken *cancel)
{
    check(cancel);
    auto margin = requiredUvMarginPixels(settings);
    auto count = mesh.indices.size() / 3;
    if (mesh.indices.empty() || mesh.indices.size() % 3 || mesh.indices.size() > 720000 ||
        mesh.positions.empty() || mesh.positions.size() > 240000 ||
        mesh.triangleRegions.size() != count)
        throw std::runtime_error("Automatic UV mesh shape or budget invalid");
    for (auto p : mesh.positions)
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
            throw std::runtime_error("Automatic UV position non-finite");
    std::vector<Face> faces(count);
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::vector<std::size_t>> edges;
    for (std::size_t t = 0; t < count; ++t)
    {
        if ((t & 1023) == 0)
            check(cancel);
        auto a = mesh.indices[3 * t], b = mesh.indices[3 * t + 1], c = mesh.indices[3 * t + 2];
        if (a >= mesh.positions.size() || b >= mesh.positions.size() || c >= mesh.positions.size())
            throw std::runtime_error("Automatic UV index out of range");
        auto e1 = subtract(mesh.positions[b], mesh.positions[a]);
        auto e2 = subtract(mesh.positions[c], mesh.positions[a]);
        auto n = cross(e1, e2);
        auto length = std::sqrt(dot(n, n));
        if (!std::isfinite(length) ||
            !(length > 1e-12 * std::max(dot(e1, e1), dot(e2, e2))))
            throw std::runtime_error("Automatic UV degenerate face");
        n = {n.x / length, n.y / length, n.z / length};
        faces[t] = {n, dominantAxis(n)};
        for (auto [i, j] : {std::pair{a, b}, std::pair{b, c}, std::pair{c, a}})
            edges[{std::min(i, j), std::max(i, j)}].push_back(t);
    }
    std::vector<std::vector<std::size_t>> neighbors(count);
    auto angleLimit = std::cos(settings.seamAngleDegrees * std::numbers::pi / 180);
    for (const auto &[edge, incident] : edges)
    {
        if (incident.size() > 2)
            throw std::runtime_error("Automatic UV non-manifold edge");
        if (incident.size() != 2)
            continue;
        auto a = incident[0], b = incident[1];
        if (faces[a].axis == faces[b].axis &&
            mesh.triangleRegions[a] == mesh.triangleRegions[b] &&
            dot(faces[a].normal, faces[b].normal) >= angleLimit)
        {
            neighbors[a].push_back(b);
            neighbors[b].push_back(a);
        }
    }
    std::vector<std::uint32_t> islandOf(count, std::numeric_limits<std::uint32_t>::max());
    std::vector<Island> islands;
    for (std::size_t t = 0; t < count; ++t)
    {
        if (islandOf[t] != std::numeric_limits<std::uint32_t>::max())
            continue;
        check(cancel);
        auto id = std::uint32_t(islands.size());
        islands.push_back({});
        auto &island = islands.back();
        island.axis = faces[t].axis;
        std::queue<std::size_t> queue;
        queue.push(t);
        islandOf[t] = id;
        while (!queue.empty())
        {
            auto current = queue.front();
            queue.pop();
            island.faces.push_back(current);
            for (int k = 0; k < 3; ++k)
            {
                auto uv = project(mesh.positions[mesh.indices[3 * current + k]], island.axis);
                island.minU = std::min(island.minU, uv[0]);
                island.minV = std::min(island.minV, uv[1]);
                island.maxU = std::max(island.maxU, uv[0]);
                island.maxV = std::max(island.maxV, uv[1]);
            }
            for (auto adjacent : neighbors[current])
                if (islandOf[adjacent] == std::numeric_limits<std::uint32_t>::max())
                {
                    islandOf[adjacent] = id;
                    queue.push(adjacent);
                }
        }
    }
    std::deque<Island> pending;
    for (auto &island : islands)
        pending.push_back(std::move(island));
    islands.clear();
    std::vector<unsigned> remainderMark(count);
    unsigned epoch = 0;
    auto appendFace = [&](Island &island, std::size_t face)
    {
        island.faces.push_back(face);
        for (int k = 0; k < 3; ++k)
        {
            auto uv = project(mesh.positions[mesh.indices[3 * face + k]], island.axis);
            island.minU = std::min(island.minU, uv[0]);
            island.minV = std::min(island.minV, uv[1]);
            island.maxU = std::max(island.maxU, uv[0]);
            island.maxV = std::max(island.maxV, uv[1]);
        }
    };
    auto splitIsland = [&](Island island)
    {
        if (island.faces.size() < 2)
            throw std::runtime_error("Automatic UV single-face overlap is impossible");
        auto middle = island.faces.size() / 2;
        Island prefix;
        prefix.axis = island.axis;
        for (std::size_t i = 0; i < middle; ++i)
            appendFace(prefix, island.faces[i]);
        pending.push_back(std::move(prefix));
        if (++epoch == 0)
            throw std::runtime_error("Automatic UV split epoch exhausted");
        for (std::size_t i = middle; i < island.faces.size(); ++i)
            remainderMark[island.faces[i]] = epoch;
        for (std::size_t i = middle; i < island.faces.size(); ++i)
        {
            auto seed = island.faces[i];
            if (remainderMark[seed] != epoch)
                continue;
            Island component;
            component.axis = island.axis;
            std::queue<std::size_t> queue;
            queue.push(seed);
            remainderMark[seed] = 0;
            while (!queue.empty())
            {
                auto face = queue.front();
                queue.pop();
                appendFace(component, face);
                for (auto adjacent : neighbors[face])
                    if (remainderMark[adjacent] == epoch)
                    {
                        remainderMark[adjacent] = 0;
                        queue.push(adjacent);
                    }
            }
            pending.push_back(std::move(component));
        }
    };
    auto acceptPending = [&]
    {
        while (!pending.empty())
        {
            check(cancel);
            auto island = std::move(pending.front());
            pending.pop_front();
            if (validateIslandOverlap(mesh, island, cancel))
                islands.push_back(std::move(island));
            else
                splitIsland(std::move(island));
        }
        std::sort(islands.begin(), islands.end(), [](const Island &a, const Island &b)
        {
            return *std::min_element(a.faces.begin(), a.faces.end()) <
                   *std::min_element(b.faces.begin(), b.faces.end());
        });
        std::fill(islandOf.begin(), islandOf.end(), std::numeric_limits<std::uint32_t>::max());
        for (std::size_t id = 0; id < islands.size(); ++id)
            for (auto face : islands[id].faces)
                islandOf[face] = std::uint32_t(id);
    };
    for (std::size_t floatSplitPasses = 0; floatSplitPasses <= count; ++floatSplitPasses)
    {
        acceptPending();
        if (!pack(islands, settings.resolution, margin, 0))
            throw std::runtime_error("Automatic UV islands cannot fit even at minimum chart size");
        double maxExtent = 0;
        for (const auto &island : islands)
            maxExtent = std::max({maxExtent, island.maxU - island.minU,
                                 island.maxV - island.minV});
        if (!(maxExtent > 0))
            throw std::runtime_error("Automatic UV projected extent is zero");
        double low = 0, high = settings.resolution / maxExtent;
        for (int i = 0; i < 28; ++i)
        {
            check(cancel);
            auto middle = (low + high) * 0.5;
            if (pack(islands, settings.resolution, margin, middle))
                low = middle;
            else
                high = middle;
        }
        if (!pack(islands, settings.resolution, margin, low))
            throw std::runtime_error("Automatic UV pack lost its accepted scale");
        AutoUvPlan plan;
        plan.cornerUv.resize(mesh.indices.size());
        plan.triangleIsland = islandOf;
        plan.islandCount = unsigned(islands.size());
        plan.marginPixels = margin;
        plan.pixelsPerMeter = low;
        double activePixels = 0;
        for (std::size_t t = 0; t < count; ++t)
        {
            auto &island = islands[plan.triangleIsland[t]];
            std::array<std::array<double, 2>, 3> uv;
            for (int k = 0; k < 3; ++k)
            {
                auto point = project(mesh.positions[mesh.indices[3 * t + k]], island.axis);
                auto x = island.x + margin + 1 + (point[0] - island.minU) * low;
                auto y = island.y + margin + 1 + (point[1] - island.minV) * low;
                uv[k] = {x, y};
                plan.cornerUv[3 * t + k] = {float(x / settings.resolution),
                                            float(1 - y / settings.resolution)};
            }
            auto area2 = (uv[1][0] - uv[0][0]) * (uv[2][1] - uv[0][1]) -
                         (uv[1][1] - uv[0][1]) * (uv[2][0] - uv[0][0]);
            if (!validProjectedTriangle(uv))
                throw std::runtime_error("Automatic UV collapsed packed triangle " +
                                         std::to_string(t) + " on axis " +
                                         std::to_string(island.axis) + " area2=" +
                                         std::to_string(area2));
            if (std::abs(area2) < 2)
                ++plan.subpixelTriangles;
            activePixels += std::abs(area2) * 0.5;
        }
        plan.activeAreaFraction = activePixels / (double(settings.resolution) * settings.resolution);
        bool split = false;
        for (std::size_t i = 0; i < islands.size(); ++i)
            if (!validateIslandOverlap(mesh, islands[i], cancel, &plan.cornerUv))
            {
                auto failed = std::move(islands[i]);
                islands.erase(islands.begin() + i);
                splitIsland(std::move(failed));
                split = true;
                break;
            }
        if (!split)
        {
            // Raster-space coverage and filtered-mip validation are separate bake gates.
            return plan;
        }
    }
    throw std::runtime_error("Automatic UV float overlap splitting exceeded face budget");
}
} // namespace Playground
