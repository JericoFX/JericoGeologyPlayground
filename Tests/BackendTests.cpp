#include "Model.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
using namespace Playground;
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main(int argc, char **argv)
{
    try
    {
        auto sample = evaluate(preset(5), 12);
        require(sample.stages.size() == preset(5).layers.size(),
                "backend exposes timings for enabled operations");
        require(!sample.regions.empty(), "backend exposes surface lineage");
        std::cout << "preset,seed,resolution,vertices,triangles,cpu_ms,geometry_hash\n";
        for (int family : {5, 6})
            for (int seed : {1, 19, 123, 927, 99991})
                for (int resolution : {12, 24})
                {
                    auto recipe = preset(family);
                    for (auto &l : recipe.layers)
                        l.seed = seed;
                    auto mesh = evaluate(recipe, resolution);
                    require(PA::validateClosedMesh(mesh.mesh).hasValue(),
                            "combined family must pass native closed mesh validation");
                    require(
                        PA::hashTriangleMesh(mesh.mesh) ==
                            PA::hashTriangleMesh(evaluate(deserialize(serialize(recipe)), resolution).mesh),
                        "seed matrix round trip preserves geometry and regions");
                    require(mesh.mesh.positions.size() <= 240000 && mesh.mesh.indices.size() / 3 <= 240000,
                            "combined budgets");
                    std::set<Jerico::Guid> known;
                    for (auto &region : mesh.regions)
                        known.insert(region.id);
                    for (auto id : mesh.mesh.triangleRegions)
                        require(known.contains(id), "every triangle region has a semantic descriptor");
                    for (auto &region : mesh.regions)
                        if (region.role == SurfaceRole::Stratum)
                            require(known.contains(region.parent), "stratum retains source lineage");
                    double sum = 0;
                    for (auto &stage : mesh.stages)
                    {
                        require(stage.milliseconds >= 0, "finite positive stage duration");
                        sum += stage.milliseconds;
                    }
                    require(sum <= mesh.milliseconds + 0.1, "stage scope is contained in generation time");
                    std::cout << family << ',' << seed << ',' << resolution << ','
                              << mesh.mesh.positions.size() << ',' << mesh.mesh.indices.size() / 3 << ','
                              << mesh.milliseconds << ',' << meshHash(mesh.mesh) << '\n';
                }
        if (argc > 1)
        {
            auto root = std::filesystem::path(argv[1]);
            nlohmann::json baseline;
            std::ifstream(root / "runtime-proof.json") >> baseline;
            for (auto &p : baseline.at("presets"))
            {
                int index = p.at("preset");
                auto recipe = load(root / ("preset-" + std::to_string(index) + ".json"));
                require(meshHash(evaluate(recipe, 32).mesh) == p.at("hash").get<std::uint64_t>(),
                        "original v1 recipe geometry remains unchanged");
            }
        }
        for (int family : {5, 6})
        {
            auto recipe = preset(family);
            auto mesh = evaluate(recipe, recipe.finalResolution);
            require(PA::validateClosedMesh(mesh.mesh).hasValue(),
                    "preset declared final resolution must be usable");
        }
        Jerico::CancellationSource cancellation;
        cancellation.requestCancellation();
        auto cancel = cancellation.token();
        bool rejected = false;
        try
        {
            evaluate(preset(5), 24, -1, -1, &cancel);
        }
        catch (const std::exception &)
        {
            rejected = true;
        }
        require(rejected, "backend cancellation is visible");
        std::cout << "PASS backend without renderer: 20 family/seed/resolution cases, semantic lineage, "
                     "timings, old recipes, cancellation\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
