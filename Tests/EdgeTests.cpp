#include "Model.hpp"
#include "SurfaceFeatures.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
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
        edge.useSharedFeatureEdges=true;
        edge.amplitude = 0.03f;
        edge.width = 0.25f;
        edge.frequency = 5;
        recipe.layers.push_back(edge);
        auto result = evaluate(recipe, 16, 1, 1);
        require(result.featureCacheMisses>0,
                "edge damage must consume the shared surface feature analysis");
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
        auto extreme=edge;
        extreme.amplitude=5;
        extreme.frequency=100;
        recipe.layers.back()=extreme;
        const auto stressed=evaluate(recipe,16);
        const auto featureField=surfaceFeatures(source.mesh,{.contact=false});
        for (std::size_t i=0;i<source.mesh.positions.size();++i)
        {
            const auto a=source.mesh.positions[i],b=stressed.mesh.positions[i];
            const auto distance=std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);
            require(distance<=.2*featureField.minimumTriangleAltitude[i]+1e-9,
                    "extreme edge damage must stay below local triangle altitude budget");
        }
        require(PA::validateClosedMesh(stressed.mesh).hasValue(),
                "extreme shared edge damage must retain closed topology");
        recipe.layers.back()=edge;
        edge.amplitude = 0;
        recipe.layers.back() = edge;
        require(meshHash(evaluate(recipe, 16).mesh) == meshHash(source.mesh), "zero strength is identity");
        recipe.layers = {edge};
        edge.amplitude=.03f;
        recipe.layers.back()=edge;
        const auto standalone=evaluate(recipe,8);
        require(standalone.featureCacheMisses>0 &&
                    standalone.layerEdgeDistance.size()==standalone.mesh.positions.size(),
                "shared edge damage must work on an unlayered rock too");
        Recipe smooth;
        smooth.base=Base::Sphere;
        smooth.layers={edge,defaultLayer(Kind::NormalNoise)};
        const auto sphere=evaluate(smooth,16);
        require(sphere.mesh.positions.size()==sphere.layerEdgeDistance.size() &&
                    std::all_of(sphere.layerEdgeDistance.begin(),sphere.layerEdgeDistance.end(),
                        [](float value){return std::isfinite(value);}),
                "smooth surfaces without creases must preserve finite metadata for later stages");
        auto legacy=edge;
        legacy.useSharedFeatureEdges=false;
        Recipe oldRecipe;
        oldRecipe.layers={layers,legacy};
        const auto oldMesh=evaluate(oldRecipe,16);
        auto oldJson=nlohmann::json::parse(serialize(oldRecipe));
        for (auto &operation:oldJson.at("layers"))
            operation.erase("useSharedFeatureEdges");
        const auto loadedOld=deserialize(oldJson.dump());
        require(!loadedOld.layers.back().useSharedFeatureEdges &&
                    meshHash(evaluate(loadedOld,16).mesh)==meshHash(oldMesh.mesh),
                "saved edge recipes without the new field retain legacy stratum geometry");
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
