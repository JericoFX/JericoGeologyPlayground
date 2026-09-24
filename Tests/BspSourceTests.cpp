#include "SolidGeometry.hpp"
#include "SurfaceGeometry.hpp"
#include "MeshReduction.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
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
