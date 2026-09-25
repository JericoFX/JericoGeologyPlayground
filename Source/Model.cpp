#include "Model.hpp"
#include "LayerFormation.hpp"
#include "MeshReduction.hpp"
#include "RockAddonMath.hpp"
#include "SolidGeometry.hpp"
#include "SurfaceFeatures.hpp"
#include "SurfaceGeometry.hpp"
#include <Jerico/ProceduralAuthoring/Geology/GeologyNoise.hpp>
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <Windows.h>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <numbers>
#include <set>
#include <stdexcept>
#include <tuple>

namespace Playground
{
using Json = nlohmann::json;
namespace
{
Vec3d add(Vec3d a, Vec3d b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3d sub(Vec3d a, Vec3d b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3d mul(Vec3d a, double s)
{
    return {a.x * s, a.y * s, a.z * s};
}
double dot(Vec3d a, Vec3d b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3d cross(Vec3d a, Vec3d b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3d unit(Vec3d a)
{
    double l = std::sqrt(dot(a, a));
    return l > 1e-12 ? mul(a, 1 / l) : Vec3d{0, 1, 0};
}
Vec3d vec(std::array<float, 3> a)
{
    return {a[0], a[1], a[2]};
}
Vec3d product(Vec3d a, Vec3d b)
{
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}
double smooth(double x)
{
    x = std::clamp(x, 0.0, 1.0);
    return x * x * (3 - 2 * x);
}
std::uint32_t hash(std::uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    return x ^ (x >> 16);
}
double random(std::uint32_t x)
{
    return double(hash(x) & 0xffffffU) / 16777215.0;
}
void checkCancel(const Jerico::CancellationToken *cancel)
{
    if (cancel && cancel->isCancellationRequested())
        throw std::runtime_error("Cancelled");
}
Vec3d rotate(Vec3d p, std::array<float, 3> degrees)
{
    const auto r = mul(vec(degrees), std::numbers::pi / 180.0);
    p = {p.x, p.y * std::cos(r.x) - p.z * std::sin(r.x), p.y * std::sin(r.x) + p.z * std::cos(r.x)};
    p = {p.x * std::cos(r.y) + p.z * std::sin(r.y), p.y, -p.x * std::sin(r.y) + p.z * std::cos(r.y)};
    return {p.x * std::cos(r.z) - p.y * std::sin(r.z), p.x * std::sin(r.z) + p.y * std::cos(r.z), p.z};
}
MeshResult primitive(Base type, int n)
{
    MeshResult result;
    std::map<std::tuple<long long, long long, long long>, std::uint32_t> ids;
    auto vertex = [&](Vec3d p)
    {
        if (type == Base::Sphere)
            p = unit(p);
        if (type == Base::Cylinder || type == Base::Cone)
        {
            double radius = std::max(std::abs(p.x), std::abs(p.z)), length = std::hypot(p.x, p.z);
            if (length > 1e-10)
            {
                p.x *= radius / length;
                p.z *= radius / length;
            }
            if (type == Base::Cone)
            {
                p.x *= (1 - p.y) * 0.5;
                p.z *= (1 - p.y) * 0.5;
            }
        }
        auto key = std::tuple{std::llround(p.x * 1e9), std::llround(p.y * 1e9), std::llround(p.z * 1e9)};
        auto [it, inserted] = ids.try_emplace(key, static_cast<std::uint32_t>(result.mesh.positions.size()));
        if (inserted)
            result.mesh.positions.push_back(p);
        return it->second;
    };
    const Vec3d axes[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    int faceId = 0;
    for (auto normal : axes)
    {
        Vec3d u = unit(cross(std::abs(normal.y) < 0.9 ? Vec3d{0, 1, 0} : Vec3d{0, 0, 1}, normal)),
              v = cross(normal, u);
        std::vector<std::uint32_t> grid((n + 1) * (n + 1));
        for (int y = 0; y <= n; ++y)
            for (int x = 0; x <= n; ++x)
                grid[y * (n + 1) + x] =
                    vertex(add(normal, add(mul(u, 2.0 * x / n - 1), mul(v, 2.0 * y / n - 1))));
        auto triangle = [&](std::uint32_t a, std::uint32_t b, std::uint32_t c)
        {
            if (a != b && b != c && a != c)
            {
                result.mesh.indices.insert(result.mesh.indices.end(), {a, b, c});
                result.mesh.triangleRegions.push_back(Solid::region(0, type == Base::Cube ? faceId : 6));
            }
        };
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
            {
                auto a = grid[y * (n + 1) + x], b = grid[y * (n + 1) + x + 1],
                     c = grid[(y + 1) * (n + 1) + x], d = grid[(y + 1) * (n + 1) + x + 1];
                if (type == Base::Cylinder && normal.y < 0)
                {
                    // Mirrored cap coordinates need the opposite local diagonal to share world-space facets.
                    triangle(a, b, c);
                    triangle(b, d, c);
                }
                else
                {
                    triangle(a, b, d);
                    triangle(a, d, c);
                }
            }
        ++faceId;
    }
    result.field.resize(result.mesh.positions.size());
    std::set<Jerico::Guid> regions(result.mesh.triangleRegions.begin(), result.mesh.triangleRegions.end());
    for (auto id : regions)
        result.regions.push_back({id, {}, SurfaceRole::Source, -1, -1});
    normals(result);
    return result;
}
double fbm(Vec3d p, const Layer &l, std::uint32_t offset = 0)
{
    p = product(mul(p, l.frequency), vec(l.coordinateScale));
    return PA::evaluateGeologyFbmReference(p, l.seed + offset, l.octaves, l.lacunarity, l.roughness);
}
Vec3d vectorNoise(Vec3d p, const Layer &l)
{
    return {fbm(p, l), fbm(p, l, 0x68bc21ebU), fbm(p, l, 0x967a889bU)};
}
double maskValue(Vec3d p, Vec3d n, const Layer &l, const SurfaceFeatureFields *features = nullptr,
                 std::size_t vertex = 0)
{
    double value = 1;
    auto direction = unit(vec(l.maskAxis == std::array<float, 3>{} ? l.direction : l.maskAxis));
    switch (l.mask)
    {
    case Mask::None:
        return 1;
    case Mask::Height:
        value = p.y;
        break;
    case Mask::Slope:
        value = 1 - std::abs(n.y);
        break;
    case Mask::Noise:
        value = 0.5 + 0.5 * fbm(p, l, 71);
        break;
    case Mask::Direction:
        value = dot(n, direction);
        break;
    case Mask::Radial:
        value = std::sqrt(dot(p, p));
        break;
    case Mask::Gradient:
        value = dot(p, direction);
        break;
    case Mask::Curvature:
        value = features->signedDihedralCurvature[vertex];
        break;
    case Mask::EdgeProximity:
        value = 1 - smooth(features->edgeDistance[vertex] / l.featureRadius);
        break;
    case Mask::Contact:
        value = features->contactOcclusion[vertex];
        break;
    case Mask::Convexity:
        value = features->convexity[vertex];
        break;
    case Mask::Concavity:
        value = features->concavity[vertex];
        break;
    case Mask::AmbientOcclusion:
        value = features->ambientOcclusion[vertex];
        break;
    case Mask::Exposure:
        value = features->exposure[vertex];
        break;
    case Mask::BoundaryProximity:
        value = 1 - smooth(features->boundaryDistance[vertex] / l.featureRadius);
        break;
    case Mask::Thickness:
        value = features->thicknessConfidence[vertex] ? features->thickness[vertex] : 0;
        break;
    }
    value = smooth((value - l.maskLow) / std::max(1e-5f, l.maskHigh - l.maskLow));
    return l.maskInvert ? 1 - value : value;
}
double voronoi(Vec3d p, const Layer &l)
{
    p = product(mul(p, l.frequency), vec(l.coordinateScale));
    int bx = int(std::floor(p.x)), by = int(std::floor(p.y)), bz = int(std::floor(p.z));
    double first = 1e20, second = 1e20;
    Vec3d nearest{};
    auto point = [&](int x, int y, int z)
    {
        auto s = l.seed ^ (std::uint32_t(x) * 0x8da6b343U) ^ (std::uint32_t(y) * 0xd8163841U) ^
                 (std::uint32_t(z) * 0xcb1ab31fU);
        return Vec3d{x + 0.5 + l.randomness * (random(s) - 0.5),
                     y + 0.5 + l.randomness * (random(s + 13) - 0.5),
                     z + 0.5 + l.randomness * (random(s + 37) - 0.5)};
    };
    for (int z = -2; z <= 2; ++z)
        for (int y = -2; y <= 2; ++y)
            for (int x = -2; x <= 2; ++x)
            {
                auto r = sub(point(bx + x, by + y, bz + z), p);
                double d = dot(r, r);
                if (d < first)
                {
                    second = first;
                    first = d;
                    nearest = r;
                }
                else if (d < second)
                    second = d;
            }
    if (l.voronoiMode == 0)
        return std::sqrt(first);
    if (l.voronoiMode == 1)
        return std::sqrt(second);
    double edge = 1e20;
    for (int z = -2; z <= 2; ++z)
        for (int y = -2; y <= 2; ++y)
            for (int x = -2; x <= 2; ++x)
            {
                auto r = sub(point(bx + x, by + y, bz + z), p), delta = sub(r, nearest);
                if (dot(delta, delta) > 1e-12)
                    edge = std::min(edge, dot(mul(add(r, nearest), 0.5), unit(delta)));
            }
    return std::max(0.0, edge);
}
Json layerJson(const Layer &l)
{
    return Json{{"kind", int(l.kind)},
                {"enabled", l.enabled},
                {"seed", l.seed},
                {"amplitude", l.amplitude},
                {"frequency", l.frequency},
                {"lacunarity", l.lacunarity},
                {"roughness", l.roughness},
                {"octaves", l.octaves},
                {"coordinateScale", l.coordinateScale},
                {"outputScale", l.outputScale},
                {"scale", l.scale},
                {"rotation", l.rotation},
                {"translation", l.translation},
                {"direction", l.direction},
                {"maskAxis", l.maskAxis},
                {"voronoiMode", l.voronoiMode},
                {"randomness", l.randomness},
                {"threshold", l.threshold},
                {"width", l.width},
                {"count", l.count},
                {"thickness", l.thickness},
                {"spacing", l.spacing},
                {"layerScale", l.layerScale},
                {"perimeterAmplitude", l.perimeterAmplitude},
                {"beddingTilt", l.beddingTilt},
                {"macroAxisAsymmetry", l.macroAxisAsymmetry},
                {"shapeOuterCaps", l.shapeOuterCaps},
                {"jitter", l.jitter},
                {"lateral", l.lateral},
                {"iterations", l.iterations},
                {"relaxStrength", l.relaxStrength},
                {"mask", int(l.mask)},
                {"maskLow", l.maskLow},
                {"maskHigh", l.maskHigh},
                {"maskInvert", l.maskInvert},
                {"coherentLayers", l.coherentLayers},
                {"thicknessFrequency", l.thicknessFrequency},
                {"thicknessCorrelation", l.thicknessCorrelation},
                {"minimumThickness", l.minimumThickness},
                {"maximumThickness", l.maximumThickness},
                {"driftCorrelation", l.driftCorrelation},
                {"rotationDrift", l.rotationDrift},
                {"scaleDrift", l.scaleDrift},
                {"driftBias", l.driftBias},
                {"envelopeShape", l.envelopeShape},
                {"envelopeRoundness", l.envelopeRoundness},
                {"cutSubdivisions", l.cutSubdivisions},
                {"phase", l.phase},
                {"profile", l.profile},
                {"alphaSource", l.alphaSource},
                {"profileSharpness", l.profileSharpness},
                {"distortion", l.distortion},
                {"solidOffset", l.solidOffset},
                {"solidAlongNormals", l.solidAlongNormals},
                {"reductionError", l.reductionError},
                {"featureRadius", l.featureRadius},
                {"featureAngle", l.featureAngle},
                {"contactSamples", l.contactSamples},
                {"projectorRadius", l.projectorRadius},
                {"projectorRotation", l.projectorRotation},
                {"damageLip", l.damageLip}};
}
void finiteJson(const Json &j)
{
    if (j.is_number_float() && !std::isfinite(j.get<double>()))
        throw std::runtime_error("Non-finite parameter");
    if (j.is_structured())
        for (auto &value : j)
            finiteJson(value);
}
} // namespace
const char *name(Kind kind)
{
    static const char *names[] = {"Transform",
                                  "Normal displacement / FBM",
                                  "Vector displacement / FBM",
                                  "Voronoi",
                                  "Layer formation",
                                  "Domain warp",
                                  "Surface / field fracture",
                                  "Mesh neighbor relax",
                                  "Structural planes",
                                  "Shared macro warp",
                                  "Macroform envelope",
                                  "Layer edge breakup",
                                  "Strata relief",
                                  "Ridged noise",
                                  "Solidify surface",
                                  "Reduce mesh",
                                  "Surface damage / wear",
                                  "Oriented damage decal",
                                  "Layer settling",
                                  "Cliff cut loop",
                                  "Grounded fracture shear",
                                  "Connected four-block chisel",
                                  "Cliff drainage incision"};
    return names[int(kind)];
}
Layer defaultLayer(Kind kind)
{
    Layer l;
    l.kind = kind;
    if (kind == Kind::Warp)
        l.amplitude = 0.4f;
    if (kind == Kind::Fracture)
        l.amplitude = 0.04f;
    if (kind == Kind::Planes)
    {
        l.count = 10;
        l.amplitude = 0.24f;
    }
    if (kind == Kind::FractureShear)
    {
        l.direction = {0.8f, 0, 0.6f};
        l.amplitude = 0.12f;
    }
    if (kind == Kind::FractureQuadrants)
    {
        l.direction = {0.8f, 0, 0.6f};
        l.amplitude = 0.12f;
    }
    if (kind == Kind::SharedWarp)
    {
        l.amplitude = 0.22f;
        l.frequency = 0.7f;
        l.octaves = 2;
        l.coordinateScale = {1, 0.12f, 1};
        l.outputScale = {0.25f, 1, 0.25f};
    }
    if (kind == Kind::Envelope)
        l.scale = {0.9f, 0.9f, 0.9f};
    if (kind == Kind::EdgeBreakup)
    {
        l.amplitude = 0.025f;
        l.frequency = 6;
        l.width = 0.2f;
        l.octaves = 2;
    }
    if (kind == Kind::StrataRelief)
    {
        l.amplitude = 0.22f;
        l.frequency = 1.5f;
        l.profile = 1;
        l.direction = {0, 1, 0};
        l.width = 0.25f;
    }
    if (kind == Kind::RidgedNoise)
    {
        l.amplitude = 0.18f;
        l.frequency = 1.6f;
        l.octaves = 4;
    }
    if (kind == Kind::Solidify)
    {
        l.thickness = 0.3f;
        l.direction = {0, 0, 1};
        l.solidOffset = -1;
    }
    if (kind == Kind::SurfaceDamage)
    {
        l.amplitude = 0.06f;
        l.frequency = 5;
        l.octaves = 2;
        l.mask = Mask::EdgeProximity;
        l.maskLow = 0;
        l.maskHigh = 1;
        l.threshold = 0.25f;
        l.width = 0.5f;
    }
    if (kind == Kind::DamageDecal)
    {
        l.amplitude = 0.12f;
        l.threshold = 0.65f;
        l.width = 0.35f;
        l.profileSharpness = 2;
        l.featureRadius = 0.15f;
        l.direction = {0, 0, 1};
        l.translation = {0, 0, 1};
    }
    if (kind == Kind::CliffCutLoop)
    {
        l.amplitude = 0.48f;
        l.translation = {0.45f, 0.15f, 0};
        l.projectorRadius = {1.45f, 1.0f};
        l.projectorRotation = 18;
        l.distortion = 0.16f;
        l.lateral = 0.1f;
    }
    if (kind == Kind::CliffDrainage)
    {
        l.amplitude = 0.16f;
        l.width = 0.30f;
        l.translation[0] = -1.5f;
        l.lateral = 0.22f;
        l.frequency = 1;
        l.thicknessFrequency = 2.2f;
        l.phase = 0.25f;
    }
    return l;
}
Recipe preset(int index)
{
    Recipe r;
    r.base = Base::Sphere;
    auto macro = defaultLayer(Kind::NormalNoise);
    macro.frequency = 1.8f;
    macro.amplitude = 0.18f;
    macro.octaves = 3;
    auto vector = defaultLayer(Kind::VectorNoise);
    vector.frequency = 1.15f;
    vector.amplitude = 0.18f;
    vector.octaves = 2;
    auto strata = defaultLayer(Kind::Layers);
    strata.count = 5;
    strata.thickness = 0.31f;
    strata.spacing = 0.045f;
    auto fine = defaultLayer(Kind::NormalNoise);
    fine.amplitude = 0.012f;
    fine.octaves = 2;
    fine.frequency = 8;
    if (index == 0)
        r.layers = {vector, macro, fine};
    if (index == 1)
    {
        r.base = Base::Cylinder;
        r.layers = {vector, strata, macro, fine};
        r.layers[2].amplitude = 0.04f;
    }
    if (index == 2)
    {
        r.base = Base::Sphere;
        auto v = defaultLayer(Kind::Voronoi);
        v.amplitude = 0.06f;
        v.voronoiMode = 0;
        v.frequency = 2.5f;
        r.layers = {vector, v, defaultLayer(Kind::Fracture), fine};
    }
    if (index == 3)
    {
        r.layers = {strata, defaultLayer(Kind::Warp), macro, fine};
        r.layers[2].coordinateScale = {0.5f, 2.5f, 0.5f};
        r.layers[2].amplitude = 0.035f;
    }
    if (index == 4)
    {
        r.base = Base::Cube;
        auto t = defaultLayer(Kind::Transform);
        t.scale = {1.7f, 1.3f, 0.6f};
        r.layers = {strata, t, vector, defaultLayer(Kind::Fracture), fine};
    }
    if (index == 5)
    {
        r.base = Base::Cylinder;
        r.finalResolution = 32;
        strata.coherentLayers = true;
        strata.count = 6;
        strata.thickness = 0.25f;
        strata.spacing = 0;
        strata.jitter = 0.6f;
        strata.thicknessFrequency = 1.9f;
        strata.thicknessCorrelation = 0.55f;
        strata.lateral = 0;
        strata.layerScale = 0;
        strata.rotationDrift = 0;
        strata.scaleDrift = 0;
        strata.perimeterAmplitude = 0.16f;
        strata.shapeOuterCaps = true;
        strata.frequency = 2.5f;
        r.layers = {strata, defaultLayer(Kind::LayerSettle)};
    }
    if (index == 6)
    {
        r.base = Base::Cube;
        auto transform = defaultLayer(Kind::Transform);
        transform.scale = {1, 0.9f, 0.8f};
        auto planes = defaultLayer(Kind::Planes);
        planes.amplitude = 0.3f;
        planes.count = 11;
        auto shared = defaultLayer(Kind::SharedWarp);
        shared.amplitude = 0.075f;
        shared.outputScale = {1, 0.5f, 1};
        auto fracture = defaultLayer(Kind::Fracture);
        fracture.amplitude = 0.012f;
        fracture.frequency = 3;
        fine.amplitude = 0.006f;
        fine.frequency = 12;
        r.layers = {transform, planes, shared, fracture, fine};
    }
    if (index == 7 || index == 8 || index == 9)
    {
        r.base = index == 9 ? Base::BspWall : Base::Plane;
        r.previewResolution = 32;
        r.finalResolution = 64;
        auto macro = defaultLayer(Kind::VectorNoise);
        macro.amplitude = 0.3f;
        macro.frequency = 0.6f;
        macro.outputScale = {0.05f, 0.05f, 1};
        macro.coordinateScale = {1, 0.3f, 1};
        auto relief = defaultLayer(Kind::StrataRelief);
        relief.amplitude = 0.45f;
        relief.frequency = 1.6f;
        relief.distortion = 0.35f;
        auto ridged = defaultLayer(Kind::RidgedNoise);
        ridged.amplitude = 0.25f;
        ridged.frequency = 1.2f;
        ridged.coordinateScale = {1, 0.4f, 1};
        if (index == 8)
        {
            relief.profile = 0;
            relief.amplitude = 0.08f;
            relief.frequency = 2.2f;
            relief.mask = Mask::Noise;
            relief.maskLow = 0.35f;
            relief.maskHigh = 0.7f;
            ridged.amplitude = 0.20f;
        }
        fine.amplitude = 0.018f;
        fine.frequency = 7;
        r.layers = {macro, relief, ridged, fine};
        if (index == 9)
            for (auto &layer : r.layers)
                if (layer.kind == Kind::StrataRelief || layer.kind == Kind::RidgedNoise ||
                    layer.kind == Kind::NormalNoise)
                {
                    layer.mask = Mask::Direction;
                    layer.maskAxis = {0, 0, -1};
                    layer.maskLow = 0.5f;
                    layer.maskHigh = 0.9f;
                }
        if (index == 8)
        {
            r.layers.push_back(defaultLayer(Kind::CliffCutLoop));
            auto solid = defaultLayer(Kind::Solidify);
            solid.thickness = 0.7f;
            solid.solidAlongNormals = false;
            r.layers.push_back(solid);
        }
    }
    if (index == 10)
    {
        r.base = Base::Cube;
        r.previewResolution = 20;
        r.finalResolution = 32;
        auto transform = defaultLayer(Kind::Transform);
        transform.scale = {1.2f, 0.85f, 0.85f};
        auto planes = defaultLayer(Kind::Planes);
        planes.count = 8;
        planes.amplitude = 0.32f;
        planes.cutSubdivisions = 3;
        auto wear = defaultLayer(Kind::SurfaceDamage);
        wear.amplitude = 0.12f;
        wear.featureRadius = 0.3f;
        wear.featureAngle = 22;
        wear.frequency = 4;
        wear.threshold = 0.15f;
        wear.width = 0.65f;
        r.layers = {transform, planes, wear};
    }
    if (index == 11)
    {
        r.base = Base::Cylinder;
        r.previewResolution = 16;
        r.finalResolution = 24;
        strata.coherentLayers = true;
        strata.count = 4;
        strata.thickness = 0.32f;
        strata.spacing = 0;
        strata.jitter = 0.55f;
        strata.thicknessFrequency = 2.3f;
        strata.thicknessCorrelation = 0.45f;
        strata.lateral = 0;
        strata.rotationDrift = 0;
        strata.layerScale = 0;
        strata.perimeterAmplitude = 0.16f;
        strata.beddingTilt = 0.10f;
        strata.macroAxisAsymmetry = 0.42f;
        strata.shapeOuterCaps = true;
        strata.frequency = 3.0f;
        r.layers = {strata, defaultLayer(Kind::LayerSettle)};
    }
    if (index == 12)
    {
        r.base = Base::Sphere;
        r.previewResolution = 20;
        r.finalResolution = 32;
        vector.amplitude = 0.1f;
        vector.frequency = 1.3f;
        macro.amplitude = 0.06f;
        macro.frequency = 2.2f;
        auto crater = defaultLayer(Kind::DamageDecal);
        crater.translation = {0, 0, -1};
        crater.direction = {0, 0, -1};
        crater.projectorRadius = {0.68f, 0.52f};
        crater.featureRadius = 0.45f;
        crater.amplitude = 0.16f;
        crater.damageLip = 0.035f;
        r.layers = {vector, macro, crater, fine};
    }
    return r;
}
void validate(const Recipe &r)
{
    auto require = [](bool ok)
    {
        if (!ok)
            throw std::runtime_error("Recipe parameter outside Playground limits");
    };
    require((r.version == 10 || r.version == 11 || r.version == 12 || r.version == 13) &&
            int(r.base) >= 0 && int(r.base) < 6 &&
            r.layers.size() <= 24);
    for (auto x : r.planeSize)
        require(std::isfinite(x) && x >= 0.1f && x <= 100);
    require(r.previewResolution >= 4 && r.previewResolution <= 64 && r.finalResolution >= 4 &&
            r.finalResolution <= 64);
    for (auto &l : r.layers)
    {
        finiteJson(layerJson(l));
        require(int(l.kind) >= 0 && int(l.kind) < 23 && int(l.mask) >= 0 && int(l.mask) < 16);
        if (l.kind == Kind::CliffDrainage)
            require(r.version >= 13 && l.mask == Mask::None && l.amplitude >= 0.01f &&
                    l.amplitude <= 0.5f && l.width >= 0.1f && l.width <= 1 &&
                    l.frequency <= 4 && l.lateral <= 1);
        if (l.kind == Kind::FractureShear)
            require(r.version >= 11 && l.mask == Mask::None && l.amplitude >= 0.01f &&
                    l.amplitude <= 0.3f &&
                    std::hypot(double(l.direction[0]), double(l.direction[2])) > 1e-8);
        if (l.kind == Kind::FractureQuadrants)
            require(r.version >= 12 && l.mask == Mask::None && l.amplitude >= 0.05f &&
                    l.amplitude <= 0.3f &&
                    std::hypot(double(l.direction[0]), double(l.direction[2])) > 1e-8);
        if (l.kind == Kind::LayerSettle)
            require(l.mask == Mask::None);
        require(l.featureRadius >= 0.001f && l.featureRadius <= 10 && l.featureAngle >= 1 &&
                l.featureAngle <= 179 && l.contactSamples >= 4 && l.contactSamples <= 64);
        if (l.kind == Kind::SurfaceDamage)
            require(l.amplitude >= 0);
        if (l.kind == Kind::DamageDecal)
            require(l.amplitude >= 0 && l.projectorRadius[0] > 0 && l.projectorRadius[0] <= 10 &&
                    l.projectorRadius[1] > 0 && l.projectorRadius[1] <= 10 &&
                    std::isfinite(l.projectorRotation) && std::abs(l.projectorRotation) <= 360 &&
                    l.damageLip >= 0 && l.damageLip <= 1 && l.threshold < 1 &&
                    l.threshold + l.width >= 0.001f);
        if (l.kind == Kind::CliffCutLoop)
            require(l.mask == Mask::None && l.amplitude > 0 && l.amplitude <= 2 &&
                    l.alphaSource >= 0 && l.alphaSource <= 1 &&
                    (l.alphaSource == 0 || (r.version >= 13 && l.profile == 2)) &&
                    l.projectorRadius[0] >= 0.1f && l.projectorRadius[0] <= 5 &&
                    l.projectorRadius[1] >= 0.1f && l.projectorRadius[1] <= 5 &&
                    std::abs(l.projectorRotation) <= 360 && l.distortion <= 0.3f);
        if (int(l.mask) >= int(Mask::Curvature))
            require(l.kind != Kind::Transform && l.kind != Kind::Layers && l.kind != Kind::Planes &&
                    l.kind != Kind::Envelope && l.kind != Kind::Solidify && l.kind != Kind::Reduce &&
                    l.kind != Kind::LayerSettle && l.kind != Kind::CliffCutLoop &&
                    l.kind != Kind::FractureShear && l.kind != Kind::FractureQuadrants &&
                    l.kind != Kind::CliffDrainage);
        require(l.reductionError >= 0.0001f && l.reductionError <= 0.5f);
        require(std::isfinite(l.phase) && std::abs(l.phase) <= 100 && l.profile >= 0 && l.profile < 3);
        require(l.profileSharpness >= 0.25f && l.profileSharpness <= 8 && l.distortion >= 0 &&
                l.distortion <= 4 && std::abs(l.solidOffset) <= 1);
        require(std::abs(l.amplitude) <= 5 && l.frequency >= 0.01f && l.frequency <= 100 && l.octaves >= 1 &&
                l.octaves <= 8);
        require(l.lacunarity >= 1 && l.lacunarity <= 4 && l.roughness >= 0 && l.roughness <= 1);
        require(l.count >= 1 && l.count <= 12 && l.thickness >= 0.02f && l.thickness <= 3 && l.spacing >= 0 &&
                l.spacing <= 2);
        require(l.iterations >= 0 && l.iterations <= 30 && l.relaxStrength >= 0 && l.relaxStrength <= 0.5f);
        require(l.width >= 0.001f && l.width <= 2 && l.threshold >= 0 && l.threshold <= 3 &&
                l.randomness >= 0 && l.randomness <= 1);
        require(l.voronoiMode >= 0 && l.voronoiMode <= 2 && l.jitter >= 0 && l.jitter <= 0.8f &&
                std::abs(l.layerScale) <= 0.15f && l.lateral >= 0 && l.lateral <= 2);
        require(l.maskHigh > l.maskLow);
        require(l.thicknessFrequency >= 0.01f && l.thicknessFrequency <= 10 && l.thicknessCorrelation >= 0 &&
                l.thicknessCorrelation <= 1);
        require(l.minimumThickness >= 0.01f && l.maximumThickness >= l.minimumThickness &&
                l.maximumThickness <= 3);
        if (l.coherentLayers)
            require(l.thickness >= l.minimumThickness && l.thickness <= l.maximumThickness);
        require(l.perimeterAmplitude >= 0 && l.perimeterAmplitude <= 0.3f);
        require(l.beddingTilt >= 0 && l.beddingTilt <= 0.25f);
        require(l.macroAxisAsymmetry >= 0 && l.macroAxisAsymmetry <= 0.45f);
        if (l.perimeterAmplitude > 0)
            require(l.kind == Kind::Layers && l.coherentLayers);
        if (l.beddingTilt > 0)
            require(l.kind == Kind::Layers && l.coherentLayers);
        if (l.macroAxisAsymmetry > 0)
            require(l.kind == Kind::Layers && l.coherentLayers);
        if (l.shapeOuterCaps)
            require(l.kind == Kind::Layers && l.coherentLayers && l.perimeterAmplitude > 0);
        require(l.driftCorrelation >= 0 && l.driftCorrelation <= 1 && std::abs(l.rotationDrift) <= 45 &&
                l.scaleDrift >= 0 && l.scaleDrift <= 0.3f);
        for (auto x : l.driftBias)
            require(std::abs(x) <= 1);
        require(l.envelopeShape >= 0 && l.envelopeShape < 4 && l.envelopeRoundness >= 0 &&
                l.envelopeRoundness <= 1);
        require(l.cutSubdivisions >= 1 && l.cutSubdivisions <= 4);
        for (auto x : l.scale)
            require(x >= 0.05f && x <= 5);
        for (auto x : l.coordinateScale)
            require(x >= 0.01f && x <= 10);
        for (auto x : l.outputScale)
            require(std::abs(x) <= 2);
        for (auto x : l.translation)
            require(std::abs(x) <= 20);
        for (auto x : l.rotation)
            require(std::abs(x) <= 360);
        for (auto x : l.direction)
            require(std::abs(x) <= 1);
        for (auto x : l.maskAxis)
            require(std::abs(x) <= 1);
    }
}
void normals(MeshResult &r)
{
    r.normals.assign(r.mesh.positions.size(), {});
    for (std::size_t t = 0; t < r.mesh.indices.size(); t += 3)
    {
        auto a = r.mesh.indices[t], b = r.mesh.indices[t + 1], c = r.mesh.indices[t + 2];
        auto n = cross(sub(r.mesh.positions[b], r.mesh.positions[a]),
                       sub(r.mesh.positions[c], r.mesh.positions[a]));
        r.normals[a] = add(r.normals[a], n);
        r.normals[b] = add(r.normals[b], n);
        r.normals[c] = add(r.normals[c], n);
    }
    for (auto &n : r.normals)
        n = unit(n);
}
void smoothMatchedLayerSeams(MeshResult &r)
{
    if (r.layerContactFaces.size() != r.mesh.indices.size() / 3 ||
        r.layerIndices.size() != r.mesh.positions.size() || r.layerIndices.empty())
        return;
    if (std::any_of(r.layerIndices.begin(), r.layerIndices.end(),
                    [](int layer) { return layer < 0 || layer >= 12; }))
        return;
    if (std::any_of(r.mesh.positions.begin(), r.mesh.positions.end(), [](Vec3d p)
                    { return std::max({std::abs(p.x), std::abs(p.y), std::abs(p.z)}) > 1e9; }))
        return;
    const int layers = *std::max_element(r.layerIndices.begin(), r.layerIndices.end()) + 1;
    if (layers < 2)
        return;
    using Key = std::tuple<long long, long long, long long>;
    using Triangle = std::array<Key, 3>;
    auto key = [](Vec3d p) -> Key
    {
        return {std::llround(p.x * 1e8), std::llround(p.y * 1e8), std::llround(p.z * 1e8)};
    };
    std::vector<std::multiset<Triangle>> tops(layers), bottoms(layers);
    std::vector<Vec3d> sideNormals(r.mesh.positions.size());
    std::vector<bool> seamVertex(r.mesh.positions.size());
    for (std::size_t t = 0; t < r.layerContactFaces.size(); ++t)
    {
        const auto a = r.mesh.indices[3 * t], b = r.mesh.indices[3 * t + 1],
                   c = r.mesh.indices[3 * t + 2];
        const int layer = r.layerIndices[a];
        if (r.layerIndices[b] != layer || r.layerIndices[c] != layer)
            return;
        const auto face = r.layerContactFaces[t];
        if (face == 1 || face == 2)
        {
            Triangle triangle{key(r.mesh.positions[a]), key(r.mesh.positions[b]),
                              key(r.mesh.positions[c])};
            std::sort(triangle.begin(), triangle.end());
            (face == 1 ? tops[layer] : bottoms[layer]).insert(triangle);
            if ((face == 1 && layer < layers - 1) || (face == 2 && layer > 0))
                for (auto i : {a, b, c})
                    seamVertex[i] = true;
        }
        else
        {
            const auto n = cross(sub(r.mesh.positions[b], r.mesh.positions[a]),
                                 sub(r.mesh.positions[c], r.mesh.positions[a]));
            for (auto i : {a, b, c})
                sideNormals[i] = add(sideNormals[i], n);
        }
    }
    for (int layer = 0; layer + 1 < layers; ++layer)
        if (tops[layer].empty() || tops[layer] != bottoms[layer + 1])
            return;
    std::map<Key, Vec3d> sharedNormals;
    for (std::size_t i = 0; i < seamVertex.size(); ++i)
        if (seamVertex[i] && dot(sideNormals[i], sideNormals[i]) > 1e-20)
        {
            auto &shared = sharedNormals[key(r.mesh.positions[i])];
            shared = add(shared, sideNormals[i]);
        }
    for (std::size_t i = 0; i < seamVertex.size(); ++i)
        if (seamVertex[i] && dot(sideNormals[i], sideNormals[i]) > 1e-20)
        {
            const auto shared = sharedNormals.at(key(r.mesh.positions[i]));
            if (dot(shared, shared) > 1e-20)
                r.normals[i] = unit(shared);
        }
}
void renderNormals(MeshResult &r, bool smoothPushBoundary,
                   const Jerico::CancellationToken *cancel)
{
    r.cornerNormals.clear();
    if (!r.hardRegions)
        return;
    std::map<std::pair<std::uint32_t, Jerico::Guid>, Vec3d> sums;
    for (std::size_t t = 0; t < r.mesh.indices.size(); t += 3)
    {
        if ((t % 768) == 0)
            checkCancel(cancel);
        auto a = r.mesh.indices[t], b = r.mesh.indices[t + 1], c = r.mesh.indices[t + 2];
        auto n = cross(sub(r.mesh.positions[b], r.mesh.positions[a]),
                       sub(r.mesh.positions[c], r.mesh.positions[a]));
        for (auto i : {a, b, c})
        {
            auto &sum = sums[{i, r.mesh.triangleRegions[t / 3]}];
            sum = add(sum, n);
        }
    }
    std::set<std::uint32_t> pushVertices;
    if (smoothPushBoundary)
    {
        std::set<Jerico::Guid> pushedRegions;
        for (const auto &region : r.regions)
            if (region.role == SurfaceRole::CutLoop)
                pushedRegions.insert(region.id);
        for (std::size_t t = 0; t < r.mesh.triangleRegions.size(); ++t)
            if (pushedRegions.contains(r.mesh.triangleRegions[t]))
                for (int corner = 0; corner < 3; ++corner)
                    pushVertices.insert(r.mesh.indices[3 * t + corner]);
    }
    r.cornerNormals.reserve(r.mesh.indices.size());
    for (std::size_t i = 0; i < r.mesh.indices.size(); ++i)
    {
        if ((i & 255) == 0)
            checkCancel(cancel);
        const auto vertex = r.mesh.indices[i];
        r.cornerNormals.push_back(pushVertices.contains(vertex) ? r.normals[vertex] :
            unit(sums.at({vertex, r.mesh.triangleRegions[i / 3]})));
    }
}
MeshResult evaluateImpl(const Recipe &recipe, const MeshResult *source, int resolution, int stopAfter,
                        int fieldLayer, const Jerico::CancellationToken *cancel)
{
    const auto start = std::chrono::steady_clock::now();
    validate(recipe);
    if (resolution < 4 || resolution > 64)
        throw std::runtime_error("Resolution must be 4..64");
    checkCancel(cancel);
    auto result = source ? *source
                         : (recipe.base == Base::BspWall ? bspWallSource(resolution, cancel)
                            : recipe.base == Base::Plane ? planeSource(recipe.planeSize, resolution)
                                                         : primitive(recipe.base, resolution));
    if (source)
    {
        if (result.mesh.positions.empty() || result.mesh.positions.size() > 240000 ||
            result.mesh.indices.empty() || result.mesh.indices.size() % 3 != 0 ||
            result.mesh.indices.size() > 720000 ||
            result.mesh.triangleRegions.size() != result.mesh.indices.size() / 3)
            throw std::runtime_error("Invalid source mesh shape or budget");
        for (auto i : result.mesh.indices)
            if (i >= result.mesh.positions.size())
                throw std::runtime_error("Source index out of range");
        for (auto p : result.mesh.positions)
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
                throw std::runtime_error("Non-finite source position");
        auto validAttribute = [&](const auto &values)
        {
            if (!values.empty() && values.size() != result.mesh.positions.size())
                throw std::runtime_error("Source metadata length does not match vertices");
            for (auto value : values)
                if (!std::isfinite(double(value)))
                    throw std::runtime_error("Non-finite source metadata");
        };
        validAttribute(result.layerIndices);
        validAttribute(result.layerThickness);
        validAttribute(result.layerEdgeDistance);
        if (result.closedSolid)
            Solid::validate(result.mesh);
        result.field.assign(result.mesh.positions.size(), 0);
        result.cornerNormals.clear();
        normals(result);
    }
    checkCancel(cancel);
    std::vector<Vec3d> warp(result.mesh.positions.size());
    std::vector<StageTiming> timings;
    timings.reserve(recipe.layers.size());
    struct StageTimer
    {
        StageTiming &record;
        MeshResult &result;
        std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
        ~StageTimer()
        {
            record.milliseconds =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            record.outputVertices = result.mesh.positions.size();
            record.outputTriangles = result.mesh.indices.size() / 3;
        }
    };
    std::vector<Vec3d> openReliefDirections;
    for (std::size_t stage = 0; stage < recipe.layers.size(); ++stage)
    {
        if (stopAfter >= 0 && int(stage) > stopAfter)
            break;
        checkCancel(cancel);
        const auto &l = recipe.layers[stage];
        if (!l.enabled)
            continue;
        const bool openRelief = !result.closedSolid &&
            (l.kind == Kind::StrataRelief || l.kind == Kind::RidgedNoise ||
             l.kind == Kind::NormalNoise);
        if (!openRelief)
            openReliefDirections.clear();
        else if (openReliefDirections.size() != result.mesh.positions.size())
            openReliefDirections = result.normals;
        timings.push_back({int(stage), l.kind, 0, result.mesh.positions.size()});
        StageTimer timer{timings.back(), result};
        if (l.kind == Kind::LayerSettle)
        {
            auto shifts = settleLayerStack(result, vec(l.direction), cancel);
            if (int(stage) == fieldLayer)
            {
                result.field.resize(result.mesh.positions.size());
                for (std::size_t i = 0; i < result.field.size(); ++i)
                    result.field[i] = float(std::abs(shifts[result.layerIndices[i]]));
            }
            continue;
        }
        if (l.kind == Kind::CliffCutLoop)
        {
            cliffCutLoop(result, warp, l, int(stage), int(stage) == fieldLayer, recipe.version >= 13, cancel);
            continue;
        }
        if (l.kind == Kind::CliffDrainage)
        {
            cliffDrainage(result, l, int(stage) == fieldLayer, cancel);
            continue;
        }
        if (l.kind == Kind::Reduce)
        {
            if (!result.closedSolid)
                throw std::runtime_error("Reduce requires Solidify before an open surface");
            MeshResult next;
            next.closedSolid = true;
            next.hardRegions = result.hardRegions;
            next.mesh = reduceClosedMesh(result.mesh, l.reductionError, next.sampledReductionError, cancel);
            next.trianglesBeforeReduction = result.mesh.indices.size() / 3;
            if (next.mesh.indices.size() == result.mesh.indices.size())
            {
                // The reducer returns the exact input when no candidate reduces it.
                // Preserve its samples and categorical metadata without reprojection.
                result.trianglesBeforeReduction = next.trianglesBeforeReduction;
                result.sampledReductionError = 0;
                continue;
            }
            next.regions = result.regions;
            next.sourceSelection = result.sourceSelection;
            next.sourceRevision = result.sourceRevision;
            std::vector<Vec3d> nextWarp;
            Solid::transferSamples(result, warp, next, nextWarp, cancel);
            normals(next);
            result = std::move(next);
            warp = std::move(nextWarp);
            continue;
        }
        if (l.kind == Kind::Solidify)
        {
            auto next = solidifySurface(result, l, cancel);
            auto prior = warp;
            warp.insert(warp.end(), prior.begin(), prior.end());
            for (auto &region : next.regions)
                if ((region.role == SurfaceRole::SolidBack || region.role == SurfaceRole::SolidRim) &&
                    region.operation < 0)
                    region.operation = int(stage);
            result = std::move(next);
            continue;
        }
        if (l.kind == Kind::Planes || l.kind == Kind::Envelope ||
            l.kind == Kind::FractureShear || l.kind == Kind::FractureQuadrants)
        {
            if (!result.closedSolid)
                throw std::runtime_error("Solid operation requires Solidify before an open surface");
            MeshResult next;
            next.mesh = l.kind == Kind::Planes ? Solid::planes(result.mesh, l, cancel)
                        : l.kind == Kind::FractureShear ? Solid::fractureShear(result.mesh, l, cancel)
                        : l.kind == Kind::FractureQuadrants ? Solid::fractureQuadrants(result.mesh, l, cancel)
                                                           : Solid::envelope(result.mesh, l, cancel);
            next.hardRegions = true;
            next.sourceSelection = result.sourceSelection;
            next.sourceRevision = result.sourceRevision;
            next.regions = result.regions;
            std::set<Jerico::Guid> original(result.mesh.triangleRegions.begin(),
                                            result.mesh.triangleRegions.end());
            std::set<Jerico::Guid> generated;
            std::set<Jerico::Guid> blockChisels;
            if (l.kind == Kind::FractureQuadrants)
                for (std::uint64_t block = 0; block < 4; ++block)
                    blockChisels.insert(Solid::region(
                        l.seed, PA::hashTriangleMesh(result.mesh) ^ (0x62000U + block)));
            for (auto id : next.mesh.triangleRegions)
                if (!original.contains(id) && generated.insert(id).second)
                    next.regions.push_back(
                        {id,
                         {},
                         l.kind == Kind::Planes ? SurfaceRole::PlaneCut
                         : l.kind == Kind::FractureShear ? SurfaceRole::FractureCut
                         : l.kind == Kind::FractureQuadrants && blockChisels.contains(id)
                             ? SurfaceRole::BlockChisel
                         : l.kind == Kind::FractureQuadrants ? SurfaceRole::FractureCut
                                                             : SurfaceRole::EnvelopeCut,
                         int(stage),
                         -1});
            std::vector<Vec3d> nextWarp;
            Solid::transferSamples(result, warp, next, nextWarp, cancel);
            if (int(stage) == fieldLayer)
            {
                next.field.assign(next.mesh.positions.size(), 0);
                for (std::size_t t = 0; t < next.mesh.triangleRegions.size(); ++t)
                    if (generated.contains(next.mesh.triangleRegions[t]))
                        for (int k = 0; k < 3; ++k)
                            next.field[next.mesh.indices[t * 3 + k]] = 1;
            }
            normals(next);
            result = std::move(next);
            warp = std::move(nextWarp);
            continue;
        }
        if (l.kind == Kind::Layers)
        {
            if (!result.closedSolid)
                throw std::runtime_error("Layer formation requires a solid; use Strata relief on a surface");
            if (l.coherentLayers)
            {
                std::vector<Vec3d> nextWarp;
                auto next =
                    coherentLayerFormation(result, l, int(stage) == fieldLayer, warp, nextWarp, cancel);
                for (auto &region : next.regions)
                    if (region.role == SurfaceRole::Stratum && region.operation < 0)
                        region.operation = int(stage);
                normals(next);
                result = std::move(next);
                warp = std::move(nextWarp);
                continue;
            }
            const auto input = result;
            const auto priorWarp = warp;
            if (input.mesh.positions.size() * l.count > 240000)
                throw std::runtime_error(
                    "Layer stack exceeds 240,000 vertices; reduce resolution or layer count");
            result = {};
            result.hardRegions = input.hardRegions;
            result.sourceSelection = input.sourceSelection;
            result.sourceRevision = input.sourceRevision;
            result.regions = input.regions;
            warp.clear();
            auto d = unit(vec(l.direction));
            auto u = unit(cross(std::abs(d.y) < 0.9 ? Vec3d{0, 1, 0} : Vec3d{1, 0, 0}, d)), v = cross(d, u);
            double lo = 1e20, hi = -1e20;
            for (auto p : input.mesh.positions)
            {
                lo = std::min(lo, dot(p, d));
                hi = std::max(hi, dot(p, d));
            }
            double cursor = 0;
            std::vector<double> thickness;
            for (int k = 0; k < l.count; ++k)
            {
                thickness.push_back(l.thickness * (1 + l.jitter * (2 * random(l.seed + k * 71) - 1)));
                cursor += thickness.back();
            }
            cursor = -0.5 * (cursor + (l.count - 1) * l.spacing);
            for (int k = 0; k < l.count; ++k)
            {
                checkCancel(cancel);
                auto offset = std::uint32_t(result.mesh.positions.size());
                auto lateral = add(mul(u, l.lateral * (2 * random(l.seed + k * 133 + 7) - 1)),
                                   mul(v, l.lateral * (2 * random(l.seed + k * 133 + 9) - 1)));
                for (std::size_t i = 0; i < input.mesh.positions.size(); ++i)
                {
                    auto p = input.mesh.positions[i];
                    double h = dot(p, d);
                    auto planar = mul(sub(p, mul(d, h)), std::max(0.15, 1.0 + k * l.layerScale));
                    result.mesh.positions.push_back(
                        add(add(planar, lateral),
                            mul(d, cursor + thickness[k] * (h - lo) / std::max(hi - lo, 1e-6))));
                    // Warp is a displacement vector in the current object space.
                    auto w = priorWarp[i];
                    double wh = dot(w, d);
                    warp.push_back(add(mul(sub(w, mul(d, wh)), std::max(0.15, 1.0 + k * l.layerScale)),
                                       mul(d, wh * thickness[k] / std::max(hi - lo, 1e-6))));
                    result.field.push_back(int(stage) == fieldLayer ? float(k) / std::max(1, l.count - 1)
                                                                    : input.field[i]);
                    result.layerIndices.push_back(k);
                    result.layerThickness.push_back(float(thickness[k]));
                }
                for (auto i : input.mesh.indices)
                    result.mesh.indices.push_back(i + offset);
                result.mesh.triangleRegions.insert(result.mesh.triangleRegions.end(),
                                                   input.mesh.triangleRegions.begin(),
                                                   input.mesh.triangleRegions.end());
                cursor += thickness[k] + l.spacing;
            }
            normals(result);
            continue;
        }
        SurfaceFeatureFields features;
        const bool needsFeatures = int(l.mask) >= int(Mask::Curvature) || l.kind == Kind::SurfaceDamage ||
                                   l.kind == Kind::DamageDecal;
        auto updateFeatures = [&]()
        {
            if (needsFeatures)
                features = surfaceFeatures(result.mesh,
                                           {.contact = l.mask == Mask::Contact,
                                            .ambient = l.mask == Mask::AmbientOcclusion,
                                            .thickness = l.mask == Mask::Thickness,
                                            .radius = l.featureRadius,
                                            .creaseAngleDegrees = l.featureAngle,
                                            .contactSamples = l.contactSamples,
                                            .ambientRadius = l.featureRadius,
                                            .ambientSamples = l.contactSamples,
                                            .thicknessRadius = l.featureRadius,
                                            .exposureDirection = vec(l.direction)},
                                           cancel);
        };
        if (l.kind == Kind::Relax)
        {
            std::vector<std::set<std::uint32_t>> adjacency(result.mesh.positions.size());
            for (std::size_t t = 0; t < result.mesh.indices.size(); t += 3)
            {
                if ((t % 768) == 0)
                    checkCancel(cancel);
                for (int k = 0; k < 3; ++k)
                {
                    auto a = result.mesh.indices[t + k], b = result.mesh.indices[t + (k + 1) % 3];
                    adjacency[a].insert(b);
                    adjacency[b].insert(a);
                }
            }
            for (int k = 0; k < l.iterations; ++k)
            {
                checkCancel(cancel);
                updateFeatures();
                auto previous = result.mesh.positions;
                for (std::size_t i = 0; i < previous.size(); ++i)
                {
                    if ((i & 255) == 0)
                        checkCancel(cancel);
                    Vec3d average{};
                    for (auto j : adjacency[i])
                        average = add(average, previous[j]);
                    if (!adjacency[i].empty())
                        result.mesh.positions[i] =
                            add(previous[i], mul(sub(mul(average, 1.0 / adjacency[i].size()), previous[i]),
                                                 l.relaxStrength * maskValue(previous[i], result.normals[i],
                                                                             l, &features, i)));
                    if (int(stage) == fieldLayer)
                    {
                        auto delta = sub(result.mesh.positions[i], previous[i]);
                        result.field[i] =
                            l.mask == Mask::None
                                ? float(std::sqrt(dot(delta, delta)))
                                : float(maskValue(previous[i], result.normals[i], l, &features, i));
                    }
                }
                normals(result);
            }
            continue;
        }
        if (l.kind == Kind::EdgeBreakup)
            layerEdgeDistances(result, l, cancel);
        updateFeatures();
        const auto beforeDamage = l.kind == Kind::SurfaceDamage || l.kind == Kind::DamageDecal
                                      ? result.mesh.positions
                                      : std::vector<Vec3d>{};
        DamageProjector damageProjector;
        DamageDecalSettings decalSettings;
        if (l.kind == Kind::DamageDecal)
        {
            damageProjector.center = vec(l.translation);
            damageProjector.normal = unit(vec(l.direction));
            damageProjector.tangent =
                unit(cross(std::abs(damageProjector.normal.y) < 0.9 ? Vec3d{0, 1, 0} : Vec3d{1, 0, 0},
                           damageProjector.normal));
            damageProjector.bitangent = cross(damageProjector.normal, damageProjector.tangent);
            damageProjector.rotationRadians = l.projectorRotation * std::numbers::pi / 180;
            decalSettings.radius = {l.projectorRadius[0], l.projectorRadius[1], 1};
            decalSettings.innerRadius = l.threshold;
            decalSettings.outerRadius = l.threshold + l.width;
            decalSettings.depth = l.amplitude;
            decalSettings.profilePower = l.profileSharpness;
            decalSettings.lipInner = std::max(0.0f, l.threshold * 0.9f);
            decalSettings.lipOuter = 1 + 0.5 * l.width;
            decalSettings.lipHeight = l.damageLip;
        }
        for (std::size_t i = 0; i < result.mesh.positions.size(); ++i)
        {
            if ((i & 255) == 0)
                checkCancel(cancel);
            auto p = result.mesh.positions[i], n = result.normals[i], q = add(p, warp[i]);
            double mask = maskValue(q, n, l, &features, i), value = 0;
            if (l.kind == Kind::Transform)
            {
                result.mesh.positions[i] =
                    add(rotate(product(p, vec(l.scale)), l.rotation), vec(l.translation));
                warp[i] = rotate(product(warp[i], vec(l.scale)), l.rotation);
                value = mask;
            }
            else if (l.kind == Kind::Warp)
            {
                auto w = mul(product(vectorNoise(q, l), vec(l.outputScale)), l.amplitude * mask);
                warp[i] = add(warp[i], w);
                value = std::sqrt(dot(w, w));
            }
            else if (l.kind == Kind::SurfaceDamage)
            {
                value = smooth((0.5 + 0.5 * fbm(q, l) - l.threshold) / l.width);
                double depth = std::min(double(l.amplitude), 0.2 * features.minimumTriangleAltitude[i]);
                if (result.layerThickness.size() == result.mesh.positions.size())
                    depth = std::min(depth, 0.2 * result.layerThickness[i]);
                result.mesh.positions[i] = add(p, mul(n, -depth * value * mask));
            }
            else if (l.kind == Kind::DamageDecal)
            {
                auto projectorPosition = projectorCoordinates(damageProjector, p);
                auto sample = sampleDamageDecal(projectorPosition, decalSettings);
                const bool front = std::abs(projectorPosition.z) <= l.featureRadius &&
                                   dot(n, damageProjector.normal) > 0;
                value = front ? sample.softMask : 0;
                auto displacement = front ? (sample.indentation + sample.lipDisplacement) * mask : 0;
                const auto direction = recipe.version >= 13 && !l.solidAlongNormals ? damageProjector.normal : n;
                result.mesh.positions[i] = add(p, mul(direction, displacement));
            }
            else if (l.kind == Kind::EdgeBreakup)
            {
                value = 1 - smooth(result.layerEdgeDistance[i] / l.width);
                double strength = l.amplitude;
                if (result.layerThickness.size() == result.mesh.positions.size())
                    strength =
                        std::clamp(strength, -0.2 * result.layerThickness[i], 0.2 * result.layerThickness[i]);
                result.mesh.positions[i] = add(p, mul(n, fbm(q, l) * strength * value * mask));
            }
            else if (l.kind == Kind::StrataRelief)
            {
                double s = dot(q, unit(vec(l.direction))) * l.frequency + l.phase +
                           l.distortion * fbm(mul(q, 0.3), l);
                double u = s - std::floor(s);
                value = l.profile == 0   ? 0.5 + 0.5 * std::sin(2 * std::numbers::pi * u)
                        : l.profile == 1 ? smooth(u / l.width) * (1 - smooth((u - 0.72) / 0.28))
                                         : 1 - std::abs(2 * u - 1);
                value = std::pow(std::clamp(value, 0.0, 1.0), l.profileSharpness);
                auto direction = openReliefDirections.empty() ? n : openReliefDirections[i];
                result.mesh.positions[i] = add(p, mul(direction, l.amplitude * (value - 0.5) * mask));
            }
            else if (l.kind == Kind::RidgedNoise)
            {
                double sum = 0, weights = 0, amplitude = 1, frequency = l.frequency;
                for (int octave = 0; octave < l.octaves; ++octave)
                {
                    double noise = PA::evaluateGeologyFbmReference(
                        product(mul(q, frequency), vec(l.coordinateScale)), l.seed + octave * 101, 1, 2, 0.5);
                    sum += amplitude * std::pow(std::max(0.0, 1 - std::abs(noise)), l.profileSharpness);
                    weights += amplitude;
                    amplitude *= l.roughness;
                    frequency *= l.lacunarity;
                }
                value = sum / weights;
                auto direction = openReliefDirections.empty() ? n : openReliefDirections[i];
                result.mesh.positions[i] = add(p, mul(direction, l.amplitude * (value - 0.5) * mask));
            }
            else if (l.kind == Kind::NormalNoise)
            {
                value = fbm(q, l);
                auto direction = openReliefDirections.empty() ? n : openReliefDirections[i];
                result.mesh.positions[i] = add(p, mul(direction, l.amplitude * value * mask));
            }
            else if (l.kind == Kind::VectorNoise || l.kind == Kind::SharedWarp)
            {
                auto field = product(vectorNoise(q, l), vec(l.outputScale));
                value = l.kind == Kind::SharedWarp ? std::sqrt(dot(field, field)) * l.amplitude : field.x;
                result.mesh.positions[i] = add(p, mul(field, l.amplitude * mask));
            }
            else
            {
                value = voronoi(q, l);
                double amount = value - 0.5;
                if (l.kind == Kind::Fracture || l.voronoiMode == 2)
                    amount = -(1 - smooth((value - l.threshold) / l.width));
                result.mesh.positions[i] = add(p, mul(n, l.amplitude * amount * mask));
            }
            if (int(stage) == fieldLayer)
                result.field[i] = float(l.mask != Mask::None ? mask : value);
        }
        if (l.kind == Kind::SurfaceDamage || l.kind == Kind::DamageDecal)
            for (std::size_t t = 0; t < result.mesh.indices.size(); t += 3)
            {
                if (t % 768 == 0)
                    checkCancel(cancel);
                auto a = result.mesh.indices[t], b = result.mesh.indices[t + 1],
                     c = result.mesh.indices[t + 2];
                auto oldNormal =
                    cross(sub(beforeDamage[b], beforeDamage[a]), sub(beforeDamage[c], beforeDamage[a]));
                auto newNormal = cross(sub(result.mesh.positions[b], result.mesh.positions[a]),
                                       sub(result.mesh.positions[c], result.mesh.positions[a]));
                if (dot(oldNormal, newNormal) <= 1e-10 * dot(oldNormal, oldNormal))
                    throw std::runtime_error(
                        "Surface displacement folds face " + std::to_string(t / 3) + " at operation " +
                        std::to_string(stage) + "; orientation ratio=" +
                        std::to_string(dot(oldNormal, newNormal) / dot(oldNormal, oldNormal)) +
                        ", shortest edge=" +
                        std::to_string(std::min({features.minimumEdgeLength[a], features.minimumEdgeLength[b],
                                                 features.minimumEdgeLength[c]})) +
                        ", minimum altitude=" +
                        std::to_string(std::sqrt(dot(oldNormal, oldNormal)) /
                                       std::max({std::sqrt(dot(sub(beforeDamage[b], beforeDamage[a]),
                                                               sub(beforeDamage[b], beforeDamage[a]))),
                                                 std::sqrt(dot(sub(beforeDamage[c], beforeDamage[a]),
                                                               sub(beforeDamage[c], beforeDamage[a]))),
                                                 std::sqrt(dot(sub(beforeDamage[c], beforeDamage[b]),
                                                               sub(beforeDamage[c], beforeDamage[b])))})) +
                        "; reduce depth or improve input sampling");
            }
        if (l.kind != Kind::Warp)
            normals(result);
    }
    for (auto p : result.mesh.positions)
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
            throw std::runtime_error("Operation produced non-finite geometry");
    smoothMatchedLayerSeams(result);
    const bool smoothPushBoundary = recipe.version >= 13 && std::any_of(
        recipe.layers.begin(), recipe.layers.end(), [](const Layer &layer)
        { return layer.enabled && layer.kind == Kind::CliffCutLoop && layer.profile == 2; });
    renderNormals(result, smoothPushBoundary, cancel);
    result.stages = std::move(timings);
    result.milliseconds =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return result;
}
MeshResult evaluate(const Recipe &recipe, int resolution, int stopAfter, int fieldLayer,
                    const Jerico::CancellationToken *cancel)
{
    return evaluateImpl(recipe, nullptr, resolution, stopAfter, fieldLayer, cancel);
}
MeshResult evaluateSource(const Recipe &recipe, const MeshResult &source, int resolution, int stopAfter,
                          int fieldLayer, const Jerico::CancellationToken *cancel)
{
    return evaluateImpl(recipe, &source, resolution, stopAfter, fieldLayer, cancel);
}
std::string serialize(const Recipe &r)
{
    validate(r);
    Json j = {{"version", r.version},
              {"base", int(r.base)},
              {"previewResolution", r.previewResolution},
              {"finalResolution", r.finalResolution},
              {"planeSize", r.planeSize},
              {"layers", Json::array()}};
    for (auto &l : r.layers)
        j["layers"].push_back(layerJson(l));
    return j.dump(2);
}
Recipe deserialize(const std::string &text)
{
    if (text.size() > 1024 * 1024)
        throw std::runtime_error("Recipe file too large");
    auto j = Json::parse(text);
    Recipe r;
    int inputVersion = j.at("version");
    if (inputVersion != 1 && inputVersion != 2 && inputVersion != 3 && inputVersion != 4 &&
        inputVersion != 5 && inputVersion != 6 && inputVersion != 7 && inputVersion != 8 &&
        inputVersion != 9 && inputVersion != 10 && inputVersion != 11 && inputVersion != 12 &&
        inputVersion != 13)
        throw std::runtime_error("Unsupported recipe version");
    if (inputVersion >= 11)
        r.version = inputVersion;
    r.base = static_cast<Base>(j.at("base").get<int>());
    r.previewResolution = j.at("previewResolution");
    r.finalResolution = j.at("finalResolution");
    if (inputVersion >= 3 && j.contains("planeSize"))
        j.at("planeSize").get_to(r.planeSize);
    if (inputVersion < 3 && int(r.base) > 3)
        throw std::runtime_error("Plane source requires recipe v3");
    if (!j.at("layers").is_array() || j.at("layers").size() > 24)
        throw std::runtime_error("Invalid layer array");
    for (auto &a : j.at("layers"))
    {
        Layer l;
        l.kind = static_cast<Kind>(a.at("kind").get<int>());
        if (inputVersion == 1 && int(l.kind) >= 8)
            throw std::runtime_error("Recipe v1 contains a v2 operation");
        if (inputVersion < 3 && int(l.kind) >= 12)
            throw std::runtime_error("Operation requires recipe v3");
        if (inputVersion < 4 && (int(l.kind) >= 16 || a.at("mask").get<int>() >= 7))
            throw std::runtime_error("Surface features require recipe v4");
        if (inputVersion < 5 && (a.at("mask").get<int>() >= 10 || int(l.kind) >= 17))
            throw std::runtime_error("Extended surface fields require recipe v5");
        if (inputVersion < 6 && int(l.kind) >= 18)
            throw std::runtime_error("Layer settling requires recipe v6");
        if (inputVersion < 10 && l.kind == Kind::CliffCutLoop)
            throw std::runtime_error("Cliff cut loop requires recipe v10");
        if (inputVersion < 11 && l.kind == Kind::FractureShear)
            throw std::runtime_error("Grounded fracture shear requires recipe v11");
        if (inputVersion < 12 && l.kind == Kind::FractureQuadrants)
            throw std::runtime_error("Four-block chisel requires recipe v12");
        if (inputVersion < 13 && l.kind == Kind::CliffDrainage)
            throw std::runtime_error("Cliff drainage requires recipe v13");
        if (inputVersion < 7 && a.value("perimeterAmplitude", 0.0f) != 0)
            throw std::runtime_error("Independent stratum perimeter requires recipe v7");
        if (inputVersion < 8 && a.contains("maskAxis") &&
            a.at("maskAxis").get<std::array<float, 3>>() != std::array<float, 3>{})
            throw std::runtime_error("Independent mask axis requires recipe v8");
        if (inputVersion < 9 && a.value("shapeOuterCaps", false))
            throw std::runtime_error("Outer stratum contour requires recipe v9");
#define READ(member) a.at(#member).get_to(l.member)
        READ(enabled);
        if (!a.at("seed").is_number_integer() || a.at("seed").get<double>() < 0 ||
            a.at("seed").get<double>() > 4294967295.0)
            throw std::runtime_error("Seed must be an unsigned 32-bit integer");
        READ(seed);
        READ(amplitude);
        READ(frequency);
        READ(lacunarity);
        READ(roughness);
        READ(octaves);
        READ(coordinateScale);
        if (inputVersion >= 2 && a.contains("outputScale"))
            READ(outputScale);
        READ(scale);
        READ(rotation);
        READ(translation);
        READ(direction);
        if (inputVersion >= 8)
            READ(maskAxis);
        READ(voronoiMode);
        READ(randomness);
        READ(threshold);
        READ(width);
        READ(count);
        READ(thickness);
        READ(spacing);
        READ(layerScale);
        if (inputVersion >= 7)
            READ(perimeterAmplitude);
        if (inputVersion >= 9)
            READ(shapeOuterCaps);
        READ(jitter);
        READ(lateral);
        READ(iterations);
        READ(relaxStrength);
        READ(maskLow);
        READ(maskHigh);
        READ(maskInvert);
        if (inputVersion >= 2)
        {
#define READ_OPTIONAL(member)                                                                                \
    if (a.contains(#member))                                                                                 \
    READ(member)
            READ_OPTIONAL(coherentLayers);
            READ_OPTIONAL(thicknessFrequency);
            READ_OPTIONAL(beddingTilt);
            READ_OPTIONAL(macroAxisAsymmetry);
            READ_OPTIONAL(thicknessCorrelation);
            READ_OPTIONAL(minimumThickness);
            READ_OPTIONAL(maximumThickness);
            READ_OPTIONAL(driftCorrelation);
            READ_OPTIONAL(rotationDrift);
            READ_OPTIONAL(scaleDrift);
            READ_OPTIONAL(driftBias);
            READ_OPTIONAL(envelopeShape);
            READ_OPTIONAL(envelopeRoundness);
            READ_OPTIONAL(cutSubdivisions);
            if (inputVersion >= 3)
            {
                READ_OPTIONAL(phase);
                READ_OPTIONAL(profile);
                READ_OPTIONAL(profileSharpness);
                READ_OPTIONAL(distortion);
                READ_OPTIONAL(solidOffset);
                READ_OPTIONAL(solidAlongNormals);
                READ_OPTIONAL(reductionError);
            }
            if (inputVersion >= 4)
            {
                READ(featureRadius);
                READ(featureAngle);
                READ(contactSamples);
            }
            if (inputVersion >= 5)
            {
                READ_OPTIONAL(projectorRadius);
                READ_OPTIONAL(projectorRotation);
                READ_OPTIONAL(damageLip);
            }
            if (inputVersion >= 13)
                READ_OPTIONAL(alphaSource);
#undef READ_OPTIONAL
        }
        l.mask = static_cast<Mask>(a.at("mask").get<int>());
#undef READ
        r.layers.push_back(l);
    }
    validate(r);
    return r;
}
void save(const Recipe &r, const std::filesystem::path &path)
{
    auto text = serialize(r);
    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out || !(out << text << '\n'))
            throw std::runtime_error("Could not write recipe");
        out.flush();
        if (!out)
            throw std::runtime_error("Could not flush recipe");
    }
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Could not replace recipe file");
}
Recipe load(const std::filesystem::path &path)
{
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("Recipe file too large");
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Could not open recipe");
    return deserialize(std::string(std::istreambuf_iterator<char>(in), {}));
}
std::uint64_t meshHash(const TriangleMesh &mesh)
{
    std::uint64_t h = 1469598103934665603ULL;
    auto append = [&](std::uint64_t x)
    {
        for (int k = 0; k < 8; ++k)
        {
            h ^= (x >> (8 * k)) & 255;
            h *= 1099511628211ULL;
        }
    };
    for (auto p : mesh.positions)
    {
        append(std::bit_cast<std::uint64_t>(p.x));
        append(std::bit_cast<std::uint64_t>(p.y));
        append(std::bit_cast<std::uint64_t>(p.z));
    }
    for (auto i : mesh.indices)
        append(i);
    return h;
}
} // namespace Playground
