#include "AutoUv.hpp"
#include "Model.hpp"
#include "SurfaceGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numbers>
#include <set>
#include <stdexcept>
using namespace Playground;
void require(bool v, const char *m)
{
    if (!v)
        throw std::runtime_error(m);
}
int main()
{
    try
    {
        Recipe recipe;
        recipe.base = Base::Plane;
        recipe.planeSize = {6, 4};
        auto plane = evaluate(recipe, 16);
        require(!plane.closedSolid, "plane is explicitly a surface");
        require(plane.mesh.positions.size() == 289 && plane.mesh.indices.size() == 16 * 16 * 6,
                "plane grid resolution");
        for (auto p : plane.mesh.positions)
            require(std::abs(p.z) < 1e-12, "plane lies in XY");
        Layer relief;
        relief.kind = Kind::StrataRelief;
        relief.amplitude = 0.3f;
        relief.frequency = 1.5f;
        recipe.layers = {relief};
        auto cliff = evaluate(recipe, 16);
        require(meshHash(cliff.mesh) != meshHash(plane.mesh), "relief deforms wall");
        Layer solid;
        solid.kind = Kind::Solidify;
        solid.thickness = 0.25f;
        recipe.layers.push_back(solid);
        auto volume = evaluate(recipe, 16);
        require(volume.closedSolid && PA::validateClosedMesh(volume.mesh).hasValue(),
                "deformed wall solidifies to closed solid");
        require(PA::hashTriangleMesh(volume.mesh) ==
                    PA::hashTriangleMesh(evaluate(deserialize(serialize(recipe)), 16).mesh),
                "surface recipe round trip");
        auto cliffPreset = preset(8);
        auto drainageRecipe = cliffPreset;
        drainageRecipe.version = 13;
        auto drainage = defaultLayer(Kind::CliffDrainage);
        drainage.amplitude = 0.16f;
        drainage.width = 0.30f;
        drainage.translation[0] = -1.5f;
        drainage.lateral = 0.22f;
        drainage.frequency = 1;
        drainage.thicknessFrequency = 2.2f;
        drainage.phase = 0.25f;
        drainageRecipe.layers.insert(drainageRecipe.layers.begin() + 4, drainage);
        auto beforeDrainage = evaluate(cliffPreset, 32, 3);
        auto openDrainage = evaluate(drainageRecipe, 32, 4);
        require(!openDrainage.closedSolid &&
                    openDrainage.mesh.indices == beforeDrainage.mesh.indices &&
                    openDrainage.mesh.triangleRegions == beforeDrainage.mesh.triangleRegions,
                "drainage preserves the original grid and source identities before the cut");
        std::set<int> crossedHeightIntervals;
        for (std::size_t i = 0; i < openDrainage.mesh.positions.size(); ++i)
        {
            const auto before = beforeDrainage.mesh.positions[i];
            const auto after = openDrainage.mesh.positions[i];
            require(std::abs(before.x - after.x) < 1e-12 && std::abs(before.y - after.y) < 1e-12,
                    "drainage displaces only wall depth");
            const auto center = -1.5 + 0.22 * std::sin(2 * std::numbers::pi *
                                                       ((before.y + 2) / 4) + 0.25);
            if (std::abs(before.x - center) < 0.10 && before.z - after.z > 0.08)
                crossedHeightIntervals.insert(int(std::floor((before.y + 2) * 2.2)));
            if (before.x > 1.5)
                require(std::abs(before.z - after.z) < 0.001,
                        "drainage remains local to its corridor");
        }
        require(crossedHeightIntervals.size() >= 3,
                "a continuous incision retreats across at least three vertical sample intervals");
        auto drainedCliff = evaluate(drainageRecipe, 32);
        require(drainedCliff.closedSolid && PA::validateClosedMesh(drainedCliff.mesh).hasValue(),
                "drained cliff remains closed after cut and solidification");
        bool keepsCut = false, keepsJoint = false;
        for (const auto &region : drainedCliff.regions)
        {
            keepsCut |= region.role == SurfaceRole::CutLoop;
            keepsJoint |= region.role == SurfaceRole::CutFace;
        }
        require(keepsCut && keepsJoint, "drainage keeps the selectable cut and its joint faces");
        require(meshHash(drainedCliff.mesh) ==
                    meshHash(evaluate(deserialize(serialize(drainageRecipe)), 32).mesh),
                "drainage is deterministic after v13 recipe roundtrip");
        auto oldDrainage = serialize(drainageRecipe);
        auto oldDrainageVersion = oldDrainage.find("\"version\": 13");
        require(oldDrainageVersion != std::string::npos, "experimental drainage uses schema v13");
        oldDrainage.replace(oldDrainageVersion, 13, "\"version\": 12");
        bool rejectsOldDrainage = false;
        try { deserialize(oldDrainage); }
        catch (const std::exception &) { rejectsOldDrainage = true; }
        require(rejectsOldDrainage, "v12 recipe rejects the new drainage operation");
        auto cliffCut = evaluate(cliffPreset, 32);
        std::set<Jerico::Guid> frontRegions;
        for (const auto &region : cliffCut.regions)
            if (region.role != SurfaceRole::SolidBack && region.role != SurfaceRole::SolidRim)
                frontRegions.insert(region.id);
        require(frontRegions.size() >= 2, "solid cliff needs a separately selectable front cut region");
        require(PA::validateClosedMesh(cliffCut.mesh).hasValue(), "cut cliff remains a closed solid");
        auto cut = evaluate(cliffPreset, 32, 4);
        require(!cut.closedSolid, "cut is applied before solidification");
        require(cut.mesh.positions.size() > 33 * 33 &&
                    cut.mesh.triangleRegions.size() > 2 * 32 * 32,
                "cut creates joint geometry instead of only displacing grid vertices");
        std::map<std::pair<double, double>, std::pair<double, double>> elevations;
        for (auto p : cut.mesh.positions)
        {
            auto [entry, inserted] = elevations.try_emplace({p.x, p.y}, p.z, p.z);
            if (!inserted)
            {
                entry->second.first = std::min(entry->second.first, p.z);
                entry->second.second = std::max(entry->second.second, p.z);
            }
        }
        std::size_t steppedVertices = 0;
        double shallowestJoint = 1e20, deepestJoint = 0;
        for (const auto &[xy, z] : elevations)
        {
            const auto depth = z.second - z.first;
            if (depth > 0.08)
            {
                ++steppedVertices;
                shallowestJoint = std::min(shallowestJoint, depth);
                deepestJoint = std::max(deepestJoint, depth);
            }
        }
        require(steppedVertices > 8 && deepestJoint - shallowestJoint > 0.02,
                "cut has a localized joint and a ledge of variable physical depth");
        auto edgeProfileRecipe = cliffPreset;
        edgeProfileRecipe.layers[4].profile = 1;
        edgeProfileRecipe.layers[4].width = 0.35f;
        require(meshHash(evaluate(deserialize(serialize(edgeProfileRecipe)), 32).mesh) ==
                    meshHash(evaluate(cliffPreset, 32).mesh),
                "legacy v10 cut profile keeps its previous geometry after roundtrip");
        edgeProfileRecipe.version = 13;
        auto edgeProfileCut = evaluate(edgeProfileRecipe, 32, 4);
        auto meanJointDepth = [](const MeshResult &surface)
        {
            std::map<std::pair<double, double>, std::pair<double, double>> heights;
            for (const auto p : surface.mesh.positions)
            {
                auto [entry, inserted] = heights.try_emplace({p.x, p.y}, p.z, p.z);
                if (!inserted)
                {
                    entry->second.first = std::min(entry->second.first, p.z);
                    entry->second.second = std::max(entry->second.second, p.z);
                }
            }
            double sum = 0;
            int count = 0;
            for (const auto &[xy, z] : heights)
                if (z.second - z.first > 0.04)
                {
                    sum += z.second - z.first;
                    ++count;
                }
            require(count > 8, "cut retains a real selectable side wall");
            return sum / count;
        };
        require(meanJointDepth(edgeProfileCut) < 0.75 * meanJointDepth(cut),
                "negative edge height profile softens the abrupt cut boundary");
        auto jointRange = [](const MeshResult &surface)
        {
            const auto originalCount = 33 * 33;
            auto joint = std::find_if(surface.regions.begin(), surface.regions.end(),
                                      [](const auto &region) { return region.role == SurfaceRole::CutFace; });
            require(joint != surface.regions.end(), "profile keeps selectable joint faces");
            double smallest = 1e20, largest = -1e20;
            int pairs = 0;
            for (std::size_t t = 0; t < surface.mesh.triangleRegions.size(); ++t)
            {
                if (surface.mesh.triangleRegions[t] != joint->id)
                    continue;
                for (int a = 0; a < 3; ++a)
                    for (int b = 0; b < 3; ++b)
                    {
                        const auto original = surface.mesh.indices[3 * t + a];
                        const auto displaced = surface.mesh.indices[3 * t + b];
                        if (original >= originalCount || displaced < originalCount)
                            continue;
                        const auto p = surface.mesh.positions[original];
                        const auto q = surface.mesh.positions[displaced];
                        if (std::abs(p.x - q.x) > 1e-10 || std::abs(p.y - q.y) > 1e-10)
                            continue;
                        const double retreat = p.z - q.z;
                        smallest = std::min(smallest, retreat);
                        largest = std::max(largest, retreat);
                        ++pairs;
                    }
            }
            require(pairs > 8, "profile has measurable joint vertex pairs");
            return std::pair{smallest, largest};
        };
        const auto [minimumRetreat, maximumRetreat] = jointRange(edgeProfileCut);
        require(minimumRetreat > 0.01 && maximumRetreat < 0.10,
                "soft cut joint stays shallow and retreats inward along its entire contour");
        auto pushRecipe = edgeProfileRecipe;
        pushRecipe.layers[4].profile = 2;
        const auto beforePush = evaluate(pushRecipe, 32, 3);
        const auto pushed = evaluate(pushRecipe, 32, 4);
        require(pushed.mesh.positions.size() == beforePush.mesh.positions.size() &&
                    pushed.mesh.indices.size() == beforePush.mesh.indices.size(),
                "RBrush push deforms the existing cliff without overlay vertices or triangles");
        require(std::any_of(pushed.regions.begin(), pushed.regions.end(),
                            [](const auto &region) { return region.role == SurfaceRole::CutLoop; }) &&
                    std::none_of(pushed.regions.begin(), pushed.regions.end(),
                                 [](const auto &region) { return region.role == SurfaceRole::CutFace; }),
                "RBrush push keeps a selectable region without a hard sidewall");
        int moved = 0, still = 0;
        for (std::size_t i = 0; i < pushed.mesh.positions.size(); ++i)
        {
            const double depth = beforePush.mesh.positions[i].z - pushed.mesh.positions[i].z;
            require(depth >= -1e-8 && depth <= pushRecipe.layers[4].amplitude + 1e-8,
                    "brush displacement is outward and bounded");
            moved += depth > 0.03;
            still += std::abs(depth) < 1e-8;
        }
        require(moved > 8 && still > 100, "brush has a local push and leaves distant cliff untouched");
        auto metricRecipe = pushRecipe;
        metricRecipe.version = 18;
        metricRecipe.layers[4].profile = 5;
        const auto metricPush = evaluate(metricRecipe, 32, 4);
        require(metricPush.mesh.indices == pushed.mesh.indices &&
                    metricPush.mesh.triangleRegions == pushed.mesh.triangleRegions,
                "metric geological push preserves the same selectable grid topology");
        double minimumCore = 1e20, maximumCore = -1e20;
        int coreSamples = 0;
        for (std::size_t i = 0; i < metricPush.mesh.positions.size(); ++i)
        {
            const auto a = beforePush.mesh.positions[i], b = metricPush.mesh.positions[i];
            const double depth = a.z - b.z;
            if (std::hypot((a.x - 0.45) / 1.45, (a.y - 0.15) / 1.0) < 0.4)
            {
                minimumCore = std::min(minimumCore, depth);
                maximumCore = std::max(maximumCore, depth);
                ++coreSamples;
            }
            if (std::abs(a.x) > 2.8 || std::abs(a.y) > 1.8)
                require(std::abs(depth) < 1e-10,
                        "metric geological push leaves the distant cliff fixed");
        }
        require(coreSamples > 20 && maximumCore - minimumCore > 0.08,
                "metric geological push gives the selected core non-flat relief");
        require(PA::validateClosedMesh(evaluate(metricRecipe, 32).mesh).hasValue(),
                "metric geological push remains a closed solid after solidify");
        require(meshHash(evaluate(metricRecipe, 32).mesh) ==
                    meshHash(evaluate(deserialize(serialize(metricRecipe)), 32).mesh),
                "metric geological push replays after serialization");
        auto alphaRecipe = pushRecipe;
        alphaRecipe.layers[4].alphaSource = 1;
        const auto alphaPush = evaluate(alphaRecipe, 32, 4);
        require(alphaPush.mesh.indices == pushed.mesh.indices &&
                    alphaPush.mesh.triangleRegions == pushed.mesh.triangleRegions,
                "CC0 alpha changes relief without changing topology or selection");
        auto macroRecipe = alphaRecipe;
        auto &macro = macroRecipe.layers.front();
        require(macro.kind == Kind::VectorNoise, "cliff macroform starts with native vector FBM");
        macro.amplitude = 0.35f;
        macro.frequency = 0.6f;
        macro.octaves = 3;
        macro.coordinateScale = {0.7f, 0.55f, 1};
        macro.outputScale = {0.8f, 0.6f, 1};
        auto originalMacro = evaluate(alphaRecipe, 64, 0);
        auto shapedMacro = evaluate(macroRecipe, 64, 0);
        require(shapedMacro.mesh.indices == originalMacro.mesh.indices &&
                    shapedMacro.mesh.triangleRegions == originalMacro.mesh.triangleRegions,
                "macro warp retains the grid topology and region identity");
        double planarShift = 0;
        for (std::size_t i = 0; i < shapedMacro.mesh.positions.size(); ++i)
        {
            auto a = originalMacro.mesh.positions[i], b = shapedMacro.mesh.positions[i];
            planarShift = std::max(planarShift, std::hypot(a.x - b.x, a.y - b.y));
        }
        require(planarShift > 0.08, "macro warp visibly changes the cliff silhouette in object space");
        for (std::size_t t = 0; t < shapedMacro.mesh.indices.size(); t += 3)
        {
            auto normal = [&](const auto &mesh)
            {
                auto p = mesh.positions[mesh.indices[t]],
                     q = mesh.positions[mesh.indices[t + 1]],
                     r = mesh.positions[mesh.indices[t + 2]];
                return Vec3d{(q.y - p.y) * (r.z - p.z) - (q.z - p.z) * (r.y - p.y),
                             (q.z - p.z) * (r.x - p.x) - (q.x - p.x) * (r.z - p.z),
                             (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x)};
            };
            auto a = normal(originalMacro.mesh), b = normal(shapedMacro.mesh);
            require(a.x * b.x + a.y * b.y + a.z * b.z >
                        0.05 * (a.x * a.x + a.y * a.y + a.z * a.z),
                    "macro warp does not reverse a high-resolution front triangle");
        }
        auto macroSolid = evaluate(macroRecipe, 64);
        require(macroSolid.closedSolid && PA::validateClosedMesh(macroSolid.mesh).hasValue(),
                "macro-warped RBrush cliff remains a closed solid");
        auto erodedRecipe = macroRecipe;
        erodedRecipe.version = 14;
        erodedRecipe.layers[4].profile = 3;
        const auto erodedOpen = evaluate(erodedRecipe, 64, 4);
        const auto macroOpen = evaluate(macroRecipe, 64, 4);
        require(erodedOpen.mesh.indices == macroOpen.mesh.indices &&
                    erodedOpen.mesh.triangleRegions == macroOpen.mesh.triangleRegions,
                "eroded outline retains topology and selectable cut identity");
        double inwardBoundary = 0, centerShift = 0;
        constexpr std::size_t cliffSide = 65;
        for (std::size_t i = 0; i < erodedOpen.mesh.positions.size(); ++i)
        {
            const auto x = i % cliffSide, y = i / cliffSide;
            const auto a = macroOpen.mesh.positions[i], b = erodedOpen.mesh.positions[i];
            if (x == 0) inwardBoundary = std::max(inwardBoundary, b.x - a.x);
            if (x == 64) inwardBoundary = std::max(inwardBoundary, a.x - b.x);
            if (y == 0) inwardBoundary = std::max(inwardBoundary, b.y - a.y);
            if (y == 64) inwardBoundary = std::max(inwardBoundary, a.y - b.y);
            if (x >= 16 && x <= 48 && y >= 16 && y <= 48)
                centerShift = std::max(centerShift, std::hypot(a.x - b.x, a.y - b.y));
        }
        require(inwardBoundary > 0.1 && centerShift < 1e-10,
                "geological edge retreat breaks the panel outline without moving the selected center");
        const auto erodedSolid = evaluate(erodedRecipe, 64);
        require(erodedSolid.closedSolid && PA::validateClosedMesh(erodedSolid.mesh).hasValue(),
                "eroded cliff remains a closed solid");
        require(meshHash(erodedSolid.mesh) ==
                    meshHash(evaluate(deserialize(serialize(erodedRecipe)), 64).mesh),
                "eroded rim recipe replays deterministically after serialization");
        auto unsupportedRim = erodedRecipe;
        unsupportedRim.version = 13;
        bool rejectsOldRim = false;
        try { evaluate(unsupportedRim, 64); }
        catch (const std::exception &) { rejectsOldRim = true; }
        require(rejectsOldRim, "old recipe version rejects new cliff rim profile");
        auto signedRim = erodedRecipe;
        signedRim.version = 17;
        signedRim.layers[4].profile = 4;
        const auto signedOpen = evaluate(signedRim, 64, 4);
        require(signedOpen.mesh.indices == macroOpen.mesh.indices &&
                    signedOpen.mesh.triangleRegions == macroOpen.mesh.triangleRegions,
                "signed cliff rim retains topology and selectable cut identity");
        double inward = 0, outward = 0, signedCenterShift = 0;
        for (std::size_t i = 0; i < signedOpen.mesh.positions.size(); ++i)
        {
            const auto x = i % cliffSide, y = i / cliffSide;
            const auto a = macroOpen.mesh.positions[i], b = signedOpen.mesh.positions[i];
            if (x == 0) { inward = std::max(inward, b.x - a.x); outward = std::max(outward, a.x - b.x); }
            if (x == 64) { inward = std::max(inward, a.x - b.x); outward = std::max(outward, b.x - a.x); }
            if (y == 0) { inward = std::max(inward, b.y - a.y); outward = std::max(outward, a.y - b.y); }
            if (y == 64) { inward = std::max(inward, a.y - b.y); outward = std::max(outward, b.y - a.y); }
            if (x >= 16 && x <= 48 && y >= 16 && y <= 48)
                signedCenterShift = std::max(signedCenterShift, std::hypot(a.x - b.x, a.y - b.y));
        }
        require(inward > 0.08 && outward > 0.08 && signedCenterShift < 1e-10,
                "signed cliff rim makes both recesses and outcrops without moving its center");
        for (std::size_t t = 0; t < signedOpen.mesh.indices.size(); t += 3)
        {
            const auto a = signedOpen.mesh.positions[signedOpen.mesh.indices[t]],
                       b = signedOpen.mesh.positions[signedOpen.mesh.indices[t + 1]],
                       c = signedOpen.mesh.positions[signedOpen.mesh.indices[t + 2]];
            const double area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            if (area <= 1e-12)
            {
                const auto aa = macroOpen.mesh.positions[macroOpen.mesh.indices[t]],
                           bb = macroOpen.mesh.positions[macroOpen.mesh.indices[t + 1]],
                           cc = macroOpen.mesh.positions[macroOpen.mesh.indices[t + 2]];
                const double previousArea = (bb.x - aa.x) * (cc.y - aa.y) -
                                            (bb.y - aa.y) * (cc.x - aa.x);
                throw std::runtime_error("signed cliff rim folds triangle " +
                                         std::to_string(t / 3) + " area=" +
                                         std::to_string(area) + " previous=" +
                                         std::to_string(previousArea));
            }
        }
        const auto signedSolid = evaluate(signedRim, 64);
        require(signedSolid.closedSolid && PA::validateClosedMesh(signedSolid.mesh).hasValue(),
                "signed cliff rim remains a closed solid");
        require(meshHash(signedSolid.mesh) ==
                    meshHash(evaluate(deserialize(serialize(signedRim)), 64).mesh),
                "signed cliff rim recipe replays deterministically");
        auto oldSignedRim = signedRim;
        oldSignedRim.version = 16;
        bool rejectsOldSignedRim = false;
        try { evaluate(oldSignedRim, 64); }
        catch (const std::exception &) { rejectsOldSignedRim = true; }
        require(rejectsOldSignedRim, "old recipe version rejects signed cliff rim profile");
        Recipe rotatedRim;
        rotatedRim.base = Base::Plane;
        rotatedRim.version = 14;
        auto yaw = defaultLayer(Kind::Transform);
        yaw.rotation = {0, 0, 90};
        auto rotatedCut = defaultLayer(Kind::CliffCutLoop);
        rotatedCut.translation = {0, 0, 0};
        rotatedCut.projectorRadius = {0.8f, 0.6f};
        rotatedCut.profile = 2;
        rotatedRim.layers = {yaw, rotatedCut};
        const auto rotatedBefore = evaluate(rotatedRim, 64);
        rotatedRim.layers[1].profile = 3;
        const auto rotatedAfter = evaluate(rotatedRim, 64);
        const auto edgeIndex = std::size_t(32 * 65);
        const auto edgeBefore = rotatedBefore.mesh.positions[edgeIndex];
        const auto edgeAfter = rotatedAfter.mesh.positions[edgeIndex];
        require(std::abs(edgeAfter.x - edgeBefore.x) < 1e-6 &&
                    edgeAfter.y - edgeBefore.y > 0.02,
                "rotated cliff rim retreats along its local grid direction");
        auto smallRim = rotatedRim;
        smallRim.planeSize = {0.3f, 0.3f};
        smallRim.layers[0].rotation = {0, 0, 0};
        smallRim.layers[1].projectorRadius = {0.1f, 0.1f};
        smallRim.layers[1].distortion = 0;
        const auto smallAfter = evaluate(smallRim, 64);
        smallRim.layers[1].profile = 2;
        const auto smallBefore = evaluate(smallRim, 64);
        double smallBorderShift = 0;
        for (std::size_t i = 0; i < smallAfter.mesh.positions.size(); ++i)
        {
            const auto x = i % cliffSide, y = i / cliffSide;
            if (x != 0 && x != 64 && y != 0 && y != 64)
                continue;
            const auto a = smallBefore.mesh.positions[i], b = smallAfter.mesh.positions[i];
            smallBorderShift = std::max(smallBorderShift, std::hypot(a.x - b.x, a.y - b.y));
        }
        require(smallBorderShift > 0.001 && smallBorderShift < 0.04,
                "small cliff rim retreat scales to the local border width");
        for (std::size_t t = 0; t < smallAfter.mesh.indices.size(); t += 3)
        {
            const auto a = smallAfter.mesh.positions[smallAfter.mesh.indices[t]],
                       b = smallAfter.mesh.positions[smallAfter.mesh.indices[t + 1]],
                       c = smallAfter.mesh.positions[smallAfter.mesh.indices[t + 2]];
            const double area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            if (area <= 1e-12)
                throw std::runtime_error("small eroded cliff folds triangle " +
                                         std::to_string(t / 3) + " area=" +
                                         std::to_string(area) + " a=" +
                                         std::to_string(a.x) + "," + std::to_string(a.y) +
                                         " b=" + std::to_string(b.x) + "," + std::to_string(b.y) +
                                         " c=" + std::to_string(c.x) + "," + std::to_string(c.y));
        }
        double alphaDifference = 0;
        for (std::size_t i = 0; i < pushed.mesh.positions.size(); ++i)
            alphaDifference = std::max(alphaDifference,
                std::abs(alphaPush.mesh.positions[i].z - pushed.mesh.positions[i].z));
        require(alphaDifference > 0.005 && alphaDifference < alphaRecipe.layers[4].amplitude,
                "officially imported RBrush alpha measurably modulates the push");
        auto nonharmonicCliff = alphaRecipe;
        nonharmonicCliff.layers[4].lacunarity = 1.87f;
        auto nonharmonicPush = evaluate(nonharmonicCliff, 32, 4);
        require(meshHash(nonharmonicPush.mesh) != meshHash(alphaPush.mesh),
                "cliff push contour responds to authored octave lacunarity");
        require(meshHash(nonharmonicPush.mesh) ==
                    meshHash(evaluate(deserialize(serialize(nonharmonicCliff)), 32, 4).mesh),
                "nonharmonic cliff contour replays deterministically");
        require(serialize(deserialize(serialize(alphaRecipe))) == serialize(alphaRecipe),
                "RBrush alpha choice survives recipe roundtrip");
        auto pushSolid = evaluate(alphaRecipe, 32);
        require(PA::validateClosedMesh(pushSolid.mesh).hasValue(),
                "alpha-modulated push remains one closed solid");
        const auto pushedRegion = std::find_if(pushSolid.regions.begin(), pushSolid.regions.end(),
            [](const auto &region) { return region.role == SurfaceRole::CutLoop; })->id;
        const auto sourceRegion = std::find_if(pushSolid.regions.begin(), pushSolid.regions.end(),
            [](const auto &region) { return region.role == SurfaceRole::Source; })->id;
        std::map<std::uint32_t, Vec3d> pushNormals, sourceNormals;
        for (std::size_t i = 0; i < pushSolid.mesh.indices.size(); ++i)
        {
            const auto region = pushSolid.mesh.triangleRegions[i / 3];
            if (region == pushedRegion)
                pushNormals[pushSolid.mesh.indices[i]] = pushSolid.cornerNormals[i];
            if (region == sourceRegion)
                sourceNormals[pushSolid.mesh.indices[i]] = pushSolid.cornerNormals[i];
        }
        int smoothBoundary = 0;
        for (const auto &[vertex, pushedNormal] : pushNormals)
            if (sourceNormals.contains(vertex))
            {
                const auto other = sourceNormals.at(vertex);
                require(pushedNormal.x * other.x + pushedNormal.y * other.y +
                        pushedNormal.z * other.z > 0.999,
                        "brush and original cliff share continuous shading at the region boundary");
                ++smoothBoundary;
            }
        require(smoothBoundary > 8, "brush normal check covers a real region boundary");
        auto edgeProfileSolid = evaluate(edgeProfileRecipe, 32);
        require(edgeProfileSolid.closedSolid && PA::validateClosedMesh(edgeProfileSolid.mesh).hasValue(),
                "softened cut remains a closed solid");
        auto edgeDamageRecipe = edgeProfileRecipe;
        edgeDamageRecipe.version = 13;
        auto damage = defaultLayer(Kind::DamageDecal);
        damage.amplitude = 0.15f;
        damage.direction = {0, 0, 1};
        damage.translation = {0.45f, 0.15f, -0.15f};
        damage.projectorRadius = {1.8f, 1.4f};
        damage.featureRadius = 1;
        damage.threshold = 0.55f;
        damage.width = 0.45f;
        damage.damageLip = 0;
        damage.solidAlongNormals = false;
        edgeDamageRecipe.layers.insert(edgeDamageRecipe.layers.begin() + 4, damage);
        auto legacyDamageRecipe = edgeDamageRecipe;
        legacyDamageRecipe.version = 12;
        auto legacyNormalDamageRecipe = legacyDamageRecipe;
        legacyNormalDamageRecipe.layers[4].solidAlongNormals = true;
        require(meshHash(evaluate(deserialize(serialize(legacyDamageRecipe)), 32).mesh) ==
                    meshHash(evaluate(legacyNormalDamageRecipe, 32).mesh),
                "legacy v12 decal direction stays along surface normals after roundtrip");
        auto edgeDamaged = evaluate(edgeDamageRecipe, 32);
        require(edgeDamaged.closedSolid && PA::validateClosedMesh(edgeDamaged.mesh).hasValue() &&
                    meshHash(edgeDamaged.mesh) != meshHash(edgeProfileSolid.mesh),
                "negative smooth height damage changes the cliff without opening it");
        auto combinedCliff = edgeDamageRecipe;
        auto strongerDrainage = drainage;
        strongerDrainage.amplitude = 0.30f;
        strongerDrainage.width = 0.22f;
        combinedCliff.layers.insert(combinedCliff.layers.begin() + 5, strongerDrainage);
        auto combinedResult = evaluate(combinedCliff, 32);
        require(combinedResult.closedSolid && PA::validateClosedMesh(combinedResult.mesh).hasValue(),
                "edge profile, negative height field and drainage keep one closed cliff");
        auto combinedFinal = evaluate(combinedCliff, 64);
        require(combinedFinal.closedSolid && PA::validateClosedMesh(combinedFinal.mesh).hasValue(),
                "projected negative height field keeps the final-resolution cliff closed");
        Jerico::Guid cutId{}, jointId{};
        for (const auto &region : cut.regions)
            if (region.role == SurfaceRole::CutLoop)
            {
                cutId = region.id;
                require(region.operation == 4 && region.parent != Jerico::Guid{},
                        "cut retains operation and source lineage");
            }
        require(cutId != Jerico::Guid{}, "cut region has a selectable stable GUID");
        for (const auto &region : cut.regions)
            if (region.role == SurfaceRole::CutFace)
            {
                jointId = region.id;
                require(region.parent == cutId && region.operation == 4,
                        "joint faces retain their cut parent and operation");
            }
        require(jointId != Jerico::Guid{} && jointId != cutId &&
                    std::count(cut.mesh.triangleRegions.begin(),
                               cut.mesh.triangleRegions.end(), jointId) > 16,
                "real cut faces have their own selectable region");
        auto lower = evaluate(cliffPreset, 16, 4);
        bool sameRegion = false, sameJoint = false;
        for (const auto &region : lower.regions)
        {
            sameRegion |= region.role == SurfaceRole::CutLoop && region.id == cutId;
            sameJoint |= region.role == SurfaceRole::CutFace && region.id == jointId &&
                         region.parent == cutId;
        }
        require(sameRegion && sameJoint,
                "cut and joint region identity survive lower resolution regeneration");
        std::map<std::pair<std::uint32_t, std::uint32_t>, std::vector<Jerico::Guid>> edges;
        for (std::size_t t = 0; t < cut.mesh.triangleRegions.size(); ++t)
            for (int k = 0; k < 3; ++k)
            {
                auto a = cut.mesh.indices[3 * t + k], b = cut.mesh.indices[3 * t + (k + 1) % 3];
                edges[std::minmax(a, b)].push_back(cut.mesh.triangleRegions[t]);
            }
        std::map<std::uint32_t, unsigned> boundaryDegree;
        for (const auto &[edge, faces] : edges)
            if (faces.size() == 2 && faces[0] != faces[1])
            {
                ++boundaryDegree[edge.first];
                ++boundaryDegree[edge.second];
            }
        require(boundaryDegree.size() > 8, "cut exposes a nontrivial edge loop");
        for (const auto &[vertex, degree] : boundaryDegree)
            require(degree == 2, "cut boundary is a closed, nonbranching loop");
        auto uv = planAutomaticUv(cut.mesh, {.resolution = 512, .mipCount = 3});
        std::set<std::uint32_t> cutCharts, sourceCharts, jointCharts;
        for (std::size_t t = 0; t < cut.mesh.triangleRegions.size(); ++t)
            (cut.mesh.triangleRegions[t] == cutId ? cutCharts :
             cut.mesh.triangleRegions[t] == jointId ? jointCharts : sourceCharts)
                .insert(uv.triangleIsland[t]);
        require(!cutCharts.empty() && !sourceCharts.empty() && !jointCharts.empty(),
                "automatic UV covers cut, joint faces and source");
        for (auto chart : cutCharts)
            require(!sourceCharts.contains(chart) && !jointCharts.contains(chart),
                    "ZenUV-style automatic charts split the cut from its joint and source");
        for (auto chart : jointCharts)
            require(!sourceCharts.contains(chart), "joint faces have distinct automatic UV charts");
        require(meshHash(cliffCut.mesh) == meshHash(evaluate(deserialize(serialize(cliffPreset)), 32).mesh),
                "cut cliff is deterministic after recipe roundtrip");
        auto v9Recipe = serialize(preset(7));
        auto v9Marker = v9Recipe.find("\"version\": 10");
        require(v9Marker != std::string::npos, "current recipe uses schema v10");
        v9Recipe.replace(v9Marker, 13, "\"version\": 9");
        require(deserialize(v9Recipe).version == 10, "existing v9 recipes migrate to v10");
        auto invalidCutVersion = serialize(cliffPreset);
        auto cutMarker = invalidCutVersion.find("\"version\": 10");
        invalidCutVersion.replace(cutMarker, 13, "\"version\": 9");
        bool rejectsOldCut = false;
        try { deserialize(invalidCutVersion); }
        catch (const std::exception &) { rejectsOldCut = true; }
        require(rejectsOldCut, "v9 recipe rejects a v10 cut operation");
        auto outsideCut = cliffPreset;
        outsideCut.layers[4].translation = {20, 20, 0};
        bool rejectsEmptyCut = false;
        try { evaluate(outsideCut, 16); }
        catch (const std::exception &) { rejectsEmptyCut = true; }
        require(rejectsEmptyCut, "cut loop rejects an empty selected patch");
        recipe.layers = {defaultLayer(Kind::Planes)};
        bool rejected = false;
        try
        {
            evaluate(recipe, 16);
        }
        catch (const std::exception &)
        {
            rejected = true;
        }
        require(rejected, "open plane is rejected before solid operation");
        auto mustReject = [](auto action)
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
            require(rejected, "direct surface helper rejects invalid input");
        };
        mustReject([] { planeSource({6, 4}, 0); });
        mustReject([] { planeSource({-6, 4}, 16); });
        auto invalid = plane;
        invalid.field.clear();
        mustReject([&] { solidifySurface(invalid, solid); });
        invalid = plane;
        invalid.normals.clear();
        mustReject([&] { solidifySurface(invalid, solid); });
        invalid = plane;
        invalid.mesh.indices[0] = 999999;
        mustReject([&] { solidifySurface(invalid, solid); });
        std::cout << "PASS plane, relief, solidify, closedness, round trip and surface/solid guard\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
