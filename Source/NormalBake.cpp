#include "NormalBake.hpp"
#include "AutoUv.hpp"
#include "SurfaceFeatures.hpp"
#include "SurfaceRay.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <numbers>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
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
Vec3d unit(Vec3d p)
{
    auto n = Projection::length(p);
    if (!(n > 0) || !std::isfinite(n))
        throw std::runtime_error("Normal bake has invalid normal");
    return D::multiply(p, 1 / n);
}
std::vector<D::Triangle> validateMesh(const MeshResult &source)
{
    const auto &m = source.mesh;
    if (m.positions.empty() || m.positions.size() > 240000 || m.indices.empty() || m.indices.size() % 3 ||
        m.indices.size() > 720000 || m.triangleRegions.size() != m.indices.size() / 3)
        throw std::runtime_error("Normal bake mesh shape or budget invalid");
    for (auto p : m.positions)
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
            throw std::runtime_error("Normal bake position non-finite");
    std::vector<D::Triangle> triangles;
    triangles.reserve(m.indices.size() / 3);
    for (std::size_t t = 0; t < m.indices.size(); t += 3)
    {
        for (int k = 0; k < 3; ++k)
            if (m.indices[t + k] >= m.positions.size())
                throw std::runtime_error("Normal bake index out of range");
        auto a = m.positions[m.indices[t]], b = m.positions[m.indices[t + 1]],
             c = m.positions[m.indices[t + 2]];
        auto e1 = D::subtract(b, a), e2 = D::subtract(c, a);
        auto area2 = Projection::length(Projection::cross(e1, e2));
        if (!(area2 > 1e-12 * std::max(D::lengthSquared(e1), D::lengthSquared(e2))))
            throw std::runtime_error("Normal bake near-degenerate triangle");
        auto bounds = D::emptyBounds();
        D::include(bounds, a);
        D::include(bounds, b);
        D::include(bounds, c);
        triangles.push_back({a, b, c, D::multiply(D::add(D::add(a, b), c), 1.0 / 3), bounds,
                             std::uint32_t(t / 3), std::uint32_t(t / 3)});
    }
    return triangles;
}
bool triangleTouchesPixel(const std::array<std::array<double, 2>, 3> &uv,
                          double pixelX, double pixelY)
{
    for (int axis = 0; axis < 2; ++axis)
    {
        auto center = axis == 0 ? pixelX : pixelY;
        auto low = std::min({uv[0][axis], uv[1][axis], uv[2][axis]});
        auto high = std::max({uv[0][axis], uv[1][axis], uv[2][axis]});
        if (high < center - 0.5 || low > center + 0.5)
            return false;
    }
    for (int edge = 0; edge < 3; ++edge)
    {
        const auto &a = uv[edge], &b = uv[(edge + 1) % 3];
        auto axisX = b[1] - a[1], axisY = a[0] - b[0];
        auto low = std::numeric_limits<double>::infinity();
        auto high = -low;
        for (const auto &point : uv)
        {
            auto projection = point[0] * axisX + point[1] * axisY;
            low = std::min(low, projection);
            high = std::max(high, projection);
        }
        auto center = pixelX * axisX + pixelY * axisY;
        auto radius = 0.5 * (std::abs(axisX) + std::abs(axisY));
        if (high < center - radius - 1e-8 || low > center + radius + 1e-8)
            return false;
    }
    return true;
}
std::vector<std::uint8_t> downsampleSpecialist(const std::vector<std::uint8_t> &source,
                                                unsigned size, bool regionIds,
                                                const Jerico::CancellationToken *cancel)
{
    const auto nextSize = size / 2;
    std::vector<std::uint8_t> next(std::size_t(nextSize) * nextSize * 4);
    for (unsigned y = 0; y < nextSize; ++y)
    {
        if ((y & 63) == 0)
            check(cancel);
        for (unsigned x = 0; x < nextSize; ++x)
        {
            std::array<std::uint32_t, 4> colors{};
            std::array<unsigned, 3> sums{};
            unsigned painted = 0;
            for (unsigned dy = 0; dy < 2; ++dy)
                for (unsigned dx = 0; dx < 2; ++dx)
                {
                    const auto at = (std::size_t(2 * y + dy) * size + 2 * x + dx) * 4;
                    if (!source[at + 3])
                        continue;
                    colors[painted++] = (std::uint32_t(source[at]) << 16) |
                                        (std::uint32_t(source[at + 1]) << 8) | source[at + 2];
                    for (unsigned channel = 0; channel < 3; ++channel)
                        sums[channel] += source[at + channel];
                }
            const auto at = (std::size_t(y) * nextSize + x) * 4;
            if (!painted)
                continue;
            if (regionIds)
            {
                unsigned bestCount = 0;
                std::uint32_t bestColor = 0xffffffffU;
                for (unsigned i = 0; i < painted; ++i)
                {
                    unsigned count = 0;
                    for (unsigned j = 0; j < painted; ++j)
                        count += colors[i] == colors[j];
                    if (count > bestCount || (count == bestCount && colors[i] < bestColor))
                    {
                        bestCount = count;
                        bestColor = colors[i];
                    }
                }
                next[at] = std::uint8_t(bestColor >> 16);
                next[at + 1] = std::uint8_t(bestColor >> 8);
                next[at + 2] = std::uint8_t(bestColor);
            }
            else
                for (unsigned channel = 0; channel < 3; ++channel)
                    next[at + channel] = std::uint8_t((sums[channel] + painted / 2) / painted);
            next[at + 3] = 255;
        }
    }
    return next;
}
} // namespace
NormalBake bakeNormals(const MeshResult &highInput, const MeshResult &low, const NormalBakeSettings &settings,
                       const Jerico::CancellationToken *cancel)
{
    const auto start = std::chrono::steady_clock::now();
    check(cancel);
    if (settings.resolution < 64 || settings.resolution > 4096 || settings.padding < 2 ||
        settings.padding > 16 || !std::isfinite(settings.projectionDistance) ||
        settings.projectionDistance <= 0 || settings.projectionDistance > 2 || int(settings.projection) < 0 ||
        int(settings.projection) > 1 || !std::isfinite(settings.featureRadius) || settings.featureRadius <= 0 ||
        settings.featureRadius > 100 || settings.featureSamples < 4 || settings.featureSamples > 128 ||
        (settings.contactRadius && (!std::isfinite(*settings.contactRadius) ||
                                    *settings.contactRadius <= 0 || *settings.contactRadius > 100)) ||
        (settings.ambientRadius && (!std::isfinite(*settings.ambientRadius) ||
                                    *settings.ambientRadius <= 0 || *settings.ambientRadius > 100)) ||
        !std::isfinite(settings.cavityRadius) || settings.cavityRadius <= 0 ||
        settings.cavityRadius > 100 || !std::isfinite(settings.edgeRadius) ||
        settings.edgeRadius <= 0 || settings.edgeRadius > 100 ||
        !std::isfinite(settings.wireRadius) || settings.wireRadius <= 0 ||
        settings.wireRadius > 100 ||
        !std::isfinite(settings.curvatureScale) || settings.curvatureScale <= 0 ||
        settings.curvatureScale > 100 || !std::isfinite(settings.curvatureRelaxation) ||
        settings.curvatureRelaxation < 0 || settings.curvatureRelaxation > 1 ||
        !std::isfinite(settings.thicknessRange) ||
        settings.thicknessRange <= 0 || settings.thicknessRange > 1000 ||
        !std::isfinite(settings.heightRange) || settings.heightRange <= 0 ||
        settings.heightRange > 1000)
        throw std::runtime_error("Normal bake settings outside limits");
    auto high = highInput;
    auto sourceTriangles = validateMesh(high);
    auto targets = validateMesh(low);
    if (settings.automaticUv && !settings.chartLayout.empty())
        throw std::runtime_error("Automatic UV and explicit triangle chart layout are mutually exclusive");
    if (!settings.chartLayout.empty())
    {
        if (settings.chartLayout.size() != targets.size())
            throw std::runtime_error("Normal atlas chart count does not match low triangles");
        std::vector<std::uint8_t> occupied(std::size_t(settings.resolution) * settings.resolution);
        for (const auto &chart : settings.chartLayout)
        {
            if (chart.size < 2 * settings.padding + 4 || chart.x > settings.resolution ||
                chart.y > settings.resolution || chart.size > settings.resolution - chart.x ||
                chart.size > settings.resolution - chart.y)
                throw std::runtime_error("Normal atlas chart outside usable texture bounds");
            for (unsigned y = chart.y; y < chart.y + chart.size; ++y)
                for (unsigned x = chart.x; x < chart.x + chart.size; ++x)
                {
                    auto &pixel = occupied[std::size_t(y) * settings.resolution + x];
                    if (pixel)
                        throw std::runtime_error("Normal atlas charts overlap");
                    pixel = 1;
                }
        }
    }
    AutoUvPlan automaticPlan;
    if (settings.automaticUv)
    {
        AutoUvSettings uvSettings;
        uvSettings.resolution = settings.resolution;
        uvSettings.mipCount = settings.automaticUvMipCount;
        uvSettings.bakeDilationPixels = settings.padding;
        automaticPlan = planAutomaticUv(low.mesh, uvSettings, cancel);
    }
    if (high.normals.size() != high.mesh.positions.size())
        normals(high);
    if (!high.cornerNormals.empty() && high.cornerNormals.size() != high.mesh.indices.size())
        throw std::runtime_error("Normal bake high corner normals do not match topology");
    for (auto n : high.normals)
        unit(n);
    for (auto n : high.cornerNormals)
        unit(n);
    SurfaceFeatureFields sourceFields;
    if (settings.specialistMaps)
        sourceFields = surfaceFeatures(high.mesh,
                                       {.contact = true,
                                        .ambient = true,
                                        .thickness = true,
                                        .radius = settings.contactRadius.value_or(settings.featureRadius * 0.25),
                                        .creaseAngleDegrees = 60,
                                        .contactSamples = int(settings.featureSamples),
                                        .ambientRadius = settings.ambientRadius.value_or(settings.featureRadius),
                                        .ambientSamples = int(settings.featureSamples),
                                        .thicknessRadius = settings.thicknessRange},
                                       cancel);
    std::vector<double> relaxedCurvature;
    const auto *bakeCurvature = &sourceFields.signedDihedralCurvature;
    if (settings.specialistMaps && settings.curvatureRelaxation > 0)
    {
        relaxedCurvature = relaxSignedCurvature(
            high.mesh, sourceFields.signedDihedralCurvature, settings.curvatureRelaxation, cancel);
        bakeCurvature = &relaxedCurvature;
    }
    auto columns = unsigned(std::ceil(std::sqrt(double(targets.size()))));
    auto tile = settings.resolution / columns;
    if (!settings.automaticUv && settings.chartLayout.empty() && tile < 2 * settings.padding + 4)
        throw std::runtime_error("Normal atlas too small: reduce low mesh or increase texture resolution");
    D::TriangleBvh bvh(std::move(sourceTriangles));
    std::map<Jerico::Guid, std::unique_ptr<D::TriangleBvh>> regionTrees;
    if (settings.projection == NormalProjection::ClosestSurface && settings.matchRegions)
    {
        std::map<Jerico::Guid, std::vector<D::Triangle>> grouped;
        for (const auto &triangle : bvh.triangles())
            grouped[high.mesh.triangleRegions[triangle.owner]].push_back(triangle);
        for (auto &[id, triangles] : grouped)
        {
            check(cancel);
            regionTrees.emplace(id, std::make_unique<D::TriangleBvh>(std::move(triangles)));
        }
    }
    NormalBake result;
    result.low = low;
    result.low.cornerNormals.clear();
    result.low.field.resize(low.mesh.positions.size());
    result.resolution = settings.resolution;
    result.settings = settings;
    if (settings.automaticUv)
    {
        result.cornerUV = automaticPlan.cornerUv;
        result.uvTriangleIsland = automaticPlan.triangleIsland;
        result.automaticUvIslandCount = automaticPlan.islandCount;
        result.automaticUvMarginPixels = automaticPlan.marginPixels;
        result.automaticUvSubpixelTriangles = automaticPlan.subpixelTriangles;
        result.automaticUvActiveAreaFraction = automaticPlan.activeAreaFraction;
    }
    result.highHash = PA::hashTriangleMesh(high.mesh);
    result.lowHash = PA::hashTriangleMesh(low.mesh);
    result.rgba.resize(std::size_t(settings.resolution) * settings.resolution * 4);
    std::vector<std::uint8_t> interiorCoverage;
    if (settings.automaticUv)
        interiorCoverage.resize(std::size_t(settings.resolution) * settings.resolution);
    if (settings.specialistMaps)
    {
        result.ambientRgba.resize(result.rgba.size());
        result.contactRgba.resize(result.rgba.size());
        result.curvatureRgba.resize(result.rgba.size());
        result.cavityRgba.resize(result.rgba.size());
        result.edgeRgba.resize(result.rgba.size());
        result.wireRgba.resize(result.rgba.size());
        result.thicknessRgba.resize(result.rgba.size());
        result.regionRgba.resize(result.rgba.size());
        result.heightRgba.resize(result.rgba.size());
    }
    std::vector<std::uint8_t> rawCurvatureRgba;
    if (settings.specialistMaps && settings.automaticUv && settings.curvatureRelaxation > 0)
        rawCurvatureRgba.resize(result.rgba.size());
    for (std::size_t i = 0; i < result.rgba.size(); i += 4)
    {
        result.rgba[i] = 128;
        result.rgba[i + 1] = 128;
        result.rgba[i + 2] = 255;
    }
    if (settings.automaticUv)
    {
        struct AccumulatedFrame
        {
            Vec3d normal{}, tangent{};
        };
        std::map<std::pair<std::uint32_t, std::uint32_t>, AccumulatedFrame> frames;
        for (std::size_t triangle = 0; triangle < targets.size(); ++triangle)
        {
            const auto &target = targets[triangle];
            auto e1 = D::subtract(target.second, target.first);
            auto e2 = D::subtract(target.third, target.first);
            auto cross = Projection::cross(e1, e2);
            auto n = unit(cross);
            auto first = triangle * 3;
            auto du1 = result.cornerUV[first + 1][0] - result.cornerUV[first][0];
            auto dv1 = result.cornerUV[first + 1][1] - result.cornerUV[first][1];
            auto du2 = result.cornerUV[first + 2][0] - result.cornerUV[first][0];
            auto dv2 = result.cornerUV[first + 2][1] - result.cornerUV[first][1];
            auto determinant = double(du1) * dv2 - double(du2) * dv1;
            auto uvScale = std::max(double(du1) * du1 + double(dv1) * dv1,
                                    double(du2) * du2 + double(dv2) * dv2);
            if (!(std::abs(determinant) > 1e-12 * uvScale))
                throw std::runtime_error("Automatic UV tangent basis singular");
            auto rawTangent = D::multiply(
                D::subtract(D::multiply(e1, dv2), D::multiply(e2, dv1)),
                determinant > 0 ? 1 : -1);
            auto tangent = unit(D::subtract(rawTangent, D::multiply(n, D::dot(rawTangent, n))));
            auto weight = Projection::length(cross);
            for (int corner = 0; corner < 3; ++corner)
            {
                auto key = std::pair{low.mesh.indices[first + corner], result.uvTriangleIsland[triangle]};
                auto &frame = frames[key];
                frame.normal = D::add(frame.normal, D::multiply(n, weight));
                frame.tangent = D::add(frame.tangent, D::multiply(tangent, weight));
            }
        }
        result.low.cornerNormals.resize(low.mesh.indices.size());
        result.cornerTangents.resize(low.mesh.indices.size());
        result.cornerBitangents.resize(low.mesh.indices.size());
        for (std::size_t triangle = 0; triangle < targets.size(); ++triangle)
        {
            auto first = triangle * 3;
            auto du1 = result.cornerUV[first + 1][0] - result.cornerUV[first][0];
            auto dv1 = result.cornerUV[first + 1][1] - result.cornerUV[first][1];
            auto du2 = result.cornerUV[first + 2][0] - result.cornerUV[first][0];
            auto dv2 = result.cornerUV[first + 2][1] - result.cornerUV[first][1];
            auto handedness = double(du1) * dv2 - double(du2) * dv1 > 0 ? 1 : -1;
            for (int corner = 0; corner < 3; ++corner)
            {
                auto key = std::pair{low.mesh.indices[first + corner], result.uvTriangleIsland[triangle]};
                const auto &frame = frames.at(key);
                auto n = unit(frame.normal);
                auto tangent = unit(D::subtract(frame.tangent, D::multiply(n, D::dot(frame.tangent, n))));
                auto index = first + corner;
                result.low.cornerNormals[index] = n;
                result.cornerTangents[index] = tangent;
                result.cornerBitangents[index] =
                    D::multiply(Projection::cross(n, tangent), handedness);
            }
        }
    }
    for (std::size_t triangle = 0; triangle < targets.size(); ++triangle)
    {
        check(cancel);
        const auto &target = targets[triangle];
        auto e1 = D::subtract(target.second, target.first), e2 = D::subtract(target.third, target.first);
        auto n = unit(Projection::cross(e1, e2)), tangent = unit(e1);
        // OBJ UVs use V up. The image rows increase downward, hence this chart has negative handedness.
        auto bitangent = D::multiply(Projection::cross(n, tangent), -1);
        unsigned x0 = settings.chartLayout.empty() ? unsigned(triangle % columns) * tile
                                                   : settings.chartLayout[triangle].x;
        unsigned y0 = settings.chartLayout.empty() ? unsigned(triangle / columns) * tile
                                                   : settings.chartLayout[triangle].y;
        unsigned width = settings.chartLayout.empty() ? tile : settings.chartLayout[triangle].size;
        unsigned height = width;
        double span = settings.automaticUv ? 0 : double(width - 2 * settings.padding);
        std::array<std::array<double, 2>, 3> pixelUv{};
        double pixelDet = 0;
        if (settings.automaticUv)
        {
            auto first = triangle * 3;
            for (int k = 0; k < 3; ++k)
            {
                auto uv = result.cornerUV[first + k];
                pixelUv[k] = {double(uv[0]) * settings.resolution,
                              (1 - double(uv[1])) * settings.resolution};
            }
            auto dx1 = pixelUv[1][0] - pixelUv[0][0];
            auto dy1 = pixelUv[1][1] - pixelUv[0][1];
            auto dx2 = pixelUv[2][0] - pixelUv[0][0];
            auto dy2 = pixelUv[2][1] - pixelUv[0][1];
            pixelDet = dx1 * dy2 - dx2 * dy1;
            if (!(std::abs(pixelDet) > 1e-9))
                throw std::runtime_error("Automatic UV bake triangle collapsed in pixel space");
            auto du1 = result.cornerUV[first + 1][0] - result.cornerUV[first][0];
            auto dv1 = result.cornerUV[first + 1][1] - result.cornerUV[first][1];
            auto du2 = result.cornerUV[first + 2][0] - result.cornerUV[first][0];
            auto dv2 = result.cornerUV[first + 2][1] - result.cornerUV[first][1];
            auto uvDet = double(du1) * dv2 - double(du2) * dv1;
            auto uvScale = std::max(double(du1) * du1 + double(dv1) * dv1,
                                    double(du2) * du2 + double(dv2) * dv2);
            if (!(std::abs(uvDet) > 1e-12 * uvScale))
                throw std::runtime_error("Automatic UV bake tangent basis singular");
            auto rawTangent = D::multiply(D::subtract(D::multiply(e1, dv2), D::multiply(e2, dv1)),
                                          1 / uvDet);
            tangent = unit(D::subtract(rawTangent, D::multiply(n, D::dot(rawTangent, n))));
            bitangent = D::multiply(Projection::cross(n, tangent), uvDet > 0 ? 1 : -1);
            auto minX = std::min({pixelUv[0][0], pixelUv[1][0], pixelUv[2][0]});
            auto maxX = std::max({pixelUv[0][0], pixelUv[1][0], pixelUv[2][0]});
            auto minY = std::min({pixelUv[0][1], pixelUv[1][1], pixelUv[2][1]});
            auto maxY = std::max({pixelUv[0][1], pixelUv[1][1], pixelUv[2][1]});
            x0 = unsigned(std::max(0.0, std::floor(minX) - settings.padding));
            y0 = unsigned(std::max(0.0, std::floor(minY) - settings.padding));
            auto xEnd = unsigned(std::min(double(settings.resolution), std::ceil(maxX) + settings.padding));
            auto yEnd = unsigned(std::min(double(settings.resolution), std::ceil(maxY) + settings.padding));
            width = xEnd - x0;
            height = yEnd - y0;
        }
        const D::TriangleBvh *nearestTree = &bvh;
        if (settings.projection == NormalProjection::ClosestSurface && settings.matchRegions)
        {
            auto found = regionTrees.find(low.mesh.triangleRegions[triangle]);
            nearestTree = found == regionTrees.end() ? nullptr : found->second.get();
        }
        for (int k = 0; k < 3; ++k)
        {
            if (!settings.automaticUv)
                result.cornerUV.push_back(
                    {float((x0 + settings.padding + (k == 1 ? span : 0)) / settings.resolution),
                     float(1 - (y0 + settings.padding + (k == 2 ? span : 0)) / settings.resolution)});
            if (!settings.automaticUv)
            {
                result.low.cornerNormals.push_back(n);
                result.cornerTangents.push_back(tangent);
                result.cornerBitangents.push_back(bitangent);
            }
        }
        for (unsigned y = 0; y < height; ++y)
        {
            if ((y & 15) == 0)
                check(cancel);
            for (unsigned x = 0; x < width; ++x)
            {
                double u, v;
                if (settings.automaticUv)
                {
                    auto px = x0 + x + 0.5 - pixelUv[0][0];
                    auto py = y0 + y + 0.5 - pixelUv[0][1];
                    u = (px * (pixelUv[2][1] - pixelUv[0][1]) -
                         py * (pixelUv[2][0] - pixelUv[0][0])) / pixelDet;
                    v = (py * (pixelUv[1][0] - pixelUv[0][0]) -
                         px * (pixelUv[1][1] - pixelUv[0][1])) / pixelDet;
                }
                else
                {
                    u = (x + 0.5 - settings.padding) / span;
                    v = (y + 0.5 - settings.padding) / span;
                }
                bool covered = settings.automaticUv
                    ? u >= -1e-9 && v >= -1e-9 && u + v <= 1 + 1e-9
                    : u >= 0 && v >= 0 && u + v <= 1;
                // Long, thin charts can cover several pixels while containing no pixel center.
                bool conservative = settings.automaticUv && !covered &&
                    (std::abs(pixelDet) < 4 ||
                     std::min(std::max({pixelUv[0][0], pixelUv[1][0], pixelUv[2][0]}) -
                                  std::min({pixelUv[0][0], pixelUv[1][0], pixelUv[2][0]}),
                              std::max({pixelUv[0][1], pixelUv[1][1], pixelUv[2][1]}) -
                                  std::min({pixelUv[0][1], pixelUv[1][1], pixelUv[2][1]})) < 1) &&
                    triangleTouchesPixel(pixelUv, x0 + x + 0.5, y0 + y + 0.5);
                if (conservative)
                {
                    covered = true;
                    u = v = 1.0 / 3.0;
                }
                if (settings.automaticUv)
                {
                    auto pixel = std::size_t(y0 + y) * settings.resolution + x0 + x;
                    if (!covered && interiorCoverage[pixel])
                        continue;
                    if (covered)
                    {
                        if (interiorCoverage[pixel])
                            continue;
                        interiorCoverage[pixel] = 1;
                        auto alpha = pixel * 4 + 3;
                        result.rgba[alpha] = 0;
                        for (auto *image : {&result.ambientRgba, &result.contactRgba,
                                            &result.curvatureRgba, &result.cavityRgba,
                                            &result.edgeRgba, &result.wireRgba,
                                            &result.thicknessRgba, &result.regionRgba,
                                            &result.heightRgba})
                            if (!image->empty())
                                (*image)[alpha] = 0;
                    }
                }
                // Project padding from the nearest point on this face; covered texels take priority.
                u = std::max(0.0, u);
                v = std::max(0.0, v);
                if (u + v > 1)
                {
                    auto sum = u + v;
                    u /= sum;
                    v /= sum;
                }
                auto p = D::add(target.first, D::add(D::multiply(e1, u), D::multiply(e2, v)));
                auto origin = D::add(p, D::multiply(n, settings.projectionDistance));
                const auto *region = settings.matchRegions ? &low.mesh.triangleRegions[triangle] : nullptr;
                Projection::Hit hit;
                double projectionDistance = 0;
                if (settings.projection == NormalProjection::NormalRay)
                {
                    hit = Projection::cast(
                        bvh, high.mesh, origin, D::multiply(n, -1), 2 * settings.projectionDistance,
                        std::numeric_limits<std::uint32_t>::max(), region, settings.projectionDistance, true);
                    projectionDistance = std::abs(settings.projectionDistance - hit.distance);
                }
                else if (nearestTree)
                {
                    hit = Projection::closestFront(*nearestTree, p, n, settings.projectionDistance);
                    projectionDistance = hit.distance;
                }
                if (covered)
                {
                    ++result.samples;
                    result.rejectedBackfaceCandidates += hit.rejectedBackfaces;
                }
                if (!std::isfinite(hit.distance))
                {
                    if (covered)
                    {
                        ++result.misses;
                        if (settings.projection == NormalProjection::ClosestSurface)
                        {
                            auto otherFront = Projection::closestFront(
                                bvh, p, n, settings.projectionDistance);
                            if (std::isfinite(otherFront.distance))
                                ++result.missesOtherRegionFront;
                            else if (bvh.closest(p, 1e-12).distance <= settings.projectionDistance)
                                ++result.missesNearbyBackface;
                            else
                                ++result.missesBeyondRadius;
                        }
                    }
                    continue;
                }
                auto normalAt = [&](int corner)
                {
                    auto index = hit.triangle * 3 + corner;
                    return high.cornerNormals.empty() ? high.normals[high.mesh.indices[index]]
                                                      : high.cornerNormals[index];
                };
                auto sourceNormal =
                    unit(D::add(D::multiply(normalAt(0), 1 - hit.u - hit.v),
                                D::add(D::multiply(normalAt(1), hit.u), D::multiply(normalAt(2), hit.v))));
                auto sampleNormal = n, sampleTangent = tangent, sampleBitangent = bitangent;
                if (settings.automaticUv)
                {
                    auto first = triangle * 3;
                    auto interpolate = [&](const std::vector<Vec3d> &values)
                    {
                        return D::add(D::multiply(values[first], 1 - u - v),
                                      D::add(D::multiply(values[first + 1], u),
                                             D::multiply(values[first + 2], v)));
                    };
                    sampleNormal = unit(interpolate(result.low.cornerNormals));
                    auto rawTangent = interpolate(result.cornerTangents);
                    sampleTangent = unit(D::subtract(rawTangent,
                                                     D::multiply(sampleNormal,
                                                                 D::dot(rawTangent, sampleNormal))));
                    sampleBitangent = D::multiply(Projection::cross(sampleNormal, sampleTangent),
                                                  pixelDet < 0 ? 1 : -1);
                }
                Vec3d mapped{D::dot(sourceNormal, sampleTangent),
                             D::dot(sourceNormal, sampleBitangent), D::dot(sourceNormal, sampleNormal)};
                auto encode = [](double value)
                { return std::uint8_t(std::lround(std::clamp(value * 0.5 + 0.5, 0.0, 1.0) * 255)); };
                auto at = (std::size_t(y0 + y) * settings.resolution + x0 + x) * 4;
                result.rgba[at] = encode(mapped.x);
                result.rgba[at + 1] = encode(mapped.y);
                result.rgba[at + 2] = encode(mapped.z);
                result.rgba[at + 3] = 255;
                if (settings.specialistMaps)
                {
                    auto interpolate = [&](const std::vector<double> &values)
                    {
                        auto base = hit.triangle * 3;
                        return values[high.mesh.indices[base]] * (1 - hit.u - hit.v) +
                               values[high.mesh.indices[base + 1]] * hit.u +
                               values[high.mesh.indices[base + 2]] * hit.v;
                    };
                    auto gray = [&](std::vector<std::uint8_t> &image, double value)
                    {
                        auto encoded = std::uint8_t(std::lround(std::clamp(value, 0.0, 1.0) * 255));
                        image[at] = image[at + 1] = image[at + 2] = encoded;
                        image[at + 3] = 255;
                    };
                    gray(result.ambientRgba, interpolate(sourceFields.ambientOcclusion));
                    gray(result.contactRgba, interpolate(sourceFields.contactOcclusion));
                    const auto sourceSignedCurvature = interpolate(sourceFields.signedDihedralCurvature);
                    auto signedCurvature = settings.curvatureRelaxation > 0
                        ? interpolate(*bakeCurvature) : sourceSignedCurvature;
                    if (sourceSignedCurvature == 0 ||
                        (sourceSignedCurvature > 0 && signedCurvature < 0) ||
                        (sourceSignedCurvature < 0 && signedCurvature > 0))
                        signedCurvature = 0;
                    gray(result.curvatureRgba,
                         0.5 + 0.5 * std::tanh(signedCurvature * settings.curvatureScale));
                    if (!rawCurvatureRgba.empty())
                        gray(rawCurvatureRgba,
                             0.5 + 0.5 * std::tanh(sourceSignedCurvature * settings.curvatureScale));
                    auto smoothMask = [](double x, double a, double b)
                    {
                        auto t = std::clamp((x - a) / (b - a), 0.0, 1.0);
                        return t * t * (3 - 2 * t);
                    };
                    // Preserve the signed field through interpolation before splitting it into masks.
                    // Interpolating pre-clamped vertex masks can falsely mark one texel both ways.
                    gray(result.cavityRgba,
                         smoothMask(std::max(-signedCurvature, 0.0) * settings.cavityRadius,
                                    0.1, 0.6));
                    gray(result.edgeRgba,
                         smoothMask(std::max(signedCurvature, 0.0) * settings.edgeRadius,
                                    0.15, 0.65));
                    auto wireAt = [&](int corner)
                    {
                        auto vertex = high.mesh.indices[hit.triangle * 3 + corner];
                        return 1 - smoothMask(sourceFields.edgeDistance[vertex], 0,
                                              settings.wireRadius);
                    };
                    gray(result.wireRgba, wireAt(0) * (1 - hit.u - hit.v) +
                                              wireAt(1) * hit.u + wireAt(2) * hit.v);
                    gray(result.thicknessRgba,
                         interpolate(sourceFields.thickness) * interpolate(sourceFields.thicknessConfidence) /
                             settings.thicknessRange);
                    auto base = hit.triangle * 3;
                    auto highPoint = D::add(
                        D::multiply(high.mesh.positions[high.mesh.indices[base]], 1 - hit.u - hit.v),
                        D::add(D::multiply(high.mesh.positions[high.mesh.indices[base + 1]], hit.u),
                               D::multiply(high.mesh.positions[high.mesh.indices[base + 2]], hit.v)));
                    auto height = D::dot(D::subtract(highPoint, p), n);
                    gray(result.heightRgba, 0.5 + 0.5 * height / settings.heightRange);
                    if (covered)
                    {
                        if (result.samples - result.misses == 1)
                            result.minimumHeight = result.maximumHeight = height;
                        else
                        {
                            result.minimumHeight = std::min(result.minimumHeight, height);
                            result.maximumHeight = std::max(result.maximumHeight, height);
                        }
                        if (std::abs(height) > settings.heightRange)
                            ++result.clippedHeightSamples;
                    }
                    auto region = Jerico::stableGuidHash(high.mesh.triangleRegions[hit.triangle]);
                    result.regionRgba[at] = std::uint8_t(region >> 16);
                    result.regionRgba[at + 1] = std::uint8_t(region >> 8);
                    result.regionRgba[at + 2] = std::uint8_t(region);
                    result.regionRgba[at + 3] = 255;
                }
                if (covered)
                {
                    auto distance = projectionDistance;
                    result.maximumDistance = std::max(result.maximumDistance, distance);
                    result.meanDistance += distance;
                    if (hit.backface)
                        ++result.backfaceHits;
                    auto decoded =
                        unit({result.rgba[at] * (2.0 / 255) - 1, result.rgba[at + 1] * (2.0 / 255) - 1,
                              result.rgba[at + 2] * (2.0 / 255) - 1});
                    result.meanEncodedAngleDegrees +=
                        std::acos(std::clamp(D::dot(mapped, decoded), -1.0, 1.0)) * 180 / std::numbers::pi;
                }
            }
        }
    }
    auto hits = result.samples - result.misses;
    if (hits)
    {
        result.meanDistance /= hits;
        result.meanEncodedAngleDegrees /= hits;
    }
    if (settings.automaticUv)
    {
        auto size = settings.resolution;
        const std::vector<std::uint8_t> *source = &result.rgba;
        for (unsigned level = 1; level < settings.automaticUvMipCount; ++level)
        {
            if (size < 2 || size % 2)
                throw std::runtime_error("Automatic UV mip count exceeds texture resolution");
            auto nextSize = size / 2;
            std::vector<std::uint8_t> next(std::size_t(nextSize) * nextSize * 4);
            for (unsigned y = 0; y < nextSize; ++y)
                for (unsigned x = 0; x < nextSize; ++x)
                {
                    Vec3d sum{};
                    unsigned painted = 0;
                    for (unsigned dy = 0; dy < 2; ++dy)
                        for (unsigned dx = 0; dx < 2; ++dx)
                        {
                            auto at = (std::size_t(2 * y + dy) * size + 2 * x + dx) * 4;
                            if ((*source)[at + 3] == 0)
                                continue;
                            ++painted;
                            sum.x += (*source)[at] * (2.0 / 255) - 1;
                            sum.y += (*source)[at + 1] * (2.0 / 255) - 1;
                            sum.z += (*source)[at + 2] * (2.0 / 255) - 1;
                        }
                    auto at = (std::size_t(y) * nextSize + x) * 4;
                    auto length = Projection::length(sum);
                    auto normal = length > 1e-12 ? D::multiply(sum, 1 / length) : Vec3d{0, 0, 1};
                    auto encode = [](double value)
                    { return std::uint8_t(std::lround(std::clamp(value * 0.5 + 0.5, 0.0, 1.0) * 255)); };
                    next[at] = encode(normal.x);
                    next[at + 1] = encode(normal.y);
                    next[at + 2] = encode(normal.z);
                    next[at + 3] = painted ? 255 : 0;
                }
            result.normalMips.push_back(std::move(next));
            source = &result.normalMips.back();
            size = nextSize;
        }
        if (settings.specialistMaps)
        {
            auto buildChain = [&](const std::vector<std::uint8_t> &base,
                                  std::vector<std::vector<std::uint8_t>> &chain, bool regionIds = false)
            {
                const std::vector<std::uint8_t> *source = &base;
                unsigned size = settings.resolution;
                for (unsigned level = 1; level < settings.automaticUvMipCount; ++level)
                {
                    chain.push_back(downsampleSpecialist(*source, size, regionIds, cancel));
                    source = &chain.back();
                    size /= 2;
                }
            };
            buildChain(result.ambientRgba, result.specialistMips.ambient);
            buildChain(result.contactRgba, result.specialistMips.contact);
            if (rawCurvatureRgba.empty())
                buildChain(result.curvatureRgba, result.specialistMips.curvature);
            else
            {
                const auto *candidateSource = &result.curvatureRgba;
                unsigned size = settings.resolution;
                for (unsigned level = 1; level < settings.automaticUvMipCount; ++level)
                {
                    auto candidateNext = downsampleSpecialist(*candidateSource, size, false, cancel);
                    auto rawNext = downsampleSpecialist(rawCurvatureRgba, size, false, cancel);
                    for (std::size_t at = 0; at < candidateNext.size(); at += 4)
                        if ((rawNext[at] > 128 && candidateNext[at] < 128) ||
                            (rawNext[at] < 128 && candidateNext[at] > 128))
                            candidateNext[at] = candidateNext[at + 1] = candidateNext[at + 2] = 128;
                    result.specialistMips.curvature.push_back(std::move(candidateNext));
                    rawCurvatureRgba = std::move(rawNext);
                    candidateSource = &result.specialistMips.curvature.back();
                    size /= 2;
                }
            }
            buildChain(result.cavityRgba, result.specialistMips.cavity);
            buildChain(result.edgeRgba, result.specialistMips.edge);
            buildChain(result.wireRgba, result.specialistMips.wire);
            buildChain(result.thicknessRgba, result.specialistMips.thickness);
            buildChain(result.regionRgba, result.specialistMips.region, true);
            buildChain(result.heightRgba, result.specialistMips.height);
        }
    }
    result.milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    check(cancel);
    return result;
}
void exportNormalBake(const NormalBake &bake, const std::filesystem::path &directory)
{
    if (bake.resolution == 0 || bake.rgba.size() != std::size_t(bake.resolution) * bake.resolution * 4 ||
        bake.cornerUV.size() != bake.low.mesh.indices.size() ||
        bake.low.cornerNormals.size() != bake.low.mesh.indices.size())
        throw std::runtime_error("Invalid normal bake export");
    if (bake.settings.automaticUv &&
        bake.uvTriangleIsland.size() != bake.low.mesh.indices.size() / 3)
        throw std::runtime_error("Invalid automatic UV chart identity export");
    for (const auto *image : {&bake.ambientRgba, &bake.contactRgba,
                              &bake.curvatureRgba, &bake.cavityRgba,
                              &bake.edgeRgba, &bake.wireRgba,
                              &bake.thicknessRgba, &bake.regionRgba, &bake.heightRgba})
        if ((!image->empty() || bake.settings.specialistMaps) && image->size() != bake.rgba.size())
            throw std::runtime_error("Invalid specialist map export dimensions");
    for (const auto *chain : {&bake.specialistMips.ambient, &bake.specialistMips.contact,
                              &bake.specialistMips.curvature, &bake.specialistMips.cavity,
                              &bake.specialistMips.edge, &bake.specialistMips.wire,
                              &bake.specialistMips.thickness, &bake.specialistMips.region,
                              &bake.specialistMips.height})
    {
        if (bake.settings.automaticUv && bake.settings.specialistMaps &&
            chain->size() != bake.settings.automaticUvMipCount - 1)
            throw std::runtime_error("Invalid specialist mip count");
        for (std::size_t level = 0; level < chain->size(); ++level)
        {
            const auto size = bake.resolution >> (level + 1);
            if (!size || (*chain)[level].size() != std::size_t(size) * size * 4)
                throw std::runtime_error("Invalid specialist mip dimensions");
        }
    }
    std::filesystem::create_directories(directory);
    auto normalPath = directory / "normal.png";
    if (!stbi_write_png(normalPath.string().c_str(), bake.resolution, bake.resolution, 4, bake.rgba.data(),
                        bake.resolution * 4))
        throw std::runtime_error("Could not write normal map");
    for (std::size_t level = 0; level < bake.normalMips.size(); ++level)
    {
        auto size = bake.resolution >> (level + 1);
        if (!size || bake.normalMips[level].size() != std::size_t(size) * size * 4 ||
            !stbi_write_png((directory / ("normal-mip-" + std::to_string(level + 1) + ".png")).string().c_str(),
                            int(size), int(size), 4, bake.normalMips[level].data(), int(size * 4)))
            throw std::runtime_error("Could not write normal mip map");
    }
    auto writeMap = [&](const std::vector<std::uint8_t> &image, const char *name)
    {
        if (!image.empty() &&
            !stbi_write_png((directory / name).string().c_str(), bake.resolution, bake.resolution, 4,
                            image.data(), bake.resolution * 4))
            throw std::runtime_error("Could not write specialist map");
    };
    writeMap(bake.ambientRgba, "ambient-occlusion.png");
    writeMap(bake.contactRgba, "contact-occlusion.png");
    writeMap(bake.curvatureRgba, "signed-curvature.png");
    writeMap(bake.cavityRgba, "cavity.png");
    writeMap(bake.edgeRgba, "convex-edge.png");
    writeMap(bake.wireRgba, "crease-distance-mask.png");
    writeMap(bake.thicknessRgba, "thickness.png");
    writeMap(bake.regionRgba, "region-id.png");
    writeMap(bake.heightRgba, "signed-height.png");
    auto writeMapMips = [&](const std::vector<std::vector<std::uint8_t>> &chain, const char *stem)
    {
        for (std::size_t level = 0; level < chain.size(); ++level)
        {
            const auto size = bake.resolution >> (level + 1);
            const auto path = directory / (std::string(stem) + "-mip-" + std::to_string(level + 1) + ".png");
            if (!stbi_write_png(path.string().c_str(), int(size), int(size), 4,
                                chain[level].data(), int(size * 4)))
                throw std::runtime_error("Could not write specialist mip map");
        }
    };
    writeMapMips(bake.specialistMips.ambient, "ambient-occlusion");
    writeMapMips(bake.specialistMips.contact, "contact-occlusion");
    writeMapMips(bake.specialistMips.curvature, "signed-curvature");
    writeMapMips(bake.specialistMips.cavity, "cavity");
    writeMapMips(bake.specialistMips.edge, "convex-edge");
    writeMapMips(bake.specialistMips.wire, "crease-distance-mask");
    writeMapMips(bake.specialistMips.thickness, "thickness");
    writeMapMips(bake.specialistMips.region, "region-id");
    writeMapMips(bake.specialistMips.height, "signed-height");
    std::ofstream mesh(directory / "low.obj");
    mesh << std::setprecision(17)
         << "# Playground low target; keep exported UVs and split normals. Tangent normal map: normal.png\n";
    for (auto p : bake.low.mesh.positions)
        mesh << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    for (auto uv : bake.cornerUV)
        mesh << "vt " << uv[0] << ' ' << uv[1] << '\n';
    for (auto n : bake.low.cornerNormals)
        mesh << "vn " << n.x << ' ' << n.y << ' ' << n.z << '\n';
    for (std::size_t t = 0; t < bake.low.mesh.indices.size(); t += 3)
    {
        mesh << "f";
        for (int k = 0; k < 3; ++k)
            mesh << ' ' << bake.low.mesh.indices[t + k] + 1 << '/' << t + k + 1 << '/' << t + k + 1;
        mesh << '\n';
    }
    mesh.close();
    if (!mesh)
        throw std::runtime_error("Could not write low mesh");
    nlohmann::json metadata = {
        {"highHash", bake.highHash},
        {"lowHash", bake.lowHash},
        {"resolution", bake.resolution},
        {"lowTriangles", bake.low.mesh.indices.size() / 3},
        {"samples", bake.samples},
        {"misses", bake.misses},
        {"missesOtherRegionFront", bake.missesOtherRegionFront},
        {"missesNearbyBackface", bake.missesNearbyBackface},
        {"missesBeyondRadius", bake.missesBeyondRadius},
        {"coverage", bake.samples ? 1 - double(bake.misses) / bake.samples : 0},
        {"backfaceHits", bake.backfaceHits},
        {"rejectedBackfaceCandidates", bake.rejectedBackfaceCandidates},
        {"projectionRadius", bake.settings.projectionDistance},
        {"padding", bake.settings.padding},
        {"matchRegions", bake.settings.matchRegions},
        {"specialistMaps", bake.settings.specialistMaps},
        {"featureRadius", bake.settings.featureRadius},
        {"cavityRadius", bake.settings.cavityRadius},
        {"edgeRadius", bake.settings.edgeRadius},
        {"wireRadius", bake.settings.wireRadius},
        {"creaseAngleDegrees", 60},
        {"contactRadius", bake.settings.contactRadius.value_or(bake.settings.featureRadius * 0.25)},
        {"ambientRadius", bake.settings.ambientRadius.value_or(bake.settings.featureRadius)},
        {"cavityMapping", "smoothstep(0.1,0.6,concavity*cavityRadius)"},
        {"convexEdgeMapping", "smoothstep(0.15,0.65,convexity*edgeRadius)"},
        {"creaseMaskMapping", "1-smoothstep(0,wireRadius,edgeGraphDistance)"},
        {"featureSamples", bake.settings.featureSamples},
        {"curvatureMapping", "0.5 + 0.5*tanh(signedCurvature*curvatureScale)"},
        {"curvatureScale", bake.settings.curvatureScale},
        {"curvatureRelaxation", bake.settings.curvatureRelaxation},
        {"thicknessMapping", "clamp(inwardFirstHit/thicknessRange,0,1); miss=0"},
        {"thicknessRange", bake.settings.thicknessRange},
        {"heightMapping", "0.5 + 0.5*clamp(dot(highHit-lowPoint,lowFaceNormal)/heightRange,-1,1)"},
        {"heightRange", bake.settings.heightRange},
        {"minimumHeight", bake.minimumHeight},
        {"maximumHeight", bake.maximumHeight},
        {"clippedHeightSamples", bake.clippedHeightSamples},
        {"sampledLowReductionError", bake.low.sampledReductionError},
        {"sampledLowSurfaceError", bake.low.sampledReductionError},
        {"lowStrategy", bake.lowStrategy},
        {"maximumProjectionDistance", bake.maximumDistance},
        {"meanProjectionDistance", bake.meanDistance},
        {"meanEncodingAngleDegrees", bake.meanEncodedAngleDegrees},
        {"cpuMs", bake.milliseconds},
        {"normalConvention",
         "Tangent RGB +Y toward increasing OBJ V. Linear texture. Preserve split normals/UVs."},
        {"atlas", "Independent triangle charts with tile-local padding; no mip chain; not production unwrap"},
        {"projection", bake.settings.projection == NormalProjection::NormalRay
                           ? "Normal-offset cage, nearest front-facing ray hit to low surface, no fallback"
                           : "Explicit closest-surface projection, same-region BVH when enabled, rejects "
                             "opposite-facing source"},
        {"missPolicy", "Neutral tangent normal with alpha zero; misses counted, never hidden"}};
    if (!bake.settings.chartLayout.empty())
    {
        nlohmann::json charts = nlohmann::json::array();
        for (const auto &chart : bake.settings.chartLayout)
            charts.push_back({chart.x, chart.y, chart.size});
        std::ofstream layout(directory / "chart-layout.json");
        layout << charts.dump() << '\n';
        layout.close();
        if (!layout)
            throw std::runtime_error("Could not write normal atlas chart layout");
        metadata["atlas"] = "Explicit independent triangle rectangles; no mip chain; experimental";
        metadata["chartLayout"] = "chart-layout.json";
    }
    if (bake.settings.automaticUv)
    {
        std::ofstream charts(directory / "uv-chart-islands.json");
        charts << nlohmann::json(bake.uvTriangleIsland).dump() << '\n';
        charts.close();
        if (!charts)
            throw std::runtime_error("Could not write automatic UV chart identities");
        metadata["atlas"] = "Connected automatic UV charts; filtered normal and specialist mip chains; experimental";
        metadata["uvChartIslands"] = "uv-chart-islands.json";
        metadata["automaticUvIslandCount"] = bake.automaticUvIslandCount;
        metadata["automaticUvMarginPixels"] = bake.automaticUvMarginPixels;
        metadata["automaticUvTargetMipCount"] = bake.settings.automaticUvMipCount;
        metadata["normalMipLevels"] = bake.normalMips.size() + 1;
        metadata["specialistMipLevels"] = bake.settings.specialistMaps ?
            bake.specialistMips.ambient.size() + 1 : 0;
        metadata["specialistMipFilter"] =
            "Coverage-aware linear average for scalar maps; categorical majority for region ID";
        metadata["regionIdSampling"] =
            "Use mip0 and nearest filtering for exact region selection; categorical mips are visualization only";
        metadata["automaticUvSubpixelTriangles"] = bake.automaticUvSubpixelTriangles;
        metadata["automaticUvActiveAreaFraction"] = bake.automaticUvActiveAreaFraction;
    }
    std::ofstream report(directory / "bake.json");
    report << metadata.dump(2) << '\n';
    report.close();
    if (!report)
        throw std::runtime_error("Could not write bake report");
}
} // namespace Playground
