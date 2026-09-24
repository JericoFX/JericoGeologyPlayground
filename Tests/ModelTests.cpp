#include "Model.hpp"
#include <Jerico/ProceduralAuthoring/Geology/GeologyNoise.hpp>
#include <cmath>
#include <iostream>
#include <limits>
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
        Jerico::CancellationSource cancellation;
        cancellation.requestCancellation();
        auto cancelled = cancellation.token();
        bool cancellationObserved = false;
        try
        {
            evaluate(recipe, 12, -1, -1, &cancelled);
        }
        catch (const std::exception &)
        {
            cancellationObserved = true;
        }
        require(cancellationObserved, "cancelled empty stack must stop before primitive generation");
        auto mesh = evaluate(recipe, 12);
        require(!mesh.mesh.positions.empty() && mesh.mesh.indices.size() >= 12,
                "RED: base primitive must produce a real indexed mesh");
        Recipe planeRecipe;
        planeRecipe.layers = {defaultLayer(static_cast<Kind>(8))};
        auto planeRock = evaluate(planeRecipe, 12);
        require(planeRock.mesh.triangleRegions.size() == planeRock.mesh.indices.size() / 3,
                "structural planes must preserve a region for every output triangle");
        require(meshHash(planeRock.mesh) != meshHash(mesh.mesh),
                "structural planes must change the base silhouette");
        for (int i = 0; i < 4; ++i)
        {
            recipe.base = static_cast<Base>(i);
            auto base = evaluate(recipe, 12);
            require(!base.mesh.positions.empty(), "each base needs geometry");
            require(base.normals.size() == base.mesh.positions.size(), "normal count");
            for (auto index : base.mesh.indices)
                require(index < base.mesh.positions.size(), "valid index");
        }
        recipe.base = Base::Sphere;
        recipe.layers = {defaultLayer(Kind::NormalNoise)};
        auto displaced = evaluate(recipe, 12);
        auto repeated = evaluate(recipe, 12);
        require(meshHash(displaced.mesh) == meshHash(repeated.mesh), "deterministic CPU recipe");
        Recipe disabled = recipe;
        disabled.layers[0].enabled = false;
        Recipe plain = recipe;
        plain.layers.clear();
        require(meshHash(evaluate(disabled, 12).mesh) == meshHash(evaluate(plain, 12).mesh),
                "disabled identity");
        auto transform = defaultLayer(Kind::Transform);
        transform.translation = {1.1f, 0.2f, 0};
        recipe.layers.push_back(transform);
        auto first = evaluate(recipe, 12);
        std::swap(recipe.layers[0], recipe.layers[1]);
        require(meshHash(first.mesh) != meshHash(evaluate(recipe, 12).mesh),
                "order must change noncommuting operations");
        require(meshHash(evaluate(deserialize(serialize(recipe)), 12).mesh) ==
                    meshHash(evaluate(recipe, 12).mesh),
                "recipe roundtrip");
        Recipe normal = plain;
        normal.layers = {defaultLayer(Kind::NormalNoise)};
        Recipe vector = normal;
        vector.layers[0].kind = Kind::VectorNoise;
        require(meshHash(evaluate(normal, 12).mesh) != meshHash(evaluate(vector, 12).mesh),
                "normal and vector models differ");
        Recipe layered = plain;
        layered.layers = {defaultLayer(Kind::Layers)};
        auto slabs = evaluate(layered, 12);
        require(slabs.mesh.positions.size() == evaluate(plain, 12).mesh.positions.size() * 4,
                "layers construct real repeated geometry");
        Recipe warp = plain;
        warp.layers = {defaultLayer(Kind::Warp)};
        require(meshHash(evaluate(warp, 12).mesh) == meshHash(evaluate(plain, 12).mesh),
                "warp changes sampling, not geometry alone");
        warp.layers.push_back(defaultLayer(Kind::NormalNoise));
        require(meshHash(evaluate(warp, 12).mesh) != meshHash(evaluate(normal, 12).mesh),
                "warp changes subsequent field");
        // Scaling the sampling domain back must preserve a previously warped field.
        Recipe scaledWarp = warp;
        auto uniformScale = defaultLayer(Kind::Transform);
        uniformScale.scale = {2, 2, 2};
        scaledWarp.layers.insert(scaledWarp.layers.begin() + 1, uniformScale);
        scaledWarp.layers.back().frequency *= 0.5f;
        scaledWarp.layers.back().amplitude *= 2;
        auto originalWarp = evaluate(warp, 12);
        auto scaledResult = evaluate(scaledWarp, 12);
        for (std::size_t i = 0; i < originalWarp.mesh.positions.size(); ++i)
        {
            auto a = originalWarp.mesh.positions[i], b = scaledResult.mesh.positions[i];
            require(std::abs(2 * a.x - b.x) + std::abs(2 * a.y - b.y) + std::abs(2 * a.z - b.z) < 1e-8,
                    "warp transforms with object-space geometry");
        }
        Recipe relaxation = normal;
        auto relax = defaultLayer(Kind::Relax);
        relax.mask = Mask::Slope;
        relax.iterations = 2;
        relaxation.layers.push_back(relax);
        auto iterative = evaluate(relaxation, 12);
        relaxation.layers.back().iterations = 1;
        relaxation.layers.push_back(relaxation.layers.back());
        require(meshHash(iterative.mesh) == meshHash(evaluate(relaxation, 12).mesh),
                "relax recomputes mask normals every iteration");
        for (int k = 0; k < 8; ++k)
        {
            Recipe one = plain;
            one.layers = {defaultLayer(static_cast<Kind>(k))};
            auto result = evaluate(one, 12, 0, 0);
            require(result.field.size() == result.mesh.positions.size(), "field aligns with mesh");
            for (auto p : result.mesh.positions)
                require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z), "finite result");
        }
        bool rejected = false;
        try
        {
            auto invalid = deserialize("{\"version\":99}");
            (void)invalid;
        }
        catch (...)
        {
            rejected = true;
        }
        require(rejected, "unknown recipe version rejected");
        auto bad = plain;
        bad.layers = {defaultLayer(Kind::NormalNoise)};
        bad.layers[0].frequency = std::numeric_limits<float>::infinity();
        rejected = false;
        try
        {
            evaluate(bad, 12);
        }
        catch (...)
        {
            rejected = true;
        }
        require(rejected, "non-finite parameters rejected");
        auto cpu = PA::evaluateGeologyFbmReference({0.3, 1.7, -0.4}, 123, 4, 2, 0.5);
        require(std::isfinite(cpu), "actual engine FBM source linked and evaluated");
        std::cout << "PASS: four bases, topology, deterministic recipes, disabled identity, order, "
                     "roundtrip, displacement models, structural layers, domain warp, fields, invalid input, "
                     "shared engine FBM\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
