#include "AutoUv.hpp"
#include "Model.hpp"
#include "SurfaceGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
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
