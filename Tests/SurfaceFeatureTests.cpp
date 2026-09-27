#include "Model.hpp"
#include "SurfaceFeatures.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>
using namespace Playground;
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, const char *message)
{
    bool rejected = false;
    try
    {
        action();
    }
    catch (const std::exception &)
    {
        rejected = true;
    }
    require(rejected, message);
}
TriangleMesh crease(double z)
{
    TriangleMesh m;
    m.positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, z}};
    m.indices = {0, 1, 2, 1, 0, 3};
    return m;
}
TriangleMesh splitRegionGrid()
{
    TriangleMesh mesh;
    Jerico::Guid left{}, right{};
    left.bytes[0] = std::byte{1};
    right.bytes[0] = std::byte{2};
    for (int y = 0; y < 3; ++y)
        for (int x = -2; x <= 2; ++x)
            mesh.positions.push_back({double(x), double(y), 0});
    for (int y = 0; y < 2; ++y)
        for (int x = -2; x < 2; ++x)
        {
            auto a = std::uint32_t(y * 5 + x + 2);
            auto b = a + 1;
            auto c = a + 5;
            auto d = c + 1;
            mesh.indices.insert(mesh.indices.end(), {a, b, d, a, d, c});
            mesh.triangleRegions.insert(mesh.triangleRegions.end(), 2, x < 0 ? left : right);
        }
    return mesh;
}
int main()
{
    try
    {
        SurfaceFeatureSettings settings;
        settings.contact = false;
        auto convex = surfaceFeatures(crease(-1), settings);
        auto concave = surfaceFeatures(crease(1), settings);
        require(concave.cavityAccumulation[0]>convex.cavityAccumulation[0] &&
                    convex.cavityAccumulation.size()==convex.convexity.size(),
                "shared cavity field must favor concavity without a separate consumer analysis");
        SurfaceFeatureCache cache;
        const auto firstCached=cache.get(crease(-1),settings);
        require(cache.misses()==1 && cache.hits()==0 &&
                    firstCached.signedDihedralCurvature[0]>0,
                "first feature request must compute from the source mesh");
        const auto secondCached=cache.get(crease(-1),settings);
        require(cache.misses()==1 && cache.hits()==1 &&
                    secondCached.signedDihedralCurvature[0]>0,
                "unchanged geometry and settings must reuse the computed fields");
        const auto changedCached=cache.get(crease(1),settings);
        require(cache.misses()==2 && changedCached.signedDihedralCurvature[0]<0,
                "position change must invalidate cached feature classification");
        auto changedSettings=settings;
        changedSettings.creaseAngleDegrees=120;
        cache.get(crease(1),changedSettings);
        require(cache.misses()==3,
                "feature settings change must invalidate cached fields");
        Recipe repeatedFeatures;
        repeatedFeatures.base=Base::Cube;
        auto wear=defaultLayer(Kind::SurfaceDamage);
        wear.amplitude=0;
        repeatedFeatures.layers={wear,wear};
        const auto reused=evaluate(repeatedFeatures,8);
        require(reused.featureCacheMisses==1 && reused.featureCacheHits>=1,
                "unchanged geology stages must share one surface feature computation");
        require(std::abs(convex.signedDihedralCurvature[0] - 3 * std::numbers::pi / 8) < 1e-10,
                "convex right-angle hinge has hand-derived positive curvature");
        require(std::abs(concave.signedDihedralCurvature[0] + 3 * std::numbers::pi / 8) < 1e-10,
                "concave right-angle hinge has negative curvature");
        require(convex.convexity[0] == convex.signedDihedralCurvature[0] && convex.concavity[0] == 0 &&
                    convex.absoluteCurvature[0] == convex.signedDihedralCurvature[0] &&
                    concave.convexity[0] == 0 &&
                    concave.concavity[0] == -concave.signedDihedralCurvature[0],
                "signed curvature exposes independent convex, concave and absolute views");
        require(convex.boundary[0] && convex.edgeDistance[0] == 0,
                "boundary validity and crease distance are explicit");
        auto reversed = crease(-1);
        for (std::size_t t = 0; t < reversed.indices.size(); t += 3)
            std::swap(reversed.indices[t + 1], reversed.indices[t + 2]);
        require(std::abs(surfaceFeatures(reversed, settings).signedDihedralCurvature[0] +
                         convex.signedDihedralCurvature[0]) < 1e-10,
                "reversing the authored front side reverses signed curvature");
        auto scaled = crease(-1);
        for (auto &p : scaled.positions)
        {
            p.x *= 3;
            p.y *= 3;
            p.z *= 3;
        }
        auto larger = surfaceFeatures(scaled, settings);
        require(std::abs(larger.signedDihedralCurvature[0] * 3 - convex.signedDihedralCurvature[0]) < 1e-10,
                "curvature has inverse-length units");
        auto regions = splitRegionGrid();
        std::vector<double> raw(regions.positions.size());
        for (int y = 0; y < 3; ++y)
            for (int x = -2; x <= 2; ++x)
                raw[y * 5 + x + 2] = x == 0 ? 1000 : x > 0 ? -7 : 0;
        raw[6] = 10;
        raw[1] = raw[5] = raw[11] = 1;
        auto originalRaw = raw;
        auto relaxed = relaxSignedCurvature(regions, raw, 0.5);
        require(relaxed[6] > 0 && relaxed[6] < 10 && relaxed[8] == -7 && relaxed[7] == 1000 &&
                    raw == originalRaw,
                "one-pass curvature relaxation reduces same-region spike without crossing semantic seam");
        auto unprotected = regions;
        unprotected.triangleRegions.clear();
        require(relaxSignedCurvature(unprotected, raw, 0.5)[6] > relaxed[6],
                "semantic region guard changes the stencil at a shared-vertex seam");
        auto opposite = raw;
        opposite[2] = opposite[7] = opposite[12] = -1000;
        require(relaxSignedCurvature(unprotected, opposite, 0.5)[6] > 0,
                "curvature relaxation must not turn convex source into a concave mask");
        require(relaxSignedCurvature(regions, raw, 0) == raw,
                "disabled curvature relaxation is an exact identity");
        rejects([&] { relaxSignedCurvature(regions, raw, -0.1); },
                "curvature relaxation rejects negative mixing weight");

        Recipe planeRecipe;
        planeRecipe.base = Base::Plane;
        auto plane = evaluate(planeRecipe, 8);
        settings.contact = true;
        settings.radius = 0.4;
        settings.ambient = true;
        settings.ambientRadius = 0.4;
        settings.ambientSamples = 16;
        settings.exposureDirection = {0, 0, 1};
        auto flat = surfaceFeatures(plane.mesh, settings);
        for (std::size_t i = 0; i < flat.signedDihedralCurvature.size(); ++i)
            require(std::abs(flat.signedDihedralCurvature[i]) < 1e-12 && flat.contactOcclusion[i] == 0 &&
                        flat.ambientOcclusion[i] == 0 && std::isinf(flat.edgeDistance[i]) &&
                        flat.exposure[i] == 1,
                    "isolated plane has no curvature, crease or contact");
        require(flat.boundaryDistance[0] == 0 && flat.boundaryDistance[40] > 0 &&
                    std::isfinite(flat.boundaryDistance[40]),
                "surface geodesic boundary distance distinguishes rim from plane interior");
        auto plates = plane.mesh;
        auto offset = std::uint32_t(plates.positions.size());
        for (auto p : plane.mesh.positions)
        {
            p.z += 0.08;
            plates.positions.push_back(p);
        }
        for (auto i : plane.mesh.indices)
            plates.indices.push_back(i + offset);
        auto contact = surfaceFeatures(plates, settings);
        require(contact.contactOcclusion[40] > 0.5 && contact.contactOcclusion[40 + offset] == 0 &&
                    contact.ambientOcclusion[40] > 0.5 && contact.ambientOcclusion[40 + offset] == 0,
                "near parallel plates have directional contact without curvature");
        require(std::abs(contact.signedDihedralCurvature[40]) < 1e-12,
                "contact does not masquerade as curvature");
        settings.radius = 0.04;
        auto separateRadii = surfaceFeatures(plates, settings);
        require(separateRadii.contactOcclusion[40] == 0 && separateRadii.ambientOcclusion[40] > 0.5,
                "contact and ambient occlusion retain independent physical radii");
        Recipe cubeRecipe;
        auto cube = evaluate(cubeRecipe, 8);
        settings.contact = false;
        settings.ambient = false;
        settings.thickness = true;
        settings.thicknessRadius = 4;
        settings.exposureDirection = {0, 1, 0};
        auto volume = surfaceFeatures(cube.mesh, settings);
        bool foundFaceCenter = false;
        for (std::size_t i = 0; i < cube.mesh.positions.size(); ++i)
        {
            auto p = cube.mesh.positions[i];
            if (std::abs(p.x) < 1e-12 && std::abs(p.y) < 1e-12 && std::abs(p.z - 1) < 1e-12)
            {
                require(volume.thicknessConfidence[i] == 1 &&
                            std::abs(volume.thickness[i] - 2) < 1e-6,
                        "closed cube face center reports two-unit inward thickness");
                foundFaceCenter = true;
            }
        }
        require(foundFaceCenter, "thickness fixture contains a face center");
        auto invalid = crease(-1);
        invalid.indices.insert(invalid.indices.end(), {0, 1, 3});
        rejects([&] { surfaceFeatures(invalid, settings); }, "non-manifold input rejected");
        invalid = crease(-1);
        invalid.positions[2] = invalid.positions[1];
        rejects([&] { surfaceFeatures(invalid, settings); }, "degenerate input rejected");
        invalid = crease(-1);
        invalid.indices[0] = 99;
        rejects([&] { surfaceFeatures(invalid, settings); }, "invalid index rejected");
        settings.radius = std::numeric_limits<double>::quiet_NaN();
        rejects([&] { surfaceFeatures(plates, settings); }, "non-finite radius rejected");
        settings = {};
        Jerico::CancellationSource cancellation;
        cancellation.requestCancellation();
        auto token = cancellation.token();
        rejects([&] { surfaceFeatures(plates, settings, &token); }, "feature evaluation cancels");

        Recipe rock;
        auto before = evaluate(rock, 16);
        auto damage = defaultLayer(Kind::SurfaceDamage);
        damage.mask = Mask::EdgeProximity;
        damage.featureRadius = 0.3f;
        damage.amplitude = 0.08f;
        rock.layers = {damage};
        auto after = evaluate(rock, 16, -1, 0);
        auto features = surfaceFeatures(before.mesh, SurfaceFeatureSettings{.contact = false});
        int protectedCount = 0, movedCount = 0;
        for (std::size_t i = 0; i < before.mesh.positions.size(); ++i)
        {
            auto p = before.mesh.positions[i], q = after.mesh.positions[i];
            double distance = std::hypot(p.x - q.x, p.y - q.y, p.z - q.z);
            if (features.edgeDistance[i] >= damage.featureRadius)
            {
                ++protectedCount;
                require(distance == 0, "wear preserves vertices beyond crease band");
            }
            if (distance > 1e-10)
                ++movedCount;
            require(distance <= damage.amplitude + 1e-8 &&
                        distance <= 0.35 * features.minimumEdgeLength[i] + 1e-8,
                    "wear is bounded by authored depth and local sampling");
            require(distance <= 0.2 * features.minimumTriangleAltitude[i] + 1e-8,
                    "thin triangles bound wear by altitude, not just edge length");
            auto n = before.normals[i];
            require((q.x - p.x) * n.x + (q.y - p.y) * n.y + (q.z - p.z) * n.z <= 1e-12,
                    "damage removes surface along inward normal");
        }
        require(protectedCount > 100 && movedCount > 30, "fixture exercises protected and worn areas");
        require(before.mesh.indices == after.mesh.indices &&
                    before.mesh.triangleRegions == after.mesh.triangleRegions,
                "damage preserves topology and semantic regions");
        require(PA::validateClosedMesh(after.mesh).hasValue(), "bounded damage remains closed");
        auto stored = serialize(rock);
        require(PA::hashTriangleMesh(after.mesh) ==
                    PA::hashTriangleMesh(evaluate(deserialize(stored), 16).mesh),
                "new recipe roundtrip preserves geometry and regions");
        require(meshHash(after.mesh) == meshHash(evaluate(rock, 16).mesh), "seed repeat is deterministic");
        rock.layers[0].seed += 97;
        require(meshHash(after.mesh) != meshHash(evaluate(rock, 16).mesh), "seed changes spatial wear");
        rock.layers[0].amplitude = 0;
        require(meshHash(before.mesh) == meshHash(evaluate(rock, 16).mesh), "zero damage is identity");
        rock.layers[0].amplitude = -0.1f;
        rejects([&] { evaluate(rock, 16); }, "negative removal depth rejected");
        rock.layers[0] = damage;
        rock.layers[0].enabled = false;
        require(meshHash(before.mesh) == meshHash(evaluate(rock, 16).mesh), "disabled damage is identity");
        rock.layers[0] = damage;
        rock.layers[0].featureRadius = 0;
        rejects([&] { evaluate(rock, 16); }, "zero feature radius rejected");
        rock.layers[0] = damage;
        rock.layers[0].mask = Mask::Curvature;
        auto curvatureMasked = evaluate(rock, 16, -1, 0);
        require(meshHash(curvatureMasked.mesh) != meshHash(before.mesh),
                "curvature drives real displacement");
        rock.layers[0].mask = Mask::Convexity;
        require(meshHash(evaluate(rock, 16).mesh) != meshHash(before.mesh),
                "derived convexity is a reusable operation mask");
        rock.layers[0].mask = Mask::Concavity;
        require(meshHash(evaluate(rock, 16).mesh) == meshHash(before.mesh),
                "convex fixture has no concavity wear");
        rock.layers[0].mask = Mask::Cavity;
        rock.layers[0].maskLow=0;
        rock.layers[0].maskHigh=1;
        require(meshHash(evaluate(rock,16).mesh)==meshHash(before.mesh),
                "shared cavity mask leaves an isolated convex rock unchanged");
        auto extendedStored = serialize(rock);
        rock.layers[0].mask = Mask::Contact;
        require(meshHash(evaluate(rock, 16).mesh) == meshHash(before.mesh),
                "isolated convex cube has no contact wear");
        Recipe craterRecipe;
        auto crater = defaultLayer(Kind::DamageDecal);
        crater.translation = {0, 0, 1};
        crater.direction = {0, 0, 1};
        crater.projectorRadius = {0.7f, 0.5f};
        crater.amplitude = 0.12f;
        crater.damageLip = 0.025f;
        craterRecipe.layers = {crater};
        auto craterBefore = evaluate(Recipe{}, 24), craterAfter = evaluate(craterRecipe, 24);
        bool dentedCenter = false, preservedBack = false;
        for (std::size_t i = 0; i < craterBefore.mesh.positions.size(); ++i)
        {
            auto p = craterBefore.mesh.positions[i], q = craterAfter.mesh.positions[i];
            if (std::abs(p.x) < 1e-12 && std::abs(p.y) < 1e-12 && std::abs(p.z - 1) < 1e-12)
                dentedCenter = q.z < p.z - 0.05;
            if (std::abs(p.x) < 1e-12 && std::abs(p.y) < 1e-12 && std::abs(p.z + 1) < 1e-12)
                preservedBack = q.z == p.z;
        }
        require(dentedCenter && preservedBack, "oriented damage projector dents only its front footprint");
        require(craterBefore.mesh.indices == craterAfter.mesh.indices &&
                    craterBefore.mesh.triangleRegions == craterAfter.mesh.triangleRegions &&
                    PA::validateClosedMesh(craterAfter.mesh).hasValue(),
                "geometric damage tier preserves topology, regions and closed mesh");
        require(meshHash(craterAfter.mesh) == meshHash(evaluate(deserialize(serialize(craterRecipe)), 24).mesh),
                "damage projector recipe repeats after serialization");
        auto craterPreset = preset(12), craterPresetBaseline = craterPreset;
        for (auto &layer : craterPresetBaseline.layers)
            if (layer.kind == Kind::DamageDecal)
                layer.enabled = false;
        require(meshHash(evaluate(craterPreset, craterPreset.finalResolution).mesh) !=
                    meshHash(evaluate(craterPresetBaseline, craterPreset.finalResolution).mesh),
                "crater-rock preset visibly exercises the native projector");
        auto oldSchema = extendedStored;
        auto at = oldSchema.find("\"version\": 10");
        require(at != std::string::npos, "extended native fields serialize as schema v10");
        oldSchema.replace(at, 13, "\"version\": 4");
        rejects([&] { deserialize(oldSchema); }, "old schema rejects new capability IDs");
        auto previousRecipe = serialize(preset(10));
        auto bspCliff = preset(9);
        require(bspCliff.layers[1].direction == std::array<float, 3>{0, 1, 0} &&
                    bspCliff.layers[1].maskAxis == std::array<float, 3>{0, 0, -1},
                "BSP cliff keeps vertical strata while masking toward its front face");
        auto oldAxis = bspCliff;
        oldAxis.layers[1].maskAxis = {};
        require(meshHash(evaluate(bspCliff, 20).mesh) != meshHash(evaluate(oldAxis, 20).mesh),
                "independent mask axis changes the BSP cliff geometry");
        require(meshHash(evaluate(bspCliff, 20).mesh) ==
                    meshHash(evaluate(deserialize(serialize(bspCliff)), 20).mesh),
                "BSP cliff with independent axis survives recipe roundtrip");
        require(PA::validateClosedMesh(evaluate(bspCliff, bspCliff.finalResolution).mesh).hasValue(),
                "BSP cliff retains closed topology with face-aligned relief");
        auto v7WithNewAxis = serialize(bspCliff);
        auto axisVersion = v7WithNewAxis.find("\"version\": 10");
        require(axisVersion != std::string::npos, "independent axis survives schema v10");
        v7WithNewAxis.replace(axisVersion, 13, "\"version\": 7");
        rejects([&] { deserialize(v7WithNewAxis); },
                "v7 recipe must not silently accept an independent mask axis");
        auto v7WithoutNewAxis = nlohmann::json::parse(serialize(preset(10)));
        v7WithoutNewAxis["version"] = 7;
        for (auto &layer : v7WithoutNewAxis["layers"])
            layer.erase("maskAxis");
        require(deserialize(v7WithoutNewAxis.dump()).version == 10,
                "v7 recipes without independent mask axes migrate to v10");
        auto previousVersion = previousRecipe.find("\"version\": 10");
        require(previousVersion != std::string::npos, "current recipe serializes as schema v10");
        previousRecipe.replace(previousVersion, 13, "\"version\": 5");
        require(deserialize(previousRecipe).version == 10,
                "existing v5 recipes migrate to the current in-memory schema");
        auto newRecipeOldVersion = serialize(preset(11));
        auto newVersion = newRecipeOldVersion.find("\"version\": 10");
        require(newVersion != std::string::npos, "perimeter recipe serializes as schema v10");
        auto v8WithOuterShape = nlohmann::json::parse(newRecipeOldVersion);
        v8WithOuterShape["version"] = 8;
        rejects([&] { deserialize(v8WithOuterShape.dump()); },
                "v8 recipe must not silently accept shaped outer caps");
        for (auto &layer : v8WithOuterShape["layers"])
            layer.erase("shapeOuterCaps");
        auto migratedV8 = deserialize(v8WithOuterShape.dump());
        require(migratedV8.version == 10 && !migratedV8.layers[0].shapeOuterCaps,
                "v8 recipe without outer shaping migrates with anchored caps");
        auto anchoredCurrent = preset(11);
        anchoredCurrent.layers[0].shapeOuterCaps = false;
        require(meshHash(evaluate(migratedV8, 12).mesh) ==
                    meshHash(evaluate(anchoredCurrent, 12).mesh),
                "v8 migration preserves its anchored outer geometry");
        auto v6WithNewPerimeter = newRecipeOldVersion;
        v6WithNewPerimeter.replace(newVersion, 13, "\"version\": 6");
        rejects([&] { deserialize(v6WithNewPerimeter); },
                "v6 recipe must not silently accept independent perimeter shaping");
        newRecipeOldVersion.replace(newVersion, 13, "\"version\": 5");
        rejects([&] { deserialize(newRecipeOldVersion); },
                "v5 recipe must not silently accept the v6 settling operation");
        for (int family : {10, 11})
            for (unsigned seed : {17U, 123U, 911U})
            {
                auto recipe = preset(family);
                std::cout << "checking family=" << family << " seed=" << seed << '\n';
                for (auto &layer : recipe.layers)
                    layer.seed = seed;
                auto baseline = recipe;
                for (auto &layer : baseline.layers)
                {
                    if (layer.kind == Kind::SurfaceDamage)
                        layer.enabled = false;
                    if (family == 11 && layer.kind == Kind::Layers)
                    {
                        layer.perimeterAmplitude = 0;
                        layer.shapeOuterCaps = false;
                    }
                }
                auto a = evaluate(baseline, recipe.finalResolution),
                     b = evaluate(recipe, recipe.finalResolution);
                require(meshHash(a.mesh) != meshHash(b.mesh),
                        "each new rock preset has observable surface shaping");
                require(PA::validateClosedMesh(b.mesh).hasValue(),
                        "each damaged seed retains closed topology");
                require(a.mesh.indices == b.mesh.indices && a.mesh.triangleRegions == b.mesh.triangleRegions,
                        "seed comparison keeps topology and regions identical");
                require(meshHash(b.mesh) ==
                            meshHash(evaluate(deserialize(serialize(recipe)), recipe.finalResolution).mesh),
                        "new preset seed matrix reproduces serialized result");
                std::cout << "preset=" << family << " seed=" << seed
                          << " triangles=" << b.mesh.indices.size() / 3 << " cpuMs=" << b.milliseconds
                          << '\n';
            }
        std::size_t invertedCliffFaces = 0;
        for (int stage = 0; stage < 4; ++stage)
        {
            auto cliff = evaluate(preset(7), 64, stage);
            invertedCliffFaces = 0;
            for (std::size_t t = 0; t < cliff.mesh.indices.size(); t += 3)
            {
                auto a = cliff.mesh.positions[cliff.mesh.indices[t]];
                auto b = cliff.mesh.positions[cliff.mesh.indices[t + 1]];
                auto c = cliff.mesh.positions[cliff.mesh.indices[t + 2]];
                auto facing = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
                invertedCliffFaces += facing <= 0;
            }
            require(invertedCliffFaces == 0,
                    "open cliff relief must preserve front-facing projected triangles at every stage");
        }
        std::cout << "PASS surface features: signed/scale curvature, crease band, independent contact, "
                     "invalid inputs, "
                     "cancellation, bounded seeded wear, topology, regions, identity and schema\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
