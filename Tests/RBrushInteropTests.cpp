#include "RBrushInterop.hpp"
#include <Jerico/AssetPipeline/RBrushHash.hpp>
#include <Jerico/ProceduralAuthoring/Geology/GeologyRockfall.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

using namespace Playground;
namespace A = Jerico::Tools::AssetPipeline;
namespace P = Jerico::Tools::ProceduralAuthoring;
namespace
{
void require(bool valid, const char *message)
{
    if (!valid)
        throw std::runtime_error(message);
}
template <typename F> void rejects(F run)
{
    bool rejected = false;
    try { run(); }
    catch (const std::exception &) { rejected = true; }
    require(rejected, "invalid RBrush tip/preset must be rejected");
}
}
int main()
try
{
    static_assert(!std::is_default_constructible_v<RBrushLocalField>);
    A::RBrushTipIR tip;
    tip.tipGuid.bytes[0] = std::byte{1};
    tip.width = tip.height = 5;
    tip.pixels.assign(25, std::byte{0});
    for (int y = 1; y <= 3; ++y)
        for (int x = 1; x <= 3; ++x)
            tip.pixels[y * 5 + x] = std::byte{255};
    A::RBrushPresetIR preset;
    preset.primaryTipGuid = tip.tipGuid;
    preset.sdfDefaults.compatible = true;
    auto sdf = deriveRBrushTipSdf(tip, preset);
    require(sdf.width == 5 && sdf.height == 5 && sdf.insideTexels == 9 &&
                sdf.signedDistanceTexels[12] < 0 && sdf.signedDistanceTexels[0] > 0,
            "canonical RBrush scalar pixels derive an inside-negative engine SDF");
    require(sdf == deriveRBrushTipSdf(tip, preset), "RBrush SDF derivation is deterministic");
    auto wide = tip;
    wide.pixelFormat = A::RBrushPixelFormat::R16Unorm;
    wide.pixels.clear();
    for (auto pixel : tip.pixels)
    {
        auto inside = pixel == std::byte{255};
        wide.pixels.push_back(inside ? std::byte{255} : std::byte{0});
        wide.pixels.push_back(inside ? std::byte{255} : std::byte{0});
    }
    auto wideSdf = deriveRBrushTipSdf(wide, preset);
    require(wideSdf.signedDistanceTexels == sdf.signedDistanceTexels &&
                wideSdf.sourceHash != sdf.sourceHash,
            "R16 canonical tips retain physical SDF while preserving distinct source provenance");
    auto inverted = preset;
    inverted.sdfDefaults.invert = true;
    require(deriveRBrushTipSdf(tip, inverted).insideTexels == 16,
            "canonical RBrush inversion reaches the engine derivation");
    auto invalid = tip;
    invalid.pixelFormat = A::RBrushPixelFormat::RGBA8Unorm;
    rejects([&] { deriveRBrushTipSdf(invalid, preset); });
    invalid = tip;
    invalid.pixels.pop_back();
    rejects([&] { deriveRBrushTipSdf(invalid, preset); });
    invalid = tip;
    invalid.tipGuid.bytes[0] = std::byte{2};
    rejects([&] { deriveRBrushTipSdf(invalid, preset); });
    preset.sdfDefaults.compatible = false;
    rejects([&] { deriveRBrushTipSdf(tip, preset); });
    auto localTip = tip;
    localTip.canonicalTipHash = A::hashRBrushTip(
        localTip.pixelFormat, localTip.width, localTip.height, localTip.pixels);
    auto localPreset = preset;
    localPreset.presetGuid.bytes[0] = std::byte{3};
    localPreset.sourceInfo.canonicalTipHash = localTip.canonicalTipHash;
    Jerico::Guid assetGuid;
    assetGuid.bytes[0] = std::byte{4};
    auto field = makeRBrushLocalField(localTip, localPreset, assetGuid, {}, 4, 4, 2, 1);
    require(field.source().canonicalTipHash == localTip.canonicalTipHash &&
                field.sdf().insideTexels == 9,
            "local RBrush field retains admitted tip identity and Engine SDF");
    require(sampleRBrushLocalField(field, {0, 0, 0}) == 1 &&
                sampleRBrushLocalField(field, {3, 0, 0}) == 0 &&
                sampleRBrushLocalField(field, {0, 0, 3}) == 0,
            "RBrush stamp must be bounded in plane and depth");
    auto middleDepth = sampleRBrushLocalField(field, {0, 0, 1.5});
    require(middleDepth > 0 && middleDepth < 1 &&
                std::abs(middleDepth - sampleRBrushLocalField(field, {0, 0, -1.5})) < 1e-12,
            "RBrush depth falloff is symmetric and continuous");
    auto edgeWeight = sampleRBrushLocalField(field, {1.75, 0, 0});
    require(edgeWeight > 0 && edgeWeight < 1 &&
                sampleRBrushLocalField(field, {2.01, 0, 0}) == 0,
            "RBrush signed-distance edge falloff stays bounded by the physical footprint");
    require(std::abs(sampleRBrushLocalField(field, {3, 0, 0}, true) - 1) < 1e-12,
            "inverted local field complements the bounded mask");
    auto rotated = P::GeologyTransform{};
    rotated.rotation = {0, 0, std::sqrt(0.5), std::sqrt(0.5)};
    auto rotatedField = makeRBrushLocalField(localTip, localPreset, assetGuid,
                                             rotated, 4, 4, 2, 1);
    require(std::abs(sampleRBrushLocalField(field, {1.5, 0, 0}) -
                     sampleRBrushLocalField(rotatedField, {0, 1.5, 0})) < 1e-12,
            "world-to-local quaternion projection follows the Engine transform contract");
    auto transformed = P::GeologyTransform{};
    transformed.translation = {5, 2, -3};
    transformed.scale = {2, 1, 1};
    auto transformedField = makeRBrushLocalField(localTip, localPreset, assetGuid,
                                                 transformed, 4, 4, 2, 1);
    require(std::abs(sampleRBrushLocalField(field, {1.5, 0, 0}) -
                     sampleRBrushLocalField(transformedField, {8, 2, -3})) < 1e-12,
            "local-field translation and nonuniform scale map world points consistently");
    auto elevated = P::GeologyTransform{};
    elevated.translation = {0, 3, 0};
    auto elevatedField = makeRBrushLocalField(localTip, localPreset, assetGuid,
                                              elevated, 4, 4, 2, 1);
    P::GeologyRockfallReferenceRequest rockfall;
    rockfall.voxelSizeMeters = 1;
    for (int x : {0, 3})
    {
        rockfall.voxels.push_back({.key = {x, 0, 0},
                                   .centerWorld = {double(x), 0, 0},
                                   .signedDistanceMeters = -0.5});
        rockfall.voxels.push_back({
            .key = {x, 3, 0},
            .centerWorld = {double(x), 3, 0},
            .signedDistanceMeters = -0.5,
            .regionalMask = sampleRBrushLocalField(elevatedField, {double(x), 3, 0})});
    }
    auto maskedFall = P::evaluateGeologyRockfallReference(rockfall);
    require(maskedFall.hasValue() &&
                maskedFall.value().diagnostics.detachedVoxelCount == 1 &&
                maskedFall.value().diagnostics.detachedComponentCount == 1,
            "canonical RBrush mask activates only the detached supported-field island it covers");
    auto isDetached = [&](const P::GeologyRockfallReferenceResult &result, int x)
    {
        auto voxel = std::find_if(result.voxels.begin(), result.voxels.end(),
                                  [x](const auto &v) { return v.key.x == x && v.key.y == 3; });
        require(voxel != result.voxels.end(), "rockfall output lost an authored voxel");
        return voxel->signedDistanceMeters > 0;
    };
    require(isDetached(maskedFall.value(), 0) && !isDetached(maskedFall.value(), 3),
            "RBrush field gates the expected island, preserving the unmasked one");
    for (auto &voxel : rockfall.voxels)
        if (voxel.key.y == 3)
            voxel.regionalMask = 1;
    auto unmaskedFall = P::evaluateGeologyRockfallReference(rockfall);
    require(unmaskedFall.hasValue() &&
                unmaskedFall.value().diagnostics.detachedVoxelCount == 2,
            "negative control: the test detects both disconnected islands without the brush mask");
    for (auto &voxel : rockfall.voxels)
        if (voxel.key.y == 3)
            voxel.regionalMask = sampleRBrushLocalField(elevatedField,
                voxel.centerWorld, true);
    auto invertedFall = P::evaluateGeologyRockfallReference(rockfall);
    require(invertedFall.hasValue() &&
                invertedFall.value().diagnostics.detachedVoxelCount == 1 &&
                !isDetached(invertedFall.value(), 0) && isDetached(invertedFall.value(), 3),
            "RBrush inversion swaps which detached island the Engine rockfall reference removes");
    std::cout << "RBrush -> Engine rockfall detached voxels: masked="
              << maskedFall.value().diagnostics.detachedVoxelCount << " unmasked="
              << unmaskedFall.value().diagnostics.detachedVoxelCount << " inverted="
              << invertedFall.value().diagnostics.detachedVoxelCount << '\n';
    auto corruptTip = localTip;
    corruptTip.pixels[12] = std::byte{0};
    rejects([&] { makeRBrushLocalField(corruptTip, localPreset, assetGuid, {}, 4, 4, 2, 1); });
    auto corruptPreset = localPreset;
    corruptPreset.sourceInfo.canonicalTipHash[0] ^= std::byte{1};
    rejects([&] { makeRBrushLocalField(localTip, corruptPreset, assetGuid, {}, 4, 4, 2, 1); });
    rejects([&] { makeRBrushLocalField(localTip, localPreset, assetGuid, {}, 0, 4, 2, 1); });
    rejects([&] { makeRBrushLocalField(localTip, localPreset, assetGuid, {}, 4, 4, 2, -1); });
    rejects([&] { makeRBrushLocalField(wide, localPreset, assetGuid, {}, 4, 4, 2, 1); });
    auto invalidRotation = P::GeologyTransform{};
    invalidRotation.rotation = {0, 0, 0, 2};
    rejects([&] { makeRBrushLocalField(localTip, localPreset, assetGuid,
                                       invalidRotation, 4, 4, 2, 1); });
    rejects([&] { makeRBrushLocalField(localTip, localPreset, assetGuid, {},
                                       4, 4, 2, std::numeric_limits<double>::quiet_NaN()); });
    rejects([&] { sampleRBrushLocalField(field,
                 {std::numeric_limits<double>::quiet_NaN(), 0, 0}); });
    auto largeTip = localTip;
    largeTip.width = largeTip.height = 257;
    largeTip.pixels.assign(257 * 257, std::byte{0});
    for (int y = 0; y < 257; ++y)
        for (int x = 0; x < 257; ++x)
            if ((x - 128) * (x - 128) + (y - 128) * (y - 128) <= 80 * 80)
                largeTip.pixels[y * 257 + x] = std::byte{255};
    largeTip.canonicalTipHash = A::hashRBrushTip(
        largeTip.pixelFormat, largeTip.width, largeTip.height, largeTip.pixels);
    auto largePreset = localPreset;
    largePreset.sourceInfo.canonicalTipHash = largeTip.canonicalTipHash;
    auto start = std::chrono::steady_clock::now();
    auto largeField = makeRBrushLocalField(largeTip, largePreset, assetGuid,
                                           {}, 8, 8, 2, 0.5);
    auto derived = std::chrono::steady_clock::now();
    require(sampleRBrushLocalField(largeField, {0, 0, 0}) == 1 &&
                sampleRBrushLocalField(largeField, {4, 4, 0}) == 0 &&
                sampleRBrushLocalField(largeField, {2.0, 0, 0}) >=
                    sampleRBrushLocalField(largeField, {2.5, 0, 0}) &&
                sampleRBrushLocalField(largeField, {2.5, 0, 0}) >=
                    sampleRBrushLocalField(largeField, {3.0, 0, 0}),
            "large canonical mask keeps circular support and monotone edge falloff");
    double integrated = 0;
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x)
            integrated += sampleRBrushLocalField(
                largeField, {-4 + (x + 0.5) / 32.0, -4 + (y + 0.5) / 32.0, 0});
    require(integrated > 1000 && integrated < 65536,
            "large RBrush field covers a bounded portion of the projection grid");
    auto sampled = std::chrono::steady_clock::now();
    auto shaderFloatWeight = [&](float x, float y, float z)
    {
        const auto &s = largeField.source();
        if (std::abs(x) > 4 || std::abs(y) > 4)
            return 0.0f;
        auto u = std::clamp(x / 8 + 0.5f, 0.0f, 1.0f) * float(s.width - 1);
        auto v = std::clamp(y / 8 + 0.5f, 0.0f, 1.0f) * float(s.height - 1);
        auto x0 = std::uint32_t(std::floor(u)), y0 = std::uint32_t(std::floor(v));
        auto x1 = std::min(x0 + 1, s.width - 1), y1 = std::min(y0 + 1, s.height - 1);
        auto at = [&](std::uint32_t a, std::uint32_t b)
        { return largeField.sdf().signedDistanceTexels[b * s.width + a]; };
        float first = std::lerp(at(x0, y0), at(x1, y0), u - float(x0));
        float second = std::lerp(at(x0, y1), at(x1, y1), u - float(x0));
        float planarDistance = std::lerp(first, second, v - float(y0)) * (8.0f / s.width);
        auto smoothstep = [](float value)
        {
            value = std::clamp(value, 0.0f, 1.0f);
            return value * value * (3.0f - 2.0f * value);
        };
        float planar = planarDistance <= 0 ? 1.0f : 1.0f - smoothstep(planarDistance / 0.5f);
        float depthDistance = std::max(std::abs(z) - 1.0f, 0.0f);
        float depth = depthDistance == 0 ? 1.0f : 1.0f - smoothstep(depthDistance / 0.5f);
        return planar * depth;
    };
    double maximumFloatDelta = 0;
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x)
            for (float z : {0.0f, 1.25f})
            {
                float u = -4 + (x + 0.5f) / 32.0f;
                float v = -4 + (y + 0.5f) / 32.0f;
                maximumFloatDelta = std::max(maximumFloatDelta,
                    std::abs(double(shaderFloatWeight(u, v, z)) -
                             sampleRBrushLocalField(largeField, {u, v, z})));
            }
    require(maximumFloatDelta <= 1e-4,
            "shader-float local-field emulation diverges from the CPU double reference");
    std::cout << "RBrush shader-float emulation max weight delta " << maximumFloatDelta
              << " over 131072 point/depth samples\n";
    std::cout << "RBrush 257x257 derive ms " <<
        std::chrono::duration<double, std::milli>(derived - start).count() <<
        ", 65536 sample ms " <<
        std::chrono::duration<double, std::milli>(sampled - derived).count() << '\n';
    std::cout << "PASS canonical RBrush tip, Engine mask SDF and local-field projection\n";
    return 0;
}
catch (const std::exception &error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
