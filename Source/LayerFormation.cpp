#include "LayerFormation.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/ProceduralAuthoring/Geology/GeologyNoise.hpp>
#include <TriangleBvh.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <limits>
#include <set>
#include <stdexcept>

namespace Playground
{
namespace D = PA::Detail;
namespace
{
Vec3d cross(Vec3d a, Vec3d b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3d unit(Vec3d p)
{
    double length = std::sqrt(D::lengthSquared(p));
    return length > 1e-12 ? D::multiply(p, 1 / length) : Vec3d{0, 1, 0};
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
struct ContactPoint
{
    double u{}, v{}, height{};
};
struct ContactFace
{
    std::array<ContactPoint, 3> points;
    double minU{}, maxU{}, minV{}, maxV{};
};
double orient(ContactPoint a, ContactPoint b, ContactPoint c)
{
    return (b.u - a.u) * (c.v - a.v) - (b.v - a.v) * (c.u - a.u);
}
ContactPoint lerp(ContactPoint a, ContactPoint b, double t)
{
    return {a.u + (b.u - a.u) * t, a.v + (b.v - a.v) * t,
            a.height + (b.height - a.height) * t};
}
double minimumClearance(const ContactFace &upper, const ContactFace &lower)
{
    std::vector<ContactPoint> polygon(upper.points.begin(), upper.points.end());
    const auto orientation = orient(lower.points[0], lower.points[1], lower.points[2]);
    if (std::abs(orientation) <= 1e-18)
        return std::numeric_limits<double>::infinity();
    const auto winding = orientation > 0 ? 1.0 : -1.0;
    for (int edge = 0; edge < 3 && !polygon.empty(); ++edge)
    {
        std::vector<ContactPoint> clipped;
        auto a = lower.points[edge], b = lower.points[(edge + 1) % 3];
        for (std::size_t i = 0; i < polygon.size(); ++i)
        {
            auto first = polygon[i], second = polygon[(i + 1) % polygon.size()];
            double f = winding * orient(a, b, first), s = winding * orient(a, b, second);
            bool insideFirst = f >= -1e-12, insideSecond = s >= -1e-12;
            if (insideFirst != insideSecond)
                clipped.push_back(lerp(first, second, std::clamp(f / (f - s), 0.0, 1.0)));
            if (insideSecond)
                clipped.push_back(second);
        }
        polygon = std::move(clipped);
    }
    double overlapTwiceArea = 0;
    for (std::size_t i = 2; i < polygon.size(); ++i)
        overlapTwiceArea += orient(polygon[0], polygon[i - 1], polygon[i]);
    if (std::abs(overlapTwiceArea) <= std::abs(orientation) * 1e-10)
        return std::numeric_limits<double>::infinity();
    double clearance = std::numeric_limits<double>::infinity();
    for (auto p : polygon)
    {
        auto a = lower.points[0], b = lower.points[1], c = lower.points[2];
        auto u = orient(a, p, c) / orientation;
        auto v = orient(a, b, p) / orientation;
        auto lowerHeight = a.height + u * (b.height - a.height) + v * (c.height - a.height);
        clearance = std::min(clearance, p.height - lowerHeight);
    }
    return clearance;
}
} // namespace
void layerEdgeDistances(MeshResult &mesh, const Layer &layer, const Jerico::CancellationToken *cancel)
{
    if (mesh.layerIndices.size() != mesh.mesh.positions.size())
        throw std::runtime_error("Edge breakup requires layer formation upstream");
    if (mesh.layerIndices.empty())
        throw std::runtime_error("Layer metadata is empty");
    for (int k : mesh.layerIndices)
        if (k < 0 || k >= 12)
            throw std::runtime_error("Layer index outside 0..11");
    auto d = unit({layer.direction[0], layer.direction[1], layer.direction[2]});
    int count = *std::max_element(mesh.layerIndices.begin(), mesh.layerIndices.end()) + 1;
    std::vector<std::vector<D::Triangle>> sides(count);
    for (std::size_t t = 0; t < mesh.mesh.indices.size(); t += 3)
    {
        if ((t % 768) == 0 && cancel && cancel->isCancellationRequested())
            throw std::runtime_error("Cancelled");
        auto ai = mesh.mesh.indices[t], bi = mesh.mesh.indices[t + 1], ci = mesh.mesh.indices[t + 2];
        auto a = mesh.mesh.positions[ai], b = mesh.mesh.positions[bi], c = mesh.mesh.positions[ci];
        auto normal = unit(cross(D::subtract(b, a), D::subtract(c, a)));
        if (std::abs(D::dot(normal, d)) >= 0.65)
            continue;
        int k = mesh.layerIndices[ai];
        if (k != mesh.layerIndices[bi] || k != mesh.layerIndices[ci])
            continue;
        auto bounds = D::emptyBounds();
        D::include(bounds, a);
        D::include(bounds, b);
        D::include(bounds, c);
        sides[k].push_back({a, b, c, D::multiply(D::add(D::add(a, b), c), 1.0 / 3), bounds,
                            std::uint32_t(t / 3), std::uint32_t(t / 3)});
    }
    std::vector<std::unique_ptr<D::TriangleBvh>> trees(count);
    for (int k = 0; k < count; ++k)
        if (!sides[k].empty())
            trees[k] = std::make_unique<D::TriangleBvh>(std::move(sides[k]));
    mesh.layerEdgeDistance.resize(mesh.mesh.positions.size());
    for (std::size_t i = 0; i < mesh.mesh.positions.size(); ++i)
    {
        if ((i & 255) == 0 && cancel && cancel->isCancellationRequested())
            throw std::runtime_error("Cancelled");
        auto &tree = trees[mesh.layerIndices[i]];
        if (!tree)
            throw std::runtime_error("No lateral layer boundary found; align Edge direction with strata");
        mesh.layerEdgeDistance[i] = float(tree->closest(mesh.mesh.positions[i], 1e-12).distance);
    }
}
MeshResult coherentLayerFormation(const MeshResult &source, const Layer &layer, bool showThickness,
                                  const std::vector<Vec3d> &sourceWarp, std::vector<Vec3d> &outputWarp,
                                  const Jerico::CancellationToken *cancel)
{
    const auto count = source.mesh.positions.size();
    if (count * layer.count > 240000)
        throw std::runtime_error("Layer formation exceeds 240,000 vertices");
    auto d = unit({layer.direction[0], layer.direction[1], layer.direction[2]});
    auto u = unit(cross(std::abs(d.y) < 0.9 ? Vec3d{0, 1, 0} : Vec3d{1, 0, 0}, d)), v = cross(d, u);
    double lo = 1e20, hi = -1e20, minU = 1e20, maxU = -1e20;
    double minV = 1e20, maxV = -1e20;
    for (auto p : source.mesh.positions)
    {
        const auto height = D::dot(p, d);
        lo = std::min(lo, height);
        hi = std::max(hi, height);
        minU = std::min(minU, D::dot(p, u));
        maxU = std::max(maxU, D::dot(p, u));
        minV = std::min(minV, D::dot(p, v));
        maxV = std::max(maxV, D::dot(p, v));
    }
    if (hi - lo < 1e-6)
        throw std::runtime_error("Layer source has no thickness along its direction");
    const auto centerHeight = 0.5 * (lo + hi);
    const auto planarCenter = D::add(D::multiply(u, 0.5 * (minU + maxU)),
                                     D::multiply(v, 0.5 * (minV + maxV)));
    const auto sourceCenter = D::add(planarCenter, D::multiply(d, centerHeight));
    double maximumRadius = 0;
    for (auto p : source.mesh.positions)
    {
        auto planar = D::subtract(D::subtract(p, D::multiply(d, D::dot(p, d))), planarCenter);
        maximumRadius = std::max(maximumRadius, std::sqrt(D::lengthSquared(planar)));
    }
    MeshResult result;
    result.hardRegions = source.hardRegions;
    result.sourceSelection=source.sourceSelection;result.sourceRevision=source.sourceRevision;
    result.regions = source.regions;
    result.mesh.positions.resize(count * layer.count);
    result.field.resize(count * layer.count);
    result.layerThickness.resize(count * layer.count);
    result.layerIndices.resize(count * layer.count);
    outputWarp.resize(count * layer.count);
    std::array<Vec3d, 12> offsets;
    std::array<double, 12> angles{}, scales{};
    std::array<double, 4> state{};
    Vec3d bias{layer.driftBias[0], layer.driftBias[1], layer.driftBias[2]};
    bias = D::subtract(bias, D::multiply(d, D::dot(bias, d)));
    for (int k = 0; k < layer.count; ++k)
    {
        for (int channel = 0; channel < 4; ++channel)
        {
            double innovation = 2 * random(layer.seed + k * 133 + 7 + channel * 2) - 1;
            state[channel] =
                k == 0 ? innovation
                       : layer.driftCorrelation * state[channel] + (1 - layer.driftCorrelation) * innovation;
        }
        offsets[k] =
            D::add(D::add(D::multiply(u, layer.lateral * state[0]), D::multiply(v, layer.lateral * state[1])),
                   D::multiply(bias, layer.lateral * (double(k) / std::max(1, layer.count - 1) - 0.5)));
        angles[k] = state[2] * layer.rotationDrift * 3.141592653589793 / 180;
        scales[k] = std::max(0.15, 1.0 + k * layer.layerScale + state[3] * layer.scaleDrift);
    }
    // Adjacent coherent strata must evaluate the same planar frame at their shared horizon.
    // Interpolate each slab between boundary frames instead of transforming whole slabs independently.
    std::array<Vec3d, 13> horizonOffsets{};
    std::array<double, 13> horizonAngles{}, horizonScales{};
    for (int boundary = 0; boundary <= layer.count; ++boundary)
    {
        const int lower = std::clamp(boundary - 1, 0, layer.count - 1);
        const int upper = std::clamp(boundary, 0, layer.count - 1);
        horizonOffsets[boundary] = D::multiply(D::add(offsets[lower], offsets[upper]), 0.5);
        horizonAngles[boundary] = 0.5 * (angles[lower] + angles[upper]);
        horizonScales[boundary] = 0.5 * (scales[lower] + scales[upper]);
    }
    std::array<std::array<double, 3>, 13> macroAxes{};
    for (int boundary = 0; boundary <= layer.count; ++boundary)
    {
        if (layer.macroAxisAsymmetry == 0)
        {
            macroAxes[boundary] = {1, 0, 1};
            continue;
        }
        const auto commonPhase = 2 * 3.141592653589793 * random(layer.seed + 0x5e71acU);
        const auto localPhase = 2 * 3.141592653589793 *
            random(layer.seed + 0xa571acU + std::uint32_t(boundary) * 1907U);
        const auto phase = layer.profile == 2
            ? commonPhase + 0.1 * std::remainder(localPhase - commonPhase,
                                                  2 * 3.141592653589793)
            : commonPhase + 0.25 * (2 * random(layer.seed + 0xa571acU +
                                                 std::uint32_t(boundary) * 1907U) - 1);
        const auto c = std::cos(phase), s = std::sin(phase);
        const auto asymmetry = layer.profile == 2 ? 0.5 * layer.macroAxisAsymmetry
                                                  : layer.macroAxisAsymmetry;
        const auto stretch = 1 + 0.12 * asymmetry *
                                random(layer.seed + 0x62ad18U + std::uint32_t(boundary) * 1907U);
        const auto squash = 1 - asymmetry *
                                (0.8 + 0.2 * random(layer.seed + 0x71d88aU +
                                                    std::uint32_t(boundary) * 1907U));
        macroAxes[boundary] = {stretch * c * c + squash * s * s,
                               (stretch - squash) * c * s,
                               stretch * s * s + squash * c * c};
    }
    auto heights = [&](Vec3d p)
    {
        auto local = D::subtract(p, sourceCenter);
        Vec3d q{D::dot(local, u) * layer.thicknessFrequency, 0,
                D::dot(local, v) * layer.thicknessFrequency};
        double shared = PA::evaluateGeologyFbmReference(q, layer.seed + 0x58e571U, 2,
                                                         layer.lacunarity, 0.5);
        std::array<double, 12> values{};
        for (int k = 0; k < layer.count; ++k)
        {
            double local = PA::evaluateGeologyFbmReference(q, layer.seed + k * 71 + 0xa8192U, 2,
                                                            layer.lacunarity, 0.5);
            double signal = layer.thicknessCorrelation * shared + (1 - layer.thicknessCorrelation) * local;
            values[k] = std::clamp(layer.thickness * (1 + layer.jitter * signal),
                                   double(layer.minimumThickness), double(layer.maximumThickness));
        }
        return values;
    };
    auto positions = [&](Vec3d p, const std::array<double, 12> &thickness,
                         double perimeterAmplitude)
    {
        std::array<Vec3d, 12> values{};
        double total = (layer.count - 1) * layer.spacing;
        for (int k = 0; k < layer.count; ++k)
            total += thickness[k];
        double cursor = -total * 0.5, h = D::dot(p, d);
        auto planar = D::subtract(D::subtract(p, D::multiply(d, h)), planarCenter);
        auto normalizedHeight = std::clamp((h - lo) / (hi - lo), 0.0, 1.0);
        auto radialLength = std::sqrt(D::lengthSquared(planar));
        const auto normalizedRadius = maximumRadius > 1e-12
                                          ? std::clamp(radialLength / maximumRadius, 0.0, 1.0)
                                          : 0.0;
        std::array<double, 13> boundaryOffset{};
        if (perimeterAmplitude > 0 && radialLength > 1e-12)
        {
            auto facetCount = std::clamp(int(std::lround(layer.frequency * 4)), 6, 24);
            auto angle = std::atan2(D::dot(planar, v), D::dot(planar, u));
            auto angularPosition = (angle + 3.141592653589793) * facetCount /
                                   (2 * 3.141592653589793);
            auto facet = int(std::floor(angularPosition)) % facetCount;
            auto facetFraction = angularPosition - std::floor(angularPosition);
            auto first = 2 * random(layer.seed + 0x328a1e77U + std::uint32_t(facet) * 4127U) - 1;
            auto next = 2 * random(layer.seed + 0x328a1e77U +
                                   std::uint32_t((facet + 1) % facetCount) * 4127U) - 1;
            auto sharedFacet = first + (next - first) * facetFraction;
            for (int boundary = 1; boundary < layer.count; ++boundary)
            {
                auto facetSeed = layer.seed + 0xa73e29d1U + std::uint32_t(boundary) * 1907U;
                if (layer.profile == 2)
                {
                    // Each horizon has its own low-frequency fracture polygon. Its two
                    // neighboring slabs still evaluate this same boundary, so contact is exact.
                    const auto localCount = 7 + int(random(facetSeed + 0x513cU) * 6);
                    const auto phase = random(facetSeed + 0x851dU);
                    const auto position = (angle + 3.141592653589793) * localCount /
                                              (2 * 3.141592653589793) + phase;
                    const auto localIndex = int(std::floor(position)) % localCount;
                    const auto t = position - std::floor(position);
                    const auto a = 2 * random(facetSeed + std::uint32_t(localIndex) * 4127U) - 1;
                    const auto b = 2 * random(facetSeed +
                                              std::uint32_t((localIndex + 1) % localCount) * 4127U) - 1;
                    boundaryOffset[boundary] = perimeterAmplitude *
                        (0.12 * sharedFacet + 0.88 * (a + (b - a) * t));
                }
                else
                {
                    auto localFirst = 2 * random(facetSeed + std::uint32_t(facet) * 4127U) - 1;
                    auto localNext = 2 * random(facetSeed +
                                                std::uint32_t((facet + 1) % facetCount) * 4127U) - 1;
                    auto localFacet = localFirst + (localNext - localFirst) * facetFraction;
                    boundaryOffset[boundary] = perimeterAmplitude *
                        (0.35 * sharedFacet + 0.65 * localFacet);
                }
            }
            if (layer.shapeOuterCaps)
            {
                // Exposed ends weather inward relative to their neighboring contact horizons.
                boundaryOffset[0] = std::min(0.0, boundaryOffset[1]) - 0.05 * perimeterAmplitude;
                boundaryOffset[layer.count] =
                    std::min(0.0, boundaryOffset[layer.count - 1]) - 0.05 * perimeterAmplitude;
            }
        }
        for (int k = 0; k < layer.count; ++k)
        {
            auto offset = boundaryOffset[k] * (1 - normalizedHeight) +
                          boundaryOffset[k + 1] * normalizedHeight;
            // Keep the angular perimeter deformation at the rim; a full-strength angular
            // displacement around the center can invert the source cap triangulation.
            offset *= normalizedRadius * normalizedRadius;
            double relief = 0;
            if (perimeterAmplitude > 0 && radialLength > 1e-12)
            {
                auto envelope = std::sin(3.141592653589793 * normalizedHeight);
                Vec3d q{D::dot(planar, u) * layer.frequency * 2.5,
                        (h - centerHeight) * layer.frequency * 2.5,
                        D::dot(planar, v) * layer.frequency * 2.5};
                relief = perimeterAmplitude * 0.22 * envelope * envelope *
                         PA::evaluateGeologyFbmReference(q, layer.seed + 0xd811a7U +
                                                         std::uint32_t(k) * 509U,
                                                         layer.octaves, layer.lacunarity,
                                                         layer.roughness);
            }
            const auto blend = [&](double lower, double upper)
            {
                return lower * (1 - normalizedHeight) + upper * normalizedHeight;
            };
            auto macroPlanar = planar;
            if (layer.macroAxisAsymmetry > 0)
            {
                const auto x = D::dot(planar, u), y = D::dot(planar, v);
                const auto xx = blend(macroAxes[k][0], macroAxes[k + 1][0]);
                const auto xy = blend(macroAxes[k][1], macroAxes[k + 1][1]);
                const auto yy = blend(macroAxes[k][2], macroAxes[k + 1][2]);
                macroPlanar = D::add(D::multiply(u, xx * x + xy * y),
                                     D::multiply(v, xy * x + yy * y));
            }
            auto side = D::multiply(macroPlanar, 1 + offset + relief);
            auto angle = blend(horizonAngles[k], horizonAngles[k + 1]);
            auto scale = blend(horizonScales[k], horizonScales[k + 1]);
            auto shift = D::add(D::multiply(horizonOffsets[k], 1 - normalizedHeight),
                                D::multiply(horizonOffsets[k + 1], normalizedHeight));
            auto rotated = D::add(D::multiply(side, std::cos(angle)),
                                  D::multiply(cross(d, side), std::sin(angle)));
            values[k] = D::add(D::add(D::add(planarCenter, D::multiply(rotated, scale)), shift),
                               D::multiply(d, centerHeight + cursor +
                                                  thickness[k] * (h - lo) / (hi - lo)));
            if (layer.beddingTilt > 0)
            {
                const auto point = values[k];
                const auto height = D::dot(point, d) - centerHeight;
                const auto horizontal = D::subtract(D::subtract(point, D::multiply(d, D::dot(point, d))),
                                                    planarCenter);
                const auto x = D::dot(horizontal, u), y = D::dot(horizontal, v);
                const auto seedAngle = double(layer.seed % 1000) * 0.019;
                const auto angle = seedAngle +
                                   0.35 * std::sin(1.7 * height / std::max(1.0, maximumRadius) + seedAngle);
                const auto slope = x * std::cos(angle) + y * std::sin(angle);
                const auto undulation = 0.2 * PA::evaluateGeologyFbmReference(
                    {x * 0.75, 0, y * 0.75}, layer.seed + 0xbedd1a7U, 2,
                    layer.lacunarity, 0.5);
                values[k] = D::add(point, D::multiply(d, layer.beddingTilt * (slope + undulation)));
            }
            cursor += thickness[k] + layer.spacing;
        }
        return values;
    };
    for (std::size_t i = 0; i < count; ++i)
    {
        if ((i & 255) == 0 && cancel && cancel->isCancellationRequested())
            throw std::runtime_error("Cancelled");
        auto thickness = heights(source.mesh.positions[i]);
        auto p = positions(source.mesh.positions[i], thickness, layer.perimeterAmplitude);
        std::array<Vec3d, 12> warped = p;
        if (D::lengthSquared(sourceWarp[i]) > 0)
        {
            auto q = D::add(source.mesh.positions[i], sourceWarp[i]);
            warped = positions(q, heights(q), layer.perimeterAmplitude);
        }
        for (int k = 0; k < layer.count; ++k)
        {
            auto at = count * k + i;
            result.mesh.positions[at] = p[k];
            result.layerIndices[at] = k;
            result.layerThickness[at] = float(thickness[k]);
            result.field[at] = showThickness ? float(thickness[k]) : source.field[i];
            outputWarp[at] = D::subtract(warped[k], p[k]);
        }
    }
    std::vector<unsigned char> sourceContacts;
    sourceContacts.reserve(source.mesh.indices.size() / 3);
    for (std::size_t t = 0; t < source.mesh.indices.size(); t += 3)
    {
        auto cap = [&](double height)
        {
            for (int corner = 0; corner < 3; ++corner)
                if (std::abs(D::dot(source.mesh.positions[source.mesh.indices[t + corner]], d) - height) > 1e-8)
                    return false;
            return true;
        };
        sourceContacts.push_back(cap(hi) ? 1 : cap(lo) ? 2 : 0);
    }
    if (std::find(sourceContacts.begin(), sourceContacts.end(), 1) == sourceContacts.end() ||
        std::find(sourceContacts.begin(), sourceContacts.end(), 2) == sourceContacts.end())
        sourceContacts.clear();
    if (layer.profile == 2 && layer.count > 1 && !sourceContacts.empty())
    {
        using Point = std::array<long long, 2>;
        using Triangle = std::array<Point, 3>;
        std::multiset<Triangle> top, bottom;
        for (std::size_t t = 0; t < sourceContacts.size(); ++t)
        {
            if (sourceContacts[t] == 0)
                continue;
            Triangle triangle;
            for (int corner = 0; corner < 3; ++corner)
            {
                const auto p = source.mesh.positions[source.mesh.indices[3 * t + corner]];
                triangle[corner] = {std::llround(D::dot(p, u) * 1e8),
                                    std::llround(D::dot(p, v) * 1e8)};
            }
            std::sort(triangle.begin(), triangle.end());
            (sourceContacts[t] == 1 ? top : bottom).insert(triangle);
        }
        if (top != bottom)
            throw std::runtime_error("Source cap triangulation cannot form exact shared horizons");
    }
    for (int k = 0; k < layer.count; ++k)
    {
        for (auto i : source.mesh.indices)
            result.mesh.indices.push_back(std::uint32_t(k * count + i));
        result.layerContactFaces.insert(result.layerContactFaces.end(), sourceContacts.begin(),
                                        sourceContacts.end());
        std::set<Jerico::Guid> recorded;
        for (auto id : source.mesh.triangleRegions)
        {
            auto stratum = Solid::region(layer.seed, Jerico::stableGuidHash(id) ^ (0x20000 + k));
            result.mesh.triangleRegions.push_back(stratum);
            if (recorded.insert(stratum).second)
                result.regions.push_back({stratum, id, SurfaceRole::Stratum, -1, k});
        }
    }
    if (layer.perimeterAmplitude > 0 && !sourceContacts.empty())
    {
        std::vector<Vec3d> referencePositions;
        auto contactFacesValid = [&](double factor)
        {
            auto at = [&](std::size_t index)
            {
                const auto &full = result.mesh.positions[index];
                return referencePositions.empty()
                    ? full
                    : D::add(referencePositions[index],
                             D::multiply(D::subtract(full, referencePositions[index]), factor));
            };
            for (int k = 0; k < layer.count; ++k)
                for (std::size_t t = 0; t < sourceContacts.size(); ++t)
                {
                    if ((t & 255) == 0 && cancel && cancel->isCancellationRequested())
                        throw std::runtime_error("Cancelled");
                    if (sourceContacts[t] == 0)
                        continue;
                    const auto ia = source.mesh.indices[3 * t];
                    const auto ib = source.mesh.indices[3 * t + 1];
                    const auto ic = source.mesh.indices[3 * t + 2];
                    const auto a = at(k * count + ia), b = at(k * count + ib),
                               c = at(k * count + ic);
                    const auto area = D::dot(cross(D::subtract(b, a), D::subtract(c, a)), d);
                    const auto sourceA = source.mesh.positions[ia];
                    const auto sourceB = source.mesh.positions[ib];
                    const auto sourceC = source.mesh.positions[ic];
                    const auto sourceArea = D::dot(cross(D::subtract(sourceB, sourceA),
                                                         D::subtract(sourceC, sourceA)), d);
                    if (area * (sourceContacts[t] == 1 ? 1 : -1) <=
                        std::abs(sourceArea) * 1e-4)
                        return false;
                }
            return true;
        };
        if (!contactFacesValid(1))
        {
            referencePositions.resize(result.mesh.positions.size());
            std::vector<Vec3d> referenceWarp(outputWarp.size());
            for (std::size_t i = 0; i < count; ++i)
            {
                if ((i & 255) == 0 && cancel && cancel->isCancellationRequested())
                    throw std::runtime_error("Cancelled");
                const auto thickness = heights(source.mesh.positions[i]);
                const auto rest = positions(source.mesh.positions[i], thickness, 0);
                auto restWarped = rest;
                if (D::lengthSquared(sourceWarp[i]) > 0)
                {
                    const auto q = D::add(source.mesh.positions[i], sourceWarp[i]);
                    restWarped = positions(q, heights(q), 0);
                }
                for (int k = 0; k < layer.count; ++k)
                {
                    const auto index = k * count + i;
                    referencePositions[index] = rest[k];
                    referenceWarp[index] = D::subtract(restWarped[k], rest[k]);
                }
            }
            if (!contactFacesValid(0))
                throw std::runtime_error("Layer contact faces fold without perimeter displacement");
            struct AreaCurve { double constant, linear, quadratic; };
            std::vector<AreaCurve> contactAreas;
            for (int k = 0; k < layer.count; ++k)
                for (std::size_t t = 0; t < sourceContacts.size(); ++t)
                {
                    if ((t & 255) == 0 && cancel && cancel->isCancellationRequested())
                        throw std::runtime_error("Cancelled");
                    if (sourceContacts[t] == 0)
                        continue;
                    const auto ia = source.mesh.indices[3 * t];
                    const auto ib = source.mesh.indices[3 * t + 1];
                    const auto ic = source.mesh.indices[3 * t + 2];
                    const auto a = k * count + ia, b = k * count + ib, c = k * count + ic;
                    const auto edge = D::subtract(referencePositions[b], referencePositions[a]);
                    const auto side = D::subtract(referencePositions[c], referencePositions[a]);
                    const auto moveA = D::subtract(result.mesh.positions[a], referencePositions[a]);
                    const auto moveB = D::subtract(result.mesh.positions[b], referencePositions[b]);
                    const auto moveC = D::subtract(result.mesh.positions[c], referencePositions[c]);
                    const auto edgeChange = D::subtract(moveB, moveA);
                    const auto sideChange = D::subtract(moveC, moveA);
                    const auto sourceEdge = D::subtract(source.mesh.positions[ib], source.mesh.positions[ia]);
                    const auto sourceSide = D::subtract(source.mesh.positions[ic], source.mesh.positions[ia]);
                    const auto threshold = 1e-4 * std::abs(D::dot(cross(sourceEdge, sourceSide), d));
                    const auto sign = sourceContacts[t] == 1 ? 1.0 : -1.0;
                    contactAreas.push_back({
                        sign * D::dot(cross(edge, side), d) - threshold,
                        sign * D::dot(D::add(cross(edgeChange, side),
                                             cross(edge, sideChange)), d),
                        sign * D::dot(cross(edgeChange, sideChange), d)});
                }
            auto validThrough = [&](double factor)
            {
                if (cancel && cancel->isCancellationRequested())
                    throw std::runtime_error("Cancelled");
                for (const auto &area : contactAreas)
                {
                    auto value = [&](double t)
                    {
                        return (area.quadratic * t + area.linear) * t + area.constant;
                    };
                    if (!(area.constant > 0) || !(value(factor) > 0))
                        return false;
                    if (area.quadratic > 0)
                    {
                        const auto minimum = -area.linear / (2 * area.quadratic);
                        if (minimum > 0 && minimum < factor && !(value(minimum) > 0))
                            return false;
                    }
                }
                return true;
            };
            double safe = 0, unsafe = 1;
            for (int iteration = 0; iteration < 20; ++iteration)
            {
                const auto candidate = 0.5 * (safe + unsafe);
                (validThrough(candidate) ? safe : unsafe) = candidate;
            }
            safe *= 0.99;
            if (!validThrough(safe))
                throw std::runtime_error("Layer perimeter cannot preserve contact orientation");
            for (std::size_t index = 0; index < result.mesh.positions.size(); ++index)
            {
                result.mesh.positions[index] = D::add(referencePositions[index],
                    D::multiply(D::subtract(result.mesh.positions[index], referencePositions[index]), safe));
                outputWarp[index] = D::add(referenceWarp[index],
                    D::multiply(D::subtract(outputWarp[index], referenceWarp[index]), safe));
            }
        }
    }
    return result;
}
std::vector<double> settleLayerStack(MeshResult &mesh, Vec3d up,
                                     const Jerico::CancellationToken *cancel)
{
    if (mesh.layerIndices.size() != mesh.mesh.positions.size() || mesh.layerIndices.empty())
        throw std::runtime_error("Layer settling requires upstream layer formation");
    auto length = std::sqrt(D::lengthSquared(up));
    if (!(length > 1e-12) || !std::isfinite(length))
        throw std::runtime_error("Layer settling requires a finite up axis");
    up = D::multiply(up, 1 / length);
    auto tangent = unit(cross(std::abs(up.y) < 0.9 ? Vec3d{0, 1, 0} : Vec3d{1, 0, 0}, up));
    auto bitangent = cross(up, tangent);
    int count = *std::max_element(mesh.layerIndices.begin(), mesh.layerIndices.end()) + 1;
    if (count < 2 || count > 12)
        throw std::runtime_error("Layer settling requires 2..12 strata");
    std::vector<std::vector<ContactFace>> tops(count), bottoms(count);
    const bool taggedContacts = mesh.layerContactFaces.size() == mesh.mesh.indices.size() / 3;
    for (std::size_t t = 0; t < mesh.mesh.indices.size(); t += 3)
    {
        if ((t & 255) == 0 && cancel && cancel->isCancellationRequested())
            throw std::runtime_error("Cancelled");
        std::array<std::uint32_t, 3> index{mesh.mesh.indices[t], mesh.mesh.indices[t + 1],
                                            mesh.mesh.indices[t + 2]};
        if (index[0] >= mesh.layerIndices.size() || index[1] >= mesh.layerIndices.size() ||
            index[2] >= mesh.layerIndices.size())
            throw std::runtime_error("Layer settling mesh index invalid");
        int k = mesh.layerIndices[index[0]];
        if (k < 0 || k >= count || mesh.layerIndices[index[1]] != k || mesh.layerIndices[index[2]] != k)
            throw std::runtime_error("Layer settling requires separate stratum triangles");
        auto a = mesh.mesh.positions[index[0]], b = mesh.mesh.positions[index[1]],
             c = mesh.mesh.positions[index[2]];
        auto normal = cross(D::subtract(b, a), D::subtract(c, a));
        auto faceLength = std::sqrt(D::lengthSquared(normal));
        if (!(faceLength > 1e-15))
            throw std::runtime_error("Layer settling requires nondegenerate faces");
        auto facing = D::dot(normal, up) / faceLength;
        if (taggedContacts)
        {
            auto tag = mesh.layerContactFaces[t / 3];
            if (tag == 0)
                continue;
            if ((tag == 1 && facing <= 0) || (tag == 2 && facing >= 0))
                throw std::runtime_error("Layer contact face is folded at triangle " +
                                         std::to_string(t / 3) + " stratum " +
                                         std::to_string(k) + " facing " +
                                         std::to_string(facing));
        }
        else if (std::abs(facing) < 0.5)
            continue;
        ContactFace face;
        for (int i = 0; i < 3; ++i)
        {
            auto p = mesh.mesh.positions[index[i]];
            face.points[i] = {D::dot(p, tangent), D::dot(p, bitangent), D::dot(p, up)};
        }
        face.minU = std::min({face.points[0].u, face.points[1].u, face.points[2].u});
        face.maxU = std::max({face.points[0].u, face.points[1].u, face.points[2].u});
        face.minV = std::min({face.points[0].v, face.points[1].v, face.points[2].v});
        face.maxV = std::max({face.points[0].v, face.points[1].v, face.points[2].v});
        (facing > 0 ? tops[k] : bottoms[k]).push_back(face);
    }
    std::vector<double> shifts(count);
    for (int k = 1; k < count; ++k)
    {
        if (tops[k - 1].empty() || bottoms[k].empty())
            throw std::runtime_error("Layer settling has no facing support faces");
        std::vector<D::Triangle> projected;
        projected.reserve(tops[k - 1].size());
        for (std::size_t i = 0; i < tops[k - 1].size(); ++i)
        {
            auto &face = tops[k - 1][i];
            auto a = Vec3d{face.points[0].u, face.points[0].v, 0};
            auto b = Vec3d{face.points[1].u, face.points[1].v, 0};
            auto c = Vec3d{face.points[2].u, face.points[2].v, 0};
            auto bounds = D::emptyBounds();
            D::include(bounds, a);
            D::include(bounds, b);
            D::include(bounds, c);
            projected.push_back({a, b, c, D::multiply(D::add(D::add(a, b), c), 1.0 / 3),
                                 bounds, std::uint32_t(i), std::uint32_t(i)});
        }
        D::TriangleBvh tree(std::move(projected));
        double gap = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < bottoms[k].size(); ++i)
        {
            if ((i & 63) == 0 && cancel && cancel->isCancellationRequested())
                throw std::runtime_error("Cancelled");
            auto &upper = bottoms[k][i];
            std::array<std::uint32_t, 128> stack{};
            std::size_t size = 1;
            while (size)
            {
                auto &node = tree.nodes()[stack[--size]];
                if (node.bounds.maximum.x < upper.minU || node.bounds.minimum.x > upper.maxU ||
                    node.bounds.maximum.y < upper.minV || node.bounds.minimum.y > upper.maxV)
                    continue;
                if (node.count == 0)
                {
                    if (size + 2 > stack.size())
                        throw std::runtime_error("Layer settling support BVH stack limit");
                    stack[size++] = node.left;
                    stack[size++] = node.right;
                    continue;
                }
                for (std::uint32_t j = 0; j < node.count; ++j)
                {
                    auto &tri = tree.triangles()[tree.order()[node.begin + j]];
                    auto &lower = tops[k - 1][tri.owner];
                    if (lower.maxU < upper.minU || lower.minU > upper.maxU ||
                        lower.maxV < upper.minV || lower.minV > upper.maxV)
                        continue;
                    gap = std::min(gap, minimumClearance(upper, lower) - shifts[k - 1]);
                }
            }
        }
        if (!std::isfinite(gap))
            throw std::runtime_error("Layer settling has no overlapping support footprint");
        shifts[k] = -gap;
    }
    for (std::size_t i = 0; i < mesh.mesh.positions.size(); ++i)
    {
        if ((i & 255) == 0 && cancel && cancel->isCancellationRequested())
            throw std::runtime_error("Cancelled");
        mesh.mesh.positions[i] = D::add(mesh.mesh.positions[i],
                                        D::multiply(up, shifts[mesh.layerIndices[i]]));
    }
    return shifts;
}
} // namespace Playground
