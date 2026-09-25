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
