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
        recipe.version=24;
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
        Recipe unsafePolicy;
        unsafePolicy.version=23;
        unsafePolicy.layers={edge};
        bool oldVersionRejected=false;
        try {serialize(unsafePolicy);}
        catch (const std::exception &) {oldVersionRejected=true;}
        require(oldVersionRejected,
                "new curvature policies require a recipe version old readers reject");
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
        MeshResult concaveSource;
        concaveSource.closedSolid=false;
        concaveSource.mesh.positions={{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
        concaveSource.mesh.indices={0,1,2,1,0,3};
        concaveSource.mesh.triangleRegions.resize(2);
        const auto concaveFeatures=surfaceFeatures(concaveSource.mesh,{.contact=false});
        require(concaveFeatures.signedDihedralCurvature[0]<0,
                "fixture must contain a concave signed crease");
        const auto concaveResult=evaluateSource(recipe,concaveSource,8);
        require(meshHash(concaveResult.mesh)==meshHash(concaveSource.mesh),
                "default shared edge damage must not chip concave creases");
        recipe.layers.back().edgeCurvaturePolicy=1;
        const auto selectedConcave=evaluateSource(recipe,concaveSource,8);
        require(meshHash(selectedConcave.mesh)!=meshHash(concaveSource.mesh),
                "concave policy must deliberately chip the recessed crease");
        recipe.layers.back().edgeCurvaturePolicy=2;
        require(meshHash(evaluateSource(recipe,concaveSource,8).mesh)==
                    meshHash(selectedConcave.mesh),
                "both-edge policy must retain concave damage");
        auto savedShared=nlohmann::json::parse(serialize(recipe));
        savedShared.at("layers").at(0).erase("edgeCurvaturePolicy");
        const auto restoredShared=deserialize(savedShared.dump());
        require(restoredShared.layers.back().edgeCurvaturePolicy==2 &&
                    meshHash(evaluateSource(restoredShared,concaveSource,8).mesh)==
                    meshHash(selectedConcave.mesh),
                "older shared-edge recipes without a policy retain both-side curvature");
        recipe.layers.back()=edge;
        Recipe smooth;
        smooth.version=24;
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
