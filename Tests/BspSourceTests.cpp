#include "SolidGeometry.hpp"
#include "SurfaceGeometry.hpp"
#include "MeshReduction.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
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
        auto a = Solid::region(1, 1), b = Solid::region(1, 2);
        PA::BrushNode wall{a,
                           PA::BrushBoolean::Add,
                           PA::BoxBrush{{3, 2, 0.5}},
                           {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
        auto other = wall;
        other.id = b;
        other.localTransform[12] = 20;
        PA::BspDocument document{Solid::region(1, 3), 37, {wall, other}};
        auto originalHash = PA::hashDocument(document);
        auto input = bspSource(document, std::span<const Jerico::Guid>(&a, 1), 12);
        auto movedInput = bspSource(document, std::span<const Jerico::Guid>(&b, 1), 12);
        require(input.sourceSelection == std::vector<Jerico::Guid>{a} && input.sourceRevision == 37,
                "BSP selection identity and revision retained");
        for (auto p : input.mesh.positions)
            require(p.x <= 3.000001, "unselected brush absent from generated geometry");
        Recipe layered;
        auto strata = defaultLayer(Kind::Layers);
        strata.coherentLayers = true;
        strata.count = 3;
        strata.thickness = 0.32f;
        strata.spacing = 0;
        strata.lateral = 0;
        strata.layerScale = 0;
        strata.rotationDrift = 0;
        strata.scaleDrift = 0;
        strata.perimeterAmplitude = 0.12f;
        strata.shapeOuterCaps = true;
        layered.layers = {strata, defaultLayer(Kind::LayerSettle)};
        auto localRock = evaluateSource(layered, input, 12);
        auto movedRock = evaluateSource(layered, movedInput, 12);
        auto shiftedSource = movedInput.mesh, shiftedRock = movedRock.mesh;
        for (auto &p : shiftedSource.positions)
            p.x -= 20;
        for (auto &p : shiftedRock.positions)
            p.x -= 20;
        auto sourceError = sampledBidirectionalMeshError(input.mesh, shiftedSource);
        auto layeredError = sampledBidirectionalMeshError(localRock.mesh, shiftedRock);
        std::cout << "translated BSP sampled source error " << sourceError
                  << " layered error " << layeredError << '\n';
        require(sourceError < 1e-3 && layeredError < 1e-3,
                "layered BSP result must follow its selected source translation within 1 mm");
        Recipe recipe;
        recipe.layers = {defaultLayer(Kind::StrataRelief), defaultLayer(Kind::RidgedNoise)};
        auto cliff = evaluateSource(recipe, input, 12);
        require(meshHash(cliff.mesh) != meshHash(input.mesh), "actual selected BSP geometry is deformed");
        require(cliff.mesh.triangleRegions == input.mesh.triangleRegions,
                "BSP source regions survive field operations");
        require(cliff.sourceSelection == input.sourceSelection && cliff.sourceRevision == 37,
                "source identity survives stack");
        require(PA::hashDocument(document) == originalHash,
                "original BSP document and unselected brush remain unchanged");
        std::uint64_t archHash = 0, squareHash = 0;
        for (auto form : {CaveForm::Arch, CaveForm::SquareTunnel})
        {
            auto cave = caveSource(form, 4);
            require(cave.closedSolid && PA::validateClosedMesh(cave.mesh).hasValue(),
                    "subtractive cave source remains a closed solid");
            require(cave.sourceSelection.size() == 2 && !cave.regions.empty(),
                    "cave retains additive and cutter BSP identities");
            auto volume = 0.0;
            for (std::size_t t = 0; t < cave.mesh.indices.size(); t += 3)
            {
                auto p = cave.mesh.positions[cave.mesh.indices[t]],
                     q = cave.mesh.positions[cave.mesh.indices[t + 1]],
                     r = cave.mesh.positions[cave.mesh.indices[t + 2]];
                volume += (p.x * (q.y * r.z - q.z * r.y) +
                           p.y * (q.z * r.x - q.x * r.z) +
                           p.z * (q.x * r.y - q.y * r.x)) / 6.0;
            }
            require(std::abs(volume) > 40 && std::abs(volume) < 70,
                    "cave cutter removes a substantial through-passage from the rock mass");
            if (form == CaveForm::Arch)
            {
                bool slopedExterior = false;
                for (std::size_t t = 0; t < cave.mesh.indices.size(); t += 3)
                {
                    auto p = cave.mesh.positions[cave.mesh.indices[t]],
                         q = cave.mesh.positions[cave.mesh.indices[t + 1]],
                         r = cave.mesh.positions[cave.mesh.indices[t + 2]];
                    const auto x = (p.x + q.x + r.x) / 3.0;
                    const auto y = (p.y + q.y + r.y) / 3.0;
                    if (x > 2.0 && y > 1.0 &&
                        std::abs(cave.normals[cave.mesh.indices[t]].x) > 0.1 &&
                        std::abs(cave.normals[cave.mesh.indices[t]].y) > 0.1)
                        slopedExterior = true;
                }
                require(slopedExterior, "cave arch has a faceted rock silhouette outside its entrance");
            }
            std::set<std::pair<std::uint32_t, std::uint32_t>> edges;
            for (std::size_t t = 0; t < cave.mesh.indices.size(); t += 3)
                for (int edge = 0; edge < 3; ++edge)
                {
                    auto a = cave.mesh.indices[t + edge], b = cave.mesh.indices[t + (edge + 1) % 3];
                    edges.insert(std::minmax(a, b));
                }
            const auto euler = std::int64_t(cave.mesh.positions.size()) -
                               std::int64_t(edges.size()) +
                               std::int64_t(cave.mesh.indices.size() / 3);
            std::cout << "cave topology " << int(form == CaveForm::Arch) << " volume " << volume
                      << " V/E/F "
                      << cave.mesh.positions.size() << '/' << edges.size() << '/'
                      << cave.mesh.indices.size() / 3 << " Euler " << euler << '\n';
            require(euler == 0, "cave opening has through-tunnel topology, not a sealed pocket");
            const auto hash = PA::hashTriangleMesh(cave.mesh);
            require(hash == PA::hashTriangleMesh(caveSource(form, 4).mesh),
                    "cave geometry and cut regions replay deterministically");
            (form == CaveForm::Arch ? archHash : squareHash) = hash;
        }
        require(archHash != squareHash, "arched cave and square tunnel have different geometry");
        const auto archBase = caveSource(CaveForm::Arch, 4);
        const auto archGeology = geologicalCaveSource(CaveForm::Arch, 4);
        require(archGeology.closedSolid && PA::validateClosedMesh(archGeology.mesh).hasValue(),
                "geological arch stays a closed through-passage");
        require(archGeology.mesh.indices == archBase.mesh.indices &&
                    archGeology.mesh.triangleRegions == archBase.mesh.triangleRegions &&
                    archGeology.sourceSelection == archBase.sourceSelection,
                "geological deformation preserves cave topology and BSP selection");
        double exteriorDepthChange = 0;
        std::size_t exteriorCount = 0;
        for (std::size_t i = 0; i < archBase.mesh.positions.size(); ++i)
            if (archBase.mesh.positions[i].z > 1.49 &&
                std::abs(archBase.mesh.positions[i].x) > 1.4)
            {
                exteriorDepthChange += std::abs(archGeology.mesh.positions[i].z -
                                                archBase.mesh.positions[i].z);
                ++exteriorCount;
            }
        require(exteriorCount > 10 && exteriorDepthChange / exteriorCount > 0.05,
                "geological arch has volumetric exterior relief, not a flat perforated slab");
        require(PA::hashTriangleMesh(archGeology.mesh) ==
                    PA::hashTriangleMesh(geologicalCaveSource(CaveForm::Arch, 4).mesh),
                "geological arch replays deterministically");
        auto faceNormal = [](const TriangleMesh &mesh, std::size_t triangle)
        {
            const auto a = mesh.positions[mesh.indices[triangle]],
                       b = mesh.positions[mesh.indices[triangle + 1]],
                       c = mesh.positions[mesh.indices[triangle + 2]];
            const auto u = Vec3d{b.x - a.x, b.y - a.y, b.z - a.z};
            const auto v = Vec3d{c.x - a.x, c.y - a.y, c.z - a.z};
            return Vec3d{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z,
                         u.x * v.y - u.y * v.x};
        };
        for (int subdivisions : {2, 4, 6, 8})
        {
            const auto before = caveSource(CaveForm::Arch, subdivisions);
            const auto after = geologicalCaveSource(CaveForm::Arch, subdivisions);
            require(PA::validateClosedMesh(after.mesh).hasValue(),
                    "each geological arch LOD remains closed");
            for (std::size_t triangle = 0; triangle < before.mesh.indices.size(); triangle += 3)
            {
                const auto x = faceNormal(before.mesh, triangle),
                           y = faceNormal(after.mesh, triangle);
                require(x.x * y.x + x.y * y.y + x.z * y.z > 0,
                        "geological cave warp does not invert a face at any sampled LOD");
            }
        }
        require(PA::hashTriangleMesh(geologicalCaveSource(CaveForm::SquareTunnel, 4).mesh) == squareHash,
                "square tunnel remains the undeformed control");
        require(PA::hashTriangleMesh(cliff.mesh) ==
                    PA::hashTriangleMesh(evaluateSource(deserialize(serialize(recipe)), input, 12).mesh),
                "BSP evaluation deterministic");
        bool rejected = false;
        auto missing = Solid::region(9, 9);
        try
        {
            bspSource(document, std::span<const Jerico::Guid>(&missing, 1), 12);
        }
        catch (const std::exception &)
        {
            rejected = true;
        }
        require(rejected, "stale selection rejected");
        auto invalid = input;
        invalid.mesh.indices[0] = 999999;
        rejected = false;
        try
        {
            evaluateSource(recipe, invalid, 12);
        }
        catch (const std::exception &)
        {
            rejected = true;
        }
        require(rejected, "invalid source is rejected before evaluation");
        invalid = input;
        invalid.layerThickness = {0.2f};
        rejected = false;
        try
        {
            evaluateSource(recipe, invalid, 12);
        }
        catch (const std::exception &)
        {
            rejected = true;
        }
        require(rejected, "misaligned source metadata is rejected before evaluation");
        std::cout << "PASS real engine BSP selection -> geology: GUIDs, revision, source immutability, "
                     "unselected isolation, translated layered shape, round trip, invalid input\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
