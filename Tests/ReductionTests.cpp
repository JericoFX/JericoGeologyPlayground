#include "Model.hpp"
#include "MeshReduction.hpp"
#include "SurfaceGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace Playground;
void require(bool v, const char *m)
{
    if (!v)
        throw std::runtime_error(m);
}
double passageRightWall(const TriangleMesh &mesh, double z)
{
    constexpr double y = -0.7;
    double nearest = std::numeric_limits<double>::infinity();
    for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
    {
        const auto a = mesh.positions[mesh.indices[t]], b = mesh.positions[mesh.indices[t + 1]],
                   c = mesh.positions[mesh.indices[t + 2]];
        const double denominator = (b.y - a.y) * (c.z - a.z) -
                                   (c.y - a.y) * (b.z - a.z);
        if (std::abs(denominator) < 1e-10)
            continue;
        const double v = ((y - a.y) * (c.z - a.z) -
                          (c.y - a.y) * (z - a.z)) / denominator;
        const double w = ((b.y - a.y) * (z - a.z) -
                          (y - a.y) * (b.z - a.z)) / denominator;
        if (v < -1e-8 || w < -1e-8 || v + w > 1.0 + 1e-8)
            continue;
        const double x = a.x + v * (b.x - a.x) + w * (c.x - a.x);
        if (x > 0.1)
            nearest = std::min(nearest, x);
    }
    return nearest;
}
int main()
{
    try
    {
        Recipe recipe;
        auto source = evaluate(recipe, 24);
        Layer reduce;
        reduce.kind = Kind::Reduce;
        reduce.reductionError = 0.01f;
        recipe.layers = {reduce};
        auto reduced = evaluate(recipe, 24);
        require(reduced.mesh.indices.size() < source.mesh.indices.size() / 4,
                "planar dense cube should reduce strongly");
        require(reduced.sampledReductionError <= 0.01, "reduction respects sampled bidirectional error");
        require(PA::validateClosedMesh(reduced.mesh).hasValue(), "reduced cube stays closed");
        recipe = preset(8);
        recipe.version = 10;
        recipe.layers[4].profile = 0;
        auto wall = evaluate(recipe, 64);
        Jerico::Guid cutId{}, jointId{};
        for (const auto &region : wall.regions)
        {
            if (region.role == SurfaceRole::CutLoop) cutId = region.id;
            if (region.role == SurfaceRole::CutFace) jointId = region.id;
        }
        require(cutId != Jerico::Guid{} && jointId != Jerico::Guid{},
                "high cliff retains cut and joint face identities");
        auto jointArea = [&](const auto &mesh)
        {
            double area = 0;
            for (std::size_t t = 0; t < mesh.triangleRegions.size(); ++t)
                if (mesh.triangleRegions[t] == jointId)
                {
                    const auto a = mesh.positions[mesh.indices[3 * t]],
                               b = mesh.positions[mesh.indices[3 * t + 1]],
                               c = mesh.positions[mesh.indices[3 * t + 2]];
                    const auto ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
                    const auto vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
                    area += 0.5 * std::sqrt(std::pow(uy * vz - uz * vy, 2) +
                                            std::pow(uz * vx - ux * vz, 2) +
                                            std::pow(ux * vy - uy * vx, 2));
                }
            return area;
        };
        const auto highJointArea = jointArea(wall.mesh);
        require(highJointArea > 0.1, "high cliff contains a physical joint face");
        recipe.layers.push_back(reduce);
        auto low = evaluate(recipe, 64);
        require(low.mesh.indices.size() <= wall.mesh.indices.size(),
                "quality fallback never increases triangles");
        require(low.sampledReductionError <= 0.01, "cliff stays inside declared sampled error");
        auto replay = evaluate(deserialize(serialize(recipe)), 64);
        require(PA::hashTriangleMesh(low.mesh) == PA::hashTriangleMesh(replay.mesh),
                "reduction reproducible with region identity");
        std::cout << "PASS quality gate: cube " << source.mesh.indices.size() / 3 << " -> "
                  << reduced.mesh.indices.size() / 3 << ", cliff " << wall.mesh.indices.size() / 3 << " -> "
                  << low.mesh.indices.size() / 3 << ", sampled error " << low.sampledReductionError << '\n';
        for (float limit : {0.005f, 0.02f, 0.05f})
        {
            recipe.layers.back().reductionError = limit;
            auto candidate = evaluate(recipe, 64);
            require(candidate.sampledReductionError <= limit, "each explicit quality budget is enforced");
            if (limit == 0.05f)
            {
                require(candidate.mesh.indices.size() < wall.mesh.indices.size(),
                        "cliff reduces at the separately declared 0.05-unit budget");
                require(std::count(candidate.mesh.triangleRegions.begin(),
                                   candidate.mesh.triangleRegions.end(), cutId) > 0 &&
                            std::count(candidate.mesh.triangleRegions.begin(),
                                       candidate.mesh.triangleRegions.end(), jointId) > 0,
                        "low cliff preserves both block and joint faces");
                require(jointArea(candidate.mesh) >= 0.5 * highJointArea,
                        "low cliff retains substantial physical joint area");
            }
            std::cout << "cliff limit " << limit << ": " << wall.mesh.indices.size() / 3 << " -> "
                      << candidate.mesh.indices.size() / 3 << ", sampled error "
                      << candidate.sampledReductionError << '\n';
        }
        auto layered = preset(11);
        for (auto &layer : layered.layers) layer.seed = 123;
        auto layeredHigh = evaluate(layered, layered.finalResolution);
        std::size_t previousLayeredTriangles = layeredHigh.mesh.indices.size() / 3;
        for (const auto &[resolution, budget] : std::array<std::pair<int, double>, 3>{
                 {{20, 0.02}, {16, 0.03}, {14, 0.04}}})
        {
            auto regenerated = evaluate(layered, resolution);
            const auto triangles = regenerated.mesh.indices.size() / 3;
            require(triangles < previousLayeredTriangles,
                    "regenerated layered LODs must strictly reduce triangle count");
            require(sampledBidirectionalMeshError(layeredHigh.mesh, regenerated.mesh) <= budget,
                    "regenerated layered LOD stays inside its original surface-error budget");
            require(PA::validateClosedMesh(regenerated.mesh).hasValue(),
                    "regenerated layered LOD remains closed");
            previousLayeredTriangles = triangles;
        }
        for (auto caveForm : {CaveForm::Arch, CaveForm::BlindArch})
        {
            auto cave = axialCaveSource(caveForm, 8);
            require(PA::validateClosedMesh(cave.mesh).hasValue(), "profiled cave remains closed");
            const auto caveMacro = cave;
            Recipe caveDetail;
            auto caveNoise = defaultLayer(Kind::NormalNoise);
            caveNoise.seed = 123;
            caveNoise.amplitude = 0.12f;
            caveNoise.frequency = 2.5f;
            caveNoise.lacunarity = 1.87f;
            caveDetail.layers = {caveNoise};
            cave = evaluateSource(caveDetail, cave, 8);
            const std::array stations{1.3, 0.2, -0.7};
            const std::array highWalls{passageRightWall(cave.mesh, stations[0]),
                                       passageRightWall(cave.mesh, stations[1]),
                                       passageRightWall(cave.mesh, stations[2])};
            std::cout << "profiled cave high wall mouth/throat/chamber " << highWalls[0]
                      << '/' << highWalls[1] << '/' << highWalls[2] << '\n';
            require(std::all_of(highWalls.begin(), highWalls.end(),
                                [](double width) { return std::isfinite(width); }) &&
                        highWalls[0] > highWalls[1] + 0.15 &&
                        highWalls[2] > highWalls[1] + 0.15,
                    "high cave contains a flared mouth, narrow throat and wider chamber");
            std::size_t previousCaveTriangles = cave.mesh.indices.size() / 3;
            for (double budget : {0.01, 0.05, 0.10})
            {
                double sampled = 0;
                const auto candidate = reduceClosedMesh(caveMacro.mesh, budget, sampled, nullptr);
                const auto triangles = candidate.indices.size() / 3;
                const auto highError = sampledBidirectionalMeshError(cave.mesh, candidate);
                require(triangles < previousCaveTriangles && sampled <= budget && highError <= 0.15,
                        "profiled cave LOD must reduce while preserving its high surface");
                require(PA::validateClosedMesh(candidate).hasValue(),
                        "profiled cave LOD remains a closed solid");
                for (std::size_t station = 0; station < stations.size(); ++station)
                {
                    const double lowWall = passageRightWall(candidate, stations[station]);
                    std::cout << "  station " << stations[station] << " right wall " << lowWall << '\n';
                    require(std::abs(lowWall - highWalls[station]) <= 0.15,
                            "profiled cave LOD preserves each measured passage section");
                }
                previousCaveTriangles = triangles;
                std::cout << "profiled cave reduction " << budget << ' ' <<
                    cave.mesh.indices.size() / 3 << " -> " << triangles <<
                    " sampled " << sampled << " detailed " << highError << '\n';
            }
        }
        require(PA::hashTriangleMesh(axialCaveSource(CaveForm::SquareTunnel, 8).mesh) ==
                    PA::hashTriangleMesh(geologicalCaveSource(CaveForm::SquareTunnel, 8).mesh),
                "axial cave experiment leaves square tunnel unchanged");
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
