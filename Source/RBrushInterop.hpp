#pragma once
#include <Jerico/AssetPipeline/RBrushIR.hpp>
#include <Jerico/ProceduralAuthoring/MaskSignedDistanceField2D.hpp>
#include <Jerico/ProceduralAuthoring/Geology/GeologyMask.hpp>
#include <stop_token>
#include <utility>

namespace Playground
{
class RBrushLocalField;
RBrushLocalField makeRBrushLocalField(
    const Jerico::Tools::AssetPipeline::RBrushTipIR &tip,
    const Jerico::Tools::AssetPipeline::RBrushPresetIR &preset,
    Jerico::Guid assetGuid,
    Jerico::Tools::ProceduralAuthoring::GeologyTransform transform,
    double widthMeters, double heightMeters, double projectionDepthMeters,
    double falloffMeters, std::stop_token stop = {});
class RBrushLocalField
{
  public:
    const Jerico::Tools::ProceduralAuthoring::GeologyRBrushLocalFieldMaskSource &source() const noexcept
    { return source_; }
    const Jerico::Tools::ProceduralAuthoring::MaskSignedDistanceField2D &sdf() const noexcept
    { return sdf_; }

  private:
    friend RBrushLocalField makeRBrushLocalField(
        const Jerico::Tools::AssetPipeline::RBrushTipIR &,
        const Jerico::Tools::AssetPipeline::RBrushPresetIR &,
        Jerico::Guid, Jerico::Tools::ProceduralAuthoring::GeologyTransform,
        double, double, double, double, std::stop_token);
    RBrushLocalField(
        Jerico::Tools::ProceduralAuthoring::GeologyRBrushLocalFieldMaskSource source,
        Jerico::Tools::ProceduralAuthoring::MaskSignedDistanceField2D sdf)
        : source_(std::move(source)), sdf_(std::move(sdf)) {}
    Jerico::Tools::ProceduralAuthoring::GeologyRBrushLocalFieldMaskSource source_;
    Jerico::Tools::ProceduralAuthoring::MaskSignedDistanceField2D sdf_;
};
double sampleRBrushLocalField(const RBrushLocalField &field,
                              Jerico::Tools::ProceduralAuthoring::Vec3d world,
                              bool inverted = false);
Jerico::Tools::ProceduralAuthoring::MaskSignedDistanceField2D deriveRBrushTipSdf(
    const Jerico::Tools::AssetPipeline::RBrushTipIR &tip,
    const Jerico::Tools::AssetPipeline::RBrushPresetIR &preset,
    std::stop_token stop = {});
}
