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
double variation(const MeshResult &mesh, int layers)
{
    std::vector<Vec3d> centers(layers);
    std::vector<int> counts(layers);
    for (std::size_t i = 0; i < mesh.mesh.positions.size(); ++i)
    {
        auto k = mesh.layerIndices[i];
        auto p = mesh.mesh.positions[i];
        centers[k].x += p.x;
        centers[k].z += p.z;
        ++counts[k];
    }
    for (int k = 0; k < layers; ++k)
    {
        centers[k].x /= counts[k];
        centers[k].z /= counts[k];
    }
    double value = 0;
    for (int k = 1; k < layers; ++k)
        value += std::hypot(centers[k].x - centers[k - 1].x, centers[k].z - centers[k - 1].z);
    return value;
}
int main()
{
    try
    {
        Recipe recipe;
        auto l = defaultLayer(Kind::Layers);
        l.coherentLayers = true;
        l.count = 8;
        l.jitter = 0;
        l.layerScale = 0;
        l.lateral = 0.6f;
        l.driftCorrelation = 0;
        recipe.layers = {l};
        auto independent = evaluate(recipe, 12);
        recipe.layers[0].driftCorrelation = 0.95f;
        auto correlated = evaluate(recipe, 12);
        require(variation(correlated, 8) < variation(independent, 8) * 0.2,
                "correlation must reduce adjacent layer drift");
        require(meshHash(correlated.mesh) == meshHash(evaluate(recipe, 12).mesh),
                "correlated drift is deterministic");
        require(meshHash(correlated.mesh) == meshHash(evaluate(deserialize(serialize(recipe)), 12).mesh),
                "correlated drift survives recipe round trip");
        recipe.layers[0].driftCorrelation = 1;
        require(variation(evaluate(recipe, 12), 8) < 1e-10, "full correlation shares drift across layers");
        std::cout
            << "PASS correlated drift: adjacent coherence, full correlation and deterministic behavior\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
