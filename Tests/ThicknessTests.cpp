#include "Model.hpp"
#include "LayerFormation.hpp"
#include "MeshReduction.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
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
int main()
{
    try
    {
        Recipe recipe;
        auto layer = defaultLayer(Kind::Layers);
        layer.coherentLayers = true;
        layer.count = 5;
        layer.thickness = 0.32f;
        layer.minimumThickness = 0.2f;
        layer.maximumThickness = 0.44f;
        layer.jitter = 0.7f;
        layer.lateral = 0;
        layer.layerScale = 0;
        recipe.layers = {layer};
        auto result = evaluate(recipe, 12, 0, 0);
        require(result.layerThickness.size() == result.mesh.positions.size(),
                "thickness field must reach all output vertices");
        auto [lo, hi] = std::minmax_element(result.layerThickness.begin(), result.layerThickness.end());
        require(*lo >= layer.minimumThickness - 1e-6f && *hi <= layer.maximumThickness + 1e-6f,
                "thickness bounds");
        require(*hi - *lo > 0.04f, "thickness must vary spatially");
        require(result.layerIndices.size() == result.mesh.positions.size(), "stratum identity retained");
        for (int k = 0; k < 5; ++k)
            require(std::find(result.layerIndices.begin(), result.layerIndices.end(), k) !=
                        result.layerIndices.end(),
                    "every stratum represented");
        recipe.layers[0].jitter = 0;
        auto uniform = evaluate(recipe, 12, 0, 0);
        for (auto t : uniform.layerThickness)
            require(std::abs(t - layer.thickness) < 1e-6f, "zero variation gives base thickness");
        require(meshHash(result.mesh) != meshHash(uniform.mesh), "variation changes real geometry");
        recipe.layers[0].jitter = layer.jitter;
        auto repeated = evaluate(deserialize(serialize(recipe)), 12, 0, 0);
        require(meshHash(result.mesh) == meshHash(repeated.mesh), "thickness recipe reproducible");
        Recipe unsupported;
        auto separated = defaultLayer(Kind::Layers);
        separated.coherentLayers = true;
        separated.count = 2;
        separated.thickness = 0.3f;
        separated.spacing = 0.08f;
        separated.jitter = separated.lateral = separated.layerScale = 0;
        unsupported.layers = {separated};
        auto raw = evaluate(unsupported, 8);
        double lowerTop = -1e30, upperBottom = 1e30;
        for (std::size_t i = 0; i < raw.mesh.positions.size(); ++i)
        {
            if (raw.layerIndices[i] == 0)
                lowerTop = std::max(lowerTop, raw.mesh.positions[i].y);
            else if (raw.layerIndices[i] == 1)
                upperBottom = std::min(upperBottom, raw.mesh.positions[i].y);
        }
        require(upperBottom - lowerTop > 0.079,
                "diagnostic fixture must reproduce the authored floating gap");
        auto edgeOnly = raw;
        for (std::size_t i = 0; i < edgeOnly.mesh.positions.size(); ++i)
            if (edgeOnly.layerIndices[i] == 1)
                edgeOnly.mesh.positions[i].x += 2;
        bool rejectedEdgeSupport = false;
        try { settleLayerStack(edgeOnly, {0, 1, 0}, nullptr); }
        catch (const std::runtime_error &) { rejectedEdgeSupport = true; }
        require(rejectedEdgeSupport, "edge-only contact is not a supporting footprint");
        unsupported.layers.push_back(defaultLayer(Kind::LayerSettle));
        auto settled = evaluate(unsupported, 8);
        lowerTop = -1e30;
        upperBottom = 1e30;
        for (std::size_t i = 0; i < settled.mesh.positions.size(); ++i)
        {
            if (settled.layerIndices[i] == 0)
                lowerTop = std::max(lowerTop, settled.mesh.positions[i].y);
            else if (settled.layerIndices[i] == 1)
                upperBottom = std::min(upperBottom, settled.mesh.positions[i].y);
        }
        require(std::abs(upperBottom - lowerTop) <= 1e-6,
                "settling must remove the gap and stop at first support");
        require(settled.mesh.indices == raw.mesh.indices &&
                    settled.mesh.triangleRegions == raw.mesh.triangleRegions,
                "settling preserves topology and semantic regions");
        require(meshHash(settled.mesh) == meshHash(evaluate(deserialize(serialize(unsupported)), 8).mesh),
                "settled rock repeats after recipe serialization");
        auto production = preset(11);
        auto rejectsInvalidMacro = [](const Recipe &candidate)
        {
            try
            {
                validate(candidate);
            }
            catch (const std::exception &)
            {
                return true;
            }
            return false;
        };
        auto excessiveMacro = production;
        excessiveMacro.layers[0].macroAxisAsymmetry = 0.5f;
        require(rejectsInvalidMacro(excessiveMacro), "macro axis asymmetry must respect its deformation limit");
        Recipe unsupportedMacro;
        auto ordinaryWarp = defaultLayer(Kind::Warp);
        ordinaryWarp.macroAxisAsymmetry = 0.1f;
        unsupportedMacro.layers.push_back(ordinaryWarp);
        require(rejectsInvalidMacro(unsupportedMacro), "macro axis asymmetry requires coherent layers");
        auto macroOnly = production;
        macroOnly.layers[0].jitter = 0;
        macroOnly.layers[0].perimeterAmplitude = 0;
        macroOnly.layers[0].shapeOuterCaps = false;
        macroOnly.layers[0].beddingTilt = 0;
        macroOnly.layers.back().enabled = false;
        Recipe macroSourceRecipe;
        macroSourceRecipe.base = production.base;
        auto macroSource = evaluate(macroSourceRecipe, production.finalResolution);
        auto macroStack = evaluate(macroOnly, production.finalResolution);
        double leastMacroRadius = 1e30, greatestMacroRadius = 0;
        int macroRimVertices = 0;
        for (std::size_t i = 0; i < macroSource.mesh.positions.size(); ++i)
        {
            const auto sourcePoint = macroSource.mesh.positions[i];
            if (sourcePoint.y < 0.99 || std::hypot(sourcePoint.x, sourcePoint.z) < 0.99)
                continue;
            const auto mapped = macroStack.mesh.positions[i];
            const auto radius = std::hypot(mapped.x, mapped.z);
            leastMacroRadius = std::min(leastMacroRadius, radius);
            greatestMacroRadius = std::max(greatestMacroRadius, radius);
            ++macroRimVertices;
        }
        require(macroRimVertices > 24 && greatestMacroRadius - leastMacroRadius > 0.20,
                "layered macroform must break the cylinder beyond local perimeter noise");
        for (float amplitude : {0.04f, 0.08f, 0.12f, 0.16f, 0.2f, 0.28f})
        {
            auto trial = production;
            trial.layers[0].perimeterAmplitude = amplitude;
            trial.layers.back().enabled = false;
            auto trialRaw = evaluate(trial, trial.finalResolution);
            auto trialShifts = settleLayerStack(trialRaw, {0, 1, 0}, nullptr);
            for (double shift : trialShifts)
                require(std::abs(shift) < 1e-6,
                        "angular perimeter must preserve cap contact across amplitudes");
        }
        auto unsettledRecipe = production;
        unsettledRecipe.layers.back().enabled = false;
        auto productionRaw = evaluate(unsettledRecipe, production.finalResolution);
        auto productionSettled = evaluate(production, production.finalResolution);
        auto horizonVariation = [&](const MeshResult &stack)
        {
            std::vector<std::vector<double>> radii(production.layers[0].count);
            for (std::size_t i = 0; i < macroSource.mesh.positions.size(); ++i)
            {
                const auto sourcePoint = macroSource.mesh.positions[i];
                if (sourcePoint.y < 0.99 || std::hypot(sourcePoint.x, sourcePoint.z) < 0.99)
                    continue;
                for (int k = 0; k < production.layers[0].count; ++k)
                {
                    const auto p = stack.mesh.positions[k * macroSource.mesh.positions.size() + i];
                    radii[k].push_back(std::hypot(p.x, p.z));
                }
            }
            double total = 0;
            for (int k = 1; k < production.layers[0].count; ++k)
            {
                const auto &a = radii[k - 1], &b = radii[k];
                double ma = 0, mb = 0, ab = 0, aa = 0, bb = 0;
                for (std::size_t i = 0; i < a.size(); ++i)
                {
                    ma += a[i];
                    mb += b[i];
                }
                ma /= a.size();
                mb /= b.size();
                for (std::size_t i = 0; i < a.size(); ++i)
                {
                    const auto x = a[i] - ma, y = b[i] - mb;
                    ab += x * y;
                    aa += x * x;
                    bb += y * y;
                }
                total += ab / std::sqrt(aa * bb);
            }
            return total / (production.layers[0].count - 1);
        };
        auto fracturedRecipe = production;
        fracturedRecipe.version = 15;
        fracturedRecipe.layers[0].profile = 2;
        auto legacyFractured = fracturedRecipe;
        legacyFractured.version = 14;
        require(rejectsInvalidMacro(legacyFractured),
                "fractured horizon profile must not change legacy recipe output");
        auto fracturedStack = evaluate(fracturedRecipe, fracturedRecipe.finalResolution);
        const auto baselineContourCorrelation = horizonVariation(productionSettled);
        const auto fracturedContourCorrelation = horizonVariation(fracturedStack);
        require(fracturedContourCorrelation + 0.08 < baselineContourCorrelation,
                "independent horizon fracture must reduce adjacent contour repetition");
        std::cout << "horizon correlation baseline=" << baselineContourCorrelation
                  << " fractured=" << fracturedContourCorrelation << '\n';
        require(PA::validateClosedMesh(fracturedStack.mesh).hasValue(),
                "independent horizon fracture must retain a closed mesh");
        require(meshHash(fracturedStack.mesh) ==
                    meshHash(evaluate(deserialize(serialize(fracturedRecipe)), fracturedRecipe.finalResolution).mesh),
                "independent horizon fracture must replay through version 15 serialization");
        auto replayedProduction = deserialize(serialize(production));
        require(replayedProduction.layers[0].beddingTilt == production.layers[0].beddingTilt &&
                    replayedProduction.layers[0].macroAxisAsymmetry ==
                        production.layers[0].macroAxisAsymmetry &&
                    meshHash(evaluate(replayedProduction, production.finalResolution).mesh) ==
                        meshHash(productionSettled.mesh),
                "inclined bedding survives recipe serialization and exact regeneration");
        using ContactVertex = std::tuple<long long, long long, long long>;
        using ContactTriangle = std::array<ContactVertex, 3>;
        auto contactTriangles = [&](const MeshResult &stack, int stratum, unsigned char face)
        {
            std::multiset<ContactTriangle> triangles;
            require(stack.layerContactFaces.size() == stack.mesh.indices.size() / 3,
                    "production contact faces must retain source roles");
            for (std::size_t t = 0; t < stack.layerContactFaces.size(); ++t)
            {
                if (stack.layerContactFaces[t] != face ||
                    stack.layerIndices[stack.mesh.indices[3 * t]] != stratum)
                    continue;
                ContactTriangle triangle;
                for (int corner = 0; corner < 3; ++corner)
                {
                    auto p = stack.mesh.positions[stack.mesh.indices[3 * t + corner]];
                    triangle[corner] = {std::llround(p.x * 1e8), std::llround(p.y * 1e8),
                                        std::llround(p.z * 1e8)};
                }
                std::sort(triangle.begin(), triangle.end());
                triangles.insert(triangle);
            }
            return triangles;
        };
        for (int k = 0; k + 1 < fracturedRecipe.layers[0].count; ++k)
            require(contactTriangles(fracturedStack, k, 1) ==
                        contactTriangles(fracturedStack, k + 1, 2),
                    "fractured horizons must remain complete shared contacts");
        auto inclinedBedding = production;
        inclinedBedding.layers[0].jitter = 0;
        inclinedBedding.layers[0].perimeterAmplitude = 0;
        inclinedBedding.layers[0].shapeOuterCaps = false;
        auto inclinedStack = evaluate(inclinedBedding, inclinedBedding.finalResolution);
        double lowestContact = 1e30, highestContact = -1e30;
        for (std::size_t t = 0; t < inclinedStack.layerContactFaces.size(); ++t)
            if (inclinedStack.layerContactFaces[t] == 1 &&
                inclinedStack.layerIndices[inclinedStack.mesh.indices[3 * t]] == 0)
                for (int corner = 0; corner < 3; ++corner)
                {
                    const auto y = inclinedStack.mesh.positions[inclinedStack.mesh.indices[3 * t + corner]].y;
                    lowestContact = std::min(lowestContact, y);
                    highestContact = std::max(highestContact, y);
                }
        require(highestContact - lowestContact > 0.15,
                "layered preset needs a visible oblique shared horizon without thickness noise");
        for (int k = 0; k + 1 < inclinedBedding.layers[0].count; ++k)
            require(contactTriangles(inclinedStack, k, 1) ==
                        contactTriangles(inclinedStack, k + 1, 2),
                    "inclined bedding must retain complete contact without floating layers");
        Recipe wideBedding;
        wideBedding.base = Base::Cylinder;
        auto widen = defaultLayer(Kind::Transform);
        widen.scale = {5, 1, 5};
        auto widenAgain = defaultLayer(Kind::Transform);
        widenAgain.scale = {4, 1, 4};
        auto wideStrata = defaultLayer(Kind::Layers);
        wideStrata.coherentLayers = true;
        wideStrata.count = 2;
        wideStrata.thickness = 0.3f;
        wideStrata.spacing = wideStrata.jitter = wideStrata.lateral = wideStrata.layerScale = 0;
        wideStrata.beddingTilt = 0.25f;
        wideStrata.macroAxisAsymmetry = 0.42f;
        wideBedding.layers = {widen, widenAgain, wideStrata};
        auto wideSource = evaluate(wideBedding, 12, 1);
        auto wideStack = evaluate(wideBedding, 12);
        int matchedPairs = 0;
        for (std::size_t i = 0; i < wideSource.mesh.positions.size(); ++i)
        {
            const auto upper = wideSource.mesh.positions[i];
            if (std::abs(upper.y - 1) > 1e-9)
                continue;
            for (std::size_t j = 0; j < wideSource.mesh.positions.size(); ++j)
            {
                const auto lower = wideSource.mesh.positions[j];
                if (std::abs(lower.y + 1) > 1e-9 ||
                    std::hypot(upper.x - lower.x, upper.z - lower.z) > 1e-9)
                    continue;
                require(wideStack.mesh.positions[i].y - wideStack.mesh.positions[j].y > 0.02,
                        "bedding tilt must not invert a wide stratum side wall");
                ++matchedPairs;
                break;
            }
        }
        require(matchedPairs > 8, "wide bedding regression must inspect source side walls");
        auto drifting = production;
        drifting.layers[0].lateral = 0.08f;
        drifting.layers.back().enabled = false;
        auto driftingStack = evaluate(drifting, drifting.finalResolution);
        for (int k = 0; k + 1 < drifting.layers[0].count; ++k)
            require(contactTriangles(driftingStack, k, 1) ==
                        contactTriangles(driftingStack, k + 1, 2),
                    "drifting coherent strata must share the full horizon without floating gaps");
        for (auto shift : settleLayerStack(driftingStack, {0, 1, 0}, nullptr))
            require(std::abs(shift) < 1e-6,
                    "drifting coherent strata cannot depend on first-point settling");
        auto tilted = drifting;
        tilted.layers[0].rotationDrift = 4.0f;
        tilted.layers[0].layerScale = 0.02f;
        tilted.layers[0].scaleDrift = 0.03f;
        auto tiltedStack = evaluate(tilted, tilted.finalResolution);
        require(PA::validateClosedMesh(tiltedStack.mesh).hasValue(),
                "coherent horizon interpolation must retain closed transformed strata");
        for (int k = 0; k + 1 < tilted.layers[0].count; ++k)
            require(contactTriangles(tiltedStack, k, 1) ==
                        contactTriangles(tiltedStack, k + 1, 2),
                    "combined drift, rotation and scale must preserve complete horizon contact");
        auto sedimentary = preset(5);
        auto localStackRecipe = sedimentary;
        localStackRecipe.layers[0].perimeterAmplitude = 0;
        localStackRecipe.layers[0].shapeOuterCaps = false;
        localStackRecipe.layers[0].jitter = 0;
        localStackRecipe.layers[0].layerScale = 0.05f;
        localStackRecipe.layers.back().enabled = false;
        auto localStack = evaluate(localStackRecipe, 16);
        auto translatedRecipe = localStackRecipe;
        auto translate = defaultLayer(Kind::Transform);
        translate.translation = {2, 3, -4};
        translatedRecipe.layers.insert(translatedRecipe.layers.begin(), translate);
        auto translatedStack = evaluate(translatedRecipe, 16);
        require(localStack.mesh.positions.size() == translatedStack.mesh.positions.size(),
                "translated layered source must preserve its topology");
        for (std::size_t i = 0; i < localStack.mesh.positions.size(); ++i)
        {
            auto a = localStack.mesh.positions[i], b = translatedStack.mesh.positions[i];
            require(std::abs(b.x - a.x - 2) < 1e-6 && std::abs(b.y - a.y - 3) < 1e-6 &&
                        std::abs(b.z - a.z + 4) < 1e-6,
                    "coherent layers must be invariant under a source translation");
        }
        auto sedimentaryStack = evaluate(sedimentary, sedimentary.finalResolution);
        auto movedSedimentary = sedimentary;
        movedSedimentary.layers.insert(movedSedimentary.layers.begin(), translate);
        auto movedStack = evaluate(movedSedimentary, movedSedimentary.finalResolution);
        require(movedStack.mesh.positions.size() == sedimentaryStack.mesh.positions.size(),
                "translated sedimentary stack must preserve its topology");
        for (std::size_t i = 0; i < sedimentaryStack.mesh.positions.size(); ++i)
        {
            auto a = sedimentaryStack.mesh.positions[i], b = movedStack.mesh.positions[i];
            require(std::abs(b.x - a.x - 2) < 1e-6 && std::abs(b.y - a.y - 3) < 1e-6 &&
                        std::abs(b.z - a.z + 4) < 1e-6,
                    "contour, thickness noise and settling must follow a translated source");
        }
        require(PA::validateClosedMesh(sedimentaryStack.mesh).hasValue(),
                "sedimentary preset must remain a closed solid");
        for (int k = 0; k + 1 < sedimentary.layers[0].count; ++k)
            require(contactTriangles(sedimentaryStack, k, 1) ==
                        contactTriangles(sedimentaryStack, k + 1, 2),
                    "sedimentary preset cannot leave air between adjacent slabs");
        for (auto shift : settleLayerStack(sedimentaryStack, {0, 1, 0}, nullptr))
            require(std::abs(shift) < 1e-6,
                    "sedimentary preset must already rest on every adjacent horizon");
        auto sedimentaryLow = evaluate(sedimentary, 16);
        require(sedimentaryLow.mesh.indices.size() < sedimentaryStack.mesh.indices.size() &&
                    sampledBidirectionalMeshError(sedimentaryStack.mesh, sedimentaryLow.mesh) < 0.08,
                "lower-resolution sedimentary replay must stay inside the surface error budget");
        for (auto seed : {7U, 17U, 911U})
        {
            auto seeded = sedimentary;
            for (auto &operation : seeded.layers)
                operation.seed = seed;
            auto stack = evaluate(seeded, seeded.finalResolution);
            require(PA::validateClosedMesh(stack.mesh).hasValue(),
                    "seeded sedimentary stack must remain closed");
            for (int k = 0; k + 1 < seeded.layers[0].count; ++k)
                require(contactTriangles(stack, k, 1) == contactTriangles(stack, k + 1, 2),
                        "seeded sedimentary horizons must coincide over every triangle");
            for (auto shift : settleLayerStack(stack, {0, 1, 0}, nullptr))
                require(std::abs(shift) < 1e-6,
                        "seeded sedimentary stack must remain supported");
            require(meshHash(stack.mesh) ==
                        meshHash(evaluate(deserialize(serialize(seeded)), seeded.finalResolution).mesh),
                    "seeded sedimentary recipe must replay exactly");
        }
        for (unsigned seed = 0; seed < 128; ++seed)
        {
            auto seeded = sedimentary;
            seeded.layers[0].seed = seed;
            seeded.finalResolution = 16;
            MeshResult stack;
            try { stack = evaluate(seeded, seeded.finalResolution); }
            catch (const std::exception &error)
            {
                throw std::runtime_error("sedimentary seed " + std::to_string(seed) + ": " + error.what());
            }
            require(PA::validateClosedMesh(stack.mesh).hasValue(),
                    "sedimentary seed sweep must retain closed strata");
            for (int k = 0; k + 1 < seeded.layers[0].count; ++k)
                require(contactTriangles(stack, k, 1) == contactTriangles(stack, k + 1, 2),
                        "sedimentary seed sweep must retain full paired contact");
            for (auto shift : settleLayerStack(stack, {0, 1, 0}, nullptr))
                require(std::abs(shift) < 1e-6,
                        "sedimentary seed sweep cannot require an extra gravity correction");
            if (seed < 32)
            {
                seeded.finalResolution = sedimentary.finalResolution;
                try { stack = evaluate(seeded, seeded.finalResolution); }
                catch (const std::exception &error)
                {
                    throw std::runtime_error("sedimentary high seed " + std::to_string(seed) +
                                             ": " + error.what());
                }
                require(PA::validateClosedMesh(stack.mesh).hasValue(),
                        "sedimentary high seed sweep must retain closed strata");
                for (int k = 0; k + 1 < seeded.layers[0].count; ++k)
                    require(contactTriangles(stack, k, 1) == contactTriangles(stack, k + 1, 2),
                            "sedimentary high seed sweep must retain full paired contact");
                for (auto shift : settleLayerStack(stack, {0, 1, 0}, nullptr))
                    require(std::abs(shift) < 1e-6,
                            "sedimentary high seed sweep cannot require an extra gravity correction");
            }
        }
        for (int k = 0; k + 1 < production.layers[0].count; ++k)
            require(contactTriangles(productionSettled, k, 1) ==
                        contactTriangles(productionSettled, k + 1, 2),
                    "every paired horizon triangle must coincide, not only its boundary vertices");
        for (auto seed : {17U, 911U})
        {
            auto seededRecipe = production;
            for (auto &operation : seededRecipe.layers)
                operation.seed = seed;
            auto seededStack = evaluate(seededRecipe, seededRecipe.finalResolution);
            for (int k = 0; k + 1 < seededRecipe.layers[0].count; ++k)
                require(contactTriangles(seededStack, k, 1) ==
                            contactTriangles(seededStack, k + 1, 2),
                        "every captured seed must retain complete triangular contact");
            for (auto shift : settleLayerStack(seededStack, {0, 1, 0}, nullptr))
                require(std::abs(shift) < 1e-6, "captured seeds remain settled at first contact");
        }
        for (unsigned seed = 0; seed < 64; ++seed)
        {
            auto seeded = production;
            seeded.layers[0].seed = seed;
            seeded.finalResolution = 16;
            MeshResult stack;
            try { stack = evaluate(seeded, seeded.finalResolution); }
            catch (const std::exception &error)
            {
                throw std::runtime_error("layered seed " + std::to_string(seed) + ": " + error.what());
            }
            require(PA::validateClosedMesh(stack.mesh).hasValue(),
                    "layered seed sweep must retain a closed solid");
            for (int k = 0; k + 1 < seeded.layers[0].count; ++k)
                require(contactTriangles(stack, k, 1) == contactTriangles(stack, k + 1, 2),
                        "layered seed sweep must retain full paired contact");
            for (auto shift : settleLayerStack(stack, {0, 1, 0}, nullptr))
                require(std::abs(shift) < 1e-6,
                        "layered seed sweep cannot require an extra gravity correction");
            if (seed < 64)
            {
                seeded.finalResolution = production.finalResolution;
                try { stack = evaluate(seeded, seeded.finalResolution); }
                catch (const std::exception &error)
                {
                    throw std::runtime_error("layered high seed " + std::to_string(seed) +
                                             ": " + error.what());
                }
                require(PA::validateClosedMesh(stack.mesh).hasValue(),
                        "layered high seed sweep must retain a closed solid");
                for (int k = 0; k + 1 < seeded.layers[0].count; ++k)
                    require(contactTriangles(stack, k, 1) == contactTriangles(stack, k + 1, 2),
                            "layered high seed sweep must retain full paired contact");
                for (auto shift : settleLayerStack(stack, {0, 1, 0}, nullptr))
                    require(std::abs(shift) < 1e-6,
                            "layered high seed sweep cannot require an extra gravity correction");
            }
        }
        std::vector<bool> sideVertex(productionSettled.mesh.positions.size());
        for (std::size_t t = 0; t < productionSettled.layerContactFaces.size(); ++t)
            if (productionSettled.layerContactFaces[t] == 0)
                for (int corner = 0; corner < 3; ++corner)
                    sideVertex[productionSettled.mesh.indices[3 * t + corner]] = true;
        using ContactKey = std::tuple<int, ContactVertex>;
        std::map<ContactKey, std::uint32_t> topBoundary, bottomBoundary;
        for (std::size_t t = 0; t < productionSettled.layerContactFaces.size(); ++t)
        {
            const auto face = productionSettled.layerContactFaces[t];
            if (face != 1 && face != 2)
                continue;
            for (int corner = 0; corner < 3; ++corner)
            {
                const auto i = productionSettled.mesh.indices[3 * t + corner];
                if (!sideVertex[i])
                    continue;
                const auto p = productionSettled.mesh.positions[i];
                const ContactVertex position{std::llround(p.x * 1e8), std::llround(p.y * 1e8),
                                             std::llround(p.z * 1e8)};
                const ContactKey key{productionSettled.layerIndices[i], position};
                (face == 1 ? topBoundary : bottomBoundary)[key] = i;
            }
        }
        std::size_t matchedSeamVertices = 0;
        for (const auto &[key, lowerIndex] : topBoundary)
        {
            auto [layer, position] = key;
            auto upper = bottomBoundary.find({layer + 1, position});
            if (upper == bottomBoundary.end())
                continue;
            const auto a = productionSettled.normals[lowerIndex];
            const auto b = productionSettled.normals[upper->second];
            require(a.x * b.x + a.y * b.y + a.z * b.z > 0.999999,
                    "touching exterior sides must use continuous seam normals");
            ++matchedSeamVertices;
        }
        require(matchedSeamVertices > 100, "production rock must expose matched side seam vertices");
        auto manuallySettled = productionRaw;
        auto shifts = settleLayerStack(manuallySettled, {0, 1, 0}, nullptr);
        require(meshHash(manuallySettled.mesh) == meshHash(productionSettled.mesh) &&
                    productionSettled.mesh.triangleRegions == productionRaw.mesh.triangleRegions,
                "the actual layered preset settles after all wear stages and preserves semantics");
        for (double shift : shifts)
            require(std::abs(shift) < 1e-6,
                    "matched geological horizons need no artificial separation before settling");
        auto repeatShifts = settleLayerStack(productionSettled, {0, 1, 0}, nullptr);
        for (double shift : repeatShifts)
            require(std::abs(shift) < 1e-6, "settled production strata stay at first contact");
        Recipe profile;
        profile.base = Base::Cylinder;
        auto profileLayer = defaultLayer(Kind::Layers);
        profileLayer.coherentLayers = true;
        profileLayer.count = 3;
        profileLayer.thickness = 0.3f;
        profileLayer.spacing = profileLayer.jitter = profileLayer.lateral = profileLayer.layerScale = 0;
        profileLayer.perimeterAmplitude = 0;
        profile.layers = {profileLayer};
        auto plainProfile = evaluate(profile, 12);
        profile.layers[0].perimeterAmplitude = 0.12f;
        auto shapedProfile = evaluate(profile, 12);
        Recipe sourceProfile;
        sourceProfile.base = Base::Cylinder;
        auto sourceCylinder = evaluate(sourceProfile, 12);
        const auto sourceCount = sourceCylinder.mesh.positions.size();
        bool changedSide = false, independentStrata = false, changedInterface = false;
        std::size_t sharedCapPairs = 0;
        for (std::size_t i = 0; i < sourceCount; ++i)
        {
            auto h = sourceCylinder.mesh.positions[i].y;
            if (std::abs(h + 1) < 1e-9 || std::abs(h - 1) < 1e-9)
            {
                int k = h < 0 ? 0 : 2;
                auto a = plainProfile.mesh.positions[sourceCount * k + i];
                auto b = shapedProfile.mesh.positions[sourceCount * k + i];
                require(std::hypot(a.x - b.x, a.y - b.y, a.z - b.z) < 1e-9,
                        "outer base and top contours remain anchored");
                if (h > 0)
                    for (std::size_t j = 0; j < sourceCount; ++j)
                    {
                        auto bottom = sourceCylinder.mesh.positions[j];
                        if (std::abs(bottom.y + 1) > 1e-9 ||
                            std::hypot(bottom.x - sourceCylinder.mesh.positions[i].x,
                                       bottom.z - sourceCylinder.mesh.positions[i].z) > 1e-9)
                            continue;
                        for (int k = 0; k < 2; ++k)
                        {
                            auto upper = shapedProfile.mesh.positions[sourceCount * k + i];
                            auto lower = shapedProfile.mesh.positions[sourceCount * (k + 1) + j];
                            require(std::hypot(upper.x - lower.x, upper.y - lower.y,
                                               upper.z - lower.z) < 1e-8,
                                    "neighboring strata share their complete interface contour");
                            auto plain = plainProfile.mesh.positions[sourceCount * k + i];
                            changedInterface |= std::hypot(upper.x - plain.x, upper.z - plain.z) > 1e-4;
                            ++sharedCapPairs;
                        }
                        break;
                    }
            }
            else
            {
                auto a = plainProfile.mesh.positions[i], b = shapedProfile.mesh.positions[i];
                auto c = plainProfile.mesh.positions[sourceCount + i];
                auto d = shapedProfile.mesh.positions[sourceCount + i];
                auto first = std::hypot(a.x - b.x, a.z - b.z);
                auto second = std::hypot(c.x - d.x, c.z - d.z);
                changedSide |= first > 1e-4;
                independentStrata |= std::abs(first - second) > 1e-4;
            }
        }
        require(changedSide && independentStrata && changedInterface && sharedCapPairs > 12 &&
                    PA::validateClosedMesh(shapedProfile.mesh).hasValue(),
                "perimeter changes each stratum while sharing nontrivial contact contours");
        require(meshHash(shapedProfile.mesh) == meshHash(evaluate(deserialize(serialize(profile)), 12).mesh),
                "perimeter recipe repeats after serialization");
        profile.layers[0].shapeOuterCaps = true;
        auto externalProfile = evaluate(profile, 12);
        std::size_t changedOuterVertices = 0;
        for (std::size_t i = 0; i < sourceCount; ++i)
        {
            auto h = sourceCylinder.mesh.positions[i].y;
            if (std::abs(h + 1) > 1e-9 && std::abs(h - 1) > 1e-9)
                continue;
            int k = h < 0 ? 0 : 2;
            auto a = shapedProfile.mesh.positions[sourceCount * k + i];
            auto b = externalProfile.mesh.positions[sourceCount * k + i];
            changedOuterVertices += std::hypot(a.x - b.x, a.z - b.z) > 1e-4;
        }
        require(changedOuterVertices > 12,
                "opt-in outer contour must break the cylindrical top and base silhouette");
        for (int k = 0; k < 2; ++k)
            require(contactTriangles(externalProfile, k, 1) ==
                        contactTriangles(externalProfile, k + 1, 2),
                    "outer contour must preserve every internal horizon triangle");
        require(PA::validateClosedMesh(externalProfile.mesh).hasValue(),
                "outer contour must preserve closed stratum topology");
        require(meshHash(externalProfile.mesh) ==
                    meshHash(evaluate(deserialize(serialize(profile)), 12).mesh),
                "opt-in outer contour must survive recipe serialization");
        std::cout << "PASS thickness bounds, spatial variation, layer identity, zero variation and recipe "
                     "roundtrip\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
