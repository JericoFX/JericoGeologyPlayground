#include "RBrushInterop.hpp"
#include <Jerico/AssetPipeline/RBrushHash.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Playground
{
namespace
{
using Vec = Jerico::Tools::ProceduralAuthoring::Vec3d;
Vec cross(Vec a, Vec b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double smoothstep(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    return value * value * (3.0 - 2.0 * value);
}
}
RBrushLocalField makeRBrushLocalField(
    const Jerico::Tools::AssetPipeline::RBrushTipIR &tip,
    const Jerico::Tools::AssetPipeline::RBrushPresetIR &preset,
    Jerico::Guid assetGuid,
    Jerico::Tools::ProceduralAuthoring::GeologyTransform transform,
    double widthMeters, double heightMeters, double projectionDepthMeters,
    double falloffMeters, std::stop_token stop)
{
    namespace A = Jerico::Tools::AssetPipeline;
    namespace P = Jerico::Tools::ProceduralAuthoring;
    if (stop.stop_requested())
        throw std::runtime_error("RBrush local field cancelled");
    if (tip.pixelFormat != A::RBrushPixelFormat::R8Unorm || tip.tipGuid.isZero() ||
        tip.tipGuid != preset.primaryTipGuid || preset.presetGuid.isZero() || assetGuid.isZero())
        throw std::runtime_error("RBrush local field requires admitted scalar R8 identity");
    const auto hash = A::hashRBrushTip(tip.pixelFormat, tip.width, tip.height, tip.pixels);
    if (hash != tip.canonicalTipHash ||
        (preset.sourceInfo.canonicalTipHash != A::RBrushDigest{} &&
         preset.sourceInfo.canonicalTipHash != hash))
        throw std::runtime_error("RBrush local field canonical tip hash mismatch");
    P::GeologyRBrushLocalFieldMaskSource source{.assetGuid = assetGuid,
                                                .presetGuid = preset.presetGuid,
                                                .tipGuid = tip.tipGuid,
                                                .canonicalTipHash = hash,
                                                .width = tip.width,
                                                .height = tip.height,
                                                .coverageR8 = tip.pixels,
                                                .transform = transform,
                                                .widthMeters = widthMeters,
                                                .heightMeters = heightMeters,
                                                .projectionDepthMeters = projectionDepthMeters,
                                                .falloffMeters = falloffMeters};
    P::GeologyMask mask{.id = preset.presetGuid,
                        .name = "RBrush local field",
                        .nodes = {{.kind = P::GeologyMaskNodeKind::RBrushLocalField,
                                   .payload = source}}};
    if (!P::validateGeologyMask(mask).hasValue())
        throw std::runtime_error("RBrush local field failed Engine mask validation");
    auto derived = P::deriveMaskSignedDistanceField2D(
        {.width = tip.width,
         .height = tip.height,
         .format = P::MaskSdf2DPixelFormat::R8Unorm,
         .coverage = source.coverageR8,
         .settings = {.threshold = 0.5f}}, stop);
    if (!derived.hasValue())
        throw std::runtime_error("RBrush local field SDF derivation failed");
    return RBrushLocalField(std::move(source), std::move(derived.value()));
}
double sampleRBrushLocalField(const RBrushLocalField &field, Vec world, bool inverted)
{
    const auto &source = field.source();
    if (!std::isfinite(world.x) || !std::isfinite(world.y) || !std::isfinite(world.z))
        throw std::runtime_error("RBrush local field sample input invalid");
    Vec translated{world.x - source.transform.translation.x,
                   world.y - source.transform.translation.y,
                   world.z - source.transform.translation.z};
    Vec inverseAxis{-source.transform.rotation[0], -source.transform.rotation[1],
                    -source.transform.rotation[2]};
    auto doubledCross = cross(inverseAxis, translated);
    doubledCross = {2 * doubledCross.x, 2 * doubledCross.y, 2 * doubledCross.z};
    auto secondCross = cross(inverseAxis, doubledCross);
    Vec local{(translated.x + source.transform.rotation[3] * doubledCross.x + secondCross.x) /
                  source.transform.scale.x,
              (translated.y + source.transform.rotation[3] * doubledCross.y + secondCross.y) /
                  source.transform.scale.y,
              (translated.z + source.transform.rotation[3] * doubledCross.z + secondCross.z) /
                  source.transform.scale.z};
    if (!std::isfinite(local.x) || !std::isfinite(local.y) || !std::isfinite(local.z))
        throw std::runtime_error("RBrush local field transform overflowed");
    const double halfWidth = source.widthMeters * 0.5;
    const double halfHeight = source.heightMeters * 0.5;
    const double halfDepth = source.projectionDepthMeters * 0.5;
    double weight = 0;
    if (std::abs(local.x) <= halfWidth && std::abs(local.y) <= halfHeight)
    {
        double x = std::clamp(local.x / source.widthMeters + 0.5, 0.0, 1.0) *
                   (source.width - 1);
        double y = std::clamp(local.y / source.heightMeters + 0.5, 0.0, 1.0) *
                   (source.height - 1);
        auto x0 = std::uint32_t(std::floor(x)), y0 = std::uint32_t(std::floor(y));
        auto x1 = std::min(x0 + 1, source.width - 1);
        auto y1 = std::min(y0 + 1, source.height - 1);
        auto signedAt = [&](std::uint32_t u, std::uint32_t v)
        { return double(field.sdf().signedDistanceTexels[v * source.width + u]); };
        double first = std::lerp(signedAt(x0, y0), signedAt(x1, y0), x - x0);
        double second = std::lerp(signedAt(x0, y1), signedAt(x1, y1), x - x0);
        double distanceMeters = std::lerp(first, second, y - y0) *
                                std::min(source.widthMeters / source.width,
                                         source.heightMeters / source.height);
        if (!std::isfinite(distanceMeters))
            throw std::runtime_error("RBrush local field signed distance is not finite");
        double planar = distanceMeters <= 0 ? 1 : source.falloffMeters == 0 ? 0 :
                        1 - smoothstep(distanceMeters / source.falloffMeters);
        double beyondDepth = std::max(std::abs(local.z) - halfDepth, 0.0);
        double depth = beyondDepth == 0 ? 1 : source.falloffMeters == 0 ? 0 :
                       1 - smoothstep(beyondDepth / source.falloffMeters);
        weight = planar * depth;
    }
    return inverted ? 1 - weight : weight;
}
Jerico::Tools::ProceduralAuthoring::MaskSignedDistanceField2D deriveRBrushTipSdf(
    const Jerico::Tools::AssetPipeline::RBrushTipIR &tip,
    const Jerico::Tools::AssetPipeline::RBrushPresetIR &preset,
    std::stop_token stop)
{
    namespace A = Jerico::Tools::AssetPipeline;
    namespace P = Jerico::Tools::ProceduralAuthoring;
    if (tip.tipGuid.isZero() || tip.tipGuid != preset.primaryTipGuid || !preset.sdfDefaults.compatible)
        throw std::runtime_error("RBrush tip/preset identity or SDF admission invalid");
    P::MaskSdf2DPixelFormat format;
    switch (tip.pixelFormat)
    {
    case A::RBrushPixelFormat::R8Unorm:
        format = P::MaskSdf2DPixelFormat::R8Unorm;
        break;
    case A::RBrushPixelFormat::R16Unorm:
        format = P::MaskSdf2DPixelFormat::R16Unorm;
        break;
    default:
        throw std::runtime_error("RBrush tip must be a canonical scalar R8/R16 mask");
    }
    auto result = P::deriveMaskSignedDistanceField2D(
        {.width = tip.width,
         .height = tip.height,
         .format = format,
         .coverage = tip.pixels,
         .settings = {.threshold = preset.sdfDefaults.threshold,
                      .invert = preset.sdfDefaults.invert}},
        stop);
    if (!result.hasValue())
        throw std::runtime_error("Canonical RBrush mask SDF derivation failed");
    return std::move(result.value());
}
}
