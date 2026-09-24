#include "Model.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace Playground;
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main()
{
    try
    {
        Recipe recipe;
        recipe.layers = {defaultLayer(Kind::Layers)};
        auto base = evaluate(recipe, 12);
        auto shared = defaultLayer(static_cast<Kind>(9));
        recipe.layers.push_back(shared);
        auto moved = evaluate(recipe, 12, 1, 1);
        require(meshHash(base.mesh) != meshHash(moved.mesh), "shared warp must move the entire formation");
        require(base.mesh.indices == moved.mesh.indices, "shared warp preserves topology");
        require(moved.mesh.triangleRegions == base.mesh.triangleRegions,
                "shared warp preserves region identity");
        auto repeat = evaluate(recipe, 12);
        require(meshHash(moved.mesh) == meshHash(repeat.mesh), "shared warp deterministic");
        recipe.layers.back().amplitude = 0;
        require(meshHash(base.mesh) == meshHash(evaluate(recipe, 12).mesh), "zero shared warp is identity");
        recipe.layers.back().amplitude = shared.amplitude;
        recipe.layers.back().enabled = false;
        require(meshHash(base.mesh) == meshHash(evaluate(recipe, 12).mesh),
                "disabled shared warp is identity");
        std::cout << "PASS shared macro warp: positions, topology, regions, deterministic, zero/disabled "
                     "identity\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
