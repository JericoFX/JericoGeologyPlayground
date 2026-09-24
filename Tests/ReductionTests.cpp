#include "Model.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <iostream>
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
        auto wall = evaluate(recipe, 64);
        recipe.layers.push_back(reduce);
        auto low = evaluate(recipe, 64);
        require(low.mesh.indices.size() <= wall.mesh.indices.size(),
                "quality fallback never increases triangles");
    require(low.sampledReductionError <= 0.01, "cliff stays inside declared sampled error");
        require(PA::hashTriangleMesh(low.mesh) ==
                    PA::hashTriangleMesh(evaluate(deserialize(serialize(recipe)), 64).mesh),
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
                require(candidate.mesh.indices.size() < wall.mesh.indices.size(),
                        "cliff reduces at the separately declared 0.05-unit budget");
            std::cout << "cliff limit " << limit << ": " << wall.mesh.indices.size() / 3 << " -> "
                      << candidate.mesh.indices.size() / 3 << ", sampled error "
                      << candidate.sampledReductionError << '\n';
        }
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
