#include "Model.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
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
        auto layers = defaultLayer(Kind::Layers);
        layers.coherentLayers = true;
        layers.count = 3;
        layers.jitter = 0;
        layers.lateral = 0;
        layers.layerScale = 0;
        recipe.layers = {layers};
        auto source = evaluate(recipe, 16);
        Layer edge;
        edge.kind = Kind::EdgeBreakup;
        edge.amplitude = 0.03f;
        edge.width = 0.25f;
        edge.frequency = 5;
        recipe.layers.push_back(edge);
        auto result = evaluate(recipe, 16, 1, 1);
        require(result.layerEdgeDistance.size() == source.mesh.positions.size(),
                "edge distance is available per vertex");
        int centers = 0, changedEdges = 0;
        for (std::size_t i = 0; i < source.mesh.positions.size(); ++i)
        {
            auto a = source.mesh.positions[i], b = result.mesh.positions[i];
            double displacement = std::hypot(a.x - b.x, a.y - b.y, a.z - b.z);
            if (std::abs(a.x) < 0.5 && std::abs(a.z) < 0.5)
            {
                ++centers;
                require(displacement < 1e-12, "face centers must remain unchanged outside edge band");
            }
            if (displacement > 1e-7)
            {
                ++changedEdges;
                require(result.layerEdgeDistance[i] < edge.width, "changes remain in edge band");
            }
        }
        require(centers > 20 && changedEdges > 20, "test includes unaffected centers and displaced edges");
        require(result.mesh.indices == source.mesh.indices &&
                    result.mesh.triangleRegions == source.mesh.triangleRegions,
                "edge breakup preserves topology and regions");
        require(PA::validateClosedMesh(result.mesh).hasValue(), "bounded edge breakup remains a closed mesh");
        require(meshHash(result.mesh) == meshHash(evaluate(deserialize(serialize(recipe)), 16).mesh),
                "edge recipe round trip");
        edge.amplitude = 0;
        recipe.layers.back() = edge;
        require(meshHash(evaluate(recipe, 16).mesh) == meshHash(source.mesh), "zero strength is identity");
        recipe.layers = {edge};
        bool rejected = false;
        try
        {
            evaluate(recipe, 8);
        }
        catch (const std::exception &)
        {
            rejected = true;
        }
        require(rejected, "edge breakup requires a layer formation upstream");
        std::cout << "PASS edge breakup: geometric distance, centers protected, bounded edges, topology, "
                     "closedness, recipe and precondition\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
