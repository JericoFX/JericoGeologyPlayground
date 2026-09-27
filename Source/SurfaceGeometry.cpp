#include "SurfaceGeometry.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/AssetPipeline/RBrushHash.hpp>
#include <Jerico/AssetPipeline/RBrushImageImporter.hpp>
#include <Jerico/ProceduralAuthoring/Geology/GeologyNoise.hpp>
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <numbers>
#include <set>
#include <sstream>
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
struct CliffAlphaTip
{
    std::uint32_t width{}, height{};
    std::vector<std::byte> pixels;
};
const CliffAlphaTip &cliffAlphaTip()
{
    static const CliffAlphaTip tip = []
    {
        namespace A = Jerico::Tools::AssetPipeline;
        const auto path = std::filesystem::path(PLAYGROUND_ROOT) /
                          "Evidence/cc0-references/cliff_side-disp-1k.png";
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw std::runtime_error("CC0 cliff RBrush alpha input is missing");
        const auto size = file.tellg();
        if (size <= 0 || size > 8 * 1024 * 1024)
            throw std::runtime_error("CC0 cliff RBrush alpha size is invalid");
        std::vector<std::byte> encoded(std::size_t(size), std::byte{0});
        file.seekg(0);
        file.read(reinterpret_cast<char *>(encoded.data()), size);
        if (!file)
            throw std::runtime_error("CC0 cliff RBrush alpha read failed");
        std::ostringstream digest;
        digest << std::hex << std::setfill('0');
        for (auto byte : A::hashRBrushBytes(encoded))
            digest << std::setw(2) << std::to_integer<unsigned>(byte);
        if (digest.str() != "2861630bab9f8a17014915ff624be83281185b04ff3f4b64a123db60d5e160d3")
            throw std::runtime_error("CC0 cliff RBrush alpha provenance changed");
        Jerico::Guid assetGuid;
        assetGuid.bytes[0] = std::byte{0x71};
        A::RBrushImageProcessing processing;
        processing.canvasMode = A::RBrushImageCanvasMode::PreserveOriginal;
        processing.bitDepth = A::RBrushImageBitDepth::ForceR8;
        processing.padding = 0;
        auto imported = A::importRBrushImages(
            {.assetGuid = assetGuid,
             .sources = {{.logicalName = "cliff_side-disp-1k.png",
                          .encodedBytes = std::move(encoded),
                          .extractions = {{.channel = A::RBrushImageChannel::Luminance,
                                           .presetName = "CC0 cliff push alpha",
                                           .processing = processing}}}}});
        if (!imported.hasValue() || imported.value().tips.size() != 1)
            throw std::runtime_error("Official RBrush cliff alpha import failed");
        const auto &source = imported.value().tips.front();
        if (source.width != 1024 || source.height != 1024 ||
            source.pixelFormat != A::RBrushPixelFormat::R8Unorm)
            throw std::runtime_error("Official RBrush cliff alpha has unexpected dimensions");
        return CliffAlphaTip{source.width, source.height, source.pixels};
    }();
    return tip;
}
double sampleCliffAlpha(double u, double v)
{
    const auto &tip = cliffAlphaTip();
    const double x = std::clamp((u / 2.6 + 0.5) * (tip.width - 1), 0.0, double(tip.width - 1));
    const double y = std::clamp((v / 2.6 + 0.5) * (tip.height - 1), 0.0, double(tip.height - 1));
    const auto x0 = std::uint32_t(x), y0 = std::uint32_t(y);
    const auto x1 = std::min(x0 + 1, tip.width - 1), y1 = std::min(y0 + 1, tip.height - 1);
    auto pixel = [&](std::uint32_t px, std::uint32_t py)
    { return double(std::to_integer<unsigned>(tip.pixels[py * tip.width + px])) / 255.0; };
    const double a = std::lerp(pixel(x0, y0), pixel(x1, y0), x - x0);
    const double b = std::lerp(pixel(x0, y1), pixel(x1, y1), x - x0);
    return std::lerp(a, b, y - y0);
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
MeshResult caveSource(CaveForm form, int subdivisions, const Jerico::CancellationToken *cancel)
{
    const auto massId = Solid::region(0xca7eU, 1);
    const auto tunnelId = Solid::region(0xca7eU, form == CaveForm::Arch ? 2 :
                                      form == CaveForm::BlindArch ? 5 : 3);
    const std::array<double, 16> identity{
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    PA::BrushNode mass{massId, PA::BrushBoolean::Add, PA::BoxBrush{{3, 2, 1.5}}, identity};
    PA::BrushNode cutter{tunnelId, PA::BrushBoolean::Subtract, {}, identity};
    auto extrude = [](const std::vector<std::array<double, 2>> &contour, double depth)
    {
        PA::ConvexBrush brush;
        const auto count = std::uint32_t(contour.size());
        for (double z : {-depth, depth})
            for (auto [x, y] : contour)
                brush.vertices.push_back({x, y, z});
        std::vector<std::uint32_t> front, back;
        for (std::uint32_t i = 0; i < count; ++i)
        {
            front.push_back(count - 1 - i);
            back.push_back(count + i);
            brush.faces.push_back({i, (i + 1) % count, count + (i + 1) % count, count + i});
        }
        brush.faces.push_back(std::move(front));
        brush.faces.push_back(std::move(back));
        return brush;
    };
    if (form == CaveForm::SquareTunnel)
    {
        cutter.shape = PA::BoxBrush{{0.9, 1.1, 2.0}};
        cutter.localTransform[13] = -0.6;
    }
    else if (form == CaveForm::Arch || form == CaveForm::BlindArch)
    {
        mass.shape = extrude({{-2.8, -2}, {2.65, -2}, {3, -1.5}, {3, 0.9},
                              {2.35, 1.75}, {1.5, 2}, {-1.9, 2}, {-2.75, 1.35},
                              {-3, 0.2}, {-3, -1.25}}, 1.5);
        constexpr int segments = 8;
        constexpr double radius = 1.15, spring = -0.45, bottom = -1.7, depth = 2.0;
        std::vector<std::array<double, 2>> contour{{-radius, bottom}, {radius, bottom},
                                                    {radius, spring}};
        for (int step = 1; step < segments; ++step)
        {
            const double angle = std::numbers::pi * step / segments;
            contour.push_back({radius * std::cos(angle), spring + radius * std::sin(angle)});
        }
        contour.push_back({-radius, spring});
        cutter.shape = extrude(contour, depth);
        if (form == CaveForm::BlindArch)
            cutter.localTransform[14] = 0.9;
    }
    else
        throw std::runtime_error("Unknown cave form");
    PA::BspDocument document{Solid::region(0xca7eU, 4), 1, {mass, cutter}};
    const std::array selected{massId, tunnelId};
    return bspSource(document, selected, subdivisions, cancel);
}
MeshResult geologicalCaveSource(CaveForm form, int subdivisions,
                                const Jerico::CancellationToken *cancel)
{
    if (form == CaveForm::SquareTunnel)
        return squareTunnelReliefSource(subdivisions, cancel);
    auto source = caveSource(form, subdivisions, cancel);
    Recipe recipe;
    auto macro = defaultLayer(Kind::VectorNoise);
    macro.seed = 123;
    macro.amplitude = 0.45f;
    macro.frequency = 0.55f;
    macro.octaves = 3;
    macro.lacunarity = 1.87f;
    macro.coordinateScale = {0.65f, 0.7f, 0.8f};
    macro.outputScale = {0.6f, 0.5f, 1.0f};
    recipe.layers = {macro};
    return evaluateSource(recipe, source, std::max(4, subdivisions), -1, -1, cancel);
}
MeshResult axialCaveSource(CaveForm form, int subdivisions,
                           const Jerico::CancellationToken *cancel)
{
    auto source = geologicalCaveSource(form, subdivisions, cancel);
    if (form == CaveForm::SquareTunnel)
        return source;
    auto bell = [](double z, double center, double radius)
    {
        const double q = (z - center) / radius;
        return std::exp(-q * q);
    };
    for (std::size_t i = 0; i < source.mesh.positions.size(); ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        auto &p = source.mesh.positions[i];
        const double axial = std::clamp(p.z / 1.5, -1.0, 1.0);
        const double mouth = std::clamp((axial - 0.35) / 0.65, 0.0, 1.0);
        const double flare = mouth * mouth * (3.0 - 2.0 * mouth);
        const double throat = bell(axial, 0.2, 0.28);
        const double chamber = bell(axial, -0.55, 0.48);
        const double width = 1.0 + 0.30 * flare - 0.22 * throat + 0.32 * chamber;
        const double height = 1.0 + 0.10 * flare - 0.08 * throat + 0.15 * chamber;
        p.x = p.x * width + 0.10 * std::sin(std::numbers::pi * (axial + 1.0) * 0.5);
        p.y = -0.5 + (p.y + 0.5) * height;
    }
    normals(source);
    return source;
}
MeshResult squareTunnelReliefSource(int subdivisions, const Jerico::CancellationToken *cancel)
{
    auto source = caveSource(CaveForm::SquareTunnel, subdivisions, cancel);
    std::vector<Vec3d> cutterNormals(source.mesh.positions.size());
    const auto cutterId = Solid::region(0xca7eU, 3);
    std::set<Jerico::Guid> cutterFaceIds;
    for (std::uint64_t face = 0; face < 6; ++face)
        cutterFaceIds.insert(PA::deriveFaceRegionGuid(cutterId, face));
    std::size_t interiorFaces = 0;
    for (std::size_t t = 0; t < source.mesh.indices.size(); t += 3)
    {
        if ((t & 767) == 0)
            check(cancel);
        if (!cutterFaceIds.contains(source.mesh.triangleRegions[t / 3]))
            continue;
        const auto a = source.mesh.positions[source.mesh.indices[t]],
                   b = source.mesh.positions[source.mesh.indices[t + 1]],
                   c = source.mesh.positions[source.mesh.indices[t + 2]];
        ++interiorFaces;
        const Vec3d u{b.x - a.x, b.y - a.y, b.z - a.z},
                    v{c.x - a.x, c.y - a.y, c.z - a.z};
        const Vec3d n{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
                      u.x * v.y - u.y * v.x};
        for (int k = 0; k < 3; ++k)
        {
            auto &sum = cutterNormals[source.mesh.indices[t + k]];
            sum.x += n.x;
            sum.y += n.y;
            sum.z += n.z;
        }
    }
    if (!interiorFaces)
        throw std::runtime_error("Square tunnel has no interior cutter faces");
    for (std::size_t i = 0; i < source.mesh.positions.size(); ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        auto &p = source.mesh.positions[i];
        const auto &sum = cutterNormals[i];
        const double length = std::sqrt(sum.x * sum.x + sum.y * sum.y + sum.z * sum.z);
        if (length <= 1e-12)
            continue;
        const double rim = std::clamp((1.5 - std::abs(p.z)) / 0.45, 0.0, 1.0);
        const double axial = rim * rim * (3.0 - 2.0 * rim);
        if (axial <= 0)
            continue;
        const Vec3d n{sum.x / length, sum.y / length, sum.z / length};
        const double wall = std::max(std::abs(n.x), 0.7 * std::abs(n.y));
        const double broad = PA::evaluateGeologyFbmReference(
            {1.1 * p.x + 2.3, 1.5 * p.y - 0.8, 1.7 * p.z + 0.4},
            0x713a64e1U, 3, 1.87, 0.5);
        const double fine = PA::evaluateGeologyFbmReference(
            {3.7 * p.x - 1.2, 3.1 * p.y + 0.5, 3.3 * p.z + 1.1},
            0x25cb49dfU, 2, 1.87, 0.5);
        const double ridge = std::sin(2.8 * p.z + 1.7 * p.y + 0.8 * broad);
        const double side = std::abs(n.x);
        const double shear = std::tanh((p.z - 0.35 * p.y -
                                        (n.x < 0 ? -0.2 : 0.2)) / 0.95);
        const double bedding = std::sin(1.2 * p.z + 0.9 * p.y +
                                        (n.x < 0 ? -0.7 : 0.7));
        const double distance = axial * wall *
            std::clamp(0.21 * broad + 0.045 * fine + 0.035 * ridge +
                       side * (0.15 * shear + 0.08 * bedding), -0.27, 0.27);
        p.x += n.x * distance;
        p.y += n.y * distance;
        p.z += n.z * distance;
    }
    normals(source);
    return source;
}
MeshResult selectedGeologicalRelief(MeshResult source,
                                   std::span<const Jerico::Guid> selectedRegions,
                                   double amplitude, std::uint32_t seed,
                                   const Jerico::CancellationToken *cancel)
{
    check(cancel);
    if (!std::isfinite(amplitude) || amplitude <= 0 || amplitude > 1 ||
        selectedRegions.empty())
        throw std::runtime_error("Selected geology needs face GUIDs and finite amplitude in (0, 1]");
    std::set<Jerico::Guid> selection(selectedRegions.begin(), selectedRegions.end());
    if (selection.size() != selectedRegions.size())
        throw std::runtime_error("Selected geology has duplicate face GUIDs");
    const auto &mesh = source.mesh;
    std::set<Jerico::Guid> found;
    std::vector<Vec3d> selectedNormals(mesh.positions.size());
    std::vector<unsigned char> protectedVertex(mesh.positions.size());
    for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
    {
        if ((t & 767) == 0)
            check(cancel);
        const auto id = mesh.triangleRegions[t / 3];
        if (!selection.contains(id))
        {
            for (int corner = 0; corner < 3; ++corner)
                protectedVertex[mesh.indices[t + corner]] = 1;
            continue;
        }
        found.insert(id);
        const auto a = mesh.positions[mesh.indices[t]],
                   b = mesh.positions[mesh.indices[t + 1]],
                   c = mesh.positions[mesh.indices[t + 2]];
        const Vec3d u{b.x - a.x, b.y - a.y, b.z - a.z},
                    v{c.x - a.x, c.y - a.y, c.z - a.z};
        const Vec3d n{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
                      u.x * v.y - u.y * v.x};
        for (int corner = 0; corner < 3; ++corner)
        {
            auto &sum = selectedNormals[mesh.indices[t + corner]];
            sum.x += n.x;
            sum.y += n.y;
            sum.z += n.z;
        }
    }
    if (found != selection)
        throw std::runtime_error("Selected geology references an absent face GUID");
    std::vector<Vec3d> displacements(mesh.positions.size());
    const bool hasProtected = std::any_of(protectedVertex.begin(), protectedVertex.end(),
                                          [](unsigned char value) { return value != 0; });
    for (std::size_t i = 0; i < mesh.positions.size(); ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        if (protectedVertex[i])
            continue;
        const auto &p = mesh.positions[i], &sum = selectedNormals[i];
        const double length = std::sqrt(sum.x * sum.x + sum.y * sum.y + sum.z * sum.z);
        if (length <= 1e-12)
            continue;
        double distance = 1;
        if (hasProtected)
            for (std::size_t j = 0; j < mesh.positions.size(); ++j)
                if (protectedVertex[j])
                {
                    const auto &q = mesh.positions[j];
                    const double dx = p.x - q.x, dy = p.y - q.y, dz = p.z - q.z;
                    distance = std::min(distance, std::sqrt(dx * dx + dy * dy + dz * dz));
                }
        const double unit = std::clamp(distance / 0.45, 0.0, 1.0);
        const double mask = unit * unit * (3.0 - 2.0 * unit);
        const double broad = PA::evaluateGeologyFbmReference(
            {0.83 * p.x + 1.7, 0.69 * p.y - 0.8, 0.91 * p.z + 0.4},
            seed, 3, 1.87, 0.5);
        const double fine = PA::evaluateGeologyFbmReference(
            {2.7 * p.x - 0.6, 2.3 * p.y + 1.1, 2.5 * p.z - 0.7},
            seed ^ 0x3f47a5d1U, 2, 1.87, 0.5);
        const double ridge = 2.0 * std::pow(1.0 - std::abs(fine), 2.0) - 1.0;
        const double signedField = std::clamp(0.62 * broad + 0.25 * fine +
                                                 0.13 * ridge, -1.0, 1.0);
        const double strength = amplitude * mask * signedField / length;
        displacements[i] = {sum.x * strength, sum.y * strength, sum.z * strength};
    }
    auto preservesFaces = [&](double scale)
    {
        for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
        {
            auto a = mesh.positions[mesh.indices[t]],
                 b = mesh.positions[mesh.indices[t + 1]],
                 c = mesh.positions[mesh.indices[t + 2]];
            const auto oldU = Vec3d{b.x - a.x, b.y - a.y, b.z - a.z},
                       oldV = Vec3d{c.x - a.x, c.y - a.y, c.z - a.z};
            const auto oldN = Vec3d{oldU.y * oldV.z - oldU.z * oldV.y,
                                    oldU.z * oldV.x - oldU.x * oldV.z,
                                    oldU.x * oldV.y - oldU.y * oldV.x};
            const auto da = displacements[mesh.indices[t]],
                       db = displacements[mesh.indices[t + 1]],
                       dc = displacements[mesh.indices[t + 2]];
            a = shifted(a, da, scale);
            b = shifted(b, db, scale);
            c = shifted(c, dc, scale);
            const auto u = Vec3d{b.x - a.x, b.y - a.y, b.z - a.z},
                       v = Vec3d{c.x - a.x, c.y - a.y, c.z - a.z};
            const auto n = Vec3d{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
                                 u.x * v.y - u.y * v.x};
            const double oldArea2 = oldN.x * oldN.x + oldN.y * oldN.y + oldN.z * oldN.z;
            if (oldN.x * n.x + oldN.y * n.y + oldN.z * n.z <= 0.05 * oldArea2)
                return false;
        }
        return true;
    };
    double scale = 1;
    if (!preservesFaces(scale))
    {
        double low = 0, high = 1;
        for (int step = 0; step < 12; ++step)
        {
            const double mid = 0.5 * (low + high);
            if (preservesFaces(mid)) low = mid;
            else high = mid;
        }
        scale = low;
    }
    if (scale < 0.25)
        throw std::runtime_error("Selected geology cannot preserve face orientation");
    for (std::size_t i = 0; i < mesh.positions.size(); ++i)
        source.mesh.positions[i] = shifted(mesh.positions[i], displacements[i], scale);
    normals(source);
    return source;
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
void cliffDrainage(MeshResult &surface, const Layer &layer, bool showField,
                   const Jerico::CancellationToken *cancel)
{
    check(cancel);
    const auto count = surface.mesh.positions.size();
    const auto side = std::size_t(std::llround(std::sqrt(double(count))));
    if (surface.closedSolid || side < 5 || side > 65 || side * side != count ||
        surface.field.size() != count ||
        surface.mesh.indices.size() != 6 * (side - 1) * (side - 1) ||
        surface.mesh.triangleRegions.size() != 2 * (side - 1) * (side - 1))
        throw std::runtime_error("Cliff drainage requires an open plane grid");
    auto [minY, maxY] = std::minmax_element(surface.mesh.positions.begin(), surface.mesh.positions.end(),
                                           [](const auto &a, const auto &b) { return a.y < b.y; });
    auto [minX, maxX] = std::minmax_element(surface.mesh.positions.begin(), surface.mesh.positions.end(),
                                           [](const auto &a, const auto &b) { return a.x < b.x; });
    const double height = maxY->y - minY->y;
    if (height <= 0 || layer.translation[0] - layer.lateral - 2 * layer.width <= minX->x ||
        layer.translation[0] + layer.lateral + 2 * layer.width >= maxX->x)
        throw std::runtime_error("Cliff drainage must stay inside the source surface");
    if (layer.profile == 1 &&
        (!std::isfinite(layer.projectorRadius[1]) || layer.projectorRadius[1] <= 0))
        throw std::runtime_error("Finite cliff incision requires a positive extent");
    if (layer.profile == 1 &&
        (layer.translation[1] - layer.projectorRadius[1] <= minY->y ||
         layer.translation[1] + layer.projectorRadius[1] >= maxY->y))
        throw std::runtime_error("Finite cliff incision must end inside the source surface");
    auto smooth = [](double x)
    {
        x = std::clamp(x, 0.0, 1.0);
        return x * x * (3 - 2 * x);
    };
    for (std::size_t i = 0; i < count; ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        auto &p = surface.mesh.positions[i];
        const double center = layer.translation[0] + layer.lateral *
            std::sin(2 * std::numbers::pi * layer.frequency * (p.y - minY->y) / height + layer.phase);
        const double dx = (p.x - center) / layer.width;
        const double corridor = std::exp(-0.5 * dx * dx);
        const double weak = 0.5 + 0.5 * std::sin(2 * std::numbers::pi *
                                                 layer.thicknessFrequency * p.y + layer.phase);
        const double extent = layer.profile == 1 ?
            1 - smooth((std::abs(p.y - layer.translation[1]) / layer.projectorRadius[1] - 0.7) / 0.3) : 1;
        const double retreat = layer.amplitude * (0.6 + 0.4 * weak) * corridor * extent;
        p.z -= retreat;
        if (showField)
            surface.field[i] = float(retreat / layer.amplitude);
    }
    normals(surface);
}
void cliffCutLoop(MeshResult &surface, std::vector<Vec3d> &warp, const Layer &layer, int operation, bool showField,
                  bool softEdgeProfile, const Jerico::CancellationToken *cancel)
{
    check(cancel);
    if (surface.closedSolid || surface.mesh.positions.empty() ||
        surface.field.size() != surface.mesh.positions.size() ||
        warp.size() != surface.mesh.positions.size())
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
        double detail = 0;
        if (softEdgeProfile && layer.profile >= 2)
        {
            const double x = std::cos(angle), y = std::sin(angle);
            const double broad = PA::evaluateGeologyFbmReference(
                {2.1 * x + 3.7, 2.1 * y - 1.8, 0.25}, layer.seed ^ 0x719be431U, 3,
                layer.lacunarity, 0.5);
            const double fine = PA::evaluateGeologyFbmReference(
                {4.3 * x - 2.4, 4.3 * y + 1.1, 0.75}, layer.seed ^ 0x2c36d857U, 2,
                layer.lacunarity, 0.5);
            detail = 0.55 * broad + 0.20 * fine;
        }
        return std::clamp(1.0 + layer.distortion *
                         (0.55 * std::sin(5 * angle + seed) +
                          0.30 * std::sin(9 * angle - seed) + detail), 0.7, 1.3);
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
                if (x == 0 || y == 0 || x + 1 == n || y + 1 == n)
                    throw std::runtime_error("Cliff cut loop must stay inside the source surface");
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
    std::vector<std::pair<std::uint32_t, std::uint32_t>> boundary;
    std::vector<unsigned> boundaryDegree(surface.mesh.positions.size());
    std::vector<std::vector<std::uint32_t>> boundaryNeighbors(surface.mesh.positions.size());
    auto appendBoundary = [&](std::uint32_t a, std::uint32_t b)
    {
        boundary.emplace_back(a, b);
        ++boundaryDegree[a];
        ++boundaryDegree[b];
        boundaryNeighbors[a].push_back(b);
        boundaryNeighbors[b].push_back(a);
    };
    for (std::size_t y = 1; y + 1 < n; ++y)
        for (std::size_t x = 1; x + 1 < n; ++x)
        {
            if (!selected[y * n + x])
                continue;
            const auto a = std::uint32_t(y * side + x), b = a + 1,
                       c = a + std::uint32_t(side), d = c + 1;
            if (!selected[(y - 1) * n + x]) appendBoundary(a, b);
            if (!selected[y * n + x + 1]) appendBoundary(b, d);
            if (!selected[(y + 1) * n + x]) appendBoundary(d, c);
            if (!selected[y * n + x - 1]) appendBoundary(c, a);
        }
    if (boundary.size() < 8 || std::any_of(boundaryDegree.begin(), boundaryDegree.end(),
                                            [](unsigned degree) { return degree != 0 && degree != 2; }))
        throw std::runtime_error("Cliff cut loop must form a single nonbranching joint");
    std::vector<unsigned char> visited(boundaryDegree.size());
    std::vector<std::uint32_t> pending{boundary.front().first};
    visited[pending.back()] = 1;
    std::size_t connectedVertices = 0;
    while (!pending.empty())
    {
        const auto vertex = pending.back();
        pending.pop_back();
        ++connectedVertices;
        for (auto neighbor : boundaryNeighbors[vertex])
            if (!visited[neighbor])
            {
                visited[neighbor] = 1;
                pending.push_back(neighbor);
            }
    }
    if (connectedVertices != boundary.size())
        throw std::runtime_error("Cliff cut loop contains disconnected joints");
    if (softEdgeProfile && layer.profile >= 2)
    {
        // RBrush-style push: deform the existing surface, never duplicate an overlay shell.
        const auto rimBasis = layer.profile == 3 || layer.profile == 4 ?
                              surface.mesh.positions : std::vector<Vec3d>{};
        const auto id = Solid::region(layer.seed, 0x434c494646000000ULL ^ std::uint64_t(operation));
        surface.regions.push_back({id, parent, SurfaceRole::CutLoop, operation, -1});
        for (std::size_t q = 0; q < selected.size(); ++q)
            if (selected[q])
                surface.mesh.triangleRegions[2 * q] = surface.mesh.triangleRegions[2 * q + 1] = id;
        for (std::size_t i = 0; i < surface.mesh.positions.size(); ++i)
        {
            if ((i & 255) == 0)
                check(cancel);
            auto &p = surface.mesh.positions[i];
            const auto [u, v] = coordinates(p);
            double interior = std::clamp((contour(u, v) - std::hypot(u, v)) /
                                         layer.width, 0.0, 1.0);
            if (layer.profile == 5)
            {
                double distance = std::numeric_limits<double>::infinity();
                for (const auto &[first, second] : boundary)
                {
                    const auto &a = surface.mesh.positions[first], &b = surface.mesh.positions[second];
                    const double dx = b.x - a.x, dy = b.y - a.y;
                    if (dx * dx + dy * dy <= 1e-16)
                        throw std::runtime_error("Metric cliff boundary has a degenerate edge");
                    const double t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) /
                                                (dx * dx + dy * dy), 0.0, 1.0);
                    distance = std::min(distance, std::hypot(p.x - (a.x + t * dx),
                                                              p.y - (a.y + t * dy)));
                }
                interior = inside[i] ? std::clamp(distance / layer.width, 0.0, 1.0) : 0.0;
            }
            const double falloff = interior * interior * (3 - 2 * interior);
            const double strata = PA::evaluateGeologyFbmReference(
                {0.8 * p.x, 0.35 * p.y, 1.1}, layer.seed ^ 0x4ae27b15U, 3,
                layer.lacunarity, 0.5);
            const double coverage = layer.alphaSource == 1 ?
                std::clamp((sampleCliffAlpha(u, v) - 0.42) / 0.35, 0.0, 1.0) : 1.0;
            const double alpha = coverage * coverage * (3 - 2 * coverage);
            double geology = std::clamp(0.85 + 0.15 * strata, 0.7, 1.0);
            if (layer.profile == 5)
            {
                const double broad = PA::evaluateGeologyFbmReference(
                    {0.73 * p.x + 1.9, 0.41 * p.y - 0.7, 0.37},
                    layer.seed ^ 0x3af914bdU, 3, layer.lacunarity, 0.5);
                const double fine = PA::evaluateGeologyFbmReference(
                    {2.1 * p.x - 0.8, 1.6 * p.y + 1.3, -0.4},
                    layer.seed ^ 0x294d5f17U, 2, layer.lacunarity, 0.5);
                const double ridge = 2.0 * std::pow(1.0 - std::abs(fine), 2.0) - 1.0;
                geology = std::clamp(0.55 + 1.4 *
                    (0.55 * broad + 0.25 * fine + 0.2 * ridge), 0.12, 1.5);
            }
            p.z -= layer.amplitude * falloff * alpha * geology;
            if (showField)
                surface.field[i] = float(falloff);
        }
        if (layer.profile == 3 || layer.profile == 4)
        {
            // Shape only the outer grid band. Non-harmonic, rotated domains
            // avoid a periodic scallop at the rim.
            const auto beforeRim = surface.mesh.positions;
            const bool signedRim = layer.profile == 4;
            auto edgeWeight = [](double unit)
            {
                const double distance = std::min(unit, 1.0 - unit);
                const double t = std::clamp(1.0 - distance / 0.18, 0.0, 1.0);
                return t * t * (3.0 - 2.0 * t);
            };
            const auto bandSteps = std::max<std::size_t>(1, std::size_t(0.18 * double(n)));
            for (std::size_t i = 0; i < surface.mesh.positions.size(); ++i)
            {
                if ((i & 255) == 0)
                    check(cancel);
                auto &p = surface.mesh.positions[i];
                const auto original = beforeRim[i];
                const double u = double(i % side) / double(n);
                const double v = double(i / side) / double(n);
                const double wx = edgeWeight(u), wy = edgeWeight(v);
                auto retreatToward = [&](std::size_t neighbor, std::size_t bandNeighbor,
                                         double desired, double weight)
                {
                    const auto basis = rimBasis[i], target = rimBasis[neighbor],
                               far = rimBasis[bandNeighbor];
                    const double dx = target.x - basis.x, dy = target.y - basis.y,
                                 dz = target.z - basis.z;
                    const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
                    if (length < 1e-8)
                        throw std::runtime_error("Cliff rim has a degenerate local grid direction");
                    const double sx = far.x - basis.x, sy = far.y - basis.y,
                                 sz = far.z - basis.z;
                    const double bandWidth = std::max(0.0, (sx * dx + sy * dy + sz * dz) / length);
                    const double distance = (signedRim ?
                        std::clamp(desired, -0.4 * bandWidth, 0.4 * bandWidth) :
                        std::min(desired, 0.4 * bandWidth)) * weight;
                    p.x += distance * dx / length;
                    p.y += distance * dy / length;
                    p.z += distance * dz / length;
                };
                if (wx > 0)
                {
                    const auto sample = signedRim ?
                        beforeRim[(i / side) * side + (u < 0.5 ? 0 : n)] : original;
                    const double noise = PA::evaluateGeologyFbmReference(
                        {0.83 * sample.y + 0.17 * sample.z,
                         0.31 * sample.y - 0.23 * sample.z, 1.37},
                        layer.seed ^ 0x49e367abU, 3, 1.87, 0.5);
                    const double field = std::clamp(0.5 + 0.5 * noise, 0.0, 1.0);
                    retreatToward(u < 0.5 ? i + 1 : i - 1,
                                  u < 0.5 ? i + bandSteps : i - bandSteps,
                                  signedRim ? 0.55 * std::clamp(1.6 * noise, -1.0, 1.0) :
                                              0.55 * field * field, wx);
                }
                if (wy > 0)
                {
                    const auto sample = signedRim ?
                        beforeRim[(v < 0.5 ? 0 : n) * side + i % side] : original;
                    const double noise = PA::evaluateGeologyFbmReference(
                        {0.61 * sample.x - 0.19 * sample.z,
                         0.47 * sample.x + 0.11 * sample.z, -2.11},
                        layer.seed ^ 0x8c125df3U, 3, 1.87, 0.5);
                    const double field = std::clamp(0.5 + 0.5 * noise, 0.0, 1.0);
                    retreatToward(v < 0.5 ? i + side : i - side,
                                  v < 0.5 ? i + bandSteps * side : i - bandSteps * side,
                                  signedRim ? 0.45 * std::clamp(1.6 * noise, -1.0, 1.0) :
                                              0.45 * field * field, wy);
                }
            }
            if (signedRim)
            {
                auto validScale = [&](double scale)
                {
                    auto position = [&](std::uint32_t index)
                    {
                        const auto &a = beforeRim[index], &b = surface.mesh.positions[index];
                        return Vec3d{a.x + scale * (b.x - a.x),
                                     a.y + scale * (b.y - a.y), a.z + scale * (b.z - a.z)};
                    };
                    auto face = [](Vec3d a, Vec3d b, Vec3d c)
                    {
                        const Vec3d u{b.x - a.x, b.y - a.y, b.z - a.z};
                        const Vec3d v{c.x - a.x, c.y - a.y, c.z - a.z};
                        return Vec3d{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
                                     u.x * v.y - u.y * v.x};
                    };
                    for (std::size_t t = 0; t < surface.mesh.indices.size(); t += 3)
                    {
                        const auto a = surface.mesh.indices[t], b = surface.mesh.indices[t + 1],
                                   c = surface.mesh.indices[t + 2];
                        const auto old = face(beforeRim[a], beforeRim[b], beforeRim[c]);
                        const auto now = face(position(a), position(b), position(c));
                        const double oldArea2 = old.x * old.x + old.y * old.y + old.z * old.z;
                        const double orientation = old.x * now.x + old.y * now.y + old.z * now.z;
                        if (orientation <= 0.05 * oldArea2 ||
                            (std::abs(old.z) > 1e-8 && old.z * now.z <= 0.05 * old.z * old.z))
                            return false;
                    }
                    return true;
                };
                double scale = 1;
                if (!validScale(scale))
                {
                    double low = 0, high = 1;
                    for (int step = 0; step < 12; ++step)
                    {
                        const double mid = 0.5 * (low + high);
                        if (validScale(mid)) low = mid;
                        else high = mid;
                    }
                    scale = low;
                }
                if (scale < 0.25)
                    throw std::runtime_error("Signed cliff rim cannot preserve face orientation");
                for (std::size_t i = 0; i < surface.mesh.positions.size(); ++i)
                {
                    auto &p = surface.mesh.positions[i];
                    const auto &a = beforeRim[i];
                    p = {a.x + scale * (p.x - a.x), a.y + scale * (p.y - a.y),
                         a.z + scale * (p.z - a.z)};
                }
            }
        }
        normals(surface);
        return;
    }
    for (std::size_t i = 0; i < boundaryDegree.size(); ++i)
        if (boundaryDegree[i])
        {
            auto &p = surface.mesh.positions[i];
            auto [u, v] = coordinates(p);
            const auto radius = std::hypot(u, v);
            if (radius < 1e-6)
                throw std::runtime_error("Cliff joint contour cannot pass through its center");
            const auto scale = 1.0 + 0.5 * (contour(u, v) / radius - 1.0);
            u *= scale * layer.projectorRadius[0];
            v *= scale * layer.projectorRadius[1];
            p.x = layer.translation[0] + cs * u - sn * v;
            p.y = layer.translation[1] + sn * u + cs * v;
        }
    const auto id = Solid::region(layer.seed, 0x434c494646000000ULL ^ std::uint64_t(operation));
    const auto faceId = Solid::region(layer.seed, 0x434c494646100000ULL ^ std::uint64_t(operation));
    surface.regions.push_back({id, parent, SurfaceRole::CutLoop, operation, -1});
    surface.regions.push_back({faceId, id, SurfaceRole::CutFace, operation, -1});
    const auto originalCount = surface.mesh.positions.size();
    std::vector<std::uint32_t> selectedVertex(originalCount);
    for (std::size_t i = 0; i < originalCount; ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        selectedVertex[i] = std::uint32_t(i);
        if (!inside[i] || inside[i] == incident[i])
            continue;
        selectedVertex[i] = std::uint32_t(surface.mesh.positions.size());
        surface.mesh.positions.push_back(surface.mesh.positions[i]);
        const auto priorWarp = warp[i];
        warp.push_back(priorWarp);
        surface.field.push_back(surface.field[i]);
        auto duplicate = [&](auto &values)
        {
            if (!values.empty())
                values.push_back(values[i]);
        };
        duplicate(surface.layerIndices);
        duplicate(surface.layerThickness);
        duplicate(surface.layerEdgeDistance);
    }
    for (std::size_t i = 0; i < originalCount; ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        if (!inside[i])
        {
            if (showField) surface.field[i] = 0;
            continue;
        }
        auto &p = surface.mesh.positions[selectedVertex[i]];
        auto [u, v] = coordinates(p);
        if (softEdgeProfile && layer.profile >= 1)
        {
            const double inside = std::clamp((contour(u, v) - std::hypot(u, v)) / layer.width,
                                             0.0, 1.0);
            const double falloff = inside * inside * (3 - 2 * inside);
            const double retreat = std::clamp(layer.amplitude + 0.3 * p.z - layer.lateral * v,
                                              0.5 * layer.amplitude, 1.5 * layer.amplitude);
            // Keep a thin real joint; a zero-width joint would collapse its selectable faces.
            p.z -= retreat * (0.08 + 0.92 * falloff);
        }
        else
            p.z += -layer.amplitude - 0.3 * p.z + layer.lateral * v;
        if (showField)
            surface.field[selectedVertex[i]] = 1;
        if (showField && selectedVertex[i] != i)
            surface.field[i] = 0;
    }
    for (std::size_t q = 0; q < selected.size(); ++q)
        if (selected[q])
        {
            for (int k = 0; k < 6; ++k)
                surface.mesh.indices[6 * q + k] = selectedVertex[surface.mesh.indices[6 * q + k]];
            surface.mesh.triangleRegions[2 * q] = surface.mesh.triangleRegions[2 * q + 1] = id;
        }
    for (const auto &[a, b] : boundary)
    {
        surface.mesh.indices.insert(surface.mesh.indices.end(),
                                    {a, b, selectedVertex[b], a, selectedVertex[b], selectedVertex[a]});
        surface.mesh.triangleRegions.insert(surface.mesh.triangleRegions.end(), {faceId, faceId});
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
