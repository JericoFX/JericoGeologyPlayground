#include "Model.hpp"
#include <Jerico/ProceduralAuthoring/Geology/GeologyNoise.hpp>
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
using namespace Playground;
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
double maximumEdgeStretch(const TriangleMesh &before, const TriangleMesh &after)
{
    require(before.indices == after.indices && before.positions.size() == after.positions.size(),
            "displacement must preserve mesh connectivity");
    double maximum = 0;
    for (std::size_t corner = 0; corner < before.indices.size(); ++corner)
    {
        auto a = before.indices[corner];
        auto b = before.indices[corner - corner % 3 + (corner + 1) % 3];
        auto distance = [](Vec3d x, Vec3d y)
        {
            return std::hypot(x.x - y.x, x.y - y.y, x.z - y.z);
        };
        maximum = std::max(maximum, distance(after.positions[a], after.positions[b]) /
                                        distance(before.positions[a], before.positions[b]));
    }
    return maximum;
}
double minimumFaceOrientation(const TriangleMesh &before, const TriangleMesh &after)
{
    require(before.indices == after.indices, "displacement must preserve faces");
    double minimum = 1;
    auto cross = [](Vec3d a, Vec3d b) -> Vec3d
    {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    };
    auto difference = [](Vec3d a, Vec3d b) -> Vec3d
    {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    };
    auto dot = [](Vec3d a, Vec3d b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
    for (std::size_t face = 0; face < before.indices.size(); face += 3)
    {
        auto a = before.indices[face], b = before.indices[face + 1], c = before.indices[face + 2];
        auto oldNormal = cross(difference(before.positions[b], before.positions[a]),
                               difference(before.positions[c], before.positions[a]));
        auto newNormal = cross(difference(after.positions[b], after.positions[a]),
                               difference(after.positions[c], after.positions[a]));
        minimum = std::min(minimum, dot(oldNormal, newNormal) / dot(oldNormal, oldNormal));
    }
    return minimum;
}
bool matchedLayerContacts(const MeshResult &mesh)
{
    using Position = std::tuple<long long, long long, long long>;
    using Face = std::array<Position, 3>;
    std::map<int, std::multiset<Face>> tops, bottoms;
    for (std::size_t face = 0; face < mesh.layerContactFaces.size(); ++face)
    {
        auto role = mesh.layerContactFaces[face];
        if (role != 1 && role != 2)
            continue;
        auto layer = mesh.layerIndices[mesh.mesh.indices[3 * face]];
        Face vertices;
        for (int corner = 0; corner < 3; ++corner)
        {
            auto p = mesh.mesh.positions[mesh.mesh.indices[3 * face + corner]];
            vertices[corner] = {std::llround(p.x * 1e8), std::llround(p.y * 1e8),
                                std::llround(p.z * 1e8)};
        }
        std::sort(vertices.begin(), vertices.end());
        (role == 1 ? tops[layer] : bottoms[layer]).insert(vertices);
    }
    if (tops.empty())
        return false;
    const auto layerCount = *std::max_element(mesh.layerIndices.begin(), mesh.layerIndices.end()) + 1;
    for (const auto &[layer, faces] : tops)
    {
        if (layer + 1 >= layerCount)
            continue;
        if (!bottoms.contains(layer + 1) || faces != bottoms.at(layer + 1))
            return false;
    }
    return true;
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
        Recipe exaggerated = plain;
        exaggerated.layers = {defaultLayer(Kind::NormalNoise)};
        exaggerated.layers[0].amplitude = 5;
        exaggerated.layers[0].frequency = 100;
        auto extreme = evaluate(exaggerated, 12);
        auto sourceMesh = evaluate(plain, 12).mesh;
        auto edgeStretch = maximumEdgeStretch(sourceMesh, extreme.mesh);
        std::cout << "exaggerated normal noise maximum edge stretch: " << edgeStretch << '\n';
        exaggerated.layers[0].amplitude = 0.18f;
        std::cout << "high frequency normal noise maximum edge stretch: "
                  << maximumEdgeStretch(evaluate(plain, 12).mesh, evaluate(exaggerated, 12).mesh) << '\n';
        exaggerated.layers[0].amplitude = 5;
        exaggerated.layers[0].frequency = 2;
        std::cout << "high amplitude normal noise maximum edge stretch: "
                  << maximumEdgeStretch(evaluate(plain, 12).mesh, evaluate(exaggerated, 12).mesh) << '\n';
        std::cout << "default normal noise maximum edge stretch: "
                  << maximumEdgeStretch(evaluate(plain, 12).mesh, evaluate(recipe, 12).mesh) << '\n';
        auto connected = preset(11);
        connected.layers.back().enabled = false;
        auto layeredSource = evaluate(connected, 12);
        require(matchedLayerContacts(layeredSource), "coherent layer baseline contacts meet");
        connected.layers.push_back(defaultLayer(Kind::NormalNoise));
        auto joined = evaluate(connected, 12);
        std::cout << "moderate contact match=" << matchedLayerContacts(joined)
                  << " minimum orientation=" << minimumFaceOrientation(layeredSource.mesh, joined.mesh)
                  << " closed=" << PA::validateClosedMesh(joined.mesh).hasValue() << '\n';
        require(matchedLayerContacts(joined) &&
                    minimumFaceOrientation(layeredSource.mesh, joined.mesh) > 0 &&
                    PA::validateClosedMesh(joined.mesh).hasValue(),
                "normal displacement keeps touching strata joined without folding faces");
        for (auto kind : {Kind::NormalNoise, Kind::VectorNoise, Kind::Voronoi,
                          Kind::Fracture, Kind::RidgedNoise, Kind::StrataRelief,
                          Kind::SharedWarp})
        {
            Recipe stress = plain;
            stress.layers = {defaultLayer(kind)};
            stress.layers[0].amplitude = 5;
            stress.layers[0].frequency = 100;
            auto result = evaluate(stress, 12);
            require(maximumEdgeStretch(sourceMesh, result.mesh) <= 1.500001,
                    "extreme field displacement cannot create needle edges");
            require(maximumEdgeStretch(result.mesh, sourceMesh) <= 2.000001,
                    "extreme field displacement cannot collapse source edges");
            require(minimumFaceOrientation(sourceMesh, result.mesh) > 0.19,
                    "extreme field displacement cannot flip a source face");
            require(PA::validateClosedMesh(result.mesh).hasValue(),
                    "extreme field displacement remains a closed mesh");
            require(meshHash(result.mesh) ==
                        meshHash(evaluate(deserialize(serialize(stress)), 12).mesh),
                    "guarded extreme deformation must replay exactly");
        }
        connected.layers.back().amplitude = 5;
        connected.layers.back().frequency = 100;
        auto stacked = evaluate(connected, 12);
        require(matchedLayerContacts(stacked),
                "extreme field displacement cannot separate touching strata");
        require(maximumEdgeStretch(layeredSource.mesh, stacked.mesh) <= 1.500001 &&
                    minimumFaceOrientation(layeredSource.mesh, stacked.mesh) > 0.19 &&
                    PA::validateClosedMesh(stacked.mesh).hasValue(),
                "extreme layered displacement preserves edges, faces and closure");
        Recipe heavyRelax = exaggerated;
        heavyRelax.layers[0].frequency = 100;
        auto relaxationLayer = defaultLayer(Kind::Relax);
        relaxationLayer.relaxStrength = 0.5f;
        relaxationLayer.iterations = 3;
        heavyRelax.layers.push_back(relaxationLayer);
        auto relaxedThree = evaluate(heavyRelax, 12, -1, 1);
        heavyRelax.layers.back().iterations = 2;
        auto relaxedTwo = evaluate(heavyRelax, 12);
        for (std::size_t i = 0; i < relaxedThree.mesh.positions.size(); ++i)
        {
            auto a = relaxedThree.mesh.positions[i], b = relaxedTwo.mesh.positions[i];
            auto distance = std::hypot(a.x - b.x, a.y - b.y, a.z - b.z);
            require(std::abs(relaxedThree.field[i] - distance) < 1e-6,
                    "extreme relaxation field records the actual guarded movement");
        }
        require(edgeStretch <= 1.5, "exaggerated deformation must not grow needle edges");
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
        auto bedding = preset(11);
        for (auto &layer : bedding.layers)
        {
            layer.seed = 123;
            if (layer.kind == Kind::Layers)
            {
                layer.perimeterAmplitude = 0;
                layer.beddingTilt = 0;
                layer.macroAxisAsymmetry = 0;
                layer.shapeOuterCaps = false;
            }
        }
        auto harmonic = evaluate(bedding, 16);
        for (auto &layer : bedding.layers)
            if (layer.kind == Kind::Layers)
                layer.lacunarity = 1.87f;
        auto nonharmonic = evaluate(bedding, 16);
        require(meshHash(harmonic.mesh) != meshHash(nonharmonic.mesh),
                "coherent bedding thickness responds to authored octave lacunarity");
        require(meshHash(nonharmonic.mesh) ==
                    meshHash(evaluate(deserialize(serialize(bedding)), 16).mesh),
                "nonharmonic bedding is deterministic after recipe roundtrip");
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
